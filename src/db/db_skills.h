/* db_skills.h — Skill table DB loader for ACK!TNG Lua scripting.
 *
 * Loads script_source fields from the skills table into skill_table[], then
 * hands off to lua_load_all_skill_scripts() to compile them into the Lua VM.
 */

#ifndef DEC_DB_SKILLS_H
#define DEC_DB_SKILLS_H 1

/* Load script_source for all skills from the DB and compile Lua scripts.
 * Must be called after db_conn_open() succeeds and after the Lua engine is
 * initialised (lua_engine_init()). */
void db_load_skill_scripts(void);

#endif /* DEC_DB_SKILLS_H */
