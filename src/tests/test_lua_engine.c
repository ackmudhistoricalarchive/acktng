/* test_lua_engine.c -- Unit tests for the Lua 5.4 scripting engine.
 *
 * Verifies lua_engine_init(), lua_load_skill_script(), and
 * lua_engine_shutdown() using a minimal stub environment.
 *
 * The five game-API registration functions (mud, char, obj, room, constants)
 * are stubbed here so we don't need the full game link graph.  Only
 * lua_engine.c and liblua5.4 are linked.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DEC_GLOBALS_H 1
#include "ack.h"
#include "lua/lua_engine.h"

/* ---- Required global stubs --------------------------------------------- */

bool fBootDb = FALSE;

/* skill_scripts is defined in db_skills.c in production; we own it here. */
char *skill_scripts[MAX_SKILL];

/* Minimal skill_table — all entries zeroed (name == NULL). */
const struct skill_type skill_table[MAX_SKILL];

void log_string(const char *str)
{
   (void)str;
}

void log_f(char *fmt, ...)
{
   (void)fmt;
}

void bugf(char *fmt, ...)
{
   (void)fmt;
}

void tail_chain(void)
{
}

/* ---- Stub out game-API registration functions --------------------------- */

/* lua_api.c dependencies are replaced with no-ops so we don't pull in the
 * full combat/heal/affect/object subsystems. */
void lua_register_mud_api(lua_State *L)
{
   (void)L;
}

/* lua_char.c — would need get_max_hp, get_curr_str, etc. */
void lua_register_char_metatable(lua_State *L)
{
   (void)L;
}

/* lua_obj.c */
void lua_register_obj_metatable(lua_State *L)
{
   (void)L;
}

/* lua_room.c */
void lua_register_room_metatable(lua_State *L)
{
   (void)L;
}

/* lua_constants.c — needs ELE_*, AFF_*, etc. */
void lua_register_constants(lua_State *L)
{
   (void)L;
}

/* ---- Tests -------------------------------------------------------------- */

static void test_init_shutdown(void)
{
   assert(lua_L == NULL);
   lua_engine_init();
   assert(lua_L != NULL);
   lua_engine_shutdown();
   assert(lua_L == NULL);

   /* Second init should work fine. */
   lua_engine_init();
   assert(lua_L != NULL);
   lua_engine_shutdown();
   assert(lua_L == NULL);
}

static void test_load_script_with_global(void)
{
   /* Script that defines a global and uses the ELE table (which is stubbed to
    * empty — just verify no crash when globals aren't populated). */
   const char *src = "local x = 42\nfunction execute(ctx) return x > 0 end";
   lua_engine_init();
   assert(lua_load_skill_script(1, src, "test-with-local"));
   lua_engine_shutdown();
}

static void test_load_script_syntax_error(void)
{
   lua_engine_init();
   /* Lua syntax error should return FALSE without crashing. */
   assert(!lua_load_skill_script(0, "this is not valid lua @@@@", "syntax-error-test"));
   lua_engine_shutdown();
}

static void test_load_script_success(void)
{
   const char *src = "function execute(ctx) return true end";

   lua_engine_init();
   assert(lua_load_skill_script(0, src, "test-spell"));
   lua_engine_shutdown();
}

static void test_load_empty_source(void)
{
   lua_engine_init();
   /* Empty source is a no-op (returns FALSE). */
   assert(!lua_load_skill_script(0, "", "empty"));
   assert(!lua_load_skill_script(0, NULL, "null"));
   lua_engine_shutdown();
}

static void test_reinit_is_noop(void)
{
   lua_engine_init();
   lua_State *first = lua_L;
   lua_engine_init(); /* Should not create a second state. */
   assert(lua_L == first);
   lua_engine_shutdown();
}

int main(void)
{
   test_init_shutdown();
   test_load_script_with_global();
   test_load_script_syntax_error();
   test_load_script_success();
   test_load_empty_source();
   test_reinit_is_noop();

   printf("test_lua_engine: all tests passed.\n");
   return 0;
}
