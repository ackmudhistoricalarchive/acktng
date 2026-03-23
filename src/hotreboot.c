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
 * Hotreboot (copyover) implementation.
 *
 * do_hotreboo()     - typo-trap command
 * do_hotreboot()    - saves all players and exec()s a new binary
 * copyover_recover() - restores player connections after exec()
 */

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <zlib.h>
#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#endif
#include <arpa/telnet.h>
#include "globals.h"
#include "socket.h"

extern int port, control;
extern FILE *fpReserve;

/* Copy the about-to-be-exec'd binary to ../cores/ack.hotreboot so the
 * startup script can pair it with any core produced by the new process
 * image.  Failures are logged but do not abort the hotreboot. */
static void snapshot_exe_for_coredump(const char *exe_path)
{
   char buf[4096];
   int src_fd, dst_fd;
   ssize_t nr, nw;

   src_fd = open(exe_path, O_RDONLY);
   if (src_fd < 0)
   {
      log_f("snapshot_exe_for_coredump: open(%s): %m", exe_path);
      return;
   }
   dst_fd = open("../cores/ack.hotreboot", O_WRONLY | O_CREAT | O_TRUNC, 0755);
   if (dst_fd < 0)
   {
      log_f("snapshot_exe_for_coredump: open ../cores/ack.hotreboot: %m");
      close(src_fd);
      return;
   }
   while ((nr = read(src_fd, buf, sizeof(buf))) > 0)
   {
      nw = write(dst_fd, buf, (size_t)nr);
      if (nw != nr)
      {
         log_f("snapshot_exe_for_coredump: short write");
         break;
      }
   }
   close(src_fd);
   close(dst_fd);
   log_f("snapshot_exe_for_coredump: saved %s -> ../cores/ack.hotreboot", exe_path);
}

/* Here it is boys and girls the HOT reboot function and all its nifty
 * little parts!! - Flar
 */
void do_hotreboo(CHAR_DATA *ch, char *argument)
{
   send_to_char("If you want to do a @@R@@fHOT@@Breboot@@N....spell it out.\n\r", ch);
   return;
}

void do_hotreboot(CHAR_DATA *ch, char *argument)
{
   FILE *fp;
   DESCRIPTOR_DATA *d, *d_next;
   char buf[MSL], buf2[MSL], buf3[MSL];

   fp = fopen(COPYOVER_FILE, "w");

   if (!fp)
   {
      send_to_char("Copyover file not writeable, aborted.\n\r", ch);
      log_f("Could not write to copyover file: %s", COPYOVER_FILE);
      perror("do_copyover:fopen");
      return;
   }

   if (auction_item != NULL)
      do_auction(ch, "stop");

   if (ch != NULL)
      sprintf(buf,
              "\n\r**** HOTreboot by %s - Please remain ONLINE ****\n\r*********** We will "
              "be back in 30 seconds!! *************\n\n\r",
              ch->name);
   else
      sprintf(buf, "\n\r**** HOTreboot: Automated Deployment - Please remain ONLINE "
                   "****\n\r*********** We will "
                   "be back in 30 seconds!! *************\n\n\r");

   /*
    * For each PLAYING descriptor( non-negative ), save its state
    */
   for (d = first_desc; d; d = d_next)
   {
      CHAR_DATA *och = CH(d);
      d_next = d->next; /* We delete from the list , so need to save this */

      if (!d->character || d->connected < 0) /* drop those logging on */
      {
         /* Use write_to_buffer so close_socket's process_output call applies
          * WebSocket framing for clients that upgraded to WebSocket. */
         write_to_buffer(d, "\n\r@Sorry, ACK! MUD rebooting. Come back in a few minutes.\n\r", 0);
         close_socket(d); /* throw'em out */
      }
      else
      {
         fprintf(fp, "%d %s %s %d\n", d->descriptor, och->name, d->host,
                 d->websocket_active ? 1 : 0);
         if (och->level == 1)
         {
            write_to_buffer(d,
                            "Since you are level one, and level one characters do not "
                            "save....you have been advanced!\n\r",
                            0);
            och->level = 2;
            och->class_level[och->class] = 2;
         }
         och->hit = och->max_hit;
         och->mana = och->max_mana;
         och->move = och->max_move;
         save_char_obj(och);
         /* Use write_to_buffer + process_output so WebSocket clients receive a
          * properly framed message and stay connected through the exec(). */
         write_to_buffer(d, buf, 0);
         process_output(d, FALSE);
      }
   }

   fprintf(fp, "-1\n");
   if (fp != NULL)
   {
      fclose(fp);
      fp = NULL;
   }

   /* Save linkdead players (in game but no active connection) */
   {
      CHAR_DATA *c;
      for (c = first_char; c; c = c->next)
      {
         if (!IS_NPC(c) && c->desc == NULL)
         {
            c->hit = c->max_hit;
            c->mana = c->max_mana;
            c->move = c->max_move;
            save_char_obj(c);
         }
      }
   }

   /* Persist corpses so they survive the exec() */
   save_corpses();

   /*
    * Close reserve and other always-open files and release other resources
    */

   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }

   /*
    * exec - descriptors are inherited
    */

   sprintf(buf, "%d", port);
   sprintf(buf2, "%d", control);
   strncpy(buf3, "-1", 100);

   /*
    * spec: handle profiling cleanly here
    */
#ifdef PROF
   if (!fork()) /* dump profile info */
      exit(0);
   signal(SIGPROF, SIG_IGN);
#endif

   /* Stop the virtual timer before exec. POSIX preserves interval timers
    * across execl but resets signal handlers to SIG_DFL. If the timer fires
    * during boot_db() in the new process (before init_alarm_handler installs
    * the SIGVTALRM handler), the default action kills the process.
    *
    * Two-step defence:
    *  1. SIG_IGN is preserved across execl, so any SIGVTALRM that is already
    *     pending in the kernel signal queue will be silently discarded when
    *     delivered to the new process.  init_alarm_handler() later overwrites
    *     this with the real handler.
    *  2. setitimer(zero) stops the repeating timer so no further signals are
    *     generated. */
   signal(SIGVTALRM, SIG_IGN);
   {
      struct itimerval zero_timer;
      memset(&zero_timer, 0, sizeof(zero_timer));
      setitimer(ITIMER_VIRTUAL, &zero_timer, NULL);
   }

   /* Tear down TLS and MCCP streams that can't survive exec() */
   {
      DESCRIPTOR_DATA *htls;
      for (htls = first_desc; htls; htls = htls->next)
      {
#ifdef HAVE_OPENSSL
         if (htls->tls_active && htls->ssl)
         {
            SSL_shutdown((SSL *)htls->ssl);
            SSL_free((SSL *)htls->ssl);
            htls->ssl = NULL;
            htls->tls_active = FALSE;
         }
#endif
         if (htls->mccp2_active && htls->mccp2_zout)
         {
            deflateEnd((z_stream *)htls->mccp2_zout);
            free(htls->mccp2_zout);
            htls->mccp2_zout = NULL;
            htls->mccp2_active = FALSE;
         }
         if (htls->mccp3_active && htls->mccp3_zin)
         {
            const unsigned char wont_mccp3[] = {IAC, WONT, TELOPT_MCCP3};
            write_to_descriptor(htls, (char *)wont_mccp3, 3);
            inflateEnd((z_stream *)htls->mccp3_zin);
            free(htls->mccp3_zin);
            htls->mccp3_zin = NULL;
            htls->mccp3_active = FALSE;
         }
      }
   }

   /* Resolve the absolute path of the running binary via /proc/self/exe so
    * hotreboot works regardless of how or from where the server was started.
    * Fall back to the compiled-in EXE_FILE path if readlink fails. */
   {
      char exe_path[PATH_MAX + 1];
      ssize_t exe_len = readlink("/proc/self/exe", exe_path, PATH_MAX);
      if (exe_len > 0 && exe_len < PATH_MAX)
      {
         exe_path[exe_len] = '\0';
         snapshot_exe_for_coredump(exe_path);
         execl(exe_path, exe_path, buf, "HOTreboot", buf2, buf3, (char *)NULL);
         log_f("do_hotreboot: execl(%s) failed: %m; retrying with EXE_FILE", exe_path);
      }
      else
         log_f("do_hotreboot: readlink /proc/self/exe failed (%m); falling back to EXE_FILE");
   }
   snapshot_exe_for_coredump(EXE_FILE);
   execl(EXE_FILE, EXE_FILE, buf, "HOTreboot", buf2, buf3, (char *)NULL);

   /*
    * Failed - sucessful exec will not return
    */

   perror("do_copyover: execl");
   log_f("do_hotreboot: execl failed: EXE_FILE=%s port=%s control=%s", EXE_FILE, buf, buf2);
   send_to_char("HOTreboot FAILED! Something is wrong in the shell!\n\r", ch);
}

/* Recover from a copyover - load players */
void copyover_recover()
{
   DESCRIPTOR_DATA *d;
   FILE *fp;
   char name[100];
   char host[MSL];
   int desc;
   int is_websocket;
   bool fOld;
   extern bool disable_timer_abort;
   log_f("Copyover recovery initiated");
   disable_timer_abort = TRUE;
   fp = fopen(COPYOVER_FILE, "r");

   if (!fp) /* there are some descriptors open which will hang forever then ? */
   {
      perror("copyover_recover:fopen");
      log_f("Copyover file not found. Exitting.\n\r");
      exit(1);
   }

   unlink(COPYOVER_FILE); /* In case something crashes - doesn't prevent reading   */

   for (;;)
   {
      is_websocket = 0;
      if (fscanf(fp, "%d %99s %8191s %d\n", &desc, name, host, &is_websocket) < 3)
         break;
      if (desc == -1)
         break;

      GET_FREE(d, desc_free);
      init_descriptor(d, desc); /* set up various stuff */

      /* Restore connection type and login flags before any output so that
       * WebSocket clients receive properly framed messages. */
      d->websocket_active = (is_websocket ? TRUE : FALSE);
      d->websocket_handshake_complete = TRUE;
      d->greeting_sent = TRUE;

      /* Liveness check — write something and verify it goes through.
       * Use write_to_buffer + process_output so WebSocket framing is applied. */
      write_to_buffer(d, "\n\rRestoring from HOTreboot...\n\r", 0);
      if (!process_output(d, FALSE))
      {
         /* Connection is dead; clean up manually — not yet in first_desc. */
         close(desc);
         if (d->outbuf)
            dispose(d->outbuf, d->outsize);
         PUT_FREE(d, desc_free);
         continue;
      }

      d->host = str_dup(host);
      d->next = NULL;
      d->prev = NULL;

      d->connected = CON_COPYOVER_RECOVER; /* -15, so close_socket frees the char */
      LINK(d, first_desc, last_desc, next, prev);

      /*
       * Now, find the pfile
       */

      log_f("copyover_recover: loading player '%s' on fd %d", name, desc);
      fOld = load_char_obj(d, name, FALSE);

      if (!fOld) /* Player file not found?! */
      {
         log_f("copyover_recover: player file not found for '%s'", name);
         write_to_buffer(
             d,
             "\n\rSomehow, your character was lost in the HOTreboot. Sorry, you must relog in.\n\r",
             0);
         close_socket(d);
      }
      else /* ok! */
      {
         CHAR_DATA *this_char;

         log_f("copyover_recover: player '%s' loaded, sending recovery message", name);
         write_to_buffer(d, "\n\rCopyover recovery complete.\n\r", 0);

         /*
          * Just In Case
          */
         if (!d->character->in_room)
            d->character->in_room = get_room_index(ROOM_VNUM_LIMBO);

         /*
          * Insert in the char_list
          */
         d->character->next = NULL;
         d->character->prev = NULL;
         this_char = d->character;

         LINK(this_char, first_char, last_char, next, prev);

         char_to_room(d->character, d->character->in_room);
         do_look(d->character, "");
         act("$n's atoms materialize and reform.", d->character, NULL, NULL, TO_ROOM);
         /*
          * d->connected = CON_PLAYING;
          */

         if (this_char->pcdata->hp_from_gain < 0)
            reset_gain_stats(this_char);
         /*       this_char->affected_by = 0;   */

         this_char->is_quitting = FALSE;
         d->connected = CON_SETTING_STATS;
         {
            OBJ_DATA *wear_object;
            AFFECT_DATA *this_aff;
            AFFECT_DATA *this_aff_next;

            this_char->max_mana = this_char->pcdata->mana_from_gain;
            this_char->max_hit = this_char->pcdata->hp_from_gain;
            this_char->max_move = this_char->pcdata->move_from_gain;
            this_char->saving_throw = get_psuedo_level(this_char) / 10;
            this_char->hitroll = 0;
            this_char->damroll = 0;
            this_char->armor = 100;
            if (this_char->login_sex != -1)
               this_char->sex = this_char->login_sex;
            this_char->affected_by = 0;

            for (wear_object = this_char->first_carry; wear_object != NULL;
                 wear_object = wear_object->next_in_carry_list)
            {
               if (wear_object->wear_loc > WEAR_NONE)
                  equip_char(this_char, wear_object, wear_object->wear_loc);
            }
            /* add spells saved to stats  */

            for (this_aff = this_char->first_saved_aff; this_aff != NULL; this_aff = this_aff_next)
            {
               this_aff_next = this_aff->next;
               UNLINK(this_aff, this_char->first_saved_aff, this_char->last_saved_aff, next, prev);

               if (this_aff->type == skill_lookup("fireshield"))
                  do_cast(this_char, "fireshield");
               else if (this_aff->type == skill_lookup("iceshield"))
                  do_cast(this_char, "iceshield");
               else if (this_aff->type == skill_lookup("shockshield"))
                  do_cast(this_char, "shockshield");
               else if (this_aff->type == skill_lookup("shadowshield"))
                  do_cast(this_char, "shadowshield");
               else if (this_aff->type == skill_lookup("thoughtshield"))
                  do_cast(this_char, "thoughtshield");
               else
                  affect_to_char(this_char, this_aff);
               PUT_FREE(this_aff, affect_free);
            }
         }
         d->connected = CON_PLAYING;
         list_who_to_output();
         log_f("copyover_recover: player '%s' fully recovered", name);
      }
   }

   if (fp != NULL)
   {
      fclose(fp);
      fp = NULL;
   }
   log_f("copyover_recover: all players recovered");
   disable_timer_abort = FALSE;
}

void do_cancelhotreboot(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *c;
   CHAR_DATA *c_next;
   char buf[MAX_STRING_LENGTH];

   if (hotreboot_countdown <= 0)
   {
      send_to_char("There is no automated hotreboot in progress.\n\r", ch);
      return;
   }

   hotreboot_countdown = 0;

   sprintf(buf, "Automated hotreboot cancelled by %s.", ch->name);
   log_string(buf);

   for (c = first_char; c != NULL; c = c_next)
   {
      c_next = c->next;
      if (!IS_NPC(c))
         send_to_char("\n\r@@e** The automated hotreboot has been cancelled. **@@N\n\r", c);
   }
}
