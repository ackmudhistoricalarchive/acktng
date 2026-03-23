/* db_load.h — ACK!TNG PostgreSQL boot-time loader.
 *
 * All functions are guarded by HAVE_LIBPQ.  When libpq is present the DB
 * loaders are the sole boot path; flat-file loaders are the #else fallback.
 *
 * The typical boot-db sequence (HAVE_LIBPQ):
 *   db_load_clans();           -- politics_data.diplomacy / treasury
 *   db_load_socials();         -- social_table[]
 *   db_load_areas_from_db();   -- areas + rooms + mobs + objects + resets …
 *   db_load_bans();            -- first_ban / last_ban
 *   db_load_rulers();          -- first_ruler_list / last_ruler_list
 *   db_load_brands();          -- first_brand / last_brand
 *   db_load_sysdata();         -- sysdata global
 */

#ifndef DEC_DB_LOAD_H
#define DEC_DB_LOAD_H 1

#ifdef HAVE_LIBPQ

/* World content -----------------------------------------------------------*/
void db_load_areas_from_db(void);

/* Runtime data files ------------------------------------------------------*/
void db_load_bans(void);
void db_load_socials(void);
void db_load_clans(void);
void db_load_rulers(void);
void db_load_brands(void);
void db_load_sysdata(void);
void db_load_boards(void);
void db_load_room_marks(void);
void db_load_corpses(void);
void db_load_chests(void);
void db_load_quest_templates(void);

#endif /* HAVE_LIBPQ */

#endif /* DEC_DB_LOAD_H */
