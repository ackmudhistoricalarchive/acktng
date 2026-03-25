/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  Ack 2.2 improvements copyright (C) 1994 by Stephen Dooley              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *       _/          _/_/_/     _/    _/     _/    ACK! MUD is modified    *
 *      _/_/        _/          _/  _/       _/    Merc2.0/2.1/2.2 code    *
 *     _/  _/      _/           _/_/         _/    (c)Stephen Zepp 1998    *
 *    _/_/_/_/      _/          _/  _/             Version #: 4.3          *
 *   _/      _/      _/_/_/     _/    _/     _/                            *
 *                                                                         *
 *                        http://ackmud.nuc.net/                           *
 *                        zenithar@ackmud.nuc.net                          *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "quest.h"

#ifndef DEC_ACT_MOB_H
#include "act_mob.h"
#endif
#include "socket.h"

static char buf[MAX_STRING_LENGTH];

/*
 * Local functions.
 */
void affect_modify args((CHAR_DATA * ch, AFFECT_DATA *paf, bool fAdd));

/*
 * Determine the tier (1=MORTAL, 2=REMORT, 3=ADEPT) of a skill from its
 * skill_level array.  Checks which range (0-5, 6-17, 18-23) has a
 * non-NO_USE entry and returns the corresponding tier.
 */
/* skill_get_tier, can_use_skill, raise_skill, skill_success, get_dt_name
 * and related functions have been moved to skills.c.
 */

/*
 * Updated pointer referencing, curtesy of Spectrum, from Beyond the Veil
 *
 */
struct obj_ref_type *obj_ref_list;

void obj_reference(struct obj_ref_type *ref)
{
   if (ref->inuse)
   {
      bugf("Reused obj_reference!");
      abort();
   }

   ref->inuse = TRUE;
   ref->next = obj_ref_list;
   obj_ref_list = ref;
}

void obj_unreference(OBJ_DATA **var)
{
   struct obj_ref_type *p, *last;

   for (p = obj_ref_list, last = NULL; p && p->var != var; last = p, p = p->next)
      ;

   if (!p)
   {
      bugf("obj_unreference: var not found");
      return;
   }

   p->inuse = FALSE;

   if (!last)
      obj_ref_list = obj_ref_list->next;
   else
      last->next = p->next;
}

struct char_ref_type *char_ref_list;

void char_reference(struct char_ref_type *ref)
{
   if (ref->inuse)
   {
      bugf("Reused char_reference!");
      abort();
   }

   ref->inuse = TRUE;
   ref->next = char_ref_list;
   char_ref_list = ref;
}

void char_unreference(CHAR_DATA **var)
{
   struct char_ref_type *p, *last;

   for (p = char_ref_list, last = NULL; p && p->var != var; last = p, p = p->next)
      ;

   if (!p)
   {
      bugf("char_unreference: var not found");
      return;
   }

   p->inuse = FALSE;

   if (!last)
      char_ref_list = char_ref_list->next;
   else
      last->next = p->next;
}

/*
 * Retrieve a character's trusted level for permission checking.
 */
int get_trust(CHAR_DATA *ch)
{
   if (ch->desc != NULL && ch->desc->original != NULL)
      ch = ch->desc->original;

   if (!IS_NPC(ch) && IS_SET(ch->pcdata->pflags, PFLAG_AMBAS))
      return (LEVEL_HERO + 1);

   if (ch->trust != 0)
      return ch->trust;

   if (IS_NPC(ch) && ch->level >= LEVEL_HERO)
      return LEVEL_HERO - 1;
   else
      return ch->level;
}

/*
 * Replacement for retrieving a character's age
 * Each tick = 1 mud hr.  (spaced at 1 minute rl)
 * 24 mud hrs = 1 mud day
 * 20 mud days = 1 mud month
 * 8 mud months = 1 mud year
 * Therefore, 24*20*8 = 3840 ticks/mins.
 * Returns a string with age info in for use by score, mst, etc
 */

void my_get_age(CHAR_DATA *ch, char *buf)
{
   int days, years, months;
   int base, ticks;

   /*
    * Base = time in seconds ch has been playing...
    */
   base = ch->played + (int)(current_time - ch->logon);

   ticks = base / 60; /* 1 tick = 60 seconds */

   days = (ticks / 24) % 20;
   months = (ticks / 480) % 8;
   years = 17 + (ticks / 3840);

   sprintf(buf + strlen(buf), "%d years, %d months and %d days", years, months, days);
   return;
}

/* Simple function to return number of hours a character has played */
int my_get_hours(CHAR_DATA *ch)
{
   int secs;
   int hrs;

   secs = ch->played + (int)(current_time - ch->logon);
   hrs = (secs / 3600);

   return hrs;
}

/*
 * Retrieve a character's age.
 */
int get_age(CHAR_DATA *ch)
{
   return 17 + (ch->played + (int)(current_time - ch->logon)) / 14400;

   /*
    * 12240 assumes 30 second hours, 24 hours a day, 20 day - Kahn
    */
}

long get_cost_to_level_remort(CHAR_DATA *ch, int class)
{
   int base, i;
   bool double_remort = FALSE;

   if (class < CLASS_SOR || class > CLASS_BRA)
      return -69;

   {
      int lv = char_class_level(ch, class);
      base = lv * lv;
   }

   // Gotta check if double remort
   for (i = CLASS_SOR; i < CLASS_SOR + MAX_REMORT; i++)
   {
      if (i != class && char_class_level(ch, i) > 0)
      {
         double_remort = TRUE;
      }
   }

   if (double_remort)
      base *= 11000;
   else
      base *= 5400;

   // Edge-case fix
   base += 690000;

   return get_racial_penalty_to_level(base, ch->race, class);
   ;
}

long get_cost_to_level(CHAR_DATA *ch, int class)
{
   int base = 0;

   if (class > MAX_CLASS)
      return -69;

   {
      int lv = char_class_level(ch, class);
      base = lv * lv;
   }

   base *= 600;

   // Edge-case fix
   base += 350;

   base = get_racial_penalty_to_level(base, ch->race, class);

   /* Primary class (ch->class) gets a 20% discount */
   if (!IS_NPC(ch) && class == ch->class)
      base = base * 80 / 100;

   return base;
}

int get_racial_penalty_to_level(int base, int race, int class)
{
   int cost = base;
   int i;

   for (i = 0; i < MAX_CLASS; i++)
   {
      if (race_table[race].limit[i] == class)
         break;
   }

   cost += (-1 + i) * cost / 10;

   return cost;
}

bool can_wield(CHAR_DATA *ch, OBJ_DATA *obj, int loc)
{
   OBJ_DATA *dual;
   if (loc == WEAR_HOLD_HAND_R)
      dual = get_eq_char(ch, WEAR_HOLD_HAND_L);
   else
      dual = get_eq_char(ch, WEAR_HOLD_HAND_R);

   if (obj == NULL)
      return FALSE;

   if (dual == NULL)
      return TRUE;

   if (!can_use(ch, obj))
      return FALSE;

   if (!IS_WEAPON(obj))
      return TRUE;

   if (IS_WEAPON(obj) && !IS_WEAPON(dual))
      return TRUE;

   if (IS_WEAPON(obj) && IS_WEAPON(dual) && can_use_skill(ch, gsn_dualwield))
      return TRUE;

   if (IS_WEAPON(obj) && IS_SET(obj->extra_flags, ITEM_FIST) && IS_WEAPON(dual) &&
       IS_SET(dual->extra_flags, ITEM_FIST) && can_use_skill(ch, gsn_dual_fist))
      return TRUE;

   return FALSE;
}

/*
 * Return the race's base maximum for a primary stat (APPLY_STR/INT/WIS/DEX/CON).
 */
static int get_race_base_stat(const struct race_type *race, int apply_type)
{
   switch (apply_type)
   {
   case APPLY_STR:
      return race->race_str;
   case APPLY_INT:
      return race->race_int;
   case APPLY_WIS:
      return race->race_wis;
   case APPLY_DEX:
      return race->race_dex;
   case APPLY_CON:
      return race->race_con;
   default:
      return 13;
   }
}

/*
 * Return the reincarnation bonus to a stat maximum for a PC.
 * Each primary stat has a specific class (or pair) that provides a bonus.
 */
static int get_stat_reinc_bonus(CHAR_DATA *ch, int apply_type)
{
   if (IS_NPC(ch))
      return 0;

   switch (apply_type)
   {
   case APPLY_STR:
      if (ch->pcdata->reincarnations[CLASS_WAR] > 0)
         return (ch->pcdata->reincarnations[CLASS_WAR] + 1) / 2;
      return 0;
   case APPLY_INT:
   {
      int bonus = 0;
      if (ch->pcdata->reincarnations[CLASS_MAG] > 0)
         bonus += (ch->pcdata->reincarnations[CLASS_MAG] + 1) / 2;
      if (ch->pcdata->reincarnations[CLASS_PSI] > 0)
         bonus += (ch->pcdata->reincarnations[CLASS_PSI] + 1) / 2;
      return bonus;
   }
   case APPLY_WIS:
      if (ch->pcdata->reincarnations[CLASS_CLE] > 0)
         return (ch->pcdata->reincarnations[CLASS_CLE] + 1) / 2;
      return 0;
   case APPLY_DEX:
      if (ch->pcdata->reincarnations[CLASS_CIP] > 0)
         return (ch->pcdata->reincarnations[CLASS_CIP] + 1) / 2;
      return 0;
   case APPLY_CON:
      if (ch->pcdata->reincarnations[CLASS_PUG] > 0)
         return (ch->pcdata->reincarnations[CLASS_PUG] + 1) / 2;
      return 0;
   default:
      return 0;
   }
}

/*
 * Compute the maximum for any primary stat (APPLY_STR/INT/WIS/DEX/CON).
 * Accounts for race base, class attribute prime bonuses, and reincarnations.
 */
int get_max_stat(CHAR_DATA *ch, int apply_type)
{
   int max = get_race_base_stat(&race_table[ch->race], apply_type);

   if (!IS_NPC(ch) && gclass_table[ch->class].attr_prime == apply_type)
      max++;

   for (int i = 0; i < 2; i++)
   {
      if (ch->remort_class[i] >= 0 && ch->remort_level[i] > 0 &&
          gclass_table[ch->remort_class[i]].attr_prime == apply_type)
         max++;
   }

   if (ch->adept_class >= 0 && ch->adept_level > 0 &&
       gclass_table[ch->adept_class].attr_prime == apply_type)
      max++;

   max += get_stat_reinc_bonus(ch, apply_type);
   return UMIN(max, STAT_MAX);
}

/*
 * Retrieve a character's current value for a primary stat.
 * NPCs get a fixed level-scaled value; PCs get race max + affects, bounded.
 */
static int get_curr_primary_stat(CHAR_DATA *ch, int apply_type)
{
   if (IS_NPC(ch))
      return (13 + (ch->level / 16));

   int max = get_max_stat(ch, apply_type);
   return URANGE(3, max + get_stat(ch, apply_type), max + 3);
}

/*
 * Retrieve character's current strength.
 */
int get_curr_str(CHAR_DATA *ch)
{
   return get_curr_primary_stat(ch, APPLY_STR);
}

int get_max_str(CHAR_DATA *ch)
{
   return get_max_stat(ch, APPLY_STR);
}

/*
 * Retrieve character's current intelligence.
 */
int get_curr_int(CHAR_DATA *ch)
{
   return get_curr_primary_stat(ch, APPLY_INT);
}

int get_max_int(CHAR_DATA *ch)
{
   return get_max_stat(ch, APPLY_INT);
}

/*
 * Retrieve character's current wisdom.
 */
int get_curr_wis(CHAR_DATA *ch)
{
   return get_curr_primary_stat(ch, APPLY_WIS);
}

int get_max_wis(CHAR_DATA *ch)
{
   return get_max_stat(ch, APPLY_WIS);
}

/*
 * Retrieve character's current dexterity.
 */
int get_curr_dex(CHAR_DATA *ch)
{
   return get_curr_primary_stat(ch, APPLY_DEX);
}

int get_max_dex(CHAR_DATA *ch)
{
   return get_max_stat(ch, APPLY_DEX);
}

/*
 * Retrieve character's current constitution.
 */
int get_curr_con(CHAR_DATA *ch)
{
   return get_curr_primary_stat(ch, APPLY_CON);
}

int get_max_con(CHAR_DATA *ch)
{
   return get_max_stat(ch, APPLY_CON);
}

int get_speed(CHAR_DATA *ch)
{
   return get_stat(ch, APPLY_SPEED);
}

int get_spellpower(CHAR_DATA *ch)
{
   int spellpower = get_stat(ch, APPLY_SPELLPOWER);

   if (IS_NPC(ch))
      spellpower += ch->spellpower_mod;

   spellpower += get_psuedo_level(ch) / 3;

   return spellpower;
}

long get_max_hp(CHAR_DATA *ch)
{
   long hp = 25;

   hp += get_stat(ch, APPLY_HIT);

   if (!IS_NPC(ch))
   {
      hp += ch->pcdata->hp_from_gain;
   }
   else
   {
      hp += ch->hp_mod;
      hp += (ch->level * 15) + (ch->level * ch->level * 3);
   }

   return hp;
}

long get_max_mana(CHAR_DATA *ch)
{
   long mana = 100;

   mana += get_stat(ch, APPLY_MANA);

   if (!IS_NPC(ch))
   {
      mana += ch->pcdata->mana_from_gain;
   }
   else
   {
      mana += (ch->level * 5) + (ch->level * ch->level * 2);
   }

   return mana;
}

long get_max_move(CHAR_DATA *ch)
{
   long move = 100;

   move += get_stat(ch, APPLY_MOVE);

   if (!IS_NPC(ch))
   {
      move += ch->pcdata->move_from_gain;
   }
   else
   {
      move += (ch->level * 5) + (ch->level * ch->level * 2);
   }

   return move;
}

int get_healing(CHAR_DATA *ch)
{
   int healing = 0;

   if (IS_NPC(ch))
      healing += ch->healing_mod;

   return healing;
}

int get_total_reincarnations(CHAR_DATA *ch)
{
   int cnt = 0;

   if (IS_NPC(ch))
      return 0;

   for (int i = 0; i < MAX_CLASS; i++)
   {
      cnt += ch->pcdata->reincarnations[i];
   }

   return cnt;
}

int get_spell_crit(CHAR_DATA *ch)
{
   // Base of 5.
   int crit = 5;

   if (!IS_NPC(ch) && can_use_skill(ch, gsn_spell_critical))
   {
      crit += 5;
   }

   crit += get_stat(ch, APPLY_SPELL_CRIT);

   crit += char_class_level(ch, CLASS_SOR) / 10;
   crit += char_class_level(ch, CLASS_WIZ) / 10;
   crit += char_class_level(ch, CLASS_WLK) / 10 * .75;

   crit += ch->spell_crit_mod;

   return crit;
}

int get_spell_crit_mult(CHAR_DATA *ch)
{
   // base of 50%
   int crit = 50;

   crit += get_stat(ch, APPLY_SPELL_CRIT_MULT);

   if (can_use_skill(ch, gsn_spell_critical_damage))
   {
      crit += get_curr_wis(ch) * 2;
   }

   crit += char_class_level(ch, CLASS_PRI) * get_curr_wis(ch) / 50;

   crit += char_class_level(ch, CLASS_PAL) * .75 * get_curr_wis(ch) / 50;

   crit += char_class_level(ch, CLASS_TEM) * get_curr_wis(ch) / 25;

   crit += ch->spell_mult_mod;

   return crit;
}

int get_crit(CHAR_DATA *ch)
{
   OBJ_DATA *wield;
   // Base of 5%
   int crit = get_curr_dex(ch) / 5;

   wield = get_eq_char(ch, WEAR_HOLD_HAND_L);
   if (!IS_WEAPON(wield))
      wield = get_eq_char(ch, WEAR_HOLD_HAND_R);
   if (!IS_WEAPON(wield))
      wield = NULL;

   if (!IS_NPC(ch) && can_use_skill(ch, gsn_enhanced_critical))
   {
      crit += get_curr_dex(ch) / 5;
      crit += 1;
   }

   crit += char_class_level(ch, CLASS_ASS) / 20;
   crit += char_class_level(ch, CLASS_WLK) / 20 * .75;

   crit += char_class_level(ch, CLASS_NIG) / 4;

   if (!IS_NPC(ch) && wield && wield->value[3] == 3 &&
       can_use_skill(ch, gsn_enhanced_sword_critical))
   {
      crit += char_class_level(ch, CLASS_SWO) / 20;
   }

   crit += get_stat(ch, APPLY_CRIT);

   crit += ch->crit_mod;

   return crit;
}

int get_crit_mult(CHAR_DATA *ch)
{
   OBJ_DATA *wield;
   int crit = 50;

   wield = get_eq_char(ch, WEAR_HOLD_HAND_L);
   if (!IS_WEAPON(wield))
      wield = get_eq_char(ch, WEAR_HOLD_HAND_R);
   if (!IS_WEAPON(wield))
      wield = NULL;

   if (!IS_NPC(ch) && can_use_skill(ch, gsn_enhanced_critical))
   {
      crit += get_curr_dex(ch) * 2;
   }

   crit += char_class_level(ch, CLASS_ASS) / 5;
   crit += char_class_level(ch, CLASS_WLK) / 5 * .75;

   crit += char_class_level(ch, CLASS_NIG) / 2;

   if (!IS_NPC(ch) && wield && wield->value[3] == 3 &&
       can_use_skill(ch, gsn_enhanced_sword_critical))
   {
      crit += char_class_level(ch, CLASS_SWO) / 5;
   }

   crit += get_stat(ch, APPLY_CRIT_MULT);

   crit += ch->crit_mult_mod;

   return crit;
}

int get_damcap(CHAR_DATA *ch)
{
   int damcap = BASE_DAMCAP;

   if (is_adept(ch))
      damcap += 500;

   damcap += get_stat(ch, APPLY_DAMCAP);

   if (!IS_NPC(ch))
   {
      damcap += ch->pcdata->reincarnation_data[REINCARNATION_DAMCAP] * 500;
   }

   return damcap;
}

int get_damroll(CHAR_DATA *ch)
{
   int dam = get_stat(ch, APPLY_DAMROLL);

   if (IS_NPC(ch))
   {
      dam += ch->dr_mod;
      dam += ch->level;
   }

   dam += get_psuedo_level(ch) / 3;

   if (stance_app[ch->stance].dr_mod != 0)
   {
      dam += dam * stance_app[ch->stance].dr_mod / 20;
      dam += stance_app[ch->stance].dr_mod;
   }

   return dam;
}

int get_hitroll(CHAR_DATA *ch)
{
   int hit = get_stat(ch, APPLY_HITROLL);

   if (IS_NPC(ch))
   {
      hit += ch->hr_mod;
      hit += ch->level * 2;
   }

   hit += get_psuedo_level(ch) / 3;

   if (stance_app[ch->stance].hr_mod != 0)
   {
      hit += hit * stance_app[ch->stance].hr_mod / 20;
      hit += stance_app[ch->stance].hr_mod;
   }

   return hit;
}

int get_ac(CHAR_DATA *ch)
{
   int ac = get_stat(ch, APPLY_AC);

   if (IS_NPC(ch))
   {
      ac += ch->ac_mod;
      ac -= ch->level * 4;
   }
   ac -= get_psuedo_level(ch) * 2;

   if (stance_app[ch->stance].ac_mod != 0)
   {
      ac -= ac * stance_app[ch->stance].ac_mod / 10;
      ac -= stance_app[ch->stance].ac_mod * 10;
   }

   return ac;
}

int get_stat(CHAR_DATA *ch, int stat)
{
   int stat_val = 0;
   OBJ_DATA *obj;
   AFFECT_DATA *paf;

   /* First we check gear */
   for (int i = 0; i < MAX_WEAR; i++)
   {
      if ((obj = get_eq_char(ch, i)) != NULL)
      {
         for (paf = obj->first_apply; paf != NULL; paf = paf->next)
         {
            if (paf->location == stat && paf->modifier != 0)
            {
               stat_val += paf->modifier;
            }
         }
      }
   }

   /* Then we check affects */
   for (paf = ch->first_affect; paf != NULL; paf = paf->next)
   {
      if (paf->location == stat && paf->modifier != 0)
      {
         stat_val += paf->modifier;
      }
   }

   return stat_val;
}

int get_max_carry_weight(CHAR_DATA *ch)
{
   if (get_curr_str(ch) > 24)
      return 800;
   if (get_curr_str(ch) > 19)
      return 700;

   return 500;
}

int get_max_carry(CHAR_DATA *ch)
{
   return 15 + MAX_WEAR;
}

int get_generation_chance()
{
   bool chance = 5;

   if (happy_hour)
      chance *= 2;

   return chance;
}

bool learn_skill(CHAR_DATA *ch, int gsn)
{
   if (IS_NPC(ch))
      return FALSE;

   if (ch->pcdata->learned[gsn] > 2000000000)
      return FALSE;

   ch->pcdata->learned[gsn]++;

   return TRUE;
}

bool is_fighting(CHAR_DATA *ch)
{
   return ch->fighting != NULL;
}

char *stat_to_string(int stat)
{
   if (stat == APPLY_CON)
      return "Con";

   if (stat == APPLY_STR)
      return "Str";

   if (stat == APPLY_WIS)
      return "Wis";

   if (stat == APPLY_INT)
      return "Int";

   if (stat == APPLY_DEX)
      return "Dex";

   return "null";
}

char *class_order(int race)
{
   static char buf[MSL];

   sprintf(buf, "%s %s %s %s %s %s", gclass_table[race_table[race].limit[0]].who_name,
           gclass_table[race_table[race].limit[1]].who_name,
           gclass_table[race_table[race].limit[2]].who_name,
           gclass_table[race_table[race].limit[3]].who_name,
           gclass_table[race_table[race].limit[4]].who_name,
           gclass_table[race_table[race].limit[5]].who_name);

   return buf;
}

/*
 * Return the slot index (0-3) of class_idx in ch->mortal_class[], or -1.
 */
int char_mortal_slot(const CHAR_DATA *ch, int class_idx)
{
   for (int i = 0; i < 4; i++)
      if (ch->mortal_class[i] == class_idx)
         return i;
   return -1;
}

/*
 * Return the slot index (0-1) of class_idx in ch->remort_class[], or -1.
 */
int char_remort_slot(const CHAR_DATA *ch, int class_idx)
{
   for (int i = 0; i < 2; i++)
      if (ch->remort_class[i] == class_idx)
         return i;
   return -1;
}

bool char_has_mortal_class(const CHAR_DATA *ch, int class_idx)
{
   return char_mortal_slot(ch, class_idx) >= 0;
}

bool char_has_remort_class(const CHAR_DATA *ch, int class_idx)
{
   return char_remort_slot(ch, class_idx) >= 0;
}

/*
 * Return the character's level in the given class index.
 * Returns 0 if the class is not chosen or has no levels yet.
 * Works for mortal, remort, and adept classes.
 */
int char_class_level(const CHAR_DATA *ch, int class_idx)
{
   int slot;

   if (IS_NPC(ch))
      return (ch->class == class_idx) ? ch->level : 0;

   slot = char_mortal_slot(ch, class_idx);
   if (slot >= 0)
      return ch->mortal_level[slot];

   slot = char_remort_slot(ch, class_idx);
   if (slot >= 0)
      return ch->remort_level[slot];

   if (ch->adept_class == class_idx)
      return ch->adept_level;

   return 0;
}

int get_wear_weight(CHAR_DATA *ch)
{
   if (get_curr_str(ch) > 19)
      return 3;

   if (get_curr_str(ch) > 9)
      return 2;

   return 1;
}

char *get_item_class(OBJ_DATA *obj)
{
   if (obj->weight > 10 && obj->weight < 16)
      return "tank";
   else if (obj->weight > 5)
      return "melee";
   else if (obj->weight > 0)
      return "caster";

   return "bugged, contact staff.";
}

bool is_same_room(CHAR_DATA *ch, CHAR_DATA *victim)
{
   if (ch == NULL)
      return FALSE;

   if (victim == NULL)
      return FALSE;

   if (ch->in_room == NULL)
      return FALSE;

   if (victim->in_room == NULL)
      return FALSE;

   if (ch->in_room->vnum == victim->in_room->vnum)
      return TRUE;

   return FALSE;
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n(CHAR_DATA *ch)
{
   if (!IS_NPC(ch) && ch->level >= LEVEL_STAFF)
      return 500;

   /*
    * if ( IS_NPC(ch) && IS_SET(ch->act, ACT_PET) )
    * return 0;
    */

   return get_max_carry(ch);
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w(CHAR_DATA *ch)
{
   if (!IS_NPC(ch) && ch->level >= LEVEL_STAFF)
      return 9999999;

   /*    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_PET) )
      return 0;   */

   return get_max_carry_weight(ch);
}

/* affect_modify, mark_to_room, mark_from_room, affect_to_room, r_affect_remove,
 * affect_to_char, affect_remove, affect_strip, is_affected, affect_join
 * have been moved to affect.c.
 */

/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *get_light_char(CHAR_DATA *ch)
{
   OBJ_DATA *obj;

   for (obj = ch->first_carry; obj != NULL; obj = obj->next_in_carry_list)
   {
      if ((obj->wear_loc != -1) && (obj->item_type == ITEM_LIGHT))
         return obj;
   }

   return NULL;
}

/*
 * Move a char out of a room.
 */
void char_from_room(CHAR_DATA *ch)
{
   OBJ_DATA *obj;

   if (ch->in_room == NULL)
   {
      sprintf(buf, "char_from_room: %s in NULL room.", IS_NPC(ch) ? ch->short_descr : ch->name);
      monitor_chan(buf, MONITOR_ROOM);

      bug("Char_from_room: NULL.", 0);
      return;
   }

   if (!IS_NPC(ch))
      --ch->in_room->area->nplayer;

   if ((obj = get_light_char(ch)) != NULL)
      --ch->in_room->light;

   UNLINK(ch, ch->in_room->first_person, ch->in_room->last_person, next_in_room, prev_in_room);

   ch->in_room = NULL;
   ch->next_in_room = NULL;
   ch->prev_in_room = NULL;

   return;
}

/*
 * Move a char into a room.
 */
void char_to_room(CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex)
{
   OBJ_DATA *obj;
   ROOM_AFFECT_DATA *raf;
   ROOM_AFFECT_DATA *raf_next;
   AFFECT_DATA af;
   if (pRoomIndex == NULL)
   {
      sprintf(buf, "char_to_room: Attempted to move %s to a NULL room.", NAME(ch));
      monitor_chan(buf, MONITOR_ROOM);

      bug("Char_to_room: NULL.", 0);
   }

   ch->in_room = pRoomIndex;
   if (IS_NPC(ch))
      TOPLINK(ch, pRoomIndex->first_person, pRoomIndex->last_person, next_in_room, prev_in_room);
   else
      LINK(ch, pRoomIndex->first_person, pRoomIndex->last_person, next_in_room, prev_in_room);

   if (!IS_NPC(ch))
      ++ch->in_room->area->nplayer;

   if ((obj = get_light_char(ch)) != NULL)
      ++ch->in_room->light;

   if (is_fighting(ch))
      if (ch->fighting->in_room != ch->in_room)
      {
         ch->fighting = NULL;
         ch->position = POS_STANDING;
      }

   if (IS_SET(ch->in_room->affected_by, ROOM_BV_FIRE_RUNE))
   {
      send_to_char("@@NAs you step into the room, you fleetingly see a mystical @@eFire@@N Rune "
                   "suspended in front of you, which then @@eEXPLODES@@N!!!\n\r",
                   ch);
      act("@@NThe @@eFire@@N Rune explodes as $n enters the room!", ch, NULL, NULL, TO_ROOM);

      for (raf = ch->in_room->first_room_affect; raf != NULL; raf = raf_next)
      {
         raf_next = raf->next;

         if (raf->bitvector == ROOM_BV_FIRE_RUNE)
         {
            ch->hit -= raf->modifier;
            r_affect_remove(ch->in_room, raf);
         }
      }
   }

   if (IS_SET(ch->in_room->affected_by, ROOM_BV_SHOCK_RUNE) && (ch->is_free == FALSE))
   {
      send_to_char("@@NAs you step into the room, you fleetingly see a mystical @@lShock@@N Rune "
                   "suspended in front of you, which then @@lZAPS@@N You!!!\n\r",
                   ch);
      act("@@NThe @@lShock@@N Rune flashes as $n enters the room!", ch, NULL, NULL, TO_ROOM);

      for (raf = ch->in_room->first_room_affect; raf != NULL; raf = raf_next)
      {
         raf_next = raf->next;

         if (raf->bitvector == ROOM_BV_SHOCK_RUNE)
         {
            ch->hit -= raf->modifier;
            r_affect_remove(ch->in_room, raf);
         }
      }
   }

   if (IS_SET(ch->in_room->affected_by, ROOM_BV_POISON_RUNE) && (ch->is_free == FALSE))
   {
      send_to_char("@@NAs you step into the room, you fleetingly see a mystical @@dPoison@@N Rune "
                   "suspended in front of you, which then @@dEXPLODES@@N!!!\n\r",
                   ch);
      act("@@NThe @@dPoison@@N Rune explodes as $n enters the room!", ch, NULL, NULL, TO_ROOM);

      for (raf = ch->in_room->first_room_affect; raf != NULL; raf = raf_next)
      {
         sh_int caster_level = 0;
         raf_next = raf->next;

         if (raf->bitvector == ROOM_BV_POISON_RUNE)
         {
            if (raf->caster == NULL)
            {
               caster_level = get_psuedo_level(ch);
            }
            else
            {
               caster_level = raf->caster->level;
            }
            if (!saves_spell(caster_level, ch))
            {
               af.type = skill_lookup("poison");
               af.duration = 12 + (caster_level / 10);
               af.location = APPLY_STR;
               af.modifier = -2;
               af.bitvector = AFF_POISON;
               affect_join(ch, &af);
               send_to_char("You feel very sick.\n\r", ch);
               act("$n looks very sick.", ch, NULL, NULL, TO_ROOM);
            }
            r_affect_remove(ch->in_room, raf);
         }
      }
   }

   send_area_music(ch);
   return;
}

/*
 * Give an obj to a char.
 */
void obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch)
{
   obj->next_in_carry_list = NULL;
   obj->prev_in_carry_list = NULL;

   if ((!IS_NPC(ch)) && (obj == quest_object))
   {
      bool valid_questor = FALSE;
      sh_int average_level;

      average_level = obj->value[3];

      if ((average_level < 20) && (get_psuedo_level(ch) < 45))
      {
         valid_questor = TRUE;
      }
      else if ((average_level < 65) && (get_psuedo_level(ch) > 45) && (get_psuedo_level(ch) < 95))
      {
         valid_questor = TRUE;
      }
      else if ((get_psuedo_level(ch) > 90) && (average_level > 65))
      {
         valid_questor = TRUE;
      }

      if (valid_questor == FALSE)
      {
         act("$n fumbles, trying to hold $p, and it falls to the ground.", ch, obj, NULL, TO_ROOM);
         act("You try to hold $p, but it seems to come alive, and slips from your grasp and falls "
             "to the ground.",
             ch, obj, NULL, TO_CHAR);
         obj_to_room(obj, ch->in_room);
         return;
      }
   }

   LINK(obj, ch->first_carry, ch->last_carry, next_in_carry_list, prev_in_carry_list);
   obj->carried_by = ch;
   obj->in_room = NULL;
   obj->in_obj = NULL;
   obj->next_in_room = NULL;
   obj->prev_in_room = NULL;
   ch->carry_number += get_obj_number(obj);
   ch->carry_weight += get_obj_weight(obj);

   if (!IS_NPC(ch))
      quest_obj_notify(ch, obj);
}

/*
 * Take an obj from its character.
 */
void obj_from_char(OBJ_DATA *obj)
{
   CHAR_DATA *ch;

   if ((ch = obj->carried_by) == NULL)
   {
      sprintf(buf, "obj_from_char: NULL ch to remove %s from.", obj->short_descr);
      monitor_chan(buf, MONITOR_OBJ);

      bug("Obj_from_char: null ch.", 0);
      return;
   }

   if (obj->wear_loc != WEAR_NONE)
      unequip_char(ch, obj);

   UNLINK(obj, ch->first_carry, ch->last_carry, next_in_carry_list, prev_in_carry_list);

   obj->carried_by = NULL;
   obj->next_in_carry_list = NULL;
   obj->prev_in_carry_list = NULL;
   obj->next_in_room = NULL;
   obj->prev_in_room = NULL;
   obj->in_room = NULL;
   obj->in_obj = NULL;

   ch->carry_number -= get_obj_number(obj);
   ch->carry_weight -= get_obj_weight(obj);
   return;
}

/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac(OBJ_DATA *obj, int iWear)
{
   if (obj->item_type != ITEM_ARMOR)
      return 0;

   switch (iWear)
   {
   case WEAR_BODY:
      return 3 * obj->value[0];
   case WEAR_HEAD:
      return 2 * obj->value[0];
   case WEAR_LEGS:
      return 2 * obj->value[0];
   case WEAR_FEET:
      return obj->value[0];
   case WEAR_HANDS:
      return obj->value[0];
   case WEAR_ARMS:
      return obj->value[0];
   case WEAR_HOLD_HAND_R:
      return obj->value[0];
   case WEAR_FINGER_L:
      return obj->value[0];
   case WEAR_FINGER_R:
      return obj->value[0];
   case WEAR_NECK_1:
      return obj->value[0];
   case WEAR_NECK_2:
      return obj->value[0];
   case WEAR_ABOUT:
      return 2 * obj->value[0];
   case WEAR_WAIST:
      return obj->value[0];
   case WEAR_WRIST_L:
      return obj->value[0];
   case WEAR_WRIST_R:
      return obj->value[0];
   case WEAR_HOLD_HAND_L:
      return obj->value[0];
   }

   return 0;
}

/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *get_eq_char(CHAR_DATA *ch, int iWear)
{
   OBJ_DATA *obj;

   for (obj = ch->first_carry; obj != NULL; obj = obj->next_in_carry_list)
   {
      if (obj->wear_loc == iWear)
         return obj;
   }

   return NULL;
}

/*
 * Equip a char with an obj.
 */
void equip_char(CHAR_DATA *ch, OBJ_DATA *obj, int iWear)
{
   AFFECT_DATA *paf;
   char log[MAX_STRING_LENGTH];

   if ((!IS_NPC(ch) && ch->desc->connected != CON_SETTING_STATS) &&
       (get_eq_char(ch, iWear) != NULL))
   {
      sprintf(log,
              "equip_char: %s (room %d) cannot be equiped with %s, as wear slot (%d) not empty.",
              NAME(ch), ch->in_room->vnum, obj->short_descr, iWear);
      monitor_chan(log, MONITOR_OBJ);

      bug(log, 0);
      return;
   }

   if ((!IS_NPC(ch) && ch->desc->connected != CON_SETTING_STATS) &&
       ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
        (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))))
   {
      act("You feel $p slither out of your grasp, and back into your inventory!", ch, obj, NULL,
          TO_CHAR);
      act("$p slithers out of $n's hands and back into $s inventory!", ch, obj, NULL, TO_ROOM);
      return;
   }

   ch->armor -= apply_ac(obj, iWear);
   obj->wear_loc = iWear;

   if (IS_SET(obj->extra_flags, ITEM_BIND_EQUIP))
   {
      act("You feel $p bind to you.\n\r", ch, obj, NULL, TO_CHAR);
      SET_BIT(obj->extra_flags, ITEM_NODROP);
      REMOVE_BIT(obj->extra_flags, ITEM_BIND_EQUIP);
   }

   for (paf = obj->first_apply; paf != NULL; paf = paf->next)
      affect_modify(ch, paf, TRUE);

   /*
    * spec: light bugfix
    */
   if ((IS_NPC(ch) || !ch->desc || ch->desc->connected != CON_SETTING_STATS) &&
       obj->item_type == ITEM_LIGHT && ch->in_room != NULL)
      ++ch->in_room->light;

   /*
    * Check to see if object has magical affects...
    */

   if (IS_SET(obj->item_apply, ITEM_APPLY_INFRA))
   {
      act("$n's eyes glow brightly.", ch, NULL, NULL, TO_ROOM);
      send_to_char("Your eyes glow brightly!\n\r", ch);
   }

   if (IS_SET(obj->item_apply, ITEM_APPLY_INV))
   {
      act("$n slowly fades out of existance.", ch, NULL, NULL, TO_ROOM);
      send_to_char("You slowly fade out of existance.\n\r", ch);
   }

   if (IS_SET(obj->item_apply, ITEM_APPLY_DET_INV))
      send_to_char("You feel more aware of your surroundings.\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_SANC))
   {
      act("$n is surrounded by a white aura.", ch, NULL, NULL, TO_ROOM);
      send_to_char("You are surrounded by a white aura.\n\r", ch);
   }

   if (IS_SET(obj->item_apply, ITEM_APPLY_SNEAK))
      send_to_char("You feel all sneaky!\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_HIDE))
      send_to_char("You feel almost hidden.\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_PROT))
      send_to_char("You feel more protected.\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_ENHANCED))
      send_to_char("You feel much meaner!\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_DET_MAG) || IS_SET(obj->item_apply, ITEM_APPLY_DET_HID) ||
       IS_SET(obj->item_apply, ITEM_APPLY_DET_EVIL) ||
       IS_SET(obj->item_apply, ITEM_APPLY_KNOW_ALIGN) ||
       IS_SET(obj->item_apply, ITEM_APPLY_DET_POISON))
      send_to_char("Your eyes tingle slightly.\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_PASS_DOOR))
   {
      act("$n turns turns very pale.", ch, NULL, NULL, TO_ROOM);
      send_to_char("You feel almost weightless!\n\r", ch);
   }

   if (IS_SET(obj->item_apply, ITEM_APPLY_FLY))
   {
      act("$n gently floats into the air!", ch, NULL, NULL, TO_ROOM);
      send_to_char("You gently float upwards!\n\r", ch);
   }

   return;
}

/*
 * Unequip a char with an obj.
 */
void unequip_char(CHAR_DATA *ch, OBJ_DATA *obj)
{
   AFFECT_DATA *paf;

   if (obj->wear_loc == WEAR_NONE)
   {
      sprintf(buf, "unequip_char: %s is not wearing %s.", NAME(ch), obj->short_descr);
      monitor_chan(buf, MONITOR_OBJ);

      bug("Unequip_char: already unequipped.", 0);
      return;
   }

   ch->armor += apply_ac(obj, obj->wear_loc);
   obj->wear_loc = -1;

   /*
    * for ( paf = obj->pIndexData->first_apply; paf != NULL; paf = paf->next )
    * affect_modify( ch, paf, FALSE );
    */
   for (paf = obj->first_apply; paf != NULL; paf = paf->next)
      affect_modify(ch, paf, FALSE);

   if (obj->item_type == ITEM_LIGHT && ch->in_room != NULL && ch->in_room->light > 0)
      --ch->in_room->light;

   /*
    * Check to see if object has magical affects...
    */

   if (IS_SET(obj->item_apply, ITEM_APPLY_INFRA))
   {
      act("$n's eyes become dim.", ch, NULL, NULL, TO_ROOM);
      send_to_char("Your eyes become dim!\n\r", ch);
   }

   if (IS_SET(obj->item_apply, ITEM_APPLY_INV))
   {
      act("$n slowly fades back into existance.", ch, NULL, NULL, TO_ROOM);
      send_to_char("You slowly fade into existance.\n\r", ch);
   }

   if (IS_SET(obj->item_apply, ITEM_APPLY_DET_INV))
      send_to_char("You feel less aware of your surroundings.\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_SANC))
   {
      act("The white aura around $n fades.", ch, NULL, NULL, TO_ROOM);
      send_to_char("The white aura around you fades.\n\r", ch);
   }

   if (IS_SET(obj->item_apply, ITEM_APPLY_SNEAK))
      send_to_char("You feel less sneaky!\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_HIDE))
      send_to_char("You feel less hidden.\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_PROT))
      send_to_char("You feel less protected.\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_ENHANCED))
      send_to_char("You feel much wimpier!\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_DET_MAG) || IS_SET(obj->item_apply, ITEM_APPLY_DET_HID) ||
       IS_SET(obj->item_apply, ITEM_APPLY_DET_EVIL) ||
       IS_SET(obj->item_apply, ITEM_APPLY_KNOW_ALIGN) ||
       IS_SET(obj->item_apply, ITEM_APPLY_DET_POISON))
      send_to_char("Your feel less well-informed.\n\r", ch);

   if (IS_SET(obj->item_apply, ITEM_APPLY_PASS_DOOR))
   {
      act("$n becomes solid again.", ch, NULL, NULL, TO_ROOM);
      send_to_char("You feel more solid!\n\r", ch);
   }

   if (IS_SET(obj->item_apply, ITEM_APPLY_FLY))
   {
      act("$n gently floats to the ground!", ch, NULL, NULL, TO_ROOM);
      send_to_char("You gently float to the ground!\n\r", ch);
   }
   if (ch->mana > get_max_mana(ch))
      ch->mana = get_max_mana(ch);
   if (ch->hit > get_max_hp(ch))
      ch->hit = get_max_hp(ch);

   return;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list(OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list)
{
   OBJ_DATA *obj;
   int nMatch;

   nMatch = 0;
   for (obj = list; obj != NULL; obj = obj->next_in_carry_list)
   {
      if (obj->pIndexData == pObjIndex)
         nMatch++;
   }

   return nMatch;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_room(OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list)
{
   OBJ_DATA *obj;
   int nMatch;

   nMatch = 0;
   for (obj = list; obj != NULL; obj = obj->next_in_room)
   {
      if (obj->pIndexData == pObjIndex)
         nMatch++;
   }

   return nMatch;
}

static void register_persistent_container(OBJ_DATA *obj)
{
   CORPSE_DATA *this_corpse;

   if (obj == NULL)
      return;

   if (obj->item_type != ITEM_CORPSE_PC)
      return;

   for (this_corpse = first_corpse; this_corpse != NULL; this_corpse = this_corpse->next)
      if (this_corpse->this_corpse == obj)
         return;

   GET_FREE(this_corpse, corpse_free);
   this_corpse->next = NULL;
   this_corpse->prev = NULL;
   this_corpse->this_corpse = obj;
   LINK(this_corpse, first_corpse, last_corpse, next, prev);
}

/*
 * Move an obj out of a room.
 */
void obj_from_room(OBJ_DATA *obj)
{
   ROOM_INDEX_DATA *in_room;

   if ((in_room = obj->in_room) == NULL)
   {
      sprintf(buf, "obj_from_room: %s in NULL room.", obj->short_descr);
      monitor_chan(buf, MONITOR_OBJ);

      bug("obj_from_room: NULL.", 0);
      /* attempt to recover by moving obj to another room */
      if (obj->carried_by != NULL)
         obj_from_char(obj);
      else if (obj->in_obj != NULL)
         obj_from_obj(obj);

      obj_to_room(obj, get_room_index(ROOM_VNUM_LIMBO));
      if ((in_room = obj->in_room) == NULL)
      {
         sprintf(buf, "obj_from_room, %s really screwed up, failed attempts to move to Limbo.",
                 obj->short_descr);
         monitor_chan(buf, MONITOR_OBJ);
         return;
      }
      /*
       * code to save everyone here  Zen
       */
   }

   UNLINK(obj, in_room->first_content, in_room->last_content, next_in_room, prev_in_room);

   obj->in_room = NULL;
   obj->next_in_room = NULL;
   obj->prev_in_room = NULL;
   obj->next_in_carry_list = NULL;
   obj->prev_in_carry_list = NULL;
   obj->carried_by = NULL;
   obj->in_obj = NULL;

   return;
}

/*
 * Move an obj into a room.
 */
void obj_to_room(OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex)
{
   obj->next_in_room = NULL;
   obj->prev_in_room = NULL;

   TOPLINK(obj, pRoomIndex->first_content, pRoomIndex->last_content, next_in_room, prev_in_room);
   obj->in_room = pRoomIndex;
   obj->carried_by = NULL;
   obj->in_obj = NULL;
   obj->next_in_carry_list = NULL;
   obj->prev_in_carry_list = NULL;

   register_persistent_container(obj);
   return;
}

/*
 * Move an object into an object.
 */
void obj_to_obj(OBJ_DATA *obj, OBJ_DATA *obj_to)
{
   obj->next_in_carry_list = NULL;
   obj->prev_in_carry_list = NULL;

   TOPLINK(obj, obj_to->first_in_carry_list, obj_to->last_in_carry_list, next_in_carry_list,
           prev_in_carry_list);
   obj->in_obj = obj_to;
   obj->in_room = NULL;
   obj->carried_by = NULL;
   obj->next_in_room = NULL;
   obj->prev_in_room = NULL;
   for (; obj_to != NULL; obj_to = obj_to->in_obj)
      if (obj_to->carried_by != NULL)
      {
         /*
          * obj_to->carried_by->carry_number += get_obj_number( obj );
          */
         obj_to->carried_by->carry_weight += get_obj_weight(obj);
      }
   return;
}

/*
 * Move an object out of an object.
 */
void obj_from_obj(OBJ_DATA *obj)
{
   OBJ_DATA *obj_from;

   if ((obj_from = obj->in_obj) == NULL)
   {
      sprintf(buf, "obj_from_obj: %s not in another object.", obj->short_descr);
      monitor_chan(buf, MONITOR_OBJ);
      bug("Obj_from_obj: null obj_from.", 0);
      return;
   }

   UNLINK(obj, obj_from->first_in_carry_list, obj_from->last_in_carry_list, next_in_carry_list,
          prev_in_carry_list);

   obj->next_in_carry_list = NULL;
   obj->prev_in_carry_list = NULL;
   obj->in_obj = NULL;
   obj->next_in_room = NULL;
   obj->prev_in_room = NULL;
   obj->carried_by = NULL;
   obj->in_room = NULL;

   for (; obj_from != NULL; obj_from = obj_from->in_obj)
   {
      if (obj_from->carried_by != NULL)
      {
         /*	    obj_from->carried_by->carry_number -= get_obj_number( obj );  */
         obj_from->carried_by->carry_weight -= get_obj_weight(obj);
      }
   }

   return;
}

/*
 * Extract an obj from the world.
 */
void extract_obj(OBJ_DATA *obj)
{
   CHAR_DATA *wch;
   OBJ_DATA *obj_content;
   struct obj_ref_type *ref;
   ROOM_INDEX_DATA *drop_room = NULL;
   for (ref = obj_ref_list; ref; ref = ref->next)
      if (*ref->var == obj)
         switch (ref->type)
         {
         case OBJ_NEXT:
            *ref->var = obj->next;
            break;
         case OBJ_NEXTCONTENT:
            *ref->var = obj->next_in_carry_list;
            break;
         case OBJ_NULL:
            *ref->var = NULL;
            break;
         default:
            bugf("Bad obj_ref_list type %d", ref->type);
            break;
         }

   if ((obj == quest_object) && quest)
   {
      if ((obj->in_obj != NULL) && ((obj->in_obj->item_type == ITEM_CORPSE_NPC) ||
                                    (obj->in_obj->item_type == ITEM_CORPSE_PC)))
      {
         drop_room = obj->in_obj->in_room;
         obj_from_obj(obj);
      }
      else if (obj->carried_by != NULL)
      {
         drop_room = obj->carried_by->in_room;
         obj_from_char(obj);
      }
      else if (obj->in_room != NULL)
      {
         drop_room = obj->in_room;
         obj_from_room(obj);
      }

      obj_to_room(obj, (drop_room != NULL ? drop_room : get_room_index(ROOM_VNUM_TEMPLE)));
      return;
   }

   if (obj->carried_by != NULL)
      obj_from_char(obj);
   else if (obj->in_room != NULL)
      obj_from_room(obj);
   else if (obj->in_obj != NULL)
      obj_from_obj(obj);

   while ((obj_content = obj->last_in_carry_list) != NULL)
      extract_obj(obj_content);

   UNLINK(obj, first_obj, last_obj, next, prev);

   {
      AFFECT_DATA *paf;

      while ((paf = obj->first_apply) != NULL)
      {
         obj->first_apply = paf->next;
         PUT_FREE(paf, affect_free);
      }
   }

   {
      EXTRA_DESCR_DATA *ed;

      while ((ed = obj->first_exdesc) != NULL)
      {
         obj->first_exdesc = ed->next;
         PUT_FREE(ed, exdesc_free);
      }
   }

   for (wch = first_char; wch; wch = wch->next)
   {
      MPROG_ACT_LIST *mpact;

      if (wch->hunt_obj == obj)
         end_hunt(wch);
      /*        wch->hunt_obj = NULL;*/
      if (wch->sitting == obj)
         do_stand(wch, "");
      for (mpact = wch->first_mpact; mpact; mpact = mpact->next)
      {
         if (mpact->obj == obj)
            mpact->obj = NULL;
         if (mpact->vo == obj)
            mpact->vo = NULL;
      }
   }

   if (obj->item_type == ITEM_CORPSE_PC)
   {
      CORPSE_DATA *this_corpse;
      for (this_corpse = first_corpse; this_corpse != NULL; this_corpse = this_corpse->next)
         if (this_corpse->this_corpse == obj)
            break;
      if (this_corpse != NULL)
      {
         UNLINK(this_corpse, first_corpse, last_corpse, next, prev);
         PUT_FREE(this_corpse, corpse_free);
      }
      save_corpses();
   }
   --obj->pIndexData->count;
   PUT_FREE(obj, obj_free);
   return;
}

/*
 * Extract a char from the world.
 */
void extract_char(CHAR_DATA *ch, bool fPull)
{
   CHAR_DATA *wch;
   OBJ_DATA *this_object;
   struct char_ref_type *ref;

   /*
    * Updated pointer referencing, curtesy of Spectrum, from Beyond the Veil
    *
    */

   if (ch->in_room == NULL)
   {
      sprintf(buf, "extract_char: %s in NULL room., Moved to room 2", NAME(ch));
      monitor_chan(buf, MONITOR_MOB);

      bug("Extract_char: NULL.", 0);

      /*
       * char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
       */

      return;
   }

   for (ref = char_ref_list; ref; ref = ref->next)
      if (*ref->var == ch)
         switch (ref->type)
         {
         case CHAR_NEXT:
            *ref->var = ch->next;
            break;
         case CHAR_NEXTROOM:
            *ref->var = ch->next_in_room;
            break;
         case CHAR_NULL:
            *ref->var = NULL;
            break;
         default:
            bugf("Bad char_ref_list type %d", ref->type);
            break;
         }

   if ((ch == quest_mob) || (ch == quest_target))
      crusade_cancel();

   if (fPull)
      die_follower(ch);

   stop_fighting(ch, TRUE);

   /* Clear testimony_target pointers for any Sentinel targeting this char */
   {
      CHAR_DATA *tch;
      for (tch = first_char; tch; tch = tch->next)
      {
         if (tch->testimony_target == ch)
         {
            tch->testimony = 0;
            tch->testimony_target = NULL;
            tch->testimony_combat_rounds = 0;
         }
      }
   }
   /* Reset this char's own testimony */
   ch->testimony = 0;
   ch->testimony_target = NULL;
   ch->testimony_cooldown = 0;
   ch->testimony_combat_rounds = 0;
   ch->is_quitting = TRUE;
   while ((this_object = ch->last_carry) != NULL)
      extract_obj(this_object);

   char_from_room(ch);

   if (!fPull)
   {
      char_to_room(ch, get_room_index(ROOM_VNUM_MORIBUND));
      return;
   }

   if (IS_NPC(ch))
      --ch->pIndexData->count;

   if (ch->desc != NULL && ch->desc->original != NULL)
      do_return(ch, "");

   for (wch = first_char; wch != NULL; wch = wch->next)
   {
      AFFECT_DATA *paf;

      if (wch->reply == ch)
         wch->reply = NULL;
      if (wch->hunting == ch || wch->hunt_for == ch)
      {
         end_hunt(wch);
         /*
          * Aeria put this in for searching
          */
         if (IS_NPC(wch))
         {
            wch->searching = ch->name;
         }
      }
      if (!str_cmp(wch->target, ch->name))
      {
         free_string(wch->target);
         wch->target = NULL; /* spec- fix the evil nasty duplicate frees */
      }
      for (paf = wch->first_affect; paf; paf = paf->next)
         if (paf->caster == ch)
            paf->caster = NULL;
   }

   /* free up any shields  */

   if (ch->first_shield != NULL)
   {
      MAGIC_SHIELD *this_shield;
      MAGIC_SHIELD *this_shield_next;

      for (this_shield = ch->first_shield; this_shield != NULL; this_shield = this_shield_next)
      {
         this_shield_next = this_shield->next;
         UNLINK(this_shield, ch->first_shield, ch->last_shield, next, prev);
         PUT_FREE(this_shield, shield_free);
      }
   }

   if (!IS_NPC(ch) && IS_SET(ch->pcdata->pflags, PFLAG_SUPER_COUNCIL))
   {
      sh_int this_council;
      MEMBER_DATA *imember;
      MEMBER_DATA *imember_next;

      this_council = SUPER_NONE;
      if (this_council != SUPER_NONE && super_councils[this_council].council_time > 0)
         for (imember = super_councils[this_council].first_member; imember != NULL;
              imember = imember_next)
         {
            imember_next = imember->next;
            if (imember->this_member == ch)
            {
               UNLINK(imember, super_councils[this_council].first_member,
                      super_councils[this_council].last_member, next, prev);
               imember->this_member = NULL;
               imember->next = NULL;
               imember->prev = NULL;
               PUT_FREE(imember, member_free);
               continue;
            }
         }
   }
   if (ch->ngroup != NULL)
   {
      NPC_GROUP_DATA *kill_group = NULL;
      DL_LIST *kill_member = NULL;
      for (kill_group = first_npc_group; kill_group; kill_group = kill_group->next)
      {
         if (kill_group->leader == ch)
         {
            UNLINK(kill_group, first_npc_group, last_npc_group, next, prev);
            PUT_FREE(kill_group, npc_group_free);
         }
         else
         {
            for (kill_member = kill_group->first_follower; kill_member;
                 kill_member = kill_member->next)
            {
               if ((CHAR_DATA *)kill_member->this_one == ch)
               {
                  UNLINK(kill_member, kill_group->first_follower, kill_group->last_follower, next,
                         prev);
                  PUT_FREE(kill_member, dl_list_free);
               }
            }
         }
      }
   }
   UNLINK(ch, first_char, last_char, next, prev);
   if (ch->desc)
      ch->desc->character = NULL;
   free_char(ch);
   return;
}

/* get_char_room, get_char_world, get_char_area, get_char,
 * get_obj_type, get_obj_room, get_obj_list, get_obj_carry,
 * get_obj_wear, get_obj_here, get_obj_world, create_money,
 * get_obj_number, get_obj_weight have been moved to lookup.c.
 */

/* room_is_dark, room_is_private, can_see, can_see_obj, can_drop_obj,
 * can_sac_obj, can_use, who_can_use have been moved to visibility.c.
 */

/* notify, auction, info, log_chan, item_has_apply have been moved to comm.c. */

/* This is for staff authorized skills. Enables staff to set which skills lower staff may use.
 * handy for abuse control --Flar
 */
bool authorized(CHAR_DATA *ch, char *skllnm)
{
   return TRUE;
}

/* For new spells, etc, eg Polymorph.  Transfer a PC to safe room, then
   'switches' them into the given mob.  The mob is placed into the
   room 'victim' was in. */

CHAR_DATA *switch_char(CHAR_DATA *victim, int mvnum, int poly_level)
{
   /*
    * Levels of polymorphism :
    * 0 : equivalent to switch
    * 1 : pc with pcdata.
    * 2 : pc with pcdata + objects
    * 3 : pc with pcdata + objs, levels and exp, pract, gold
    * 4 : as 3, but same stats(hp/mana/move)
    */

   CHAR_DATA *mob;
   ROOM_INDEX_DATA *location;
   OBJ_DATA *eq;
   int foo;

   location = victim->in_room;

   char_from_room(victim);
   char_to_room(victim, get_room_index(ROOM_VNUM_BODIES));

   mob = create_mobile(get_mob_index(mvnum));

   switch (poly_level)
   {

   case 4: /* Level 4 */
      mob->hit = victim->hit;
      mob->max_hit = victim->max_hit;
      mob->mana = victim->mana;
      mob->max_mana = victim->max_mana;
      mob->max_move = victim->max_move;
      mob->move = victim->move;

   case 3: /* Level 3 */
      mob->level = victim->level;
      mob->gold = victim->gold;
      mob->exp = victim->exp;
      for (foo = 0; foo < 4; foo++)
      {
         mob->mortal_class[foo] = victim->mortal_class[foo];
         mob->mortal_level[foo] = victim->mortal_level[foo];
      }
      for (foo = 0; foo < 2; foo++)
      {
         mob->remort_class[foo] = victim->remort_class[foo];
         mob->remort_level[foo] = victim->remort_level[foo];
      }
      mob->adept_class = victim->adept_class;
      mob->adept_level = victim->adept_level;
      mob->practice = victim->practice;

   case 2: /* Level 2 */
      while ((eq = victim->first_carry) != NULL)
      {
         obj_from_char(eq);
         obj_to_char(eq, mob);
      }

   case 1: /* Level 1 */
      mob->pcdata = victim->pcdata;
      REMOVE_BIT(mob->act, ACT_IS_NPC);

   case 0: /* Level 0 */
      mob->desc = victim->desc;
      mob->desc->character = mob;
      mob->desc->original = victim;
      mob->switched = TRUE;

      victim->desc = NULL;
      victim->switched = TRUE;
      break;
   default:
      bug("Invalid poly_level %d encountered.", poly_level);
      break;
   }

   mob->poly_level = poly_level;
   mob->old_body = victim;
   char_to_room(mob, location);
   return (mob);
}

CHAR_DATA *unswitch_char(CHAR_DATA *victim)
{
   /*
    * Check poly_level, and copy back relevant stats, etc.
    */

   CHAR_DATA *original;
   ROOM_INDEX_DATA *location;
   OBJ_DATA *eq;
   int foo;

   if (victim->switched == FALSE)
      return victim;

   location = victim->in_room;
   original = victim->old_body;

   char_from_room(original);
   char_to_room(original, location);

   original->switched = FALSE;
   original->desc = victim->desc;
   original->desc->character = original;
   original->desc->original = NULL;

   switch (victim->poly_level)
   {
   case 4:
      original->hit = victim->hit;
      original->max_hit = victim->max_hit;
      original->mana = victim->mana;
      original->max_mana = victim->max_mana;
      original->max_move = victim->max_move;
      original->move = victim->move;

   case 3:
      original->level = victim->level;
      original->exp = victim->exp;
      original->gold = victim->gold;
      for (foo = 0; foo < 4; foo++)
      {
         original->mortal_class[foo] = victim->mortal_class[foo];
         original->mortal_level[foo] = victim->mortal_level[foo];
      }
      for (foo = 0; foo < 2; foo++)
      {
         original->remort_class[foo] = victim->remort_class[foo];
         original->remort_level[foo] = victim->remort_level[foo];
      }
      original->adept_class = victim->adept_class;
      original->adept_level = victim->adept_level;

   case 2:
      while ((eq = victim->first_carry) != NULL)
      {
         obj_from_char(eq);
         obj_to_char(eq, original);
      }

   case 1:
      original->pcdata = victim->pcdata;

   case 0:
      SET_BIT(victim->act, ACT_IS_NPC);
   }

   victim->desc = NULL;
   extract_char(victim, TRUE);
   return original;
}

void set_stun(CHAR_DATA *victim, int stunTime)
{
   /*
    * Sets the victim's wait_state and position
    * -Damane-        4/26/96
    */
   if (victim->position != POS_SLEEPING)
      victim->position = POS_STUNNED;

   victim->stunTimer = stunTime;
   return;
}

bool is_shielded(CHAR_DATA *ch, sh_int shield_type)
{
   MAGIC_SHIELD *shield;

   for (shield = ch->first_shield; shield != NULL; shield = shield->next)
      if (shield->type == shield_type)
         return TRUE;

   return FALSE;
}

void remove_shield(CHAR_DATA *ch, MAGIC_SHIELD *shield)
{
   if (ch->first_shield == NULL)
   {
      sprintf(buf, "shield_remove: %s did not have a shield to remove.",
              IS_NPC(ch) ? ch->short_descr : ch->name);
      monitor_chan(buf, MONITOR_MOB);

      bug("Remove_shield: no shield.", 0);
      return;
   }

   UNLINK(shield, ch->first_shield, ch->last_shield, next, prev);
   PUT_FREE(shield, shield_free);

   return;
}
