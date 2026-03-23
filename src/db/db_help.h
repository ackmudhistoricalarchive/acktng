#ifndef DB_HELP_H
#define DB_HELP_H 1

/* db_help.h — Runtime on-demand help/shelp/lore lookup via PostgreSQL.
 *
 * Compiled only when HAVE_LIBPQ is defined.  Provides a persistent
 * read/write PGconn* (the "help connection") that is opened after boot
 * completes and closed on shutdown.  All command handlers call these
 * functions directly instead of walking in-memory linked lists.
 */

#ifdef HAVE_LIBPQ

#include <stddef.h>

/* Open the help runtime connection.  area_dir is the running directory
 * (typically ".") used to locate data/db.conf.
 * Returns  1 on success,
 *          0 on connection failure,
 *         -1 if db.conf is absent (DB not configured).
 * Also populates top_help with a COUNT(*) from help_entries. */
int db_help_open(const char *area_dir);

/* Close the help connection on shutdown. */
void db_help_close(void);

/* Look up a help entry by keyword and trust level.
 * Tries exact match first, then prefix match.
 * Writes the matched keyword into kw_out and body text into text_out.
 * Returns 1 on success, 0 if not found. */
int db_help_lookup(const char *keyword, int level, char *kw_out, size_t kw_sz, char *text_out,
                   size_t text_sz, int *level_out);

/* Same as db_help_lookup but searches shelp_entries. */
int db_shelp_lookup(const char *keyword, int level, char *kw_out, size_t kw_sz, char *text_out,
                    size_t text_sz, int *level_out);

/* Look up a lore entry.  npc_flags drives variant selection (flag scoring).
 * Returns 1 on success, 0 if not found.  Writes body into text_out. */
int db_lore_lookup(const char *keyword, long npc_flags, char *text_out, size_t text_sz);

/* --- Build / OLC helpers ------------------------------------------------- */

/* Search help_entries for entries whose keywords contain arg.
 * Calls result_cb(rank, keyword, body_snippet) for each match. */
typedef void (*db_help_find_cb)(int rank, const char *keyword, const char *body_snippet,
                                void *userdata);
void db_help_find(const char *arg, db_help_find_cb cb, void *userdata);

/* Return the id, keyword, and body of the Nth (1-based) entry whose
 * keywords contain arg.  Returns 1 on success, 0 if not found. */
int db_help_find_nth(const char *arg, int n, int *id_out, char *kw_out, size_t kw_sz,
                     char *body_out, size_t body_sz);

/* Update the body of a help entry by id. Returns 1 on success. */
int db_help_update_body(int id, const char *body);

/* Insert a new help entry.  Returns the new id on success, 0 on failure. */
int db_help_insert(int level, const char *keywords, const char *body);

/* Collect lore entries whose flags are a strict subset of npc_flags.
 * Calls result_cb for each match (up to max_results), in descending score
 * order (score = popcount of matching flags).
 * Returns the number of results passed to the callback. */
int db_lore_collect_by_flags(long npc_flags, int max_results,
                             void (*result_cb)(const char *keyword, const char *body,
                                               void *userdata),
                             void *userdata);

#endif /* HAVE_LIBPQ */
#endif /* DB_HELP_H */
