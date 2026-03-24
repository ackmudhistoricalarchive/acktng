/* lua_obj.c -- OBJ_DATA userdata for ACK!TNG Lua scripting.
 *
 * Exposes object properties to Lua scripts as read-only method calls on
 * an obj userdata value.
 */

#include "globals.h"

#include <lauxlib.h>
#include <lua.h>

#include "lua_api.h"

#define OBJ_MT "ack.obj"

void lua_push_obj(lua_State *L, OBJ_DATA *obj)
{
   if (!obj)
   {
      lua_pushnil(L);
      return;
   }
   OBJ_DATA **ud = (OBJ_DATA **)lua_newuserdata(L, sizeof(OBJ_DATA *));
   *ud = obj;
   luaL_setmetatable(L, OBJ_MT);
}

OBJ_DATA *lua_check_obj(lua_State *L, int idx)
{
   OBJ_DATA **ud = (OBJ_DATA **)luaL_checkudata(L, idx, OBJ_MT);
   if (!ud || !*ud)
      luaL_error(L, "invalid obj userdata");
   return *ud;
}

static int obj_get_name(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   lua_pushstring(L, obj->name ? obj->name : "");
   return 1;
}

static int obj_get_short_descr(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   lua_pushstring(L, obj->short_descr ? obj->short_descr : "");
   return 1;
}

static int obj_get_item_type(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   lua_pushinteger(L, obj->item_type);
   return 1;
}

static int obj_get_level(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   lua_pushinteger(L, obj->level);
   return 1;
}

/* obj:get_value(index) -- returns value[index], index 0-9 */
static int obj_get_value(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   int idx = (int)luaL_checkinteger(L, 2);
   if (idx < 0 || idx > 9)
      luaL_error(L, "obj:get_value index out of range (0-9)");
   lua_pushinteger(L, obj->value[idx]);
   return 1;
}

static int obj_get_in_room(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   lua_push_room(L, obj->in_room);
   return 1;
}

static int obj_get_carried_by(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   lua_push_char(L, obj->carried_by);
   return 1;
}

static int obj_get_wear_loc(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   lua_pushinteger(L, obj->wear_loc);
   return 1;
}

static const luaL_Reg obj_methods[] = {{"get_name", obj_get_name},
                                       {"get_short_descr", obj_get_short_descr},
                                       {"get_item_type", obj_get_item_type},
                                       {"get_level", obj_get_level},
                                       {"get_value", obj_get_value},
                                       {"get_in_room", obj_get_in_room},
                                       {"get_carried_by", obj_get_carried_by},
                                       {"get_wear_loc", obj_get_wear_loc},
                                       {NULL, NULL}};

void lua_register_obj_metatable(lua_State *L)
{
   luaL_newmetatable(L, OBJ_MT);
   luaL_setfuncs(L, obj_methods, 0);
   lua_pushvalue(L, -1);
   lua_setfield(L, -2, "__index");
   lua_pop(L, 1);
}
