/* lua_engine.h -- Lua 5.4 scripting engine for ACK!TNG.
 *
 * Public interface used by db.c (init/shutdown/load), magic.c (spell
 * dispatch), interp.c (skill dispatch), and act_wiz.c (luareload command).
 *
 * All functions declared here are implemented in lua_engine.c.
 */

#ifndef DEC_LUA_ENGINE_H
#define DEC_LUA_ENGINE_H 1

#include <lua.h>

/* Expose the global VM so that lua_api.c / lua_char.c etc. can access it
 * without a separate accessor function. */
extern lua_State *lua_L;

/* Instruction limit per script call (prevents runaway scripts from hanging
 * the game loop). Tune via MAX_LUA_INSTRUCTIONS if needed. */
#define MAX_LUA_INSTRUCTIONS 100000

/* Lifecycle ----------------------------------------------------------------*/
void lua_engine_init(void);
void lua_engine_shutdown(void);

/* Script loading -----------------------------------------------------------*/

/* Load and compile one script from source text; cache its execute()
 * function in the registry keyed by sn.  Returns TRUE on success. */
bool lua_load_skill_script(int sn, const char *source, const char *name);

/* Load all shared library modules from the lua_libraries DB table, then
 * compile every non-empty skill_table[sn].script_source. */
void lua_load_all_skill_scripts(void);

/* Dispatch ----------------------------------------------------------------*/

/* Call the Lua execute() function for a spell.  Returns FALSE when the
 * skill has no script or the script returns false/errors. */
bool lua_spell_execute(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj);

/* Call the Lua execute() function for a skill command. */
void lua_skill_execute(int sn, CHAR_DATA *ch, char *argument);

/* In-game reload command --------------------------------------------------*/
void do_luareload(CHAR_DATA *ch, char *argument);

#endif /* DEC_LUA_ENGINE_H */
