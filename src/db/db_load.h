/* db_load.h — ACK!TNG PostgreSQL boot-time loader.
 *
 * All functions are guarded by HAVE_LIBPQ.  Callers must either be compiled
 * with HAVE_LIBPQ or wrap calls in #ifdef USE_DB_LOAD / #endif.
 *
 * The typical boot-db sequence when USE_DB_LOAD is defined:
 *   db_load_clans();           -- politics_data.diplomacy / treasury
 *   db_load_socials();         -- social_table[]
 *   db_load_areas_from_db();   -- areas + rooms + mobs + objects + resets …
 *   db_load_helps_from_db();   -- first_help / last_help
 *   db_load_shelps_from_db();  -- first_shelp / last_shelp
 *   db_load_lore_from_db();    -- first_lore / last_lore
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

/* Help / shelp / lore directories ----------------------------------------*/
void db_load_helps_from_db(void);
void db_load_shelps_from_db(void);
void db_load_lore_from_db(void);

/* Runtime data files ------------------------------------------------------*/
void db_load_bans(void);
void db_load_socials(void);
void db_load_clans(void);
void db_load_rulers(void);
void db_load_brands(void);
void db_load_sysdata(void);

#endif /* HAVE_LIBPQ */

#endif /* DEC_DB_LOAD_H */
