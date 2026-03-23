#ifndef DB_CONN_H
#define DB_CONN_H 1

/* db_conn.h — PostgreSQL connection management for ACK!TNG.
 *
 * Compiled only when HAVE_LIBPQ is defined (libpq-dev is present).
 * All other code guards DB calls with #ifdef HAVE_LIBPQ.
 *
 * Boot path (synchronous):
 *   db_conn_open()  — open the connection, apply schema if needed,
 *                     check schema_version.
 *   db_conn_get()   — return the PGconn* for boot-time queries.
 *   db_conn_close() — close after all db_load_* calls complete.
 *
 * Config file: $ACK_DB_CONF if set, otherwise <area_dir>/../data/db.conf.
 * DB is disabled if the file does not exist.
 */

#ifdef HAVE_LIBPQ

#include <libpq-fe.h>

/* Expected schema version compiled into this binary. */
#define DB_SCHEMA_VERSION 7

/* Open the boot connection.
 * Returns  1 on success,
 *          0 on failure (db.conf present but connection/schema error),
 *         -1 if db.conf is absent (DB disabled; caller should skip DB work).
 * On failure a descriptive message is printed to stderr. */
int db_conn_open(const char *area_dir);

/* Return the active boot PGconn*.  NULL if not connected. */
PGconn *db_conn_get(void);

/* Close the boot connection after all db_load_* calls are done. */
void db_conn_close(void);

/* Execute a query and return the result; caller must PQclear() it.
 * Returns NULL on connection error (logs the error). */
PGresult *db_query(const char *sql);

/* Execute a parameterised query.  nParams, paramValues as per PQexecParams.
 * Caller must PQclear() the result.  Returns NULL on connection error. */
PGresult *db_query_params(const char *sql, int nParams, const char *const *paramValues);

/* Log a PQ error and return 0 (convenience for error-path code). */
int db_log_error(const char *context, PGresult *res);

#endif /* HAVE_LIBPQ */

#endif /* DB_CONN_H */
