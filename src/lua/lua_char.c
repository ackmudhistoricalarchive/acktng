/* lua_char.c -- CHAR_DATA userdata for ACK!TNG Lua scripting.
 *
 * Exposes character properties to Lua scripts as method calls on a char
 * userdata value.  Read-only getters cover most fields; mutable setters
 * are restricted to values spell/skill scripts legitimately need to write.
 * NPC-only structural setters (set_level, set_max_hp) live in lua_api.c.
 */

#include "globals.h"

#include <lauxlib.h>
#include <lua.h>

#include "lua_api.h"

#define CHAR_MT "ack.char"

void lua_push_char(lua_State *L, CHAR_DATA *ch)
{
   if (!ch)
   {
      lua_pushnil(L);
      return;
   }
   CHAR_DATA **ud = (CHAR_DATA **)lua_newuserdata(L, sizeof(CHAR_DATA *));
   *ud = ch;
   luaL_setmetatable(L, CHAR_MT);
}

CHAR_DATA *lua_check_char(lua_State *L, int idx)
{
   CHAR_DATA **ud = (CHAR_DATA **)luaL_checkudata(L, idx, CHAR_MT);
   if (!ud || !*ud)
      luaL_error(L, "invalid char userdata");
   return *ud;
}

/* ---- read-only getters -------------------------------------------------- */

static int ch_get_hp(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->hit);
   return 1;
}

static int ch_get_max_hp(lua_State *L)
{
   lua_pushinteger(L, get_max_hp(lua_check_char(L, 1)));
   return 1;
}

static int ch_get_mana(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->mana);
   return 1;
}

static int ch_get_max_mana(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->max_mana);
   return 1;
}

static int ch_get_move(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->move);
   return 1;
}

static int ch_get_max_move(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->max_move);
   return 1;
}

static int ch_get_level(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->level);
   return 1;
}

/* ch:get_class_level(cls) */
static int ch_get_class_level(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int cls = (int)luaL_checkinteger(L, 2);
   if (cls < 0 || cls >= MAX_TOTAL_CLASS)
      luaL_error(L, "class index out of range");
   lua_pushinteger(L, char_class_level(ch, cls));
   return 1;
}

static int ch_get_alignment(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->alignment);
   return 1;
}

static int ch_get_gold(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->gold);
   return 1;
}

static int ch_get_name(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   lua_pushstring(L, ch->name ? ch->name : "");
   return 1;
}

static int ch_get_room(lua_State *L)
{
   lua_push_room(L, lua_check_char(L, 1)->in_room);
   return 1;
}

static int ch_get_fighting(lua_State *L)
{
   lua_push_char(L, lua_check_char(L, 1)->fighting);
   return 1;
}

static int ch_get_position(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->position);
   return 1;
}

static int ch_get_str(lua_State *L)
{
   lua_pushinteger(L, get_curr_str(lua_check_char(L, 1)));
   return 1;
}

static int ch_get_dex(lua_State *L)
{
   lua_pushinteger(L, get_curr_dex(lua_check_char(L, 1)));
   return 1;
}

static int ch_get_wis(lua_State *L)
{
   lua_pushinteger(L, get_curr_wis(lua_check_char(L, 1)));
   return 1;
}

static int ch_get_int(lua_State *L)
{
   lua_pushinteger(L, get_curr_int(lua_check_char(L, 1)));
   return 1;
}

static int ch_get_con(lua_State *L)
{
   lua_pushinteger(L, get_curr_con(lua_check_char(L, 1)));
   return 1;
}

static int ch_get_chi(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->chi);
   return 1;
}

static int ch_is_npc(lua_State *L)
{
   lua_pushboolean(L, IS_NPC(lua_check_char(L, 1)));
   return 1;
}

/* ch:is_affected(sn) */
static int ch_is_affected(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   lua_pushboolean(L, is_affected(ch, sn));
   return 1;
}

/* ch:learned(sn) -- returns learned% (0 for NPCs) */
static int ch_learned(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   if (IS_NPC(ch) || !ch->pcdata || sn < 0 || sn >= MAX_SKILL)
   {
      lua_pushinteger(L, 0);
      return 1;
   }
   lua_pushinteger(L, ch->pcdata->learned[sn]);
   return 1;
}

/* ch:cooldown(sn) */
static int ch_cooldown(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   if (sn < 0 || sn >= MAX_SKILL)
   {
      lua_pushinteger(L, 0);
      return 1;
   }
   lua_pushinteger(L, ch->cooldown[sn]);
   return 1;
}

/* ch:has_aff(bit) -- IS_AFFECTED check on affected_by bitvector */
static int ch_has_aff(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int bit = (int)luaL_checkinteger(L, 2);
   lua_pushboolean(L, (ch->affected_by & bit) != 0);
   return 1;
}

static int ch_get_sex(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->sex);
   return 1;
}

static int ch_set_sex(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   ch->sex = (short)luaL_checkinteger(L, 2);
   return 0;
}

/* ch:get_master() -> char | nil */
static int ch_get_master(lua_State *L)
{
   lua_push_char(L, lua_check_char(L, 1)->master);
   return 1;
}

static int ch_get_extract_timer(lua_State *L)
{
   lua_pushinteger(L, lua_check_char(L, 1)->extract_timer);
   return 1;
}

static int ch_set_extract_timer(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   ch->extract_timer = (short)luaL_checkinteger(L, 2);
   return 0;
}

/* ---- mutable setters ---------------------------------------------------- */

static int ch_set_hp(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   long val = (long)luaL_checkinteger(L, 2);
   ch->hit = val;
   update_pos(ch);
   return 0;
}

static int ch_set_mana(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   ch->mana = (long)luaL_checkinteger(L, 2);
   return 0;
}

static int ch_set_move(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   ch->move = (long)luaL_checkinteger(L, 2);
   return 0;
}

static int ch_set_alignment(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   ch->alignment = (short)luaL_checkinteger(L, 2);
   return 0;
}

static int ch_set_gold(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   ch->gold = (int)luaL_checkinteger(L, 2);
   return 0;
}

static int ch_set_position(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   ch->position = (short)luaL_checkinteger(L, 2);
   return 0;
}

/* ---- metatable registration --------------------------------------------- */

static const luaL_Reg char_methods[] = {{"get_hp", ch_get_hp},
                                        {"get_max_hp", ch_get_max_hp},
                                        {"get_mana", ch_get_mana},
                                        {"get_max_mana", ch_get_max_mana},
                                        {"get_move", ch_get_move},
                                        {"get_max_move", ch_get_max_move},
                                        {"get_level", ch_get_level},
                                        {"get_class_level", ch_get_class_level},
                                        {"get_alignment", ch_get_alignment},
                                        {"get_gold", ch_get_gold},
                                        {"get_name", ch_get_name},
                                        {"get_room", ch_get_room},
                                        {"get_fighting", ch_get_fighting},
                                        {"get_position", ch_get_position},
                                        {"get_str", ch_get_str},
                                        {"get_dex", ch_get_dex},
                                        {"get_wis", ch_get_wis},
                                        {"get_int", ch_get_int},
                                        {"get_con", ch_get_con},
                                        {"get_chi", ch_get_chi},
                                        {"is_npc", ch_is_npc},
                                        {"is_affected", ch_is_affected},
                                        {"has_aff", ch_has_aff},
                                        {"learned", ch_learned},
                                        {"cooldown", ch_cooldown},
                                        {"get_sex", ch_get_sex},
                                        {"get_master", ch_get_master},
                                        {"get_extract_timer", ch_get_extract_timer},
                                        {"set_hp", ch_set_hp},
                                        {"set_mana", ch_set_mana},
                                        {"set_move", ch_set_move},
                                        {"set_alignment", ch_set_alignment},
                                        {"set_gold", ch_set_gold},
                                        {"set_position", ch_set_position},
                                        {"set_sex", ch_set_sex},
                                        {"set_extract_timer", ch_set_extract_timer},
                                        {NULL, NULL}};

void lua_register_char_metatable(lua_State *L)
{
   luaL_newmetatable(L, CHAR_MT);
   luaL_setfuncs(L, char_methods, 0);
   lua_pushvalue(L, -1);
   lua_setfield(L, -2, "__index");
   lua_pop(L, 1);
}
