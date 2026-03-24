/* lua_api.c -- mud.* C API module for ACK!TNG Lua scripting.
 *
 * Implements the mud.* Lua table that gives scripts access to game engine
 * primitives.  All functions here are thin wrappers around existing C
 * functions — they do not introduce new game mechanics.
 *
 * Registration entry point: lua_register_mud_api(L)
 * Push/check helpers for userdata: lua_push_char/obj/room, lua_check_char/obj/room
 *   (declared in lua_api.h; push implementations live in lua_char.c,
 *    lua_obj.c, lua_room.c — forward declarations are picked up via lua_api.h)
 */

#include "globals.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "../headers/magic.h"
#include "../headers/skills.h"
#include "lua_api.h"

/* ---- Combat -------------------------------------------------------------- */

/* mud.damage(ch, victim, dam, sn, element, show) */
static int mud_damage(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int dam = (int)luaL_checkinteger(L, 3);
   int sn = (int)luaL_checkinteger(L, 4);
   int element = (int)luaL_checkinteger(L, 5);
   bool show = lua_toboolean(L, 6);
   lua_pushboolean(L, sp_damage(NULL, ch, victim, dam, element, sn, show));
   return 1;
}

/* mud.damage_from_obj(obj, ch, victim, dam, element, sn, show) */
static int mud_damage_from_obj(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   CHAR_DATA *ch = lua_check_char(L, 2);
   CHAR_DATA *victim = lua_check_char(L, 3);
   int dam = (int)luaL_checkinteger(L, 4);
   int element = (int)luaL_checkinteger(L, 5);
   int sn = (int)luaL_checkinteger(L, 6);
   bool show = lua_toboolean(L, 7);
   lua_pushboolean(L, sp_damage(obj, ch, victim, dam, element, sn, show));
   return 1;
}

/* mud.war_attack(ch, argument, gsn) */
static int mud_war_attack(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *argument = luaL_checkstring(L, 2);
   int gsn = (int)luaL_checkinteger(L, 3);
   war_attack(ch, (char *)argument, gsn);
   return 0;
}

/* mud.saves_spell(level, victim) -> bool */
static int mud_saves_spell(lua_State *L)
{
   int level = (int)luaL_checkinteger(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   lua_pushboolean(L, saves_spell(level, victim));
   return 1;
}

/* mud.is_safe(ch, victim) -> bool */
static int mud_is_safe(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   lua_pushboolean(L, is_safe(ch, victim));
   return 1;
}

/* ---- Healing and Resources ----------------------------------------------- */

/* mud.heal(victim, amount) */
static int mud_heal(lua_State *L)
{
   CHAR_DATA *victim = lua_check_char(L, 1);
   long amount = (long)luaL_checkinteger(L, 2);
   victim->hit = UMIN(victim->hit + amount, get_max_hp(victim));
   update_pos(victim);
   return 0;
}

/* mud.heal_mana(victim, amount) */
static int mud_heal_mana(lua_State *L)
{
   CHAR_DATA *victim = lua_check_char(L, 1);
   long amount = (long)luaL_checkinteger(L, 2);
   victim->mana = UMIN(victim->mana + amount, victim->max_mana);
   return 0;
}

/* mud.heal_move(victim, amount) */
static int mud_heal_move(lua_State *L)
{
   CHAR_DATA *victim = lua_check_char(L, 1);
   long amount = (long)luaL_checkinteger(L, 2);
   victim->move = UMIN(victim->move + amount, victim->max_move);
   return 0;
}

/* ---- Affects ------------------------------------------------------------- */

/* Parse a Lua table at stack index into af.  Zeros af first. */
static void lua_to_affect(lua_State *L, int index, AFFECT_DATA *af)
{
   memset(af, 0, sizeof(*af));
   luaL_checktype(L, index, LUA_TTABLE);

   lua_getfield(L, index, "type");
   af->type = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, index, "duration");
   af->duration = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, index, "location");
   af->location = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, index, "modifier");
   af->modifier = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, index, "bitvector");
   af->bitvector = (int)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, index, "duration_type");
   af->duration_type = (short)luaL_optinteger(L, -1, DURATION_HOUR);
   lua_pop(L, 1);
}

/* mud.apply_affect(victim, af_table)
 * af_table keys: type, duration, location, modifier, bitvector, duration_type */
static int mud_apply_affect(lua_State *L)
{
   CHAR_DATA *victim = lua_check_char(L, 1);
   AFFECT_DATA af;
   lua_to_affect(L, 2, &af);
   affect_to_char(victim, &af);
   return 0;
}

/* mud.affect_join(victim, af_table) -- same format as apply_affect */
static int mud_affect_join(lua_State *L)
{
   CHAR_DATA *victim = lua_check_char(L, 1);
   AFFECT_DATA af;
   lua_to_affect(L, 2, &af);
   affect_join(victim, &af);
   return 0;
}

/* mud.affect_strip(victim, sn) */
static int mud_affect_strip(lua_State *L)
{
   CHAR_DATA *victim = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   affect_strip(victim, sn);
   return 0;
}

/* mud.is_affected(victim, sn) -> bool */
static int mud_is_affected(lua_State *L)
{
   CHAR_DATA *victim = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   lua_pushboolean(L, is_affected(victim, sn));
   return 1;
}

/* ---- Objects and World --------------------------------------------------- */

/* mud.create_object(vnum, level) -> obj | nil */
static int mud_create_object(lua_State *L)
{
   int vnum = (int)luaL_checkinteger(L, 1);
   int level = (int)luaL_checkinteger(L, 2);
   OBJ_INDEX_DATA *pObjIdx = get_obj_index(vnum);
   if (!pObjIdx)
   {
      lua_pushnil(L);
      return 1;
   }
   lua_push_obj(L, create_object(pObjIdx, level));
   return 1;
}

/* mud.obj_to_room(obj, room) */
static int mud_obj_to_room(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   ROOM_INDEX_DATA *room = lua_check_room(L, 2);
   obj_to_room(obj, room);
   return 0;
}

/* mud.obj_to_char(obj, ch) */
static int mud_obj_to_char(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   CHAR_DATA *ch = lua_check_char(L, 2);
   obj_to_char(obj, ch);
   return 0;
}

/* mud.obj_from_obj(obj) */
static int mud_obj_from_obj(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   obj_from_obj(obj);
   return 0;
}

/* mud.extract_obj(obj) */
static int mud_extract_obj(lua_State *L)
{
   OBJ_DATA *obj = lua_check_obj(L, 1);
   extract_obj(obj);
   return 0;
}

/* mud.get_obj_carry(ch, name) -> obj | nil */
static int mud_get_obj_carry(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *name = luaL_checkstring(L, 2);
   lua_push_obj(L, get_obj_carry(ch, (char *)name));
   return 1;
}

/* mud.get_obj_room(room, name) -> obj | nil
 * Searches room->first_content linked list. */
static int mud_get_obj_room(lua_State *L)
{
   ROOM_INDEX_DATA *room = lua_check_room(L, 1);
   const char *name = luaL_checkstring(L, 2);
   OBJ_DATA *obj;
   for (obj = room->first_content; obj; obj = obj->next_in_room)
   {
      if (is_name(name, obj->name))
         break;
   }
   lua_push_obj(L, obj);
   return 1;
}

/* mud.get_obj_contents(container) -> table of obj userdata */
static int mud_get_obj_contents(lua_State *L)
{
   OBJ_DATA *container = lua_check_obj(L, 1);
   lua_newtable(L);
   int i = 1;
   for (OBJ_DATA *item = container->first_content; item; item = item->next_content)
   {
      lua_push_obj(L, item);
      lua_rawseti(L, -2, i++);
   }
   return 1;
}

/* mud.get_room(vnum) -> room | nil */
static int mud_get_room(lua_State *L)
{
   int vnum = (int)luaL_checkinteger(L, 1);
   lua_push_room(L, get_room_index(vnum));
   return 1;
}

/* mud.transfer(ch, room) */
static int mud_transfer(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   ROOM_INDEX_DATA *room = lua_check_room(L, 2);
   char_from_room(ch);
   char_to_room(ch, room);
   return 0;
}

/* mud.chars_in_room(room) -> table of char userdata */
static int mud_chars_in_room(lua_State *L)
{
   ROOM_INDEX_DATA *room = lua_check_room(L, 1);
   lua_newtable(L);
   int i = 1;
   for (CHAR_DATA *rch = room->first_person; rch; rch = rch->next_in_room)
   {
      lua_push_char(L, rch);
      lua_rawseti(L, -2, i++);
   }
   return 1;
}

/* mud.interpret(ch, command)
 * Restricted to a whitelist of non-Lua-dispatched commands to avoid
 * reentrancy on the single lua_State. */
static int mud_interpret(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *cmd = luaL_checkstring(L, 2);
   static const char *allowed[] = {"wear",  "remove", "drop",   "get",  "put", "give",
                                   "north", "south",  "east",   "west", "up",  "down",
                                   "wield", "hold",   "shield", NULL};
   bool ok = FALSE;
   for (int i = 0; allowed[i]; i++)
   {
      size_t len = strlen(allowed[i]);
      if (strncasecmp(cmd, allowed[i], len) == 0 &&
          (cmd[len] == '\0' || isspace((unsigned char)cmd[len])))
      {
         ok = TRUE;
         break;
      }
   }
   if (!ok)
   {
      lua_pushstring(L, "mud.interpret: command not in allowed whitelist");
      lua_error(L);
      return 0;
   }
   interpret(ch, (char *)cmd);
   return 0;
}

/* ---- Characters and Followers -------------------------------------------- */

/* mud.create_mobile(vnum) -> char | nil */
static int mud_create_mobile(lua_State *L)
{
   int vnum = (int)luaL_checkinteger(L, 1);
   MOB_INDEX_DATA *pMobIdx = get_mob_index(vnum);
   if (!pMobIdx)
   {
      lua_pushnil(L);
      return 1;
   }
   lua_push_char(L, create_mobile(pMobIdx));
   return 1;
}

/* mud.char_to_room(ch, room) */
static int mud_char_to_room(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   ROOM_INDEX_DATA *room = lua_check_room(L, 2);
   char_to_room(ch, room);
   return 0;
}

/* mud.add_follower(mob, master) */
static int mud_add_follower(lua_State *L)
{
   CHAR_DATA *mob = lua_check_char(L, 1);
   CHAR_DATA *master = lua_check_char(L, 2);
   add_follower(mob, master);
   return 0;
}

/* mud.stop_follower(mob) */
static int mud_stop_follower(lua_State *L)
{
   CHAR_DATA *mob = lua_check_char(L, 1);
   stop_follower(mob);
   return 0;
}

/* mud.extract_char(mob, pull) */
static int mud_extract_char(lua_State *L)
{
   CHAR_DATA *mob = lua_check_char(L, 1);
   bool pull = lua_toboolean(L, 2);
   extract_char(mob, pull);
   return 0;
}

/* mud.set_mob_level(mob, level) -- NPCs only */
static int mud_set_mob_level(lua_State *L)
{
   CHAR_DATA *mob = lua_check_char(L, 1);
   if (!IS_NPC(mob))
      luaL_error(L, "mud.set_mob_level: target must be an NPC");
   mob->level = (short)luaL_checkinteger(L, 2);
   return 0;
}

/* mud.set_mob_max_hp(mob, hp) -- NPCs only */
static int mud_set_mob_max_hp(lua_State *L)
{
   CHAR_DATA *mob = lua_check_char(L, 1);
   if (!IS_NPC(mob))
      luaL_error(L, "mud.set_mob_max_hp: target must be an NPC");
   mob->max_hit = (long)luaL_checkinteger(L, 2);
   return 0;
}

/* ---- Output and Communication -------------------------------------------- */

/* mud.send(ch, text) */
static int mud_send(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *text = luaL_checkstring(L, 2);
   send_to_char(text, ch);
   return 0;
}

/* mud.act(format, ch, arg1, arg2, target_type)
 * target_type: "room" -> TO_ROOM, "char" -> TO_CHAR, "vict" -> TO_VICT,
 *              "notchar" -> TO_NOTVICT  (default: TO_ROOM) */
static int mud_act(lua_State *L)
{
   const char *fmt = luaL_checkstring(L, 1);
   CHAR_DATA *ch = lua_check_char(L, 2);

   /* arg1 / arg2 can be char or obj userdata, or nil */
   void *arg1 = NULL;
   void *arg2 = NULL;

   if (!lua_isnil(L, 3))
   {
      void *ud = luaL_testudata(L, 3, "ack.obj");
      if (ud)
         arg1 = *(OBJ_DATA **)ud;
      else if ((ud = luaL_testudata(L, 3, "ack.char")))
         arg1 = *(CHAR_DATA **)ud;
   }

   if (!lua_isnil(L, 4))
   {
      void *ud = luaL_testudata(L, 4, "ack.char");
      if (ud)
         arg2 = *(CHAR_DATA **)ud;
      else if ((ud = luaL_testudata(L, 4, "ack.obj")))
         arg2 = *(OBJ_DATA **)ud;
   }

   const char *target_type = luaL_optstring(L, 5, "room");
   int to_type = TO_ROOM;
   if (!str_cmp(target_type, "char"))
      to_type = TO_CHAR;
   else if (!str_cmp(target_type, "vict"))
      to_type = TO_VICT;
   else if (!str_cmp(target_type, "notchar"))
      to_type = TO_NOTVICT;

   act(fmt, ch, arg1, arg2, to_type);
   return 0;
}

/* mud.echo_room(room, text) */
static int mud_echo_room(lua_State *L)
{
   ROOM_INDEX_DATA *room = lua_check_room(L, 1);
   const char *text = luaL_checkstring(L, 2);
   send_to_room((char *)text, room);
   return 0;
}

/* ---- Skill System -------------------------------------------------------- */

/* mud.raise_skill(ch, sn) */
static int mud_raise_skill(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   lua_pushboolean(L, raise_skill(ch, sn));
   return 1;
}

/* mud.wait_state(ch, beats) */
static int mud_wait_state(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int beats = (int)luaL_checkinteger(L, 2);
   WAIT_STATE(ch, beats);
   return 0;
}

/* mud.set_cooldown(ch, sn, ticks) */
static int mud_set_cooldown(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   int ticks = (int)luaL_checkinteger(L, 3);
   if (sn >= 0 && sn < MAX_SKILL)
      ch->cooldown[sn] = (short)ticks;
   return 0;
}

/* mud.can_use_skill(ch, sn) -> bool */
static int mud_can_use_skill(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   lua_pushboolean(L, can_use_skill(ch, sn));
   return 1;
}

/* mud.subtract_energy(ch, gsn) */
static int mud_subtract_energy(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int gsn = (int)luaL_checkinteger(L, 2);
   subtract_energy_cost(ch, gsn);
   return 0;
}

/* mud.mana_cost(ch, sn) -> int */
static int mud_mana_cost(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   lua_pushinteger(L, mana_cost(ch, sn));
   return 1;
}

/* mud.skill_success(ch, sn) -> bool */
static int mud_skill_success(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   lua_pushboolean(L, skill_success(ch, NULL, sn, 0));
   return 1;
}

/* ---- Randomness and Utility ---------------------------------------------- */

/* mud.dice(n, m) -> int */
static int mud_dice(lua_State *L)
{
   int n = (int)luaL_checkinteger(L, 1);
   int m = (int)luaL_checkinteger(L, 2);
   lua_pushinteger(L, dice(n, m));
   return 1;
}

/* mud.number_range(lo, hi) -> int */
static int mud_number_range(lua_State *L)
{
   int lo = (int)luaL_checkinteger(L, 1);
   int hi = (int)luaL_checkinteger(L, 2);
   lua_pushinteger(L, number_range(lo, hi));
   return 1;
}

/* mud.number_percent() -> int */
static int mud_number_percent(lua_State *L)
{
   lua_pushinteger(L, number_percent());
   return 1;
}

/* mud.UMIN(a, b) -> int */
static int mud_UMIN(lua_State *L)
{
   lua_Integer a = luaL_checkinteger(L, 1);
   lua_Integer b = luaL_checkinteger(L, 2);
   lua_pushinteger(L, a < b ? a : b);
   return 1;
}

/* mud.UMAX(a, b) -> int */
static int mud_UMAX(lua_State *L)
{
   lua_Integer a = luaL_checkinteger(L, 1);
   lua_Integer b = luaL_checkinteger(L, 2);
   lua_pushinteger(L, a > b ? a : b);
   return 1;
}

/* ---- Dispatch table ------------------------------------------------------ */

static const luaL_Reg mud_api[] = {
    /* Combat */
    {"damage", mud_damage},
    {"damage_from_obj", mud_damage_from_obj},
    {"war_attack", mud_war_attack},
    {"saves_spell", mud_saves_spell},
    {"is_safe", mud_is_safe},
    /* Healing */
    {"heal", mud_heal},
    {"heal_mana", mud_heal_mana},
    {"heal_move", mud_heal_move},
    /* Affects */
    {"apply_affect", mud_apply_affect},
    {"affect_join", mud_affect_join},
    {"affect_strip", mud_affect_strip},
    {"is_affected", mud_is_affected},
    /* Objects / world */
    {"create_object", mud_create_object},
    {"obj_to_room", mud_obj_to_room},
    {"obj_to_char", mud_obj_to_char},
    {"obj_from_obj", mud_obj_from_obj},
    {"extract_obj", mud_extract_obj},
    {"get_obj_carry", mud_get_obj_carry},
    {"get_obj_room", mud_get_obj_room},
    {"get_obj_contents", mud_get_obj_contents},
    {"get_room", mud_get_room},
    {"transfer", mud_transfer},
    {"chars_in_room", mud_chars_in_room},
    {"interpret", mud_interpret},
    /* Characters / followers */
    {"create_mobile", mud_create_mobile},
    {"char_to_room", mud_char_to_room},
    {"add_follower", mud_add_follower},
    {"stop_follower", mud_stop_follower},
    {"extract_char", mud_extract_char},
    {"set_mob_level", mud_set_mob_level},
    {"set_mob_max_hp", mud_set_mob_max_hp},
    /* Output */
    {"send", mud_send},
    {"act", mud_act},
    {"echo_room", mud_echo_room},
    /* Skill system */
    {"raise_skill", mud_raise_skill},
    {"wait_state", mud_wait_state},
    {"set_cooldown", mud_set_cooldown},
    {"can_use_skill", mud_can_use_skill},
    {"subtract_energy", mud_subtract_energy},
    {"mana_cost", mud_mana_cost},
    {"skill_success", mud_skill_success},
    /* Randomness */
    {"dice", mud_dice},
    {"number_range", mud_number_range},
    {"number_percent", mud_number_percent},
    {"UMIN", mud_UMIN},
    {"UMAX", mud_UMAX},
    {NULL, NULL}};

/* Called from lua_engine_init() to register the mud.* global table. */
void lua_register_mud_api(lua_State *L)
{
   luaL_newlib(L, mud_api);
   lua_setglobal(L, "mud");
}
