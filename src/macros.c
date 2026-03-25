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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "tables.h"
#include <math.h>

void reset_gain_stats(CHAR_DATA *ch)
{
   int curr;
   ch->pcdata->mana_from_gain = 100;
   ch->pcdata->hp_from_gain = 25;
   ch->pcdata->move_from_gain = 100;

   for (int i = 0; i < 4; i++)
   {
      if (ch->mortal_class[i] < 0)
         continue;
      curr = ch->mortal_level[i];
      ch->mortal_level[i] = 0;
      for (int j = 0; j < curr; j++)
         advance_level(ch, ch->mortal_class[i], FALSE);
   }

   for (int i = 0; i < 2; i++)
   {
      if (ch->remort_class[i] < 0)
         continue;
      curr = ch->remort_level[i];
      ch->remort_level[i] = 0;
      for (int j = 0; j < curr; j++)
         advance_level_remort(ch, ch->remort_class[i], FALSE);
   }

   if (ch->adept_class >= 0)
   {
      curr = ch->adept_level;
      ch->adept_level = 0;
      for (int j = 0; j < curr; j++)
         advance_level_adept(ch, ch->adept_class, FALSE);
   }

   ch->max_hit = ch->pcdata->hp_from_gain;
   ch->hit = ch->max_hit;
   ch->max_mana = ch->pcdata->mana_from_gain;
   ch->mana = ch->max_mana;
   ch->max_move = ch->pcdata->move_from_gain;
   ch->move = ch->max_move;
}

sh_int get_remort_level(CHAR_DATA *ch)
{
   sh_int index;
   sh_int max_remort_level = 0;

   if (!is_remort(ch))
      return 0;

   for (index = 0; index < 2; index++)
      if (ch->remort_class[index] >= 0 && ch->remort_level[index] > max_remort_level)
         max_remort_level = ch->remort_level[index];
   return max_remort_level;
}

sh_int get_psuedo_level(CHAR_DATA *ch)
{

   sh_int psuedo_level = 0;
   sh_int index, total_remort_level = 0;

   if (!is_remort(ch) || IS_NPC(ch))
      return ch->level;
   else
   {

      for (index = 0; index < 2; index++)
         if (ch->remort_class[index] >= 0 && ch->remort_level[index] > 0)
            total_remort_level += ch->remort_level[index];

      psuedo_level = (ch->level + (total_remort_level / 4));
   }
   return psuedo_level;
}

bool ok_to_use(CHAR_DATA *ch, int value)
{
   if (value == NO_USE && get_trust(ch) < 85)
   {
      send_to_char("Only Creators may use this value.\n\r", ch);
      return FALSE;
   }
   return TRUE;
}

bool check_level_use(CHAR_DATA *ch, int level)
{
   char buf[MAX_STRING_LENGTH];
   char out[MAX_STRING_LENGTH];

   if (get_trust(ch) >= level)
      return TRUE;

   sprintf(out, "This option limited to ");

   switch (level)
   {
   case 85:
      strcat(out, "Creators only.\n\r");
      break;
   case 84:
      strcat(out, "Supremes or higher.\n\r");
      break;
   case 83:
      strcat(out, "Dieties or higher.\n\r");
      break;
   case 82:
      strcat(out, "Staff or higher.\n\r");
      break;
   case 81:
      strcat(out, "Adepts or higher.\n\r");
      break;
   default:
      sprintf(buf, "level %d players and higher.\n\r", level);
      strcat(out, buf);
   }
   send_to_char(out, ch);
   return FALSE;
}

long exp_to_level_adept(CHAR_DATA *ch)
{
   long exp;
   int max = 0;

   max = ch->adept_level;

   if (!max)
      return 6969;

   exp = (30000 + (max * 5000));

   exp += exp * max / 2;

   exp -= exp / 4;

   for (int i = 0; i < get_total_reincarnations(ch); i++)
      exp += exp / 10;

   return exp * 3;
}

long exp_to_level_remort(CHAR_DATA *ch, int index)
{
   long cost;

   if (IS_NPC(ch))
      return 69;

   if (char_class_level(ch, index) <= 0)
      return 0;

   cost = get_cost_to_level_remort(ch, index);

   cost -= cost / 4;

   for (int i = 0; i < get_total_reincarnations(ch); i++)
      cost += cost / 10;

   return (cost);
}

long exp_to_level(CHAR_DATA *ch, int index)
{
   int diff = 0;
   long cost;

   if (IS_NPC(ch))
      return 69;

   cost = get_cost_to_level(ch, index);

   for (int i = 0; i < 4; i++)
   {
      if (ch->mortal_level[i] > diff)
         diff = ch->mortal_level[i];
   }

   diff = (diff - char_class_level(ch, index));

   if (diff > 10)
      cost += cost * diff / 10;

   if (diff > 20)
      cost += cost * diff / 10;

   if (diff > 30)
      cost += cost * diff / 10;

   cost -= cost / 4;

   for (int i = 0; i < get_total_reincarnations(ch); i++)
      cost += cost / 10;

   return (cost);
}

int exp_to_level_vamp(int level)
{
   int exp = 0;

   switch (level)
   {
   case 0:
   case 1:
   case 3:
   case 4:
   case 5:
      exp = (250 + level * 5);
      break;
   case 6:
   case 7:
   case 8:
   case 9:
      exp = (300 + level * 6);
      break;
   case 10:
   case 11:
   case 12:
      exp = (350 + level * 5);
      break;
   case 13:
   case 14:
   case 15:
   case 16:
      exp = (450 + level * 6);
      break;
   case 17:
   case 18:
   case 19:
   case 20:
      exp = (500 + level * 5);
      break;
   }
   return (exp);
}

int exp_for_mobile(int level, CHAR_DATA *mob)
{
   int value, base_value;

   if (level < 0)
   {
      bug("exp_for_mobile: negative level %d.", level);
      level = 1;
   }
   else if (level >= MAX_MOB_LEVEL)
   {
      sprintf(log_buf, "exp_for_mobile: %s level %d exceeds table max %d.", mob->name, level,
              MAX_MOB_LEVEL - 1);
      bug(log_buf, 0);
      level = MAX_MOB_LEVEL - 1;
   }

   base_value = exp_table[level].mob_base;
   value = base_value;

   /* now we have the base for the mobs level..let's add multipliers based on the skills it has
      thse multpliers should add up to no more than 150% of the base, for a total of 2.5 times base
      max exp for the mob--that's with EVERY skill in the book :)   */

   if (IS_SET(mob->skills, MOB_SECOND))
      value += .05 * base_value;
   if (IS_SET(mob->skills, MOB_THIRD))
      value += .200 * base_value;
   if (IS_SET(mob->skills, MOB_FOURTH))
      value += .1 * base_value;
   if (IS_SET(mob->skills, MOB_FIFTH))
      value += .200 * base_value;
   if (IS_SET(mob->skills, MOB_SIXTH))
      value += .1 * base_value;
   if (IS_SET(mob->skills, MOB_PUNCH))
      value += .050 * base_value;
   if (IS_SET(mob->skills, MOB_HEADBUTT))
      value += .100 * base_value;
   if (IS_SET(mob->skills, MOB_KNEE))
      value += .050 * base_value;
   if (IS_SET(mob->skills, MOB_DISARM))
      value += .050 * base_value;
   if (IS_SET(mob->skills, MOB_TRIP))
      value += .050 * base_value;
   if (IS_SET(mob->skills, MOB_NODISARM))
      value += .150 * base_value;
   if (IS_SET(mob->skills, MOB_DODGE))
      value += .150 * base_value;
   if (IS_SET(mob->skills, MOB_COUNTER))
      value += .200 * base_value;
   if (IS_SET(mob->skills, MOB_PARRY))
      value += .05 * base_value;
   if (IS_SET(mob->skills, MOB_MARTIAL))
      value += .200 * base_value;
   if (IS_SET(mob->skills, MOB_ENHANCED))
      value += .300 * base_value;
   if (IS_SET(mob->skills, MOB_NOTRIP))
      value += .100 * base_value;
   if (IS_SET(mob->skills, MOB_DUALWIELD))
      value += .050 * base_value;
   if (IS_SET(mob->skills, MOB_DIRT))
      value += .150 * base_value;
   if (IS_SET(mob->skills, MOB_CHARGE))
      value += .050 * base_value;

   if (IS_AFFECTED(mob, AFF_SANCTUARY))
      value += .400 * base_value;
   if (IS_AFFECTED(mob, AFF_INVISIBLE))
      value += .050 * base_value;
   if (IS_AFFECTED(mob, AFF_DETECT_INVIS))
      value += .100 * base_value;
   if (IS_AFFECTED(mob, AFF_ANTI_MAGIC))
      value += .350 * base_value;

   if (IS_AFFECTED(mob, AFF_CLOAK_FLAMING))
      value += .600 * base_value;
   if (IS_AFFECTED(mob, AFF_CLOAK_ABSORPTION))
      value += .150 * base_value;
   if (IS_AFFECTED(mob, AFF_CLOAK_REFLECTION))
      value += .350 * base_value;

   if ((IS_AFFECTED(mob, AFF_CLOAK_REFLECTION)) && (IS_AFFECTED(mob, AFF_CLOAK_ABSORPTION)) &&
       (IS_AFFECTED(mob, AFF_CLOAK_FLAMING)))
      value += .200 * base_value;

   if (IS_SET(mob->def, DEF_CURE_LIGHT))
      value += .100 * base_value;
   if (IS_SET(mob->def, DEF_CURE_SERIOUS))
      value += .200 * base_value;
   if (IS_SET(mob->def, DEF_CURE_CRITIC))
      value += .350 * base_value;
   if (IS_SET(mob->def, DEF_HEAL))
      value += .400 * base_value;
   if (IS_SET(mob->def, DEF_FIRESHIELD))
      value += .300 * base_value;
   if (IS_SET(mob->def, DEF_SHOCKSHIELD))
      value += .200 * base_value;
   if (IS_SET(mob->def, DEF_ICESHIELD))
      value += .100 * base_value;

   if (IS_SET(mob->act, ACT_SOLO))
      value += .500 * base_value;

   if (IS_SET(mob->act, ACT_BOSS))
      value += 1.500 * base_value;

   if (IS_SET(mob->act, ACT_WIMPY))
      value += .500 * base_value;

   return (value);
}

/*
 * Functions to return details regarding a PC and skill_table
 * Uses 2 #defined values in merc.h to determine what to return
 * These also make adapting to remort classes a lot easier - all
 * the code goes here instead of in all the skills and do_cast()
 *
 * skill_table[] now includes flags to determine if for mortals
 * or remortals, so use that here, and use correct array.
 */

int skill_table_lookup(CHAR_DATA *ch, int sn, int return_type)
{
   int best_class = -1;
   int best_level = -1;
   int return_value;
   int cnt;

   if (IS_NPC(ch))
   {
      best_class = ch->class;
      best_level = ch->level;
   }
   else
   {
      /*
       * Check normal classes first
       */

      switch (skill_get_tier(sn))
      {
      case MORTAL:
         for (cnt = 0; cnt < 4; cnt++)
         {
            int cl = ch->mortal_class[cnt];
            if (cl < 0)
               continue;
            if (ch->mortal_level[cnt] >= skill_table[sn].skill_level[cl] &&
                ch->mortal_level[cnt] > best_level)
            {
               best_level = ch->mortal_level[cnt];
               best_class = cl;
            }
         }
         break;
      case REMORT:
         for (cnt = 0; cnt < 2; cnt++)
         {
            int cl = ch->remort_class[cnt];
            if (cl < 0)
               continue;
            if (ch->remort_level[cnt] >= skill_table[sn].skill_level[cl] &&
                ch->remort_level[cnt] > best_level)
            {
               best_level = ch->remort_level[cnt];
               best_class = cl;
            }
         }
         break;
      }
   }
   switch (return_type)
   {
   case RETURN_BEST_CLASS:
      return_value = best_class;
      break;
   case RETURN_BEST_LEVEL:
      return_value = best_level;
      break;
   default:
      bug("skill_table_lookup: invalid return_type:%d", return_type);
      return_value = -1;
      break;
   }
   return (return_value);
}

bool is_remort(CHAR_DATA *ch)
{
   if (IS_NPC(ch))
      return FALSE;

   for (int i = 0; i < 2; i++)
   {
      if (ch->remort_class[i] >= 0 && ch->remort_level[i] > 0)
         return TRUE;
   }

   return FALSE;
}

bool is_adept(CHAR_DATA *ch)
{
   if (IS_NPC(ch))
      return FALSE;

   return ch->adept_class >= 0 && ch->adept_level > 0;
}

int get_adept_level(CHAR_DATA *ch)
{
   return ch->adept_level;
}

int get_item_value(OBJ_DATA *obj)
{
   AFFECT_DATA *this_aff;
   int ac_mod = 0;
   int dr_mod = 0;
   int hp_mod = 0;
   int hr_mod = 0;
   int mana_mod = 0;
   /*
    * int     move_mod = 0;
    */
   int save_mod = 0;
   int cost = 0;
   sh_int wear_loc = WEAR_NONE;
   char buf[MSL];

   /* fix this up to use apply_ac by getting the wear loc */
   for (wear_loc = 1; wear_loc < MAX_WEAR; wear_loc++)
   {
      if (CAN_WEAR(obj, (1 << wear_loc)))
         break;
   }
   if (wear_loc == MAX_WEAR)
   {
      /*
       * sprintf( buf, "Object has no wear loc" );
       * monitor_chan( buf, MONITOR_OBJ );
       */
      ac_mod = 0;
   }
   else
      ac_mod = apply_ac(obj, 1 << wear_loc);

   for (this_aff = obj->first_apply; this_aff != NULL; this_aff = this_aff->next)
   {
      switch (this_aff->location)
      {
      default:
         bug("Get_item_value: unknown location %d.", this_aff->location);
         sprintf(buf, "Get_item_value called for unknown location %d.", this_aff->location);
         monitor_chan(buf, MONITOR_OBJ);
         break;
      case APPLY_NONE:
         break;
      case APPLY_STR:
         break;
      case APPLY_DEX:
         break;
      case APPLY_INT:
         break;
      case APPLY_WIS:
         break;
      case APPLY_CON:
         break;
      case APPLY_SEX:
         break;
      case APPLY_CLASS:
         break;
      case APPLY_LEVEL:
         break;
      case APPLY_AGE:
         break;
      case APPLY_HEIGHT:
         break;
      case APPLY_WEIGHT:
         break;
      case APPLY_MANA:
         mana_mod += this_aff->modifier;
         break;
      case APPLY_HIT:
         hp_mod += this_aff->modifier;
         break;
      case APPLY_MOVE:
         break;
      case APPLY_GOLD:
         break;
      case APPLY_EXP:
         break;
      case APPLY_AC:
         ac_mod += this_aff->modifier;
         break;
      case APPLY_HITROLL:
         hr_mod += this_aff->modifier;
         break;
      case APPLY_DAMROLL:
         dr_mod += this_aff->modifier;
         break;
      case APPLY_SAVING_PARA:
      case APPLY_SAVING_ROD:
      case APPLY_SAVING_PETRI:
      case APPLY_SAVING_BREATH:
      case APPLY_SAVING_SPELL:
         save_mod += this_aff->modifier;
         break;
      case APPLY_SPELLPOWER:
      case APPLY_SPELL_CRIT:
      case APPLY_SPELL_CRIT_MULT:
      case APPLY_CRIT:
      case APPLY_CRIT_MULT:
      case APPLY_DAMCAP:
      case APPLY_SPEED:
         break;
      }
   }

   cost = obj->level * 1 + ac_mod * -8 + dr_mod * 5 + hr_mod * 5 + save_mod * 2 + hp_mod * 4 +
          mana_mod * 3;

   if (IS_SET(obj->item_apply, ITEM_APPLY_ENHANCED))
      cost = cost * 1.3;

   if (IS_SET(obj->item_apply, ITEM_APPLY_SANC))
      cost = cost * 1.3;

   sprintf(buf, "Cost computed for item %d.", cost);
   /*  monitor_chan( buf, MONITOR_OBJ );  */

   if (obj->item_type == ITEM_ENCHANTMENT)
      cost = abs(obj->value[1] * 100);
   return UMAX(10, cost);

   return -1;
}
