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
#include <unistd.h>
#if defined(__CYGWIN__)
#include <crypt.h>
#endif
#include "globals.h"
#include "socket.h"
#include "tables.h"
#include "cursor.h"
#include <math.h>
#ifndef DEC_MAGIC_H
#include "magic.h"
#endif
#ifndef CONFIG_H
#include "config.h"
#endif
#include "db/db_help.h"

extern bool deathmatch;

char *const where_name[] = {
    "-*light*-             ", /* light  */
    "-*floating above*-    ", "-*enveloped by*-      ", "-*worn on horns*-     ",
    "-*worn on head*-      ", "-*worn on face*-      ", "-*worn on beak*-      ",
    "-*worn on ear*-       ", "-*worn on ear*-       ", "-*worn on neck*-      ",
    "-*worn on neck*-      ", /* 10 */
    "-*worn on wings*-     ", "-*worn on shoulders*- ", "-*worn on arms*-      ",
    "-*worn around wrist*- ", "-*worn around wrist*- ", /* 15 */
    "-*worn on hands*-     ", "-*worn on finger*-    ", "-*worn on finger*-    ",
    "-*worn on claws*-     ", "-*held in left hand*- ", /* 20 */
    "-*held in right hand*-", "-*two-handed*-        ", "-*buckler*-           ",
    "-*worn as a cape*-    ", "-*worn about waist*-  ", /* 25 */
    "-*worn on body*-      ", "-*worn on tail*-      ", "-*worn on legs*-      ",
    "-*worn on feet*-      ", "-*worn on hooves*-    ", /* 30 */
    "-*clan colors*-       ", "-*invasion emblem*-   "};

/*
 * Local functions.
 */
char *format_obj_to_char args((OBJ_DATA * obj, CHAR_DATA *ch, bool fShort));
void show_list_to_char args((OBJ_DATA * list, CHAR_DATA *ch, bool fShort, bool fShowNothing));
void show_char_to_char_0 args((CHAR_DATA * victim, CHAR_DATA *ch));
void show_char_to_char_1 args((CHAR_DATA * victim, CHAR_DATA *ch));
void show_char_to_char args((CHAR_DATA * list, CHAR_DATA *ch));
bool check_blind args((CHAR_DATA * ch));
int find_race_index_by_name(const char *name);
void parse_shelp_query(char *argument, char *search_term, size_t search_term_size,
                       char *full_argument, size_t full_argument_size);

int find_race_index_by_name(const char *name)
{
   int i;

   if (name == NULL || name[0] == '\0')
      return -1;

   for (i = 0; i < MAX_RACE; i++)
   {
      if (!str_cmp(race_table[i].race_name, name) || !str_cmp(race_table[i].race_title, name))
         return i;
   }

   for (i = 0; i < MAX_RACE; i++)
   {
      if (!str_prefix(name, race_table[i].race_name) || !str_prefix(name, race_table[i].race_title))
         return i;
   }

   return -1;
}
int score_should_show_invasion_rewards(CHAR_DATA *ch)
{
   if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL)
      return 0;

   if (ch->pcdata->invasion_rewards[0] > 0 || ch->pcdata->invasion_rewards[1] > 0 ||
       ch->pcdata->invasion_rewards[2] > 0)
      return 1;

   return 0;
}

void parse_shelp_query(char *argument, char *search_term, size_t search_term_size,
                       char *full_argument, size_t full_argument_size)
{
   char *trimmed_argument;
   size_t argument_length;

   one_argument(argument, search_term);

   trimmed_argument = argument;
   while (isspace(*trimmed_argument))
      trimmed_argument++;

   snprintf(full_argument, full_argument_size, "%s", trimmed_argument);

   argument_length = strlen(full_argument);
   if (argument_length >= 2 &&
       ((full_argument[0] == '"' && full_argument[argument_length - 1] == '"') ||
        (full_argument[0] == '\'' && full_argument[argument_length - 1] == '\'')))
   {
      memmove(full_argument, full_argument + 1, argument_length - 2);
      full_argument[argument_length - 2] = '\0';
   }

   if (search_term[0] == '\0')
   {
      snprintf(search_term, search_term_size, "shelp");
      snprintf(full_argument, full_argument_size, "shelp");
   }
}

char *format_obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch, bool fShort)
{
   static char buf[MAX_STRING_LENGTH];

   sprintf(buf, "%s", color_string(ch, "objects"));
   if (IS_OBJ_STAT(obj, ITEM_INVIS))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Invis) ");

   if ((IS_AFFECTED(ch, AFF_DETECT_EVIL) || item_has_apply(ch, ITEM_APPLY_DET_EVIL)) &&
       IS_OBJ_STAT(obj, ITEM_EVIL))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Red Aura) ");

   if ((IS_AFFECTED(ch, AFF_DETECT_MAGIC) || item_has_apply(ch, ITEM_APPLY_DET_MAG)) &&
       IS_OBJ_STAT(obj, ITEM_MAGIC))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Magical) ");

   if (fShort)
   {
      if (obj->short_descr != NULL)
         safe_strcat(MAX_STRING_LENGTH, buf, obj->short_descr);
   }
   else
   {
      if (obj->description != NULL)
         safe_strcat(MAX_STRING_LENGTH, buf, obj->description);
   }
   safe_strcat(MAX_STRING_LENGTH, buf, color_string(ch, "normal"));
   return buf;
}

void do_rhelp(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH], sendBuf[MAX_STRING_LENGTH];
   int i;

   one_argument(argument, arg);

   if (argument[0] == '\0')
   {
      send_to_char("Please specify a race (rlist for a list of races).\r\n", ch);
      return;
   }

   i = find_race_index_by_name(arg);
   if (i < 0)
   {
      send_to_char("No such race found.\r\n", ch);
      return;
   }

   sprintf(buf, "RHELP for race %s\r\n", race_table[i].race_title);
   send_to_char(buf, ch);

   {
      char kw_buf[256];
      char text_buf[16384];
      int lev_out;
      if (db_help_lookup(race_table[i].race_title, get_trust(ch), kw_buf, sizeof(kw_buf), text_buf,
                         sizeof(text_buf), &lev_out))
         send_to_char(text_buf[0] == '.' ? text_buf + 1 : text_buf, ch);
   }

   strcpy(sendBuf, "");

   sprintf(buf, "Class Order: %s\r\n", class_order(i));
   strcat(sendBuf, buf);

   sprintf(buf, "Base Attributes: Str %d Int %d Wis %d Dex %d Con %d\r\n", race_table[i].race_str,
           race_table[i].race_int, race_table[i].race_wis, race_table[i].race_dex,
           race_table[i].race_con);
   strcat(sendBuf, buf);

   sprintf(buf, "Racial Skills: %s\r\n", race_table[i].skill);
   strcat(sendBuf, buf);

   strcat(sendBuf, "Race Specials:");

   if (race_table[i].race_flags == RACE_MOD_NONE)
      strcat(sendBuf, " None");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_FAST_HEAL))
      strcat(sendBuf, " Fast Heal");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_SLOW_HEAL))
      strcat(sendBuf, " Slow Heal");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_STRONG_MAGIC))
      strcat(sendBuf, " Strong Magic");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_WEAK_MAGIC))
      strcat(sendBuf, " Weak Magic");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_NO_MAGIC))
      strcat(sendBuf, " No Magic");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_IMMUNE_POISON))
      strcat(sendBuf, " Immune Poison");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_RESIST_MAGIC))
      strcat(sendBuf, " Resist Spells");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_WOODLAND))
      strcat(sendBuf, " Woodland");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_DARKNESS))
      strcat(sendBuf, " Darkness");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_HUGE))
      strcat(sendBuf, " Huge");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_LARGE))
      strcat(sendBuf, " Large");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_TINY))
      strcat(sendBuf, " Tiny");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_SMALL))
      strcat(sendBuf, " Small");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_TAIL))
      strcat(sendBuf, " Tail");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_TOUGH_SKIN))
      strcat(sendBuf, " Tough Skin");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_STONE_SKIN))
      strcat(sendBuf, " Stone Skin");
   if (IS_SET(race_table[i].race_flags, RACE_MOD_IRON_SKIN))
      strcat(sendBuf, " Iron Skin");

   strcat(sendBuf, "\r\n");

   strcat(sendBuf, "Strong Realms:");

   if (race_table[i].strong_realms == ELEMENT_NONE)
      strcat(sendBuf, " None");
   if (IS_SET(race_table[i].strong_realms, ELEMENT_FIRE))
      strcat(sendBuf, " Fire");
   if (IS_SET(race_table[i].strong_realms, ELEMENT_AIR))
      strcat(sendBuf, " Air");
   if (IS_SET(race_table[i].strong_realms, ELEMENT_WATER))
      strcat(sendBuf, " Water");
   if (IS_SET(race_table[i].strong_realms, ELEMENT_EARTH))
      strcat(sendBuf, " Earth");
   if (IS_SET(race_table[i].strong_realms, ELEMENT_POISON))
      strcat(sendBuf, " Poison");
   if (IS_SET(race_table[i].strong_realms, ELEMENT_MENTAL))
      strcat(sendBuf, " Mental");
   if (IS_SET(race_table[i].strong_realms, ELEMENT_HOLY))
      strcat(sendBuf, " Holy");
   if (IS_SET(race_table[i].strong_realms, ELEMENT_PHYSICAL))
      strcat(sendBuf, " Physical");

   strcat(sendBuf, "\r\n");

   strcat(sendBuf, "Weak Realms:");

   if (race_table[i].weak_realms == ELEMENT_NONE)
      strcat(sendBuf, " None");
   if (IS_SET(race_table[i].weak_realms, ELEMENT_FIRE))
      strcat(sendBuf, " Fire");
   if (IS_SET(race_table[i].weak_realms, ELEMENT_AIR))
      strcat(sendBuf, " Air");
   if (IS_SET(race_table[i].weak_realms, ELEMENT_WATER))
      strcat(sendBuf, " Water");
   if (IS_SET(race_table[i].weak_realms, ELEMENT_EARTH))
      strcat(sendBuf, " Earth");
   if (IS_SET(race_table[i].weak_realms, ELEMENT_POISON))
      strcat(sendBuf, " Poison");
   if (IS_SET(race_table[i].weak_realms, ELEMENT_MENTAL))
      strcat(sendBuf, " Mental");
   if (IS_SET(race_table[i].weak_realms, ELEMENT_HOLY))
      strcat(sendBuf, " Holy");
   if (IS_SET(race_table[i].weak_realms, ELEMENT_PHYSICAL))
      strcat(sendBuf, " Physical");

   strcat(sendBuf, "\r\n");

   strcat(sendBuf, "Resist Realms:");

   if (race_table[i].resist_realms == ELEMENT_NONE)
      strcat(sendBuf, " None");
   if (IS_SET(race_table[i].resist_realms, ELEMENT_FIRE))
      strcat(sendBuf, " Fire");
   if (IS_SET(race_table[i].resist_realms, ELEMENT_AIR))
      strcat(sendBuf, " Air");
   if (IS_SET(race_table[i].resist_realms, ELEMENT_WATER))
      strcat(sendBuf, " Water");
   if (IS_SET(race_table[i].resist_realms, ELEMENT_EARTH))
      strcat(sendBuf, " Earth");
   if (IS_SET(race_table[i].resist_realms, ELEMENT_POISON))
      strcat(sendBuf, " Poison");
   if (IS_SET(race_table[i].resist_realms, ELEMENT_MENTAL))
      strcat(sendBuf, " Mental");
   if (IS_SET(race_table[i].resist_realms, ELEMENT_HOLY))
      strcat(sendBuf, " Holy");
   if (IS_SET(race_table[i].resist_realms, ELEMENT_PHYSICAL))
      strcat(sendBuf, " Physical");

   strcat(sendBuf, "\r\n");

   strcat(sendBuf, "Suscept Realms:");

   if (race_table[i].suscept_realms == ELEMENT_NONE)
      strcat(sendBuf, " None");
   if (IS_SET(race_table[i].suscept_realms, ELEMENT_FIRE))
      strcat(sendBuf, " Fire");
   if (IS_SET(race_table[i].suscept_realms, ELEMENT_AIR))
      strcat(sendBuf, " Air");
   if (IS_SET(race_table[i].suscept_realms, ELEMENT_WATER))
      strcat(sendBuf, " Water");
   if (IS_SET(race_table[i].suscept_realms, ELEMENT_EARTH))
      strcat(sendBuf, " Earth");
   if (IS_SET(race_table[i].suscept_realms, ELEMENT_POISON))
      strcat(sendBuf, " Poison");
   if (IS_SET(race_table[i].suscept_realms, ELEMENT_MENTAL))
      strcat(sendBuf, " Mental");
   if (IS_SET(race_table[i].suscept_realms, ELEMENT_HOLY))
      strcat(sendBuf, " Holy");
   if (IS_SET(race_table[i].suscept_realms, ELEMENT_PHYSICAL))
      strcat(sendBuf, " Physical");

   strcat(sendBuf, "\r\n");

   strcat(sendBuf, "Nonstandard wear slots: ");

   for (int j = 0; j < MAX_WEAR; j++)
   {
      if (race_table[i].wear_locs[j] == TRUE && race_table[0].wear_locs[j] == FALSE)
      {
         if (j == 3)
            strcat(sendBuf, "horns ");
         if (j == 6)
            strcat(sendBuf, "beak ");
         if (j == 11)
            strcat(sendBuf, "wings ");
         if (j == 19)
            strcat(sendBuf, "claws ");
         if (j == 27)
            strcat(sendBuf, "tail ");
         if (j == 30)
            strcat(sendBuf, "hooves ");
      }
   }

   send_to_char(sendBuf, ch);
}

void do_stancehelp(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   bool found = FALSE;
   int i;

   one_argument(argument, arg);

   if (argument[0] == '\0')
   {
      send_to_char("A list of valid stances:\r\n", ch);
      for (i = 0; i < MAX_STANCE; i++)
      {
         sprintf(buf, "Stance %s\n\r", stance_app[i].name);
         send_to_char(buf, ch);
      }
      return;
   }

   for (i = 0; i < MAX_STANCE; i++)
   {
      if (!str_cmp(stance_app[i].name, arg))
      {
         found = TRUE;
         break;
      }
   }

   if (!found)
   {
      for (i = 0; i < MAX_STANCE; i++)
      {
         if (!str_prefix(arg, stance_app[i].name))
         {
            found = TRUE;
            break;
         }
      }
   }

   if (!found)
   {
      send_to_char("No such stance found.\r\n", ch);
      return;
   }

   sprintf(buf, "STANCE HELP for stance %s\r\n", stance_app[i].name);
   send_to_char(buf, ch);
   int tier = stance_app[i].tier;
   if (tier == MORTAL)
   {
      sprintf(buf, "Stance %s is a mortal tier stance\n\r", stance_app[i].name);
      send_to_char(buf, ch);
      if (stance_app[i].class_override != -1)
      {
         sprintf(buf,
                 "There is a class override on %s for the first/primary class of %s. It gets the "
                 "stance at any level if it's your primary class.\n\r",
                 stance_app[i].name, gclass_table[stance_app[i].class_override].class_name);
         send_to_char(buf, ch);
      }
      if (stance_app[i].class_level > 0)
      {
         sprintf(buf, "There is a class requirement on %s of %s at %d\n\r", stance_app[i].name,
                 gclass_table[stance_app[i].class_index].class_name, stance_app[i].class_level);
         send_to_char(buf, ch);
      }
      if (stance_app[i].class_level2 > 0)
      {
         sprintf(buf, "There is an additional class requirement on %s of %s at %d\n\r",
                 stance_app[i].name, gclass_table[stance_app[i].class_index2].class_name,
                 stance_app[i].class_level2);
         send_to_char(buf, ch);
      }
   }
   else if (tier == REMORT)
   {
      sprintf(buf, "Stance %s is a remort tier stance\n\r", stance_app[i].name);
      send_to_char(buf, ch);
      if (stance_app[i].class_level > 0)
      {
         sprintf(buf, "There is a class requirement on %s of %s at %d\n\r", stance_app[i].name,
                 gclass_table[stance_app[i].class_index].class_name, stance_app[i].class_level);
         send_to_char(buf, ch);
      }
      if (stance_app[i].class_level2 > 0)
      {
         sprintf(buf, "There is an additional class requirement on %s of %s at %d\n\r",
                 stance_app[i].name, gclass_table[stance_app[i].class_index2].class_name,
                 stance_app[i].class_level2);
         send_to_char(buf, ch);
      }
   }
   else if (tier == ADEPT)
   {
      sprintf(buf, "Stance %s is an adept tier stance\n\r", stance_app[i].name);
      send_to_char(buf, ch);
      if (stance_app[i].class_level > 0)
      {
         sprintf(buf, "There is a class requirement on %s of %s at %d\n\r", stance_app[i].name,
                 gclass_table[stance_app[i].class_index].class_name, stance_app[i].class_level);
         send_to_char(buf, ch);
      }
      if (stance_app[i].class_level2 > 0)
      {
         sprintf(buf, "There is an additional class requirement on %s of %s at %d\n\r",
                 stance_app[i].name, gclass_table[stance_app[i].class_index2].class_name,
                 stance_app[i].class_level2);
         send_to_char(buf, ch);
      }
   }

   int specials = stance_app[i].specials;
   if (specials != 0)
   {
      sprintf(buf, "%s has the additional special properties of ", stance_app[i].name);
      send_to_char(buf, ch);
      if (IS_SET(specials, STANCE_DUAL_CAST))
         send_to_char("dual-casting ", ch);
      if (IS_SET(specials, STANCE_MULTI_CAST))
         send_to_char("multi-casting ", ch);
      if (IS_SET(specials, STANCE_NINJA))
         send_to_char("being undetectable ", ch);
      if (IS_SET(specials, STANCE_NO_HIT))
         send_to_char("no autoattacks ", ch);
      if (IS_SET(specials, STANCE_DUAL_BACKSTAB))
         send_to_char("double backstabs ", ch);
      send_to_char("\n\r", ch);
   }

   sprintf(buf,
           "AC mod: %d%%  HR mod: %d%%  DR mod: %d%%  SPEED mod: %d  HEAL mod: %d%%  SPELL mod: "
           "%d%%\n\r",
           stance_app[i].ac_mod * 10, stance_app[i].hr_mod * 5, stance_app[i].dr_mod * 5,
           stance_app[i].speed_mod, stance_app[i].heal_mod * 10, stance_app[i].spell_mod * 10);
   send_to_char(buf, ch);
}

/*    char *text;
   long int value;
   int cost;
   char *info;*/

/*     {"none", RACE_MOD_NONE, 0, ""},
    {"miracle heal", RACE_MOD_MIRACLE_HEAL, 1, "Increases regeneration by 3x"},
    {"extraordinary heal", RACE_MOD_EXTRAORDINARY_HEAL, 1, "Increase regeneration by 2.5x"},
    {"fast heal", RACE_MOD_FAST_HEAL, 1, "Increases regeneration by 2x"},
    {"slow heal", RACE_MOD_SLOW_HEAL, -1, "Halves regeneration"},
    {"strong magic", RACE_MOD_STRONG_MAGIC, 1, "Increases all magical damage by 10%"},
    {"magic adept", RACE_MOD_MAGIC_ADEPT, 3, "Increases all magical damage by 20%"},
    {"magic genius", RACE_MOD_MAGIC_GENIUS, 5, "Increases all magical damage by 30%"},
    {"weak magic", RACE_MOD_WEAK_MAGIC, -1, "Reduces all magical damage by 25%"},
    {"no magic", RACE_MOD_NO_MAGIC, -1, "Makes you extremely bad at magic"},
    {"immune poison", RACE_MOD_IMMUNE_POISON, -1, "Grants poison immunity"},
    {"resist magic", RACE_MOD_RESIST_MAGIC, 1, "Reduces magical damage by 10%"},
    {"thwart magic", RACE_MOD_THWART_MAGIC, 2, "Reduces magical damage by 20%"},
    {"magic foil", RACE_MOD_MAGIC_FOIL, 3, "Reduces magical damage by 30%"},
    {"woodland", RACE_MOD_WOODLAND, 1, "Adds woodland"},
    {"darkness", RACE_MOD_DARKNESS, 1, "Adds darkness"},
    {"huge", RACE_MOD_HUGE, 1, "Adds huge"},
    {"large", RACE_MOD_LARGE, 1, "Adds large"},
    {"small", RACE_MOD_SMALL, 1, "Adds small"},
    {"tiny", RACE_MOD_TINY, 1, "Adds tiny"},
    {"tail", RACE_MOD_TAIL, 3, "Adds tail equip racial and tail extra attack."},
    {"tough skin", RACE_MOD_TOUGH_SKIN, 1, "Reduces physical damage by 10%"},
    {"stone skin", RACE_MOD_STONE_SKIN, 2, "Reduces physical damage by 20%"},
    {"iron skin", RACE_MOD_IRON_SKIN, 3, "Reduces physical damage by 30%"},
    {"strong", RACE_MOD_STRONG, 1, "Increases physical damage by 10%"},
    {"hercules", RACE_MOD_HERCULES, 3, "Increases physical damage by 20%"},
    {"goliath", RACE_MOD_GOLIATH, 5, "Increases physical damage by 30%"},*/

void do_rmodhelp(CHAR_DATA *ch, char *argument)
{
   char arg[MSL];
   char buf[MSL];

   one_argument(argument, arg);

   if (!str_prefix(arg, "list") || !str_prefix(arg, "show") || !str_prefix(arg, "info"))
   {
      send_to_char("Race Mod           Cost Pre-Req\n\r", ch);
      for (int i = 0; tab_race_mod[i].text != NULL; i++)
      {
         if (tab_race_mod[i].cost == 0)
            continue;

         sprintf(buf, "%18s %4d %s\n\r", tab_race_mod[i].text, tab_race_mod[i].cost,
                 lookup_race_bit_value(tab_race_mod[i].value2));
         send_to_char(buf, ch);
      }

      return;
   }

   for (int i = 0; tab_race_mod[i].text != NULL; i++)
   {
      if (str_cmp(tab_race_mod[i].text, arg))
         continue;

      send_to_char("------RACE MOD INFO-----\n\r", ch);
      sprintf(buf,
              "Name: %s\n\rAA Pre-Req Racial Mod(s): %s\n\rReincarnation Point cost: %d\n\rAffect: "
              "%s\n\r",
              tab_race_mod[i].text, lookup_race_bit_value(tab_race_mod[i].value2),
              tab_race_mod[i].cost, tab_race_mod[i].info);
      send_to_char(buf, ch);
      break;
   }
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char(OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing)
{
   char buf[MAX_STRING_LENGTH];
   char **prgpstrShow;
   int *prgnShow;
   char *pstrShow;
   OBJ_DATA *obj;
   int nShow;
   int iShow;
   int count;
   bool fCombine;

   if (ch->desc == NULL)
      return;
   buf[0] = '\0';

   /*
    * Alloc space for output lines.
    */
   count = 0;
   for (obj = list; obj != NULL; obj = obj->next_in_carry_list)
      count++;
   prgpstrShow = qgetmem(count * sizeof(char *));
   prgnShow = qgetmem(count * sizeof(int));
   nShow = 0;

   /*
    * Format the list of objects.
    */
   for (obj = list; obj != NULL; obj = obj->next_in_carry_list)
   {
      if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj))
      {
         pstrShow = format_obj_to_char(obj, ch, fShort);
         fCombine = FALSE;

         if (IS_NPC(ch) || IS_SET(ch->config, CONFIG_COMBINE))
         {
            /*
             * Look for duplicates, case sensitive.
             * Matches tend to be near end so run loop backwords.
             */
            for (iShow = nShow - 1; iShow >= 0; iShow--)
            {
               if (!strcmp(prgpstrShow[iShow], pstrShow))
               {
                  prgnShow[iShow]++;
                  fCombine = TRUE;
                  break;
               }
            }
         }

         /*
          * Couldn't combine, or didn't want to.
          */
         if (!fCombine)
         {
            prgpstrShow[nShow] = str_dup(pstrShow);
            prgnShow[nShow] = 1;
            nShow++;
         }
      }
   }

   /*
    * Output the formatted list.
    */
   for (iShow = 0; iShow < nShow; iShow++)
   {
      if (IS_NPC(ch) || IS_SET(ch->config, CONFIG_COMBINE))
      {
         if (prgnShow[iShow] != 1)
         {
            sprintf(buf, "(%2d) ", prgnShow[iShow]);
            send_to_char(buf, ch);
         }
         else
         {
            send_to_char("     ", ch);
         }
      }
      send_to_char(prgpstrShow[iShow], ch);
      send_to_char("\n\r", ch);
      free_string(prgpstrShow[iShow]);
   }

   if (fShowNothing && nShow == 0)
   {
      if (IS_NPC(ch) || IS_SET(ch->config, CONFIG_COMBINE))
         send_to_char("     ", ch);
      send_to_char("Nothing.\n\r", ch);
   }

   /*
    * Clean up.
    */
   qdispose(prgpstrShow, count * sizeof(char *));
   qdispose(prgnShow, count * sizeof(int));

   return;
}

void show_room_list_to_char(OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing)
{
   char buf[MAX_STRING_LENGTH];
   char **prgpstrShow;
   int *prgnShow;
   char *pstrShow;
   OBJ_DATA *obj;
   int nShow;
   int iShow;
   int count;
   bool fCombine;

   if (ch->desc == NULL)
      return;
   buf[0] = '\0';

   /*
    * Alloc space for output lines.
    */
   count = 0;
   for (obj = list; obj != NULL; obj = obj->next_in_room)
      count++;
   prgpstrShow = qgetmem(count * sizeof(char *));
   prgnShow = qgetmem(count * sizeof(int));
   nShow = 0;

   /*
    * Format the list of objects.
    */
   for (obj = list; obj != NULL; obj = obj->next_in_room)
   {
      if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj) && str_cmp(obj->description, ""))
      {
         pstrShow = format_obj_to_char(obj, ch, fShort);
         fCombine = FALSE;

         if (IS_NPC(ch) || IS_SET(ch->config, CONFIG_COMBINE))
         {
            /*
             * Look for duplicates, case sensitive.
             * Matches tend to be near end so run loop backwords.
             */
            for (iShow = nShow - 1; iShow >= 0; iShow--)
            {
               if (!strcmp(prgpstrShow[iShow], pstrShow))
               {
                  prgnShow[iShow]++;
                  fCombine = TRUE;
                  break;
               }
            }
         }

         /*
          * Couldn't combine, or didn't want to.
          */
         if (!fCombine)
         {
            prgpstrShow[nShow] = str_dup(pstrShow);
            prgnShow[nShow] = 1;
            nShow++;
         }
      }
   }

   /*
    * Output the formatted list.
    */
   for (iShow = 0; iShow < nShow; iShow++)
   {
      if ((IS_NPC(ch) || IS_SET(ch->config, CONFIG_COMBINE)) && str_cmp(prgpstrShow[iShow], ""))
      {
         if (prgnShow[iShow] != 1)
         {
            sprintf(buf, "(%2d) ", prgnShow[iShow]);
            send_to_char(buf, ch);
         }
         else
         {
            send_to_char("     ", ch);
         }
      }
      send_to_char(prgpstrShow[iShow], ch);
      send_to_char("\n\r", ch);
      free_string(prgpstrShow[iShow]);
   }

   if (fShowNothing && nShow == 0)
   {
      if (IS_NPC(ch) || IS_SET(ch->config, CONFIG_COMBINE))
         send_to_char("     ", ch);
      send_to_char("Nothing.\n\r", ch);
   }

   /*
    * Clean up.
    */
   qdispose(prgpstrShow, count * sizeof(char *));
   qdispose(prgnShow, count * sizeof(int));

   return;
}

void show_char_to_char_0(CHAR_DATA *victim, CHAR_DATA *ch)
{
   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];

   sprintf(buf, "%s", color_string(ch, "mobiles"));
   buf2[0] = '\0';

   if (!IS_NPC(victim))
   {
      /*
       * Then show what race they are (about time this added ;)
       */
      sprintf(buf2, "(%s) ", race_table[victim->race].race_name);
      safe_strcat(MAX_STRING_LENGTH, buf, buf2);
   }

   if (IS_NPC(victim))
   {
      if (IS_SET(victim->act, ACT_UNDEAD) && (is_affected(ch, skill_lookup("detect undead")) ||
                                              item_has_apply(ch, ITEM_APPLY_DET_UNDEAD)))
         safe_strcat(MAX_STRING_LENGTH, buf, "(UnDead) ");
   }

   if (IS_AFFECTED(victim, AFF_INVISIBLE))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Invis) ");

   if (IS_AFFECTED(victim, AFF_HIDE) || item_has_apply(victim, ITEM_APPLY_HIDE))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Hide) ");

   if (IS_AFFECTED(victim, AFF_CHARM))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Charm) ");

   if (IS_AFFECTED(victim, AFF_PASS_DOOR) || item_has_apply(victim, ITEM_APPLY_PASS_DOOR))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Clear) ");

   if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Pink) ");

   if (IS_EVIL(victim) &&
       (IS_AFFECTED(ch, AFF_DETECT_EVIL) || item_has_apply(ch, ITEM_APPLY_DET_EVIL)))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Red) ");

   if (IS_AFFECTED(victim, AFF_SANCTUARY) || item_has_apply(victim, ITEM_APPLY_SANC))
      safe_strcat(MAX_STRING_LENGTH, buf, "(White) ");

   if (IS_NPC(victim) && IS_SET(victim->act, ACT_BOSS))
      safe_strcat(MAX_STRING_LENGTH, buf, "(Boss) ");

   if (!IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER))
      safe_strcat(MAX_STRING_LENGTH, buf, "(KILLER) ");

   if (!IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF))
      safe_strcat(MAX_STRING_LENGTH, buf, "(THIEF) ");

   /*
    * strcat to buf *linkdead* if not connected?
    */
   if (!IS_NPC(victim))
      if ((victim->desc) != NULL && victim->desc->connected != CON_PLAYING)
         safe_strcat(MAX_STRING_LENGTH, buf, "(LINKDEAD)");

   if (!IS_NPC(victim) && IS_SET(victim->pcdata->pflags, PFLAG_RULER))
      safe_strcat(MSL, buf, get_ruler_title(victim->pcdata->ruler_rank, victim->login_sex));
   if (victim->position == POS_STANDING && victim->long_descr[0] != '\0')
   {
      if (IS_NPC(victim))
      {
         char mob_long_descr[MAX_STRING_LENGTH];
         int i;
         int j = 0;

         for (i = 0; victim->long_descr[i] != '\0' && j < MAX_STRING_LENGTH - 1; i++)
         {
            if (victim->long_descr[i] == '\n' || victim->long_descr[i] == '\r')
               continue;
            mob_long_descr[j++] = victim->long_descr[i];
         }

         mob_long_descr[j] = '\0';
         safe_strcat(MAX_STRING_LENGTH, buf, mob_long_descr);
         safe_strcat(MAX_STRING_LENGTH, buf, "\n\r");
      }
      else
         safe_strcat(MAX_STRING_LENGTH, buf, victim->long_descr);

      safe_strcat(MAX_STRING_LENGTH, buf, color_string(ch, "normal"));

      if ((IS_AFFECTED(victim, AFF_CLOAK_FLAMING)) || (IS_AFFECTED(victim, AFF_CLOAK_ABSORPTION)) ||
          (IS_AFFECTED(victim, AFF_CLOAK_REFLECTION)) ||
          (is_affected(victim, skill_lookup("cloak:misery")) ||
           is_affected(victim, skill_lookup("cloak:drain")) ||
           is_affected(victim, skill_lookup("cloak:iron")) ||
           is_affected(victim, skill_lookup("cloak:mental"))) ||
          (is_affected(victim, skill_lookup("cloak:elements"))))
      {
         safe_strcat(MAX_STRING_LENGTH, buf, "  @@NCLOAK:");
         if (IS_AFFECTED(victim, AFF_CLOAK_FLAMING))
            safe_strcat(MAX_STRING_LENGTH, buf, " @@eFLAMING@@N");
         if (is_affected(victim, skill_lookup("cloak:elements")))
            safe_strcat(MAX_STRING_LENGTH, buf, " @@lELEMENTS@@N");
         if (IS_AFFECTED(victim, AFF_CLOAK_ABSORPTION))
            safe_strcat(MAX_STRING_LENGTH, buf, " @@lABSORB@@N");
         if (IS_AFFECTED(victim, AFF_CLOAK_REFLECTION))
            safe_strcat(MAX_STRING_LENGTH, buf, " @@mREFLECT@@N");
         if (is_affected(victim, skill_lookup("cloak:misery")))
            safe_strcat(MSL, buf, " @@RMISERY@@N");
         if (is_affected(victim, skill_lookup("cloak:drain")))
            safe_strcat(MSL, buf, " @@rDRAIN@@N");
         if (is_affected(victim, skill_lookup("cloak:iron")))
            safe_strcat(MSL, buf, " @@WIRON@@N");
         if (is_affected(victim, skill_lookup("cloak:mental")))
            safe_strcat(MSL, buf, " @@mMENTAL@@N");
         safe_strcat(MAX_STRING_LENGTH, buf, "\n\r");
      }

      if (victim->first_shield != NULL)
      {
         MAGIC_SHIELD *this_shield;
         safe_strcat(MSL, buf, "   @@WSHIELD: @@N");
         for (this_shield = victim->first_shield; this_shield != NULL;
              this_shield = this_shield->next)
            safe_strcat(MSL, buf, this_shield->name);
         safe_strcat(MSL, buf, "\n\r");
      }

      send_to_char(buf, ch);
      return;
   }

   safe_strcat(MAX_STRING_LENGTH, buf, PERS(victim, ch));
   if (!IS_NPC(victim) && !IS_SET(ch->config, CONFIG_BRIEF))
      safe_strcat(MAX_STRING_LENGTH, buf, victim->pcdata->title);

   switch (victim->position)
   {
   case POS_DEAD:
      safe_strcat(MAX_STRING_LENGTH, buf, " is DEAD!!");
      break;
   case POS_MORTAL:
      safe_strcat(MAX_STRING_LENGTH, buf, " is mortally wounded.");
      break;
   case POS_INCAP:
      safe_strcat(MAX_STRING_LENGTH, buf, " is incapacitated.");
      break;
   case POS_STUNNED:
      safe_strcat(MAX_STRING_LENGTH, buf, " is lying here stunned.");
      break;
   case POS_SLEEPING:
      safe_strcat(MAX_STRING_LENGTH, buf, " is sleeping here.");
      break;
   case POS_RESTING:
      if (victim->sitting != NULL && victim->sitting->in_room == victim->in_room)
      {
         char sit[MAX_INPUT_LENGTH];
         sprintf(sit, " is here, resting on %s.", victim->sitting->short_descr);
         safe_strcat(MAX_STRING_LENGTH, buf, sit);
      }
      else
         safe_strcat(MAX_STRING_LENGTH, buf, " is resting here.");
      break;
   case POS_STANDING:
   {
      safe_strcat(MAX_STRING_LENGTH, buf, " is here");
      if (!IS_NPC(victim) && (victim->stance > 0) &&
          (!IS_SET(stance_app[victim->stance].specials, STANCE_NINJA)))
      {
         char stance_buf[MSL];
         sprintf(stance_buf, " in the Stance of the %s.", stance_app[victim->stance].name);
         safe_strcat(MSL, buf, stance_buf);
      }
      else
         safe_strcat(MSL, buf, ".");
      break;
   }
   case POS_WRITING:
      safe_strcat(MAX_STRING_LENGTH, buf, " is writing a note.");
      break;
   case POS_BUILDING:
      safe_strcat(MAX_STRING_LENGTH, buf, " is BUILDING!!");
      break;
   case POS_FIGHTING:
      safe_strcat(MAX_STRING_LENGTH, buf, " is here, fighting ");
      if (victim->fighting == NULL)
         safe_strcat(MAX_STRING_LENGTH, buf, "thin air??");
      else if (victim->fighting == ch)
         safe_strcat(MAX_STRING_LENGTH, buf, "YOU!");
      else if (victim->in_room == victim->fighting->in_room)
      {
         safe_strcat(MAX_STRING_LENGTH, buf, PERS(victim->fighting, ch));
         safe_strcat(MAX_STRING_LENGTH, buf, ".");
      }
      else
         safe_strcat(MAX_STRING_LENGTH, buf, "somone who left??");
      break;
   }

   /*
    * buf[0] = UPPER(buf[0]);
    */
   safe_strcat(MAX_STRING_LENGTH, buf, color_string(ch, "normal"));
   safe_strcat(MAX_STRING_LENGTH, buf, "\n\r");

   if ((IS_AFFECTED(victim, AFF_CLOAK_FLAMING)) ||
       (is_affected(victim, skill_lookup("cloak:elements"))) ||
       (IS_AFFECTED(victim, AFF_CLOAK_ADEPT)) || (IS_AFFECTED(victim, AFF_CLOAK_ABSORPTION)) ||
       (IS_AFFECTED(victim, AFF_CLOAK_REFLECTION)) ||
       is_affected(victim, skill_lookup("cloak:misery")) ||
       is_affected(victim, skill_lookup("cloak:drain")) ||
       is_affected(victim, skill_lookup("cloak:iron")) ||
       is_affected(victim, skill_lookup("cloak:mental")))
   {
      safe_strcat(MAX_STRING_LENGTH, buf, "  @@NCLOAK:");
      if (IS_AFFECTED(victim, AFF_CLOAK_FLAMING))
         safe_strcat(MAX_STRING_LENGTH, buf, " @@eFLAMING@@N");
      if (is_affected(victim, skill_lookup("cloak:elements")))
         safe_strcat(MAX_STRING_LENGTH, buf, " @@lELEMENTS@@N");
      if (IS_AFFECTED(victim, AFF_CLOAK_ABSORPTION))
         safe_strcat(MAX_STRING_LENGTH, buf, " @@lABSORB@@N");
      if (IS_AFFECTED(victim, AFF_CLOAK_REFLECTION))
         safe_strcat(MAX_STRING_LENGTH, buf, " @@mREFLECT@@N");

      if (IS_AFFECTED(victim, AFF_CLOAK_ADEPT))
         safe_strcat(MAX_STRING_LENGTH, buf, " @@WADEPT@@N");
      if (is_affected(victim, skill_lookup("cloak:misery")))
         safe_strcat(MSL, buf, " @@RMISERY@@N");
      if (is_affected(victim, skill_lookup("cloak:drain")))
         safe_strcat(MSL, buf, " @@rDRAIN@@N");
      if (is_affected(victim, skill_lookup("cloak:iron")))
         safe_strcat(MSL, buf, " @@WIRON@@N");
      if (is_affected(victim, skill_lookup("cloak:mental")))
         safe_strcat(MSL, buf, " @@mMENTAL@@N");
      safe_strcat(MAX_STRING_LENGTH, buf, "\n\r");
   }
   if (victim->first_shield != NULL)
   {
      MAGIC_SHIELD *this_shield;
      safe_strcat(MSL, buf, "   @@WSHIELD: @@N");
      for (this_shield = victim->first_shield; this_shield != NULL; this_shield = this_shield->next)
         safe_strcat(MSL, buf, this_shield->name);
      safe_strcat(MSL, buf, "\n\r");
   }

   send_to_char(buf, ch);
   return;
}

void show_char_to_char_1(CHAR_DATA *victim, CHAR_DATA *ch)
{
   char buf[MAX_STRING_LENGTH];
   OBJ_DATA *obj;
   int iWear;
   int pct;
   bool found;
   buf[0] = '\0';

   if (can_see(victim, ch))
   {

      act("$n looks at you.", ch, NULL, victim, TO_VICT);
      act("$n looks at $N.", ch, NULL, victim, TO_NOTVICT);
   }

   if (victim->description[0] != '\0')
   {
      send_to_char(victim->description, ch);
   }
   else
   {
      act("You see nothing special about $M.", ch, NULL, victim, TO_CHAR);
   }

   if (get_max_hp(victim) > 0)
      pct = (100 * victim->hit) / get_max_hp(victim);
   else
      pct = -1;

   strcpy(buf, PERS(victim, ch));

   if (pct >= 100)
      safe_strcat(MAX_STRING_LENGTH, buf, " is in pristine condition.\n\r");
   else if (pct >= 90)
      safe_strcat(MAX_STRING_LENGTH, buf, " is slightly scratched.\n\r");
   else if (pct >= 80)
      safe_strcat(MAX_STRING_LENGTH, buf, " has some light bruising.\n\r");
   else if (pct >= 70)
      safe_strcat(MAX_STRING_LENGTH, buf, " has some shallow cuts.\n\r");
   else if (pct >= 60)
      safe_strcat(MAX_STRING_LENGTH, buf, " has several weeping wounds.\n\r");
   else if (pct >= 50)
      safe_strcat(MAX_STRING_LENGTH, buf, " looks like a traffic accident.\n\r");
   else if (pct >= 40)
      safe_strcat(MAX_STRING_LENGTH, buf, " is bleeding slowly into a puddle.\n\r");
   else if (pct >= 30)
      safe_strcat(MAX_STRING_LENGTH, buf, " is spraying blood all over.\n\r");
   else if (pct >= 20)
      safe_strcat(MAX_STRING_LENGTH, buf, " is having trouble living.\n\r");
   else if (pct >= 10)
      safe_strcat(MAX_STRING_LENGTH, buf, " looks ready to kick the bucket.\n\r");
   else
      safe_strcat(MAX_STRING_LENGTH, buf, " is DYING.\n\r");

   buf[0] = UPPER(buf[0]);
   send_to_char(buf, ch);

   found = FALSE;

   {
      for (iWear = 0; iWear < MAX_WEAR; iWear++)
      {
         if ((obj = get_eq_char(victim, iWear)) != NULL && can_see_obj(ch, obj))
         {
            if (!found)
            {
               send_to_char("\n\r", ch);
               act("$N is using:", ch, NULL, victim, TO_CHAR);
               found = TRUE;
            }
            send_to_char(where_name[iWear], ch);
            send_to_char(format_obj_to_char(obj, ch, TRUE), ch);
            send_to_char("\n\r", ch);
         }
      }

      if (victim != ch && !IS_NPC(ch) && can_use_skill(ch, gsn_peek))
      {
         send_to_char("\n\rYou peek at the inventory:\n\r", ch);
         show_list_to_char(victim->first_carry, ch, TRUE, TRUE);
      }
   }
   return;
}

void show_char_to_char(CHAR_DATA *list, CHAR_DATA *ch)
{
   CHAR_DATA *rch;

   for (rch = list; rch != NULL; rch = rch->next_in_room)
   {
      if (rch == ch)
         continue;

      if (!IS_NPC(rch) && IS_SET(rch->act, PLR_WIZINVIS) && get_trust(ch) < rch->invis)
         continue;

      if (can_see(ch, rch))
      {
         show_char_to_char_0(rch, ch);
      }
      else if (room_is_dark(ch->in_room) &&
               (IS_AFFECTED(rch, AFF_INFRARED) || item_has_apply(rch, ITEM_APPLY_INFRA)))
      {
         send_to_char("You see glowing red eyes watching YOU!\n\r", ch);
      }
   }

   return;
}

bool check_blind(CHAR_DATA *ch)
{
   if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
      return TRUE;

   if (IS_AFFECTED(ch, AFF_BLIND))
   {
      send_to_char("You cannot see a thing!\n\r", ch);
      return FALSE;
   }

   return TRUE;
}

void do_look(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char out[MAX_STRING_LENGTH];
   CHAR_DATA *ppl;
   ROOM_INDEX_DATA *room;
   EXIT_DATA *pexit;
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   char pdesc[MSL];
   int door;
   bool found;
   buf[0] = '\0';
   out[0] = '\0';

   if (!IS_NPC(ch) && ch->desc == NULL)
      return;

   if (ch->position < POS_SLEEPING)
   {
      send_to_char("You cannot see anything but stars!\n\r", ch);
      return;
   }

   if (ch->position == POS_SLEEPING)
   {
      send_to_char("You are sleeping!\n\r", ch);
      return;
   }

   if (!check_blind(ch))
      return;

   if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT) && room_is_dark(ch->in_room))
   {
      send_to_char("It is pitch black ... \n\r", ch);
      show_char_to_char(ch->in_room->first_person, ch);
      return;
   }

   argument = one_argument(argument, arg1);
   argument = one_argument(argument, arg2);

   if (arg1[0] == '\0' || !str_cmp(arg1, "auto"))
   {
      /*
       * 'look' or 'look auto'
       */

      /* v2 WebSocket: send structured room panel and map; skip text output */
      if (!IS_NPC(ch) && ch->desc && ch->desc->websocket_active)
      {
         ws_send_room(ch->desc, ch);
         ws_send_map(ch->desc, ch);
         return;
      }

      if (IS_SWITCHED(ch) || (!IS_NPC(ch) && IS_SET(ch->config, CONFIG_MAPPER)))
      {
         send_to_char("\n\r", ch);
         do_mapper(ch, "7");
         send_to_char("\n\r", ch);
      }
      else
      {
         sprintf(out, "%s%s%s\n\r", color_string(ch, "rooms"), ch->in_room->name,
                 color_string(ch, "normal"));

         send_to_char(out, ch);
         if (IS_SWITCHED(ch) || (!IS_NPC(ch) && IS_SET(ch->config, CONFIG_AUTOEXIT)))
            do_exits(ch, "auto");
         if (arg1[0] == '\0' ||
             (IS_SWITCHED(ch) || (!IS_NPC(ch) && !IS_SET(ch->config, CONFIG_BRIEF))))
         {
            char *string_format(char *str, int *numlines, int width, int height, bool unjust);
            int wid = (IS_NPC(ch) ? 80 : ch->pcdata->term_columns);

            {
               char room_descr[MAX_STRING_LENGTH];
               const char *formatted_description = string_format(
                   ch->in_room->description, NULL, wid, 10000, !IS_SET(ch->config, CONFIG_JUSTIFY));
               int i;
               int j = 0;

               for (i = 0; formatted_description[i] != '\0' && j < MAX_STRING_LENGTH - 1; i++)
               {
                  if (formatted_description[i] == '\n' || formatted_description[i] == '\r')
                     continue;
                  room_descr[j++] = formatted_description[i];
               }

               room_descr[j] = '\0';
               out[0] = '\0';
               safe_strcat(MAX_STRING_LENGTH, out, color_string(ch, "rooms"));
               safe_strcat(MAX_STRING_LENGTH, out, room_descr);
               safe_strcat(MAX_STRING_LENGTH, out, color_string(ch, "normal"));
               safe_strcat(MAX_STRING_LENGTH, out, "\n\r");
            }
            send_to_char(out, ch);
         }
      }

      /*
       * Show any room-affects
       */
      if ((ch->in_room->affected_by != 0) && ((is_affected(ch, skill_lookup("detect magic"))) ||
                                              (item_has_apply(ch, ITEM_APPLY_DET_MAG))))
      {
         sprintf(out, "%s", color_string(ch, "rooms"));
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_SHADE))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA menacing @@ddark shadow@@N hangs in the sky above you.\n\r");
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_ENCAPS))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA barely visible @@renergy web@@N is blocking all exits here.\n\r");
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_FIRE_RUNE))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA mystical @@eFire @@NRune@@N hangs in the air above you.\n\r");
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_SHOCK_RUNE))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA mystical @@lShock@@N Rune@@N hangs in the air above you.\n\r");
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_POISON_RUNE))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA mystical @@dPoison@@N Rune hangs in the air above you.\n\r");
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_HEAL_REGEN))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA majestic @@mHealing Light@@N encompasses the room.\n\r");
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_HEAL_STEAL))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA menacing @@dWithering shadow@@N enfolds the room.\n\r");
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_MANA_REGEN))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA powerful @@eMana Flare@@N empowers the room.\n\r");
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_MANA_STEAL))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA mind sapping @@dMana Drain@@N enfolds the room.\n\r");
         if (IS_SET(ch->in_room->affected_by, ROOM_BV_HOLD))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NThe magical bars of a @@rCage@@N surround the room.\n\r");

         if (IS_SET(ch->in_room->affected_by, ROOM_BV_SOUL_NET))
            safe_strcat(MAX_STRING_LENGTH, out,
                        "@@NA demonic @@dSoul Net@@N enshrouds the room.\n\r");

         safe_strcat(MAX_STRING_LENGTH, out, color_string(ch, "normal"));
         send_to_char(out, ch);
      }

      show_room_list_to_char(ch->in_room->first_content, ch, FALSE, FALSE);
      show_char_to_char(ch->in_room->first_person, ch);
      {
         /*         char money_show[MSL];
                  if (ch->in_room->gold > 0)
                  {
                     snprintf(money_show, sizeof(money_show), "%d gold lies in a pile.\n\r",
            ch->in_room->gold); send_to_char(money_show, ch);
                  }*/
      }

      return;
   }

   if (!str_cmp(arg1, "i") || !str_cmp(arg1, "in"))
   {
      /*
       * 'look in'
       */
      if (arg2[0] == '\0')
      {
         send_to_char("Look in what?\n\r", ch);
         return;
      }

      if ((obj = get_obj_here(ch, arg2)) == NULL)
      {
         send_to_char("You do not see that here.\n\r", ch);
         return;
      }

      switch (obj->item_type)
      {
      default:
         send_to_char("That is not a container.\n\r", ch);
         break;

      case ITEM_PORTAL:
         if (obj->value[1] == 0)
         {
            act("You don't seem to be able to look in $p.", ch, obj, NULL, TO_CHAR);
            return;
         }

         if ((room = get_room_index(obj->value[0])) == NULL)
         {
            send_to_char("You see nothing but blackness!\n\r", ch);
            return;
         }

         act("$n looks into $p.", ch, obj, NULL, TO_ROOM);

         if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT) && room_is_dark(ch->in_room))
         {
            act("$p comes out into a dark place.  You see nothing!\n\r", ch, obj, NULL, TO_CHAR);
            return;
         }

         sprintf(buf, "You look in $p and see: %s%s.%s", color_string(ch, "rooms"), room->name,
                 color_string(ch, "normal"));
         act(buf, ch, obj, NULL, TO_CHAR);

         found = FALSE;
         if (room->first_person != NULL)
         {
            send_to_char("You see the following beings:\n\r", ch);
            for (ppl = room->first_person; ppl != NULL; ppl = ppl->next_in_room)
            {
               if (can_see(ch, ppl))
               {
                  found = TRUE;
                  sprintf(buf, "%s%s%s\n\r", color_string(ch, "mobiles"),
                          (IS_NPC(ppl) ? ppl->short_descr : ppl->name), color_string(ch, "normal"));
                  send_to_char(buf, ch);
               }
            }
         }
         if (!found)
            act("You see no beings through $p.", ch, obj, NULL, TO_CHAR);

         if (room->first_content != NULL)
         {
            send_to_char("Some objects are visible:\n\r", ch);
            show_room_list_to_char(room->first_content, ch, TRUE, FALSE);
         }
         else
            act("You see no objects through $p.", ch, obj, NULL, TO_CHAR);

         break;

      case ITEM_SPELL_MATRIX:
      case ITEM_CONTAINER:
      case ITEM_CORPSE_NPC:
      case ITEM_CORPSE_PC:
         if (IS_SET(obj->value[1], CONT_CLOSED))
         {
            send_to_char("It is closed.\n\r", ch);
            break;
         }

         act("$p contains:", ch, obj, NULL, TO_CHAR);
         show_list_to_char(obj->first_in_carry_list, ch, TRUE, TRUE);
         {
         }
         break;
      }
      return;
   }

   if (!str_cmp(arg1, "board") || !str_cmp(arg2, "board"))
   {
      /*
       * int bnum;
       */

      if ((obj = get_obj_here(ch, "board")) == NULL)
      {
         send_to_char("You do not see that here.\n\r", ch);
         return;
      }

      do_show_contents(ch, obj);
      return;
   }

   if ((victim = get_char_room(ch, arg1)) != NULL)
   {
      show_char_to_char_1(victim, ch);
      return;
   }

   for (obj = ch->first_carry; obj != NULL; obj = obj->next_in_carry_list)
   {
      if (can_see_obj(ch, obj))
      {
         sprintf(pdesc, "\n\r%s", get_extra_descr(arg1, obj->first_exdesc));
         if (str_cmp(pdesc, "\n\r(null)"))
         {
            send_to_char(pdesc, ch);
            act("$L$n closely examines $p.", ch, obj, NULL, TO_ROOM);
            return;
         }

         sprintf(pdesc, "\n\r%s", get_extra_descr(arg1, obj->pIndexData->first_exdesc));

         if (str_cmp(pdesc, "\n\r(null)"))
         {
            send_to_char(pdesc, ch);
            act("$L$n closely examines $p.", ch, obj, NULL, TO_ROOM);
            return;
         }
      }

      if (is_name(arg1, obj->name))
      {
         send_to_char(obj->description, ch);
         return;
      }
   }

   for (obj = ch->in_room->first_content; obj != NULL; obj = obj->next_in_room)
   {
      if (can_see_obj(ch, obj))
      {
         sprintf(pdesc, "\n\r%s", get_extra_descr(arg1, obj->first_exdesc));

         if (str_cmp(pdesc, "\n\r(null)"))
         {
            send_to_char(pdesc, ch);
            act("$L$n closely examines $p.", ch, obj, NULL, TO_ROOM);
            return;
         }

         sprintf(pdesc, "\n\r%s", get_extra_descr(arg1, obj->pIndexData->first_exdesc));
         if (str_cmp(pdesc, "\n\r(null)"))
         {
            send_to_char(pdesc, ch);
            act("$L$n closely examines $p.", ch, obj, NULL, TO_ROOM);
            return;
         }
      }

      if (is_name(arg1, obj->name))
      {
         send_to_char(obj->description, ch);
         act("$L$n closely examines $p.", ch, obj, NULL, TO_ROOM);
         return;
      }
   }

   sprintf(pdesc, "\n\r%s", get_extra_descr(arg1, ch->in_room->first_exdesc));
   if (str_cmp(pdesc, "\n\r(null)"))
   {
      send_to_char(pdesc, ch);
      act("$L$n closely examines the $t.", ch, arg1, NULL, TO_ROOM);
      return;
   }

   if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north"))
      door = 0;
   else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east"))
      door = 1;
   else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south"))
      door = 2;
   else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west"))
      door = 3;
   else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up"))
      door = 4;
   else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down"))
      door = 5;
   else
   {
      send_to_char("You do not see that here.\n\r", ch);
      return;
   }

   /*
    * 'look direction'
    */
   if ((pexit = ch->in_room->exit[door]) == NULL)
   {
      send_to_char("Nothing special there.\n\r", ch);
      return;
   }

   if ((pexit->description != NULL) && (pexit->description[0] != '\0') &&
       ((!str_cmp(pexit->keyword, "")) ||
        ((str_cmp(pexit->keyword, "")) && (!str_cmp(pexit->keyword, arg1)))))
      send_to_char(pexit->description, ch);
   else
      send_to_char("Nothing special there.\n\r", ch);

   if ((pexit->keyword != NULL) && (pexit->keyword[0] != '\0') && (pexit->keyword[0] != ' ') &&
       (!str_cmp(pexit->keyword, arg1)))
   {
      if (IS_SET(pexit->exit_info, EX_CLOSED))
      {
         act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
      }
      else if (IS_SET(pexit->exit_info, EX_ISDOOR))
      {
         act("The $d is open.", ch, NULL, pexit->keyword, TO_CHAR);
      }
   }

   return;
}

void do_examine(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   buf[0] = '\0';

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Examine what?\n\r", ch);
      return;
   }

   do_look(ch, arg);

   if ((obj = get_obj_here(ch, arg)) != NULL)
   {
      switch (obj->item_type)
      {
      default:
         break;

      case ITEM_CONTAINER:
      case ITEM_CORPSE_NPC:
      case ITEM_CORPSE_PC:
         send_to_char("When you look inside, you see:\n\r", ch);
         sprintf(buf, "in %s", arg);
         do_look(ch, buf);
      }
      if (ch->position > POS_RESTING)
         trigger_handler(ch, obj, TRIGGER_EXAMINE);
   }

   return;
}

/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits(CHAR_DATA *ch, char *argument)
{
   extern char *const compass_name[];
   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   EXIT_DATA *pexit;
   bool found;
   bool fAuto;
   int door;

   buf[0] = '\0';
   buf2[0] = '\0';

   fAuto = !str_cmp(argument, "auto");

   if (!check_blind(ch))
      return;

   strcpy(buf, fAuto ? "[Exits:" : "Obvious exits:\n\r");

   found = FALSE;
   for (door = 0; door <= 5; door++)
   {
      /*
       * check for players that are blind (irl)
       */
      if (!IS_NPC(ch) && IS_SET(ch->pcdata->pflags, PFLAG_BLIND_PLAYER))
      {
         if ((pexit = ch->in_room->exit[door]) != NULL && pexit->to_room != NULL)
         {
            found = TRUE;
            if (IS_SET(pexit->exit_info, EX_CLOSED))
            {
               sprintf(buf2, " closed %s",
                       (!str_cmp(pexit->keyword, "") ? compass_name[door] : pexit->keyword));
            }
            else
            {
               sprintf(buf2, " %s",
                       (!str_cmp(pexit->keyword, "") ? compass_name[door] : pexit->keyword));
            }
            safe_strcat(MAX_STRING_LENGTH, buf, buf2);
            continue;
         }
      }

      /*
       * Check for thieves with find_doors...
       */
      if (!IS_NPC(ch))
         if ((pexit = ch->in_room->exit[door]) != NULL && pexit->to_room != NULL &&
             IS_SET(pexit->exit_info, EX_CLOSED) && !IS_SET(pexit->exit_info, EX_NODETECT) &&
             can_use_skill(ch, gsn_find_doors) && (!str_cmp(pexit->keyword, "")))
         {
            found = TRUE;
            if (fAuto)
               sprintf(buf2, " (%s)", compass_name[door]);
            else
               sprintf(buf2, "%-5s - Door.\n\r", capitalize(compass_name[door]));

            safe_strcat(MAX_STRING_LENGTH, buf, buf2);
            continue;
         }

      if ((pexit = ch->in_room->exit[door]) != NULL && pexit->to_room != NULL &&
          !IS_SET(pexit->exit_info, EX_CLOSED) && (!str_cmp(pexit->keyword, "")))
      {
         found = TRUE;
         if (fAuto)
         {
            safe_strcat(MAX_STRING_LENGTH, buf, " ");
            safe_strcat(MAX_STRING_LENGTH, buf, compass_name[door]);
         }
         else
         {
            sprintf(buf + strlen(buf), "%-5s - %s\n\r", capitalize(compass_name[door]),
                    room_is_dark(pexit->to_room) ? "Too dark to tell" : pexit->to_room->name);
         }
      }
   }

   if (!found)
      safe_strcat(MAX_STRING_LENGTH, buf, fAuto ? " none" : "None.\n\r");

   if (fAuto)
      safe_strcat(MAX_STRING_LENGTH, buf, "]\n\r");

   send_to_char(buf, ch);
   return;
}

void do_score(CHAR_DATA *ch, char *argument)
{
   /* v2 WebSocket: send structured Score panel; skip text output */
   if (!IS_NPC(ch) && ch->desc && ch->desc->websocket_active)
   {
      ws_send_score(ch->desc, ch);
      return;
   }

   /*
    * Score text has been re-formatted to look a little nicer ;)
    * * Affected by now dealt with by command 'affected'
    * * Also now added color
    * *  -- Stephen
    */

   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   int cnt;
   const int score_inner_width = 78;

   sprintf(buf, "@@y%s%s, Race: %s, Clan: %s\n\rAge: ", IS_NPC(ch) ? ch->short_descr : ch->name,
           IS_NPC(ch) ? "" : ch->pcdata->title,
           IS_NPC(ch) ? "n/a" : race_table[ch->race].race_title,
           IS_NPC(ch) ? "n/a" : clan_table[ch->pcdata->clan].clan_name);
   my_get_age(ch, buf);
   send_to_char(buf, ch);
   sprintf(buf, " (%d hours RL)\n\r", my_get_hours(ch));
   send_to_char(buf, ch);

   /*
    * sprintf( buf,
    * "@@WYou are: @@y%s%s  \n\r@@WAge: @@y%d @@W( @@y%d @@Whrs ) Race: @@y%s  @@WClan:
    * @@y%s.@@g\n\r", IS_NPC(ch) ? ch->short_descr : ch->name, IS_NPC(ch) ? "" : ch->pcdata->title,
    * get_age(ch),
    * (get_age(ch) - 17) * 2,
    * IS_NPC(ch) ? "n/a"  : IS_VAMP(ch) ? "Vampire" : race_table[ch->race].race_title,
    * IS_NPC(ch) ? " n/a" : clan_table[ch->pcdata->clan].clan_name );
    * send_to_char( buf, ch );
    */

   sprintf(
       buf,
       "@@c+------------------------------------------------------------------------------+\n\r");
   send_to_char(buf, ch);

   sprintf(buf,
           "@@y%4ld/%4ld @@WHit @@y%4ld/%4ld @@WMana @@y%4ld/%4ld @@WMovement @@y%3d @@WPractices",
           ch->hit, get_max_hp(ch), ch->mana, get_max_mana(ch), ch->move, get_max_move(ch),
           ch->practice);
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);

   if (IS_NPC(ch))
   {
      sprintf(buf,
              "@@R[ @@WStr:  @@y%2d  @@WInt:  @@y%2d  @@WWis:  @@y%2d  @@WDex:  @@y%2d  @@WCon:  "
              "@@y%2d @@R]",
              get_curr_str(ch), get_curr_int(ch), get_curr_wis(ch), get_curr_dex(ch),
              get_curr_con(ch));
      sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
      send_to_char(buf2, ch);
   }
   else
   {
      sprintf(buf,
              "@@R[ @@WStr:@@y%2d/%2d  @@WInt:@@y%2d/%2d  @@WWis:@@y%2d/%2d  @@WDex:@@y%2d/%2d  "
              "@@WCon:@@y%2d/%2d @@R]",
              get_curr_str(ch), get_max_str(ch), get_curr_int(ch), get_max_int(ch),
              get_curr_wis(ch), get_max_wis(ch), get_curr_dex(ch), get_max_dex(ch),
              get_curr_con(ch), get_max_con(ch));
      sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
      send_to_char(buf2, ch);
   }

   if (!IS_NPC(ch))
   {
      buf2[0] = '\0';
      for (cnt = 0; cnt < 4; cnt++)
      {
         if (ch->mortal_class[cnt] >= 0 && ch->mortal_level[cnt] > 0)
         {
            sprintf(buf, "@@c%s:@@W%2d ", gclass_table[ch->mortal_class[cnt]].who_name,
                    ch->mortal_level[cnt]);
            safe_strcat(MAX_STRING_LENGTH, buf2, buf);
         }
      }
   }
   else
      sprintf(buf2, " @@WLevel: @@y%d", ch->level);

   sprintf(buf, "@@c|%s@@c|\n\r", center_text(buf2, score_inner_width));
   send_to_char(buf, ch);

   if (is_remort(ch))
   {
      buf2[0] = '\0';
      for (cnt = 0; cnt < 2; cnt++)
      {
         if (ch->remort_class[cnt] >= 0)
         {
            sprintf(buf, "@@m%s:@@W%2d ", gclass_table[ch->remort_class[cnt]].who_name,
                    ch->remort_level[cnt]);
            safe_strcat(MAX_STRING_LENGTH, buf2, buf);
         }
      }
      sprintf(buf, "@@c|%s@@c|\n\r", center_text(buf2, score_inner_width));
      send_to_char(buf, ch);
   }

   if (is_adept(ch))
   {
      buf2[0] = '\0';
      if (ch->adept_class >= 0)
      {
         sprintf(buf, "@@d%s:@@W%2d ", gclass_table[ch->adept_class].who_name, ch->adept_level);
         safe_strcat(MAX_STRING_LENGTH, buf2, buf);
      }
      sprintf(buf, "@@c|%s@@c|\n\r", center_text(buf2, score_inner_width));
      send_to_char(buf, ch);
   }

   sprintf(
       buf,
       "@@WExps: @@y%9ld @@c== @@aQuest: @@y%4d @@c== @@GInvasion: @@y%4d @@c== @@MQuest: @@y%4d",
       ch->exp, ch->quest_points, IS_NPC(ch) ? 0 : ch->pcdata->invasion_points,
       IS_NPC(ch) ? 0 : ch->pcdata->post_quest_points);
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);

   if (ch->overgrowth > 0)
   {
      sprintf(buf, "@@aOvergrowth: @@y%d%%@@N", ch->overgrowth);
      sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
      send_to_char(buf2, ch);
   }

   if (score_should_show_invasion_rewards(ch))
   {
      sprintf(buf,
              "@@WInvader's Commendation: @@y%d @@c== @@WInvader's Ribbon: @@y%d @@c== "
              "@@WInvader's Medal: @@y%d",
              ch->pcdata->invasion_rewards[0], ch->pcdata->invasion_rewards[1],
              ch->pcdata->invasion_rewards[2]);
      sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
      send_to_char(buf2, ch);
   }

   if (get_trust(ch) != ch->level)
   {
      sprintf(buf, "@@WYou are trusted at level @@y%2d", get_trust(ch));
      sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
      send_to_char(buf2, ch);
   }

   sprintf(buf, "@@WYou are carrying @@y%4d/%4d @@Witems, weight @@y%4.2f/%7d @@Wkg.",
           ch->carry_number, get_max_carry(ch), ch->carry_weight, get_max_carry_weight(ch));
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);

   sprintf(buf, "@@WAutoexit: @@y%s   @@WAutoloot: @@y%s  @@WAutosac: @@y%s @@WAutoass: @@y%s",
           (!IS_NPC(ch) && IS_SET(ch->config, CONFIG_AUTOEXIT)) ? "*ON* " : "*OFF*",
           (!IS_NPC(ch) && IS_SET(ch->config, CONFIG_AUTOLOOT)) ? "*ON* " : "*OFF*",
           (!IS_NPC(ch) && IS_SET(ch->config, CONFIG_AUTOSAC)) ? "*ON* " : "*OFF*",
           (!IS_NPC(ch) && IS_SET(ch->config, CONFIG_AUTOASSIST)) ? "*ON* " : "*OFF*");
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);

   if (!IS_NPC(ch))
   {
      sprintf(buf, " @@WYou have killed a total of: @@y%d @@WNPCs, and @@y%d @@WPlayers.",
              ch->pcdata->mkills, ch->pcdata->pkills);
      sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
      send_to_char(buf2, ch);

      sprintf(buf, " @@WA total of @@y%d @@WNPCs and @@y%d @@WPlayers have killed you.",
              ch->pcdata->mkilled, ch->pcdata->pkilled);
      sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
      send_to_char(buf2, ch);

      sprintf(buf, " @@WWimpy Set to @@y%d @@WHitPoints.  Page Length is @@y%d @@Wlines.",
              ch->wimpy, ch->pcdata->pagelen);
      sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
      send_to_char(buf2, ch);
   }
   sprintf(buf, "%s", "");
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);
   sprintf(buf, "@@WYou have assumed the @@yStance @@Wof the@@N %s.", stance_app[ch->stance].name);
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);

   switch (ch->position)
   {
   case POS_DEAD:
      sprintf(buf, " You are @@yDEAD!!");
      break;
   case POS_MORTAL:
      sprintf(buf, " You are @@ymortally wounded.");
      break;
   case POS_INCAP:
      sprintf(buf, " You are @@yincapaciated.");
      break;
   case POS_STUNNED:
      sprintf(buf, " You are @@ystunned.");
      break;
   case POS_SLEEPING:
      sprintf(buf, " You are @@ysleeping.");
      break;
   case POS_RESTING:
      sprintf(buf, " You are @@yresting.");
      break;
   case POS_STANDING:
      sprintf(buf, " You are @@ystanding.");
      break;
   case POS_FIGHTING:
      sprintf(buf, " You are @@yfighting.");
      break;
   default:
      sprintf(buf, " You are @@ynone.");
      break;
   }
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);

   sprintf(buf, "@@WYou are ");
   if (get_ac(ch) >= 101)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@yall skin and bones.");
   else if (get_ac(ch) >= 80)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@ynaked.");
   else if (get_ac(ch) >= 0)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@ybadly armored.");
   else if (get_ac(ch) >= -100)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@yslightly armored.");
   else if (get_ac(ch) >= -300)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@ysomewhat armored.");
   else if (get_ac(ch) >= -500)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@yarmored.");
   else if (get_ac(ch) >= -800)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@ywell armored.");
   else if (get_ac(ch) >= -1400)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@ystrongly armored.");
   else if (get_ac(ch) >= -2000)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@yheavily armored.");
   else if (get_ac(ch) >= -3000)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@ysuperbly armored.");
   else if (get_ac(ch) >= -4500)
      safe_strcat(MAX_STRING_LENGTH, buf, "@@ydivinely armored.");
   else
      safe_strcat(MAX_STRING_LENGTH, buf, "@@yalmost invincible!");

   snprintf(buf2, sizeof(buf2), " @@WArmor Class: @@y%5d.   ", get_ac(ch));
   safe_strcat(MAX_STRING_LENGTH, buf2, buf);
   sprintf(buf, "@@c|%s@@c|\n\r", center_text(buf2, score_inner_width));
   send_to_char(buf, ch);

   sprintf(buf, " @@WHitroll: @@y%-5d @@WDamroll: @@y%-5d @@WCrit: @@y%-5d @@WMult: @@y%-5d",
           get_hitroll(ch), get_damroll(ch), get_crit(ch), get_crit_mult(ch));
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);

   sprintf(buf, " @@WSpellpower: @@y%-5d @@WSpell Crit: @@y%-5d @@WSpell Crit Mult: @@y%-5d",
           get_spellpower(ch), get_spell_crit(ch), get_spell_crit_mult(ch));
   if (ch->arcane_power > 0)
   {
      char arcane_buf[MSL];
      sprintf(arcane_buf, " @@WArcane Power: @@y%-5d@@W (+%d%% spell crit)", ch->arcane_power,
              ch->arcane_power * 5);
      safe_strcat(MAX_STRING_LENGTH, buf, arcane_buf);
   }
   if (ch->mental_power > 0)
   {
      char mental_buf[MSL];
      sprintf(mental_buf, " @@mMental Power: @@y%-5d@@W (-%d%% psi/nec/ego/kin cost)",
              ch->mental_power, ch->mental_power * 2);
      safe_strcat(MAX_STRING_LENGTH, buf, mental_buf);
   }
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);
   sprintf(buf,
           "@@WParry: @@y%-3d @@WDodge: @@y%-3d@@W Block: @@y%-3d@@W Counter: @@y%-3d@@W Damcap: "
           "@@y%-5d",
           get_parry(ch), get_dodge(ch), get_block(ch), get_counter(ch), get_damcap(ch));
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);
   sprintf(buf, "@@W Eva Pierc: @@y%-5d", get_evasion_piercing(ch));
   sprintf(buf2, "@@c|%s@@c|\n\r", center_text(buf, score_inner_width));
   send_to_char(buf2, ch);

   if (ch->alignment > 900)
      strcpy(buf, "@@yangelic.");
   else if (ch->alignment > 700)
      strcpy(buf, "@@ysaintly.");
   else if (ch->alignment > 300)
      strcpy(buf, "@@ygood.");
   else if (ch->alignment >= 0)
      strcpy(buf, "@@ygood.");
   else if (ch->alignment > -350)
      strcpy(buf, "@@ymean.");
   else if (ch->alignment > -700)
      strcpy(buf, "@@yevil.");
   else if (ch->alignment > -900)
      strcpy(buf, "@@ydemonic.");
   else
      strcpy(buf, "@@ysatanic!");

   snprintf(buf2, sizeof(buf2), "@@WAlignment: @@y%5d@@W.  You are ", ch->alignment);
   safe_strcat(MAX_STRING_LENGTH, buf2, buf);
   sprintf(buf, "@@c|%s@@c|\n\r", center_text(buf2, score_inner_width));
   send_to_char(buf, ch);

   sprintf(buf, "@@c+------------------------------------------------------------------------------"
                "+@@g\n\r");
   send_to_char(buf, ch);

   return;
}

void do_affected(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   AFFECT_DATA *paf;
   buf[0] = '\0';

   send_to_char(
       "\n\r@@a********************** @@mYou are affected by: @@a*********************@@N\n\r", ch);

   if (ch->first_affect != NULL)
   {
      for (paf = ch->first_affect; paf != NULL; paf = paf->next)
      {
         sprintf(buf, "Spell: '%s'", skill_table[paf->type].name);
         send_to_char(buf, ch);

         if (ch->level >= 16)
         {
            char duration[MSL];
            if (paf->duration < 0 && paf->duration_type == DURATION_HOUR)
               sprintf(duration, "permanent duration");
            else if (paf->duration_type == DURATION_HOUR)
               sprintf(duration, "%d hours", paf->duration);
            else
               sprintf(duration, "%d rounds", paf->duration);
            if (paf->location > APPLY_NONE)
            {
               snprintf(buf, sizeof(buf), " modifies %s by %d for ", affect_loc_name(paf->location),
                        paf->modifier);
               safe_strcat(MAX_STRING_LENGTH, buf, duration);
            }
            else
            {
               snprintf(buf, sizeof(buf), " lasts for ");
               safe_strcat(MAX_STRING_LENGTH, buf, duration);
            }
            send_to_char(buf, ch);
         }

         send_to_char(".\n\r", ch);
      }
      /*
       * Check to see if items have applies...
       */
   }

   if (item_has_apply(ch, ITEM_APPLY_INFRA))
      send_to_char("Spell: 'Infravision' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_INV))
      send_to_char("Spell: 'Invisibilty' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_DET_INV))
      send_to_char("Spell: 'Detect Invisibilty' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_SANC))
      send_to_char("Spell: 'Sanctuary' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_SNEAK))
      send_to_char("Skill: 'Sneak' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_HIDE))
      send_to_char("Skill: 'Hide' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_PROT))
      send_to_char("Spell: 'Protection' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_ENHANCED))
      send_to_char("Skill: 'Enhanced Damage' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_DET_MAG))
      send_to_char("Spell: 'Detect Magic' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_DET_HID))
      send_to_char("Spell: 'Detect Hidden' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_DET_EVIL))
      send_to_char("Spell: 'Detect Evil' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_PASS_DOOR))
      send_to_char("Spell: 'Pass Door' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_DET_POISON))
      send_to_char("Spell: 'Detect Posion' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_FLY))
      send_to_char("Spell: 'Fly' permanent duration.\n\r", ch);
   if (item_has_apply(ch, ITEM_APPLY_KNOW_ALIGN))
      send_to_char("Spell: 'Know Alignment' permanent duration.\n\r", ch);

   sprintf(buf, "*****************************************************************\n\r");
   send_to_char(buf, ch);

   return;
}

char *const day_name[] = {"the Moon", "the Bull",       "Deception", "Thunder",
                          "Freedom",  "the Great Gods", "the Sun"};

/*
 * Number of months reduced from 17 (!) to 8
 * This is to bring the time it takes a character to age on mud year
 * down from 200+ rl hrs to 64 rl hrs
 * (Player's age stats were screwed in base merc!)
 */

char *const month_name[] = {
    "the Frost Giant", "the Old Forces",   "the Grand Struggle",   "Futility",
    "the Dark Shades", "the Long Shadows", "the Ancient Darkness", "the Great Evil"};

void do_time(CHAR_DATA *ch, char *argument)
{
   extern char str_boot_time[];
   char buf[MAX_STRING_LENGTH];
   char *suf;
   int day;

   day = time_info.day + 1;

   if (day > 4 && day < 20)
      suf = "th";
   else if (day % 10 == 1)
      suf = "st";
   else if (day % 10 == 2)
      suf = "nd";
   else if (day % 10 == 3)
      suf = "rd";
   else
      suf = "th";

   sprintf(buf,
           "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\rAck! started up at %s\rThe "
           "system time is %s\r",
           (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
           time_info.hour >= 12 ? "pm" : "am", day_name[day % 7], day, suf,
           month_name[time_info.month], str_boot_time, (char *)ctime(&current_time));

   send_to_char(buf, ch);
   return;
}

void do_weather(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char buf2[MSL];

   static char *const sky_look[4] = {"cloudless", "cloudy", "rainy", "lit by flashes of lightning"};
   buf[0] = '\0';
   buf2[0] = '\0';

   if (!IS_OUTSIDE(ch))
   {
      send_to_char("You can't see the weather indoors.\n\r", ch);
      return;
   }

   if (IS_SET(ch->in_room->affected_by, ROOM_BV_SHADE))
   {
      send_to_char("You see no weather - a dark shadow masks the sky.\n\r", ch);
      return;
   }

   sprintf(buf, "The sky is %s and %s.\n\r", sky_look[weather_info.sky],
           weather_info.change >= 0 ? "a warm southerly breeze blows"
                                    : "a cold northern gust blows");
   switch (weather_info.moon_loc)
   {
   case MOON_DOWN:
      safe_strcat(MSL, buf, "The moon is not in the sky.\n\r");
      break;
   case MOON_RISE:
      sprintf(buf2, "A %s @@yMoon@@N is just rising.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;
   case MOON_LOW:
      sprintf(buf2, "A %s @@yMoon@@N is low on the horizon.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;
   case MOON_PEAK:
      sprintf(buf2, "A %s @@yMoon@@N is high above you.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;
   case MOON_FALL:
      sprintf(buf2, "A %s @@yMoon@@N is falling.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;

   case MOON_SET:
      sprintf(buf2, "A %s @@yMoon@@N has just set.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;
   }

   send_to_char(buf, ch);
   return;
}

void do_help(CHAR_DATA *ch, char *argument)
{
   if (argument[0] == '\0')
      argument = "summary";

   {
      char kw_buf[256];
      char text_buf[16384];
      int lev_out = 0;

      if (!db_help_lookup(argument, get_trust(ch), kw_buf, sizeof(kw_buf), text_buf,
                          sizeof(text_buf), &lev_out))
      {
         send_to_char("No help on that word.\n\r", ch);
         return;
      }

      if (lev_out >= 0 && str_cmp(argument, "staffmotd"))
      {
         send_to_char(kw_buf, ch);
         send_to_char("\n\r", ch);
      }

      /* Strip leading '.' to allow initial blanks. */
      if (text_buf[0] == '.')
         send_to_char(text_buf + 1, ch);
      else
         send_to_char(text_buf, ch);
   }
   return;
}

/*
 * New 'who' command originally by Alander of Rivers of Mud.
 * Changed a lot since then though :P
 * List comes in 4 parts: Imms, Adepts, remorts then morts
 */

#define SHOW_STAFF 0
#define SHOW_ADEPT 1
#define SHOW_REMORT 2
#define SHOW_MORTAL 3
#define SHOW_FINISH 4

/* Returns the character to show in the who list, or NULL if this descriptor
 * should be skipped (not yet in CON_PLAYING state, or has no character). */
CHAR_DATA *who_get_char(DESCRIPTOR_DATA *d)
{
   if (d->connected != CON_PLAYING)
      return NULL;
   return (d->original != NULL) ? d->original : d->character;
}

void do_who(CHAR_DATA *ch, char *argument)
{
   DESCRIPTOR_DATA *d;

   char buf[MAX_STRING_LENGTH * 10];
   char buf2[MAX_STRING_LENGTH * 4];
   char buf3[MAX_STRING_LENGTH * 4];
   char buf4[MAX_STRING_LENGTH * 4];
   char fgs[MAX_STRING_LENGTH * 4];
   char clan_job[MAX_STRING_LENGTH];
   int cnt, slength, excess, nlength;
   bool idle = FALSE, invis = FALSE, wanted = FALSE;
   int player_cnt = 0;
   bool print = FALSE;

   /*
    * Set default arguments.
    */
   buf[0] = '\0';
   buf2[0] = '\0';
   buf3[0] = '\0';
   buf4[0] = '\0';

   sprintf(buf, "WHO Listing: " mudnamecolor "\n\r");
   safe_strcat(MAX_STRING_LENGTH, buf,
               "@@R+-------------------------------------------------------------------------------"
               "-------+\n\r");
   safe_strcat(MAX_STRING_LENGTH, buf,
               "| @@dPr Wl Sw Eg Br To Ws@@R                                                       "
               "         "
               " |\n\r");
   safe_strcat(MAX_STRING_LENGTH, buf,
               "| @@mSo Pa An Kn Ne Mo Wi@@R                                                       "
               "         "
               " |\n\r");
   safe_strcat(
       MAX_STRING_LENGTH, buf,
       "| @@bMa Cl Th Wa Ps Pu Dr Se @@eRace Clan  ABJPW    Player    Title                  "
       "@@R(flags) @@R|\n\r");
   safe_strcat(MAX_STRING_LENGTH, buf,
               "|---------------------------------------------+------------------------------------"
               "----|\n\r");

   for (int i = 0; i < 4; i++)
   {
      print = FALSE;

      for (d = first_desc; d != NULL; d = d->next)
      {
         CHAR_DATA *wch = who_get_char(d);

         if (wch == NULL)
            continue;

         if (i == 0 && wch->level <= MAX_MORTAL)
            continue;
         if (i == 1 && (!is_adept(wch) || wch->level > MAX_MORTAL))
            continue;
         if (i == 2 && (!is_remort(wch) || is_adept(wch) || wch->level > MAX_MORTAL))
            continue;
         if (i == 3 && (is_remort(wch) || is_adept(wch) || wch->level > MAX_MORTAL))
            continue;
         player_cnt++;

         if (!print)
         {
            print = TRUE;
            switch (i)
            {
            case 0:
               safe_strcat(MAX_STRING_LENGTH, buf,
                           "@@R|---------------------------------------------|----------@@"
                           "lStaff@@R---------"
                           "----------------|@@g\n\r");
               break;
            case 1:
               safe_strcat(MAX_STRING_LENGTH, buf,
                           "@@R|---------------------------------------------|------------@@"
                           "WAdepts@@R------"
                           "----------------|@@g\n\r");
               break;
            case 2:
               safe_strcat(MAX_STRING_LENGTH, buf,
                           "@@R|---------------------------------------------|----------@@"
                           "mRemortals@@R-----"
                           "----------------|@@g\n\r");
               break;
            case 3:
               safe_strcat(MAX_STRING_LENGTH, buf,
                           "@@R|---------------------------------------------|-----------@@"
                           "cMortals@@R------"
                           "----------------|@@g\n\r");
               break;
            }
         }

         if (wch->level > MAX_MORTAL)
         {
            /* Staff: display who_name colored by rank; fall back to rank title if unset */
            const char *wn = str_cmp(wch->pcdata->who_name, "off") ? wch->pcdata->who_name : NULL;
            char tmp[MAX_STRING_LENGTH];
            switch (wch->level)
            {
            case MAX_LEVEL - 0:
               sprintf(tmp, "@@l%s@@g", wn ? wn : "Creator");
               break;
            case MAX_LEVEL - 1:
               sprintf(tmp, "@@B%s@@g", wn ? wn : "Supreme");
               break;
            case MAX_LEVEL - 2:
               sprintf(tmp, "@@a%s@@g", wn ? wn : "Deity");
               break;
            case MAX_LEVEL - 3:
               sprintf(tmp, "@@c%s@@g", wn ? wn : "Angel");
               break;
            default:
               sprintf(tmp, "@@W%s@@g", wn ? wn : "Staff");
               break;
            }
            /* Center within 24 visible chars to match mortal/adept column width */
            sprintf(buf3, "%s", center_text(tmp, 24));
         }
         else if (IS_SET(wch->pcdata->pflags, PFLAG_AMBAS))
         {
            sprintf(buf3, "%s", center_text("AMBASSADOR", 24));
         }
         else if (is_adept(wch))
         {
            /* Adept: center title within 24 visible chars */
            sprintf(buf3, "%s", center_text(get_adept_name(wch), 24));
         }
         else if (is_remort(wch))
         {
            /* Remort: display remort class levels */
            buf3[0] = '\0';
            for (cnt = 0; cnt < 2; cnt++)
            {
               if (wch->remort_class[cnt] < 0)
                  continue;
               if (wch->remort_level[cnt] == MAX_MORTAL)
                  sprintf(buf4, " @@m *@@N");
               else
                  sprintf(buf4, " @@m%2d@@N", wch->remort_level[cnt]);
               safe_strcat(MAX_STRING_LENGTH, buf3, buf4);
            }
         }
         else
         {
            /* Mortal: display mortal class levels */
            buf3[0] = '\0';
            for (cnt = 0; cnt < 4; cnt++)
            {
               if (wch->mortal_class[cnt] < 0)
                  continue;
               if (wch->mortal_level[cnt] == MAX_MORTAL)
                  sprintf(buf4, " @@b *@@N");
               else
                  sprintf(buf4, " @@b%2d@@N", wch->mortal_level[cnt]);
               safe_strcat(MAX_STRING_LENGTH, buf3, buf4);
            }
         }
         /*
          * Work out what to show for 'flags' column.
          * PKOK, AFK, LEADER, WRITING, BUILDING
          * ADDED: race and clan (Stephen)
          */
         clan_job[0] = '\0';
         if (IS_SET(wch->pcdata->pflags, PFLAG_CLAN_BOSS))
            safe_strcat(MAX_STRING_LENGTH, clan_job, "*");
         else if (IS_SET(wch->pcdata->pflags, PFLAG_CLAN_LEADER))
            safe_strcat(MAX_STRING_LENGTH, clan_job, "L");
         else if (IS_SET(wch->pcdata->pflags, PFLAG_CLAN_ARMOURER))
            safe_strcat(MAX_STRING_LENGTH, clan_job, "!");
         else
            safe_strcat(MAX_STRING_LENGTH, clan_job, " ");

         sprintf(fgs, "%3s %5s %s%s%s%s%s", race_table[wch->race].race_name,
                 clan_table[wch->pcdata->clan].clan_abbr,
                 IS_SET(wch->pcdata->pflags, PFLAG_AFK) ? "A" : " ",
                 wch->position == POS_BUILDING ? "B" : " ", clan_job,
                 IS_SET(wch->pcdata->pflags, PFLAG_PKOK) ? "P" : " ",
                 wch->position == POS_WRITING ? "W" : " ");
         /*
          * Oh look... another hack needed due to change in who format!
          */
         /*
          * Make sure that the title (and wanted/idle flag) will fit ok
          */
         /*
          * excess holds number of chars we have to lose ):
          */
         nlength = strlen(wch->name);
         slength = 1 + my_strlen(wch->pcdata->title) + nlength;

         excess = 0;

         idle = FALSE;
         wanted = FALSE;
         invis = FALSE;
         if (IS_SET(wch->act, PLR_WIZINVIS))
         {
            excess += 6;
            invis = TRUE;
         }
         else
         {
            if (IS_SET(wch->act, PLR_KILLER) || IS_SET(wch->act, PLR_THIEF))
            {
               excess += 8;
               wanted = TRUE;
            }

            if (wch->timer > 5)
            {
               excess += 10;
               idle = TRUE;
            }
         }
         sprintf(buf4, "%s", wch->pcdata->title);

         if (slength + excess > 43)
         {
            if (invis)
               sprintf(buf4, "%1.26s", wch->pcdata->title);
            else if (idle && !wanted)
               sprintf(buf4, "%1.24s", wch->pcdata->title);
            else if (!idle && wanted)
               sprintf(buf4, "%1.28s", wch->pcdata->title);
            else if (idle && wanted)
               sprintf(buf4, "%1.18s", wch->pcdata->title);
            else
               sprintf(buf4, "%1.31s", wch->pcdata->title);
         }
         else
         {
            for (cnt = slength + excess; cnt < 43; cnt++)
            {
               strcat(buf4, " ");
            }
         }
         if (invis)
         {
            if (slength + excess <= 43)
            {
               buf2[0] = '\0';
               safe_strcat(MAX_STRING_LENGTH, buf2, "@@R|");
               safe_strcat(MAX_STRING_LENGTH, buf2, color_string(ch, "stats"));
               safe_strcat(MAX_STRING_LENGTH, buf2, buf3);
               safe_strcat(MAX_STRING_LENGTH, buf2, color_string(ch, "stats"));
               safe_strcat(MAX_STRING_LENGTH, buf2, "  ");
               safe_strcat(MAX_STRING_LENGTH, buf2, fgs);
               safe_strcat(MAX_STRING_LENGTH, buf2, " @@R| ");
               safe_strcat(MAX_STRING_LENGTH, buf2, color_string(ch, "stats"));
               safe_strcat(MAX_STRING_LENGTH, buf2, wch->name);
               safe_strcat(MAX_STRING_LENGTH, buf2, buf4);
               safe_strcat(MAX_STRING_LENGTH, buf2, IS_SET(wch->act, PLR_WIZINVIS) ? "(WIZI)" : "");
               safe_strcat(MAX_STRING_LENGTH, buf2, "@@R|@@g\n\r");
               safe_strcat(MAX_STRING_LENGTH, buf, buf2);
            }
            else
            {
               buf2[0] = '\0';
               safe_strcat(MAX_STRING_LENGTH, buf2, "@@R|");
               safe_strcat(MAX_STRING_LENGTH, buf2, color_string(ch, "stats"));
               safe_strcat(MAX_STRING_LENGTH, buf2, buf3);
               safe_strcat(MAX_STRING_LENGTH, buf2, color_string(ch, "stats"));
               safe_strcat(MAX_STRING_LENGTH, buf2, "  ");
               safe_strcat(MAX_STRING_LENGTH, buf2, fgs);
               safe_strcat(MAX_STRING_LENGTH, buf2, " @@R| ");
               safe_strcat(MAX_STRING_LENGTH, buf2, color_string(ch, "stats"));
               safe_strcat(MAX_STRING_LENGTH, buf2, wch->name);
               safe_strcat(MAX_STRING_LENGTH, buf2, buf4);
               safe_strcat(MAX_STRING_LENGTH, buf2,
                           IS_SET(wch->act, PLR_WIZINVIS) ? " (WIZI) " : "");
               safe_strcat(MAX_STRING_LENGTH, buf2, "@@R|@@g\n\r");
               safe_strcat(MAX_STRING_LENGTH, buf, buf2);
            }
         }
         else
         {
            buf2[0] = '\0';
            safe_strcat(MAX_STRING_LENGTH, buf2, "@@R|");
            safe_strcat(MAX_STRING_LENGTH, buf2, color_string(ch, "stats"));
            safe_strcat(MAX_STRING_LENGTH, buf2, buf3);
            safe_strcat(MAX_STRING_LENGTH, buf2, color_string(ch, "stats"));
            safe_strcat(MAX_STRING_LENGTH, buf2, "  ");
            safe_strcat(MAX_STRING_LENGTH, buf2, fgs);
            safe_strcat(MAX_STRING_LENGTH, buf2, " @@R| ");
            safe_strcat(MAX_STRING_LENGTH, buf2, color_string(ch, "stats"));
            safe_strcat(MAX_STRING_LENGTH, buf2, wch->name);
            safe_strcat(MAX_STRING_LENGTH, buf2, buf4);
            safe_strcat(MAX_STRING_LENGTH, buf2,
                        (IS_SET(wch->act, PLR_KILLER) || IS_SET(wch->act, PLR_THIEF)) ? "(WANTED)"
                                                                                      : "");
            safe_strcat(MAX_STRING_LENGTH, buf, buf2);

            if (wch->timer > 5)
               snprintf(buf2, sizeof(buf2), "[IDLE:%2d] @@R|@@g\n\r", wch->timer);
            else
               snprintf(buf2, sizeof(buf2), "@@R|@@g\n\r");
            safe_strcat(MAX_STRING_LENGTH, buf, buf2);
         }
      }
   }

   safe_strcat(
       MAX_STRING_LENGTH, buf,
       "@@R|---------------------------------------------+------------------------------------"
       "----|\n\r");
   send_to_char(buf, ch);
   buf[0] = '\0';

   sprintf(buf4, "(%d Player%s)  KEY: (A)fk  (B)uilding  (*)Clan Boss  (P)kok  (W)riting",
           player_cnt, player_cnt == 1 ? "" : "s");
   sprintf(buf2, "@@R|@@G %s @@R|\n\r", center_text(buf4, 84));
   safe_strcat(MAX_STRING_LENGTH, buf, buf2);
   sprintf(buf4, " (L) Clan Leader  (!) Clan Armourer ");
   sprintf(buf2, "@@R|@@G %s @@R|\n\r", center_text(buf4, 84));
   safe_strcat(MAX_STRING_LENGTH, buf, buf2);

   if (happy_hour)
   {
      sprintf(buf4, "@@yHappy Hour is active!@@N");
      sprintf(buf2, "@@R|@@G %s @@R|\r\n", center_text(buf4, 84));
      safe_strcat(MAX_STRING_LENGTH, buf, buf2);
   }

   sprintf(buf4, "There have been a maximum of %d player%s logged on this session", player_cnt,
           player_cnt == 1 ? "" : "s");
   sprintf(buf2, "@@R|@@G %s @@R|\n\r", center_text(buf4, 84));
   safe_strcat(MAX_STRING_LENGTH, buf, buf2);
   safe_strcat(MAX_STRING_LENGTH, buf,
               "@@R|-------------------------------------------------------------------------------"
               "-------|\n\r");

   send_to_char(buf, ch);
   send_to_char(color_string(ch, "normal"), ch);

   return;
}

void do_inventory(CHAR_DATA *ch, char *argument)
{
   if (!IS_NPC(ch) && ch->desc && ch->desc->websocket_active)
   {
      ws_send_inventory(ch->desc, ch);
      return;
   }
   send_to_char("You are carrying:\n\r", ch);
   show_list_to_char(ch->first_carry, ch, TRUE, TRUE);
   return;
}

void do_equipment(CHAR_DATA *ch, char *argument)
{
   do_wear(ch, "");
   return;
}

void do_compare(CHAR_DATA *ch, char *argument)
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   OBJ_DATA *obj1;
   OBJ_DATA *obj2;
   int value1;
   int value2;
   char *msg;

   argument = one_argument(argument, arg1);
   argument = one_argument(argument, arg2);
   if (arg1[0] == '\0')
   {
      send_to_char("Compare what to what?\n\r", ch);
      return;
   }

   if ((obj1 = get_obj_carry(ch, arg1)) == NULL)
   {
      send_to_char("You do not have that item.\n\r", ch);
      return;
   }

   if (arg2[0] == '\0')
   {
      for (obj2 = ch->first_carry; obj2 != NULL; obj2 = obj2->next_in_carry_list)
      {
         if (obj2->wear_loc != WEAR_NONE && can_see_obj(ch, obj2) &&
             obj1->item_type == obj2->item_type &&
             (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
            break;
      }

      if (obj2 == NULL)
      {
         send_to_char("You aren't wearing anything comparable.\n\r", ch);
         return;
      }
   }
   else
   {
      if ((obj2 = get_obj_carry(ch, arg2)) == NULL)
      {
         send_to_char("You do not have that item.\n\r", ch);
         return;
      }
   }

   msg = NULL;
   value1 = 0;
   value2 = 0;

   if (obj1 == obj2)
   {
      msg = "You compare $p to itself.  It looks about the same.";
   }
   else if (obj1->item_type != obj2->item_type)
   {
      msg = "You can't compare $p and $P.";
   }
   else
   {
      switch (obj1->item_type)
      {
      default:
         msg = "You can't compare $p and $P.";
         break;

      case ITEM_ARMOR:
         value1 = obj1->value[0];
         value2 = obj2->value[0];
         break;

      case ITEM_WEAPON:
         value1 = obj1->value[1] + obj1->value[2];
         value2 = obj2->value[1] + obj2->value[2];
         break;
      }
   }

   if (msg == NULL)
   {
      if (value1 == value2)
         msg = "$p and $P look about the same.";
      else if (value1 > value2)
         msg = "$p looks better than $P.";
      else
         msg = "$p looks worse than $P.";
   }

   act(msg, ch, obj1, obj2, TO_CHAR);
   return;
}

void do_credits(CHAR_DATA *ch, char *argument)
{
   do_help(ch, "diku");
   do_help(ch, "ack");
   send_to_char("\n\rAck!Mud 2.2, Modified Merc2.2 by: Stimpy, Thalen\n\r", ch);
   send_to_char("Ack!Mud 4.3 by Zenithar, with help from\n\r", ch);
   send_to_char("Spectrum, Altrag, Ramias, Universe, and others\n\r", ch);
   return;
}

void do_where(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   DESCRIPTOR_DATA *d;
   bool found;
   buf[0] = '\0';

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Players near you:\n\r", ch);
      sprintf(buf, "In %s %s @@N:\n\r", ch->in_room->area->level_label, ch->in_room->area->name);
      send_to_char(buf, ch);
      found = FALSE;
      for (d = first_desc; d; d = d->next)
      {
         if (d->connected == CON_PLAYING && (victim = d->character) != NULL && !IS_NPC(victim) &&
             victim->in_room != NULL && victim->in_room->area == ch->in_room->area &&
             can_see(ch, victim))
         {
            found = TRUE;
            sprintf(buf, "%-28s %s\n\r", victim->name, victim->in_room->name);
            send_to_char(buf, ch);
         }
      }
      if (!found)
         send_to_char("None\n\r", ch);
   }
   else
   {
      found = FALSE;
      for (victim = first_char; victim != NULL; victim = victim->next)
      {
         /*
          * &&   victim->in_room->area == ch->in_room->area
          * * taken out from below to allow global where use
          * * and then put back in... global where no fun at all. ;)
          * * -- Stephen
          */

         if (victim->in_room != NULL &&
             (!IS_AFFECTED(victim, AFF_HIDE) && !item_has_apply(victim, ITEM_APPLY_HIDE)) &&
             (victim->in_room->area == ch->in_room->area) &&
             (!IS_AFFECTED(victim, AFF_SNEAK) && !item_has_apply(victim, ITEM_APPLY_SNEAK)) &&
             can_see(ch, victim) && !(IS_NPC(victim) && invasion_is_hidden_mobile(victim)) &&
             is_name(arg, victim->name))
         {
            found = TRUE;
            sprintf(buf, "%-28s %s\n\r", PERS(victim, ch), victim->in_room->name);
            send_to_char(buf, ch);
            break;
         }
      }
      if (!found)
         act("You didn't find any $T.", ch, NULL, arg, TO_CHAR);
   }

   return;
}

void do_consider(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   char *msg = '\0';
   char *buf = '\0';
   char *buf2 = '\0';
   char *buf3 = '\0';
   char *buf4 = '\0';
   float diff;
   int hpdiff;
   int hrdiff;
   int drdiff;
   int acdiff;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Consider killing whom?\n\r", ch);
      return;
   }

   if ((victim = get_char_room(ch, arg)) == NULL)
   {
      send_to_char("They're not here.\n\r", ch);
      return;
   }

   if (!IS_NPC(victim))
      send_to_char("Remember there are downfalls to PKilling!\n\r", ch);

   /*
    * Stephen - bypass class adjustment if victim == NPC
    */
   /*
    * Also, only look at modifiers if victim == NPC
    */

   /*    diff = victim->level - ch->level; */

   diff = (get_psuedo_level(victim) - get_psuedo_level(ch));
   /*
    * Additions for difficulty.
    */
   /*
    * Class:
    */
   if (!IS_NPC(victim))
   {
      switch (ch->class)
      {
      case 0:
         diff += 0.3;
         break; /* Magi       */
      case 1:
         diff += 0.4;
         break; /* Cleric     */
      case 2:
         diff += 0.1;
         break; /* Cipher     */
      case 3:
         diff += 0;
         break; /* Warden     */
      case 4:
         diff -= 0.4;
         break; /* Psionicist */
      case 5:
         diff += 0.5;
         break; /* Conjurer   */
      case 6:
         diff -= 0.2;
         break; /* Templar    */
      case 7:
         diff += 0.2;
         break; /* Ranger     */
      }
   }

   /*
    * Mod rolls.
    */
   if (IS_NPC(victim))
   {
      diff += victim->hr_mod / 4;
      diff += victim->dr_mod / 4;
      diff -= victim->ac_mod / 30;
   }

   if (diff >= 10)
      msg = "Death will thank you for your gift.";
   if (diff <= 9)
      msg = "$N laughs at you mercilessly.";
   if (diff <= 4)
      msg = "$N says 'Do you feel lucky, punk?'.";
   if (diff <= 1)
      msg = "The perfect match!";
   if (diff <= -2)
      msg = "$N looks like an easy kill.";
   if (diff <= -5)
      msg = "$N is no match for you.";
   if (diff <= -10)
      msg = "You can kill $N naked and weaponless.";

   act(msg, ch, NULL, victim, TO_CHAR);
   msg = "";

   /*
    * additions by king@tinuviel.cs.wcu.edu
    */
   hpdiff = (ch->hit - victim->hit);

   if (((diff >= 0) && (hpdiff <= 0)) || ((diff <= 0) && (hpdiff >= 0)))
   {
      send_to_char("Also,", ch);
   }
   else
   {
      send_to_char("However,", ch);
   }

   if (hpdiff >= 101)
      buf = " you are currently much healthier than $E.";
   if (hpdiff <= 100)
      buf = " you are currently healthier than $E.";
   if (hpdiff <= 50)
      buf = " you are currently slightly healthier than $E.";
   if (hpdiff <= 25)
      buf = " you are a teensy bit healthier than $E.";
   if (hpdiff <= 0)
      buf = " $E is a teensy bit healthier than you.";
   if (hpdiff <= -25)
      buf = " $E is slightly healthier than you.";
   if (hpdiff <= -50)
      buf = " $E is healthier than you.";
   if (hpdiff <= -100)
      buf = " $E is much healthier than you.";

   act(buf, ch, NULL, victim, TO_CHAR);
   buf = "";

   /* LLolth added the following code to make consider show more information */

   drdiff = (get_damroll(ch) - get_damroll(victim));
   if (drdiff >= 20)
      buf2 = "You hit alot harder than $E.";
   if (drdiff <= 10)
      buf2 = "You hit slightly harder than $E.";
   if (drdiff <= 5)
      buf2 = "$E hits about the same as you.";
   if (drdiff <= -5)
      buf2 = "$E hits about the same as you.";
   if (drdiff <= -10)
      buf2 = "$E hits slightly harder than you.";
   if (drdiff <= -20)
      buf2 = "$E hits alot harder than you.";
   act(buf2, ch, NULL, victim, TO_CHAR);
   buf = "";

   hrdiff = (get_hitroll(ch) - get_hitroll(victim));
   if (hrdiff >= 20)
      buf3 = "You hit alot more often than $E.";
   if (hrdiff <= 10)
      buf3 = "You hit slightly more often than $E.";
   if (hrdiff <= 5)
      buf3 = "You and $E hit about equally as often.";
   if (hrdiff <= -10)
      buf3 = "$E hits slightly more often than you.";
   if (hrdiff <= -20)
      buf3 = "$E hits alot more often than you.";
   act(buf3, ch, NULL, victim, TO_CHAR);
   buf3 = "";

   acdiff = (get_ac(victim) - get_ac(ch));
   if (acdiff >= 100)
      buf4 = "You are Armored like a tank compared to $E.";
   if (acdiff <= 75)
      buf4 = "You are armored quite a bit better than $E is.";
   if (acdiff <= 50)
      buf4 = "You are more heavily armored than $E is.";
   if (acdiff <= 25)
      buf4 = "You are slightly more armored than $E is.";
   if (acdiff <= 5)
      buf4 = "You and $E are armored pretty equally.";
   if (acdiff <= -6)
      buf4 = "$E is slightly more armored than you are.";
   if (acdiff <= -50)
      buf4 = "$E is more heavily armored than you are.";
   if (acdiff <= -75)
      buf4 = "$E is armored quite a bit better than you are.";
   if (acdiff <= -100)
      buf4 = "$E is armored like a tank compared to you.";

   act(buf4, ch, NULL, victim, TO_CHAR);
   buf = "";
   return;
}

void set_title(CHAR_DATA *ch, char *title)
{
   char buf[MAX_STRING_LENGTH];
   buf[0] = '\0';

   if (IS_NPC(ch))
   {
      bug("Set_title: NPC.", 0);
      return;
   }

   if (isalpha(title[0]) || isdigit(title[0]))
   {
      buf[0] = ' ';
      strcpy(buf + 1, title);
   }
   else
   {
      strcpy(buf, title);
   }

   free_string(ch->pcdata->title);
   ch->pcdata->title = str_dup(buf);
   return;
}

void do_title(CHAR_DATA *ch, char *argument)
{
   /*
    * Changed this to limit title length, and to remove and brackets. -S-
    */

   char buf[MAX_STRING_LENGTH];
   int cnt;
   bool changed;
   buf[0] = '\0';

   if (IS_NPC(ch))
      return;

   if (argument[0] == '\0')
   {
      send_to_char("Change your title to what?\n\r", ch);
      return;
   }
   changed = FALSE;

   for (cnt = 0; cnt < strlen(argument); cnt++)
   {
      if (argument[cnt] == '[' || argument[cnt] == ']')
      {
         changed = TRUE;
         argument[cnt] = (argument[cnt] == ']') ? '>' : '<';
      }
   }

   if (changed)
      send_to_char("You used either [ or ] in your title.  They have been removed!\n\r", ch);

   /*
    * my_strlen handles color codes as zero length
    */
   if (my_strlen(argument) > 29)
   {
      send_to_char("Title too long.  Please try again.\n\r", ch);
      return;
   }

   smash_tilde(argument);
   set_title(ch, argument);
   sprintf(buf, "You are now: %s%s.\n\r", ch->name, ch->pcdata->title);
   send_to_char(buf, ch);
}

void do_description(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   buf[0] = '\0';

   if (argument[0] != '\0')
   {
      buf[0] = '\0';
      smash_tilde(argument);
      if (argument[0] == '+')
      {
         if (ch->description != NULL)
            safe_strcat(MAX_STRING_LENGTH, buf, ch->description);
         argument++;
         while (isspace(*argument))
            argument++;
      }

      if (strlen(buf) + strlen(argument) >= MAX_STRING_LENGTH - 2)
      {
         send_to_char("Description too long.\n\r", ch);
         return;
      }

      safe_strcat(MAX_STRING_LENGTH, buf, argument);
      safe_strcat(MAX_STRING_LENGTH, buf, "\n\r");
      free_string(ch->description);
      ch->description = str_dup(buf);
   }

   send_to_char("Your description is:\n\r", ch);
   send_to_char(ch->description ? ch->description : "(None).\n\r", ch);
   return;
}

void do_report(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_INPUT_LENGTH];

   sprintf(buf, "You report: %ld/%ld hp %ld/%ld mana %ld/%ld mv %ld xp.\n\r", ch->hit,
           get_max_hp(ch), ch->mana, get_max_mana(ch), ch->move, get_max_move(ch), ch->exp);

   send_to_char(buf, ch);

   sprintf(buf, "$n reports: %ld/%ld hp %ld/%ld mana %ld/%ld mv %ld xp.", ch->hit, get_max_hp(ch),
           ch->mana, get_max_mana(ch), ch->move, get_max_move(ch), ch->exp);

   act(buf, ch, NULL, NULL, TO_ROOM);

   return;
}

/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   int wimpy;
   if (deathmatch)
   {
      send_to_char("Not during a @@eDeath MAtch@@N!!!\n\r", ch);
      return;
   }
   buf[0] = '\0';

   one_argument(argument, arg);

   if (arg[0] == '\0')
      wimpy = get_max_hp(ch) / 5;
   else
      wimpy = atoi(arg);

   if (wimpy < 0)
   {
      send_to_char("Your courage exceeds your wisdom.\n\r", ch);
      return;
   }

   if (wimpy > get_max_hp(ch))
   {
      send_to_char("Such cowardice ill becomes you.\n\r", ch);
      return;
   }

   ch->wimpy = wimpy;
   sprintf(buf, "Wimpy set to %d hit points.\n\r", wimpy);
   send_to_char(buf, ch);
   return;
}

void do_password(CHAR_DATA *ch, char *argument)
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char *pArg;
   char *pwdnew;
   char *p;
   char cEnd;

   if (IS_NPC(ch))
      return;
   arg1[0] = '\0';
   arg2[0] = '\0';

   /*
    * Can't use one_argument here because it smashes case.
    * So we just steal all its code.  Bleagh.
    */
   pArg = arg1;
   while (isspace(*argument))
      argument++;

   cEnd = ' ';
   if (*argument == '\'' || *argument == '"')
      cEnd = *argument++;

   while (*argument != '\0')
   {
      if (*argument == cEnd)
      {
         argument++;
         break;
      }
      *pArg++ = *argument++;
   }
   *pArg = '\0';

   pArg = arg2;
   while (isspace(*argument))
      argument++;

   cEnd = ' ';
   if (*argument == '\'' || *argument == '"')
      cEnd = *argument++;

   while (*argument != '\0')
   {
      if (*argument == cEnd)
      {
         argument++;
         break;
      }
      *pArg++ = *argument++;
   }
   *pArg = '\0';

   if ((ch->pcdata->pwd[0] != '\0') && (arg1[0] == '\0' || arg2[0] == '\0'))
   {
      send_to_char("Syntax: password <old> <new>.\n\r", ch);
      return;
   }

   if ((ch->pcdata->pwd[0] != '\0') && (strcmp(crypt(arg1, ch->pcdata->pwd), ch->pcdata->pwd)))
   {
      WAIT_STATE(ch, 40);
      send_to_char("Wrong password.  Wait 10 seconds.\n\r", ch);
      return;
   }

   if (strlen(arg2) < 5)
   {
      send_to_char("New password must be at least five characters long.\n\r", ch);
      return;
   }

   /*
    * No tilde allowed because of player file format.
    */
   pwdnew = crypt(arg2, ch->name);
   for (p = pwdnew; *p != '\0'; p++)
   {
      if (*p == '~')
      {
         send_to_char("New password not acceptable, try again.\n\r", ch);
         return;
      }
   }

   free_string(ch->pcdata->pwd);
   ch->pcdata->pwd = str_dup(pwdnew);
   save_char_obj(ch);
   send_to_char("Ok.\n\r", ch);
   return;
}

void do_socials(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char out[MAX_STRING_LENGTH * 2];
   int iSocial;
   int col;
   buf[0] = '\0';

   col = 0;
   out[0] = '\0';

   for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
   {
      sprintf(buf, "%-12s", social_table[iSocial].name);
      safe_strcat(MAX_STRING_LENGTH, out, buf);
      if (++col % 6 == 0)
         safe_strcat(MAX_STRING_LENGTH, out, "\n\r");
   }

   if (col % 6 != 0)
      safe_strcat(MAX_STRING_LENGTH, out, "\n\r");
   send_to_char(out, ch);
   return;
}

/* for old command table--Aeria */
/*
 * Contributed by Alander.
 */
/*
 * void do_commands( CHAR_DATA *ch, char *argument )
 * {
 * char buf[MAX_STRING_LENGTH];
 * char buf1[MAX_STRING_LENGTH];
 * int cmd;
 * int col;
 *
 * buf[0] = '\0';
 *
 * if ( IS_NPC(ch) )
 * return;
 *
 *
 *
 * buf1[0] = '\0';
 * col = 0;
 * for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
 * {
 * if ( cmd_table[cmd].level <  LEVEL_HERO
 * &&   cmd_table[cmd].level <= get_trust( ch ) )
 * {
 *
 * if ( cmd_table[cmd].level == CLAN_ONLY
 * && ch->pcdata->clan == 0 )
 * continue;
 *
 * if ( cmd_table[cmd].level == BOSS_ONLY
 * && !IS_SET( ch->act, PLR_CLAN_LEADER ) )
 * continue;
 *
 * if ( cmd_table[cmd].level == VAMP_ONLY
 * && !IS_VAMP( ch ) )
 * continue;
 * sprintf( buf, "%-12s", cmd_table[cmd].name );
 * safe_strcat(MAX_STRING_LENGTH, buf1, buf );
 * if ( ++col % 6 == 0 )
 * safe_strcat(MAX_STRING_LENGTH, buf1, "\n\r" );
 * }
 * }
 *
 * if ( col % 6 != 0 )
 * safe_strcat(MAX_STRING_LENGTH, buf1, "\n\r" );
 *
 * send_to_char( buf1, ch );
 * return;
 * }
 */

struct show_cmds
{
   char buf[MSL];
   sh_int col;
};

static char *const cmd_group_names[] = {
    "@@WMisc",    "@@aCommunication",     "@@mConfiguration", "@@eInformation",
    "@@rActions", "@@dItem Manipulation", "@@cInterMud",      "@@yStaff"};

void do_commands(CHAR_DATA *ch, char *argument)
{
   static struct show_cmds show_table[8];
   char buf[MAX_STRING_LENGTH];
   char buf1[MAX_STRING_LENGTH];
   char arg1[MSL];
   int cmd;
   int col = 0;
   sh_int show_only = -1;

   buf[0] = '\0';
   buf1[0] = '\0';

   if (IS_NPC(ch))
      return;
   /* NOTE: This is better coded via a build_tab.c style lookup, but this is
      quicker to code right now :) Zen */

   argument = one_argument(argument, arg1);
   if (arg1[0] != '\0')
   {
      if (!str_prefix(arg1, "miscellaneous"))
         show_only = 0;
      else if (!str_prefix(arg1, "communication"))
         show_only = 1;
      else if (!str_prefix(arg1, "configuration"))
         show_only = 2;
      else if (!str_prefix(arg1, "information"))
         show_only = 3;
      else if (!str_prefix(arg1, "actions"))
         show_only = 4;
      else if (!str_prefix(arg1, "objects") || !str_prefix(arg1, "manipulation"))
         show_only = 5;
      else if (!str_prefix(arg1, "staff"))
         show_only = 6;
   }

   for (col = 0; col < 7; col++)
   {
      sprintf(show_table[col].buf, "%s:\n\r", cmd_group_names[col]);
      show_table[col].col = 0;
   }

   col = 0;

   for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
   {
      if (cmd_table[cmd].level <= L_GOD && cmd_table[cmd].level <= get_trust(ch))
      {

         if (cmd_table[cmd].level == CLAN_ONLY && ch->pcdata->clan == 0)
            continue;

         if (cmd_table[cmd].level == BOSS_ONLY && !IS_SET(ch->act, PLR_CLAN_LEADER))
            continue;

         /*	     if ( cmd_table[cmd].level == SYSTEM_ONLY
                  && get_trust( ch ) < L_SUP )
                continue;
         */
         if (cmd_table[cmd].show == C_SHOW_NEVER)
            continue;
         if ((cmd_table[cmd].show == C_SHOW_SKILL) &&
             (!can_use_skill(ch, skill_lookup(cmd_table[cmd].name))))
            continue;

         sprintf(buf, "%-12s", cmd_table[cmd].name);
         safe_strcat(MSL, show_table[cmd_table[cmd].type].buf, buf);
         if (++show_table[cmd_table[cmd].type].col % 6 == 0)
            safe_strcat(MSL, show_table[cmd_table[cmd].type].buf, "\n\r");
      }
   }

   send_to_char(buf1, ch);
   for (col = 0; col < 7; col++)
   {
      if ((show_only > -1) && (show_only != col))
         continue;
      safe_strcat(MAX_STRING_LENGTH, show_table[col].buf, "\n\r");
      send_to_char(show_table[col].buf, ch);
   }

   return;
}

struct chan_type
{
   int bit;
   int min_level;
   char *name;
   char *on_string;
   char *off_string;
};

struct chan_type channels[] = {
    {CHANNEL_AUCTION, 0, "auction", "[ +AUCTION  ] You hear biddings.@@N\n\r",
     "[ -auction  ] You don't hear biddings.\n\r"},
    {CHANNEL_MUSIC, 0, "music", "[ +MUSIC    ] You hear people's music quotes.\n\r",
     "[ -music    ] You don't hear people's music quotes.\n\r"},
    {CHANNEL_STAFFTALK, LEVEL_HERO, "stafftalk",
     "[ +STAFFTALK] You hear what other staff have to say.\n\r",
     "[ -stafftalk] You don't hear what other staff have to say.\n\r"},
    {CHANNEL_NOTIFY, LEVEL_HERO, "notify", "[ +NOTIFY   ] You hear player information.\n\r",
     "[ -notify   ] You don't hear player information.\n\r"},
    {CHANNEL_LOG, 0, "log", "[ +LOG      ] You receive LOG_FILE details.\n\r",
     "[ -log      ] You don't receive LOG_FILE details.\n\r"},

    {CHANNEL_INVASION, 0, "invasion", "[ +INVASION ] You hear invasion reports!\n\r",
     "[ -invasion ] You ignore invasion reports.\n\r"},

    {CHANNEL_NEWBIE, 0, "newbie", "[ +NEWBIE   ] You hear newbie's chit-chat.\n\r",
     "[ -newbie   ] You don't hear newbie's chit-chat.\n\r"},

    {CHANNEL_QUESTION, 0, "question", "[ +QUESTION ] You hear player's questions & answers.\n\r",
     "[ -question ] You don't hear player's questions & answers.\n\r"},

    {CHANNEL_SHOUT, 0, "shout", "[ +SHOUT    ] You hear people shouting.\n\r",
     "[ -shout    ] You don't hear people shouting.\n\r"},

    {CHANNEL_YELL, 0, "yell", "[ +YELL     ] You hear people yelling.\n\r",
     "[ -yell     ] You don't hear people yelling.\n\r"},

    {CHANNEL_CLAN, 0, "clan", "[ +CLAN     ] You hear clan chit-chat.\n\r",
     "[ -clan     ] You don't hear clan chit-chat.\n\r"},

    {CHANNEL_GAME, 0, "game", "[ +GAME     ] You hear players participating in games.\n\r",
     "[ -game     ] You no longer hear people playing games.\n\r"},

    {CHANNEL_RACE, 0, "race", "[ +RACE     ] You hear your race's chit-chat.\n\r",
     "[ -race     ] You don't hear your race's chit-chat.\n\r"},

    {CHANNEL_FLAME, 0, "flame", "[ +FLAME    ] You hear players flaming each other.\n\r",
     "[ -flame    ] You don't hear players flaming each other.\n\r"},

    {CHANNEL_ZZZ, 0, "zzz", "[ +ZZZ      ] You hear sleeping players chatting.\n\r",
     "[ -zzz      ] You don't hear sleeping players chatting.\n\r"},

    {CHANNEL_INFO, 0, "info", "[ +INFO     ] You hear information about deaths, etc.\n\r",
     "[ -info     ] You don't hear information about deaths, etc.\n\r"},

    {CHANNEL_CREATOR, 85, "creator", "[ +CREATOR  ] You hear Creator's discussing Mud matters.\n\r",
     "[ -creator  ] You don't hear Creators discussing Mud matters\n\r"},

    {CHANNEL_ALLCLAN, 85, "allclan", "[ +ALLCLAN  ] You hear ALL clan channels.\n\r",
     "[ -allclan  ] You don't hear ALL clan channels.\n\r"},

    {CHANNEL_ALLRACE, 85, "allrace", "[ +ALLRACE  ] You hear ALL race channels.\n\r",
     "[ -allrace  ] You don't hear ALL race channels.\n\r"},

    {CHANNEL_HERMIT, 0, "hermit", "[ +HERMIT   ] You are NOT ignoring all channels.\n\r",
     "[ -hermit   ] YOU ARE IGNORING ALL CHANNELS!\n\r"},

    {CHANNEL_BEEP, 0, "beep", "[ +BEEP     ] You accept 'beeps' from other players.\n\r",
     "[ -beep     ] You are ignoring 'beeps' from other players.\n\r"},
    {CHANNEL_ALLCLAN, 85, "allclan", "[ +ALLCLAN  ] You hear ALL clan channels.\n\r",
     "[ -allclan  ] You don't hear ALL clan channels.\n\r"},

    {CHANNEL_DIPLOMAT, 0, "diplomat", "[ +DIPLOMAT ] You hear diplomatic negotioations.\n\r",
     "[ -diplomat ] YOU do not hear diplomatic negotiations\n\r"},

    {CHANNEL_REMORTTALK, 0, "{", "[ +{        ] You hear remorts gossiping amongst themselves.\n\r",
     "[ -{        ] You are ignoring idle remort chatter.\n\r"},

    {CHANNEL_CRUSADE, 0, "crusade", "[ +CRUSADE  ] You hear quest information.\n\r",
     "[ -crusade  ] You are ignoring quest information.\n\r"},

    {CHANNEL_ADEPT, 0, "adept", "[  +ADEPT   ] You hear Adepts plotting to take over the mud!\n\r",
     "[  -adept   ] You ignore those bored dudes and dudettes.\n\r"},

    {CHANNEL_GAME, 0, "game", "[   +GAME   ] You hear non-roleplaying games.\n\r",
     "[   -game   ] You are ignoring non-roleplaying games.\n\r"},

    {CHANNEL_OOC, 0, "ooc", "[   +OOC    ] You hear non-roleplaying chit-chat.\n\r",
     "[   -ooc    ] You ignore those boring non-roleplaying fools!\n\r"},

    {0, 0, NULL, NULL}};

void do_channels(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char buffer[MAX_STRING_LENGTH];
   int a, trust;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      if (!IS_NPC(ch) && IS_SET(ch->act, PLR_SILENCE))
      {
         send_to_char("You are silenced.\n\r", ch);
         return;
      }

      trust = get_trust(ch);
      buffer[0] = '\0';
      safe_strcat(MAX_STRING_LENGTH, buffer, "Channels:\n\r");

      for (a = 0; channels[a].bit != 0; a++)
      {
         if (trust >= channels[a].min_level)
         {
            char colbuf[10];
            colbuf[0] = '\0';

            if (IS_SET(ch->deaf, channels[a].bit))
            {
               if (!IS_NPC(ch))
               {
                  sprintf(colbuf, "@@%c", ch->pcdata->dimcol);
                  safe_strcat(MSL, buffer, colbuf);
               }
               safe_strcat(MAX_STRING_LENGTH, buffer, channels[a].off_string);
            }
            else
            {
               if (!IS_NPC(ch))
               {
                  sprintf(colbuf, "@@%c", ch->pcdata->hicol);
                  safe_strcat(MSL, buffer, colbuf);
               }
               safe_strcat(MAX_STRING_LENGTH, buffer, channels[a].on_string);
            }
         }

         safe_strcat(MAX_STRING_LENGTH, buffer, "@@N");
      }
      send_to_char(buffer, ch);
   }
   else
   {
      bool fClear;
      int bit;

      if (arg[0] == '+')
         fClear = TRUE;
      else if (arg[0] == '-')
         fClear = FALSE;
      else
      {
         send_to_char("Channels -channel or +channel?\n\r", ch);
         return;
      }

      /*
       * Now check through table to set/unset channel...
       */
      bit = 0;
      for (a = 0; channels[a].bit != 0; a++)
      {
         if (channels[a].min_level > get_trust(ch))
            continue;
         if (!str_prefix(arg + 1, channels[a].name))
         {
            bit = channels[a].bit;
            break;
         }
      }
      if (bit == 0)
      {
         send_to_char("Set or clear which channel?\n\r", ch);
         return;
      }

      if (fClear)
         REMOVE_BIT(ch->deaf, bit);
      else
         SET_BIT(ch->deaf, bit);

      send_to_char("Ok.\n\r", ch);
   }

   return;
}

/*
 * Contributed by Grodyn.
 */
void do_config(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MSL];
   if (IS_NPC(ch))
      return;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("[ Keyword  ] Option\n\r", ch);

      send_to_char(IS_SET(ch->act, PLR_NOSUMMON) ? "[+NOSUMMON ] You may not be summoned.\n\r"
                                                 : "[-nosummon ] You may be summoned.\n\r",
                   ch);

      send_to_char(IS_SET(ch->act, PLR_NOVISIT) ? "[+NOVISIT  ] You may not be 'visited'.\n\r"
                                                : "[-novisit  ] You may be 'visited'.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_COLOR)
                       ? "[+COLOR    ] You recieve 'ANSI' color.\n\r"
                       : "[-color    ] You don't receive 'ANSI' color.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_AUTOEXIT)
                       ? "[+AUTOEXIT ] You automatically see exits.\n\r"
                       : "[-autoexit ] You don't automatically see exits.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_AUTOLOOT)
                       ? "[+AUTOLOOT ] You automatically loot corpses.\n\r"
                       : "[-autoloot ] You don't automatically loot corpses.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_AUTOASSIST)
                       ? "[+AUTOASS  ] You automatically assist.\n\r"
                       : "[-autoass  ] You don't automatically assist.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_SHORT_FIGHT)
                       ? "[+SHORTFIGHT] You see short autoattack messages.\n\r"
                       : "[-shortfight] You see detailed autoattack messages.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_AUTOSAC)
                       ? "[+AUTOSAC  ] You automatically sacrifice corpses.\n\r"
                       : "[-autosac  ] You don't automatically sacrifice corpses.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_BLANK)
                       ? "[+BLANK    ] You have a blank line before your prompt.\n\r"
                       : "[-blank    ] You have no blank line before your prompt.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_BRIEF) ? "[+BRIEF    ] You see brief descriptions.\n\r"
                                                    : "[-brief    ] You see long descriptions.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_COMBINE)
                       ? "[+COMBINE  ] You see object lists in combined format.\n\r"
                       : "[-combine  ] You see object lists in single format.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_PROMPT) ? "[+PROMPT   ] You have a prompt.\n\r"
                                                     : "[-prompt   ] You don't have a prompt.\n\r",
                   ch);

      send_to_char(IS_SET(ch->config, CONFIG_TELNET_GA)
                       ? "[+TELNETGA ] You receive a telnet GA sequence.\n\r"
                       : "[-telnetga ] You don't receive a telnet GA sequence.\n\r",
                   ch);
      send_to_char(IS_SET(ch->config, CONFIG_FULL_ANSI)
                       ? "[+FULLANSI ] Your client supports FULL ANSI.\n\r"
                       : "[-fullansi ] Your client does not support full ANSI (GMUD).\n\r",
                   ch);
      send_to_char(IS_SET(ch->config, CONFIG_MAPPER)
                       ? "[+DISPLAY  ] You are viewing the ASCII display map!\n\r"
                       : "[-display  ] Your are not viewing the ASCII display map.\n\r",
                   ch);
      send_to_char(IS_SET(ch->config, CONFIG_JUSTIFY)
                       ? "[+JUSTIFY  ] You are viewing rooms in space justified format.\n\r"
                       : "[-justify  ] Your are not viewing rooms space justified.\n\r",
                   ch);
      send_to_char(IS_SET(ch->act, PLR_NO_PRAY) ? "[+NOPRAY   ] You cannot use 'pray'.\n\r" : "",
                   ch);

      send_to_char(IS_SET(ch->act, PLR_SILENCE) ? "[+SILENCE  ] You are silenced.\n\r" : "", ch);

      send_to_char(!IS_SET(ch->act, PLR_NO_EMOTE) ? "" : "[-emote    ] You can't emote.\n\r", ch);

      send_to_char(!IS_SET(ch->act, PLR_NO_TELL) ? "" : "[-tell     ] You can't use 'tell'.\n\r",
                   ch);
      sprintf(buf, "Terminal set to:  %i Rows, %i Columns.\n\r", ch->pcdata->term_rows,
              ch->pcdata->term_columns);
      send_to_char(buf, ch);
   }
   else
   {
      bool fSet;
      int bit;
      bool config_var = TRUE;
      if (arg[0] == '+')
         fSet = TRUE;
      else if (arg[0] == '-')
         fSet = FALSE;
      else
      {
         send_to_char("Config -option or +option?\n\r", ch);
         return;
      }

      if (!str_cmp(arg + 1, "autoexit"))
         bit = CONFIG_AUTOEXIT;
      else if (!str_cmp(arg + 1, "autoloot"))
         bit = CONFIG_AUTOLOOT;
      else if (!str_cmp(arg + 1, "autoassist"))
         bit = CONFIG_AUTOASSIST;
      else if (!str_cmp(arg + 1, "autosac"))
         bit = CONFIG_AUTOSAC;
      else if (!str_cmp(arg + 1, "shortfight"))
         bit = CONFIG_SHORT_FIGHT;
      else if (!str_cmp(arg + 1, "blank"))
         bit = CONFIG_BLANK;
      else if (!str_cmp(arg + 1, "brief"))
         bit = CONFIG_BRIEF;
      else if (!str_cmp(arg + 1, "combine"))
         bit = CONFIG_COMBINE;
      else if (!str_cmp(arg + 1, "prompt"))
         bit = CONFIG_PROMPT;
      else if (!str_cmp(arg + 1, "telnetga"))
         bit = CONFIG_TELNET_GA;
      else if (!str_cmp(arg + 1, "nosummon"))
      {
         config_var = FALSE;
         bit = PLR_NOSUMMON;
      }
      else if (!str_cmp(arg + 1, "novisit"))
      {
         config_var = FALSE;
         bit = PLR_NOVISIT;
      }
      else if (!str_cmp(arg + 1, "color"))
         bit = CONFIG_COLOR;
      else if (!str_cmp(arg + 1, "fullansi"))
         bit = CONFIG_FULL_ANSI;
      else if (!str_cmp(arg + 1, "display"))
         bit = CONFIG_MAPPER;
      else if (!str_cmp(arg + 1, "justify"))
         bit = CONFIG_JUSTIFY;

      else if (!str_prefix("rows", arg + 1))
      {
         char arg1[MSL];
         argument = one_argument(argument, arg1);
         argument = one_argument(argument, arg1);
         if (!is_number(arg1))
         {
            send_to_char("That is not a number.\n\r", ch);
            return;
         }
         ch->pcdata->term_rows = URANGE(10, atoi(arg1), 80);
         return;
      }
      else
      {
         send_to_char("Config which option?\n\r", ch);
         return;
      }

      if (fSet)
      {
         if (config_var)
            SET_BIT(ch->config, bit);
         else
            SET_BIT(ch->act, bit);

         if (bit == CONFIG_FULL_ANSI)
         {
            char scrollbuf[MSL];
            sprintf(scrollbuf, "%s%s%s%i;%ir%s%i;%iH", CRS_RESET, CRS_CLS, CRS_CMD, 0,
                    ch->pcdata->term_rows - 12, CRS_CMD, ch->pcdata->term_rows - 13, 0);
            send_to_char(scrollbuf, ch);
         }
      }
      else
      {
         if (config_var)
            REMOVE_BIT(ch->config, bit);
         else
            REMOVE_BIT(ch->act, bit);
         if (bit == CONFIG_FULL_ANSI)
         {

            send_to_char(CRS_RESET, ch);
            send_to_char("Done.\n\r", ch);
         }
      }

      send_to_char("Ok.\n\r", ch);
   }

   return;
}

void do_wizlist(CHAR_DATA *ch, char *argument)
{

   do_help(ch, "wizlist");
   return;
}

void do_race_list(CHAR_DATA *ch, char *argument)
{
   int iRace;
   char buf[MAX_STRING_LENGTH];

   send_to_char("    Here follows a list of current races within ACK! Mud:\n\r", ch);
   send_to_char("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r", ch);
   send_to_char("\n\r", ch);
   if (IS_STAFF(ch))
      send_to_char("    No.   Room.    Abbr.    Name.   M/C   Classes: (Good->Bad)\n\r", ch);
   else
      send_to_char("   Abbr.    Name.  M/C  Classes: (Good->Bad)\n\r", ch);

   for (iRace = 0; iRace < MAX_RACE; iRace++)
   {
      if (IS_STAFF(ch))
      {
         sprintf(buf, "   %3d   %5d    %5s     %8s %2d %s %s\n\r", iRace, race_table[iRace].recall,
                 race_table[iRace].race_name, race_table[iRace].race_title,
                 race_table[iRace].classes, class_order(iRace),
                 (race_table[iRace].player_allowed == TRUE ? "@@aPlayer@@N" : "@@eNPC ONLY@@N"));
         send_to_char(buf, ch);
      }

      else if (race_table[iRace].player_allowed == TRUE)
      {
         sprintf(buf, "   %5s    %8s    %2d %s\n\r", race_table[iRace].race_name,
                 race_table[iRace].race_title, race_table[iRace].classes,
                 race_table[iRace].comment);
         send_to_char(buf, ch);
      }
   }
   send_to_char("\n\r", ch);
   send_to_char("M/C = Number of classes available.\n\r", ch);
   send_to_char("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r", ch);
   return;
}

void do_clan_list(CHAR_DATA *ch, char *argument)
{
   int iClan;
   char buf[MAX_STRING_LENGTH];

   send_to_char("\n\r    Here follows a list of current clans for " mudnamecolor ":\n\r", ch);
   send_to_char("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r", ch);
   send_to_char("\n\r", ch);
   if (IS_STAFF(ch))
      send_to_char("    No.   Room.   Abbr.     Leader    Name.\n\r", ch);
   else
      send_to_char("   Abbr.          Leader     Name.\n\r", ch);

   for (iClan = 0; iClan < MAX_CLAN; iClan++)
   {
      if (!IS_STAFF(ch) && iClan == 0)
         continue; /* Don't list 'none' as a clan :) */
      if (IS_STAFF(ch))
         sprintf(buf, "   %3d   %5d    %s  %12s  %s\n\r", iClan, clan_table[iClan].clan_room,
                 clan_table[iClan].clan_abbr, clan_table[iClan].leader,
                 clan_table[iClan].clan_name);
      else
         sprintf(buf, "   %s    %12s  %s\n\r", clan_table[iClan].clan_abbr,
                 clan_table[iClan].leader, clan_table[iClan].clan_name);

      send_to_char(buf, ch);
   }
   send_to_char("\n\r", ch);
   send_to_char("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r", ch);
   return;
}

void do_spells(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char buf1[MAX_STRING_LENGTH];
   int sn;
   int col;
   buf[0] = '\0';

   /*
    * if ( ( !IS_NPC( ch ) &&
    * !gclass_table[ch->class].fMana )
    * ||  IS_NPC ( ch ) )
    */
   if (IS_NPC(ch))
   {
      send_to_char("You do not know how to cast spells!\n\r", ch);
      return;
   }

   buf1[0] = '\0';

   col = 0;
   for (sn = 0; sn < MAX_SKILL; sn++)
   {
      if (skill_table[sn].name == NULL)
         break;

      if (!can_use_skill(ch, sn))
         continue;

      if (mana_cost(ch, sn) < 2)
         continue;

      sprintf(buf, "%18s %3dpts ", skill_table[sn].name, mana_cost(ch, sn));
      safe_strcat(MAX_STRING_LENGTH, buf1, buf);
      if (++col % 3 == 0)
         safe_strcat(MAX_STRING_LENGTH, buf1, "\n\r");
   }

   send_to_char(buf1, ch);
   return;
}

void do_slist(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char buf1[MAX_STRING_LENGTH];
   int class;
   int foo;
   int sn;
   int col;
   int level;
   bool pSpell;
   bool any;
   bool remort_class;
   bool adept_class;
   buf[0] = '\0';
   buf1[0] = '\0';

   if (IS_NPC(ch))
   {
      send_to_char("You do not need any stinking spells!\n\r", ch);
      return;
   }

   if (argument[0] == '\0')
   {
      send_to_char("USAGE:  <abbrev>.  abbrev = 3-letter class abreviation.\n\r", ch);
      return;
   }

   any = FALSE;
   class = -1;
   remort_class = FALSE;
   adept_class = FALSE;
   for (foo = 0; foo < MAX_TOTAL_CLASS; foo++)
   {
      if (!str_cmp(gclass_table[foo].who_name, argument) ||
          !str_cmp(gclass_table[foo].class_name, argument))
      {
         any = TRUE;
         class = foo;
         if (IS_REMORT_CLASS(foo))
            remort_class = TRUE;
         else if (IS_ADEPT_CLASS(foo))
            adept_class = TRUE;
      }
   }

   if (!any)
   {
      send_to_char("That abbreviation not recognized!\n\r", ch);
      return;
   }

   buf1[0] = '\0';

   safe_strcat(MAX_STRING_LENGTH, buf1, "ALL Spells available for the class ");
   if (adept_class)
   {
      safe_strcat(MAX_STRING_LENGTH, buf1, "@@W");
      safe_strcat(MAX_STRING_LENGTH, buf1, gclass_table[class].class_name);
      safe_strcat(MAX_STRING_LENGTH, buf1, "@@N\n\r\n\r");
   }
   else if (remort_class)
   {
      safe_strcat(MAX_STRING_LENGTH, buf1, "@@m");
      safe_strcat(MAX_STRING_LENGTH, buf1, gclass_table[class].class_name);
      safe_strcat(MAX_STRING_LENGTH, buf1, "@@N\n\r\n\r");
   }
   else

   {
      safe_strcat(MAX_STRING_LENGTH, buf1, "@@b");
      safe_strcat(MAX_STRING_LENGTH, buf1, gclass_table[class].class_name);
      safe_strcat(MAX_STRING_LENGTH, buf1, "@@N\n\r\n\r");
   }

   safe_strcat(MAX_STRING_LENGTH, buf1, "@@rLv          @@lSpells@@N\n\r\n\r");

   for (level = 1; level < LEVEL_STAFF; level++)
   {

      col = 0;
      pSpell = TRUE;

      for (sn = 0; sn < MAX_SKILL; sn++)
      {
         if (skill_table[sn].name == NULL)
            break;
         if (skill_table[sn].skill_level[class] != level)
            continue;
         if ((adept_class) && (skill_get_tier(sn) == ADEPT))
         {

            if (skill_table[sn].skill_level[class] > MAX_ADEPT)
               continue;

            if (pSpell)
            {

               sprintf(buf, "@@r%2d:@@N", level);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
               pSpell = FALSE;
            }

            if (++col % 5 == 0)
               safe_strcat(MAX_STRING_LENGTH, buf1, "   ");

            if (can_use_skill(ch, sn))
            {
               sprintf(buf, "@@m%18s@@N", skill_table[sn].name);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            }
            else if (skill_table[sn].skill_level[class] > char_class_level(ch, class))
            {
               sprintf(buf, "@@d%18s@@N", skill_table[sn].name);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            }
            else
            {
               sprintf(buf, "@@a%18s@@N", skill_table[sn].name);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            }
            if (col % 4 == 0)
               safe_strcat(MAX_STRING_LENGTH, buf1, "\n\r");
         }
         else if ((remort_class) && (skill_get_tier(sn) == REMORT))
         {

            if (pSpell)
            {

               sprintf(buf, "@@r%2d:@@N", level);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
               pSpell = FALSE;
            }

            if (++col % 5 == 0)
               safe_strcat(MAX_STRING_LENGTH, buf1, "   ");

            if (can_use_skill(ch, sn))
            {
               sprintf(buf, "@@x@@m%18s@@N", skill_table[sn].name);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            }
            else if (skill_table[sn].skill_level[class] > char_class_level(ch, class))
            {
               sprintf(buf, "@@d%18s@@N", skill_table[sn].name);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            }
            else
            {
               sprintf(buf, "@@a%18s@@N", skill_table[sn].name);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            }
            if (col % 4 == 0)
               safe_strcat(MAX_STRING_LENGTH, buf1, "\n\r");
         }
         else if ((!remort_class) && (!adept_class) && (skill_get_tier(sn) == MORTAL))
         {
            if (pSpell)
            {
               sprintf(buf, "@@r%2d:@@N", level);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
               pSpell = FALSE;
            }

            if (++col % 5 == 0)
               safe_strcat(MAX_STRING_LENGTH, buf1, "   ");

            if (can_use_skill(ch, sn))
            {
               sprintf(buf, "@@x@@m%18s@@N", skill_table[sn].name);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            }
            else if (skill_table[sn].skill_level[class] > char_class_level(ch, class))
            {
               sprintf(buf, "@@d%18s@@N", skill_table[sn].name);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            }
            else
            {
               sprintf(buf, "@@a%18s@@N", skill_table[sn].name);
               safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            }
            if (col % 4 == 0)
               safe_strcat(MAX_STRING_LENGTH, buf1, "\n\r");
         }
      }

      if (col % 4 != 0)
         safe_strcat(MAX_STRING_LENGTH, buf1, "\n\r");
   }

   send_to_char(buf1, ch);
   return;
}

/* by passing the conf command - Kahn */
void do_autoexit(CHAR_DATA *ch, char *argument)
{
   (IS_SET(ch->config, CONFIG_AUTOEXIT) ? do_config(ch, "-autoexit") : do_config(ch, "+autoexit"));
}

void do_autoloot(CHAR_DATA *ch, char *argument)
{
   (IS_SET(ch->config, CONFIG_AUTOLOOT) ? do_config(ch, "-autoloot") : do_config(ch, "+autoloot"));
}

void do_autosac(CHAR_DATA *ch, char *argument)
{
   (IS_SET(ch->config, CONFIG_AUTOSAC) ? do_config(ch, "-autosac") : do_config(ch, "+autosac"));
}

void do_blank(CHAR_DATA *ch, char *argument)
{
   (IS_SET(ch->config, CONFIG_BLANK) ? do_config(ch, "-blank") : do_config(ch, "+blank"));
}

void do_brief(CHAR_DATA *ch, char *argument)
{
   (IS_SET(ch->config, CONFIG_BRIEF) ? do_config(ch, "-brief") : do_config(ch, "+brief"));
}

void do_combine(CHAR_DATA *ch, char *argument)
{
   (IS_SET(ch->config, CONFIG_COMBINE) ? do_config(ch, "-combine") : do_config(ch, "+combine"));
}

void do_pagelen(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   int lines;

   one_argument(argument, arg);

   if (arg[0] == '\0')
      lines = 20;
   else
      lines = atoi(arg);

   if (lines < 1)
   {
      send_to_char("Negative or Zero values for a page pause is not legal.\n\r", ch);
      return;
   }

   ch->pcdata->pagelen = lines;
   sprintf(buf, "Page pause set to %d lines.\n\r", lines);
   send_to_char(buf, ch);
   return;
}

void do_learned(CHAR_DATA *ch, char *argument)
{
   char buf[MSL], buf1[MSL];
   int col = 0;

   if (IS_NPC(ch))
   {
      send_to_char("Nuh uh naughty naughty!\n\r", ch);
      return;
   }

   sprintf(buf1, "Skills learned:\n\r");
   for (int i = 0; i < MAX_SKILL; i++)
   {
      if (can_use_skill(ch, i))
      {
         sprintf(buf, "@@W%16s-@@y%-7s@@g  ", skill_table[i].name,
                 learnt_name(i, ch->pcdata->learned[i]));

         safe_strcat(MAX_STRING_LENGTH, buf1, buf);
         if (++col % 3 == 0)
            safe_strcat(MAX_STRING_LENGTH, buf1, "\n\r");
      }
   }

   send_to_char(buf1, ch);
}

void do_auto(CHAR_DATA *ch, char *argument)
{

   do_config(ch, "");
   return;
}

void do_players(CHAR_DATA *ch, char *argument)
{

   send_to_char("This command is no longer needed, as 'WHO' carries all details.\n\r", ch);
   return;
}

void do_diagnose(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int pct;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Diagnose whom?\n\r", ch);
      return;
   }

   if ((victim = get_char_room(ch, arg)) == NULL)
   {
      send_to_char("They're not here.\n\r", ch);
      return;
   }

   if (can_see(victim, ch))
   {
      act("$n gives you the once-over.", ch, NULL, victim, TO_VICT);
      act("$n gives $N the once-over.", ch, NULL, victim, TO_NOTVICT);
   }
   else
   {
      send_to_char("They're not here.\n\r", ch);
      return;
   }

   if (get_max_hp(victim) > 0)
      pct = (100 * victim->hit) / get_max_hp(victim);
   else
      pct = -1;

   strcpy(buf, PERS(victim, ch));

   if (pct >= 100)
      safe_strcat(MAX_STRING_LENGTH, buf, " --  [5] 4  3  2  1   \n\r");
   else if (pct >= 80)
      safe_strcat(MAX_STRING_LENGTH, buf, " --   5 [4] 3  2  1   \n\r");
   else if (pct >= 60)
      safe_strcat(MAX_STRING_LENGTH, buf, " --   5  4 [3] 2  1   \n\r");
   else if (pct >= 40)
      safe_strcat(MAX_STRING_LENGTH, buf, " --   5  4  3 [2] 1   \n\r");
   else
      safe_strcat(MAX_STRING_LENGTH, buf, " --   5  4  3  2 [1]  \n\r");

   buf[0] = UPPER(buf[0]);
   send_to_char(buf, ch);

   return;
}

void do_heal(CHAR_DATA *ch, char *argument)
{
   /*
    * This function used when a player types heal when in a room with
    * * a mob with ACT_HEAL set.  Cost is based on the ch's level.
    * * -- Stephen
    */

   CHAR_DATA *mob;
   char buf[MAX_STRING_LENGTH];
   int mult; /* Multiplier used to calculate costs. */
   buf[0] = '\0';

   /*
    * Check for mob with act->heal
    */
   for (mob = ch->in_room->first_person; mob; mob = mob->next_in_room)
   {
      if (IS_NPC(mob) && IS_SET(mob->act, ACT_HEAL))
         break;
   }

   if (mob == NULL)
   {
      send_to_char("You can't do that here.\n\r", ch);
      return;
   }

   mult = UMAX(10, get_psuedo_level(ch) / 2);

   if (argument[0] == '\0')
   {
      /*
       * Work out costs of different spells.
       */
      send_to_char("Costs for spells:\n\r", ch);
      sprintf(buf, "Sanctuary:          %d.\n\r", mult * 100);
      send_to_char(buf, ch);
      sprintf(buf, "Heal:               %d.\n\r", mult * 50);
      send_to_char(buf, ch);
      sprintf(buf, "Invisibilty:        %d.\n\r", mult * 20);
      send_to_char(buf, ch);
      sprintf(buf, "Detect Invisibilty: %d.\n\r", mult * 10);
      send_to_char(buf, ch);
      sprintf(buf, "Refresh:            %d.\n\r", mult * 10);
      send_to_char(buf, ch);
      sprintf(buf, "Night Vision:       %d.\n\r", mult * 20);
      send_to_char(buf, ch);
      sprintf(buf, "Magical Dispel:     %d.\n\r", mult * 200);
      send_to_char(buf, ch);
      sprintf(buf, "Mana:     %d.\n\r", 1000);
      send_to_char(buf, ch);

      send_to_char("Type HEAL [S|H|I|D|R|N|M|P]\n\r", ch);
      send_to_char("Eg: 'HEAL H' will result in the heal spell being cast.\n\r", ch);
      send_to_char("\n\r**ALL** Spells will be cast on the buyer ONLY.\n\r", ch);
      return;
   }

   switch (UPPER(argument[0]))
   {
   case 'S': /* Sanc */
      if (ch->gold < (mult * 100))
      {
         send_to_char("You don't have enough money...\n\r", ch);
         return;
      }
      act("$N gestures towards $n.", ch, NULL, mob, TO_NOTVICT);
      act("$N gestures towards you.", ch, NULL, mob, TO_CHAR);
      spell_sanctuary(skill_lookup("sanc"), mult, ch, ch, NULL);
      ch->gold -= mult * 100;
      break;
   case 'P': /* mana */
      if (ch->gold < 1000)
      {
         send_to_char("You don't have enough money...\n\r", ch);
         return;
      }
      act("$N gestures towards $n.", ch, NULL, mob, TO_NOTVICT);
      act("$N gestures towards you.", ch, NULL, mob, TO_CHAR);
      ch->gold -= 1000;
      ch->mana = UMIN(get_max_mana(ch), ch->mana + 50);
      break;

   case 'H': /* Heal */
      if (ch->gold < (mult * 50))
      {
         send_to_char("You don't have enough money...\n\r", ch);
         return;
      }
      act("$N gestures towards $n.", ch, NULL, mob, TO_NOTVICT);
      act("$N gestures towards you.", ch, NULL, mob, TO_CHAR);
      spell_heal(skill_lookup("heal"), mult, mob, ch, NULL);
      ch->gold -= mult * 50;
      break;
   case 'I': /* invis */
      if (ch->gold < (mult * 20))
      {
         send_to_char("You don't have enough money...\n\r", ch);
         return;
      }
      act("$N gestures towards $n.", ch, NULL, mob, TO_NOTVICT);
      act("$N gestures towards you.", ch, NULL, mob, TO_CHAR);
      spell_invis(skill_lookup("invis"), mult, mob, ch, NULL);
      ch->gold -= mult * 20;
      break;
   case 'D': /* detect invis */
      if (ch->gold < (mult * 10))
      {
         send_to_char("You don't have enough money...\n\r", ch);
         return;
      }
      act("$N gestures towards $n.", ch, NULL, mob, TO_NOTVICT);
      act("$N gestures towards you.", ch, NULL, mob, TO_CHAR);
      spell_detect_invis(skill_lookup("detect invis"), mult, mob, ch, NULL);
      ch->gold -= mult * 10;
      break;
   case 'R': /* refresh */
      if (ch->gold < (mult * 10))
      {
         send_to_char("You don't have enough money...\n\r", ch);
         return;
      }
      act("$N gestures towards $n.", ch, NULL, mob, TO_NOTVICT);
      act("$N gestures towards you.", ch, NULL, mob, TO_CHAR);
      spell_refresh(skill_lookup("refresh"), mult, mob, ch, NULL);
      ch->gold -= mult * 10;
      break;
   case 'N': /* Infra */
      if (ch->gold < (mult * 20))
      {
         send_to_char("You don't have enough money...\n\r", ch);
         return;
      }
      act("$N gestures towards $n.", ch, NULL, mob, TO_NOTVICT);
      act("$N gestures towards you.", ch, NULL, mob, TO_CHAR);
      spell_infravision(skill_lookup("infra"), mult, ch, ch, NULL);
      ch->gold -= mult * 20;
      break;
   case 'M': /* dispel */
      if (ch->gold < (mult * 200))
      {
         send_to_char("You don't have enough money...\n\r", ch);
         return;
      }
      /*
       * No acts, as they are in spell_dispel_magic.  Doh.
       */
      spell_dispel_magic(skill_lookup("dispel magic"), mult * 5, mob, ch, NULL);
      ch->gold -= mult * 200;
      break;
   default:
      send_to_char("Are you sure you're reading the instructions right??\n\r", ch);
      return;
   }
   return;
}

void do_gain(CHAR_DATA *ch, char *argument)
{
   /*
    * Allow ch to gain a level in a chosen class.
    * * Only can be done at prac/train mob. -S-
    * * Now handles remort chars
    */

   CHAR_DATA *mob;
   char buf[MAX_STRING_LENGTH];
   long_int cost = 0;
   int cnt;
   int subpop;
   bool any;
   bool found = FALSE;
   int c; /* The class to gain in */
   bool remort = FALSE;
   bool adept = FALSE;
   sh_int morts_at_max = 0;
   sh_int remorts_at_max = 0;
   sh_int num_remorts = 0;
   bool allow_remort = FALSE;
   bool allow_adept = FALSE;

   buf[0] = '\0';

   if (IS_NPC(ch))
   {
      send_to_char("Hahaha, not for NPCs.\n\r", ch);
      return;
   }

   if (argument[0] == '\0')
   {
      do_worth(ch, argument);
      return;
   }

   /*
    * Check for mob with act->pac/train
    */
   for (mob = ch->in_room->first_person; mob; mob = mob->next_in_room)
   {
      if (IS_NPC(mob) && (IS_SET(mob->act, ACT_TRAIN) || IS_SET(mob->act, ACT_PRACTICE)))
         break;
   }

   if (mob == NULL)
   {
      send_to_char("You can't do that here.\n\r", ch);
      return;
   }

   for (cnt = 0; cnt < 4; cnt++)
      if (ch->mortal_class[cnt] >= 0 && ch->mortal_level[cnt] == MAX_MORTAL)
         morts_at_max++;
   for (cnt = 0; cnt < 2; cnt++)
   {
      if (ch->remort_class[cnt] >= 0)
         num_remorts++;
      if (ch->remort_class[cnt] >= 0 && ch->remort_level[cnt] == MAX_MORTAL)
         remorts_at_max++;
   }

   /* first case.. remort  */
   if (morts_at_max > 0 && num_remorts == 0)
      allow_remort = TRUE;

   /* Second case.. double remort */
   if (morts_at_max == MAX_PC_CLASS && remorts_at_max == 1 && num_remorts == 1)
      allow_remort = TRUE;

   /* third case..can adept */
   if (remorts_at_max >= 2 && !is_adept(ch))
   {
      allow_adept = TRUE;
      send_to_char("Adept allowed!\n\r", ch);
   }

   /*
    * If an argument supplied, make sure it's valid :P
    */

   any = FALSE;
   c = -1;

   for (cnt = 0; cnt < MAX_TOTAL_CLASS; cnt++)
   {
      if (!str_cmp(gclass_table[cnt].who_name, argument) ||
          !str_cmp(gclass_table[cnt].class_name, argument))
      {
         if (IS_MORTAL_CLASS(cnt))
         {
            found = TRUE;
            if (char_has_mortal_class(ch, cnt))
            {
               any = TRUE;
               c = cnt;
            }
         }
         else if (IS_REMORT_CLASS(cnt))
         {
            if (char_has_remort_class(ch, cnt) || allow_remort)
            {
               if (char_class_level(ch, gclass_table[cnt].prereq[0]) < MAX_MORTAL)
               {
                  send_to_char("You require level 100 in the mortal class before you can remort in "
                               "this class!\n\r",
                               ch);
                  continue;
               }

               any = TRUE;
               remort = TRUE;
               c = cnt;
            }
            found = TRUE;
         }
         else if (IS_ADEPT_CLASS(cnt))
         {
            if (ch->adept_class == cnt || allow_adept)
            {
               if (char_class_level(ch, gclass_table[cnt].prereq[0]) < MAX_MORTAL &&
                   char_class_level(ch, gclass_table[cnt].prereq[1]) < MAX_MORTAL)
               {
                  send_to_char("You need to be level 100 in at least one of this class's remort "
                               "prerequisites before you can adept!\n\r",
                               ch);
                  return;
               }

               any = TRUE;
               adept = TRUE;
               c = cnt;
            }
            found = TRUE;
         }
      }
   }

   if (!found)
   {
      send_to_char("That's not a class!\n\r", ch);
      return;
   }

   if (!any)
   {
      send_to_char("You are ineligible to level in this class.\n\r", ch);
      return;
   }

   /*
    * Ok, so now class should be valid.  Check if enough exp and valid
    */
   if (remort)
   {
      if (char_class_level(ch, gclass_table[c].prereq[0]) < MAX_MORTAL)
      {
         send_to_char("You are not ready to remort in this class yet, it requires level 100 in the "
                      "mortal.\n\r",
                      ch);
         return;
      }

      /* Can't take two remort classes that share the same mortal prerequisite */
      for (cnt = 0; cnt < 2; cnt++)
      {
         int rc = ch->remort_class[cnt];
         if (rc < 0 || rc == c)
            continue;
         if (gclass_table[rc].prereq[0] == gclass_table[c].prereq[0])
         {
            sprintf(buf, "You cannot remort in %s, you already have levels in %s.\n\r",
                    gclass_table[c].who_name, gclass_table[rc].who_name);
            send_to_char(buf, ch);
            return;
         }
      }
      cost = exp_to_level_remort(ch, c);
   }
   else if (adept)
   {
      if (ch->adept_level < 1)
         cost = 0;
      else
         cost = exp_to_level_adept(ch);
   }
   else
   {
      cost = exp_to_level(ch, c);
   }

   if (ch->exp < cost)
   {
      sprintf(buf, "Cost is %ld Exp.  You only have %ld (%ld short).\n\r", (long)cost, ch->exp,
              ((long)cost - ch->exp));
      send_to_char(buf, ch);
      return;
   }

   if ((adept) && (ch->adept_level < MAX_ADEPT))
   {
      send_to_char("@@WYou have reached another step on the stairway to Wisdom!!!@@N\n\r", ch);
      ch->exp -= cost;
      sprintf(buf, "%s @@W advances in the way of the Adept, %s!!\n\r", ch->name,
              gclass_table[c].class_name);
      info(buf, 1);
      free_string(ch->pcdata->who_name);
      ch->pcdata->who_name = str_dup(get_adept_name(ch));
      advance_level_adept(ch, c, TRUE);
      return;
   }
   else if (adept)
   {
      send_to_char("@@aYou peer down upon all the hapless mortals, knowing that you have reached "
                   "the final step upon the stairway of Wisdom.@@N\n\r",
                   ch);
      return;
   }

   if ((!adept && remort && char_class_level(ch, c) + 1 >= LEVEL_HERO) ||
       (!adept && !remort && char_class_level(ch, c) + 1 >= LEVEL_HERO))
   {
      send_to_char("If you wish to advance this class, please ask a Staff member.\n\r", ch);
      return;
   }
   /*
    * Check to see if able to reach new level
    */
   if (remort)
      sprintf(buf, "You gain a %s level!\n\r", gclass_table[c].class_name);
   else if (!adept)
      sprintf(buf, "You gain a %s level!\n\r", gclass_table[c].class_name);
   send_to_char(buf, ch);

   /*
    * Use info channel to inform of level gained!
    */

   if (remort)
      sprintf(buf, "%s advances in the way of the %s.", ch->name, gclass_table[c].class_name);
   else if (!adept)
      sprintf(buf, "%s advances in the way of the %s.", ch->name, gclass_table[c].class_name);
   info(buf, 1);

   if (remort)
      advance_level_remort(ch, c, TRUE);
   else
      advance_level(ch, c, TRUE);

   ch->exp -= cost;

   /*
    * Maintain ch->level as max level of the lot
    */
   for (subpop = 0; subpop < 4; subpop++)
   {
      if (ch->mortal_level[subpop] > ch->level)
         ch->level = ch->mortal_level[subpop];
   }
   for (subpop = 0; subpop < 2; subpop++)
   {
      if (ch->remort_level[subpop] > ch->level)
         ch->level = ch->remort_level[subpop];
   }
   do_save(ch, "");
   return;
}

void do_assassinate(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   int cost;
   CHAR_DATA *mob;
   CHAR_DATA *victim;
   buf[0] = '\0';

   /*
    * Check for mob with act->merc
    */
   for (mob = ch->in_room->first_person; mob; mob = mob->next_in_room)
   {
      if (IS_NPC(mob) && (IS_SET(mob->act, ACT_MERCENARY)))
         break;
   }

   if (mob == NULL)
   {
      send_to_char("There doesn't seem to be a mercenary around!\n\r", ch);
      return;
   }

   if (mob->hunting != NULL || mob->hunt_obj != NULL)
   {
      act("$N looks to be busy already!", ch, NULL, mob, TO_CHAR);
      return;
   }

   cost = (mob->level * 10000);

   if (argument[0] == '\0')
   {
      sprintf(buf, "$N tells you, 'The cost is %d GP.'", cost);
      act(buf, ch, NULL, mob, TO_CHAR);
      return;
   }

   if ((victim = get_char_world(ch, argument)) == NULL)
   {
      act("$N tells you, 'I can't locate the victim.  Sorry.'", ch, NULL, mob, TO_CHAR);
      return;
   }

   if (victim == ch)
      return;

   if (IS_NPC(victim))
   {
      act("$N tells you, 'I only go after REAL players, they're more fun!`", ch, NULL, mob,
          TO_CHAR);
      return;
   }

   if (victim->level < 12)
   {
      act("$N tells you, 'I'm not interested in small fry.'", ch, NULL, mob, TO_CHAR);
      return;
   }

   if (ch->gold < cost)
   {
      act("$N tells you, 'You can't afford my services!'", ch, NULL, mob, TO_CHAR);
      return;
   }

   if (!set_hunt(mob, ch, victim, NULL, HUNT_WORLD | HUNT_MERC, HUNT_INFORM | HUNT_CR))
   {
      act("$N tells you, 'I can't seem to find the target!'", ch, NULL, mob, TO_CHAR);
      return;
   }
   /*   if ( !make_hunt( mob, victim ) )
      {
         act( "$N tells you, 'I can't seem to find the target!'", ch, NULL, mob, TO_CHAR );
         return;
      }*/

   ch->gold -= cost;
   act("$n gives $N some gold coins.", ch, NULL, mob, TO_NOTVICT);
   act("$n says '$N shall die by my hand!`", mob, NULL, victim, TO_ROOM);
   sprintf(buf, "%s employs the services of %s to assassinate %s!!", ch->name, mob->short_descr,
           victim->name);
   info(buf, 1);
   return;
}

void do_alias(CHAR_DATA *ch, char *argument)
{
   /*
    * Handle aliases - setting and clearing, as well as listing.
    */

   int cnt;
   int alias_no;
   char buf[MAX_STRING_LENGTH];
   char arg1[MAX_STRING_LENGTH];
   char arg2[MAX_STRING_LENGTH];
   char arg3[MAX_STRING_LENGTH];

   buf[0] = '\0';

   if (IS_NPC(ch))
   {
      send_to_char("Not a chance!\n\r", ch);
      return;
   }
   smash_tilde(argument);
   if (argument[0] == '\0')
   {
      send_to_char("Defined Aliases:\n\r", ch);

      for (cnt = 0; cnt < MAX_ALIASES; cnt++)
      {
         sprintf(buf, "(%d) [Name:] %12s  [Aliases:] %s\n\r", cnt, ch->pcdata->alias_name[cnt],
                 ch->pcdata->alias[cnt]);
         send_to_char(buf, ch);
      }
      send_to_char("\n\rTo Set an Alias:\n\r", ch);
      send_to_char("ALIAS <num> <name> <alias>\n\r", ch);
      send_to_char("-enter 'CLEAR' as name to clear an alias.\n\r", ch);
      return;
   }

   argument = one_argument(argument, arg1); /* Number */
   argument = one_argument(argument, arg2); /* name   */
   strcpy(arg3, argument);                  /* alias  */

   if (arg3[0] == '\0' && str_cmp(arg2, "clear"))
   {
      send_to_char("\n\rTo Set an Alias:\n\r", ch);
      send_to_char("ALIAS <num> <name> <alias>\n\r", ch);
      send_to_char("-enter 'CLEAR' as name to clear an alias.\n\r", ch);
      return;
   }

   if (!is_number(arg1))
   {
      send_to_char("First argument must be an integer.\n\r", ch);
      return;
   }
   else
      alias_no = atoi(arg1);

   if (alias_no < 0 || alias_no > (MAX_ALIASES - 1))
   {
      sprintf(buf, "Valid alias numbers are 0 to %d.\n\r", MAX_ALIASES - 1);
      send_to_char(buf, ch);
      return;
   }

   if (!str_cmp("clear", arg2))
   {
      free_string(ch->pcdata->alias_name[alias_no]);
      free_string(ch->pcdata->alias[alias_no]);

      ch->pcdata->alias_name[alias_no] = str_dup("<none>");
      ch->pcdata->alias[alias_no] = str_dup("<none>");

      /*
       * Clear the alias (enter <none> for name and alias
       */
      return;
   }

   /*
    * Hopefully, now just set the (new) alias...
    */

   free_string(ch->pcdata->alias_name[alias_no]);
   free_string(ch->pcdata->alias[alias_no]);

   ch->pcdata->alias_name[alias_no] = str_dup(arg2);
   ch->pcdata->alias[alias_no] = str_dup(arg3);
   return;
}

void do_color(CHAR_DATA *ch, char *argument)
{
   /*
    * Allow users to set which color they get certain texts in. -S-
    */

   char buf[MAX_STRING_LENGTH];
   char arg1[MAX_STRING_LENGTH];
   char arg2[MAX_STRING_LENGTH];
   int col;
   int cnt;
   int ansi_number;
   int color_number;
   buf[0] = '\0';

   if (IS_NPC(ch))
      return;

   /*
    * First check to see if there is NO argument.  If so, display
    * * the current settings for players color.
    * * In this context, 'color' means the type of text, eg "say" or "shout"
    * *                  'ansi'   means the actual color
    * *                  ->Confusing, right?
    * * -S-
    */

   if (IS_NPC(ch))
      return;
   col = 0;

   if (argument[0] == '\0')
   {
      send_to_char("@@yPresent color Configuration:@@g\n\r\n\r", ch);

      for (cnt = 0; cnt < MAX_COLOR; cnt++)
      {
         sprintf(buf, "@@W%8s: %s%-12s@@N   ", color_table[cnt].name,
                 ansi_table[ch->pcdata->color[cnt]].value, ansi_table[ch->pcdata->color[cnt]].name);
         send_to_char(buf, ch);
         if (++col % 3 == 0)
            send_to_char("\n\r", ch);
      }
      if (col % 3 != 0)
         send_to_char("\n\r", ch);

      send_to_char("\n\r@@yAvailable colors:@@g\n\r", ch);

      col = 0;

      for (cnt = 0; cnt < MAX_ANSI; cnt++)
      {
         sprintf(buf, "%s%-12s@@N  ", IS_SET(ch->config, CONFIG_COLOR) ? ansi_table[cnt].value : "",
                 ansi_table[cnt].name);
         send_to_char(buf, ch);
         if (++col % 5 == 0)
            send_to_char("\n\r", ch);
      }
      if (col % 5 != 0)
         send_to_char("\n\r", ch);

      send_to_char("\n\rUSAGE: color <name> <color>\n\r", ch);
      send_to_char("Eg:     color say red\n\r", ch);
      send_to_char("OR: color highlighted/dimmed <color> for emphasized or dimmed text.\n\r", ch);

      send_to_char("NOTE: The 'stats' info covers who, score, etc.\n\r", ch);
      return;
   }

   argument = one_argument(argument, arg1); /* The name, eg 'say'   */
   argument = one_argument(argument, arg2); /* The color, eg 'red' */

   if (arg2[0] == '\0')
   {
      do_color(ch, ""); /* Generate message */
      return;
   }
   if (!str_prefix(arg1, "highlighted"))
      color_number = -2;
   else if (!str_prefix(arg1, "dimmed"))
      color_number = -3;
   else
   {
      /*
       * Check to see if the name is valid
       */
      color_number = -1;
      for (cnt = 0; cnt < MAX_COLOR; cnt++)
         if (!str_cmp(arg1, color_table[cnt].name))
            color_number = color_table[cnt].index;
   }

   if (color_number == -1)
   {
      /*
       * list possible choices
       */
      /*
       * do_color( ch, "help" );
       */
      return;
   }

   /*
    * color (the name) is ok.  Now find the ansi (the color)
    */
   ansi_number = -1;
   for (cnt = 0; cnt < MAX_ANSI; cnt++)
      if (!str_cmp(arg2, ansi_table[cnt].name))
         ansi_number = ansi_table[cnt].index;

   if (ansi_number == -1)
   {
      /*
       * list possible choice
       */
      /*
       * do_color( ch, "help" );
       */
      return;
   }

   /*
    * Ok now, we have color_number, which is the index to pcdata->color[]
    * * so we need to set the value of it to the color.
    * * -S-
    */

   if (color_number == -2)
   {
      ch->pcdata->hicol = ansi_table[ansi_number].letter;
      return;
   }
   else if (color_number == -3)
   {
      ch->pcdata->dimcol = ansi_table[ansi_number].letter;
      return;
   }

   ch->pcdata->color[color_number] = ansi_number;
   send_to_char("OK.\n\r", ch);
   return;
}

/* A simple, return the char sequence, function -S- */

char *color_string(CHAR_DATA *ch, char *argument)
{
   int cnt;
   int num;

   /*
    * if we don't want to send the string, return ""
    */
   /*
    * argument should be the string to find, eg "say"
    */

   if (IS_NPC(ch) || argument[0] == '\0')
      return ("");

   if (!IS_SET(ch->config, CONFIG_COLOR))
      return ("");

   if (!str_cmp(argument, "normal"))
      return ("\x1b[0m");

   /*
    * By here, ch is a PC and wants color
    */
   num = -1;
   for (cnt = 0; cnt < MAX_COLOR; cnt++)
      if (!str_cmp(argument, color_table[cnt].name))
         num = cnt;

   if (num == -1) /* bug report? */
      return ("");

   return (ansi_table[ch->pcdata->color[num]].value);
}

void do_worth(CHAR_DATA *ch, char *argument)
{
   /*
    * Show details regarding cost to level each class, etc
    */
   bool any = FALSE;
   char buf[MAX_STRING_LENGTH];
   long_int cost;
   int cnt;

   if (IS_NPC(ch))
   {
      send_to_char("Only for PCs.\n\r", ch);
      return;
   }

   send_to_char("Costs in Exp for you to level:\n\r", ch);
   send_to_char("Cost is shown first, followed by how much more exp you need.\n\r\n\r", ch);
   send_to_char("CLASS NAME:        COST:    DIFFERENCE:\n\r\n\r", ch);

   if (ch->adept_class >= 0 && ch->adept_level > 0 && ch->adept_level < MAX_ADEPT)
   {
      cost = exp_to_level_adept(ch);
      any = TRUE;
      sprintf(buf, "%-14s  %9ld %9ld.\n\r", gclass_table[ch->adept_class].who_name, (long)cost,
              (long)UMAX(0, cost - ch->exp));
      send_to_char(buf, ch);
   }

   for (cnt = 0; cnt < 4; cnt++)
   {
      if (ch->mortal_class[cnt] >= 0 && ch->mortal_level[cnt] < MAX_MORTAL)
      {
         any = TRUE;
         cost = exp_to_level(ch, ch->mortal_class[cnt]);
         sprintf(buf, "%-14s  %9ld %9ld.\n\r", gclass_table[ch->mortal_class[cnt]].who_name,
                 (long)cost, (long)UMAX(0, cost - ch->exp));
         send_to_char(buf, ch);
      }
   }

   /*
    * Check for remort classes
    */
   for (cnt = 0; cnt < 2; cnt++)
   {
      if (ch->remort_class[cnt] >= 0 && ch->remort_level[cnt] < MAX_MORTAL)
      {
         any = TRUE;
         cost = exp_to_level_remort(ch, ch->remort_class[cnt]);
         sprintf(buf, "%-14s  %9ld %9ld.\n\r", gclass_table[ch->remort_class[cnt]].who_name,
                 (long)cost, (long)UMAX(0, cost - ch->exp));
         send_to_char(buf, ch);
      }
   }

   if (!any)
      send_to_char("None to show!!\n\r", ch);

   return;
}

void do_whois(CHAR_DATA *ch, char *argument)
{
   /*
    * Show ch some details about the 'victim'
    * * Make sure ch can see victim!
    * * -S-
    */

   CHAR_DATA *victim;
   char buf[MAX_STRING_LENGTH];

   if ((victim = get_char_world(ch, argument)) == NULL)
   {
      send_to_char("No such player found.\n\r", ch);
      return;
   }

   if (!can_see(ch, victim))
   {
      send_to_char("No such player found.\n\r", ch);
      return;
   }

   if (IS_NPC(victim))
   {
      send_to_char("Not on NPCs.\n\r", ch);
      return;
   }

   /*
    * Ok, so now show the details!
    */
   sprintf(buf, "-=-=-=-=-=-=-=-=-=-=- %9s -=-=-=-=-=-=-=-=-=-=-\n\r", victim->name);
   if (IS_STAFF(victim))
   {
      sprintf(buf + strlen(buf), " [ %3s ]\n\r", victim->pcdata->who_name);
   }
   else if (is_adept(victim))
   {
      sprintf(buf + strlen(buf), " %s \n\r", get_adept_name(victim));
   }
   else
   {
      {
         int i;
         char class_buf[MAX_STRING_LENGTH];
         class_buf[0] = '\0';
         for (i = 0; i < 4; i++)
         {
            if (victim->mortal_class[i] >= 0)
            {
               char tmp[32];
               sprintf(tmp, " %s:%2d", gclass_table[victim->mortal_class[i]].who_name,
                       victim->mortal_level[i] > 0 ? victim->mortal_level[i] : 0);
               strcat(class_buf, tmp);
            }
         }
         sprintf(buf + strlen(buf), "Levels: [%s ]\n\r", class_buf);
         if (is_remort(victim))
         {
            class_buf[0] = '\0';
            for (i = 0; i < 2; i++)
            {
               if (victim->remort_class[i] >= 0)
               {
                  char tmp[32];
                  sprintf(tmp, " %s:%2d", gclass_table[victim->remort_class[i]].who_name,
                          victim->remort_level[i] > 0 ? victim->remort_level[i] : 0);
                  strcat(class_buf, tmp);
               }
            }
            sprintf(buf + strlen(buf), "Remort: [%s ]\n\r", class_buf);
         }
      }
   }
   sprintf(buf + strlen(buf), "Sex: %s.  Race: %s.  Clan: %s.\n\r",
           (victim->sex == SEX_MALE)     ? "Male"
           : (victim->sex == SEX_FEMALE) ? "Female"
                                         : "None",
           race_table[victim->race].race_name, clan_table[victim->pcdata->clan].clan_name);

   /*
    * if (victim->pcdata->pflags != 0)
    * sprintf( buf+strlen(buf), "Player is
    * %s\n\r",bit_table_lookup(tab_player_flags,victim->pcdata->pflags)); taken out to not show
    * vamps :P
    */
   if (IS_SET(victim->pcdata->pflags, PFLAG_PKOK))
      sprintf(buf + strlen(buf), "Player is @@ePKOK@@N\n\r");
   sprintf(buf + strlen(buf), "Players Killed: %d.  Times killed by players: %d.\n\r",
           victim->pcdata->pkills, victim->pcdata->pkilled);

   sprintf(buf + strlen(buf), "Mobs killed: %d.  Times killed by mobs: %d.\n\r",
           victim->pcdata->mkills, victim->pcdata->mkilled);

   if (IS_STAFF(victim))
   {
      sprintf(buf + strlen(buf), "%s is a Staff member.\r\n", victim->name);
   }
   /*
    * Description here, or email address?
    */

   sprintf(buf + strlen(buf), "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\r");
   send_to_char(buf, ch);
   return;
}

void do_shelp(CHAR_DATA *ch, char *argument)
{
   char search_term[MAX_INPUT_LENGTH];
   char full_argument[MAX_INPUT_LENGTH];

   parse_shelp_query(argument, search_term, sizeof(search_term), full_argument,
                     sizeof(full_argument));

   {
      char kw_buf[256];
      char text_buf[16384];
      int lev_out = 0;

      if (!db_shelp_lookup(full_argument, get_trust(ch), kw_buf, sizeof(kw_buf), text_buf,
                           sizeof(text_buf), &lev_out))
      {
         send_to_char("No help on that word.\n\r", ch);
         return;
      }

      if (lev_out >= 0)
      {
         send_to_char(kw_buf, ch);
         send_to_char("\n\r", ch);
      }

      if (text_buf[0] == '.')
         send_to_char(text_buf + 1, ch);
      else
         send_to_char(text_buf, ch);
   }
   return;
}

long get_room_lore_flags(CHAR_DATA *ch)
{
   CHAR_DATA *mob;
   long flags = 0;

   if (ch->in_room == NULL)
      return 0;

   for (mob = ch->in_room->first_person; mob != NULL; mob = mob->next_in_room)
   {
      if (IS_NPC(mob) && mob->lore_flags != 0)
         flags |= mob->lore_flags;
   }
   return flags;
}

void do_lore(CHAR_DATA *ch, char *argument)
{
   if (argument[0] == '\0')
      argument = "lore";

   {
      /* PCs always see only the default (unflagged) lore entry */
      long npc_flags = IS_NPC(ch) ? get_room_lore_flags(ch) : 0;
      char text_buf[16384];
      if (db_lore_lookup(argument, npc_flags, text_buf, sizeof(text_buf)))
      {
         if (text_buf[0] == '.')
            send_to_char(text_buf + 1, ch);
         else
            send_to_char(text_buf, ch);
         return;
      }
   }

   send_to_char("No lore on that subject.\n\r", ch);
   return;
}

void do_afk(CHAR_DATA *ch, char *argument)
{
   int value;

   if (IS_NPC(ch))
      return;

   value = table_lookup(tab_player_flags, "AFK");

   if (IS_SET(ch->pcdata->pflags, value))
   {
      REMOVE_BIT(ch->pcdata->pflags, value);
      send_to_char("AFK flag turned off.\n\r", ch);
   }
   else
   {
      SET_BIT(ch->pcdata->pflags, value);
      send_to_char("AFK flag turned on.\n\r", ch);
   }
   return;
}

void do_colist(CHAR_DATA *ch, char *argument)
{
   int col, n;
   char buf[MAX_STRING_LENGTH];

   if (IS_NPC(ch))
      return;

   send_to_char("@@Wcolor Codes: @@yTo use color codes within a string, use the following\n\r", ch);
   send_to_char("characters in sequence: @ @ <letter>.  Do not leave any spaces when you use\n\r",
                ch);
   send_to_char("the codes.  (Spaces are left above so that the characters are not interpreted\n\r",
                ch);
   send_to_char("as colors when you see this text!)@@g\n\r\n\r", ch);

   n = 0;

   for (col = 0; col < MAX_ANSI; col++)
   {
      sprintf(buf, "%c - %s%-14s@@N    ", ansi_table[col].letter,
              IS_SET(ch->config, CONFIG_COLOR) ? ansi_table[col].value : "", ansi_table[col].name);
      send_to_char(buf, ch);
      if (++n % 3 == 0)
         send_to_char("\n\r", ch);
   }
   if (n % 3 != 0)
      send_to_char("\n\r", ch);

   send_to_char("\n\r", ch);
   return;
}

void do_loot(CHAR_DATA *ch, char *argument)
{

   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *corpse;
   OBJ_DATA *obj = NULL;
   int counter, num;

   one_argument(argument, arg);

   if (IS_NPC(ch))
   {
      send_to_char("NPC's cannot loot corpses.\n\r", ch);
      return;
   }

   if (arg[0] == '\0')
   {
      send_to_char("Loot what?\n\r", ch);
      return;
   }

   if (!str_cmp(arg, "all") || !str_prefix("all.", arg))
   {
      send_to_char("You can't do that.\n\r", ch);
      return;
   }

   corpse = get_obj_room(ch, arg, ch->in_room->first_content);
   if (corpse == NULL)
   {
      act("I see no $T here.", ch, NULL, arg, TO_CHAR);
      return;
   }

   if (corpse->item_type == ITEM_CORPSE_NPC)
   {
      send_to_char("Just go ahead and take it.\n\r", ch);
      return;
   }

   if (corpse->item_type != ITEM_CORPSE_PC)
   {
      send_to_char("You cannot loot that.\n\r", ch);
      return;
   }
   /*
    * begin checking for lootability
    */

   if ((ch->pcdata->clan == 0) && (!IS_SET(ch->pcdata->pflags, PFLAG_PKOK)))
   {
      send_to_char("You cannot loot corpses.\n\r", ch);
      return;
   }

   if (corpse->value[3] == 0)
   {
      send_to_char("You cannot loot this corpse.\n\r", ch);
      return;
   }

   if ((ch->pcdata->clan == corpse->value[2]) ||
       ((IS_SET(ch->pcdata->pflags, PFLAG_PKOK)) && (corpse->value[0] == 1)))
   {
      counter = number_range(1, 100);

      if (counter >= 40)
      {
         if (corpse->first_in_carry_list == NULL)
         {
            send_to_char("There isn't anything in the corpse.\n\r", ch);
            return;
         }

         num = 0;
         for (obj = corpse->first_in_carry_list; obj != NULL; obj = obj->next_in_carry_list)
         {
            ++num;
         }

         counter = number_range(1, num);

         obj = corpse->first_in_carry_list;
         for (num = 1; num < counter; ++num)
         {
            obj = obj->next_in_carry_list;
         }

         if (!IS_SET(obj->extra_flags, ITEM_NOLOOT))
         {
            get_obj(ch, obj, corpse);

            /*
             * just incase...
             */
            if (ch->level > 1)
            {
               do_save(ch, "");
            }
         }
         else
         {
            send_to_char("You failed to loot the corpse.\n\r", ch);
            return;
         }

         corpse->value[3] = corpse->value[3] - 1;
         return;
      }
      else
      {
         send_to_char("You failed to loot the corpse.\n\r", ch);
         return;
      }
   }

   send_to_char("You cannot loot this corpse.\n\r", ch);
   return;
}

void do_finger(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   char name[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];
   bool found = FALSE;
   DESCRIPTOR_DATA d;
   DESCRIPTOR_DATA *this_d;

   argument = one_argument(argument, name);

   for (this_d = first_desc; this_d != NULL; this_d = this_d->next)
   {
      if ((this_d->connected > 0) && !str_cmp(this_d->character->name, name))
      {
         do_whois(ch, name);
         return;
      }
   }

   found = load_char_obj(&d, name, TRUE);

   if (!found)
   {
      sprintf(buf, "No pFile found for '%s'.\n\r", capitalize(name));
      send_to_char(buf, ch);
      return;
   }

   victim = d.character;
   d.character = NULL;
   victim->desc = NULL;

   sprintf(buf, "Name: %s.\n\r", capitalize(victim->name));
   send_to_char(buf, ch);

   sprintf(buf, "Last Login was from: %s.\n\r", victim->pcdata->host);
   send_to_char(buf, ch);

   sprintf(buf, "pFile was last saved at: %s.", victim->pcdata->lastlogin);
   send_to_char(buf, ch);

   free_char(victim);

   return;
}
