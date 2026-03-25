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
#include "../headers/pub_society.h"
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

/* mud.damage_from_obj(obj_or_nil, ch, victim, dam, element, sn, show) */
static int mud_damage_from_obj(lua_State *L)
{
   OBJ_DATA *obj = NULL;
   {
      void *ud = luaL_testudata(L, 1, "ack.obj");
      if (ud)
         obj = *(OBJ_DATA **)ud;
   }
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
   char arg_buf[MAX_INPUT_LENGTH];
   strncpy(arg_buf, argument, sizeof(arg_buf) - 1);
   arg_buf[sizeof(arg_buf) - 1] = '\0';
   war_attack(ch, arg_buf, gsn);
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
   lua_getfield(L, index, "element");
   af->element = (int)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, index, "level");
   af->level = (int)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, index, "caster");
   {
      void *ud = luaL_testudata(L, -1, "ack.char");
      if (ud)
         af->caster = *(CHAR_DATA **)ud;
   }
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
   char name_buf[MAX_INPUT_LENGTH];
   strncpy(name_buf, name, sizeof(name_buf) - 1);
   name_buf[sizeof(name_buf) - 1] = '\0';
   lua_push_obj(L, get_obj_carry(ch, name_buf));
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
   char cmd_buf[MAX_INPUT_LENGTH];
   strncpy(cmd_buf, cmd, sizeof(cmd_buf) - 1);
   cmd_buf[sizeof(cmd_buf) - 1] = '\0';
   interpret(ch, cmd_buf);
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

   {
      void *ud = luaL_testudata(L, 3, "ack.obj");
      if (ud)
         arg1 = *(OBJ_DATA **)ud;
      else if ((ud = luaL_testudata(L, 3, "ack.char")))
         arg1 = *(CHAR_DATA **)ud;
   }

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
   char text_buf[MAX_INPUT_LENGTH];
   strncpy(text_buf, text, sizeof(text_buf) - 1);
   text_buf[sizeof(text_buf) - 1] = '\0';
   send_to_room(text_buf, room);
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

/* ---- Character queries --------------------------------------------------- */

/* mud.get_pseudo_level(ch) -> int */
static int mud_get_pseudo_level(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   lua_pushinteger(L, get_psuedo_level(ch));
   return 1;
}

/* mud.is_fighting(ch) -> bool */
static int mud_is_fighting(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   lua_pushboolean(L, is_fighting(ch));
   return 1;
}

/* mud.get_char_room(ch, name) -> char | nil */
static int mud_get_char_room(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *name = luaL_checkstring(L, 2);
   char name_buf[MAX_INPUT_LENGTH];
   strncpy(name_buf, name, sizeof(name_buf) - 1);
   name_buf[sizeof(name_buf) - 1] = '\0';
   lua_push_char(L, get_char_room(ch, name_buf));
   return 1;
}

/* mud.is_same_group(ch1, ch2) -> bool */
static int mud_is_same_group(lua_State *L)
{
   CHAR_DATA *ch1 = lua_check_char(L, 1);
   CHAR_DATA *ch2 = lua_check_char(L, 2);
   lua_pushboolean(L, is_same_group(ch1, ch2));
   return 1;
}

/* mud.item_has_apply(ch, type) -> bool */
static int mud_item_has_apply(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int type = (int)luaL_checkinteger(L, 2);
   lua_pushboolean(L, item_has_apply(ch, type));
   return 1;
}

/* ---- Combat support ------------------------------------------------------ */

/* mud.set_fighting(ch, victim) */
static int mud_set_fighting(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   set_fighting(ch, victim, TRUE);
   return 0;
}

/* mud.stop_fighting(ch, both) */
static int mud_stop_fighting(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   bool both = lua_toboolean(L, 2);
   stop_fighting(ch, both);
   return 0;
}

/* mud.check_killer(ch, victim) */
static int mud_check_killer(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   check_killer(ch, victim);
   return 0;
}

/* mud.can_hit_skill(ch, victim, sn) -> bool */
static int mud_can_hit_skill(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int sn = (int)luaL_checkinteger(L, 3);
   lua_pushboolean(L, can_hit_skill(ch, victim, sn));
   return 1;
}

/* mud.calculate_damage(ch, victim, dam, dt, element, crit_possible) -> int */
static int mud_calculate_damage(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int dam = (int)luaL_checkinteger(L, 3);
   int dt = (int)luaL_checkinteger(L, 4);
   int element = (int)luaL_optinteger(L, 5, 0);
   bool crit = (bool)lua_toboolean(L, 6);
   lua_pushinteger(L, calculate_damage(ch, victim, dam, dt, element, crit));
   return 1;
}

/* mud.basic_damage(ch, victim, dam, dt) -> int */
static int mud_basic_damage(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int dam = (int)luaL_checkinteger(L, 3);
   int dt = (int)luaL_checkinteger(L, 4);
   lua_pushinteger(L, damage(ch, victim, dam, dt));
   return 1;
}

/* mud.one_hit(ch, victim, dt) */
static int mud_one_hit(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int dt = (int)luaL_checkinteger(L, 3);
   one_hit(ch, victim, dt);
   return 0;
}

/* mud.aoe_damage(ch, sn, level, min_dam, max_dam, element, flags) */
static int mud_aoe_damage(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   int level = (int)luaL_checkinteger(L, 3);
   int min_dam = (int)luaL_checkinteger(L, 4);
   int max_dam = (int)luaL_checkinteger(L, 5);
   int element = (int)luaL_checkinteger(L, 6);
   int flags = (int)luaL_optinteger(L, 7, 0);

   OBJ_DATA *obj = NULL;
   {
      void *ud = luaL_testudata(L, 8, "ack.obj");
      if (ud)
         obj = *(OBJ_DATA **)ud;
   }
   aoe_damage(ch, obj, sn, level, min_dam, max_dam, element, flags);
   return 0;
}

/* mud.breath_damage(ch, sn, element) */
static int mud_breath_damage(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   int element = (int)luaL_checkinteger(L, 3);
   breath_damage(ch, sn, element);
   return 0;
}

/* ---- Skill system -------------------------------------------------------- */

/* mud.can_use_skill_message(ch, sn) -> bool */
static int mud_can_use_skill_message(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   lua_pushboolean(L, can_use_skill_message(ch, sn));
   return 1;
}

/* mud.can_use_pub_society_skill(ch, sn) -> bool */
static int mud_can_use_pub_society_skill(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int sn = (int)luaL_checkinteger(L, 2);
   lua_pushboolean(L, can_use_pub_society_skill(ch, sn));
   return 1;
}

/* mud.skill_lookup(name) -> sn | -1 */
static int mud_skill_lookup(lua_State *L)
{
   const char *name = luaL_checkstring(L, 1);
   lua_pushinteger(L, skill_lookup(name));
   return 1;
}

/* mud.is_valid_finisher(ch) -> bool */
static int mud_is_valid_finisher(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   lua_pushboolean(L, is_valid_finisher(ch));
   return 1;
}

/* mud.reset_combo(ch) */
static int mud_reset_combo(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   reset_combo(ch);
   return 0;
}

/* mud.get_chi(ch) -> int */
static int mud_get_chi(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   lua_pushinteger(L, get_chi(ch));
   return 1;
}

/* mud.chi_skill_cost(base_cost, cooldown) -> int */
static int mud_chi_skill_cost(lua_State *L)
{
   int base_cost = (int)luaL_checkinteger(L, 1);
   int cooldown = (int)luaL_checkinteger(L, 2);
   lua_pushinteger(L, chi_skill_cost(base_cost, cooldown));
   return 1;
}

/* mud.pug_attack(ch, argument, gsn) */
static int mud_pug_attack(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *argument = luaL_checkstring(L, 2);
   int gsn = (int)luaL_checkinteger(L, 3);
   char arg_buf[MAX_INPUT_LENGTH];
   strncpy(arg_buf, argument, sizeof(arg_buf) - 1);
   arg_buf[sizeof(arg_buf) - 1] = '\0';
   pug_attack(ch, arg_buf, gsn);
   return 0;
}

/* ---- Healing ------------------------------------------------------------- */

/* mud.heal_character(ch, victim, base_heal, sn, hot) */
static int mud_heal_character(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int base_heal = (int)luaL_checkinteger(L, 3);
   int sn = (int)luaL_checkinteger(L, 4);
   bool hot = lua_toboolean(L, 5);
   heal_character(ch, victim, base_heal, sn, hot);
   return 0;
}

/* ---- Room affects -------------------------------------------------------- */

/* mud.affect_to_room(room, af_table) -- table has: type, duration, location, modifier, bitvector */
static int mud_affect_to_room(lua_State *L)
{
   ROOM_INDEX_DATA *room = lua_check_room(L, 1);
   luaL_checktype(L, 2, LUA_TTABLE);

   ROOM_AFFECT_DATA raf;
   memset(&raf, 0, sizeof(raf));

   lua_getfield(L, 2, "type");
   raf.type = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, 2, "duration");
   raf.duration = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, 2, "location");
   raf.location = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, 2, "modifier");
   raf.modifier = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, 2, "bitvector");
   raf.bitvector = (int)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);
   lua_getfield(L, 2, "caster");
   {
      void *ud = luaL_testudata(L, -1, "ack.char");
      if (ud)
         raf.caster = *(CHAR_DATA **)ud;
   }
   lua_pop(L, 1);

   affect_to_room(room, &raf);
   return 0;
}

/* ---- Wizard elemental dot spells ----------------------------------------- */

/* mud.cast_wizard_elemental_dot_spell(sn, level, ch, victim, obj_or_nil,
 *                                     cast_msg, dmg_msg, element) -> bool */
static int mud_cast_wizard_elemental_dot_spell(lua_State *L)
{
   int sn = (int)luaL_checkinteger(L, 1);
   int level = (int)luaL_checkinteger(L, 2);
   CHAR_DATA *ch = lua_check_char(L, 3);
   CHAR_DATA *victim = lua_check_char(L, 4);
   OBJ_DATA *obj = NULL;
   {
      void *ud = luaL_testudata(L, 5, "ack.obj");
      if (ud)
         obj = *(OBJ_DATA **)ud;
   }
   const char *cast_msg = luaL_checkstring(L, 6);
   const char *dmg_msg = luaL_checkstring(L, 7);
   int element = (int)luaL_checkinteger(L, 8);
   lua_pushboolean(
       L, cast_wizard_elemental_dot_spell(sn, level, ch, victim, obj, cast_msg, dmg_msg, element));
   return 1;
}

/* mud.trigger_elemental_spell_combo(ch, victim, obj_or_nil, sn, level) -> bool */
static int mud_trigger_elemental_spell_combo(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   OBJ_DATA *obj = NULL;
   {
      void *ud = luaL_testudata(L, 3, "ack.obj");
      if (ud)
         obj = *(OBJ_DATA **)ud;
   }
   int sn = (int)luaL_checkinteger(L, 4);
   int level = (int)luaL_checkinteger(L, 5);
   lua_pushboolean(L, trigger_elemental_spell_combo(ch, victim, obj, sn, level));
   return 1;
}

/* mud.apply_elemental_spell_debuff(ch, victim, sn, msg) */
static int mud_apply_elemental_spell_debuff(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int sn = (int)luaL_checkinteger(L, 3);
   const char *msg = luaL_checkstring(L, 4);
   apply_elemental_spell_debuff(ch, victim, sn, msg);
   return 0;
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

/* ---- World queries ------------------------------------------------------- */

/* mud.get_char_world(ch, name) -> char | nil */
static int mud_get_char_world(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *name = luaL_checkstring(L, 2);
   char name_buf[MAX_INPUT_LENGTH];
   strncpy(name_buf, name, sizeof(name_buf) - 1);
   name_buf[sizeof(name_buf) - 1] = '\0';
   lua_push_char(L, get_char_world(ch, name_buf));
   return 1;
}

/* ---- Combat skills / commands -------------------------------------------- */

/* mud.combo(ch, victim, sn) -> bool */
static int mud_combo(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int sn = (int)luaL_checkinteger(L, 3);
   lua_pushboolean(L, combo(ch, victim, sn));
   return 1;
}

/* mud.backstab(ch, victim, is_backstab) */
static int mud_backstab(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   bool is_bs = lua_toboolean(L, 3);
   backstab(ch, victim, is_bs);
   return 0;
}

/* mud.stun(ch, victim) */
static int mud_stun(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   stun(ch, victim);
   return 0;
}

/* mud.disarm(ch, victim) */
static int mud_disarm(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   disarm(ch, victim);
   return 0;
}

/* mud.trip(ch, victim) */
static int mud_trip(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   trip(ch, victim);
   return 0;
}

/* mud.do_poison(ch, arg, gsn) -> bool */
static int mud_do_poison(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *arg = luaL_optstring(L, 2, "");
   int gsn = (int)luaL_checkinteger(L, 3);
   lua_pushboolean(L, do_poison(ch, (char *)arg, gsn));
   return 1;
}

/* mud.multi_hit(ch, victim, dt) */
static int mud_multi_hit(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int dt = (int)luaL_checkinteger(L, 3);
   multi_hit(ch, victim, dt);
   return 0;
}

/* mud.can_see(ch, victim) -> bool */
static int mud_can_see(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   lua_pushboolean(L, can_see(ch, victim));
   return 1;
}

/* mud.do_spell_heal(ch, victim, sn) */
static int mud_do_spell_heal(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int sn = (int)luaL_checkinteger(L, 3);
   do_spell_heal(ch, victim, sn);
   return 0;
}

/* mud.room_is_private(room) -> bool */
static int mud_room_is_private(lua_State *L)
{
   ROOM_INDEX_DATA *room = lua_check_room(L, 1);
   lua_pushboolean(L, room_is_private(room));
   return 1;
}

/* mud.set_hunt(ch, fch, victim, set_flags[, rem_flags]) -> bool */
static int mud_set_hunt(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *fch = lua_check_char(L, 2);
   CHAR_DATA *victim = lua_check_char(L, 3);
   int set_flags = (int)luaL_checkinteger(L, 4);
   int rem_flags = (int)luaL_optinteger(L, 5, 0);
   lua_pushboolean(L, set_hunt(ch, fch, victim, NULL, set_flags, rem_flags));
   return 1;
}

/* mud.gain_exp(ch, amount) */
static int mud_gain_exp(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   int amount = (int)luaL_checkinteger(L, 2);
   gain_exp(ch, amount);
   return 0;
}

/* mud.do_say(ch, msg) */
static int mud_do_say(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *msg = luaL_checkstring(L, 2);
   char msg_buf[MAX_INPUT_LENGTH];
   strncpy(msg_buf, msg, sizeof(msg_buf) - 1);
   msg_buf[sizeof(msg_buf) - 1] = '\0';
   do_say(ch, msg_buf);
   return 0;
}

/* mud.do_look(ch[, arg]) */
static int mud_do_look(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *arg = luaL_optstring(L, 2, "");
   char arg_buf[MAX_INPUT_LENGTH];
   strncpy(arg_buf, arg, sizeof(arg_buf) - 1);
   arg_buf[sizeof(arg_buf) - 1] = '\0';
   do_look(ch, arg_buf);
   return 0;
}

/* mud.do_sleep(ch[, arg]) */
static int mud_do_sleep(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   const char *arg = luaL_optstring(L, 2, "");
   char arg_buf[MAX_INPUT_LENGTH];
   strncpy(arg_buf, arg, sizeof(arg_buf) - 1);
   arg_buf[sizeof(arg_buf) - 1] = '\0';
   do_sleep(ch, arg_buf);
   return 0;
}

/* mud.apply_necromancer_debuff(ch, victim, sn, dam[, obj]) */
static int mud_apply_necromancer_debuff(lua_State *L)
{
   CHAR_DATA *ch = lua_check_char(L, 1);
   CHAR_DATA *victim = lua_check_char(L, 2);
   int sn = (int)luaL_checkinteger(L, 3);
   int dam = (int)luaL_checkinteger(L, 4);
   OBJ_DATA *obj = NULL;
   {
      void *ud = luaL_testudata(L, 5, "ack.obj");
      if (ud)
         obj = *(OBJ_DATA **)ud;
   }
   apply_necromancer_damage_debuff(ch, victim, sn, dam, obj);
   return 0;
}

/* mud.cast_spell(sn, level, ch, victim_or_nil[, obj_or_nil]) -> bool
 * Calls the C spell_fun directly (bypasses Lua dispatch — safe to call
 * from within another Lua spell).  Returns false if no C impl. */
static int mud_cast_spell(lua_State *L)
{
   int sn = (int)luaL_checkinteger(L, 1);
   int level = (int)luaL_checkinteger(L, 2);
   CHAR_DATA *ch = lua_check_char(L, 3);

   void *vo = NULL;
   {
      void *ud = luaL_testudata(L, 4, "ack.char");
      if (ud)
         vo = *(CHAR_DATA **)ud;
   }

   OBJ_DATA *obj = NULL;
   {
      void *ud = luaL_testudata(L, 5, "ack.obj");
      if (ud)
         obj = *(OBJ_DATA **)ud;
   }

   if (sn < 0 || sn >= MAX_SKILL || !skill_table[sn].spell_fun)
   {
      lua_pushboolean(L, 0);
      return 1;
   }
   lua_pushboolean(L, (*skill_table[sn].spell_fun)(sn, level, ch, vo, obj));
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
    {"set_fighting", mud_set_fighting},
    {"stop_fighting", mud_stop_fighting},
    {"check_killer", mud_check_killer},
    {"can_hit_skill", mud_can_hit_skill},
    {"calculate_damage", mud_calculate_damage},
    {"basic_damage", mud_basic_damage},
    {"one_hit", mud_one_hit},
    {"aoe_damage", mud_aoe_damage},
    {"breath_damage", mud_breath_damage},
    {"cast_wizard_elemental_dot_spell", mud_cast_wizard_elemental_dot_spell},
    {"trigger_elemental_spell_combo", mud_trigger_elemental_spell_combo},
    {"apply_elemental_spell_debuff", mud_apply_elemental_spell_debuff},
    /* Healing */
    {"heal", mud_heal},
    {"heal_mana", mud_heal_mana},
    {"heal_move", mud_heal_move},
    {"heal_character", mud_heal_character},
    /* Affects */
    {"apply_affect", mud_apply_affect},
    {"affect_join", mud_affect_join},
    {"affect_strip", mud_affect_strip},
    {"is_affected", mud_is_affected},
    {"affect_to_room", mud_affect_to_room},
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
    /* Characters / queries */
    {"get_pseudo_level", mud_get_pseudo_level},
    {"is_fighting", mud_is_fighting},
    {"get_char_room", mud_get_char_room},
    {"get_char_world", mud_get_char_world},
    {"is_same_group", mud_is_same_group},
    {"item_has_apply", mud_item_has_apply},
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
    {"can_use_skill_message", mud_can_use_skill_message},
    {"can_use_pub_society_skill", mud_can_use_pub_society_skill},
    {"skill_lookup", mud_skill_lookup},
    {"subtract_energy", mud_subtract_energy},
    {"mana_cost", mud_mana_cost},
    {"skill_success", mud_skill_success},
    {"is_valid_finisher", mud_is_valid_finisher},
    {"reset_combo", mud_reset_combo},
    {"get_chi", mud_get_chi},
    {"chi_skill_cost", mud_chi_skill_cost},
    {"pug_attack", mud_pug_attack},
    /* Combat skills / commands */
    {"combo", mud_combo},
    {"backstab", mud_backstab},
    {"stun", mud_stun},
    {"disarm", mud_disarm},
    {"trip", mud_trip},
    {"do_poison", mud_do_poison},
    {"multi_hit", mud_multi_hit},
    {"can_see", mud_can_see},
    {"do_spell_heal", mud_do_spell_heal},
    {"room_is_private", mud_room_is_private},
    {"set_hunt", mud_set_hunt},
    {"gain_exp", mud_gain_exp},
    {"do_say", mud_do_say},
    {"do_look", mud_do_look},
    {"do_sleep", mud_do_sleep},
    {"apply_necromancer_debuff", mud_apply_necromancer_debuff},
    {"cast_spell", mud_cast_spell},
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
