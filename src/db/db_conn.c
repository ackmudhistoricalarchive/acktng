#ifdef HAVE_LIBPQ

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

#include "db_conn.h"

static PGconn *boot_conn = NULL;

/* Read the connection string from <area_dir>/../data/db.conf.
 * Caller must free() the returned string.  Returns NULL if the file is absent
 * (callers should fall back to PG environment variables in that case). */
static char *read_db_conf(const char *area_dir)
{
   char path[512];
   FILE *fp;
   long file_size;
   char *buf;
   size_t n;

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
   /* Strip trailing whitespace / newline */
   while (n > 0 &&
          (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ' || buf[n - 1] == '\t'))
   {
      buf[--n] = '\0';
   }
   return buf;
}

int db_conn_open(const char *area_dir)
{
   char *connstr;
   PGresult *res;
   int ver;

   connstr = read_db_conf(area_dir);
   if (!connstr)
   {
      /* data/db.conf absent — DB is disabled for this boot */
      return -1;
   }
   boot_conn = PQconnectdb(connstr);
   free(connstr);

   if (PQstatus(boot_conn) != CONNECTION_OK)
   {
      fprintf(stderr, "DB: connection failed: %s\n", PQerrorMessage(boot_conn));
      PQfinish(boot_conn);
      boot_conn = NULL;
      return 0;
   }

   /* Check schema_version */
   res = PQexec(boot_conn, "SELECT version FROM schema_version ORDER BY applied_at DESC "
                           "LIMIT 1");
   if (PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      fprintf(stderr, "DB: schema_version query failed: %s\n", PQerrorMessage(boot_conn));
      PQclear(res);
      PQfinish(boot_conn);
      boot_conn = NULL;
      return 0;
   }

   if (PQntuples(res) == 0)
   {
      fprintf(stderr, "DB: schema_version table is empty — has schema.sql been "
                      "applied?\n");
      PQclear(res);
      PQfinish(boot_conn);
      boot_conn = NULL;
      return 0;
   }

   ver = atoi(PQgetvalue(res, 0, 0));
   PQclear(res);

   if (ver != DB_SCHEMA_VERSION)
   {
      fprintf(stderr,
              "DB: schema version mismatch: database has %d, binary "
              "expects %d\n",
              ver, DB_SCHEMA_VERSION);
      PQfinish(boot_conn);
      boot_conn = NULL;
      return 0;
   }

   fprintf(stderr, "DB: connected (schema v%d)\n", ver);
   return 1;
}

PGconn *db_conn_get(void)
{
   return boot_conn;
}

void db_conn_close(void)
{
   if (boot_conn)
   {
      PQfinish(boot_conn);
      boot_conn = NULL;
   }
}

PGresult *db_query(const char *sql)
{
   PGresult *res;
   if (!boot_conn)
   {
      fprintf(stderr, "DB: db_query called with no connection\n");
      return NULL;
   }
   res = PQexec(boot_conn, sql);
   if (!res)
   {
      fprintf(stderr, "DB: PQexec returned NULL: %s\n", PQerrorMessage(boot_conn));
      return NULL;
   }
   return res;
}

PGresult *db_query_params(const char *sql, int nParams, const char *const *paramValues)
{
   PGresult *res;
   if (!boot_conn)
   {
      fprintf(stderr, "DB: db_query_params called with no connection\n");
      return NULL;
   }
   res = PQexecParams(boot_conn, sql, nParams, NULL, paramValues, NULL, NULL, 0);
   if (!res)
   {
      fprintf(stderr, "DB: PQexecParams returned NULL: %s\n", PQerrorMessage(boot_conn));
      return NULL;
   }
   return res;
}

int db_log_error(const char *context, PGresult *res)
{
   if (res)
   {
      fprintf(stderr, "DB error [%s]: %s\n", context, PQresultErrorMessage(res));
   }
   else if (boot_conn)
   {
      fprintf(stderr, "DB error [%s]: %s\n", context, PQerrorMessage(boot_conn));
   }
   else
   {
      fprintf(stderr, "DB error [%s]: no connection\n", context);
   }
   return 0;
}

#endif /* HAVE_LIBPQ */
