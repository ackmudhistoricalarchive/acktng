/* db_help.c — Runtime on-demand help/shelp/lore lookup for ACK!TNG.
 *
 * Maintains a persistent PGconn* opened after boot (db_help_open) and
 * closed on shutdown (db_help_close).  Command handlers call the lookup
 * functions directly; no in-memory lists are maintained.
 *
 * Synchronous queries are acceptable here: PostgreSQL indexed lookups
 * on keyword columns are sub-millisecond and the game loop tolerates
 * brief waits for infrequent staff/player commands.
 */

#ifdef HAVE_LIBPQ

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

#include "globals.h" /* for extern int top_help */
#include "db_help.h"

/* ---------------------------------------------------------------------------
 * Internal connection
 * --------------------------------------------------------------------------- */

static PGconn *help_conn = NULL;

/* Read the connection string from db.conf (mirrors logic in db_conn.c).
 * Caller must free() the returned string.  Returns NULL if absent. */
static char *read_help_conf(const char *area_dir)
{
   char path[512];
   const char *env_conf;
   FILE *fp;
   long file_size;
   char *buf;
   size_t n;

   env_conf = getenv("ACK_DB_CONF");
   if (env_conf && env_conf[0])
      snprintf(path, sizeof(path), "%s", env_conf);
   else
      snprintf(path, sizeof(path), "%s/../data/db.conf", area_dir);

   fp = fopen(path, "r");
   if (!fp)
      return NULL;

   if (fseek(fp, 0, SEEK_END) != 0)
   {
      fclose(fp);
      return NULL;
   }
   file_size = ftell(fp);
   rewind(fp);

   if (file_size <= 0)
   {
      fclose(fp);
      return NULL;
   }

   buf = malloc((size_t)file_size + 1);
   if (!buf)
   {
      fclose(fp);
      return NULL;
   }

   n = fread(buf, 1, (size_t)file_size, fp);
   fclose(fp);

   if (n == 0)
   {
      free(buf);
      return NULL;
   }

   buf[n] = '\0';
   while (n > 0 &&
          (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ' || buf[n - 1] == '\t'))
      buf[--n] = '\0';
   return buf;
}

int db_help_open(const char *area_dir)
{
   char *connstr;
   PGresult *res;

   connstr = read_help_conf(area_dir);
   if (!connstr)
      return -1;

   help_conn = PQconnectdb(connstr);
   free(connstr);

   if (PQstatus(help_conn) != CONNECTION_OK)
   {
      fprintf(stderr, "DB help: connection failed: %s\n", PQerrorMessage(help_conn));
      PQfinish(help_conn);
      help_conn = NULL;
      return 0;
   }

   /* Cache help count for MSSP */
   res = PQexec(help_conn, "SELECT COUNT(*) FROM help_entries");
   if (res && PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
   {
      extern int top_help;
      top_help = atoi(PQgetvalue(res, 0, 0));
   }
   if (res)
      PQclear(res);

   fprintf(stderr, "DB help: runtime connection open (top_help=%d).\n", top_help);
   return 1;
}

void db_help_close(void)
{
   if (help_conn)
   {
      PQfinish(help_conn);
      help_conn = NULL;
   }
}

/* ---------------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------------- */

/* Count set bits in a long value. */
static int count_bits_long(long v)
{
   int c = 0;
   unsigned long u = (unsigned long)v;
   while (u)
   {
      c += (int)(u & 1u);
      u >>= 1;
   }
   return c;
}

/* Execute a parameterised query. Returns NULL on error (already logged). */
static PGresult *hq(const char *sql, int np, const char *const *vals)
{
   PGresult *res;
   if (!help_conn)
      return NULL;
   res = PQexecParams(help_conn, sql, np, NULL, vals, NULL, NULL, 0);
   if (!res || (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK))
   {
      fprintf(stderr, "DB help query error: %s\n",
              res ? PQresultErrorMessage(res) : PQerrorMessage(help_conn));
      if (res)
         PQclear(res);
      return NULL;
   }
   return res;
}

/* ---------------------------------------------------------------------------
 * Help / shelp lookup
 * --------------------------------------------------------------------------- */

/* Shared implementation for help_entries and shelp_entries. */
static int lookup_help_table(const char *table, const char *keyword, int level, char *kw_out,
                             size_t kw_sz, char *text_out, size_t text_sz, int *level_out)
{
   char sql[512];
   char level_str[16];
   const char *vals[2];
   PGresult *res;

   if (!help_conn)
      return 0;

   snprintf(level_str, sizeof(level_str), "%d", level);

   /* One query: exact match first (order 0), then prefix match (order 1).
    * Among prefix matches, prefer shorter keywords (closer match). */
   snprintf(sql, sizeof(sql),
            "SELECT level, keywords, body FROM %s "
            "WHERE level <= $1 "
            "  AND (LOWER(keywords) = LOWER($2) "
            "       OR LOWER(keywords) LIKE LOWER($2) || '%%') "
            "ORDER BY "
            "  CASE WHEN LOWER(keywords) = LOWER($2) THEN 0 ELSE 1 END, "
            "  LENGTH(keywords) "
            "LIMIT 1",
            table);

   vals[0] = level_str;
   vals[1] = keyword;
   res = hq(sql, 2, vals);
   if (!res || PQntuples(res) == 0)
   {
      if (res)
         PQclear(res);
      return 0;
   }

   if (level_out)
      *level_out = atoi(PQgetvalue(res, 0, 0));
   snprintf(kw_out, kw_sz, "%s", PQgetvalue(res, 0, 1));
   snprintf(text_out, text_sz, "%s", PQgetvalue(res, 0, 2));
   PQclear(res);
   return 1;
}

int db_help_lookup(const char *keyword, int level, char *kw_out, size_t kw_sz, char *text_out,
                   size_t text_sz, int *level_out)
{
   return lookup_help_table("help_entries", keyword, level, kw_out, kw_sz, text_out, text_sz,
                            level_out);
}

int db_shelp_lookup(const char *keyword, int level, char *kw_out, size_t kw_sz, char *text_out,
                    size_t text_sz, int *level_out)
{
   return lookup_help_table("shelp_entries", keyword, level, kw_out, kw_sz, text_out, text_sz,
                            level_out);
}

/* ---------------------------------------------------------------------------
 * Lore lookup with flag scoring
 * --------------------------------------------------------------------------- */

/* Run a lore query (exact or prefix match on topic keywords).
 * Fetches all matching topic entries and picks the best by flag score. */
static int lore_query(const char *keyword, long npc_flags, int prefix_ok, char *text_out,
                      size_t text_sz)
{
   const char *vals[1];
   const char *sql;
   PGresult *res;
   int i, n;
   const char *best_body = NULL;
   int best_score = -1;

   if (!help_conn)
      return 0;

   vals[0] = keyword;

   if (!prefix_ok)
      sql = "SELECT e.flags, e.body "
            "FROM lore_topics t "
            "JOIN lore_entries e ON e.topic_id = t.id "
            "WHERE LOWER(t.keywords) = LOWER($1) "
            "ORDER BY e.seq";
   else
      sql = "SELECT e.flags, e.body "
            "FROM lore_topics t "
            "JOIN lore_entries e ON e.topic_id = t.id "
            "WHERE LOWER(t.keywords) LIKE LOWER($1) || '%' "
            "ORDER BY LENGTH(t.keywords), e.seq";

   res = hq(sql, 1, vals);
   if (!res)
      return 0;

   n = PQntuples(res);
   for (i = 0; i < n; i++)
   {
      long flags = atol(PQgetvalue(res, i, 0));
      const char *body = PQgetvalue(res, i, 1);
      int score;

      /* Flagged entries must be a full subset of npc_flags. */
      if (flags != 0 && (flags & npc_flags) != flags)
         continue;

      score = count_bits_long(flags & npc_flags);

      if (flags == 0 && best_score < 0)
      {
         /* Default (unflagged) entry — lowest priority. */
         best_body = body;
         best_score = 0;
      }
      else if (score > best_score)
      {
         best_body = body;
         best_score = score;
      }
   }

   if (best_body)
   {
      snprintf(text_out, text_sz, "%s", best_body);
      PQclear(res);
      return 1;
   }

   PQclear(res);
   return 0;
}

int db_lore_lookup(const char *keyword, long npc_flags, char *text_out, size_t text_sz)
{
   /* Exact match pass first, then prefix. */
   if (lore_query(keyword, npc_flags, 0, text_out, text_sz))
      return 1;
   return lore_query(keyword, npc_flags, 1, text_out, text_sz);
}

/* ---------------------------------------------------------------------------
 * Lore collection by NPC flags (for AI system-prompt injection)
 * --------------------------------------------------------------------------- */

int db_lore_collect_by_flags(long npc_flags, int max_results,
                             void (*result_cb)(const char *keyword, const char *body,
                                               void *userdata),
                             void *userdata)
{
   /* Fetch all lore entries that could match (flags <= npc_flags means every
    * set bit in entry.flags is also set in npc_flags). We do the exact
    * flag-subset test and scoring in C because PostgreSQL bitwise ops on
    * integer columns require casts and would add complexity.
    *
    * We keep the top max_results entries by score in a small heap. */
   static const char *sql = "SELECT t.keywords, e.flags, e.body "
                            "FROM lore_topics t "
                            "JOIN lore_entries e ON e.topic_id = t.id "
                            "WHERE e.flags != 0 "
                            "ORDER BY e.seq";
   PGresult *res;
   int i, n;

   /* Per-slot tracking */
#define LCF_MAX 16
   const char *kw_slots[LCF_MAX];
   const char *body_slots[LCF_MAX];
   int score_slots[LCF_MAX];
   int slot_count = 0;

   if (!help_conn || max_results <= 0 || !result_cb)
      return 0;
   if (max_results > LCF_MAX)
      max_results = LCF_MAX;

   res = PQexec(help_conn, sql);
   if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      if (res)
         PQclear(res);
      return 0;
   }

   n = PQntuples(res);
   for (i = 0; i < n; i++)
   {
      long flags = atol(PQgetvalue(res, i, 1));
      int score;

      if (flags == 0)
         continue;
      if ((flags & npc_flags) != flags)
         continue;

      score = count_bits_long(flags & npc_flags);

      if (slot_count < max_results)
      {
         kw_slots[slot_count] = PQgetvalue(res, i, 0);
         body_slots[slot_count] = PQgetvalue(res, i, 2);
         score_slots[slot_count] = score;
         slot_count++;
      }
      else
      {
         /* Replace the lowest-scoring slot */
         int worst = 0, j;
         for (j = 1; j < slot_count; j++)
            if (score_slots[j] < score_slots[worst])
               worst = j;
         if (score > score_slots[worst])
         {
            kw_slots[worst] = PQgetvalue(res, i, 0);
            body_slots[worst] = PQgetvalue(res, i, 2);
            score_slots[worst] = score;
         }
      }
   }

   /* Fire callbacks; we call in slot order (not sorted by score) since the
    * caller (system-prompt builder) just needs the entries, not rank order. */
   for (i = 0; i < slot_count; i++)
      result_cb(kw_slots[i], body_slots[i], userdata);

   PQclear(res);
   return slot_count;
#undef LCF_MAX
}

/* ---------------------------------------------------------------------------
 * Build / OLC helpers
 * --------------------------------------------------------------------------- */

void db_help_find(const char *arg, db_help_find_cb cb, void *userdata)
{
   const char *vals[1];
   char like_arg[512];
   PGresult *res;
   int i, n, rank = 0;

   if (!help_conn || !arg || !cb)
      return;

   snprintf(like_arg, sizeof(like_arg), "%%%s%%", arg);
   vals[0] = like_arg;

   res = hq("SELECT keywords, body FROM help_entries "
            "WHERE LOWER(keywords) LIKE LOWER($1) "
            "ORDER BY keywords",
            1, vals);
   if (!res)
      return;

   n = PQntuples(res);
   for (i = 0; i < n; i++)
   {
      const char *kw = PQgetvalue(res, i, 0);
      const char *body = PQgetvalue(res, i, 1);
      cb(++rank, kw, body, userdata);
   }
   PQclear(res);
}

int db_help_find_nth(const char *arg, int n, int *id_out, char *kw_out, size_t kw_sz,
                     char *body_out, size_t body_sz)
{
   const char *vals[1];
   char like_arg[512];
   char offset_str[16];
   char sql[512];
   PGresult *res;

   if (!help_conn || n < 1)
      return 0;

   snprintf(like_arg, sizeof(like_arg), "%%%s%%", arg);
   snprintf(offset_str, sizeof(offset_str), "%d", n - 1);

   snprintf(sql, sizeof(sql),
            "SELECT id, keywords, body FROM help_entries "
            "WHERE LOWER(keywords) LIKE LOWER($1) "
            "ORDER BY keywords "
            "LIMIT 1 OFFSET %d",
            n - 1);

   vals[0] = like_arg;
   res = hq(sql, 1, vals);
   if (!res || PQntuples(res) == 0)
   {
      if (res)
         PQclear(res);
      return 0;
   }

   if (id_out)
      *id_out = atoi(PQgetvalue(res, 0, 0));
   snprintf(kw_out, kw_sz, "%s", PQgetvalue(res, 0, 1));
   snprintf(body_out, body_sz, "%s", PQgetvalue(res, 0, 2));
   PQclear(res);
   return 1;
}

int db_help_update_body(int id, const char *body)
{
   char id_str[16];
   const char *vals[2];
   PGresult *res;

   if (!help_conn || id <= 0 || !body)
      return 0;

   snprintf(id_str, sizeof(id_str), "%d", id);
   vals[0] = body;
   vals[1] = id_str;

   res = hq("UPDATE help_entries SET body=$1 WHERE id=$2::int", 2, vals);
   if (!res)
      return 0;
   PQclear(res);
   return 1;
}

int db_help_insert(int level, const char *keywords, const char *body)
{
   char level_str[16];
   char filename[256];
   const char *vals[4];
   PGresult *res;
   int new_id;

   if (!help_conn || !keywords || !body)
      return 0;

   snprintf(level_str, sizeof(level_str), "%d", level);

   /* Use keywords as filename (sanitised to first word, lowercase). */
   {
      const char *p = keywords;
      size_t i = 0;
      while (*p && *p != ' ' && i < sizeof(filename) - 1)
      {
         filename[i++] = (*p >= 'A' && *p <= 'Z') ? (char)(*p + 32) : *p;
         p++;
      }
      filename[i] = '\0';
   }

   vals[0] = level_str;
   vals[1] = keywords;
   vals[2] = body;
   vals[3] = filename;

   res = hq("INSERT INTO help_entries (level, keywords, body, filename) "
            "VALUES ($1::int, $2, $3, $4) RETURNING id",
            4, vals);
   if (!res || PQntuples(res) == 0)
   {
      if (res)
         PQclear(res);
      return 0;
   }
   new_id = atoi(PQgetvalue(res, 0, 0));
   PQclear(res);
   return new_id;
}

#endif /* HAVE_LIBPQ */
