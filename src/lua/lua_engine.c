/* lua_engine.c -- Lua 5.4 scripting engine for ACK!TNG.
 *
 * Manages a single shared lua_State for the lifetime of the server process.
 * Scripts are loaded from the skills DB table at boot, compiled to Lua
 * bytecode, and cached in the "skill_envs" registry table keyed by sn.
 *
 * Sandboxing:
 *   - Only base, table, string, math libraries are opened.
 *   - load(), dofile(), loadfile() are removed from _G.
 *   - require() is replaced with a sandbox that loads only from lua_libraries.
 *   - An instruction-count hook aborts scripts exceeding MAX_LUA_INSTRUCTIONS.
 *   - A reentrancy guard prevents nested Lua dispatch from corrupting the VM.
 */

#include "globals.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <libpq-fe.h>

#include "../db/db_conn.h"
#include "lua_api.h"
#include "lua_engine.h"

/* Forward declarations for metatable registration (each in its own .c). */
void lua_register_char_metatable(lua_State *L);
void lua_register_obj_metatable(lua_State *L);
void lua_register_room_metatable(lua_State *L);
void lua_register_constants(lua_State *L);

/* The global Lua VM — a single state shared across all spell/skill calls. */
lua_State *lua_L = NULL;

/* Reentrancy guard: TRUE while a script is executing. */
static bool lua_executing = FALSE;

/* ---- Instruction limit hook --------------------------------------------- */

static void lua_instruction_hook(lua_State *L, lua_Debug *ar)
{
   (void)ar;
   luaL_error(L, "Lua: script exceeded instruction limit (%d instructions)", MAX_LUA_INSTRUCTIONS);
}

/* ---- Sandboxed require() ------------------------------------------------- */

/* Loads only from the "skill_libs" registry table, which is populated at
 * boot from the lua_libraries DB table.  Never touches the filesystem. */
static int lua_custom_require(lua_State *L)
{
   const char *modname = luaL_checkstring(L, 1);

   /* Return cached result if already loaded. */
   luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
   lua_getfield(L, -1, modname);
   if (!lua_isnil(L, -1))
      return 1; /* already in _LOADED */
   lua_pop(L, 1);

   /* Look up module source in "skill_libs" registry table. */
   luaL_getsubtable(L, LUA_REGISTRYINDEX, "skill_libs");
   lua_getfield(L, -1, modname);
   if (lua_isnil(L, -1))
   {
      lua_pop(L, 2);
      return luaL_error(L, "module '%s' not found in lua_libraries", modname);
   }

   const char *src = lua_tostring(L, -1);
   lua_pop(L, 2); /* src string + skill_libs */

   /* Compile and execute module source. */
   char chunk_name[128];
   snprintf(chunk_name, sizeof(chunk_name), "lib:%s", modname);
   if (luaL_loadbuffer(L, src, strlen(src), chunk_name) != LUA_OK)
      return lua_error(L); /* propagate compile error */

   if (lua_pcall(L, 0, 1, 0) != LUA_OK)
      return lua_error(L);

   /* Cache in _LOADED and return. */
   luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
   lua_pushvalue(L, -2);
   lua_setfield(L, -2, modname);
   lua_pop(L, 1);
   return 1;
}

/* ---- Lua VM lifecycle ---------------------------------------------------- */

void lua_engine_init(void)
{
   if (lua_L)
      return; /* already initialised */

   lua_L = luaL_newstate();
   if (!lua_L)
   {
      log_string("lua_engine_init: luaL_newstate() failed");
      return;
   }

   /* Open only safe standard libraries. */
   luaL_requiref(lua_L, "base", luaopen_base, 1);
   lua_pop(lua_L, 1);
   luaL_requiref(lua_L, "table", luaopen_table, 1);
   lua_pop(lua_L, 1);
   luaL_requiref(lua_L, "string", luaopen_string, 1);
   lua_pop(lua_L, 1);
   luaL_requiref(lua_L, "math", luaopen_math, 1);
   lua_pop(lua_L, 1);

   /* Remove dangerous globals that could escape the sandbox. */
   lua_pushnil(lua_L);
   lua_setglobal(lua_L, "dofile");
   lua_pushnil(lua_L);
   lua_setglobal(lua_L, "loadfile");
   lua_pushnil(lua_L);
   lua_setglobal(lua_L, "load"); /* can compile arbitrary Lua strings */

   /* Replace require() with a sandboxed version. */
   lua_pushcfunction(lua_L, lua_custom_require);
   lua_setglobal(lua_L, "require");

   /* Register game API modules. */
   lua_register_mud_api(lua_L);
   lua_register_char_metatable(lua_L);
   lua_register_obj_metatable(lua_L);
   lua_register_room_metatable(lua_L);
   lua_register_constants(lua_L);

   log_string("Lua 5.4 engine initialised.");
}

void lua_engine_shutdown(void)
{
   if (!lua_L)
      return;
   lua_close(lua_L);
   lua_L = NULL;
}

/* ---- Script loading and caching ----------------------------------------- */

/* Store a library module source in the "skill_libs" registry table so that
 * the sandboxed require() can find it by name. */
static void lua_cache_library(const char *name, const char *source)
{
   luaL_getsubtable(lua_L, LUA_REGISTRYINDEX, "skill_libs");
   lua_pushstring(lua_L, source);
   lua_setfield(lua_L, -2, name);
   lua_pop(lua_L, 1);
}

/* Compile one skill/spell script from source text.  The compiled execute()
 * function is stored in the "skill_envs" registry table, keyed by sn.
 * Returns TRUE on success. */
bool lua_load_skill_script(int sn, const char *source, const char *name)
{
   if (!lua_L || !source || source[0] == '\0')
      return FALSE;

   /* Compile the source string to a Lua chunk. */
   if (luaL_loadbuffer(lua_L, source, strlen(source), name) != LUA_OK)
   {
      log_f("Lua compile error [%s]: %s", name, lua_tostring(lua_L, -1));
      lua_pop(lua_L, 1);
      return FALSE;
   }
   /* Stack: [chunk] */

   /* Create an isolated environment for this script.
    * env inherits from _G via __index so game constants are visible. */
   lua_newtable(lua_L); /* [chunk, env] */
   lua_newtable(lua_L); /* [chunk, env, mt] */
   lua_getglobal(lua_L, "_G");
   lua_setfield(lua_L, -2, "__index"); /* mt.__index = _G */
   lua_setmetatable(lua_L, -2);        /* env mt set; [chunk, env] */

   /* Save env to registry BEFORE setupvalue removes it from the stack. */
   luaL_getsubtable(lua_L, LUA_REGISTRYINDEX, "skill_envs"); /* [chunk, env, skill_envs] */
   lua_pushvalue(lua_L, -2);                                 /* [chunk, env, skill_envs, env] */
   lua_rawseti(lua_L, -2, sn); /* skill_envs[sn] = env;       [chunk, env, skill_envs] */
   lua_pop(lua_L, 1);          /* [chunk, env] */

   /* Set env as the chunk's _ENV upvalue (this pops env from the stack). */
   lua_setupvalue(lua_L, -2, 1); /* [chunk] */

   /* Execute the chunk so that execute() and any locals get defined in env. */
   if (lua_pcall(lua_L, 0, 0, 0) != LUA_OK)
   {
      log_f("Lua runtime error [%s]: %s", name, lua_tostring(lua_L, -1));
      lua_pop(lua_L, 1);
      /* Remove the failed registry entry. */
      luaL_getsubtable(lua_L, LUA_REGISTRYINDEX, "skill_envs");
      lua_pushnil(lua_L);
      lua_rawseti(lua_L, -2, sn);
      lua_pop(lua_L, 1);
      return FALSE;
   }
   /* Stack: [] — env is in registry, execute() is defined inside it. */
   return TRUE;
}

/* Retrieve a named function from a skill's cached environment and push it
 * onto the stack.  Returns TRUE if the function was found. */
static bool lua_get_script_function(lua_State *L, int sn, const char *fname)
{
   luaL_getsubtable(L, LUA_REGISTRYINDEX, "skill_envs"); /* [skill_envs] */
   lua_rawgeti(L, -1, sn);                               /* [skill_envs, env] */
   lua_remove(L, -2);                                    /* [env] */
   if (!lua_istable(L, -1))
   {
      lua_pop(L, 1);
      return FALSE;
   }
   lua_getfield(L, -1, fname); /* [env, fn] */
   lua_remove(L, -2);          /* [fn] */
   if (!lua_isfunction(L, -1))
   {
      lua_pop(L, 1);
      return FALSE;
   }
   return TRUE;
}

/* Load all shared library modules from lua_libraries, then compile every
 * skill/spell that has a non-empty script_source. */
void lua_load_all_skill_scripts(void)
{
   if (!lua_L)
      return;

   /* Load shared libraries from the lua_libraries DB table. */
   PGconn *db = db_conn_get();
   if (db)
   {
      PGresult *res = PQexec(db, "SELECT name, source FROM lua_libraries ORDER BY name");
      if (PQresultStatus(res) == PGRES_TUPLES_OK)
      {
         int rows = PQntuples(res);
         for (int i = 0; i < rows; i++)
         {
            const char *lib_name = PQgetvalue(res, i, 0);
            const char *lib_src = PQgetvalue(res, i, 1);
            lua_cache_library(lib_name, lib_src);
         }
         log_f("Lua: loaded %d shared library module(s).", rows);
      }
      PQclear(res);
   }

   /* Compile each skill/spell that has a Lua script. */
   int loaded = 0;
   for (int sn = 0; sn < MAX_SKILL; sn++)
   {
      if (!skill_table[sn].name)
         continue;
      if (!skill_scripts[sn] || skill_scripts[sn][0] == '\0')
         continue;
      if (lua_load_skill_script(sn, skill_scripts[sn], skill_table[sn].name))
         loaded++;
   }
   if (loaded > 0)
      log_f("Lua: compiled %d skill/spell script(s).", loaded);
}

/* ---- Spell dispatch ------------------------------------------------------ */

bool lua_spell_execute(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   if (!lua_L)
      return FALSE;
   if (!skill_scripts[sn] || skill_scripts[sn][0] == '\0')
      return FALSE;

   if (lua_executing)
   {
      log_f("Lua: reentrant call to lua_spell_execute [%s] — skipped", skill_table[sn].name);
      return FALSE;
   }

   if (!lua_get_script_function(lua_L, sn, "execute"))
   {
      log_f("Lua: no execute() for spell [%s]", skill_table[sn].name);
      return FALSE;
   }

   /* Build context table. */
   lua_newtable(lua_L);
   lua_pushinteger(lua_L, sn);
   lua_setfield(lua_L, -2, "sn");
   lua_pushinteger(lua_L, level);
   lua_setfield(lua_L, -2, "level");
   lua_pushinteger(lua_L, skill_table[sn].beats);
   lua_setfield(lua_L, -2, "beats");
   lua_push_char(lua_L, ch);
   lua_setfield(lua_L, -2, "ch");

   if (skill_table[sn].target == TAR_CHAR_OFFENSIVE ||
       skill_table[sn].target == TAR_CHAR_DEFENSIVE || skill_table[sn].target == TAR_CHAR_SELF ||
       skill_table[sn].target == TAR_CHAR_NOTSELF)
   {
      lua_push_char(lua_L, (CHAR_DATA *)vo);
      lua_setfield(lua_L, -2, "victim");
   }
   else if (skill_table[sn].target == TAR_OBJ_INV)
   {
      lua_push_obj(lua_L, (OBJ_DATA *)vo);
      lua_setfield(lua_L, -2, "obj_target");
   }
   if (obj)
   {
      lua_push_obj(lua_L, obj);
      lua_setfield(lua_L, -2, "cast_obj");
   }

   lua_sethook(lua_L, lua_instruction_hook, LUA_MASKCOUNT, MAX_LUA_INSTRUCTIONS);
   lua_executing = TRUE;

   bool result = FALSE;
   if (lua_pcall(lua_L, 1, 1, 0) != LUA_OK)
   {
      const char *err = lua_tostring(lua_L, -1);
      log_f("Lua spell error [%s]: %s", skill_table[sn].name, err ? err : "(unknown)");
      if (ch->desc && ch->level >= LEVEL_STAFF)
         send_to_char(err ? err : "(Lua error)", ch);
      lua_pop(lua_L, 1);
   }
   else
   {
      result = lua_toboolean(lua_L, -1);
      lua_pop(lua_L, 1);
   }

   lua_executing = FALSE;
   lua_sethook(lua_L, NULL, 0, 0);
   return result;
}

/* ---- Skill dispatch ------------------------------------------------------ */

void lua_skill_execute(int sn, CHAR_DATA *ch, char *argument)
{
   if (!lua_L)
      return;
   if (!skill_scripts[sn] || skill_scripts[sn][0] == '\0')
      return;

   if (lua_executing)
   {
      log_f("Lua: reentrant call to lua_skill_execute [%s] — skipped", skill_table[sn].name);
      return;
   }

   if (!lua_get_script_function(lua_L, sn, "execute"))
   {
      log_f("Lua: no execute() for skill [%s]", skill_table[sn].name);
      return;
   }

   lua_newtable(lua_L);
   lua_pushinteger(lua_L, sn);
   lua_setfield(lua_L, -2, "sn");
   lua_pushinteger(lua_L, skill_table[sn].beats);
   lua_setfield(lua_L, -2, "beats");
   lua_push_char(lua_L, ch);
   lua_setfield(lua_L, -2, "ch");
   lua_pushstring(lua_L, argument ? argument : "");
   lua_setfield(lua_L, -2, "argument");

   lua_sethook(lua_L, lua_instruction_hook, LUA_MASKCOUNT, MAX_LUA_INSTRUCTIONS);
   lua_executing = TRUE;

   if (lua_pcall(lua_L, 1, 1, 0) != LUA_OK)
   {
      const char *err = lua_tostring(lua_L, -1);
      log_f("Lua skill error [%s]: %s", skill_table[sn].name, err ? err : "(unknown)");
      if (ch->desc && ch->level >= LEVEL_STAFF)
         send_to_char(err ? err : "(Lua error)", ch);
      lua_pop(lua_L, 1);
   }
   else
   {
      lua_pop(lua_L, 1); /* discard return value */
   }

   lua_executing = FALSE;
   lua_sethook(lua_L, NULL, 0, 0);
}

/* ---- Hot reload ---------------------------------------------------------- */

void do_luareload(CHAR_DATA *ch, char *argument)
{
   if (!lua_L)
   {
      send_to_char("Lua engine is not initialised.\n\r", ch);
      return;
   }

   if (argument[0] == '\0' || !str_cmp(argument, "all"))
   {
      lua_load_all_skill_scripts();
      send_to_char("All Lua scripts and libraries reloaded from database.\n\r", ch);
      return;
   }

   int sn = skill_lookup(argument);
   if (sn < 0)
   {
      send_to_char("No such skill or spell.\n\r", ch);
      return;
   }

   if (!skill_scripts[sn] || skill_scripts[sn][0] == '\0')
   {
      send_to_char("That skill has no Lua script.\n\r", ch);
      return;
   }

   if (lua_load_skill_script(sn, skill_scripts[sn], skill_table[sn].name))
      send_to_char("Script reloaded.\n\r", ch);
   else
      send_to_char("Failed to reload script — check logs.\n\r", ch);
}
