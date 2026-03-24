#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

#define DEC_GLOBALS_H 1
#include "ack.h"
#include "db_worker.h"

/* -----------------------------------------------------------------------
 * Internal types
 * ----------------------------------------------------------------------- */

typedef struct db_request
{
   DB_OP_TYPE type;
   void *buf;                 /* serialised data blob (heap-allocated copy) */
   size_t len;                /* byte length of buf */
   char name[64];             /* player name for WRITE_PLAYER coalescing */
   struct descriptor_data *d; /* for DB_READ_PLAYER */
   struct db_request *next;
} DB_REQUEST;

/* -----------------------------------------------------------------------
 * Globals
 * ----------------------------------------------------------------------- */

int db_worker_failed = 0;

static pthread_t worker_thread;
static pthread_mutex_t req_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t req_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t res_mutex = PTHREAD_MUTEX_INITIALIZER;

static DB_REQUEST *req_head = NULL;
static DB_REQUEST *req_tail = NULL;

static DB_PLAYER_RESULT *res_head = NULL;

static char worker_connstr[512] = "";
static PGconn *worker_conn = NULL;

/* Consecutive failure counter for emergency fallback */
#define DB_MAX_FAILURES 3
static int consecutive_failures = 0;

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

static void post_result(struct descriptor_data *d, int error, int found, char *raw_save)
{
   DB_PLAYER_RESULT *r = calloc(1, sizeof(DB_PLAYER_RESULT));
   if (!r)
   {
      free(raw_save);
      return;
   }
   r->d = d;
   r->ch = NULL;
   r->error = error;
   r->found = found;
   r->raw_save = raw_save;
   r->next = NULL;
   pthread_mutex_lock(&res_mutex);
   r->next = res_head;
   res_head = r;
   pthread_mutex_unlock(&res_mutex);
}

/* Ensure the worker has an open connection.  Returns 1 on success. */
static int ensure_connected(void)
{
   if (worker_conn && PQstatus(worker_conn) == CONNECTION_OK)
      return 1;
   if (worker_conn)
   {
      PQfinish(worker_conn);
      worker_conn = NULL;
   }
   worker_conn = PQconnectdb(worker_connstr);
   if (PQstatus(worker_conn) != CONNECTION_OK)
   {
      fprintf(stderr, "DB worker: reconnect failed: %s\n", PQerrorMessage(worker_conn));
      PQfinish(worker_conn);
      worker_conn = NULL;
      return 0;
   }
   return 1;
}

/* -----------------------------------------------------------------------
 * Per-operation handlers (called on worker thread)
 * ----------------------------------------------------------------------- */

/* Execute a simple parameterised statement that returns no rows.
 * Returns 1 on success, 0 on error. */
static int worker_exec(const char *sql, int nParams, const char *const *params)
{
   PGresult *res;
   ExecStatusType status;
   int ok;

   if (!ensure_connected())
      return 0;

   res = PQexecParams(worker_conn, sql, nParams, NULL, params, NULL, NULL, 0);
   status = PQresultStatus(res);
   ok = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);
   if (!ok)
      fprintf(stderr, "DB worker exec error [%s]: %s\n", sql, PQresultErrorMessage(res));
   PQclear(res);
   return ok;
}

/* DB_WRITE_PLAYER — buf is the serialised player file text (same format as
 * the flat player file).  Extracts pwd_hash from the Password line and
 * upserts into players (name, pwd_hash, raw_save). */
static int handle_write_player(DB_REQUEST *req)
{
   const char *text = (const char *)req->buf;
   const char *p;
   char pwd[256] = "";
   const char *params[3];
   int i;

   /* Extract password hash: line format is "Password     HASH~\n" */
   p = strstr(text, "\nPassword     ");
   if (p)
   {
      p += strlen("\nPassword     ");
      for (i = 0; i < (int)sizeof(pwd) - 1 && *p && *p != '~' && *p != '\n'; i++)
         pwd[i] = *p++;
      pwd[i] = '\0';
   }

   params[0] = req->name;
   params[1] = pwd;
   params[2] = text;
   return worker_exec("INSERT INTO players (name, pwd_hash, raw_save) "
                      "VALUES ($1, $2, $3) "
                      "ON CONFLICT (name) DO UPDATE SET "
                      "pwd_hash = EXCLUDED.pwd_hash, "
                      "raw_save = EXCLUDED.raw_save",
                      3, params);
}

/* DB_READ_PLAYER — SELECT player row and post raw_save back to game thread.
 * The game thread hydrates the CHAR_DATA from raw_save in db_worker_poll_results. */
static void handle_read_player(DB_REQUEST *req)
{
   PGresult *res;
   const char *params[1];
   int nrows;

   if (!ensure_connected())
   {
      post_result(req->d, 1, 0, NULL);
      return;
   }

   params[0] = req->name;
   res = PQexecParams(worker_conn, "SELECT raw_save FROM players WHERE name = $1", 1, NULL, params,
                      NULL, NULL, 0);

   if (PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      fprintf(stderr, "DB worker: read_player query failed: %s\n", PQresultErrorMessage(res));
      PQclear(res);
      post_result(req->d, 1, 0, NULL);
      return;
   }

   nrows = PQntuples(res);

   if (nrows == 0)
   {
      PQclear(res);
      post_result(req->d, 0, 0, NULL); /* new player */
      return;
   }

   {
      const char *raw = PQgetvalue(res, 0, 0);
      char *raw_copy = (raw && raw[0]) ? strdup(raw) : NULL;
      PQclear(res);
      post_result(req->d, 0, 1, raw_copy);
   }
}

/* -----------------------------------------------------------------------
 * Serialisation helpers (for game-thread-side wrappers below)
 *
 * Format: fields delimited by \x1f (US), records by \x1e (RS).
 * Buffer is a plain heap string terminated by NUL.
 * ----------------------------------------------------------------------- */

#define SER_FS '\x1f' /* field separator  */
#define SER_RS '\x1e' /* record separator */

typedef struct
{
   char *buf;
   size_t len;
   size_t cap;
} SBuf;

static void sbuf_init(SBuf *s)
{
   s->buf = NULL;
   s->len = 0;
   s->cap = 0;
}

static void sbuf_grow(SBuf *s, size_t need)
{
   if (s->len + need + 1 <= s->cap)
      return;
   s->cap = (s->cap + need + 256) * 2;
   s->buf = realloc(s->buf, s->cap);
}

static void sbuf_putc(SBuf *s, char c)
{
   sbuf_grow(s, 1);
   s->buf[s->len++] = c;
   s->buf[s->len] = '\0';
}

static void sbuf_puts(SBuf *s, const char *str)
{
   size_t n = strlen(str ? str : "");
   sbuf_grow(s, n);
   memcpy(s->buf + s->len, str ? str : "", n);
   s->len += n;
   s->buf[s->len] = '\0';
}

static void sbuf_puti(SBuf *s, long v)
{
   char tmp[32];
   snprintf(tmp, sizeof(tmp), "%ld", v);
   sbuf_puts(s, tmp);
}

/* Mark end of current record. */
static void sbuf_rs(SBuf *s)
{
   sbuf_putc(s, SER_RS);
}
/* Field separator within a record. */
static void sbuf_fs(SBuf *s)
{
   sbuf_putc(s, SER_FS);
}

/* -----------------------------------------------------------------------
 * Deserialisation helpers (worker-thread side)
 * ----------------------------------------------------------------------- */

/* Advance *p past the current record, returning start of next or NULL. */
static const char *rec_next(const char *p)
{
   while (*p && *p != SER_RS)
      p++;
   return (*p == SER_RS) ? p + 1 : NULL;
}

/* Copy field starting at *p into out[out_sz], advance *p to next field or
 * end of record.  Returns 1 if more fields remain. */
static int rec_field(const char **p, char *out, size_t out_sz)
{
   const char *start = *p;
   const char *end = start;
   size_t n;

   while (*end && *end != SER_FS && *end != SER_RS)
      end++;

   n = (size_t)(end - start);
   if (n >= out_sz)
      n = out_sz - 1;
   memcpy(out, start, n);
   out[n] = '\0';

   if (*end == SER_FS)
   {
      *p = end + 1;
      return 1;
   }
   *p = end; /* stop at RS or NUL */
   return 0;
}

/* -----------------------------------------------------------------------
 * Write handler implementations
 * ----------------------------------------------------------------------- */

/* DB_WRITE_BANS — TRUNCATE + INSERT all bans.
 * Each record: ban_type \x1f address \x1f banned_by */
static int handle_write_bans(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   int ok = 1;

   if (!ensure_connected())
      return 0;

   PQclear(PQexec(worker_conn, "BEGIN"));
   PQclear(PQexec(worker_conn, "DELETE FROM bans"));

   while (p && *p && *p != '\0')
   {
      char ban_type_s[8], address[256], banned_by[256];
      const char *params[3];

      rec_field(&p, ban_type_s, sizeof(ban_type_s));
      rec_field(&p, address, sizeof(address));
      rec_field(&p, banned_by, sizeof(banned_by));

      params[0] = ban_type_s;
      params[1] = address;
      params[2] = banned_by;
      if (!worker_exec("INSERT INTO bans (ban_type, address, banned_by) "
                       "VALUES ($1,$2,$3)",
                       3, params))
         ok = 0;

      p = rec_next(p);
   }

   PQclear(PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK"));
   return ok;
}

/* DB_WRITE_SOCIALS — replace all socials.
 * Each record: name \x1f char_no_arg \x1f others_no_arg \x1f char_found \x1f
 *              others_found \x1f vict_found \x1f char_auto \x1f others_auto */
static int handle_write_socials(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   int ok = 1;

   if (!ensure_connected())
      return 0;

   PQclear(PQexec(worker_conn, "BEGIN"));
   PQclear(PQexec(worker_conn, "DELETE FROM socials"));

   while (p && *p && *p != '\0')
   {
      char name[128], cna[512], ona[512], cf[512], of_[512], vf[512], ca[512], oa[512];
      const char *params[8];

      rec_field(&p, name, sizeof(name));
      if (!name[0])
         break;
      rec_field(&p, cna, sizeof(cna));
      rec_field(&p, ona, sizeof(ona));
      rec_field(&p, cf, sizeof(cf));
      rec_field(&p, of_, sizeof(of_));
      rec_field(&p, vf, sizeof(vf));
      rec_field(&p, ca, sizeof(ca));
      rec_field(&p, oa, sizeof(oa));

      params[0] = name;
      params[1] = cna;
      params[2] = ona;
      params[3] = cf;
      params[4] = of_;
      params[5] = vf;
      params[6] = ca;
      params[7] = oa;
      if (!worker_exec("INSERT INTO socials "
                       "(name,char_no_arg,others_no_arg,char_found,others_found,"
                       "vict_found,char_auto,others_auto) "
                       "VALUES ($1,$2,$3,$4,$5,$6,$7,$8) "
                       "ON CONFLICT (name) DO UPDATE SET "
                       "char_no_arg=EXCLUDED.char_no_arg,"
                       "others_no_arg=EXCLUDED.others_no_arg,"
                       "char_found=EXCLUDED.char_found,"
                       "others_found=EXCLUDED.others_found,"
                       "vict_found=EXCLUDED.vict_found,"
                       "char_auto=EXCLUDED.char_auto,"
                       "others_auto=EXCLUDED.others_auto",
                       8, params))
         ok = 0;

      p = rec_next(p);
   }

   PQclear(PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK"));
   return ok;
}

/* DB_WRITE_CLANS — update diplomacy + treasury.
 * Each record: clan_id \x1f treasury \x1f dip_0 \x1f dip_1 \x1f ... */
static int handle_write_clans(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   int ok = 1;

   if (!ensure_connected())
      return 0;

   PQclear(PQexec(worker_conn, "BEGIN"));

   while (p && *p && *p != '\0')
   {
      char clan_id_s[8], treasury_s[32], gold_s[32];
      char war_matrix[256];
      int i;
      const char *params[4];
      char dip[MAX_CLAN][16];

      rec_field(&p, clan_id_s, sizeof(clan_id_s));
      if (!clan_id_s[0])
         break;
      rec_field(&p, treasury_s, sizeof(treasury_s));
      rec_field(&p, gold_s, sizeof(gold_s));

      /* Read per-clan diplomacy values */
      war_matrix[0] = '\0';
      {
         char tmp[32];
         char arr[256];
         arr[0] = '\0';
         for (i = 0; i < MAX_CLAN; i++)
         {
            if (*p && *p != SER_RS)
               rec_field(&p, dip[i], sizeof(dip[i]));
            else
               strncpy(dip[i], "0", sizeof(dip[i]));
            snprintf(tmp, sizeof(tmp), "%s%s", (i == 0 ? "" : ","), dip[i]);
            strncat(arr, tmp, sizeof(arr) - strlen(arr) - 1);
         }
         snprintf(war_matrix, sizeof(war_matrix), "{%s}", arr);
      }

      params[0] = clan_id_s;
      params[1] = treasury_s;
      params[2] = gold_s;
      params[3] = war_matrix;
      if (!worker_exec("INSERT INTO clans (id, gold, member_count, war_matrix) "
                       "VALUES ($1,$3,0,$4) "
                       "ON CONFLICT (id) DO UPDATE SET "
                       "gold=EXCLUDED.gold, war_matrix=EXCLUDED.war_matrix",
                       4, params))
         ok = 0;

      p = rec_next(p);
   }

   PQclear(PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK"));
   return ok;
}

/* DB_WRITE_SYSDATA — update the single sysdata row.
 * Record: w_lock \x1f shownumbers */
static int handle_write_sysdata(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char wl[4], sn[4];
   const char *params[2];

   if (!ensure_connected())
      return 0;

   rec_field(&p, wl, sizeof(wl));
   rec_field(&p, sn, sizeof(sn));

   params[0] = wl;
   params[1] = sn;
   return worker_exec("UPDATE sysdata SET bln_val_0=$1::boolean, bln_val_1=$2::boolean "
                      "WHERE id=1",
                      2, params);
}

/* DB_WRITE_RULERS — replace all rulers.
 * Each record: name */
static int handle_write_rulers(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   int ok = 1;

   if (!ensure_connected())
      return 0;

   PQclear(PQexec(worker_conn, "BEGIN"));
   PQclear(PQexec(worker_conn, "DELETE FROM rulers"));

   while (p && *p && *p != '\0')
   {
      char name[128];
      const char *params[1];

      rec_field(&p, name, sizeof(name));
      if (!name[0])
         break;

      params[0] = name;
      if (!worker_exec("INSERT INTO rulers (name) VALUES ($1) "
                       "ON CONFLICT (name) DO NOTHING",
                       1, params))
         ok = 0;

      p = rec_next(p);
   }

   PQclear(PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK"));
   return ok;
}

/* DB_WRITE_BRANDS — replace all brands.
 * Each record: branded_by \x1f item_name \x1f brand_date \x1f description */
static int handle_write_brands(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   int ok = 1;

   if (!ensure_connected())
      return 0;

   PQclear(PQexec(worker_conn, "BEGIN"));
   PQclear(PQexec(worker_conn, "DELETE FROM brands"));

   while (p && *p && *p != '\0')
   {
      char branded_by[128], item_name[512], brand_date[64], description[512];
      const char *params[4];

      rec_field(&p, branded_by, sizeof(branded_by));
      if (!branded_by[0])
         break;
      rec_field(&p, item_name, sizeof(item_name));
      rec_field(&p, brand_date, sizeof(brand_date));
      rec_field(&p, description, sizeof(description));

      params[0] = branded_by;
      params[1] = item_name;
      params[2] = brand_date;
      params[3] = description;
      if (!worker_exec("INSERT INTO brands (branded_by, item_name, brand_date, description) "
                       "VALUES ($1,$2,$3,$4)",
                       4, params))
         ok = 0;

      p = rec_next(p);
   }

   PQclear(PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK"));
   return ok;
}

/* DB_WRITE_ROOM_MARKS — replace all room marks.
 * Each record: room_vnum \x1f mark_text */
static int handle_write_room_marks(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   int ok = 1;

   if (!ensure_connected())
      return 0;

   PQclear(PQexec(worker_conn, "BEGIN"));
   PQclear(PQexec(worker_conn, "DELETE FROM room_marks"));

   while (p && *p && *p != '\0')
   {
      char room_vnum_s[16], mark_text[1024];
      const char *params[2];

      rec_field(&p, room_vnum_s, sizeof(room_vnum_s));
      if (!room_vnum_s[0])
         break;
      rec_field(&p, mark_text, sizeof(mark_text));

      params[0] = room_vnum_s;
      params[1] = mark_text;
      if (!worker_exec("INSERT INTO room_marks (room_vnum, mark_text) VALUES ($1,$2)", 2, params))
         ok = 0;

      p = rec_next(p);
   }

   PQclear(PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK"));
   return ok;
}

/* DB_WRITE_CORPSES — not yet fully implemented (schema populated by import only). */
static int handle_write_corpses(DB_REQUEST *req)
{
   (void)req;
   return 1;
}

/* Chest serialisation format (SBuf / SER_RS / SER_FS):
 *
 *  Record 0 (chest header):
 *    vnum FS owner_name FS max_items RS
 *
 *  Records 1…N (items in DFS pre-order, parent before children):
 *    sort_order FS parent_sort FS obj_vnum FS name FS short_descr FS
 *    description FS extra_flags FS wear_flags FS wear_loc FS
 *    class_flags FS item_type FS weight FS level FS timer FS cost FS
 *    v0 FS v1 FS v2 FS v3 FS v4 FS v5 FS v6 FS v7 FS v8 FS v9 FS
 *    objfun RS
 *
 *  parent_sort == -1 means direct child of the chest (nest==1).
 */
static int handle_write_chest(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char f[3][1024];
   PGresult *res;
   int chest_vnum, max_items;
   const char *cv[32];
   char vbuf[32], mibuf[32];
   char chest_id_s[32];

   if (!p || req->len == 0)
      return 0;
   if (!ensure_connected())
      return 0;

   /* --- Parse header record ------------------------------------------- */
   if (!rec_field(&p, f[0], sizeof(f[0])))
      return 0; /* vnum */
   if (!rec_field(&p, f[1], sizeof(f[1])))
      return 0; /* owner_name */
   if (!rec_field(&p, f[2], sizeof(f[2])))
      return 0; /* max_items */
   p = rec_next(p);

   chest_vnum = atoi(f[0]);
   max_items = atoi(f[2]);

   /* --- BEGIN transaction -------------------------------------------- */
   res = PQexec(worker_conn, "BEGIN");
   if (PQresultStatus(res) != PGRES_COMMAND_OK)
   {
      fprintf(stderr, "DB worker chest: BEGIN failed: %s\n", PQerrorMessage(worker_conn));
      PQclear(res);
      return 0;
   }
   PQclear(res);

   /* --- Upsert keep_chests row --------------------------------------- */
   snprintf(vbuf, sizeof(vbuf), "%d", chest_vnum);
   snprintf(mibuf, sizeof(mibuf), "%d", max_items);
   cv[0] = vbuf;
   cv[1] = f[1]; /* owner_name */
   cv[2] = mibuf;
   res = PQexecParams(worker_conn,
                      "INSERT INTO keep_chests (vnum, owner_name, max_items)"
                      " VALUES ($1, $2, $3)"
                      " ON CONFLICT (vnum) DO UPDATE"
                      "   SET owner_name=$2, max_items=$3, updated_at=NOW()"
                      " RETURNING id",
                      3, NULL, cv, NULL, NULL, 0);
   if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
   {
      fprintf(stderr, "DB worker chest: upsert keep_chests failed: %s\n",
              PQerrorMessage(worker_conn));
      PQclear(res);
      PQexec(worker_conn, "ROLLBACK");
      return 0;
   }
   snprintf(chest_id_s, sizeof(chest_id_s), "%s", PQgetvalue(res, 0, 0));
   PQclear(res);

   /* --- Delete existing items for this chest ------------------------- */
   cv[0] = chest_id_s;
   res = PQexecParams(worker_conn, "DELETE FROM keep_chest_items WHERE chest_id=$1", 1, NULL, cv,
                      NULL, NULL, 0);
   if (PQresultStatus(res) != PGRES_COMMAND_OK)
   {
      fprintf(stderr, "DB worker chest: DELETE items failed: %s\n", PQerrorMessage(worker_conn));
      PQclear(res);
      PQexec(worker_conn, "ROLLBACK");
      return 0;
   }
   PQclear(res);

   /* Track sort_order → generated DB id mapping for parent lookups */
#define CHEST_SORT_MAP_SIZE 512
   int map_sort[CHEST_SORT_MAP_SIZE];
   int map_id[CHEST_SORT_MAP_SIZE];
   int map_n = 0;

   /* --- Insert item records ------------------------------------------ */
   while (*p)
   {
      char sort_s[32], parent_sort_s[32], obj_vnum_s[32];
      char item_name[1024], item_short[1024], item_desc[4096];
      char eflags_s[32], wflags_s[32], wloc_s[32], cflags_s[32];
      char itype_s[32], weight_s[32], level_s[32], timer_s[32], cost_s[32];
      char v[10][32], objfun_s[256];
      int vi, parent_sort, parent_db_id_str_found;
      char parent_db_id_s[32];

      if (!rec_field(&p, sort_s, sizeof(sort_s)))
         break;
      if (!rec_field(&p, parent_sort_s, sizeof(parent_sort_s)))
         break;
      if (!rec_field(&p, obj_vnum_s, sizeof(obj_vnum_s)))
         break;
      if (!rec_field(&p, item_name, sizeof(item_name)))
         break;
      if (!rec_field(&p, item_short, sizeof(item_short)))
         break;
      if (!rec_field(&p, item_desc, sizeof(item_desc)))
         break;
      if (!rec_field(&p, eflags_s, sizeof(eflags_s)))
         break;
      if (!rec_field(&p, wflags_s, sizeof(wflags_s)))
         break;
      if (!rec_field(&p, wloc_s, sizeof(wloc_s)))
         break;
      if (!rec_field(&p, cflags_s, sizeof(cflags_s)))
         break;
      if (!rec_field(&p, itype_s, sizeof(itype_s)))
         break;
      if (!rec_field(&p, weight_s, sizeof(weight_s)))
         break;
      if (!rec_field(&p, level_s, sizeof(level_s)))
         break;
      if (!rec_field(&p, timer_s, sizeof(timer_s)))
         break;
      if (!rec_field(&p, cost_s, sizeof(cost_s)))
         break;
      for (vi = 0; vi < 10; vi++)
         if (!rec_field(&p, v[vi], sizeof(v[vi])))
            break;
      if (!rec_field(&p, objfun_s, sizeof(objfun_s)))
         break;
      p = rec_next(p);

      /* Resolve parent sort_order → DB id */
      parent_sort = atoi(parent_sort_s);
      parent_db_id_str_found = 0;
      snprintf(parent_db_id_s, sizeof(parent_db_id_s), "NULL");

      if (parent_sort >= 0)
      {
         int ki;
         for (ki = 0; ki < map_n; ki++)
            if (map_sort[ki] == parent_sort)
            {
               snprintf(parent_db_id_s, sizeof(parent_db_id_s), "%d", map_id[ki]);
               parent_db_id_str_found = 1;
               break;
            }
         if (!parent_db_id_str_found)
            fprintf(stderr, "DB worker chest: parent sort %d not found for sort %s\n", parent_sort,
                    sort_s);
      }

      /* Build INSERT */
      cv[0] = chest_id_s;
      cv[1] = parent_db_id_str_found ? parent_db_id_s : NULL;
      cv[2] = item_name;
      cv[3] = item_short;
      cv[4] = item_desc;
      cv[5] = obj_vnum_s;
      cv[6] = eflags_s;
      cv[7] = wflags_s;
      cv[8] = wloc_s;
      cv[9] = cflags_s;
      cv[10] = itype_s;
      cv[11] = weight_s;
      cv[12] = level_s;
      cv[13] = timer_s;
      cv[14] = cost_s;
      cv[15] = v[0];
      cv[16] = v[1];
      cv[17] = v[2];
      cv[18] = v[3];
      cv[19] = v[4];
      cv[20] = v[5];
      cv[21] = v[6];
      cv[22] = v[7];
      cv[23] = v[8];
      cv[24] = v[9];
      cv[25] = objfun_s[0] ? objfun_s : NULL;
      cv[26] = sort_s;

      res = PQexecParams(worker_conn,
                         "INSERT INTO keep_chest_items"
                         " (chest_id, parent_id, name, short_descr, description,"
                         "  vnum, extra_flags, wear_flags, wear_loc, class_flags,"
                         "  item_type, weight, level, timer, cost,"
                         "  value_0, value_1, value_2, value_3, value_4,"
                         "  value_5, value_6, value_7, value_8, value_9,"
                         "  objfun, sort_order)"
                         " VALUES"
                         " ($1, $2, $3, $4, $5,"
                         "  $6, $7, $8, $9, $10,"
                         "  $11, $12, $13, $14, $15,"
                         "  $16, $17, $18, $19, $20,"
                         "  $21, $22, $23, $24, $25,"
                         "  $26, $27)"
                         " RETURNING id",
                         27, NULL, cv, NULL, NULL, 0);
      if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
      {
         fprintf(stderr, "DB worker chest: INSERT item failed: %s\n", PQerrorMessage(worker_conn));
         PQclear(res);
         PQexec(worker_conn, "ROLLBACK");
         return 0;
      }

      /* Record the mapping sort_order → DB id */
      if (map_n < CHEST_SORT_MAP_SIZE)
      {
         map_sort[map_n] = atoi(sort_s);
         map_id[map_n] = atoi(PQgetvalue(res, 0, 0));
         map_n++;
      }
      PQclear(res);
   }
#undef CHEST_SORT_MAP_SIZE

   res = PQexec(worker_conn, "COMMIT");
   if (PQresultStatus(res) != PGRES_COMMAND_OK)
   {
      fprintf(stderr, "DB worker chest: COMMIT failed: %s\n", PQerrorMessage(worker_conn));
      PQclear(res);
      PQexec(worker_conn, "ROLLBACK");
      return 0;
   }
   PQclear(res);
   return 1;
}

/* -----------------------------------------------------------------------
 * OLC / area write handlers
 * ----------------------------------------------------------------------- */

/* DB_WRITE_AREA — upsert one area's metadata.
 * Record: area_num FS min_vnum FS max_vnum FS name FS keyword FS level_label FS
 *         min_level FS max_level FS offset FS reset_rate FS reset_msg FS
 *         owner FS can_read FS can_write FS music FS flags RS */
static int handle_write_area(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char area_num[16], min_vnum[16], max_vnum[16];
   char name[512], keyword[256], level_label[256];
   char min_level[16], max_level[16], offset[16];
   char reset_rate[16], reset_msg[512];
   char owner[256], can_read[256], can_write[256];
   char music[256], flags[16];
   const char *params[15];

   if (!ensure_connected())
      return 0;

   rec_field(&p, area_num, sizeof(area_num));
   rec_field(&p, min_vnum, sizeof(min_vnum));
   rec_field(&p, max_vnum, sizeof(max_vnum));
   rec_field(&p, name, sizeof(name));
   rec_field(&p, keyword, sizeof(keyword));
   rec_field(&p, level_label, sizeof(level_label));
   rec_field(&p, min_level, sizeof(min_level));
   rec_field(&p, max_level, sizeof(max_level));
   rec_field(&p, offset, sizeof(offset));
   rec_field(&p, reset_rate, sizeof(reset_rate));
   rec_field(&p, reset_msg, sizeof(reset_msg));
   rec_field(&p, owner, sizeof(owner));
   rec_field(&p, can_read, sizeof(can_read));
   rec_field(&p, can_write, sizeof(can_write));
   rec_field(&p, music, sizeof(music));
   rec_field(&p, flags, sizeof(flags));

   params[0] = area_num;
   params[1] = name;
   params[2] = keyword;
   params[3] = level_label;
   params[4] = min_level;
   params[5] = max_level;
   params[6] = offset;
   params[7] = reset_rate;
   params[8] = reset_msg;
   params[9] = owner[0] ? owner : NULL;
   params[10] = can_read[0] ? can_read : NULL;
   params[11] = can_write[0] ? can_write : NULL;
   params[12] = music[0] ? music : NULL;
   params[13] = flags;
   params[14] = min_vnum;

   return worker_exec("UPDATE areas SET "
                      "area_number=$1, name=$2, keyword=$3, level_label=$4, "
                      "level_min=$5, level_max=$6, map_offset=$7, reset_rate=$8, "
                      "reset_msg=$9, owner=$10, can_read=$11, can_write=$12, "
                      "music=$13, flags=$14 "
                      "WHERE min_vnum=$15",
                      15, params);
}

/* DB_WRITE_ROOM — upsert one room.
 * Record 0: "R" FS area_min_vnum FS vnum FS name FS description FS room_flags FS sector_type RS
 * Record N (exits): "X" FS dir FS desc FS keyword FS exit_flags FS key FS to_vnum RS
 * Record N (exdesc): "E" FS keyword FS description RS */
static int handle_write_room(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char type_c[4], area_min[16], vnum_s[16], name[512], desc[4096], rf[16], sec[16];
   const char *params[7];
   PGresult *res;
   int ok = 1;

   if (!ensure_connected())
      return 0;

   /* Parse room header record */
   rec_field(&p, type_c, sizeof(type_c)); /* "R" */
   rec_field(&p, area_min, sizeof(area_min));
   rec_field(&p, vnum_s, sizeof(vnum_s));
   rec_field(&p, name, sizeof(name));
   rec_field(&p, desc, sizeof(desc));
   rec_field(&p, rf, sizeof(rf));
   rec_field(&p, sec, sizeof(sec));
   p = rec_next(p);

   PQclear(PQexec(worker_conn, "BEGIN"));

   /* Upsert room row */
   params[0] = vnum_s;
   params[1] = area_min;
   params[2] = name;
   params[3] = desc;
   params[4] = rf;
   params[5] = sec;
   if (!worker_exec("INSERT INTO rooms (vnum, area_id, name, description, room_flags, sector_type)"
                    " VALUES ($1,(SELECT id FROM areas WHERE min_vnum<=$1::int AND "
                    "max_vnum>=$1::int),$3,$4,$5,$6)"
                    " ON CONFLICT (vnum) DO UPDATE SET"
                    " name=EXCLUDED.name, description=EXCLUDED.description,"
                    " room_flags=EXCLUDED.room_flags, sector_type=EXCLUDED.sector_type",
                    6, params))
      ok = 0;

   if (ok)
   {
      /* Delete old exits and extra descs */
      params[0] = vnum_s;
      worker_exec("DELETE FROM room_exits WHERE room_vnum=$1", 1, params);
      worker_exec("DELETE FROM room_extra_descs WHERE room_vnum=$1", 1, params);

      /* Insert new exits and extra descs */
      while (p && *p)
      {
         char t[4];
         rec_field(&p, t, sizeof(t));
         if (t[0] == 'X')
         {
            char dir[8], edesc[2048], ekw[256], eflags[16], ekey[16], evnum[16];
            rec_field(&p, dir, sizeof(dir));
            rec_field(&p, edesc, sizeof(edesc));
            rec_field(&p, ekw, sizeof(ekw));
            rec_field(&p, eflags, sizeof(eflags));
            rec_field(&p, ekey, sizeof(ekey));
            rec_field(&p, evnum, sizeof(evnum));
            p = rec_next(p);

            params[0] = vnum_s;
            params[1] = dir;
            params[2] = evnum[0] ? evnum : NULL;
            params[3] = eflags;
            params[4] = ekey[0] ? ekey : NULL;
            params[5] = ekw[0] ? ekw : NULL;
            params[6] = edesc[0] ? edesc : NULL;
            if (!worker_exec(
                    "INSERT INTO room_exits"
                    " (room_vnum,direction,dest_vnum,exit_flags,key_vnum,keyword,description)"
                    " VALUES ($1,$2,$3,$4,$5,$6,$7)",
                    7, params))
               ok = 0;
         }
         else if (t[0] == 'E')
         {
            char ekw[512], edesc[4096];
            rec_field(&p, ekw, sizeof(ekw));
            rec_field(&p, edesc, sizeof(edesc));
            p = rec_next(p);

            params[0] = vnum_s;
            params[1] = ekw;
            params[2] = edesc;
            if (!worker_exec("INSERT INTO room_extra_descs (room_vnum,keyword,description)"
                             " VALUES ($1,$2,$3)",
                             3, params))
               ok = 0;
         }
         else
         {
            p = rec_next(p);
         }
      }
   }

   res = PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK");
   PQclear(res);
   return ok;
}

/* DB_WRITE_MOB — upsert one mob prototype.
 * Single record with all fields. */
static int handle_write_mob(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char area_min[16], vnum_s[16];
   char player_name[512], short_descr[512], long_descr[512], desc[4096];
   char act_flags[32], affected_by[16], alignment[16], level[16], sex[16];
   char hp_mod[16], ac_mod[16], hr_mod[16], dr_mod[16];
   char cls[8], clan[8], race[8], position[8], skills[16], cast_flags[16], def[16];
   char strong_magic[16], weak_magic[16], race_mods[16], power_skills[16], power_cast[16];
   char resist[16], suscept[16];
   char spellpower[16], crit[16], crit_mult[16], spell_crit[16], spell_mult[16];
   char parry[16], dodge[16], block[16], pierce[16];
   char ai_knowledge[16], accent[16], ai_prompt[2048];
   char loot_amount[16], loot[9][16], loot_chance[9][16], lore_flags[32];
   const char *params[56];
   int i;

   if (!ensure_connected())
      return 0;

   rec_field(&p, area_min, sizeof(area_min));
   rec_field(&p, vnum_s, sizeof(vnum_s));
   rec_field(&p, player_name, sizeof(player_name));
   rec_field(&p, short_descr, sizeof(short_descr));
   rec_field(&p, long_descr, sizeof(long_descr));
   rec_field(&p, desc, sizeof(desc));
   rec_field(&p, act_flags, sizeof(act_flags));
   rec_field(&p, affected_by, sizeof(affected_by));
   rec_field(&p, alignment, sizeof(alignment));
   rec_field(&p, level, sizeof(level));
   rec_field(&p, sex, sizeof(sex));
   rec_field(&p, hp_mod, sizeof(hp_mod));
   rec_field(&p, ac_mod, sizeof(ac_mod));
   rec_field(&p, hr_mod, sizeof(hr_mod));
   rec_field(&p, dr_mod, sizeof(dr_mod));
   rec_field(&p, cls, sizeof(cls));
   rec_field(&p, clan, sizeof(clan));
   rec_field(&p, race, sizeof(race));
   rec_field(&p, position, sizeof(position));
   rec_field(&p, skills, sizeof(skills));
   rec_field(&p, cast_flags, sizeof(cast_flags));
   rec_field(&p, def, sizeof(def));
   rec_field(&p, strong_magic, sizeof(strong_magic));
   rec_field(&p, weak_magic, sizeof(weak_magic));
   rec_field(&p, race_mods, sizeof(race_mods));
   rec_field(&p, power_skills, sizeof(power_skills));
   rec_field(&p, power_cast, sizeof(power_cast));
   rec_field(&p, resist, sizeof(resist));
   rec_field(&p, suscept, sizeof(suscept));
   rec_field(&p, spellpower, sizeof(spellpower));
   rec_field(&p, crit, sizeof(crit));
   rec_field(&p, crit_mult, sizeof(crit_mult));
   rec_field(&p, spell_crit, sizeof(spell_crit));
   rec_field(&p, spell_mult, sizeof(spell_mult));
   rec_field(&p, parry, sizeof(parry));
   rec_field(&p, dodge, sizeof(dodge));
   rec_field(&p, block, sizeof(block));
   rec_field(&p, pierce, sizeof(pierce));
   rec_field(&p, ai_knowledge, sizeof(ai_knowledge));
   rec_field(&p, accent, sizeof(accent));
   rec_field(&p, ai_prompt, sizeof(ai_prompt));
   rec_field(&p, loot_amount, sizeof(loot_amount));
   for (i = 0; i < 9; i++)
      rec_field(&p, loot[i], sizeof(loot[i]));
   for (i = 0; i < 9; i++)
      rec_field(&p, loot_chance[i], sizeof(loot_chance[i]));
   rec_field(&p, lore_flags, sizeof(lore_flags));

   params[0] = vnum_s;
   params[1] = area_min;
   params[2] = player_name;
   params[3] = short_descr;
   params[4] = long_descr;
   params[5] = desc;
   params[6] = act_flags;
   params[7] = affected_by;
   params[8] = alignment;
   params[9] = level;
   params[10] = sex;
   params[11] = hp_mod;
   params[12] = ac_mod;
   params[13] = hr_mod;
   params[14] = dr_mod;
   params[15] = cls;
   params[16] = clan;
   params[17] = race;
   params[18] = position;
   params[19] = skills;
   params[20] = cast_flags;
   params[21] = def;
   params[22] = strong_magic;
   params[23] = weak_magic;
   params[24] = race_mods;
   params[25] = power_skills;
   params[26] = power_cast;
   params[27] = resist;
   params[28] = suscept;
   params[29] = spellpower;
   params[30] = crit;
   params[31] = crit_mult;
   params[32] = spell_crit;
   params[33] = spell_mult;
   params[34] = parry;
   params[35] = dodge;
   params[36] = block;
   params[37] = pierce;
   params[38] = ai_knowledge;
   params[39] = accent;
   params[40] = ai_prompt[0] ? ai_prompt : NULL;
   params[41] = loot_amount;
   for (i = 0; i < 9; i++)
      params[42 + i] = loot[i];
   for (i = 0; i < 9; i++)
      params[51 + i] = loot_chance[i];
   /* params[51..59] used; total = 60 but only 51+9=60 params */

   return worker_exec(
       "INSERT INTO mobiles"
       " (vnum,area_id,player_name,short_descr,long_descr,description,"
       "  act_flags,affected_by,alignment,level,sex,"
       "  hp_mod,ac_mod,hr_mod,dr_mod,"
       "  class,clan,race,position,skills,cast_flags,def,"
       "  strong_magic,weak_magic,race_mods,power_skills,power_cast,"
       "  resist,suscept,spellpower,crit,crit_mult,spell_crit,spell_mult,"
       "  parry,dodge,block,pierce,ai_knowledge,accent,ai_prompt,"
       "  loot_amount,loot_0,loot_1,loot_2,loot_3,loot_4,loot_5,loot_6,loot_7,loot_8,"
       "  loot_chance_0,loot_chance_1,loot_chance_2,loot_chance_3,loot_chance_4,"
       "  loot_chance_5,loot_chance_6,loot_chance_7,loot_chance_8)"
       " VALUES"
       " ($1,(SELECT id FROM areas WHERE min_vnum<=$1::int AND max_vnum>=$1::int),$3,$4,$5,$6,"
       "  $7,$8,$9,$10,$11,"
       "  $12,$13,$14,$15,"
       "  $16,$17,$18,$19,$20,$21,$22,"
       "  $23,$24,$25,$26,$27,"
       "  $28,$29,$30,$31,$32,$33,$34,"
       "  $35,$36,$37,$38,$39,$40,$41,"
       "  $42,$43,$44,$45,$46,$47,$48,$49,$50,$51,"
       "  $52,$53,$54,$55,$56,$57,$58,$59,$60)"
       " ON CONFLICT (vnum) DO UPDATE SET"
       "  area_id=(SELECT id FROM areas WHERE min_vnum<=$1::int AND max_vnum>=$1::int),"
       "  player_name=$3,short_descr=$4,long_descr=$5,description=$6,"
       "  act_flags=$7,affected_by=$8,alignment=$9,level=$10,sex=$11,"
       "  hp_mod=$12,ac_mod=$13,hr_mod=$14,dr_mod=$15,"
       "  class=$16,clan=$17,race=$18,position=$19,skills=$20,cast_flags=$21,def=$22,"
       "  strong_magic=$23,weak_magic=$24,race_mods=$25,power_skills=$26,power_cast=$27,"
       "  resist=$28,suscept=$29,spellpower=$30,crit=$31,crit_mult=$32,"
       "  spell_crit=$33,spell_mult=$34,parry=$35,dodge=$36,block=$37,pierce=$38,"
       "  ai_knowledge=$39,accent=$40,ai_prompt=$41,"
       "  loot_amount=$42,loot_0=$43,loot_1=$44,loot_2=$45,loot_3=$46,loot_4=$47,"
       "  loot_5=$48,loot_6=$49,loot_7=$50,loot_8=$51,"
       "  loot_chance_0=$52,loot_chance_1=$53,loot_chance_2=$54,loot_chance_3=$55,"
       "  loot_chance_4=$56,loot_chance_5=$57,loot_chance_6=$58,loot_chance_7=$59,"
       "  loot_chance_8=$60",
       60, params);
}

/* DB_WRITE_OBJ — upsert one object prototype.
 * Record 0: "O" FS area_min FS vnum FS name FS short_descr FS description FS
 *            item_type FS extra_flags FS wear_flags FS item_apply FS v[0..9] FS weight FS level RS
 * Record N (affect): "A" FS location FS modifier RS
 * Record N (exdesc): "E" FS keyword FS description RS */
static int handle_write_obj(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char type_c[4], area_min[16], vnum_s[16];
   char name[512], short_descr[512], desc[4096];
   char item_type[16], extra_flags[32], wear_flags[16], item_apply[16];
   char val[10][32], weight[16], level[16];
   const char *params[21];
   PGresult *res;
   int ok = 1;
   int i;

   if (!ensure_connected())
      return 0;

   rec_field(&p, type_c, sizeof(type_c)); /* "O" */
   rec_field(&p, area_min, sizeof(area_min));
   rec_field(&p, vnum_s, sizeof(vnum_s));
   rec_field(&p, name, sizeof(name));
   rec_field(&p, short_descr, sizeof(short_descr));
   rec_field(&p, desc, sizeof(desc));
   rec_field(&p, item_type, sizeof(item_type));
   rec_field(&p, extra_flags, sizeof(extra_flags));
   rec_field(&p, wear_flags, sizeof(wear_flags));
   rec_field(&p, item_apply, sizeof(item_apply));
   for (i = 0; i < 10; i++)
      rec_field(&p, val[i], sizeof(val[i]));
   rec_field(&p, weight, sizeof(weight));
   rec_field(&p, level, sizeof(level));
   p = rec_next(p);

   PQclear(PQexec(worker_conn, "BEGIN"));

   params[0] = vnum_s;
   params[1] = area_min;
   params[2] = name;
   params[3] = short_descr;
   params[4] = desc;
   params[5] = item_type;
   params[6] = extra_flags;
   params[7] = wear_flags;
   params[8] = item_apply;
   for (i = 0; i < 10; i++)
      params[9 + i] = val[i];
   params[19] = weight;
   params[20] = level;

   if (!worker_exec(
           "INSERT INTO objects"
           " (vnum,area_id,name,short_descr,description,item_type,extra_flags,"
           "  wear_flags,item_apply,value_0,value_1,value_2,value_3,value_4,"
           "  value_5,value_6,value_7,value_8,value_9,weight,level)"
           " VALUES"
           " ($1,(SELECT id FROM areas WHERE min_vnum<=$1::int AND max_vnum>=$1::int),"
           "  $3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$20,$21)"
           " ON CONFLICT (vnum) DO UPDATE SET"
           "  area_id=(SELECT id FROM areas WHERE min_vnum<=$1::int AND max_vnum>=$1::int),"
           "  name=$3,short_descr=$4,description=$5,item_type=$6,extra_flags=$7,"
           "  wear_flags=$8,item_apply=$9,"
           "  value_0=$10,value_1=$11,value_2=$12,value_3=$13,value_4=$14,"
           "  value_5=$15,value_6=$16,value_7=$17,value_8=$18,value_9=$19,"
           "  weight=$20,level=$21",
           21, params))
      ok = 0;

   if (ok)
   {
      const char *pv[3];
      pv[0] = vnum_s;
      worker_exec("DELETE FROM object_affects WHERE obj_vnum=$1", 1, pv);
      worker_exec("DELETE FROM object_extra_descs WHERE obj_vnum=$1", 1, pv);

      while (p && *p)
      {
         char t[4];
         rec_field(&p, t, sizeof(t));
         if (t[0] == 'A')
         {
            char loc[16], mod[16];
            rec_field(&p, loc, sizeof(loc));
            rec_field(&p, mod, sizeof(mod));
            p = rec_next(p);
            pv[0] = vnum_s;
            pv[1] = loc;
            pv[2] = mod;
            if (!worker_exec("INSERT INTO object_affects (obj_vnum,location,modifier)"
                             " VALUES ($1,$2,$3)",
                             3, pv))
               ok = 0;
         }
         else if (t[0] == 'E')
         {
            char ekw[512], edesc[4096];
            rec_field(&p, ekw, sizeof(ekw));
            rec_field(&p, edesc, sizeof(edesc));
            p = rec_next(p);
            pv[0] = vnum_s;
            pv[1] = ekw;
            pv[2] = edesc;
            if (!worker_exec("INSERT INTO object_extra_descs (obj_vnum,keyword,description)"
                             " VALUES ($1,$2,$3)",
                             3, pv))
               ok = 0;
         }
         else
         {
            p = rec_next(p);
         }
      }
   }

   res = PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK");
   PQclear(res);
   return ok;
}

/* DB_WRITE_RESET_LIST — replace all resets for one area.
 * Record 0: area_min_vnum RS
 * Records 1..N: seq FS command FS ifflag FS arg1 FS arg2 FS arg3 FS notes FS auto_msg RS */
static int handle_write_reset_list(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char area_min[16];
   PGresult *res;
   int ok = 1;

   if (!ensure_connected())
      return 0;

   rec_field(&p, area_min, sizeof(area_min));
   p = rec_next(p);

   PQclear(PQexec(worker_conn, "BEGIN"));

   {
      const char *pv[1];
      pv[0] = area_min;
      worker_exec("DELETE FROM resets WHERE area_id="
                  "(SELECT id FROM areas WHERE min_vnum=$1::int)",
                  1, pv);
   }

   while (p && *p)
   {
      char seq_s[16], cmd_s[4], ifflag_s[16];
      char a1[16], a2[16], a3[16], notes[512], auto_msg[512];
      const char *pv[9];

      rec_field(&p, seq_s, sizeof(seq_s));
      rec_field(&p, cmd_s, sizeof(cmd_s));
      rec_field(&p, ifflag_s, sizeof(ifflag_s));
      rec_field(&p, a1, sizeof(a1));
      rec_field(&p, a2, sizeof(a2));
      rec_field(&p, a3, sizeof(a3));
      rec_field(&p, notes, sizeof(notes));
      rec_field(&p, auto_msg, sizeof(auto_msg));
      p = rec_next(p);

      if (!cmd_s[0])
         break;

      pv[0] = area_min;
      pv[1] = seq_s;
      pv[2] = cmd_s;
      pv[3] = ifflag_s;
      pv[4] = a1;
      pv[5] = a2;
      pv[6] = a3;
      pv[7] = notes[0] ? notes : NULL;
      pv[8] = auto_msg[0] ? auto_msg : NULL;
      if (!worker_exec(
              "INSERT INTO resets (area_id,seq,command,ifflag,arg1,arg2,arg3,notes,auto_msg)"
              " VALUES ((SELECT id FROM areas WHERE min_vnum=$1::int),$2,$3,$4,$5,$6,$7,$8,$9)",
              9, pv))
         ok = 0;
   }

   res = PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK");
   PQclear(res);
   return ok;
}

/* DB_WRITE_SHOP — upsert one shop.
 * Record: keeper FS buy[0..4] FS profit_buy FS profit_sell FS open_hour FS close_hour RS */
static int handle_write_shop(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char keeper[16], buy[5][16], pb[16], ps[16], oh[16], ch_[16];
   const char *params[11];
   int i;

   if (!ensure_connected())
      return 0;

   rec_field(&p, keeper, sizeof(keeper));
   for (i = 0; i < 5; i++)
      rec_field(&p, buy[i], sizeof(buy[i]));
   rec_field(&p, pb, sizeof(pb));
   rec_field(&p, ps, sizeof(ps));
   rec_field(&p, oh, sizeof(oh));
   rec_field(&p, ch_, sizeof(ch_));

   params[0] = keeper;
   for (i = 0; i < 5; i++)
      params[1 + i] = buy[i];
   params[6] = pb;
   params[7] = ps;
   params[8] = oh;
   params[9] = ch_;

   return worker_exec(
       "INSERT INTO shops (keeper_vnum,buy_type_0,buy_type_1,buy_type_2,buy_type_3,buy_type_4,"
       "profit_buy,profit_sell,open_hour,close_hour)"
       " VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10)"
       " ON CONFLICT (keeper_vnum) DO UPDATE SET"
       " buy_type_0=$2,buy_type_1=$3,buy_type_2=$4,buy_type_3=$5,buy_type_4=$6,"
       " profit_buy=$7,profit_sell=$8,open_hour=$9,close_hour=$10",
       10, params);
}

/* DB_DELETE_ROOM/MOB/OBJ — delete one entity by vnum.
 * Record: vnum RS */
static int handle_delete_room(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char vnum_s[16];
   const char *params[1];

   if (!ensure_connected())
      return 0;
   rec_field(&p, vnum_s, sizeof(vnum_s));
   params[0] = vnum_s;
   return worker_exec("DELETE FROM rooms WHERE vnum=$1", 1, params);
}

static int handle_delete_mob(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char vnum_s[16];
   const char *params[1];

   if (!ensure_connected())
      return 0;
   rec_field(&p, vnum_s, sizeof(vnum_s));
   params[0] = vnum_s;
   return worker_exec("DELETE FROM mobiles WHERE vnum=$1", 1, params);
}

static int handle_delete_obj(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char vnum_s[16];
   const char *params[1];

   if (!ensure_connected())
      return 0;
   rec_field(&p, vnum_s, sizeof(vnum_s));
   params[0] = vnum_s;
   return worker_exec("DELETE FROM objects WHERE vnum=$1", 1, params);
}

/* DB_WRITE_BOARD — upsert board metadata and replace all messages.
 * Record 0: vnum FS expiry_time FS min_read_lev FS min_write_lev FS clan RS
 * Records 1..N: datetime FS author FS title FS message RS */
static int handle_write_board(DB_REQUEST *req)
{
   const char *p = (const char *)req->buf;
   char vnum_s[16], expiry_s[16], min_read[16], min_write[16], clan_s[16];
   char board_id_s[32];
   const char *params[6];
   PGresult *res;
   int ok = 1;
   int seq = 0;

   if (!ensure_connected())
      return 0;

   rec_field(&p, vnum_s, sizeof(vnum_s));
   rec_field(&p, expiry_s, sizeof(expiry_s));
   rec_field(&p, min_read, sizeof(min_read));
   rec_field(&p, min_write, sizeof(min_write));
   rec_field(&p, clan_s, sizeof(clan_s));
   p = rec_next(p);

   PQclear(PQexec(worker_conn, "BEGIN"));

   /* Upsert board row */
   params[0] = vnum_s;
   params[1] = expiry_s;
   params[2] = min_read;
   params[3] = min_write;
   params[4] = clan_s;
   res = PQexecParams(worker_conn,
                      "INSERT INTO boards (vnum,expiry_days,min_read_lev,min_write_lev,clan)"
                      " VALUES ($1,$2,$3,$4,$5)"
                      " ON CONFLICT (vnum) DO UPDATE SET"
                      "  expiry_days=$2,min_read_lev=$3,min_write_lev=$4,clan=$5"
                      " RETURNING id",
                      5, NULL, params, NULL, NULL, 0);
   if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
   {
      fprintf(stderr, "DB worker board: upsert boards failed: %s\n", PQerrorMessage(worker_conn));
      PQclear(res);
      PQexec(worker_conn, "ROLLBACK");
      return 0;
   }
   snprintf(board_id_s, sizeof(board_id_s), "%s", PQgetvalue(res, 0, 0));
   PQclear(res);

   /* Delete existing messages */
   params[0] = board_id_s;
   if (!worker_exec("DELETE FROM board_messages WHERE board_id=$1", 1, params))
      ok = 0;

   /* Insert all current messages */
   while (ok && p && *p)
   {
      char dt_s[32], author[256], title[512], body[8192];
      char seq_s[16];
      const char *pv[6];

      rec_field(&p, dt_s, sizeof(dt_s));
      rec_field(&p, author, sizeof(author));
      rec_field(&p, title, sizeof(title));
      rec_field(&p, body, sizeof(body));
      p = rec_next(p);

      if (!dt_s[0])
         break;

      snprintf(seq_s, sizeof(seq_s), "%d", ++seq);
      pv[0] = board_id_s;
      pv[1] = dt_s;
      pv[2] = author;
      pv[3] = title;
      pv[4] = body;
      pv[5] = seq_s;
      if (!worker_exec("INSERT INTO board_messages (board_id,posted_at,author,title,body,seq)"
                       " VALUES ($1,$2,$3,$4,$5,$6)",
                       6, pv))
         ok = 0;
   }

   res = PQexec(worker_conn, ok ? "COMMIT" : "ROLLBACK");
   PQclear(res);
   return ok;
}

/* -----------------------------------------------------------------------
 * Public serialisation wrappers (called from game thread)
 * ----------------------------------------------------------------------- */

void db_worker_save_bans(BAN_DATA *first_ban_arg)
{
   BAN_DATA *b;
   SBuf s;
   sbuf_init(&s);

   for (b = first_ban_arg; b; b = b->next)
   {
      sbuf_puti(&s, b->newbie ? 1 : 0);
      sbuf_fs(&s);
      sbuf_puts(&s, b->name ? b->name : "");
      sbuf_fs(&s);
      sbuf_puts(&s, b->banned_by ? b->banned_by : "");
      sbuf_rs(&s);
   }

   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_BANS, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_socials(struct social_type *table, int count)
{
   int i;
   SBuf s;
   sbuf_init(&s);

   for (i = 0; i < count; i++)
   {
      if (!table[i].name || !table[i].name[0])
         continue;
      sbuf_puts(&s, table[i].name);
      sbuf_fs(&s);
      sbuf_puts(&s, table[i].char_no_arg ? table[i].char_no_arg : "");
      sbuf_fs(&s);
      sbuf_puts(&s, table[i].others_no_arg ? table[i].others_no_arg : "");
      sbuf_fs(&s);
      sbuf_puts(&s, table[i].char_found ? table[i].char_found : "");
      sbuf_fs(&s);
      sbuf_puts(&s, table[i].others_found ? table[i].others_found : "");
      sbuf_fs(&s);
      sbuf_puts(&s, table[i].vict_found ? table[i].vict_found : "");
      sbuf_fs(&s);
      sbuf_puts(&s, table[i].char_auto ? table[i].char_auto : "");
      sbuf_fs(&s);
      sbuf_puts(&s, table[i].others_auto ? table[i].others_auto : "");
      sbuf_rs(&s);
   }

   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_SOCIALS, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_clans(const sh_int diplomacy[][MAX_CLAN], const long int *treasury, int nclan)
{
   int i, j;
   SBuf s;
   sbuf_init(&s);

   for (i = 1; i < nclan; i++)
   {
      sbuf_puti(&s, i);
      sbuf_fs(&s);
      sbuf_puti(&s, treasury[i]);
      sbuf_fs(&s);
      sbuf_puti(&s, 0); /* gold — kept in clans table; not tracked per-game-cycle */
      for (j = 0; j < nclan; j++)
      {
         sbuf_fs(&s);
         sbuf_puti(&s, (long)diplomacy[i][j]);
      }
      sbuf_rs(&s);
   }

   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_CLANS, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_sysdata(int w_lock, int shownumbers)
{
   SBuf s;
   sbuf_init(&s);
   sbuf_puts(&s, w_lock ? "true" : "false");
   sbuf_fs(&s);
   sbuf_puts(&s, shownumbers ? "true" : "false");
   sbuf_rs(&s);

   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_SYSDATA, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_rulers(RULER_LIST *first_ruler_arg)
{
   RULER_LIST *rl;
   SBuf s;
   sbuf_init(&s);

   for (rl = first_ruler_arg; rl; rl = rl->next)
   {
      if (!rl->this_one || !rl->this_one->name || !rl->this_one->name[0])
         continue;
      sbuf_puts(&s, rl->this_one->name);
      sbuf_rs(&s);
   }

   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_RULERS, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_brands(DL_LIST *first_brand_arg)
{
   DL_LIST *dl;
   SBuf s;
   sbuf_init(&s);

   for (dl = first_brand_arg; dl; dl = dl->next)
   {
      BRAND_DATA *b = (BRAND_DATA *)dl->this_one;
      if (!b)
         continue;
      sbuf_puts(&s, b->branded_by ? b->branded_by : "");
      sbuf_fs(&s);
      sbuf_puts(&s, b->branded ? b->branded : "");
      sbuf_fs(&s);
      sbuf_puts(&s, b->dt_stamp ? b->dt_stamp : "");
      sbuf_fs(&s);
      sbuf_puts(&s, b->message ? b->message : "");
      sbuf_rs(&s);
   }

   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_BRANDS, s.buf, s.len, NULL);
      free(s.buf);
   }
}

/* Serialise one keep chest's contents DFS pre-order. */
static void sbuf_chest_items(SBuf *s, OBJ_DATA *obj, int parent_sort, int *counter)
{
   int my_sort = (*counter)++;
   char objfun_name[64];
   const char *fun_name = "";
   OBJ_DATA *child;

   if (obj->obj_fun)
   {
      const char *n = rev_obj_fun_lookup(obj->obj_fun);
      if (n)
         fun_name = n;
   }
   else
   {
      fun_name = "";
   }
   snprintf(objfun_name, sizeof(objfun_name), "%s", fun_name ? fun_name : "");

   sbuf_puti(s, (long)my_sort);
   sbuf_fs(s);
   sbuf_puti(s, (long)parent_sort);
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->pIndexData->vnum);
   sbuf_fs(s);
   sbuf_puts(s, obj->name ? obj->name : "");
   sbuf_fs(s);
   sbuf_puts(s, obj->short_descr ? obj->short_descr : "");
   sbuf_fs(s);
   sbuf_puts(s, obj->description ? obj->description : "");
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->extra_flags);
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->wear_flags);
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->wear_loc);
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->item_apply);
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->item_type);
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->weight);
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->level);
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->timer);
   sbuf_fs(s);
   sbuf_puti(s, (long)obj->cost);
   sbuf_fs(s);
   {
      int vi;
      for (vi = 0; vi < 10; vi++)
      {
         sbuf_puti(s, (long)obj->value[vi]);
         sbuf_fs(s);
      }
   }
   sbuf_puts(s, objfun_name);
   sbuf_rs(s);

   /* Recurse into contents (children of this item) */
   for (child = obj->first_in_carry_list; child; child = child->next_in_carry_list)
      sbuf_chest_items(s, child, my_sort, counter);
}

void db_worker_save_chest(OBJ_DATA *chest)
{
   SBuf s;
   int sort_counter = 0;
   OBJ_DATA *item;
   /* Extract owner name from short_descr: "<name>'s Keep Chest" */
   char owner[MAX_INPUT_LENGTH] = "";
   if (chest->short_descr)
   {
      const char *ap = strstr(chest->short_descr, "'s Keep Chest");
      if (ap)
      {
         size_t len = (size_t)(ap - chest->short_descr);
         if (len >= sizeof(owner))
            len = sizeof(owner) - 1;
         strncpy(owner, chest->short_descr, len);
         owner[len] = '\0';
      }
      else
      {
         strncpy(owner, chest->short_descr, sizeof(owner) - 1);
         owner[sizeof(owner) - 1] = '\0';
      }
   }

   sbuf_init(&s);

   /* Header record: vnum, owner_name, max_items */
   sbuf_puti(&s, (long)chest->pIndexData->vnum);
   sbuf_fs(&s);
   sbuf_puts(&s, owner);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)chest->value[3]);
   sbuf_rs(&s);

   /* Items (DFS pre-order, -1 parent_sort = direct child of chest) */
   for (item = chest->first_in_carry_list; item; item = item->next_in_carry_list)
      sbuf_chest_items(&s, item, -1, &sort_counter);

   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_CHEST, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_room_marks(MARK_LIST_MEMBER *first_mark_arg)
{
   MARK_LIST_MEMBER *ml;
   SBuf s;
   sbuf_init(&s);

   for (ml = first_mark_arg; ml; ml = ml->next)
   {
      MARK_DATA *m = ml->mark;
      if (!m)
         continue;
      sbuf_puti(&s, (long)m->room_vnum);
      sbuf_fs(&s);
      sbuf_puts(&s, m->message ? m->message : "");
      sbuf_rs(&s);
   }

   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_ROOM_MARKS, s.buf, s.len, NULL);
      free(s.buf);
   }
}

/* -----------------------------------------------------------------------
 * OLC / board public serialisation wrappers (called from game thread)
 * ----------------------------------------------------------------------- */

void db_worker_save_area_meta(AREA_DATA *pArea)
{
   SBuf s;
   sbuf_init(&s);
   sbuf_puti(&s, (long)pArea->area_num);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pArea->min_vnum);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pArea->max_vnum);
   sbuf_fs(&s);
   sbuf_puts(&s, pArea->name ? pArea->name : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pArea->keyword ? pArea->keyword : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pArea->level_label ? pArea->level_label : "");
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pArea->min_level);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pArea->max_level);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pArea->offset);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pArea->reset_rate);
   sbuf_fs(&s);
   sbuf_puts(&s, pArea->reset_msg ? pArea->reset_msg : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pArea->owner ? pArea->owner : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pArea->can_read ? pArea->can_read : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pArea->can_write ? pArea->can_write : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pArea->music ? pArea->music : "");
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pArea->flags);
   sbuf_rs(&s);
   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_AREA, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_room(ROOM_INDEX_DATA *pRoom)
{
   SBuf s;
   int d;
   EXTRA_DESCR_DATA *pEd;
   int locks;

   sbuf_init(&s);
   /* Room header */
   sbuf_puts(&s, "R");
   sbuf_fs(&s);
   sbuf_puti(&s, (long)(pRoom->area ? pRoom->area->min_vnum : 0));
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pRoom->vnum);
   sbuf_fs(&s);
   sbuf_puts(&s, pRoom->name ? pRoom->name : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pRoom->description ? pRoom->description : "");
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pRoom->room_flags);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pRoom->sector_type);
   sbuf_rs(&s);
   /* Exits */
   for (d = 0; d < 6; d++)
   {
      EXIT_DATA *pexit = pRoom->exit[d];
      if (!pexit)
         continue;
      locks = pexit->exit_info;
      if (IS_SET(locks, EX_CLOSED))
         REMOVE_BIT(locks, EX_CLOSED);
      if (IS_SET(locks, EX_LOCKED))
         REMOVE_BIT(locks, EX_LOCKED);
      sbuf_puts(&s, "X");
      sbuf_fs(&s);
      sbuf_puti(&s, (long)d);
      sbuf_fs(&s);
      sbuf_puts(&s, pexit->description ? pexit->description : "");
      sbuf_fs(&s);
      sbuf_puts(&s, pexit->keyword ? pexit->keyword : "");
      sbuf_fs(&s);
      sbuf_puti(&s, (long)locks);
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pexit->key);
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pexit->vnum);
      sbuf_rs(&s);
   }
   /* Extra descs */
   for (pEd = pRoom->first_exdesc; pEd; pEd = pEd->next)
   {
      sbuf_puts(&s, "E");
      sbuf_fs(&s);
      sbuf_puts(&s, pEd->keyword ? pEd->keyword : "");
      sbuf_fs(&s);
      sbuf_puts(&s, pEd->description ? pEd->description : "");
      sbuf_rs(&s);
   }
   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_ROOM, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_mob(MOB_INDEX_DATA *pMob)
{
   SBuf s;
   int i;
   sbuf_init(&s);
   sbuf_puti(&s, (long)(pMob->area ? pMob->area->min_vnum : 0));
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->vnum);
   sbuf_fs(&s);
   sbuf_puts(&s, pMob->player_name ? pMob->player_name : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pMob->short_descr ? pMob->short_descr : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pMob->long_descr ? pMob->long_descr : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pMob->description ? pMob->description : "");
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->act);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->affected_by);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->alignment);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->level);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->sex);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->hp_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->ac_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->hr_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->dr_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->class);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->clan);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->race);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->position);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->skills);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->cast);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->def);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->strong_magic);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->weak_magic);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->race_mods);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->power_skills);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->power_cast);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->resist);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->suscept);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->spellpower_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->crit_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->crit_mult_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->spell_crit_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->spell_mult_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->parry_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->dodge_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->block_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->pierce_mod);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->ai_knowledge);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->accent);
   sbuf_fs(&s);
   sbuf_puts(&s, pMob->ai_prompt ? pMob->ai_prompt : "");
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->loot_amount);
   for (i = 0; i < MAX_LOOT; i++)
   {
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pMob->loot[i]);
   }
   for (i = 0; i < MAX_LOOT; i++)
   {
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pMob->loot_chance[i]);
   }
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pMob->lore_flags);
   sbuf_rs(&s);
   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_MOB, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_obj(OBJ_INDEX_DATA *pObj)
{
   SBuf s;
   AFFECT_DATA *pAf;
   EXTRA_DESCR_DATA *pEd;
   int i;
   sbuf_init(&s);
   /* Object header */
   sbuf_puts(&s, "O");
   sbuf_fs(&s);
   sbuf_puti(&s, (long)(pObj->area ? pObj->area->min_vnum : 0));
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pObj->vnum);
   sbuf_fs(&s);
   sbuf_puts(&s, pObj->name ? pObj->name : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pObj->short_descr ? pObj->short_descr : "");
   sbuf_fs(&s);
   sbuf_puts(&s, pObj->description ? pObj->description : "");
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pObj->item_type);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pObj->extra_flags);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pObj->wear_flags);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pObj->item_apply);
   for (i = 0; i < 10; i++)
   {
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pObj->value[i]);
   }
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pObj->weight);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pObj->level);
   sbuf_rs(&s);
   /* Affects */
   for (pAf = pObj->first_apply; pAf; pAf = pAf->next)
   {
      sbuf_puts(&s, "A");
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pAf->location);
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pAf->modifier);
      sbuf_rs(&s);
   }
   /* Extra descs */
   for (pEd = pObj->first_exdesc; pEd; pEd = pEd->next)
   {
      sbuf_puts(&s, "E");
      sbuf_fs(&s);
      sbuf_puts(&s, pEd->keyword ? pEd->keyword : "");
      sbuf_fs(&s);
      sbuf_puts(&s, pEd->description ? pEd->description : "");
      sbuf_rs(&s);
   }
   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_OBJ, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_resets(AREA_DATA *pArea)
{
   RESET_DATA *pReset;
   SBuf s;
   int seq = 0;
   sbuf_init(&s);
   /* Header: area min_vnum */
   sbuf_puti(&s, (long)pArea->min_vnum);
   sbuf_rs(&s);
   for (pReset = pArea->first_reset; pReset; pReset = pReset->next)
   {
      char cmd[4];
      cmd[0] = pReset->command;
      cmd[1] = '\0';
      sbuf_puti(&s, (long)(++seq));
      sbuf_fs(&s);
      sbuf_puts(&s, cmd);
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pReset->ifflag);
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pReset->arg1);
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pReset->arg2);
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pReset->arg3);
      sbuf_fs(&s);
      sbuf_puts(&s, pReset->notes ? pReset->notes : "");
      sbuf_fs(&s);
      sbuf_puts(&s, pReset->auto_message ? pReset->auto_message : "");
      sbuf_rs(&s);
   }
   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_RESET_LIST, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_shop(SHOP_DATA *pShop)
{
   SBuf s;
   int i;
   sbuf_init(&s);
   sbuf_puti(&s, (long)pShop->keeper);
   for (i = 0; i < MAX_TRADE; i++)
   {
      sbuf_fs(&s);
      sbuf_puti(&s, (long)pShop->buy_type[i]);
   }
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pShop->profit_buy);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pShop->profit_sell);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pShop->open_hour);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)pShop->close_hour);
   sbuf_rs(&s);
   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_SHOP, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_delete_room(int vnum)
{
   SBuf s;
   sbuf_init(&s);
   sbuf_puti(&s, (long)vnum);
   sbuf_rs(&s);
   if (s.buf)
   {
      db_worker_enqueue_write(DB_DELETE_ROOM, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_delete_mob(int vnum)
{
   SBuf s;
   sbuf_init(&s);
   sbuf_puti(&s, (long)vnum);
   sbuf_rs(&s);
   if (s.buf)
   {
      db_worker_enqueue_write(DB_DELETE_MOB, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_delete_obj(int vnum)
{
   SBuf s;
   sbuf_init(&s);
   sbuf_puti(&s, (long)vnum);
   sbuf_rs(&s);
   if (s.buf)
   {
      db_worker_enqueue_write(DB_DELETE_OBJ, s.buf, s.len, NULL);
      free(s.buf);
   }
}

void db_worker_save_board(BOARD_DATA *board)
{
   MESSAGE_DATA *msg;
   SBuf s;
   sbuf_init(&s);
   /* Board header */
   sbuf_puti(&s, (long)board->vnum);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)board->expiry_time);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)board->min_read_lev);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)board->min_write_lev);
   sbuf_fs(&s);
   sbuf_puti(&s, (long)board->clan);
   sbuf_rs(&s);
   /* Messages */
   for (msg = board->first_message; msg; msg = msg->next)
   {
      sbuf_puti(&s, (long)msg->datetime);
      sbuf_fs(&s);
      sbuf_puts(&s, msg->author ? msg->author : "");
      sbuf_fs(&s);
      sbuf_puts(&s, msg->title ? msg->title : "");
      sbuf_fs(&s);
      sbuf_puts(&s, msg->message ? msg->message : "");
      sbuf_rs(&s);
   }
   if (s.buf)
   {
      db_worker_enqueue_write(DB_WRITE_BOARD, s.buf, s.len, NULL);
      free(s.buf);
   }
}

static int dispatch(DB_REQUEST *req)
{
   switch (req->type)
   {
   case DB_WRITE_PLAYER:
      return handle_write_player(req);
   case DB_WRITE_CLANS:
      return handle_write_clans(req);
   case DB_WRITE_BANS:
      return handle_write_bans(req);
   case DB_WRITE_SOCIALS:
      return handle_write_socials(req);
   case DB_WRITE_CORPSES:
      return handle_write_corpses(req);
   case DB_WRITE_SYSDATA:
      return handle_write_sysdata(req);
   case DB_WRITE_RULERS:
      return handle_write_rulers(req);
   case DB_WRITE_BRANDS:
      return handle_write_brands(req);
   case DB_WRITE_ROOM_MARKS:
      return handle_write_room_marks(req);
   case DB_WRITE_CHEST:
      return handle_write_chest(req);
   case DB_WRITE_AREA:
      return handle_write_area(req);
   case DB_WRITE_ROOM:
      return handle_write_room(req);
   case DB_WRITE_MOB:
      return handle_write_mob(req);
   case DB_WRITE_OBJ:
      return handle_write_obj(req);
   case DB_WRITE_RESET_LIST:
      return handle_write_reset_list(req);
   case DB_WRITE_SHOP:
      return handle_write_shop(req);
   case DB_DELETE_ROOM:
      return handle_delete_room(req);
   case DB_DELETE_MOB:
      return handle_delete_mob(req);
   case DB_DELETE_OBJ:
      return handle_delete_obj(req);
   case DB_WRITE_BOARD:
      return handle_write_board(req);
   case DB_READ_PLAYER:
      handle_read_player(req);
      return 1;
   case DB_OP_SHUTDOWN:
      return 1;
   default:
      fprintf(stderr, "DB worker: unknown op type %d\n", (int)req->type);
      return 0;
   }
}

/* -----------------------------------------------------------------------
 * Worker thread main loop
 * ----------------------------------------------------------------------- */

static void *worker_main(void *arg)
{
   (void)arg;

   for (;;)
   {
      DB_REQUEST *req;
      int ok;

      /* Wait for work */
      pthread_mutex_lock(&req_mutex);
      while (req_head == NULL)
         pthread_cond_wait(&req_cond, &req_mutex);

      /* Dequeue from head */
      req = req_head;
      req_head = req->next;
      if (req_head == NULL)
         req_tail = NULL;
      pthread_mutex_unlock(&req_mutex);

      if (req->type == DB_OP_SHUTDOWN)
      {
         free(req->buf);
         free(req);
         break;
      }

      ok = dispatch(req);
      if (!ok)
      {
         consecutive_failures++;
         if (consecutive_failures >= DB_MAX_FAILURES && !db_worker_failed)
         {
            fprintf(stderr,
                    "DB worker: %d consecutive failures — entering "
                    "emergency fallback mode\n",
                    consecutive_failures);
            db_worker_failed = 1;
         }
      }
      else
      {
         consecutive_failures = 0;
      }

      free(req->buf);
      free(req);
   }

   if (worker_conn)
   {
      PQfinish(worker_conn);
      worker_conn = NULL;
   }
   return NULL;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void db_worker_start(const char *connstr)
{
   if (connstr)
      strncpy(worker_connstr, connstr, sizeof(worker_connstr) - 1);

   if (pthread_create(&worker_thread, NULL, worker_main, NULL) != 0)
   {
      perror("DB worker: pthread_create failed");
      db_worker_failed = 1;
   }
}

void db_worker_stop(void)
{
   /* Enqueue shutdown sentinel at the tail so pending writes are flushed */
   DB_REQUEST *req = calloc(1, sizeof(DB_REQUEST));
   if (!req)
      return;
   req->type = DB_OP_SHUTDOWN;

   pthread_mutex_lock(&req_mutex);
   if (req_tail)
   {
      req_tail->next = req;
      req_tail = req;
   }
   else
   {
      req_head = req_tail = req;
   }
   pthread_cond_signal(&req_cond);
   pthread_mutex_unlock(&req_mutex);

   pthread_join(worker_thread, NULL);
}

void db_worker_enqueue_write(DB_OP_TYPE type, const void *buf, size_t len, const char *name)
{
   DB_REQUEST *req;

   pthread_mutex_lock(&req_mutex);

   /* Coalesce player saves: if a pending DB_WRITE_PLAYER for the same name
    * exists, replace its payload in-place rather than appending a new entry.
    */
   if (type == DB_WRITE_PLAYER && name && name[0])
   {
      DB_REQUEST *p;
      for (p = req_head; p; p = p->next)
      {
         if (p->type == DB_WRITE_PLAYER && strcmp(p->name, name) == 0)
         {
            /* Replace buffer */
            free(p->buf);
            p->buf = malloc(len + 1);
            if (p->buf)
            {
               memcpy(p->buf, buf, len);
               ((char *)p->buf)[len] = '\0';
               p->len = len;
            }
            pthread_mutex_unlock(&req_mutex);
            return;
         }
      }
   }

   req = calloc(1, sizeof(DB_REQUEST));
   if (!req)
   {
      pthread_mutex_unlock(&req_mutex);
      return;
   }
   req->type = type;
   req->len = len;
   req->buf = malloc(len + 1);
   if (req->buf)
   {
      memcpy(req->buf, buf, len);
      ((char *)req->buf)[len] = '\0';
   }
   if (name)
      strncpy(req->name, name, sizeof(req->name) - 1);

   /* Writes go to the tail */
   req->next = NULL;
   if (req_tail)
   {
      req_tail->next = req;
      req_tail = req;
   }
   else
   {
      req_head = req_tail = req;
   }

   pthread_cond_signal(&req_cond);
   pthread_mutex_unlock(&req_mutex);
}

void db_worker_enqueue_load_player(struct descriptor_data *d, const char *name)
{
   DB_REQUEST *req = calloc(1, sizeof(DB_REQUEST));
   if (!req)
      return;
   req->type = DB_READ_PLAYER;
   req->d = d;
   strncpy(req->name, name, sizeof(req->name) - 1);
   req->buf = NULL;
   req->len = 0;

   d->connected = CON_LOADING_FROM_DB;

   /* Reads go to the head for priority */
   pthread_mutex_lock(&req_mutex);
   req->next = req_head;
   req_head = req;
   if (req_tail == NULL)
      req_tail = req;
   pthread_cond_signal(&req_cond);
   pthread_mutex_unlock(&req_mutex);
}

void db_worker_poll_results(void)
{
   DB_PLAYER_RESULT *list;
   DB_PLAYER_RESULT *r;
   DB_PLAYER_RESULT *next;

   pthread_mutex_lock(&res_mutex);
   list = res_head;
   res_head = NULL;
   pthread_mutex_unlock(&res_mutex);

   for (r = list; r; r = next)
   {
      next = r->next;

      if (!r->d || r->d->connected != CON_LOADING_FROM_DB)
      {
         free(r->raw_save);
         free(r);
         continue;
      }

      if (r->error)
      {
         /* DB error — tell the player to try again and close */
         write_to_buffer(r->d, "Database error \xe2\x80\x94 please try again later.\n\r", 0);
         close_socket(r->d);
      }
      else if (r->found && r->raw_save)
      {
         /* Existing player: hydrate CHAR_DATA from raw_save on the game thread */
         CHAR_DATA *ch = r->d->character;
         if (load_char_from_raw(ch, r->raw_save))
         {
            finish_player_login(r->d, TRUE);
         }
         else
         {
            write_to_buffer(r->d, "Error loading character \xe2\x80\x94 please try again.\n\r", 0);
            close_socket(r->d);
         }
      }
      else
      {
         /* New player — character is already alloc'd and init'd; run post-load checks */
         finish_player_login(r->d, FALSE);
      }

      free(r->raw_save);
      free(r);
   }
}

char *db_worker_fetch_player_raw_save(const char *name)
{
   PGconn *conn;
   PGresult *res;
   const char *params[1];
   char *result = NULL;
   char connstr[512] = "";
   FILE *fp;
   const char *env_conf;
   char path[512];

   /* Read the same db.conf the boot connection uses. */
   env_conf = getenv("ACK_DB_CONF");
   if (env_conf && env_conf[0])
      snprintf(path, sizeof(path), "%s", env_conf);
   else
      snprintf(path, sizeof(path), "../data/db.conf");

   fp = fopen(path, "r");
   if (fp)
   {
      size_t n = fread(connstr, 1, sizeof(connstr) - 1, fp);
      fclose(fp);
      connstr[n] = '\0';
      /* Strip trailing whitespace */
      while (n > 0 && (connstr[n - 1] == '\n' || connstr[n - 1] == '\r' || connstr[n - 1] == ' ' ||
                       connstr[n - 1] == '\t'))
         connstr[--n] = '\0';
   }

   conn = PQconnectdb(connstr);
   if (PQstatus(conn) != CONNECTION_OK)
   {
      fprintf(stderr, "db_worker_fetch_player_raw_save: connect failed: %s\n",
              PQerrorMessage(conn));
      PQfinish(conn);
      return NULL;
   }

   params[0] = name;
   res = PQexecParams(conn, "SELECT raw_save FROM players WHERE name = $1", 1, NULL, params, NULL,
                      NULL, 0);

   if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
   {
      const char *raw = PQgetvalue(res, 0, 0);
      if (raw && raw[0])
         result = strdup(raw);
   }

   PQclear(res);
   PQfinish(conn);
   return result;
}
