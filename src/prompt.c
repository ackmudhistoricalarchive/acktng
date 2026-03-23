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
 *    _/_/_/_/      _/          _/  _/             Version #: 4.3.1        *
 *   _/      _/      _/_/_/     _/    _/     _/                            *
 *                                                                         *
 *                        http://ackmud.nuc.net/                           *
 *                        zenithar@ackmud.nuc.net                          *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/*
 * This file contains prompt display and formatting logic:
 *   bust_a_prompt() — the main player-prompt renderer
 *   do_prompt()     — the player command to set a custom prompt
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "prompt.h"

/*
 * Forward declarations for functions defined in other modules.
 */
bool is_valid_finisher(CHAR_DATA *ch);
int get_combo_count(CHAR_DATA *ch);
int get_max_combo(CHAR_DATA *ch);
int get_chi(CHAR_DATA *ch);

/*
 * Returns true if the default prompt should show current HP (i.e. not at max).
 */
bool prompt_should_show_hp(CHAR_DATA *ch)
{
   return ch != NULL && ch->hit < get_max_hp(ch);
}

/*
 * Returns true if the default prompt should show current mana (i.e. not at max).
 */
bool prompt_should_show_mana(CHAR_DATA *ch)
{
   return ch != NULL && ch->mana < get_max_mana(ch);
}

/*
 * Returns true if the default prompt should show current movement (i.e. not at max).
 */
bool prompt_should_show_move(CHAR_DATA *ch)
{
   return ch != NULL && ch->move < get_max_move(ch);
}

/*
 * Returns the maximum value for the given prompt code ('H', 'M', or 'V').
 */
long prompt_max_value_for_code(CHAR_DATA *ch, char code)
{
   if (ch == NULL)
      return 0;

   switch (code)
   {
   case 'H':
      return get_max_hp(ch);
   case 'M':
      return get_max_mana(ch);
   case 'V':
      return get_max_move(ch);
   default:
      return 0;
   }
}

/*
 * Formats the builder/editor info-prompt line into dest.
 */
void format_builder_prompt(char *dest, size_t dest_size, const char *mode, const char *details)
{
   if (dest == NULL || dest_size == 0)
      return;

   snprintf(dest, dest_size, "< %s %s >", mode != NULL ? mode : "", details != NULL ? details : "");
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt(DESCRIPTOR_DATA *d)
{
   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   const char *str;
   const char *i = " ";
   char *point;
   CHAR_DATA *ch;
   CHAR_DATA *victim;
   int check = -1;

   ch = d->character;

   /* v2 WebSocket clients have no use for a text prompt */
   if (d->websocket_active)
      return;

   /*
    * if editing a note, show a REAL simple prompt ;P
    */
   if (ch->position == POS_WRITING && !IS_NPC(ch))
   {
      write_to_buffer(d, ">", 0);
      return;
   }

   /*
    * If editing, show an 'info-prompt' -S-
    */
   if (ch->position == POS_BUILDING && !IS_NPC(ch))
   {
      ROOM_INDEX_DATA *room;
      OBJ_INDEX_DATA *obj;
      MOB_INDEX_DATA *mob;
      char msg[MAX_STRING_LENGTH];  /* Mode */
      char msg2[MAX_STRING_LENGTH]; /* what */
      char msg3[MAX_STRING_LENGTH]; /* all */

      if (ch->act_build == ACT_BUILD_NOWT)
      {
         sprintf(msg, "Mode:  None");
         sprintf(msg2, "Use Redit, Medit or Oedit to select mode.");
      }

      if (ch->act_build == ACT_BUILD_REDIT)
      {
         sprintf(msg, "Mode: Redit");
         if (ch->build_vnum == -1)
            sprintf(msg2, "No vnum is set.");
         else
         {
            room = get_room_index(ch->build_vnum);
            if (room != NULL)
               sprintf(msg2, "[%5d]: %s", ch->build_vnum, room->name);
         }
      }

      if (ch->act_build == ACT_BUILD_OEDIT)
      {
         sprintf(msg, "Mode: Oedit");
         if (ch->build_vnum == -1)
            sprintf(msg2, "No vnum set.");
         else
         {
            obj = get_obj_index(ch->build_vnum);
            if (obj != NULL)
               sprintf(msg2, "[%5d]: %s", ch->build_vnum, obj->short_descr);
         }
      }

      if (ch->act_build == ACT_BUILD_MEDIT)
      {
         sprintf(msg, "Mode: Medit");
         if (ch->build_vnum == -1)
            sprintf(msg2, "No vnum set.");
         else
         {
            mob = get_mob_index(ch->build_vnum);
            if (mob != NULL)
               sprintf(msg2, "[%5d]: %s", ch->build_vnum, mob->short_descr);
         }
      }
      format_builder_prompt(msg3, sizeof(msg3), msg, msg2);
      write_to_buffer(d, msg3, 0);
      return;
   }

   /*
    * set default prompt to ">" -Uni
    */
   if (!IS_SET(ch->config, CONFIG_PROMPT))
   {
      sprintf(buf2, "\r");
      if (prompt_should_show_hp(ch))
      {
         check = 1;
         sprintf(buf, "<%ldhp", ch->hit);
         strcat(buf2, buf);
      }
      if (prompt_should_show_mana(ch))
      {
         if (check == 1)
            sprintf(buf, " %ldmn", ch->mana);
         else
            sprintf(buf, "<%ldmn", ch->mana);
         check = 1;
         strcat(buf2, buf);
      }
      if (prompt_should_show_move(ch))
      {
         if (check == 1)
            sprintf(buf, " %ldmv", ch->move);
         else
            sprintf(buf, "<%ldmv", ch->move);
         strcat(buf2, buf);
         check = 1;
      }
      if (check == 1)
         send_to_char(buf2, ch);

      send_to_char("> ", ch);
      return;
   }

   if (ch->prompt == NULL || ch->prompt[0] == '\0')
   {
      send_to_char("@@N> ", ch);
      return;
   }

   point = buf;
   str = d->original != NULL ? d->original->prompt : d->character->prompt;
   sprintf(buf2, "%s", "@@N");
   i = buf2;
   while ((*point = *i) != '\0')
      ++point, ++i;
   while (*str != '\0')
   {
      if (*str != '%')
      {
         *point++ = *str++;
         continue;
      }
      ++str;
      switch (*str)
      {
      default:
         i = "> ";
         break;
      case 'w':
      {
         sh_int cl_index = -1;
         int cost;
         bool adept = FALSE;
         ++str;
         switch (*str)
         {
         default:
            sprintf(buf2, "%s", "BadClass");
            i = buf2;
            break;
         case 'm':
         case 'M':
         {
            ++str;
            if ((*str == 'a') || (*str == 'A'))
            {
               cl_index = 0;
               break;
            }
            else if ((*str == 'o') || (*str == 'O'))
            {
               cl_index = 4;
               break;
            }
            else
            {
               sprintf(buf2, "%s", "BadClass");
               i = buf2;
               break;
            }
         }

         case 'c':
            ++str;
            cl_index = 1;
            break;
         case 'C':
            ++str;
            cl_index = 1;
            break;
         case 't':
         case 'T':
            ++str;
            cl_index = 2;
            break;
         case 'w':
         case 'W':
            ++str;
            cl_index = 3;
            break;
         case 'p':
         case 'P':
            ++str;
            cl_index = 4;
            break;
         case 's':
         case 'S':
            ++str;
            cl_index = 0;
            break;
         case 'a':
         case 'A':
         {
            ++str;
            if ((*str == 's') || (*str == 'S'))
            {
               cl_index = 1;
               break;
            }
            else if ((*str == 'd') || (*str == 'D'))
            {
               cl_index = 0;
               adept = TRUE;
               break;
            }
            else
            {
               sprintf(buf2, "%s", "BadClass");
               i = buf2;
               break;
            }
         }

         case 'k':
         case 'K':
            ++str;
            cl_index = 2;
            break;
         case 'n':
         case 'N':
            ++str;
            cl_index = 3;
            break;
         }
         if (cl_index < 0)
            break;
         if (adept == TRUE)
            cost = exp_to_level_adept(ch);
         else
            cost = exp_to_level(ch, cl_index);
         sprintf(buf2, "%ld", UMAX(0, cost - ch->exp));
         i = buf2;
         break;
      }
      case 'W':
      {
         sh_int cl_index = -1;
         int cost;
         bool adept = FALSE;
         ++str;
         switch (*str)
         {
         default:
            sprintf(buf2, "%s", "BadClass");
            i = buf2;
            break;
         case 'm':
         case 'M':
         {
            ++str;
            if ((*str == 'a') || (*str == 'A'))
            {
               cl_index = 0;
               break;
            }
            else if ((*str == 'o') || (*str == 'O'))
            {
               cl_index = 4;
               break;
            }
            else
            {
               sprintf(buf2, "%s", "BadClass");
               i = buf2;
               break;
            }
         }

         case 'c':
            ++str;
            cl_index = 1;
            break;
         case 'C':
            ++str;
            cl_index = 1;
            break;
         case 't':
         case 'T':
            ++str;
            cl_index = 2;
            break;
         case 'w':
         case 'W':
            ++str;
            cl_index = 3;
            break;
         case 'p':
         case 'P':
            ++str;
            cl_index = 4;
            break;
         case 's':
         case 'S':
            ++str;
            cl_index = 0;
            break;
         case 'a':
         case 'A':
         {
            ++str;
            if ((*str == 's') || (*str == 'S'))
            {
               cl_index = 1;
               break;
            }
            else if ((*str == 'd') || (*str == 'D'))
            {
               cl_index = 0;
               adept = TRUE;
               break;
            }
            else
            {
               sprintf(buf2, "%s", "BadClass");
               i = buf2;
               break;
            }
         }

         case 'k':
         case 'K':
            ++str;
            cl_index = 2;
            break;
         case 'n':
         case 'N':
            ++str;
            cl_index = 3;
            break;
         }
         if (cl_index < 0)
            break;
         if (adept == TRUE)
            cost = exp_to_level_adept(ch);
         else
            cost = exp_to_level(ch, cl_index);
         sprintf(buf2, "%d", UMAX(0, cost));
         i = buf2;
         break;
      }
      case 'h':
         sprintf(buf2, "%ld", ch->hit);
         i = buf2;
         break;
      case '!':
         sprintf(buf2, "%d", get_hitroll(ch));
         i = buf2;
         break;
      case '+':
         sprintf(buf2, "%d", get_damroll(ch));
         i = buf2;
         break;
      case '*':
         sprintf(buf2, "%d", get_ac(ch));
         i = buf2;
         break;
      case 'H':
         sprintf(buf2, "%ld", prompt_max_value_for_code(ch, *str));
         i = buf2;
         break;
      case 'm':
         sprintf(buf2, "%ld", ch->mana);
         i = buf2;
         break;
      case 'M':
         sprintf(buf2, "%ld", prompt_max_value_for_code(ch, *str));
         i = buf2;
         break;
      case 'v':
         sprintf(buf2, "%ld", ch->move);
         i = buf2;
         break;
      case 'V':
         sprintf(buf2, "%ld", prompt_max_value_for_code(ch, *str));
         i = buf2;
         break;
      case 'x':
         sprintf(buf2, "%ld", ch->exp);
         i = buf2;
         break;
      case 'g':
         sprintf(buf2, "%d", ch->gold);
         i = buf2;
         break;
      case 'a':
         if (ch->level < 5)
            sprintf(buf2, "%d", ch->alignment);
         else
            sprintf(buf2, "%s", IS_GOOD(ch) ? "good" : IS_EVIL(ch) ? "evil" : "good");
         i = buf2;
         break;
      case 'r':
         if (ch->in_room != NULL)
            sprintf(buf2, "%s", ch->in_room->name);
         else
            sprintf(buf2, " ");
         i = buf2;
         break;
      case 'R':
         if (IS_STAFF(ch) && ch->in_room != NULL)
            sprintf(buf2, "%d", ch->in_room->vnum);
         else
            sprintf(buf2, " ");
         i = buf2;
         break;
      case 's':
         if (!IS_NPC(ch))
            sprintf(buf2, "%s", stance_app[ch->stance].name);
         else
            sprintf(buf2, " ");
         i = buf2;
         break;

      case 'z':
         if (IS_STAFF(ch) && ch->in_room != NULL)
            sprintf(buf2, "%s", ch->in_room->area->name);
         else
            sprintf(buf2, " ");
         i = buf2;
         break;
      case '%':
         sprintf(buf2, "%%");
         i = buf2;
         break;
      case 'c':
         if (!IS_NPC(ch))
            sprintf(buf2, "\n\r");
         else
            sprintf(buf2, " ");
         i = buf2;
         break;
      case 'C':
         if (is_valid_finisher(ch))
            sprintf(buf2, " @@rR@@N");
         else
            sprintf(buf2, " %d/%d", get_combo_count(ch), get_max_combo(ch));
         i = buf2;
         break;
      case 'i':
         if (IS_NPC(ch))
            break;
         if (IS_STAFF(ch))
            sprintf(buf2, "INVIS: %d", IS_SET(ch->act, PLR_WIZINVIS) ? ch->invis : 0);
         else
         {
            if ((IS_AFFECTED(ch, AFF_INVISIBLE)) || (IS_AFFECTED(ch, AFF_HIDE)) ||
                (item_has_apply(ch, ITEM_APPLY_INV)) || (item_has_apply(ch, ITEM_APPLY_HIDE)))
            {
               sprintf(buf2, "%s", "INVIS");
            }
            else
            {
               sprintf(buf2, "%s", "VIS");
            }
         }
         i = buf2;
         break;

      case 'I':
         sprintf(buf2, " %d", get_chi(ch));
         i = buf2;
         break;
      case 'A':
         if (!IS_NPC(ch))
            sprintf(buf2, "%d", ch->arcane_power);
         else
            sprintf(buf2, " ");
         i = buf2;
         break;
      case 'P':
         if (!IS_NPC(ch))
            sprintf(buf2, "%d", ch->holy_power);
         else
            sprintf(buf2, " ");
         i = buf2;
         break;
      case 'Q':
         if (!IS_NPC(ch))
            sprintf(buf2, "%d", ch->mental_power);
         else
            sprintf(buf2, " ");
         i = buf2;
         break;
      case 't':
         if (!IS_NPC(ch))
            sprintf(buf2, "%d %s", (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
                    time_info.hour >= 12 ? "pm" : "am");
         else
            sprintf(buf2, " ");
         i = buf2;
         break;
      case 'F':
         sprintf(buf2, " ");
         i = buf2;
         break;

      case 'p':
         if (!IS_NPC(ch))
         {
            if (weather_info.moon_loc == MOON_DOWN)
               sprintf(buf2, "%s", "DOWN");
            else if (weather_info.moon_loc == MOON_PEAK)
               sprintf(buf2, "PEAK:%s", get_moon_phase_name());
            else
               sprintf(buf2, "LOW:%s", get_moon_phase_name());
         }
         else
            sprintf(buf2, " ");
         i = buf2;
         break;
      }
      ++str;
      while ((*point = *i) != '\0')
         ++point, ++i;
   }

   /*
    * possible HUGE string bug here
    */

   /*
    * Also have part of battle prompt showing tank status -S-
    */
   if (is_fighting(ch)) /* only way to do it, sorry */
   {
      /*
       * We'll just show a percentage for people in the group
       */
      CHAR_DATA *tank;
      char buf[200];

      buf[0] = '\0';

      /*
       * See who the target (ch->fighting) is fighting (->fighting)
       */
      tank = ch->fighting->fighting;
      if (tank != NULL)
      {
         int percent;
         char wound[100];

         if (get_max_hp(tank) > 0)
            percent = tank->hit * 100 / get_max_hp(tank);
         else
            percent = -1;

         if (percent >= 95)
            sprintf(wound, "@@rExcellent@@N");
         else if (percent >= 85)
            sprintf(wound, "@@GScratches@@N");
         else if (percent >= 70)
            sprintf(wound, "@@ySmall wounds@@N");
         else if (percent >= 55)
            sprintf(wound, "@@bLarger wounds@@N");
         else if (percent >= 45)
            sprintf(wound, "@@mBleeding freely@@N");
         else if (percent >= 30)
            sprintf(wound, "@@RLeaking guts@@N");
         else if (percent >= 15)
            sprintf(wound, "@@eA MESS!@@N");
         else
            sprintf(wound, "@@2@@W@@fALMOST DEAD!!!@@N");
         if (!IS_NPC(ch) && IS_SET(ch->pcdata->pflags, PFLAG_BLIND_PLAYER))
            sprintf(buf, "@@W%s %s @@N ", tank == ch ? "YOU" : "Tank", wound);
         else
            sprintf(buf, "@@a[@@W%s@@a:%s@@a]@@N ", tank == ch ? "YOU" : "Tank", wound);
         write_to_buffer(d, buf, 0);
      }
   }

   /*
    * battle prompt
    */
   if ((victim = ch->fighting) != NULL)

   {
      int percent;
      /*
       * int percent2;
       */
      char wound[100];
      char buf[MAX_STRING_LENGTH];

      if (get_max_hp(victim) > 0)
         percent = victim->hit * 100 / get_max_hp(victim);
      else
         percent = -1;

      if (percent >= 95)
         sprintf(wound, "@@rExcellent@@N");
      else if (percent >= 85)
         sprintf(wound, "@@GScratches@@N");
      else if (percent >= 70)
         sprintf(wound, "@@ySmall wounds@@N");
      else if (percent >= 55)
         sprintf(wound, "@@bLarger wounds@@N");
      else if (percent >= 45)
         sprintf(wound, "@@mBleeding freely@@N");
      else if (percent >= 30)
         sprintf(wound, "@@RLeaking guts@@N");
      else if (percent >= 15)
         sprintf(wound, "@@eA MESS!@@N");
      else
         sprintf(wound, "@@2@@WALMOST DEAD!!!@@N");
      if (!IS_NPC(ch) && IS_SET(ch->pcdata->pflags, PFLAG_BLIND_PLAYER))
         sprintf(buf, "@@WVictim %s @@N\n\r", wound);
      else
         sprintf(buf, "@@a[@@WVictim@@a:%s@@a]@@N\n\r", wound);

      /*
       * buf[0] = UPPER(buf[0]);
       */
      write_to_buffer(d, buf, 0);
      /*
       * No newline, to keep both prompts on one line...
       */
   }

   /*
    * MAG Mod should be point-buf + 1, not point-buf (for end null char.)
    */

   write_to_buffer(d, buf, point - buf);
   return;
}

/* Do_prompt from Morgenes from Aldara Mud */
void do_prompt(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   buf[0] = '\0';

   if (argument[0] == '\0')
   {
      (IS_SET(ch->config, CONFIG_PROMPT) ? do_config(ch, "-prompt") : do_config(ch, "+prompt"));
      return;
   }

   if (!strcmp(argument, "all"))
      strcpy(buf, "<%hhp %mm %vmv> ");
   else
   {
      if (strlen(argument) > 200)
         argument[200] = '\0';
      strcpy(buf, argument);
      smash_tilde(buf);
   }

   free_string(ch->prompt);
   ch->prompt = str_dup(buf);
   send_to_char("Ok.\n\r", ch);
   return;
}
