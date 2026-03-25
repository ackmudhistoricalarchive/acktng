#include "globals.h"

bool is_legal_stance(CHAR_DATA *ch, int stance);
/*
   char *name;
   int ac_mod;
   int dr_mod;
   int hr_mod;
   int speed_mod;
   int heal_mod;
   int spell_mod;
   int specials;
   */
const struct stance_app_type stance_app[MAX_STANCE] = {
    {"Adventurer", 0, 0, 0, 0, 0, 0, 0, MORTAL, -1, -1, -1, -1},
    /* Mortal stances */
    {"Ember Protocol", 5, -3, -3, 0, 0, 2, STANCE_MULTI_CAST | STANCE_NO_HIT, MORTAL, CLASS_MAG, 60,
     -1, -1, CLASS_MAG},
    {"Long Hold", 5, -3, -3, 0, 0, 2, STANCE_MULTI_CAST | STANCE_NO_HIT, MORTAL, CLASS_PSI, 60, -1,
     -1, CLASS_PSI},
    {"Solar Office", 0, 0, 0, 0, 1, 0, 0, MORTAL, CLASS_CLE, 60, -1, -1, CLASS_CLE},
    {"Dunmar's Watch", 0, 1, 0, 0, 0, 0, 0, MORTAL, CLASS_WAR, 60, -1, -1, CLASS_WAR},
    {"Gap Intuition", -1, 0, 1, 0, 0, 0, 0, MORTAL, CLASS_CIP, 60, -1, -1, CLASS_CIP},
    {"Economy of Motion", 0, 0, 0, 1, 0, 0, 0, MORTAL, CLASS_PUG, 60, -1, -1, CLASS_PUG},
    /* Remort stances */
    {"Assassin", -3, 0, 3, 1, 0, 0, 0, REMORT, CLASS_ASS, 1, -1, -1, -1},
    {"Ninja", 0, 0, 0, 2, 0, 0, STANCE_NINJA, REMORT, CLASS_ASS, 30, -1, -1, -1},
    {"Warlock", -2, 0, 2, 1, 0, 2, 0, REMORT, CLASS_WLK, 1, -1, -1, -1},
    {"Priest", -3, 0, 0, 0, 3, 0, 0, REMORT, CLASS_PRI, 1, -1, -1, -1},
    {"Paladin", 0, 3, 3, 1, 2, 0, 0, REMORT, CLASS_PAL, 1, -1, -1, -1},
    {"Sorcerer", 2, -3, -3, 0, -2, 5, STANCE_DUAL_CAST | STANCE_NO_HIT, REMORT, CLASS_SOR, 1, -1,
     -1, -1},
    {"Wizard", 2, -3, -3, 0, -2, 5, STANCE_DUAL_CAST | STANCE_NO_HIT, REMORT, CLASS_WIZ, 1, -1, -1,
     -1},
    {"Knight", -3, 2, 2, 1, 0, 0, 0, REMORT, CLASS_KNI, 1, -1, -1, -1},
    {"Swordsman", -3, 2, 2, 1, 0, 0, 0, REMORT, CLASS_SWO, 1, -1, -1, -1},
    {"Necromancer", 2, -3, -3, 0, 0, 5, STANCE_DUAL_CAST | STANCE_NO_HIT, REMORT, CLASS_NEC, 1, -1,
     -1, -1},
    {"Egomancer", 2, -3, -3, 0, 0, 5, STANCE_DUAL_CAST | STANCE_NO_HIT, REMORT, CLASS_EGO, 1, -1,
     -1, -1},
    {"Deepest Form", 0, 3, 3, 2, 0, 0, 0, REMORT, CLASS_MON, 1, -1, -1, -1},
    {"Brawler", 0, 3, 3, 2, 0, 0, 0, REMORT, CLASS_BRA, 1, -1, -1, -1},
    /* Adept Stances */
    {"Magi", 2, -1, -1, 0, 0, 3,
     STANCE_EITHER_CLASS | STANCE_MULTI_CAST | STANCE_DUAL_CAST | STANCE_NO_HIT, ADEPT, CLASS_GMA,
     1, CLASS_KIN, 1, -1},
    {"Templar", -2, 3, 3, 3, 5, 0, 0, ADEPT, CLASS_TEM, 1, -1, -1, -1},
    {"Nightblade", -3, 5, 5, 3, 0, 3, STANCE_DUAL_BACKSTAB, ADEPT, CLASS_NIG, 1, -1, -1, -1},
    {"Crusader", -3, 3, 3, 3, 0, 0, 0, ADEPT, CLASS_CRU, 1, -1, -1, -1},
    {"Martial", -1, 5, 5, 5, 0, 0, 0, ADEPT, CLASS_MAR, 1, -1, -1, -1},
    /* Sentinel lineage stances */
    {"Silence-Court Vigil", -1, 0, 1, 0, 0, 0, 0, MORTAL, CLASS_SEN, 60, -1, -1, CLASS_SEN},
    {"Root Posture", 0, 1, 0, 0, 1, 0, 0, MORTAL, CLASS_DRU, 60, -1, -1, CLASS_DRU},
    {"Sentencing Form", -2, 2, 3, 1, 0, 0, 0, REMORT, CLASS_JUS, 1, -1, -1, -1},
    {"Record-Keeper's Guard", -3, 1, 1, 1, 2, 0, 0, REMORT, CLASS_ARB, 1, -1, -1, -1},
    {"Thorn Boundary", -2, 3, 2, 1, 0, 0, 0, REMORT, CLASS_THO, 1, -1, -1, -1},
    {"Wild Resonance", -1, 0, 1, 1, 3, 0, 0, REMORT, CLASS_WIL, 1, -1, -1, -1},
    {"Full Tribunal", -2, 3, 3, 3, 2, 0, 0, ADEPT, CLASS_INQ, 1, -1, -1, -1},
    {"Substrate Ascendance", -1, 3, 2, 2, 4, 0, 0, ADEPT, CLASS_HIE, 1, -1, -1, -1},
};

void do_stance(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_STRING_LENGTH];
   char cat_buf[MSL];
   char msg_buf[MSL];
   int i = 0;

   if (IS_NPC(ch))
   {
      send_to_char("Not for Npcs!\n\r", ch);
      return;
   }

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      sprintf(msg_buf, "\n\r%s\n\r", "Fighting Stances available to you:\n\r");

      for (int i = 0; i < MAX_STANCE; i++)
      {
         if (is_legal_stance(ch, i))
         {
            sprintf(cat_buf, "%s\n\r", stance_app[i].name);

            safe_strcat(MSL, msg_buf, cat_buf);
            sprintf(cat_buf, "%s", "");
         }
      }
      sprintf(cat_buf, "%s",
              "Type stance <stance name> to change your current fighting stance.\n\r You may place "
              "your current Stance in your prompt with a \%s\n\r");
      safe_strcat(MSL, msg_buf, cat_buf);
      send_to_char(msg_buf, ch);
      return;
   }

   for (i = 0; i < MAX_STANCE; i++)
   {
      if (!str_prefix(arg, stance_app[i].name))
         break;
   }

   if (i == MAX_STANCE)
   {
      do_stance(ch, "");
      act("$n poses in a strange fashion, looking rather silly.", ch, NULL, NULL, TO_ROOM);
      send_to_char(
          "You twist about wildly, but are unable to figure out just the right Stance.\n\r", ch);
      return;
   }

   if (is_legal_stance(ch, i))
   {
      ch->stance = i;
      sprintf(msg_buf, "$n assumes the Stance of the %s!", stance_app[i].name);
      act(msg_buf, ch, NULL, NULL, TO_ROOM);
      sprintf(msg_buf, "You assume the Stance of the %s!\n\r", stance_app[i].name);
      send_to_char(msg_buf, ch);
   }

   WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
   return;
}

bool is_legal_stance(CHAR_DATA *ch, int stance)
{
   bool legal_stance = TRUE;

   if (stance_app[stance].tier == ADEPT)
   {
      {
         if (stance_app[stance].class_index > -1 &&
             char_class_level(ch, stance_app[stance].class_index) < stance_app[stance].class_level)
            legal_stance = FALSE;

         if (stance_app[stance].class_index2 > -1 &&
             char_class_level(ch, stance_app[stance].class_index2) <
                 stance_app[stance].class_level2)
            legal_stance = FALSE;

         if (IS_SET(stance_app[stance].specials, STANCE_EITHER_CLASS) &&
             stance_app[stance].class_index2 > -1 &&
             char_class_level(ch, stance_app[stance].class_index) >= stance_app[stance].class_level)
            legal_stance = TRUE;

         if (IS_SET(stance_app[stance].specials, STANCE_EITHER_CLASS) &&
             stance_app[stance].class_index2 > -1 &&
             char_class_level(ch, stance_app[stance].class_index2) >=
                 stance_app[stance].class_level2)
            legal_stance = TRUE;
      }
   }
   else if (stance_app[stance].tier == REMORT)
   {
      {
         if (stance_app[stance].class_index > -1 &&
             char_class_level(ch, stance_app[stance].class_index) < stance_app[stance].class_level)
            legal_stance = FALSE;

         if (stance_app[stance].class_index2 > -1 &&
             char_class_level(ch, stance_app[stance].class_index2) <
                 stance_app[stance].class_level2)
            legal_stance = FALSE;

         if (IS_SET(stance_app[stance].specials, STANCE_EITHER_CLASS) &&
             stance_app[stance].class_index2 > -1 &&
             char_class_level(ch, stance_app[stance].class_index) >= stance_app[stance].class_level)
            legal_stance = TRUE;

         if (IS_SET(stance_app[stance].specials, STANCE_EITHER_CLASS) &&
             stance_app[stance].class_index2 > -1 &&
             char_class_level(ch, stance_app[stance].class_index2) >=
                 stance_app[stance].class_level2)
            legal_stance = TRUE;
      }
   }
   else if (stance_app[stance].tier == MORTAL)
   {
      {
         if (stance_app[stance].class_index > -1 &&
             char_class_level(ch, stance_app[stance].class_index) < stance_app[stance].class_level)
            legal_stance = FALSE;

         if (stance_app[stance].class_index2 > -1 &&
             char_class_level(ch, stance_app[stance].class_index2) <
                 stance_app[stance].class_level2)
            legal_stance = FALSE;

         if (IS_SET(stance_app[stance].specials, STANCE_EITHER_CLASS) &&
             stance_app[stance].class_index2 > -1 &&
             char_class_level(ch, stance_app[stance].class_index) >= stance_app[stance].class_level)
            legal_stance = TRUE;

         if (IS_SET(stance_app[stance].specials, STANCE_EITHER_CLASS) &&
             stance_app[stance].class_index2 > -1 &&
             char_class_level(ch, stance_app[stance].class_index2) >=
                 stance_app[stance].class_level2)
            legal_stance = TRUE;
      }

      if (stance_app[stance].class_override > -1)
      {
         if (stance_app[stance].class_override == ch->class)
            legal_stance = TRUE;
      }
   }

   return legal_stance;
}
