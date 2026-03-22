#ifndef DB_WORKER_H
#define DB_WORKER_H 1

/* db_worker.h — Async database worker thread for ACK!TNG.
 *
 * The MUD runs a single-threaded game loop; any synchronous libpq call would
 * block select() and lag all players.  This module provides a background
 * thread that owns the runtime PGconn* and processes a lock-protected queue
 * of read/write operations.
 *
 * Boot-time loads (areas, rooms, mobiles, etc.) are synchronous and use
 * db_conn.h directly, before game_loop() starts.
 *
 * Runtime flow:
 *   - Game thread calls db_worker_enqueue_write() or
 *     db_worker_enqueue_load_player() and returns immediately.
 *   - Worker thread wakes, executes the DB operation, posts results.
 *   - Game thread calls db_worker_poll_results() each tick to pick up
 *     completed player loads and advance the login state machine.
 *
 * Compiled only when HAVE_LIBPQ is defined.
 */

#ifdef HAVE_LIBPQ

/* Opaque forward declarations — db_worker.c includes ack.h for full types. */
struct descriptor_data;
struct char_data;

typedef enum
{
   /* Writes — game thread serialises data and enqueues */
   DB_WRITE_PLAYER,     /* save one player character */
   DB_WRITE_CLANS,      /* save all clan data        */
   DB_WRITE_BANS,       /* save ban list             */
   DB_WRITE_SOCIALS,    /* save social table         */
   DB_WRITE_CORPSES,    /* save corpse list          */
   DB_WRITE_SYSDATA,    /* save system data          */
   DB_WRITE_RULERS,     /* save ruler list           */
   DB_WRITE_BRANDS,     /* save brand list           */
   DB_WRITE_ROOM_MARKS, /* save room marks           */
   DB_WRITE_CHEST,      /* save one keep chest       */
   /* Reads — worker fetches and posts result */
   DB_READ_PLAYER, /* load one player at login */
   /* Control */
   DB_OP_SHUTDOWN, /* drain queue then exit thread */
} DB_OP_TYPE;

/* Result of an async player load, posted to the results queue. */
typedef struct db_player_result
{
   struct descriptor_data *d;
   struct char_data *ch; /* NULL if not found or on error */
   int error;            /* 1 if a DB error occurred */
   struct db_player_result *next;
} DB_PLAYER_RESULT;

/* Start the worker thread.  Call after all boot db_load_* are done.
 * connstr is a libpq connection string; NULL to re-read data/db.conf. */
void db_worker_start(const char *connstr);

/* Flush the queue, wait for the thread to exit. */
void db_worker_stop(void);

/* Enqueue a write operation.  Returns immediately.
 * buf/len is a deep copy of the serialised data blob.
 * name is the player name for DB_WRITE_PLAYER coalescing (may be NULL). */
void db_worker_enqueue_write(DB_OP_TYPE type, const void *buf, size_t len, const char *name);

/* Enqueue an async player load.  Sets d->connected = CON_LOADING_FROM_DB. */
void db_worker_enqueue_load_player(struct descriptor_data *d, const char *name);

/* Drain the results queue.  Call once per game tick from game_loop().
 * Advances login state machine for each completed load result. */
void db_worker_poll_results(void);

/* 1 if the worker has entered emergency fallback mode (DB connection lost). */
extern int db_worker_failed;

/* -----------------------------------------------------------------------
 * Convenience wrappers — game thread calls these instead of calling
 * db_worker_enqueue_write() directly.  Each serialises the given live
 * game data and enqueues the appropriate DB_WRITE_* operation.
 * All return immediately (non-blocking).
 * ----------------------------------------------------------------------- */

/* Save all site bans to the DB. */
void db_worker_save_bans(struct ban_data *first_ban_arg);

/* Save the full social table (count = maxSocial). */
void db_worker_save_socials(struct social_type *table, int count);

/* Save clan diplomacy and treasury (nclan = MAX_CLAN).
 * Callers must include config.h (via ack.h) before this header so that
 * MAX_CLAN is defined when the array dimension is evaluated. */
void db_worker_save_clans(const short diplomacy[][MAX_CLAN], const long *treasury, int nclan);

/* Save sysdata boolean flags. */
void db_worker_save_sysdata(int w_lock, int shownumbers);

/* Save ruler list. */
void db_worker_save_rulers(struct ruler_list *first_ruler_arg);

/* Save brand list. */
void db_worker_save_brands(struct dl_list *first_brand_arg);

/* Save room mark list. */
void db_worker_save_room_marks(struct mark_list_member *first_mark_arg);

/* Save one keep chest (and its contents). */
void db_worker_save_chest(struct obj_data *chest);

#endif /* HAVE_LIBPQ */

#endif /* DB_WORKER_H */
