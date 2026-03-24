/* db_skills.c — Load skill script_source from PostgreSQL at boot.
 *
 * Queries the skills table for rows with a non-null script_source and
 * populates skill_table[sn].script_source.  Then hands off to
 * lua_load_all_skill_scripts() to compile them into the Lua VM.
 */

#include "globals.h"

#include <libpq-fe.h>

#include "../lua/lua_engine.h"
#include "db_conn.h"
#include "db_skills.h"

/* Lua script source for each skill, indexed by sn.  NULL = use C function. */
char *skill_scripts[MAX_SKILL];

void db_load_skill_scripts(void)
{
   PGconn *db = db_conn_get();
   if (!db)
   {
      log_string("db_load_skill_scripts: no DB connection — skipping");
      return;
   }

   PGresult *res = PQexec(db, "SELECT sn, script_source"
                              "  FROM skills"
                              " WHERE script_source IS NOT NULL"
                              "   AND script_source <> ''");

   if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      db_log_error("db_load_skill_scripts", res);
      if (res)
         PQclear(res);
      return;
   }

   int rows = PQntuples(res);
   int loaded = 0;

   for (int i = 0; i < rows; i++)
   {
      int sn = atoi(PQgetvalue(res, i, 0));
      const char *src = PQgetvalue(res, i, 1);

      if (sn < 0 || sn >= MAX_SKILL)
      {
         log_f("db_load_skill_scripts: sn %d out of range — skipped", sn);
         continue;
      }
      if (!skill_table[sn].name)
      {
         log_f("db_load_skill_scripts: sn %d has no name in skill_table — skipped", sn);
         continue;
      }

      /* Free any previous source (hot-reload path). */
      if (skill_scripts[sn])
         free_string(skill_scripts[sn]);

      skill_scripts[sn] = str_dup(src);
      loaded++;
   }

   PQclear(res);
   log_f("DB: loaded script_source for %d skill(s).", loaded);

   /* Compile all loaded scripts into the Lua VM. */
   lua_load_all_skill_scripts();
}
