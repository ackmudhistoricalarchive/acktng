/* lua_room.c -- ROOM_INDEX_DATA userdata for ACK!TNG Lua scripting.
 *
 * Exposes room properties to Lua scripts as read-only method calls on a
 * room userdata value (e.g. room:get_vnum(), room:get_name()).
 */

#include "globals.h"

#include <lauxlib.h>
#include <lua.h>

#include "lua_api.h"

#define ROOM_MT "ack.room"

/* Push a ROOM_INDEX_DATA* as a Lua userdata. */
void lua_push_room(lua_State *L, ROOM_INDEX_DATA *room)
{
   if (!room)
   {
      lua_pushnil(L);
      return;
   }
   ROOM_INDEX_DATA **ud = (ROOM_INDEX_DATA **)lua_newuserdata(L, sizeof(ROOM_INDEX_DATA *));
   *ud = room;
   luaL_setmetatable(L, ROOM_MT);
}

/* Extract and validate a room userdata from the stack. */
ROOM_INDEX_DATA *lua_check_room(lua_State *L, int idx)
{
   ROOM_INDEX_DATA **ud = (ROOM_INDEX_DATA **)luaL_checkudata(L, idx, ROOM_MT);
   if (!ud || !*ud)
      luaL_error(L, "invalid room userdata");
   return *ud;
}

/* room:get_vnum() */
static int room_get_vnum(lua_State *L)
{
   ROOM_INDEX_DATA *room = lua_check_room(L, 1);
   lua_pushinteger(L, room->vnum);
   return 1;
}

/* room:get_name() */
static int room_get_name(lua_State *L)
{
   ROOM_INDEX_DATA *room = lua_check_room(L, 1);
   lua_pushstring(L, room->name ? room->name : "");
   return 1;
}

static const luaL_Reg room_methods[] = {
    {"get_vnum", room_get_vnum}, {"get_name", room_get_name}, {NULL, NULL}};

void lua_register_room_metatable(lua_State *L)
{
   luaL_newmetatable(L, ROOM_MT);
   luaL_setfuncs(L, room_methods, 0);
   lua_pushvalue(L, -1);
   lua_setfield(L, -2, "__index");
   lua_pop(L, 1);
}
