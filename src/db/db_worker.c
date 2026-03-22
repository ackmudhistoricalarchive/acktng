#ifdef HAVE_LIBPQ

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

static void post_result(struct descriptor_data *d, struct char_data *ch, int error)
{
   DB_PLAYER_RESULT *r = calloc(1, sizeof(DB_PLAYER_RESULT));
   if (!r)
      return;
   r->d = d;
   r->ch = ch;
   r->error = error;
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

/* DB_WRITE_PLAYER — buf is a NUL-terminated JSON blob.
 * Uses INSERT ... ON CONFLICT (name) DO UPDATE to upsert. */
static int handle_write_player(DB_REQUEST *req)
{
   /* TODO: deserialise buf into individual columns and upsert.
    * For now store the raw JSON as raw_save only, which serves as a
    * transitional fallback until the full serialiser is implemented. */
   const char *params[2];
   params[0] = req->name;
   params[1] = (const char *)req->buf;
   return worker_exec("INSERT INTO players (name, pwd_hash, raw_save) "
                      "VALUES ($1, '', $2) "
                      "ON CONFLICT (name) DO UPDATE SET raw_save = EXCLUDED.raw_save",
                      2, params);
}

/* DB_READ_PLAYER — SELECT player row and reconstruct CHAR_DATA.
 * Posts result to the results queue when done. */
static void handle_read_player(DB_REQUEST *req)
{
   PGresult *res;
   const char *params[1];
   int nrows;

   if (!ensure_connected())
   {
      post_result(req->d, NULL, 1);
      return;
   }

   params[0] = req->name;
   res = PQexecParams(worker_conn,
                      "SELECT name, pwd_hash, raw_save FROM players "
                      "WHERE name = $1",
                      1, NULL, params, NULL, NULL, 0);

   if (PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      fprintf(stderr, "DB worker: read_player query failed: %s\n", PQresultErrorMessage(res));
      PQclear(res);
      post_result(req->d, NULL, 1);
      return;
   }

   nrows = PQntuples(res);
   PQclear(res);

   if (nrows == 0)
   {
      /* New player */
      post_result(req->d, NULL, 0);
      return;
   }

   /* TODO: hydrate CHAR_DATA from columns.
    * For now signal "found" with a NULL ch; the poll handler will fall
    * back to load_char_obj() from the flat file or raw_save. */
   post_result(req->d, NULL, 0);
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

      if (r->error)
      {
         /* DB error — tell the player to try again and close */
         if (r->d && r->d->connected == CON_LOADING_FROM_DB)
         {
            write_to_buffer(r->d, "Database error — please try again later.\n\r", 0);
            close_socket(r->d);
         }
      }
      else if (r->ch == NULL)
      {
         /* Row not found: new player — proceed to name confirmation */
         if (r->d && r->d->connected == CON_LOADING_FROM_DB)
            r->d->connected = CON_CONFIRM_NEW_NAME;
      }
      else
      {
         /* Existing player loaded */
         if (r->d && r->d->connected == CON_LOADING_FROM_DB)
         {
            r->d->character = r->ch;
            r->ch->desc = r->d;
            r->d->connected = CON_GET_OLD_PASSWORD;
            write_to_buffer(r->d, "Password: ", 0);
         }
      }

      free(r);
   }
}

#endif /* HAVE_LIBPQ */
