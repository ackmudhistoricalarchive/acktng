/* lua_constants.c -- Pre-load game constants into the Lua VM.
 *
 * Creates read-only tables ELE, AFF, APPLY, POS, TAR, CLASS, DURATION in
 * the global environment so Lua scripts can reference game constants by
 * name (e.g. ELE.FIRE, AFF.BLIND) instead of raw integers.
 */

#include "globals.h"

#include <lua.h>
#include <lauxlib.h>

#include "../headers/magic.h"

/* Helper: create a global table and populate it with integer fields. */
static void push_const_table(lua_State *L, const char *tname, const char *const keys[],
                             const int vals[], int n)
{
   lua_newtable(L);
   for (int i = 0; i < n; i++)
   {
      lua_pushinteger(L, vals[i]);
      lua_setfield(L, -2, keys[i]);
   }
   /* Make the table read-only via a metatable __newindex that errors. */
   lua_newtable(L); /* metatable */
   lua_pushcfunction(L, (lua_CFunction)luaL_error);
   lua_setfield(L, -2, "__newindex");
   lua_pushvalue(L, -2); /* table itself as __index */
   lua_setfield(L, -2, "__index");
   lua_setmetatable(L, -2);
   lua_setglobal(L, tname);
}

void lua_register_constants(lua_State *L)
{
   /* ELE — element types (from magic.h) */
   {
      static const char *keys[] = {"NONE",  "PHYSICAL", "MENTAL", "HOLY",   "AIR",
                                   "EARTH", "WATER",    "FIRE",   "SHADOW", "POISON"};
      static const int vals[] = {ELE_NONE,  ELE_PHYSICAL, ELE_MENTAL, ELE_HOLY,   ELE_AIR,
                                 ELE_EARTH, ELE_WATER,    ELE_FIRE,   ELE_SHADOW, ELE_POISON};
      push_const_table(L, "ELE", keys, vals, 10);
   }

   /* AFF — affect bitvectors (from config.h) */
   {
      static const char *keys[] = {"BLIND",        "INVISIBLE",     "DETECT_EVIL", "DETECT_INVIS",
                                   "DETECT_MAGIC", "DETECT_HIDDEN", "SANCTUARY",   "FAERIE_FIRE",
                                   "INFRARED",     "CURSE",         "POISON",      "PROTECT",
                                   "SNEAK",        "HIDE",          "SLEEP",       "CHARM",
                                   "FLYING",       "PASS_DOOR",     "ANTI_MAGIC",  "BLASTED",
                                   "REMORT_CURSE", "CONFUSED",      "HOLD",        "PARALYSIS"};
      static const int vals[] = {
          AFF_BLIND,         AFF_INVISIBLE, AFF_DETECT_EVIL, AFF_DETECT_INVIS, AFF_DETECT_MAGIC,
          AFF_DETECT_HIDDEN, AFF_SANCTUARY, AFF_FAERIE_FIRE, AFF_INFRARED,     AFF_CURSE,
          AFF_POISON,        AFF_PROTECT,   AFF_SNEAK,       AFF_HIDE,         AFF_SLEEP,
          AFF_CHARM,         AFF_FLYING,    AFF_PASS_DOOR,   AFF_ANTI_MAGIC,   AFF_BLASTED,
          AFF_REMORT_CURSE,  AFF_CONFUSED,  AFF_HOLD,        AFF_PARALYSIS};
      push_const_table(L, "AFF", keys, vals, 24);
   }

   /* APPLY — affect location codes (from config.h) */
   {
      static const char *keys[] = {
          "NONE",       "STR",         "DEX",        "INT",          "WIS",     "CON",     "SEX",
          "CLASS",      "LEVEL",       "AGE",        "HEIGHT",       "WEIGHT",  "MANA",    "HIT",
          "MOVE",       "GOLD",        "EXP",        "AC",           "HITROLL", "DAMROLL", "DOT",
          "SPELLPOWER", "SAVING_PARA", "SAVING_ROD", "SAVING_SPELL", "SPEED"};
      static const int vals[] = {
          APPLY_NONE,   APPLY_STR,        APPLY_DEX,         APPLY_INT,        APPLY_WIS,
          APPLY_CON,    APPLY_SEX,        APPLY_CLASS,       APPLY_LEVEL,      APPLY_AGE,
          APPLY_HEIGHT, APPLY_WEIGHT,     APPLY_MANA,        APPLY_HIT,        APPLY_MOVE,
          APPLY_GOLD,   APPLY_EXP,        APPLY_AC,          APPLY_HITROLL,    APPLY_DAMROLL,
          APPLY_DOT,    APPLY_SPELLPOWER, APPLY_SAVING_PARA, APPLY_SAVING_ROD, APPLY_SAVING_SPELL,
          APPLY_SPEED};
      push_const_table(L, "APPLY", keys, vals, 26);
   }

   /* POS — position values (from config.h) */
   {
      static const char *keys[] = {"DEAD",     "MORTAL",  "INCAP",    "STUNNED",
                                   "SLEEPING", "RESTING", "FIGHTING", "STANDING"};
      static const int vals[] = {POS_DEAD,     POS_MORTAL,  POS_INCAP,    POS_STUNNED,
                                 POS_SLEEPING, POS_RESTING, POS_FIGHTING, POS_STANDING};
      push_const_table(L, "POS", keys, vals, 8);
   }

   /* TAR — spell target types (from config.h) */
   {
      static const char *keys[] = {"IGNORE",    "CHAR_OFFENSIVE", "CHAR_DEFENSIVE",
                                   "CHAR_SELF", "OBJ_INV",        "CHAR_NOTSELF"};
      static const int vals[] = {TAR_IGNORE,    TAR_CHAR_OFFENSIVE, TAR_CHAR_DEFENSIVE,
                                 TAR_CHAR_SELF, TAR_OBJ_INV,        TAR_CHAR_NOTSELF};
      push_const_table(L, "TAR", keys, vals, 6);
   }

   /* CLASS — class indices (from config.h) */
   {
      static const char *keys[] = {"MAG", "CLE", "CIP", "WAR", "PSI", "PUG", "SOR", "PAL",
                                   "ASS", "KNI", "NEC", "MON", "WIZ", "PRI", "WLK", "SWO",
                                   "EGO", "BRA", "GMA", "TEM", "NIG", "CRU", "KIN", "MAR",
                                   "DRU", "THO", "WIL", "HIE", "SEN"};
      static const int vals[] = {CLASS_MAG, CLASS_CLE, CLASS_CIP, CLASS_WAR, CLASS_PSI, CLASS_PUG,
                                 CLASS_SOR, CLASS_PAL, CLASS_ASS, CLASS_KNI, CLASS_NEC, CLASS_MON,
                                 CLASS_WIZ, CLASS_PRI, CLASS_WLK, CLASS_SWO, CLASS_EGO, CLASS_BRA,
                                 CLASS_GMA, CLASS_TEM, CLASS_NIG, CLASS_CRU, CLASS_KIN, CLASS_MAR,
                                 CLASS_DRU, CLASS_THO, CLASS_WIL, CLASS_HIE, CLASS_SEN};
      push_const_table(L, "CLASS", keys, vals, 29);
   }

   /* DURATION — affect duration types (from config.h) */
   {
      static const char *keys[] = {"HOUR", "ROUND"};
      static const int vals[] = {DURATION_HOUR, DURATION_ROUND};
      push_const_table(L, "DURATION", keys, vals, 2);
   }

   /* FLAGS — element modifier flags (from magic.h) */
   {
      static const char *keys[] = {"NO_REFLECT", "NO_ABSORB"};
      static const int vals[] = {NO_REFLECT, NO_ABSORB};
      push_const_table(L, "FLAGS", keys, vals, 2);
   }

   /* AOE — area-of-effect flags (from magic.h) */
   {
      static const char *keys[] = {"SAVES", "SKIP_GROUP"};
      static const int vals[] = {AOE_SAVES, AOE_SKIP_GROUP};
      push_const_table(L, "AOE", keys, vals, 2);
   }

   /* CLOAK — cloak affect bitvectors (from config.h) */
   {
      static const char *keys[] = {"REFLECTION", "FLAMING", "ABSORPTION", "ADEPT"};
      static const int vals[] = {AFF_CLOAK_REFLECTION, AFF_CLOAK_FLAMING, AFF_CLOAK_ABSORPTION,
                                 AFF_CLOAK_ADEPT};
      push_const_table(L, "CLOAK", keys, vals, 4);
   }

   /* ITEM_APPLY — item equipment passive apply bits (from config.h) */
   {
      static const char *keys[] = {"NONE",       "INFRA",      "INV",       "DET_INV",    "SANC",
                                   "SNEAK",      "HIDE",       "PROT",      "ENHANCED",   "DET_MAG",
                                   "DET_HID",    "DET_EVIL",   "PASS_DOOR", "DET_POISON", "FLY",
                                   "KNOW_ALIGN", "DET_UNDEAD", "HEATED"};
      static const int vals[] = {ITEM_APPLY_NONE,       ITEM_APPLY_INFRA,      ITEM_APPLY_INV,
                                 ITEM_APPLY_DET_INV,    ITEM_APPLY_SANC,       ITEM_APPLY_SNEAK,
                                 ITEM_APPLY_HIDE,       ITEM_APPLY_PROT,       ITEM_APPLY_ENHANCED,
                                 ITEM_APPLY_DET_MAG,    ITEM_APPLY_DET_HID,    ITEM_APPLY_DET_EVIL,
                                 ITEM_APPLY_PASS_DOOR,  ITEM_APPLY_DET_POISON, ITEM_APPLY_FLY,
                                 ITEM_APPLY_KNOW_ALIGN, ITEM_APPLY_DET_UNDEAD, ITEM_APPLY_HEATED};
      push_const_table(L, "ITEM_APPLY", keys, vals, 18);
   }
}
