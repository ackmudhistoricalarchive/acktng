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

/*
 * save_areas.c — Area-level game state save.
 *
 * Saves persistent area-level data that survives server reboots:
 *   save_marks  — room marks/annotations
 *   save_bans   — site ban list
 */

#include "save.h"
#ifdef HAVE_LIBPQ
#include "../db/db_worker.h"
#endif

void save_marks()
{

   FILE *fp;
   char mark_file_name[MAX_STRING_LENGTH];
   MARK_LIST_MEMBER *mark_list;

   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }
   sprintf(mark_file_name, "%s", MARKS_FILE);

   if ((fp = fopen(mark_file_name, "w")) == NULL)
   {
      bug("Save Mark list: fopen", 0);
      perror("failed open of roommarks.lst in save_marks");
   }
   else
   {
      for (mark_list = first_mark_list; mark_list != NULL; mark_list = mark_list->next)
      {
         fprintf(fp, "#MARK~\n");
         fprintf(fp, "%d\n", mark_list->mark->room_vnum);
         fprintf(fp, "%s~\n", mark_list->mark->message);
         fprintf(fp, "%s~\n", mark_list->mark->author);
         fprintf(fp, "%d\n", mark_list->mark->duration);
         fprintf(fp, "%d\n", mark_list->mark->type);
      }
      fprintf(fp, "#END~\n\n");
   }

   fflush(fp);
   if (fp != NULL)
   {
      fclose(fp);
      fp = NULL;
   }

#ifdef HAVE_LIBPQ
   db_worker_save_room_marks(first_mark_list);
#endif
   return;
}

void save_bans()
{

   FILE *fp;
   char ban_file_name[MAX_STRING_LENGTH];
   BAN_DATA *pban;

   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }
   sprintf(ban_file_name, "%s", BANS_FILE);

   if ((fp = fopen(ban_file_name, "w")) == NULL)
   {
      bug("Save ban list: fopen", 0);
      perror("failed open of bans.lst in save_ban");
   }
   else
   {
      for (pban = first_ban; pban != NULL; pban = pban->next)
      {
         fprintf(fp, "#BAN~\n");
         fprintf(fp, "%d\n", (pban->newbie ? 1 : 0));
         fprintf(fp, "%s~\n", pban->name);
         fprintf(fp, "%s~\n", pban->banned_by);
      }
      fprintf(fp, "#END~\n\n");
   }

   fflush(fp);
   if (fp != NULL)
   {
      fclose(fp);
      fp = NULL;
   }

#ifdef HAVE_LIBPQ
   db_worker_save_bans(first_ban);
#endif
   return;
}

/*
 * Load marked rooms
 *
 */

void load_marks(void)
{

   FILE *marksfp;
   char marks_file_name[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];

   snprintf(marks_file_name, sizeof(marks_file_name), "%s", MARKS_FILE);

   db_format_status(buf, sizeof(buf), "Loading", marks_file_name);
   log_f("%s", buf);

   if ((marksfp = fopen(marks_file_name, "r")) == NULL)
   {
      log_f("Load marks Table: fopen");
      perror("failed open of marks_table.dat in load_marks_table");
   }
   else
   {
      fpArea = marksfp;
      db_set_area_name(marks_file_name);

      for (;;)
      {
         char *word;
         int rvnum = 0;
         MARK_DATA *mark;

         word = fread_string(marksfp);
         if (!str_cmp(word, "#MARK"))
         {
            GET_FREE(mark, mark_free);
            rvnum = fread_number(marksfp);
            mark->message = fread_string(marksfp);
            mark->author = fread_string(marksfp);
            mark->duration = fread_number(marksfp);
            mark->type = fread_number(marksfp);

            mark_to_room(rvnum, mark);
            free_string(word);
         }
         else if (!str_cmp(word, "#END"))
         {
            free_string(word);
            break;
         }
         else
         {
            free_string(word);
            log_f("Load_marks: bad section.");
            break;
         }
      }

      if (marksfp != NULL)
      {
         fclose(marksfp);
         marksfp = NULL;
      }
      // Since fpArea is just a pointer to the above marksfp, setting it to NULL should be enough -V
      fpArea = NULL;
      db_format_status(buf, sizeof(buf), "Done Loading", marks_file_name);
      monitor_chan(buf, MONITOR_CLAN);
   }
}

void load_bans(void)
{

   FILE *bansfp;
   char bans_file_name[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];

   snprintf(bans_file_name, sizeof(bans_file_name), "%s", BANS_FILE);
   db_format_status(buf, sizeof(buf), "Loading", bans_file_name);
   log_f("%s", buf);

   if ((bansfp = fopen(bans_file_name, "r")) == NULL)
   {
      log_f("Load bans Table: fopen");
      perror("failed open of bans_table.dat in load_bans_table");
   }
   else
   {
      fpArea = bansfp;
      db_set_area_name(bans_file_name);

      for (;;)
      {
         char *word;
         BAN_DATA *pban;
         word = fread_string(bansfp);
         if (!str_cmp(word, "#BAN"))
         {
            sh_int get_bool = FALSE;

            GET_FREE(pban, ban_free);
            get_bool = fread_number(bansfp);
            if (get_bool == 1)
               pban->newbie = TRUE;
            else
               pban->newbie = FALSE;
            pban->name = fread_string(bansfp);
            pban->banned_by = fread_string(bansfp);
            LINK(pban, first_ban, last_ban, next, prev);
            free_string(word);
         }
         else if (!str_cmp(word, "#END"))
         {
            free_string(word);
            break;
         }
         else
         {
            free_string(word);
            log_f("Load_bans: bad section.");
            break;
         }
      }

      if (bansfp != NULL)
      {
         fclose(bansfp);
         bansfp = NULL;
      }
      fpArea = NULL;
      db_format_status(buf, sizeof(buf), "Done Loading", bans_file_name);
      log_f("%s", buf);
   }
}
