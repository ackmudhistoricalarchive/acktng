/* migrate_chests.c — Migrate flat-file keep chests to PostgreSQL.
 *
 * Standalone tool (not linked with the server).  Reads data/chest/<vnum>
 * flat files and upserts their contents into the keep_chests and
 * keep_chest_items PostgreSQL tables.
 *
 * Usage (run from the area/ directory, same as the server):
 *   ./tools/migrate_chests [options] [connstr]
 *
 * Options:
 *   --dry-run       Parse and report; do not write to the database.
 *   --vnum <N>      Migrate only the chest whose file name is <N>.
 *
 * connstr defaults to the contents of data/db.conf (relative to area/).
 * If that file is absent, libpq PG* environment variables are used.
 *
 * Build: make tools/migrate_chests  (from src/)
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

/* -----------------------------------------------------------------------
 * Connection state
 * ----------------------------------------------------------------------- */

static PGconn *conn;
static int dry_run = 0;
static int errors;

/* -----------------------------------------------------------------------
 * SQL helpers
 * ----------------------------------------------------------------------- */

static int exec_sql(const char *sql)
{
   PGresult *res = PQexec(conn, sql);
   int ok = (PQresultStatus(res) == PGRES_COMMAND_OK || PQresultStatus(res) == PGRES_TUPLES_OK);
   if (!ok)
      fprintf(stderr, "SQL error: %s\n--- %s\n", PQresultErrorMessage(res), sql);
   PQclear(res);
   return ok;
}

static PGresult *exec_params(const char *sql, int nparams, const char *const *params)
{
   PGresult *res = PQexecParams(conn, sql, nparams, NULL, params, NULL, NULL, 0);
   if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK)
   {
      fprintf(stderr, "SQL error: %s\n--- %s\n", PQresultErrorMessage(res), sql);
      errors++;
   }
   return res;
}

/* -----------------------------------------------------------------------
 * Flat-file parsing helpers
 * ----------------------------------------------------------------------- */

/* Read a line from fp into buf (up to size-1 chars), stripping \n.
 * Returns buf on success, NULL at EOF. */
static char *read_line(FILE *fp, char *buf, size_t size)
{
   if (!fgets(buf, (int)size, fp))
      return NULL;
   buf[strcspn(buf, "\r\n")] = '\0';
   return buf;
}

/* Read a tilde-terminated string from fp (consumes up to and including '~').
 * Returns a malloc'd copy; caller must free().  Returns "" on error. */
static char *read_tilde_string(FILE *fp)
{
   char buf[16384];
   size_t len = 0;
   int c;

   while ((c = fgetc(fp)) != EOF)
   {
      if (c == '~')
         break;
      if (len + 1 < sizeof(buf))
         buf[len++] = (char)c;
   }
   /* consume rest of line after tilde */
   while ((c = fgetc(fp)) != EOF && c != '\n')
      ;
   buf[len] = '\0';
   /* strip leading newline if present */
   char *p = buf;
   if (*p == '\n')
      p++;
   return strdup(p);
}

/* -----------------------------------------------------------------------
 * Item record (one #OBJECT block)
 * ----------------------------------------------------------------------- */

#define MAX_NEST_LEVELS 16
#define MAX_ITEMS_PER_CHEST 512

typedef struct
{
   int nest;
   int vnum;
   char name[256];
   char short_descr[256];
   char description[4096];
   unsigned long long extra_flags;
   int wear_flags;
   int wear_loc;
   int class_flags;
   int item_type;
   int weight;
   int level;
   int timer;
   int cost;
   int value[10];
   char objfun[128];
} ChestItem;

/* -----------------------------------------------------------------------
 * Parse one chest file into a dynamic array of items.
 * Returns item count; items array is malloc'd (caller must free).
 * The first item (index 0) is the chest itself (Nest=0).
 * ----------------------------------------------------------------------- */
static int parse_chest_file(const char *path, ChestItem **items_out)
{
   FILE *fp = fopen(path, "r");
   if (!fp)
   {
      fprintf(stderr, "migrate_chests: cannot open %s: %s\n", path, strerror(errno));
      return -1;
   }

   ChestItem *items = NULL;
   int count = 0;
   int capacity = 0;
   char line[8192];

   while (read_line(fp, line, sizeof(line)))
   {
      if (strcmp(line, "#END") == 0)
         break;

      if (strcmp(line, "#OBJECT") == 0)
      {
         /* Grow array if needed */
         if (count >= capacity)
         {
            capacity = capacity ? capacity * 2 : 16;
            items = realloc(items, capacity * sizeof(ChestItem));
            if (!items)
            {
               fprintf(stderr, "migrate_chests: out of memory\n");
               fclose(fp);
               return -1;
            }
         }

         ChestItem *it = &items[count];
         memset(it, 0, sizeof(*it));
         it->wear_loc = -1;
         it->timer = -1;

         /* Parse fields until "End" */
         while (read_line(fp, line, sizeof(line)))
         {
            char *sp = strchr(line, ' ');
            char keyword[64] = "";
            char *rest = "";

            if (sp)
            {
               size_t klen = (size_t)(sp - line);
               if (klen >= sizeof(keyword))
                  klen = sizeof(keyword) - 1;
               strncpy(keyword, line, klen);
               keyword[klen] = '\0';
               rest = sp + 1;
               while (*rest == ' ')
                  rest++;
            }
            else
            {
               strncpy(keyword, line, sizeof(keyword) - 1);
            }

            if (strcmp(keyword, "End") == 0)
               break;
            if (keyword[0] == '*')
               continue; /* comment */

            if (strcmp(keyword, "Nest") == 0)
            {
               it->nest = atoi(rest);
            }
            else if (strcmp(keyword, "Vnum") == 0)
            {
               it->vnum = atoi(rest);
            }
            else if (strcmp(keyword, "Name") == 0)
            {
               /* tilde-terminated: rest starts with the value, tilde may be on same line */
               char *tilde = strchr(rest, '~');
               if (tilde)
               {
                  *tilde = '\0';
                  strncpy(it->name, rest, sizeof(it->name) - 1);
               }
               else
               {
                  strncpy(it->name, rest, sizeof(it->name) - 1);
                  /* read continuation until tilde */
                  size_t cur = strlen(it->name);
                  int c;
                  while ((c = fgetc(fp)) != EOF && c != '~')
                  {
                     if (cur + 1 < sizeof(it->name))
                        it->name[cur++] = (char)c;
                  }
                  it->name[cur] = '\0';
                  /* consume rest of line */
                  while ((c = fgetc(fp)) != EOF && c != '\n')
                     ;
               }
            }
            else if (strcmp(keyword, "ShortDescr") == 0)
            {
               char *tilde = strchr(rest, '~');
               if (tilde)
               {
                  *tilde = '\0';
                  strncpy(it->short_descr, rest, sizeof(it->short_descr) - 1);
               }
               else
               {
                  strncpy(it->short_descr, rest, sizeof(it->short_descr) - 1);
                  size_t cur = strlen(it->short_descr);
                  int c;
                  while ((c = fgetc(fp)) != EOF && c != '~')
                  {
                     if (cur + 1 < sizeof(it->short_descr))
                        it->short_descr[cur++] = (char)c;
                  }
                  it->short_descr[cur] = '\0';
                  while ((c = fgetc(fp)) != EOF && c != '\n')
                     ;
               }
            }
            else if (strcmp(keyword, "Description") == 0)
            {
               char *tilde = strchr(rest, '~');
               if (tilde)
               {
                  *tilde = '\0';
                  strncpy(it->description, rest, sizeof(it->description) - 1);
               }
               else
               {
                  strncpy(it->description, rest, sizeof(it->description) - 1);
                  size_t cur = strlen(it->description);
                  int c;
                  while ((c = fgetc(fp)) != EOF && c != '~')
                  {
                     if (cur + 1 < sizeof(it->description))
                        it->description[cur++] = (char)c;
                  }
                  it->description[cur] = '\0';
                  while ((c = fgetc(fp)) != EOF && c != '\n')
                     ;
               }
            }
            else if (strcmp(keyword, "ExtraFlags") == 0)
            {
               it->extra_flags = strtoull(rest, NULL, 10);
            }
            else if (strcmp(keyword, "WearFlags") == 0)
            {
               it->wear_flags = atoi(rest);
            }
            else if (strcmp(keyword, "WearLoc") == 0)
            {
               it->wear_loc = atoi(rest);
            }
            else if (strcmp(keyword, "ClassFlags") == 0)
            {
               it->class_flags = atoi(rest);
            }
            else if (strcmp(keyword, "ItemType") == 0)
            {
               it->item_type = atoi(rest);
            }
            else if (strcmp(keyword, "Weight") == 0)
            {
               it->weight = atoi(rest);
            }
            else if (strcmp(keyword, "Level") == 0)
            {
               it->level = atoi(rest);
            }
            else if (strcmp(keyword, "Timer") == 0)
            {
               it->timer = atoi(rest);
            }
            else if (strcmp(keyword, "Cost") == 0)
            {
               it->cost = atoi(rest);
            }
            else if (strcmp(keyword, "Values") == 0)
            {
               sscanf(rest, "%d %d %d %d", &it->value[0], &it->value[1], &it->value[2],
                      &it->value[3]);
            }
            else if (strcmp(keyword, "Objfun") == 0)
            {
               char *tilde = strchr(rest, '~');
               if (tilde)
                  *tilde = '\0';
               strncpy(it->objfun, rest, sizeof(it->objfun) - 1);
            }
            /* Spell N, ExtraDescr, Affect: skip (values already in value[] or not stored) */
            else if (strcmp(keyword, "ExtraDescr") == 0)
            {
               /* skip keyword~ and description~ */
               int c2;
               int tildes = 0;
               while ((c2 = fgetc(fp)) != EOF && tildes < 2)
                  if (c2 == '~')
                     tildes++;
               while ((c2 = fgetc(fp)) != EOF && c2 != '\n')
                  ;
            }
            else if (strcmp(keyword, "Affect") == 0)
            {
               /* skip 5 numbers on rest of line */
               /* already consumed */
            }
            /* else: unknown keyword, skip */
         }

         count++;
      }
   }

   fclose(fp);
   *items_out = items;
   return count;
}

/* -----------------------------------------------------------------------
 * Migrate one chest file into the database.
 * Returns 1 on success, 0 on failure.
 * ----------------------------------------------------------------------- */
static int migrate_chest(int file_vnum, const char *path)
{
   ChestItem *items = NULL;
   int nItems = parse_chest_file(path, &items);
   if (nItems <= 0)
   {
      free(items);
      return nItems == 0 ? 1 : 0; /* empty file is OK */
   }

   /* items[0] is the chest itself (Nest=0) */
   ChestItem *chest_item = &items[0];

   /* Extract owner name from short_descr: "<owner>'s Keep Chest" */
   char owner[256] = "";
   {
      const char *ap = strstr(chest_item->short_descr, "'s Keep Chest");
      if (ap)
      {
         size_t len = (size_t)(ap - chest_item->short_descr);
         if (len >= sizeof(owner))
            len = sizeof(owner) - 1;
         strncpy(owner, chest_item->short_descr, len);
         owner[len] = '\0';
      }
      else
      {
         strncpy(owner, chest_item->short_descr, sizeof(owner) - 1);
      }
   }

   int max_items = chest_item->value[3] > 0 ? chest_item->value[3] : 50;

   printf("  chest vnum=%d owner='%s' max_items=%d  (%d content items)\n", file_vnum, owner,
          max_items, nItems - 1);

   if (dry_run)
   {
      free(items);
      return 1;
   }

   /* --- BEGIN transaction -------------------------------------------- */
   if (!exec_sql("BEGIN"))
   {
      free(items);
      errors++;
      return 0;
   }

   /* --- Upsert keep_chests ------------------------------------------- */
   char vbuf[32], mibuf[32];
   snprintf(vbuf, sizeof(vbuf), "%d", file_vnum);
   snprintf(mibuf, sizeof(mibuf), "%d", max_items);
   {
      const char *params[3] = {vbuf, owner, mibuf};
      PGresult *res = exec_params("INSERT INTO keep_chests (vnum, owner_name, max_items)"
                                  " VALUES ($1, $2, $3)"
                                  " ON CONFLICT (vnum) DO UPDATE"
                                  "   SET owner_name=$2, max_items=$3, updated_at=NOW()"
                                  " RETURNING id",
                                  3, params);
      if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
      {
         fprintf(stderr, "migrate_chests: failed to upsert chest vnum=%d\n", file_vnum);
         PQclear(res);
         exec_sql("ROLLBACK");
         free(items);
         return 0;
      }
      char chest_id_s[32];
      snprintf(chest_id_s, sizeof(chest_id_s), "%s", PQgetvalue(res, 0, 0));
      PQclear(res);

      /* --- Delete existing items for this chest --------------------- */
      const char *dp[1] = {chest_id_s};
      PGresult *dr = exec_params("DELETE FROM keep_chest_items WHERE chest_id=$1", 1, dp);
      PQclear(dr);

      /* --- Insert content items (Nest>0) ----------------------------- */
      /* We track: for each nest level, the most recent item's DB id.
       * Parent of an item at Nest N is the most recent item at Nest N-1. */
      int nest_db_id[MAX_NEST_LEVELS]; /* DB id of most recent item at each nest */
      memset(nest_db_id, 0, sizeof(nest_db_id));

      for (int i = 1; i < nItems; i++)
      {
         ChestItem *it = &items[i];
         int nest = it->nest;

         if (nest < 1 || nest >= MAX_NEST_LEVELS)
            nest = 1;

         /* Determine parent_id */
         char parent_id_s[32];
         const char *parent_id_param = NULL;
         if (nest > 1 && nest_db_id[nest - 1] > 0)
         {
            snprintf(parent_id_s, sizeof(parent_id_s), "%d", nest_db_id[nest - 1]);
            parent_id_param = parent_id_s;
         }

         char nest_s[16], obj_vnum_s[32];
         char eflags_s[32], wflags_s[32], wloc_s[32], cflags_s[32];
         char itype_s[32], weight_s[32], level_s[32], timer_s[32], cost_s[32];
         char v[10][32], sort_s[32];
         char *objfun_param = it->objfun[0] ? it->objfun : NULL;

         snprintf(nest_s, sizeof(nest_s), "%d", nest);
         snprintf(obj_vnum_s, sizeof(obj_vnum_s), "%d", it->vnum);
         snprintf(eflags_s, sizeof(eflags_s), "%llu", it->extra_flags);
         snprintf(wflags_s, sizeof(wflags_s), "%d", it->wear_flags);
         snprintf(wloc_s, sizeof(wloc_s), "%d", it->wear_loc);
         snprintf(cflags_s, sizeof(cflags_s), "%d", it->class_flags);
         snprintf(itype_s, sizeof(itype_s), "%d", it->item_type);
         snprintf(weight_s, sizeof(weight_s), "%d", it->weight);
         snprintf(level_s, sizeof(level_s), "%d", it->level);
         snprintf(timer_s, sizeof(timer_s), "%d", it->timer);
         snprintf(cost_s, sizeof(cost_s), "%d", it->cost);
         for (int vi = 0; vi < 10; vi++)
            snprintf(v[vi], sizeof(v[vi]), "%d", it->value[vi]);
         snprintf(sort_s, sizeof(sort_s), "%d", i);

         const char *params2[27] = {chest_id_s,
                                    nest_s,
                                    parent_id_param,
                                    obj_vnum_s,
                                    it->name,
                                    it->short_descr,
                                    it->description,
                                    eflags_s,
                                    wflags_s,
                                    wloc_s,
                                    cflags_s,
                                    itype_s,
                                    weight_s,
                                    level_s,
                                    timer_s,
                                    cost_s,
                                    v[0],
                                    v[1],
                                    v[2],
                                    v[3],
                                    v[4],
                                    v[5],
                                    v[6],
                                    v[7],
                                    v[8],
                                    v[9],
                                    sort_s};

         PGresult *ir = exec_params("INSERT INTO keep_chest_items"
                                    "  (chest_id, nest, parent_id, vnum,"
                                    "   name, short_descr, description,"
                                    "   extra_flags, wear_flags, wear_loc, class_flags, item_type,"
                                    "   weight, level, timer, cost,"
                                    "   value_0, value_1, value_2, value_3,"
                                    "   value_4, value_5, value_6, value_7,"
                                    "   value_8, value_9, sort_order)"
                                    " VALUES"
                                    "  ($1, $2, $3, $4,"
                                    "   $5, $6, $7,"
                                    "   $8, $9, $10, $11, $12,"
                                    "   $13, $14, $15, $16,"
                                    "   $17, $18, $19, $20,"
                                    "   $21, $22, $23, $24,"
                                    "   $25, $26, $27)"
                                    " RETURNING id",
                                    27, params2);

         if (PQresultStatus(ir) != PGRES_TUPLES_OK || PQntuples(ir) == 0)
         {
            fprintf(stderr, "migrate_chests: failed to insert item %d for chest vnum=%d\n", i,
                    file_vnum);
            PQclear(ir);
            exec_sql("ROLLBACK");
            free(items);
            return 0;
         }

         int new_id = atoi(PQgetvalue(ir, 0, 0));
         PQclear(ir);

         /* Record this item's DB id at its nest level */
         if (nest < MAX_NEST_LEVELS)
            nest_db_id[nest] = new_id;
      }
   }

   if (!exec_sql("COMMIT"))
   {
      exec_sql("ROLLBACK");
      free(items);
      errors++;
      return 0;
   }

   free(items);
   return 1;
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
   const char *connstr = NULL;
   int only_vnum = 0; /* 0 = migrate all */

   /* Parse arguments */
   for (int i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "--dry-run") == 0)
      {
         dry_run = 1;
      }
      else if (strcmp(argv[i], "--vnum") == 0 && i + 1 < argc)
      {
         only_vnum = atoi(argv[++i]);
      }
      else if (argv[i][0] != '-')
      {
         connstr = argv[i];
      }
      else
      {
         fprintf(stderr, "Usage: migrate_chests [--dry-run] [--vnum N] [connstr]\n");
         return 1;
      }
   }

   /* Read connstr from data/db.conf if not provided */
   char conf_connstr[512] = "";
   if (!connstr)
   {
      FILE *cf = fopen("data/db.conf", "r");
      if (cf)
      {
         if (fgets(conf_connstr, sizeof(conf_connstr), cf))
         {
            conf_connstr[strcspn(conf_connstr, "\r\n")] = '\0';
            if (conf_connstr[0])
               connstr = conf_connstr;
         }
         fclose(cf);
      }
   }

   /* Connect to PostgreSQL */
   if (dry_run)
   {
      printf("migrate_chests: DRY RUN mode — no database writes.\n");
   }
   else
   {
      conn = PQconnectdb(connstr ? connstr : "");
      if (PQstatus(conn) != CONNECTION_OK)
      {
         fprintf(stderr, "migrate_chests: DB connection failed: %s\n", PQerrorMessage(conn));
         PQfinish(conn);
         return 1;
      }
      printf("migrate_chests: connected to database.\n");
   }

   /* Scan data/chest/ */
   const char *chest_dir = "data/chest";
   int migrated = 0;
   int skipped = 0;
   int failed = 0;

   if (only_vnum > 0)
   {
      char path[512];
      snprintf(path, sizeof(path), "%s/%d", chest_dir, only_vnum);
      printf("migrate_chests: migrating vnum %d from %s\n", only_vnum, path);
      if (migrate_chest(only_vnum, path))
         migrated++;
      else
         failed++;
   }
   else
   {
      DIR *dir = opendir(chest_dir);
      if (!dir)
      {
         fprintf(stderr, "migrate_chests: cannot open %s: %s\n", chest_dir, strerror(errno));
         if (!dry_run)
            PQfinish(conn);
         return 1;
      }

      struct dirent *entry;
      while ((entry = readdir(dir)) != NULL)
      {
         if (entry->d_name[0] == '.')
            continue;
         int file_vnum = atoi(entry->d_name);
         if (file_vnum <= 0)
            continue;

         char path[512];
         snprintf(path, sizeof(path), "%s/%s", chest_dir, entry->d_name);
         printf("migrate_chests: vnum %d ...\n", file_vnum);
         if (migrate_chest(file_vnum, path))
            migrated++;
         else
            failed++;
      }
      closedir(dir);
   }

   printf("migrate_chests: done. migrated=%d skipped=%d failed=%d errors=%d\n", migrated, skipped,
          failed, errors);

   if (!dry_run)
      PQfinish(conn);

   return (failed + errors) > 0 ? 1 : 0;
}
