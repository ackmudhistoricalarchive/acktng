/* lua_api.h -- mud.* C API module for ACK!TNG Lua scripting.
 *
 * Declares the registration function called from lua_engine_init().
 * All mud.* functions are implemented in lua_api.c.
 */

#ifndef DEC_LUA_API_H
#define DEC_LUA_API_H 1

#include <lua.h>

/* Register the mud.* module into the Lua VM. */
void lua_register_mud_api(lua_State *L);

/* Push / extract game object pointers as Lua userdata.
 * Used by lua_engine.c when building spell/skill context tables. */
void lua_push_char(lua_State *L, CHAR_DATA *ch);
void lua_push_obj(lua_State *L, OBJ_DATA *obj);
void lua_push_room(lua_State *L, ROOM_INDEX_DATA *room);

CHAR_DATA *lua_check_char(lua_State *L, int idx);
OBJ_DATA *lua_check_obj(lua_State *L, int idx);
ROOM_INDEX_DATA *lua_check_room(lua_State *L, int idx);

#endif /* DEC_LUA_API_H */
