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
 * Login flow, character creation menus, and connection state machine (nanny).
 *
 * Public surface:
 *   is_parse_name_syntax_valid()  - name syntax check (unit-testable)
 *   is_login_name_format_valid()  - full name validation (unit-testable)
 *   comm_format_class_menu_line() - menu formatting helper (unit-testable)
 *   nanny()                       - main login state machine
 *   check_parse_name()            - name + mob-conflict check
 *   check_reconnect()             - link-dead reconnect
 *   check_playing()               - duplicate login guard
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#if defined(__CYGWIN__)
#include <crypt.h>
#endif
#include "globals.h"
#include "cursor.h"
#include "socket.h"
#include "prompt.h"
#include "db/db_worker.h"

/* Forward declarations for functions defined later in this file. */
bool check_parse_name(char *name);
bool check_reconnect(DESCRIPTOR_DATA *d, char *name, bool fConn);
bool check_playing(DESCRIPTOR_DATA *d, char *name);

/* ---------------------------------------------------------------------------
 * Name-validation helpers — no I/O dependencies, fully unit-testable.
 * --------------------------------------------------------------------------- */

bool is_parse_name_syntax_valid(const char *name)
{
   const char *pc;
   bool fIll = TRUE;
   size_t name_len;

   if (name == NULL)
      return FALSE;

   name_len = strlen(name);
   if (name_len < 3 || name_len > 12)
      return FALSE;

   for (pc = name; *pc != '\0'; pc++)
   {
      char c = (char)tolower((unsigned char)*pc);

      if (!isalpha((unsigned char)*pc))
         return FALSE;
      if (c != 'i' && c != 'l')
         fIll = FALSE;
   }

   return !fIll;
}

static bool is_reserved_login_name(const char *name)
{
   static const char *const reserved_names[] = {"all",      "auto",  "everymob", "localmobs",
                                                "immortal", "staff", "zen",      "self",
                                                "someone",  "tank",  "enemy"};
   size_t i;

   if (name == NULL || name[0] == '\0')
      return FALSE;

   for (i = 0; i < (sizeof(reserved_names) / sizeof(reserved_names[0])); i++)
   {
      if (strcasecmp(name, reserved_names[i]) == 0)
         return TRUE;
   }

   return FALSE;
}

bool is_login_name_format_valid(const char *name)
{
   if (is_reserved_login_name(name))
      return FALSE;

   return is_parse_name_syntax_valid(name);
}

void comm_format_class_menu_line(char *dest, size_t dest_size, const char *who_name,
                                 const char *stat, const char *class_name)
{
   if (dest == NULL || dest_size == 0)
      return;

   snprintf(dest, dest_size, "%3.3s    %3.3s    %-10.10s\n\r", who_name != NULL ? who_name : "",
            stat != NULL ? stat : "", class_name != NULL ? class_name : "");
}

/* ---------------------------------------------------------------------------
 * Everything below requires the full server environment.
 * --------------------------------------------------------------------------- */

#ifndef UNIT_TEST_LOGIN

static int count_playing_players(void)
{
   DESCRIPTOR_DATA *d;
   int playing_players = 0;

   for (d = first_desc; d != NULL; d = d->next)
   {
      if (d->connected == CON_PLAYING && d->character != NULL)
         playing_players++;
   }

   return playing_players;
}

static void normalize_login_class(CHAR_DATA *ch)
{
   if (ch == NULL || IS_NPC(ch))
      return;

   /* Ensure ch->class mirrors the prime mortal class slot. */
   if (ch->mortal_class[0] >= 0)
   {
      ch->class = (sh_int)ch->mortal_class[0];
      return;
   }

   /* Fallback for characters with no mortal_class set (very old or corrupt save). */
   ch->class = 0;
   ch->mortal_class[0] = 0;
   ch->mortal_level[0] = UMAX(ch->level, 1);
}

void show_menu_to(DESCRIPTOR_DATA *d)
{
   CHAR_DATA *ch = d->character;
   char buf[MAX_STRING_LENGTH];
   char menu[MAX_STRING_LENGTH];

   sprintf(menu, "\n\rCharacter Creation Menu.\n\r\n\r");
   strcat(menu, "Options:\n\r");

   sprintf(buf, "        1. Set Gender.       Currently:%s\n\r",
           !IS_SET(d->check, CHECK_SEX) ? "Not Set."
           : ch->sex == SEX_NEUTRAL     ? "Neutral."
           : ch->sex == SEX_MALE        ? "Male."
                                        : "Female.");
   strcat(menu, buf);

   sprintf(buf, "        2. Set Race.         Currently:%s\n\r",
           !IS_SET(d->check, CHECK_RACE) ? "Not Set." : race_table[ch->race].race_title);
   strcat(menu, buf);

   strcat(menu, "        3. Set Class.        Currently:");
   if (IS_SET(d->check, CHECK_CLASS))
   {
      int fubar;
      sprintf(buf, "\n\r        ");
      strcat(menu, buf);
      for (fubar = 0; fubar < 4; fubar++)
      {
         if (ch->mortal_class[fubar] >= 0)
         {
            strcat(menu, gclass_table[ch->mortal_class[fubar]].who_name);
            strcat(menu, ". ");
         }
      }
      strcat(menu, "\n\r");
   }
   else
      strcat(menu, "Not Set.\n\r");

   strcat(menu, "        4. Exit Creation Process.\n\r");

   strcat(menu, "\n\rPlease Select 1-4: ");
   write_to_buffer(d, menu, 0);
   return;
}

void show_smenu_to(DESCRIPTOR_DATA *d)
{
   CHAR_DATA *ch = d->character;
   char buf[MAX_STRING_LENGTH];
   char menu[MAX_STRING_LENGTH];

   sprintf(menu, "\n\rCharacter Creation: Gender.\n\r\n\r");

   strcat(menu, "Please Select:\n\r");
   strcat(menu, "              M : Male.\n\r");
   strcat(menu, "              F : Female.\n\r");
   strcat(menu, "              N : Neutral.\n\r\n\r");

   if (IS_SET(d->check, CHECK_SEX))
      sprintf(buf, "Current Choice: %s\n\r",
              ch->sex == SEX_NEUTRAL ? "Neutral."
              : ch->sex == SEX_MALE  ? "Male."
                                     : "Female.");
   else
      sprintf(buf, "No Current Selection.\n\r");

   strcat(menu, buf);
   strcat(menu, "\n\rPlease Select M/F/N: ");

   write_to_buffer(d, menu, 0);
   return;
}

void show_rmenu_to(DESCRIPTOR_DATA *d)
{
   char menu[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];
   int iRace;

   sprintf(menu, "\n\rCharacter Creation: Race.\n\r\n\r");
   strcat(menu, "Notes: a) Race determines abilities in different classes.\n\r");
   strcat(menu, "       b) Each Race will soon have seperate hometowns.\n\r");
   strcat(menu, "       c) Race determines your attributes.\n\r\n\r");
   strcat(menu, "Abr   Name             Class Order           Str Int Wis Dex Con\n\r");
   strcat(menu, "---   ----             ---------------       --- --- --- --- ---\n\r");

   for (iRace = 0; iRace < MAX_RACE; iRace++)
   {
      if (race_table[iRace].player_allowed == FALSE)
         continue;
      sprintf(buf, "%3s   %-10s    %s", race_table[iRace].race_name, race_table[iRace].race_title,
              class_order(iRace));
      strcat(menu, buf);

      sprintf(buf, "  %-2d  %-2d  %-2d  %-2d  %-2d\n\r", race_table[iRace].race_str,
              race_table[iRace].race_int, race_table[iRace].race_wis, race_table[iRace].race_dex,
              race_table[iRace].race_con);
      strcat(menu, buf);
   }

   strcat(menu, "\n\rPlease Select Your Race (Abr), or type the full race name for race info: ");
   write_to_buffer(d, menu, 0);
   return;
}

void show_ahelp_menu_to(DESCRIPTOR_DATA *d)
{

   char menu[MAX_STRING_LENGTH];
   sprintf(menu, "%s", "");
   strcat(menu, "Str affects items you can wear and weight you can carry, and your hitroll and "
                "damroll.\n\r");
   strcat(menu, "Int affects your mana gain, how many Npcs you can control effectively, and spell "
                "success.\n\r");
   strcat(menu,
          "Wis affects how many practices you get, your mana, and your saving against spells.\n\r");
   strcat(menu,
          "Dex affects your ac, how many items you can carry, and your ability to dodge.\n\r");
   strcat(menu, "Con affects how many hitpoints you gain per level.\n\r");
   strcat(menu, "\n\rPlease Select: (A)ccept, return to menu, (H)help stats, or (R)eroll: ");

   write_to_buffer(d, menu, 0);
   return;
}

void show_cmenu_to(DESCRIPTOR_DATA *d)
{
   char menu[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];
   int iClass;

   sprintf(menu, "Character Creation: Class Order.\n\r\n\r");
   strcat(menu, "This option allows you to select the order of your classes.\n\r");
   strcat(menu, "Being a MultiClass Mud, this order is very important, as it\n\r");
   strcat(menu, "will determine how easily you progress in each class, and\n\r");
   strcat(menu, "how well you can use the skills/spells of each class.\n\r");
   strcat(menu, "There are eight classes.  Please list, in order of best to\n\r");
   strcat(menu, "worst, the order your 4 classes will be.\n\r");
   strcat(menu, "(The 1st you pick will be your prime class, gaining a +1 bonus.\n\r");
   strcat(menu, "For example, psi mag cle cip.\n\r");
   strcat(menu, "Abr    Atr    Name\n\r");
   strcat(menu, "---    ---    ----------\n\r");

   for (iClass = 0; iClass < MAX_TOTAL_CLASS; iClass++)
   {
      if (!IS_MORTAL_CLASS(iClass))
         continue;
      char stat[MSL];
      strcpy(stat, stat_to_string(gclass_table[iClass].attr_prime));
      comm_format_class_menu_line(buf, sizeof(buf), gclass_table[iClass].who_name, stat,
                                  gclass_table[iClass].class_name);
      strcat(menu, buf);
   }
   strcat(menu, "\n\rOrder: ");
   write_to_buffer(d, menu, 0);
   return;
}

/*
 * finish_player_login -- post-load state machine advancement.
 * Called after a character has been loaded (synchronously or via DB async path).
 * fOld: TRUE if an existing player was loaded, FALSE for a new character.
 */
void finish_player_login(DESCRIPTOR_DATA *d, bool fOld)
{
   CHAR_DATA *ch = d->character;
   BAN_DATA *pban;

   if (IS_SET(ch->act, PLR_DENY))
   {
      sprintf(log_buf, "Denying access to %s@%s.", ch->name, d->host);
      log_string(log_buf);
      monitor_chan(log_buf, MONITOR_CONNECT);
      write_to_buffer(d, "You are denied access.\n\r", 0);
      close_socket(d);
      return;
   }

   /* TEMP FIX ZEN */
   if (IS_SET(ch->config, CONFIG_JUSTIFY))
      REMOVE_BIT(ch->config, CONFIG_JUSTIFY);

   if (check_reconnect(d, ch->name, FALSE))
   {
      fOld = TRUE;
   }
   else
   {
      if (wizlock && !IS_HERO(ch) && !ch->wizbit && !is_name(ch->name, sysdata.playtesters))
      {
         write_to_buffer(d, "\n\r             " mudnamenocolor " IS CURRENTLY WIZLOCKED.\n\r", 0);
         write_to_buffer(d, "Please Try Connecting Again In A Little While, When Any Problems\n\r",
                         0);
         write_to_buffer(d, "       We Are Working On Have Been Solved.  Thank You.\n\r", 0);
         close_socket(d);
         return;
      }
      if (deathmatch && !IS_HERO(ch) && !ch->wizbit)
      {
         write_to_buffer(d, "\n\r             " mudnamenocolor " IS CURRENTLY WIZLOCKED.\n\r", 0);
         write_to_buffer(
             d, "Sorry, The Players Connected At This Time Are Currently Participating\n\r", 0);
         write_to_buffer(d, "     In A DEATHMATCH Murder-Fest.  Please try later!\n\r", 0);
         write_to_buffer(d, "  Deathmatches are usually held on Thursdays and Sundays.  They\n\r",
                         0);
         write_to_buffer(d, " normally last about 30 minutes.  Please be patient!\n\r", 0);
         close_socket(d);
         return;
      }
      if (check_playing(d, ch->name))
         return;
   }

   if (fOld)
   {
      for (pban = first_ban; pban != NULL; pban = pban->next)
      {
         if (!str_prefix(pban->name, d->host) && (pban->newbie == FALSE))
         {
            char buf[MAX_STRING_LENGTH];
            sprintf(buf, "Denying access to banned site %s", d->host);
            monitor_chan(buf, MONITOR_CONNECT);
            write_to_descriptor(d, "Your site has been banned from this Mud.  BYE BYE!\n\r", 0);
            d->connected = CON_QUITTING;
            close_socket(d);
            return;
         }
      }

      /* Old player */
      write_to_buffer(d, "Password: ", 0);
      write_to_buffer(d, echo_off_str, 0);
      d->connected = CON_GET_OLD_PASSWORD;
   }
   else
   {
      /* New player */
      /* New characters with same name fix by Salem's Lot */
      if (check_playing(d, ch->name))
         return;

      for (pban = first_ban; pban != NULL; pban = pban->next)
      {
         if (!str_prefix(pban->name, d->host))
         {
            char buf[MAX_STRING_LENGTH];
            sprintf(buf, "Denying access to banned site %s", d->host);
            monitor_chan(buf, MONITOR_CONNECT);
            write_to_descriptor(d, "Your site has been banned from this Mud.  BYE BYE!\n\r", 0);
            d->connected = CON_QUITTING;
            close_socket(d);
            return;
         }
      }

      {
         char buf[MAX_STRING_LENGTH];
         sprintf(buf, "Did I get that right, %s (Y/N)? ", ch->name);
         write_to_buffer(d, buf, 0);
      }
      d->connected = CON_CONFIRM_NEW_NAME;
   }
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny(DESCRIPTOR_DATA *d, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char msg[MAX_STRING_LENGTH];
   CHAR_DATA *ch;
   char *pwdnew;
   char *p;
   int iClass;
   int lines;
   bool fOld;
   int cnt;
   NOTE_DATA *pnote;
   int notes;

   void trigger_happy_hour(void);

   while (isspace(*argument))
      argument++;

   ch = d->character;

   if (d->connected == CON_GET_NAME)
   {
      if (argument[0] == '\0')
      {
         close_socket(d);
         return;
      }

      argument[0] = UPPER(argument[0]);

      sprintf(buf, "%s provided as name from login from site %s.", argument, d->host);
      monitor_chan(buf, MONITOR_CONNECT);

      if (!check_parse_name(argument))
      {
         sprintf(buf, "Illegal name %s from site %s.", argument, d->host);
         monitor_chan(buf, MONITOR_CONNECT);
         write_to_buffer(d, "Illegal name, try another.\n\rName: ", 0);
         return;
      }

      if (!db_worker_failed)
      {
         alloc_char_for_login(d, argument);
         db_worker_enqueue_load_player(d, argument);
         return;
      }
      fOld = load_char_obj(d, argument, FALSE);
      finish_player_login(d, fOld);
      return;
   }

   if (d->connected == CON_GET_OLD_PASSWORD)
   {
      write_to_buffer(d, "\n\r", 2);
      if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd))
      {
         write_to_buffer(d, "Wrong password.\n\r", 0);
         sprintf(buf, "FAILED LOGIN for %s from site %s.", ch->name, d->host);
         monitor_chan(buf, MONITOR_CONNECT);
         ch->pcdata->failures++;
         save_char_obj(ch);
         close_socket(d);
         return;
      }

      write_to_buffer(d, echo_on_str, 0);

      if (check_reconnect(d, ch->name, TRUE))
         return;

      if (check_playing(d, ch->name))
         return;

      sprintf(log_buf, "%s has connected.", ch->name);
      monitor_chan(log_buf, MONITOR_CONNECT);

      sprintf(log_buf, "Site Name: %s.", d->host);
      monitor_chan(log_buf, MONITOR_CONNECT);

      log_string(log_buf);
      lines = ch->pcdata->pagelen;
      ch->pcdata->pagelen = 20;

      normalize_login_class(ch);

      /* Ensure the prime mortal slot has at least level 1 on old saves. */
      if (ch->mortal_class[0] >= 0 && ch->mortal_level[0] < 1)
         ch->mortal_level[0] = UMAX(ch->level, 1);

      if (IS_HERO(ch))
      {
         DL_LIST *brands;
         sh_int numbrands;
         char msgbuf[MSL];
         for (brands = first_brand, numbrands = 0; brands; brands = brands->next, numbrands++)
            ;
         do_help(ch, "staffmotd");
         sprintf(msgbuf, "There are currently %d outstanding brands.\n\r%s", numbrands,
                 ((numbrands < 50)
                      ? ""
                      : "@@eWarning: Process these brands immediately using staffbrand list, "
                        "staffbrand read, and staffbrand remove to avoid disk overflow!!@@N\n\r"));
         send_to_char(msgbuf, ch);
      }
      else
      {
         do_help(ch, "motd");
      }
      ch->pcdata->pagelen = lines;
      d->connected = CON_READ_MOTD;
   }

   if (d->connected == CON_CONFIRM_NEW_NAME)
   {
      switch (*argument)
      {
      case 'y':
      case 'Y':
         sprintf(buf, "New character.\n\rGive me a password for %s: %s", ch->name, echo_off_str);
         write_to_buffer(d, buf, 0);
         d->connected = CON_GET_NEW_PASSWORD;
         return;

      case 'n':
      case 'N':
         write_to_buffer(d, "Ok, what IS it, then? ", 0);
         free_char(d->character);
         d->character = NULL;
         d->connected = CON_GET_NAME;
         return;

      default:
         write_to_buffer(d, "Please type Yes or No? ", 0);
         return;
      }
      return;
   }

   if (d->connected == CON_GET_NEW_PASSWORD)
   {
      write_to_buffer(d, "\n\r", 2);

      if (strlen(argument) < 5)
      {
         write_to_buffer(d, "Password must be at least five characters long.\n\rPassword: ", 0);
         return;
      }

      pwdnew = crypt(argument, ch->name);
      for (p = pwdnew; *p != '\0'; p++)
      {
         if (*p == '~')
         {
            write_to_buffer(d, "New password not acceptable, try again.\n\rPassword: ", 0);
            return;
         }
      }

      free_string(ch->pcdata->pwd);
      ch->pcdata->pwd = str_dup(pwdnew);
      write_to_buffer(d, "Please retype password: ", 0);
      d->connected = CON_CONFIRM_NEW_PASSWORD;
      return;
   }

   if (d->connected == CON_CONFIRM_NEW_PASSWORD)
   {
      write_to_buffer(d, "\n\r", 2);

      if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd))
      {
         write_to_buffer(d, "Passwords don't match.\n\rRetype password: ", 0);
         d->connected = CON_GET_NEW_PASSWORD;
         return;
      }
      write_to_buffer(d, echo_on_str, 0);
      show_menu_to(d);
      d->connected = CON_MENU;
      return;
   }

   if (d->connected == CON_MENU)
   {
      int number;

      if (!is_number(argument))
      {
         write_to_buffer(d, "\n\rPlease Enter A Number.\n\r", 0);
         show_menu_to(d);
         return;
      }
      number = atoi(argument);
      if (number < 1 && number > 5)
      {
         write_to_buffer(d, "\n\rPlease Enter A Number Between 1 And 4.\n\r", 0);
         show_menu_to(d);
         return;
      }

      switch (number)
      {
      case 1:
         d->connected = CON_GET_NEW_SEX;
         show_smenu_to(d);
         break;
      case 2:
         d->connected = CON_GET_RACE;
         show_rmenu_to(d);
         break;
      case 3:
         d->connected = CON_GET_NEW_CLASS;
         show_cmenu_to(d);
         break;
      case 4:
         if (!IS_SET(d->check, CHECK_SEX) || !IS_SET(d->check, CHECK_CLASS) ||
             !IS_SET(d->check, CHECK_RACE))
         {
            write_to_buffer(d, "ALL Options Must Be Selected First.\n\r", 0);
            show_menu_to(d);
            return;
         }
         sprintf(log_buf, "%s@%s new player.", ch->name, d->host);
         log_string(log_buf);
         monitor_chan(log_buf, MONITOR_CONNECT);
         write_to_buffer(d, "\n\r", 2);
         ch->pcdata->pagelen = 20;

         /*
          * prompt already set?
          * ch->prompt = str_dup("<%hhp %mm %vmv> ");
          * old crash bug was b/c of prompts somehow
          */

         do_help(ch, "newun");
         d->connected = CON_READ_MOTD;
         /*
          * Display motd, and all other malarky
          */
         break;
      }
      return;
   }

   if (d->connected == CON_GET_NEW_SEX)
   {
      switch (argument[0])
      {
      case 'm':
      case 'M':
         ch->sex = SEX_MALE;
         ch->login_sex = SEX_MALE;
         break;
      case 'f':
      case 'F':
         ch->sex = SEX_FEMALE;
         ch->login_sex = SEX_FEMALE;
         break;
      case 'n':
      case 'N':
         ch->sex = SEX_NEUTRAL;
         ch->login_sex = SEX_NEUTRAL;
         break;
      default:
         write_to_buffer(d, "That's not a sex.\n\rWhat IS your sex? ", 0);
         show_smenu_to(d);
         return;
      }
      write_to_buffer(d, "\n\r\n\r", 0);
      if (!IS_SET(d->check, CHECK_SEX))
         SET_BIT(d->check, CHECK_SEX);
      d->connected = CON_MENU;
      show_menu_to(d);
      return;
   }

   if (d->connected == CON_GET_NEW_CLASS)
   {
      char arg[MAX_STRING_LENGTH];
      char *rest;
      int cnt;
      int chosen[4];
      int num_chosen = 0;
      bool dup;

      /* Reset class fields. */
      for (cnt = 0; cnt < 4; cnt++)
      {
         ch->mortal_class[cnt] = -1;
         ch->mortal_level[cnt] = 0;
      }

      /* Parse up to 4 class names from the input line. */
      rest = argument;
      while (num_chosen < 4 && rest[0] != '\0')
      {
         int match = -1;
         rest = one_argument(rest, arg);
         if (arg[0] == '\0')
            break;
         for (cnt = 0; cnt < MAX_TOTAL_CLASS; cnt++)
         {
            if (IS_MORTAL_CLASS(cnt) && (!str_cmp(arg, gclass_table[cnt].who_name) ||
                                         !str_cmp(arg, gclass_table[cnt].class_name)))
            {
               match = cnt;
               break;
            }
         }
         if (match == -1)
         {
            char errbuf[MAX_STRING_LENGTH];
            sprintf(errbuf,
                    "'%s' is not a valid class. Please try again.\n\r"
                    "Enter 4 class names in order, e.g. psi mag cle cip.\n\r",
                    arg);
            write_to_buffer(d, errbuf, 0);
            show_cmenu_to(d);
            return;
         }
         /* Check for duplicates in what we have so far. */
         dup = FALSE;
         for (cnt = 0; cnt < num_chosen; cnt++)
            if (chosen[cnt] == match)
            {
               dup = TRUE;
               break;
            }
         if (dup)
         {
            char errbuf[MAX_STRING_LENGTH];
            sprintf(errbuf, "You listed %s more than once. Please try again.\n\r",
                    gclass_table[match].class_name);
            write_to_buffer(d, errbuf, 0);
            show_cmenu_to(d);
            return;
         }
         chosen[num_chosen++] = match;
      }

      if (num_chosen < 4)
      {
         write_to_buffer(d, "Please choose exactly 4 classes in order, e.g. psi mag cle cip.\n\r",
                         0);
         show_cmenu_to(d);
         return;
      }

      for (cnt = 0; cnt < 4; cnt++)
         ch->mortal_class[cnt] = chosen[cnt];
      /* Prime class gets level 1; others start at 0. */
      ch->mortal_level[0] = 1;
      ch->class = (sh_int)ch->mortal_class[0];

      d->connected = CON_MENU;
      if (!IS_SET(d->check, CHECK_CLASS))
         SET_BIT(d->check, CHECK_CLASS);
      show_menu_to(d);
      return;
   }

   if (d->connected == CON_GET_RACE)
   {
      for (iClass = 0; iClass < MAX_RACE; iClass++)
      {
         if ((!str_cmp(argument, race_table[iClass].race_name)) &&
             (race_table[iClass].player_allowed == TRUE))
         {
            ch->race = iClass;
            break;
         }
         /* curtesy of Sylvana@LCN */
         else if (!str_cmp(argument, race_table[iClass].race_title))
         {
            do_help(ch, argument);
         }
      }

      if ((iClass == MAX_RACE) || (race_table[iClass].player_allowed == FALSE))
      {
         write_to_buffer(d, "Invalid Choice.\n\r", 0);
         show_rmenu_to(d);
         return;
      }

      if (!IS_SET(d->check, CHECK_RACE))
         SET_BIT(d->check, CHECK_RACE);
      show_menu_to(d);
      d->connected = CON_MENU;
      return;
   }

   if (d->connected == CON_READ_MOTD)
   {
      /*
       * Prime level idea dropped.  Give ch 1 level in their prime class.
       * mortal_level[0] is already set to 1 from CON_GET_NEW_CLASS for new chars.
       */

      LINK(ch, first_char, last_char, next, prev);
      d->connected = CON_PLAYING;
      list_who_to_output();

      if (IS_SET(ch->config, CONFIG_FULL_ANSI))
      {
         char scrollbuf[MSL];
         sprintf(scrollbuf, "%s%s%s%i;%ir%s%%i;%iH", CRS_RESET, CRS_CLS, CRS_CMD, 0,
                 ch->pcdata->term_rows - 12, CRS_CMD, ch->pcdata->term_rows - 13);
         send_to_char(scrollbuf, ch);
      }
      send_to_char("\n\rWelcome to " mudnamecolor ".  May your visit here be ... mutated.\n\r", ch);

      if (ch->level == 0)
      {
         /*
          * OBJ_DATA *obj; unused
          */

         switch (gclass_table[ch->class].attr_prime)
         {
         case APPLY_STR:
            ch->pcdata->max_str++;
            break;
         case APPLY_INT:
            ch->pcdata->max_int++;
            break;
         case APPLY_WIS:
            ch->pcdata->max_wis++;
            break;
         case APPLY_DEX:
            ch->pcdata->max_dex++;
            break;
         case APPLY_CON:
            ch->pcdata->max_con++;
            break;
         }

         ch->level = 1;
         /* mortal_class[] and mortal_level[0]=1 already set in CON_GET_NEW_CLASS. */

         ch->exp = 0;
         ch->hit = ch->max_hit;
         ch->mana = ch->max_mana;
         ch->move = ch->max_move;
         ch->pcdata->mana_from_gain = ch->max_mana;
         ch->pcdata->hp_from_gain = ch->max_hit;
         ch->pcdata->move_from_gain = ch->max_move;

         ch->pcdata->clan = 0; /* no clan */
         sprintf(buf, " needs a new title!");
         set_title(ch, buf);

         {
            char race_skill[MSL];
            char *race_skill_list;
            race_skill_list = race_table[ch->race].skill;
            for (;;)
            {
               race_skill_list = one_argument(race_skill_list, race_skill);
               int gsn = skill_lookup(race_skill);
               if (strlen(race_skill) < 1)
                  break;
               if (gsn <= 0)
                  break;
               ch->pcdata->learned[gsn] = 101;
            }
         }
         ch->deaf = 0;
         char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
         ch->pcdata->is_new_player = TRUE;
         {
            CHAR_DATA *mob;
            for (mob = ch->in_room->first_person; mob != NULL; mob = mob->next_in_room)
               if (IS_NPC(mob) && mob->spec_fun != NULL)
                  (*mob->spec_fun)(mob);
         }
         /*
          * Needed in case newbie drops an item.
          */

         /*
          * This seems to be causing a prob on meson right now.. byebye!
          *
          * obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0 );
          * obj_to_char( obj, ch );
          * equip_char( ch, obj, WEAR_LIGHT );
          *
          * obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0 );
          * obj_to_char( obj, ch );
          * equip_char( ch, obj, WEAR_BODY );
          *
          * obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0 );
          * obj_to_char( obj, ch );
          * equip_char( ch, obj, WEAR_SHIELD );
          *
          * obj = create_object( get_obj_index(gclass_table[ch->class].weapon),
          * 0 );
          * obj_to_char( obj, ch );
          * equip_char( ch, obj, WEAR_WIELD );
          */
      }
      else if (ch->in_room != NULL)
      {
         char_to_room(ch, ch->in_room);
      }
      else if (IS_STAFF(ch))
      {
         char_to_room(ch, get_room_index(ROOM_VNUM_CHAT));
      }
      else
      {
         char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
      }

      /*
       * check for login failures, then clear count.
       */
      if (ch->pcdata->failures != 0 && ch->level != 1)
      {
         sprintf(msg, "WARNING:  There have been %d failed login attempts.\n\r",
                 ch->pcdata->failures);
         send_to_char(msg, ch);
         ch->pcdata->failures = 0;
      }

      if (ch->level > 1)
      {
         sprintf(msg, "\n\rLast successful login from: %s\n\r\n\r", ch->pcdata->host);
         send_to_char(msg, ch);
         if (strcmp(d->host, ch->pcdata->host))
         {
            sprintf(msg, "%s connected from %s ( last login was from %s ) !", ch->name, d->host,
                    ch->pcdata->host);
            log_string(msg);
            monitor_chan(msg, MONITOR_CONNECT);
            if ((ch->level > 80))
            {
               sprintf(msg, "WARNING!!! %s logged in with level %d.\n\r", ch->name, ch->level);
               log_string(msg);
            }
         }
         if (ch->pcdata->host != NULL)
            free_string(ch->pcdata->host);
         ch->pcdata->host = str_dup(d->host);
      }

      /*
       * Check for new notes waiting at the post office
       */
      notes = 0;

      for (pnote = first_note; pnote; pnote = pnote->next)
         if (is_note_to(ch, pnote) && str_cmp(ch->name, pnote->sender) &&
             pnote->date_stamp > ch->last_note)
            notes++;

      if (notes == 1)
         send_to_char("\n\rYou have one new letter waiting.\n\r", ch);
      else if (notes > 1)
      {
         sprintf(buf, "\n\rYou have %d new letters waiting.\n\r", notes);
         send_to_char(buf, ch);
      }

      act("$n enters " mudnamecolor ".", ch, NULL, NULL, TO_ROOM);

      sprintf(buf, "%s has entered the game.", ch->name);
      monitor_chan(buf, MONITOR_CONNECT);

      {
         int playing_players = count_playing_players();

         if (playing_players > max_players)
         {
            max_players = playing_players;
            trigger_happy_hour();
            happy_hour_ticks_remaining = HAPPY_HOUR_AUTO_DURATION;
            info("New player record reached! Happy hour triggered for half an in-game day!", 1);
         }
      }

      if ((number_range(0, 99) < (ch->balance / 10000000)) &&
          (ch->balance > (get_psuedo_level(ch) * 100000)))
      {
         int loss;
         int new_balance;
         loss = number_range(ch->balance * .3, ch->balance * .6);
         new_balance = UMAX(ch->balance - loss, get_psuedo_level(ch) * 100000);
         ch->balance = UMIN(ch->balance, new_balance);
      }

      if (!IS_NPC(ch))
      {
         if (!str_cmp(ch->pcdata->room_enter, ""))
         {
            switch (ch->race)
            {

            case 0:
               ch->pcdata->room_enter = str_dup("saunters arrogantly in from");
               ch->pcdata->room_exit = str_dup("walks");
               break;

            case 1:
               ch->pcdata->room_enter = str_dup("sneaks in from");
               ch->pcdata->room_exit = str_dup("disappears");
               break;

            case 2:
               ch->pcdata->room_enter = str_dup("barges into you from");
               ch->pcdata->room_exit = str_dup("charges");
               break;

            case 3:
               ch->pcdata->room_enter = str_dup("quietly glides in from");
               ch->pcdata->room_exit = str_dup("glides quietly");
               break;

            case 4:
               ch->pcdata->room_enter = str_dup("appears in from");
               ch->pcdata->room_exit = str_dup("fades away");
               break;

            case 5:
               ch->pcdata->room_enter = str_dup("stomps angrily in from");
               ch->pcdata->room_exit = str_dup("angrily stomps");
               break;
            case 6:
               ch->pcdata->room_enter = str_dup("skitters in from");
               ch->pcdata->room_exit = str_dup("skitters");
               break;
            case 7:
               ch->pcdata->room_enter = str_dup("slithers in from");
               ch->pcdata->room_exit = str_dup("slithers");
               break;
            case 8:
               ch->pcdata->room_enter = str_dup("flys at you from");
               ch->pcdata->room_exit = str_dup("flys away");
               break;
            case 9:
               ch->pcdata->room_enter = str_dup("gallops in from");
               ch->pcdata->room_exit = str_dup("gallops");
               break;
            case 10:
               ch->pcdata->room_enter = str_dup("marches in from");
               ch->pcdata->room_exit = str_dup("marches");
               break;
            case 11:
               ch->pcdata->room_enter = str_dup("flitters in from");
               ch->pcdata->room_exit = str_dup("flitters");
               break;
            case 12:
               ch->pcdata->room_enter = str_dup("boars you down from");
               ch->pcdata->room_exit = str_dup("rushes");
               break;
            case 13:
               ch->pcdata->room_enter = str_dup("flys at you from");
               ch->pcdata->room_exit = str_dup("flys away");
               break;
            default:
               ch->pcdata->room_enter = str_dup("moves in from");
               ch->pcdata->room_exit = str_dup("moves");
               break;
            }
         }
      }

      if (ch->pcdata->hp_from_gain < 0)
         reset_gain_stats(ch);
      /*       ch->affected_by = 0;   */

      ch->is_quitting = FALSE;
      d->connected = CON_SETTING_STATS;
      {
         OBJ_DATA *wear_object;
         AFFECT_DATA *this_aff;
         AFFECT_DATA *this_aff_next;

         ch->max_mana = ch->pcdata->mana_from_gain;
         ch->max_hit = ch->pcdata->hp_from_gain;
         ch->max_move = ch->pcdata->move_from_gain;
         ch->saving_throw = get_psuedo_level(ch) / 10;
         ch->hitroll = 0;
         ch->damroll = 0;
         ch->armor = 100;
         if (ch->login_sex != -1)
            ch->sex = ch->login_sex;
         ch->affected_by = 0;

         for (wear_object = ch->first_carry; wear_object != NULL;
              wear_object = wear_object->next_in_carry_list)
         {
            if (wear_object->wear_loc > WEAR_NONE)
               equip_char(ch, wear_object, wear_object->wear_loc);
         }
         /* add spells saved to stats  */

         for (this_aff = ch->first_saved_aff; this_aff != NULL; this_aff = this_aff_next)
         {
            this_aff_next = this_aff->next;
            UNLINK(this_aff, ch->first_saved_aff, ch->last_saved_aff, next, prev);

            if (this_aff->type == skill_lookup("fireshield"))
               do_cast(ch, "fireshield");
            else if (this_aff->type == skill_lookup("iceshield"))
               do_cast(ch, "iceshield");
            else if (this_aff->type == skill_lookup("shockshield"))
               do_cast(ch, "shockshield");
            else if (this_aff->type == skill_lookup("shadowshield"))
               do_cast(ch, "shadowshield");
            else if (this_aff->type == skill_lookup("thoughtshield"))
               do_cast(ch, "thoughtshield");
            else
               affect_to_char(ch, this_aff);
            PUT_FREE(this_aff, affect_free);
         }
      }
      d->connected = CON_PLAYING;
      list_who_to_output();

      do_look(ch, "auto");
      /* v2 WebSocket: push initial equipment state on game entry */
      if (d->websocket_active)
         ws_send_equipment(d, ch);

      return;
   }

   return;
}

/*
 * Parse a name for acceptability.
 */
bool check_parse_name(char *name)
{
   if (!is_login_name_format_valid(name))
      return FALSE;

   /*
    * Prevent players from naming themselves after mobs.
    */
   {
      extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
      MOB_INDEX_DATA *pMobIndex;
      int iHash;

      for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
      {
         for (pMobIndex = mob_index_hash[iHash]; pMobIndex != NULL; pMobIndex = pMobIndex->next)
         {
            if (is_name(name, pMobIndex->player_name))
               return FALSE;
         }
      }
   }

   return TRUE;
}

/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect(DESCRIPTOR_DATA *d, char *name, bool fConn)
{
   CHAR_DATA *ch;
   OBJ_DATA *obj;

   for (ch = first_char; ch != NULL; ch = ch->next)
   {
      if (!IS_NPC(ch) && (!fConn || ch->desc == NULL) && !str_cmp(d->character->name, ch->name))
      {
         if (fConn == FALSE)
         {
            free_string(d->character->pcdata->pwd);
            d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
         }
         else
         {
            free_char(d->character);
            d->character = ch;
            ch->desc = d;
            ch->timer = 0;
            send_to_char("Reconnecting.\n\r", ch);
            act("$n reconnects.", ch, NULL, NULL, TO_ROOM);
            sprintf(log_buf, "%s@%s reconnected.", ch->name, d->host);
            log_string(log_buf);
            monitor_chan(log_buf, MONITOR_CONNECT);
            d->connected = CON_PLAYING;
            list_who_to_output();

            /*
             * Contributed by Gene Choi
             */
            if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL && obj->item_type == ITEM_LIGHT &&
                ch->in_room != NULL)
               ++ch->in_room->light;
         }
         return TRUE;
      }
   }

   return FALSE;
}

/*
 * Check if already playing.
 */
bool check_playing(DESCRIPTOR_DATA *d, char *name)
{
   DESCRIPTOR_DATA *dold;
   char buf[MAX_STRING_LENGTH];

   for (dold = first_desc; dold; dold = dold->next)
   {
      if (dold != d && dold->character != NULL && dold->connected != CON_GET_NAME &&
          dold->connected != CON_GET_OLD_PASSWORD &&
          !str_cmp(name, dold->original ? dold->original->name : dold->character->name))
      {
         sprintf(buf, "Player from site %s tried to login as %s (already playing) !", d->host,
                 name);
         monitor_chan(buf, MONITOR_CONNECT);
         /* Not sure if we want to do this..players can cheat and try to log back in as themselves
            to end a fight Zen dold->character->position = POS_STANDING; if (
            dold->character->fighting != NULL ) dold->character->fighting = NULL;
         */
         d->connected = CON_RECONNECTING;
         do_quit(dold->character, "");
         write_to_buffer(
             d,
             "Already playing. If you were not fighting or dead, you were disconnected\n\rName: ",
             0);
         d->connected = CON_GET_NAME;
         if (d->character != NULL)
         {
            free_char(d->character);
            d->character = NULL;
         }
         return TRUE;
      }
   }

   return FALSE;
}

#endif /* !UNIT_TEST_LOGIN */
