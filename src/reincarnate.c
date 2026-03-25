#include "globals.h"

int stat_calculated_value(int val, int statcap_bonus);
void remove_all(CHAR_DATA *ch);
OBJ_DATA *find_clan_eq(CHAR_DATA *ch);
void claneq_adjust(OBJ_DATA *obj, CHAR_DATA *ch);

void do_reincarnate(CHAR_DATA *ch, char *argument)
{
   char arg[MSL];
   char buf[MSL];

   argument = one_argument(argument, arg);

   if (!str_prefix(arg, "information"))
   {
      send_to_char("================REINCARNATION==============\n\r", ch);

      send_to_char("Mortal reincarnations: ", ch);
      for (int i = 0; i < MAX_CLASS; i++)
      {
         sprintf(buf, "%s: %d ", gclass_table[i].who_name, ch->pcdata->reincarnations[i]);
         send_to_char(buf, ch);
      }
      send_to_char("\n\r", ch);

      send_to_char("Remort reincarnations: ", ch);
      for (int i = CLASS_SOR; i < CLASS_SOR + MAX_REMORT; i++)
      {
         sprintf(buf, "%s: %d ", gclass_table[i].who_name, ch->pcdata->reincarnations[i]);
         send_to_char(buf, ch);
      }
      send_to_char("\n\r", ch);

      send_to_char("Adept reincarnations: ", ch);
      for (int i = CLASS_GMA; i < CLASS_GMA + MAX_CLASS; i++)
      {
         sprintf(buf, "%s: %d ", gclass_table[i].who_name, ch->pcdata->reincarnations[i]);
         send_to_char(buf, ch);
      }
      send_to_char("\n\r", ch);
      sprintf(buf, "Reincarnate HP: %d Mana: %d Move: %d Statcap: %d, Damcap: %d\n\r",
              ch->pcdata->reincarnation_data[REINCARNATION_HP] / 10,
              ch->pcdata->reincarnation_data[REINCARNATION_MANA] / 10,
              ch->pcdata->reincarnation_data[REINCARNATION_MOVE] / 10,
              ch->pcdata->reincarnation_data[REINCARNATION_STATCAP],
              ch->pcdata->reincarnation_data[REINCARNATION_DAMCAP]);
      send_to_char(buf, ch);
   }
   else if (!str_prefix(arg, "buy"))
   {
      /* #define REINCARNATION_DAMCAP 0
      #define REINCARNATION_RMOD 1
      #define REINCARNATION_STATS 2
      #define REINCARNATION_HP 3
      #define REINCARNATION_MANA 4
      #define REINCARNATION_MOVE 5
      #define REINCARNATION_POINTS 6*/

      argument = one_argument(argument, arg);

      if (!str_prefix(arg, "help"))
      {
         send_to_char("Buy:\n\r", ch);
         sprintf(buf, "Damcap %d: %d\n\r", ch->pcdata->reincarnation_data[REINCARNATION_DAMCAP] + 1,
                 ch->pcdata->reincarnation_data[REINCARNATION_DAMCAP] * 2 + 1);
         send_to_char(buf, ch);
         sprintf(buf, "Statcap %d: %d\n\r",
                 ch->pcdata->reincarnation_data[REINCARNATION_STATCAP] + 1,
                 ch->pcdata->reincarnation_data[REINCARNATION_STATCAP] *
                         ch->pcdata->reincarnation_data[REINCARNATION_STATCAP] +
                     1);
         send_to_char(buf, ch);

         return;
      }
      if (!str_prefix(arg, "damcap"))
      {
         int cost = ch->pcdata->reincarnation_data[REINCARNATION_DAMCAP] * 2 + 1;
         if (ch->pcdata->reincarnation_data[REINCARNATION_POINTS] < cost)
         {
            send_to_char("You do not have enough reincarnation points to increase your damcap.\n\r",
                         ch);
            return;
         }
         ch->pcdata->reincarnation_data[REINCARNATION_POINTS] -= cost;
         ch->pcdata->reincarnation_data[REINCARNATION_DAMCAP]++;
         send_to_char("Reincarnation damcap permanently increased!\n\r", ch);
         return;
      }
      if (!str_prefix(arg, "statcap"))
      {
         int cost = ch->pcdata->reincarnation_data[REINCARNATION_STATCAP] *
                        ch->pcdata->reincarnation_data[REINCARNATION_STATCAP] +
                    1;
         if (ch->pcdata->reincarnation_data[REINCARNATION_POINTS] < cost)
         {
            send_to_char(
                "You do not have enough reincarnation points to increase your statcap.\n\r", ch);
            return;
         }
         ch->pcdata->reincarnation_data[REINCARNATION_POINTS] -= cost;
         ch->pcdata->reincarnation_data[REINCARNATION_STATCAP]++;
         send_to_char("Reincarnation statcap permanently increased!\n\r", ch);
         return;
      }
   }
   else if (!str_prefix(arg, "set"))
   {
      if (get_adept_level(ch) < 20)
      {
         send_to_char("Only realm lords can reincarnate set!\n\r", ch);
         return;
      }
      argument = one_argument(argument, arg);
      if (!str_prefix(arg, "show"))
      {
         send_to_char("Current reincarnation info:\n\r", ch);
         if (ch->pcdata->reincarnate_race >= 0)
            sprintf(buf, "Race: %s\n\r", race_table[ch->pcdata->reincarnate_race].race_name);
         else
            sprintf(buf, "Race: (not set)\n\r");
         send_to_char(buf, ch);
         if (IS_SET(ch->pcdata->reincarnate_confirm, REINCARNATE_CLASS) &&
             ch->pcdata->reincarnate_class >= 0)
            sprintf(buf, "Class: %s\n\r", gclass_table[ch->pcdata->reincarnate_class].class_name);
         else
            sprintf(buf, "Class: (not set)\n\r");
         send_to_char(buf, ch);
         if (IS_SET(ch->pcdata->reincarnate_confirm, REINCARNATE_CONFIRM))
            send_to_char("Reincarnation CONFIRMED, reincarnate set done to reincarnate.\n\r", ch);
      }
      else if (!str_prefix(arg, "race"))
      {
         argument = one_argument(argument, arg);
         ch->pcdata->reincarnate_race = -1;
         REMOVE_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_RACE);
         REMOVE_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_CONFIRM);

         for (int i = 0; i < MAX_RACE; i++)
         {
            if (!str_cmp(arg, race_table[i].race_name) || !str_cmp(arg, race_table[i].race_title))
            {
               if (!race_table[i].player_allowed)
               {
                  send_to_char("Players may not select this race.\n\r", ch);
                  return;
               }
               ch->pcdata->reincarnate_race = i;
               sprintf(buf, "%s selected as reincarnation race.\n\r", race_table[i].race_name);
               send_to_char(buf, ch);
            }
         }

         if (ch->pcdata->reincarnate_race == -1)
         {
            send_to_char("Invalid race selection, see rlist for options.\n\r", ch);
            return;
         }

         SET_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_RACE);
      }
      else if (!str_prefix(arg, "class"))
      {
         argument = one_argument(argument, arg);
         ch->pcdata->reincarnate_class = -1;
         REMOVE_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_CLASS);
         REMOVE_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_CONFIRM);

         for (int i = 0; i < MAX_TOTAL_CLASS; i++)
         {
            if (!IS_MORTAL_CLASS(i))
               continue;
            if (!str_cmp(arg, gclass_table[i].who_name) ||
                !str_cmp(arg, gclass_table[i].class_name))
            {
               ch->pcdata->reincarnate_class = i;
               sprintf(buf, "%s selected as reincarnation class.\n\r", gclass_table[i].class_name);
               send_to_char(buf, ch);
               break;
            }
         }

         if (ch->pcdata->reincarnate_class == -1)
         {
            send_to_char(
                "Invalid class selection. Use who_name or class_name, e.g. MAG or Magi.\n\r", ch);
            return;
         }

         SET_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_CLASS);
      }
      else if (!str_prefix(arg, "confirm"))
      {
         if (ch->pcdata->reincarnate_race == -1)
         {
            send_to_char("Race not set. Use reincarnate set race <race>.\n\r", ch);
            REMOVE_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_RACE);
            REMOVE_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_CONFIRM);
            return;
         }
         if (ch->pcdata->reincarnate_class == -1)
         {
            send_to_char("Class not set. Use reincarnate set class <class>.\n\r", ch);
            REMOVE_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_CLASS);
            REMOVE_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_CONFIRM);
            return;
         }
         send_to_char("Reincarnation confirm set!\n\r", ch);
         SET_BIT(ch->pcdata->reincarnate_confirm, REINCARNATE_CONFIRM);
      }
      else if (!str_prefix(arg, "done"))
      {
         if (!IS_SET(ch->pcdata->reincarnate_confirm, REINCARNATE_RACE) ||
             !IS_SET(ch->pcdata->reincarnate_confirm, REINCARNATE_CLASS) ||
             !IS_SET(ch->pcdata->reincarnate_confirm, REINCARNATE_CONFIRM))
         {
            send_to_char("You haven't confirmed your reincarnation yet!\n\r", ch);
            return;
         }

         remove_all(ch);

         ch->quest_points = ch->exp / 100000 + 1;
         ch->exp = 0;

         ch->pcdata->reincarnation_data[REINCARNATION_HP] += ch->pcdata->hp_from_gain;
         ch->pcdata->hp_from_gain = 0;
         ch->pcdata->reincarnation_data[REINCARNATION_MANA] += ch->pcdata->mana_from_gain;
         ch->pcdata->mana_from_gain = 0;
         ch->pcdata->reincarnation_data[REINCARNATION_MOVE] += ch->pcdata->move_from_gain;
         ch->pcdata->reincarnation_data[REINCARNATION_POINTS]++;
         ch->pcdata->move_from_gain = 0;
         ch->pcdata->reincarnations[ch->class]++;
         ch->race = ch->pcdata->reincarnate_race;
         ch->class = ch->pcdata->reincarnate_class;
         ch->level = 1;
         send_to_char("Done\n\r", ch);
         if (ch->adept_class >= 0 && ch->adept_level > 0)
            ch->pcdata->reincarnations[ch->adept_class]++;
         for (int i = 0; i < 2; i++)
         {
            if (ch->remort_class[i] >= 0 && ch->remort_level[i] > 0)
               ch->pcdata->reincarnations[ch->remort_class[i]]++;
         }
         for (int i = 0; i < 4; i++)
            ch->mortal_level[i] = 0;
         for (int i = 0; i < 2; i++)
            ch->remort_level[i] = 0;
         ch->adept_level = 0;
         ch->level = 1;
         ch->mortal_class[0] = ch->class;
         ch->mortal_level[0] = 1;
         reset_gain_stats(ch);
         advance_level(ch, ch->class, TRUE);

         /* Reset clan eq to level 1 after reincarnation */
         if (ch->pcdata->clan > 0)
         {
            OBJ_DATA *ceq = find_clan_eq(ch);
            if (ceq != NULL)
            {
               ceq->level = 1;
               claneq_adjust(ceq, ch);
               send_to_char(
                   "@@eYour clan colors fade and reform, diminished by your rebirth.@@N\n\r", ch);
            }
         }

         do_save(ch, "");
      }
      else
      {
         snprintf(buf, sizeof(buf), "Invalid input %.128s\n\r", arg);
         send_to_char(buf, ch);
         send_to_char("Valid set args: show, race, class, confirm, done", ch);
      }
   }
   else
      send_to_char("Valid args: information, buy, set\n\r", ch);
}

int get_reincarnate_hp_level(CHAR_DATA *ch)
{
   return stat_calculated_value(ch->pcdata->reincarnation_data[REINCARNATION_HP],
                                ch->pcdata->reincarnation_data[REINCARNATION_STATCAP]);
}

int get_reincarnate_mana_level(CHAR_DATA *ch)
{
   return stat_calculated_value(ch->pcdata->reincarnation_data[REINCARNATION_MANA],
                                ch->pcdata->reincarnation_data[REINCARNATION_STATCAP]);
}

int get_reincarnate_move_level(CHAR_DATA *ch)
{
   return stat_calculated_value(ch->pcdata->reincarnation_data[REINCARNATION_MOVE],
                                ch->pcdata->reincarnation_data[REINCARNATION_STATCAP]);
}

int stat_calculated_value(int val, int statcap_bonus)
{
   int steps = 5000;
   int steps_1 = 3;
   int steps_2 = 6;
   int steps_3 = 10;
   int value = val / 10;
   int calculated_value = 0;

   steps += statcap_bonus * 1000;

   if (value > steps)
   {
      calculated_value += steps;
      value -= steps;
   }
   else
   {
      calculated_value += value;
      value = 0;
   }

   if (value > steps * steps_1)
   {
      calculated_value += steps;
      value -= steps * steps_1;
   }
   else
   {
      calculated_value += value / steps_1;
      value = 0;
   }

   if (value > steps * steps_2)
   {
      calculated_value += steps;
      value -= steps * steps_2;
   }
   else
   {
      calculated_value += value / steps_2;
      value = 0;
   }

   if (value > 0)
      calculated_value += value / steps_3;

   return calculated_value / 100;
}
