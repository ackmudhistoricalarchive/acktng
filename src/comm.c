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
 * This file contains OS-dependent networking, the main game loop, and
 * the core player output system (send_to_char, show_string, act).
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 *
 * Login/character creation state machine lives in login.c.
 * Hotreboot implementation lives in hotreboot.c.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>
#if defined(__CYGWIN__)
#include <crypt.h>
#endif
/* For child forking and stuff */
#include <sys/wait.h>
#include <unistd.h> /* for execl */
#include "globals.h"
#include "cursor.h"
#include "socket.h"
#include "prompt.h"
#include "npc_dialogue.h"
#ifdef HAVE_LIBPQ
#include "db/db_worker.h"
#include "db/db_help.h"
#endif

void trigger_happy_hour(void);
void copyover_recover args((void));

/* ---- GSGP (Game Scry Game Protocol) support ---- */

#define GSGP_BOARD_MAX 10
#define GSGP_PLAYERS_MAX 200
#define GSGP_NAME_LEN 14 /* player names are max 12 chars; +2 for safety */
#define GSGP_BUF_MAX 8192
#define WHO_BUF_MAX 16384

static char gsgp_json_cache[GSGP_BUF_MAX];
static char wholist_html_cache[WHO_BUF_MAX];

const char *get_gsgp_json(void)
{
   return gsgp_json_cache;
}
const char *get_wholist_html(void)
{
   return wholist_html_cache;
}

struct gsgp_entry
{
   char name[GSGP_NAME_LEN];
   int level;
   int pkills;
   int qpts;
};

#ifndef UNIT_TEST_COMM

static int gsgp_by_level(const void *a, const void *b)
{
   return ((const struct gsgp_entry *)b)->level - ((const struct gsgp_entry *)a)->level;
}

static int gsgp_by_pkills(const void *a, const void *b)
{
   return ((const struct gsgp_entry *)b)->pkills - ((const struct gsgp_entry *)a)->pkills;
}

static int gsgp_by_qpts(const void *a, const void *b)
{
   return ((const struct gsgp_entry *)b)->qpts - ((const struct gsgp_entry *)a)->qpts;
}

/*
 * Global variables.
 */
FILE *fpReserve; /* Reserved file handle         */
bool god;        /* All new chars are gods!      */
bool merc_down;  /* Shutdown                     */
bool wizlock;    /* Game is wizlocked            */
bool deathmatch; /* Deathmatch happening?        */
char str_boot_time[MAX_INPUT_LENGTH];
time_t current_time; /* Time of this pulse           */

/* port and control moved from local to global for HOTreboot - Flar */
int port;
int control = -1;
int control_ws = -1;  /* plain WebSocket listen socket (e.g. nginx loopback) */
int control_wss = -1; /* native WSS listen socket */

int max_players = 0;
int hotreboot_countdown = 0; /* Pulses remaining until automatic hotreboot; 0 = inactive */

/* Write HTML-escaped version of src into dst.  Replaces < > & " ' with
 * their HTML entity equivalents to prevent XSS in the /who endpoint. */
static void html_escape_name(const char *src, char *dst, size_t dstsz)
{
   size_t out = 0;
   if (dstsz == 0)
      return;
   while (*src != '\0' && out + 7 < dstsz)
   {
      switch (*src)
      {
      case '<':
         memcpy(dst + out, "&lt;", 4);
         out += 4;
         break;
      case '>':
         memcpy(dst + out, "&gt;", 4);
         out += 4;
         break;
      case '&':
         memcpy(dst + out, "&amp;", 5);
         out += 5;
         break;
      case '"':
         memcpy(dst + out, "&quot;", 6);
         out += 6;
         break;
      case '\'':
         memcpy(dst + out, "&#39;", 5);
         out += 5;
         break;
      default:
         dst[out++] = *src;
         break;
      }
      src++;
   }
   dst[out] = '\0';
}

/* Write JSON-escaped version of src into dst.  Escapes backslash,
 * double-quote, and ASCII control characters to prevent JSON injection
 * in the /gsgp endpoint. */
static void json_escape_name(const char *src, char *dst, size_t dstsz)
{
   size_t out = 0;
   if (dstsz == 0)
      return;
   while (*src != '\0' && out + 7 < dstsz)
   {
      unsigned char c = (unsigned char)*src;
      if (c == '"' || c == '\\')
      {
         dst[out++] = '\\';
         dst[out++] = (char)c;
      }
      else if (c < 0x20)
      {
         snprintf(dst + out, dstsz - out, "\\u%04x", c);
         out += 6;
      }
      else
      {
         dst[out++] = (char)c;
      }
      src++;
   }
   dst[out] = '\0';
}

static void write_gsgp_board(char *buf, size_t bufsz, size_t *off, const char *board_name,
                             struct gsgp_entry *arr, int n, int (*cmp)(const void *, const void *),
                             bool skip_zero, bool last)
{
   struct gsgp_entry sorted[GSGP_PLAYERS_MAX];
   int i, written = 0, r;

   if (n > GSGP_PLAYERS_MAX)
      n = GSGP_PLAYERS_MAX;
   memcpy(sorted, arr, (size_t)n * sizeof(struct gsgp_entry));
   qsort(sorted, (size_t)n, sizeof(struct gsgp_entry), cmp);

   r = snprintf(buf + *off, bufsz - *off, "    {\"name\":\"%s\",\"entries\":[", board_name);
   if (r > 0)
      *off += (size_t)r;
   for (i = 0; i < n && written < GSGP_BOARD_MAX; i++)
   {
      int val = (cmp == gsgp_by_level)    ? sorted[i].level
                : (cmp == gsgp_by_pkills) ? sorted[i].pkills
                                          : sorted[i].qpts;
      if (skip_zero && val == 0)
         continue;
      if (written > 0)
      {
         r = snprintf(buf + *off, bufsz - *off, ",");
         if (r > 0)
            *off += (size_t)r;
      }
      {
         char safe_name[128];
         json_escape_name(sorted[i].name, safe_name, sizeof(safe_name));
         r = snprintf(buf + *off, bufsz - *off, "{\"name\":\"%s\",\"value\":%d}", safe_name, val);
      }
      if (r > 0)
         *off += (size_t)r;
      written++;
   }
   r = snprintf(buf + *off, bufsz - *off, "]}");
   if (r > 0)
      *off += (size_t)r;
   if (!last)
   {
      r = snprintf(buf + *off, bufsz - *off, ",");
      if (r > 0)
         *off += (size_t)r;
   }
   r = snprintf(buf + *off, bufsz - *off, "\n");
   if (r > 0)
      *off += (size_t)r;
}

static void write_gsgp_data(int online_count, struct gsgp_entry *entries, int n)
{
   size_t off = 0;
   int r;

   r = snprintf(gsgp_json_cache + off, GSGP_BUF_MAX - off, "{\n");
   if (r > 0)
      off += (size_t)r;
   r = snprintf(gsgp_json_cache + off, GSGP_BUF_MAX - off, "  \"name\":\"ACK!MUD TNG\",\n");
   if (r > 0)
      off += (size_t)r;
   r = snprintf(gsgp_json_cache + off, GSGP_BUF_MAX - off, "  \"active_players\":%d,\n",
                online_count);
   if (r > 0)
      off += (size_t)r;
   r = snprintf(gsgp_json_cache + off, GSGP_BUF_MAX - off, "  \"leaderboards\":[\n");
   if (r > 0)
      off += (size_t)r;
   write_gsgp_board(gsgp_json_cache, GSGP_BUF_MAX, &off, "Top Players by Level", entries, n,
                    gsgp_by_level, FALSE, FALSE);
   write_gsgp_board(gsgp_json_cache, GSGP_BUF_MAX, &off, "Top PKillers", entries, n, gsgp_by_pkills,
                    TRUE, FALSE);
   write_gsgp_board(gsgp_json_cache, GSGP_BUF_MAX, &off, "Top Quest Point Earners", entries, n,
                    gsgp_by_qpts, FALSE, TRUE);
   r = snprintf(gsgp_json_cache + off, GSGP_BUF_MAX - off, "  ]\n");
   if (r > 0)
      off += (size_t)r;
   snprintf(gsgp_json_cache + off, GSGP_BUF_MAX - off, "}\n");
}

void list_who_to_output(void)
{
   DESCRIPTOR_DATA *d;
   struct gsgp_entry gsgp_entries[GSGP_PLAYERS_MAX];
   size_t off = 0;
   int r;
   int online_count = 0;
   int gsgp_n = 0;

   r = snprintf(wholist_html_cache + off, WHO_BUF_MAX - off, "<h2>Players Online</h2>\n");
   if (r > 0)
      off += (size_t)r;
   r = snprintf(wholist_html_cache + off, WHO_BUF_MAX - off, "<ul>\n");
   if (r > 0)
      off += (size_t)r;

   for (d = first_desc; d != NULL; d = d->next)
   {
      CHAR_DATA *who_ch;

      if (d->connected != CON_PLAYING)
         continue;

      who_ch = (d->original != NULL) ? d->original : d->character;
      if (who_ch == NULL || IS_NPC(who_ch))
         continue;

      {
         char safe_name[128];
         html_escape_name(who_ch->name, safe_name, sizeof(safe_name));
         r = snprintf(wholist_html_cache + off, WHO_BUF_MAX - off, "<li>%s</li>\n", safe_name);
      }
      if (r > 0)
         off += (size_t)r;
      online_count++;

      if (gsgp_n < GSGP_PLAYERS_MAX)
      {
         strncpy(gsgp_entries[gsgp_n].name, who_ch->name, GSGP_NAME_LEN - 1);
         gsgp_entries[gsgp_n].name[GSGP_NAME_LEN - 1] = '\0';
         gsgp_entries[gsgp_n].level = who_ch->level;
         gsgp_entries[gsgp_n].pkills = who_ch->pcdata->pkills;
         gsgp_entries[gsgp_n].qpts = who_ch->quest_points;
         gsgp_n++;
      }
   }

   snprintf(wholist_html_cache + off, WHO_BUF_MAX - off, "</ul>\n");

   write_gsgp_data(online_count, gsgp_entries, gsgp_n);
}

int main(int argc, char **argv)
{
   struct timeval now_time;
   bool fCopyOver = FALSE; /* HOTreboot??? Well is it...is it???? - Flar */
   extern int abort_threshold;
   int ws_port = -1;
   int wss_port = -1;
   int control_tls = -1;
   int tls_port = -1;
   int control_sniff = -1;
   int sniff_port = -1;
   int control_http = -1;
   int http_port = 8080;
   int flag_start = 1; /* argv index where flag parsing begins */
#ifdef HAVE_OPENSSL
   const char *tls_cert = "../data/tls/cert.pem";
   const char *tls_key = "../data/tls/key.pem";
#endif

   /*
    * Init time.
    */
   gettimeofday(&now_time, NULL);
   current_time = (time_t)now_time.tv_sec;
   strcpy(str_boot_time, ctime(&current_time));

   /*
    * Get the port number.
    *
    * The plain-telnet port is optional.  When omitted, the server opens no
    * plain-telnet listening socket; pass --sniff-port or --tls-port instead.
    *
    * HOTreboot exec format: <exe> <port|-1> HOTreboot <control_fd> <ws_fd>
    * Detect HOTreboot first so port=-1 round-trips without a validation error.
    */
   port = -1; /* -1 = no plain-telnet socket */

   if (argc > 2 && argv[2] && !strcmp(argv[2], "HOTreboot"))
   {
      /* HOTreboot path: inherit listen sockets, no flag parsing needed.
       * Format: <exe> <port> HOTreboot <control_fd> <control_ws_fd> <control_wss_fd> */
      fCopyOver = TRUE;
      port = atoi(argv[1]);    /* may be -1 */
      control = atoi(argv[3]); /* inherited plain-telnet fd */
      if (argc > 4)
         control_ws = atoi(argv[4]); /* inherited plain-WS fd (nginx loopback) */
      if (argc > 5)
         control_wss = atoi(argv[5]); /* inherited native-WSS fd */
      flag_start = argc;              /* skip flags */
   }
   else if (argc > 1 && is_number(argv[1]) && argv[1][0] != '-')
   {
      /* Normal start with explicit plain-telnet port */
      port = atoi(argv[1]);
      if (port <= 1024)
      {
         fprintf(stderr, "Port number must be above 1024.\n");
         exit(1);
      }
      flag_start = 2;
   }
   else if (argc > 1 && argv[1][0] != '-')
   {
      /* Non-numeric, non-flag first argument */
      fprintf(stderr,
              "Usage: %s [port] [--sniff-port N] [--tls-port N] [--ws-loopback N] [--wss-port N]\n",
              argv[0]);
      exit(1);
   }
   /* else: no positional arg – port stays -1, flag_start stays 1 */

   /*
    * Parse --ws-loopback <port>: open a second socket for WebSocket
    * connections forwarded by a TLS proxy (e.g. nginx).
    */
   {
      int i;
      for (i = flag_start; i < argc; i++)
      {
         if (!strcmp(argv[i], "--ws-loopback") && i + 1 < argc)
         {
            ws_port = atoi(argv[++i]);
            if (ws_port <= 1024)
            {
               fprintf(stderr, "--ws-loopback port must be above 1024.\n");
               exit(1);
            }
         }
         else if (!strcmp(argv[i], "--wss-port") && i + 1 < argc)
         {
            wss_port = atoi(argv[++i]);
            if (wss_port <= 1024)
            {
               fprintf(stderr, "--wss-port must be above 1024.\n");
               exit(1);
            }
         }
         else if (!strcmp(argv[i], "--tls-port") && i + 1 < argc)
         {
            tls_port = atoi(argv[++i]);
            if (tls_port <= 1024)
            {
               fprintf(stderr, "--tls-port must be above 1024.\n");
               exit(1);
            }
         }
         else if (!strcmp(argv[i], "--sniff-port") && i + 1 < argc)
         {
            sniff_port = atoi(argv[++i]);
            if (sniff_port <= 1024)
            {
               fprintf(stderr, "--sniff-port must be above 1024.\n");
               exit(1);
            }
         }
         else if (!strcmp(argv[i], "--http-port") && i + 1 < argc)
         {
            http_port = atoi(argv[++i]);
            if (http_port <= 0)
            {
               fprintf(stderr, "--http-port must be a positive integer.\n");
               exit(1);
            }
         }
         else if (!strcmp(argv[i], "--no-http"))
            http_port = -1;
#ifdef HAVE_OPENSSL
         else if (!strcmp(argv[i], "--tls-cert") && i + 1 < argc)
            tls_cert = argv[++i];
         else if (!strcmp(argv[i], "--tls-key") && i + 1 < argc)
            tls_key = argv[++i];
#endif
      }
   }

   /* Require at least one listening port */
   if (!fCopyOver && port <= 0 && sniff_port <= 0 && tls_port <= 0 && ws_port <= 0 && wss_port <= 0)
   {
      fprintf(stderr,
              "Error: no listening port.  Specify a port number or --sniff-port / --tls-port.\n");
      exit(1);
   }

   rename("../log/comlog.old", "../log/comlog.crash");
   rename("../log/comlog.txt", "../log/comlog.old");

   /*
    * Run the game.
    */
   if (!fCopyOver) /* We already have the port if Copyovered. */
   {
      if (port > 0)
         control = init_socket(port, INADDR_ANY);
      if (ws_port > 0)
         control_ws = init_socket(ws_port, INADDR_ANY);
      if (http_port > 0)
         control_http = init_socket(http_port, INADDR_ANY);
      if (tls_port > 0 || sniff_port > 0 || wss_port > 0)
      {
#ifdef HAVE_OPENSSL
         bool tls_ctx_ok = init_tls_context(tls_cert, tls_key);
         if (tls_port > 0)
         {
            if (tls_ctx_ok)
               control_tls = init_socket(tls_port, INADDR_ANY);
            else
               fprintf(stderr, "Warning: TLS context init failed; --tls-port ignored.\n");
         }
         if (sniff_port > 0)
         {
            if (!tls_ctx_ok)
               fprintf(stderr,
                       "Warning: TLS context init failed; sniff port will serve plain only.\n");
            control_sniff = init_socket(sniff_port, INADDR_ANY);
         }
         if (wss_port > 0)
         {
            if (tls_ctx_ok)
               control_wss = init_socket(wss_port, INADDR_ANY);
            else
               fprintf(stderr, "Warning: TLS context init failed; --wss-port ignored.\n");
         }
#else
         if (tls_port > 0)
            fprintf(stderr, "Warning: OpenSSL not compiled in; --tls-port ignored.\n");
         if (sniff_port > 0)
         {
            fprintf(
                stderr,
                "Note: OpenSSL not compiled in; sniff port will serve plain connections only.\n");
            control_sniff = init_socket(sniff_port, INADDR_ANY);
         }
         if (wss_port > 0)
            fprintf(stderr, "Warning: OpenSSL not compiled in; --wss-port ignored.\n");
#endif
      }
   }
   global_port = port;
   global_ws_port = ws_port;
   global_wss_port = wss_port;
   global_tls_port = tls_port;
   global_sniff_port = sniff_port;
   global_http_port = http_port;
#ifdef HAVE_OPENSSL
   /* HOTreboot: if a native WSS socket was inherited, re-create the TLS
    * context (it lives in user-space memory and did not survive exec()). */
   if (fCopyOver && control_wss >= 0)
      init_tls_context(tls_cert, tls_key);
#endif
   if (fCopyOver)
      abort_threshold = BOOT_DB_ABORT_THRESHOLD;
   boot_db();
#ifdef HAVE_LIBPQ
   db_worker_start(NULL);
   db_help_open(".");
#endif
   npc_dialogue_init();
   init_mssp_counts();
#ifndef WIN32
   init_alarm_handler();
#endif
   {
      /* Build the "ready" message from an array of "port (label)" tokens. */
      struct
      {
         int port;
         const char *label;
      } listeners[] = {
          {port, "telnet"},
          {ws_port, "WebSocket loopback"},
          {control_tls >= 0 ? tls_port : -1, "TLS"},
          {sniff_port, "auto"},
          {control_wss >= 0 ? wss_port : -1, "WSS"},
      };
      int n = sizeof(listeners) / sizeof(listeners[0]);
      int pos = sprintf(log_buf, "ACK! MUD is ready");
      int count = 0;
      int i;
      for (i = 0; i < n; i++)
      {
         if (listeners[i].port > 0)
            count++;
      }
      if (count == 0)
      {
         pos += sprintf(log_buf + pos, " (HOTreboot recovery, no plain-telnet port)");
      }
      else
      {
         int seen = 0;
         pos += sprintf(log_buf + pos, " on port");
         for (i = 0; i < n; i++)
         {
            if (listeners[i].port <= 0)
               continue;
            if (seen > 0)
               pos += sprintf(log_buf + pos, (seen == count - 1) ? " and" : ",");
            pos += sprintf(log_buf + pos, " %d (%s)", listeners[i].port, listeners[i].label);
            seen++;
         }
      }
      sprintf(log_buf + pos, ".");
   }
   log_string(log_buf);
   if (fCopyOver)
   {
      extern bool disable_timer_abort;
      extern int last_checkpoint;
      extern int get_user_seconds(void);
      disable_timer_abort = TRUE;
      copyover_recover();
      last_checkpoint = get_user_seconds(); /* prevent false alarm abort after long recovery */
      disable_timer_abort = FALSE;
   }

   /* Seed WHO web output immediately on boot/copyover recovery. */
   list_who_to_output();

   game_loop(control, control_ws, control_tls, control_sniff, control_http, control_wss);
#ifdef HAVE_LIBPQ
   db_worker_stop();
   db_help_close();
#endif
   if (control >= 0)
      close(control);
   if (control_ws >= 0)
      close(control_ws);
   if (control_wss >= 0)
      close(control_wss);
   if (control_tls >= 0)
      close(control_tls);
   if (control_sniff >= 0)
      close(control_sniff);
   if (control_http >= 0)
      close(control_http);

   /*
    * That's all, folks.
    */
   log_string("Normal termination of game.");
   exit(0);
   return 0;
}

void stop_idling(CHAR_DATA *ch)
{
   if (ch == NULL || ch->desc == NULL || ch->desc->connected != CON_PLAYING ||
       ch->was_in_room == NULL || ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
      return;

   ch->timer = 0;
   char_from_room(ch);
   char_to_room(ch, ch->was_in_room);
   ch->was_in_room = NULL;
   act("$n has returned from the void.", ch, NULL, NULL, TO_ROOM);
   return;
}

/*
 * Write a message to all active descriptors.
 */
void send_to_descrips(const char *message)
{
   return;
}

/*
 * Write to one char.
 */
void send_to_char(const char *txt, CHAR_DATA *ch)
{
   if (ch == NULL)
      return;
   if (txt == NULL || ch->desc == NULL)
      return;
   /*
    * Large leak fixed here.. -- Altrag
    */
   if (ch->desc->showstr_head != NULL)
   {
      char *ssh;

      ssh = qgetmem(strlen(ch->desc->showstr_head) + strlen(txt) + 1);
      strcpy(ssh, ch->desc->showstr_head);
      strcat(ssh, txt);
      if (ch->desc->showstr_point)
         ch->desc->showstr_point += (ssh - ch->desc->showstr_head);
      else
         ch->desc->showstr_point = ssh;
      qdispose(ch->desc->showstr_head, strlen(ch->desc->showstr_head) + 1);
      ch->desc->showstr_head = ssh;
   }
   else
   {
      ch->desc->showstr_head = qgetmem(strlen(txt) + 1);
      strcpy(ch->desc->showstr_head, txt);
      ch->desc->showstr_point = ch->desc->showstr_head;
   }
   if (ch->desc->showstr_point == ch->desc->showstr_head)
      show_string(ch->desc, "");
   return;
}

/* The heart of the pager.  Thanks to N'Atas-Ha, ThePrincedom
   for porting this SillyMud code for MERC 2.0 and laying down the groundwork.
   Thanks to Blackstar, hopper.cs.uiowa.edu 4000 for which
   the improvements to the pager was modeled from.  - Kahn */
/* Leak fixes.. alloc_mem'd stuff shouldnt be free_string'd. -- Altrag */
/* Spec: buffer overflow fixes, internal buffer sizes increased */

void show_string(struct descriptor_data *d, char *input)
{
   char buffer[MAX_STRING_LENGTH * 2];
   char buf[MAX_INPUT_LENGTH];
   register char *scan, *chk;
   int lines = 0, toggle = 1;
   int space;

   one_argument(input, buf);

   switch (UPPER(buf[0]))
   {
   case '\0':
   case 'C': /* show next page of text */
      lines = 0;
      break;

   case 'R': /* refresh current page of text */
      lines = -1 - (d->character->pcdata->pagelen);
      break;

   case 'B': /* scroll back a page of text */
      lines = -(2 * d->character->pcdata->pagelen);
      break;

   case 'H': /* Show some help */
      write_to_buffer(d, "C, or Return = continue, R = redraw this page,\n\r", 0);
      write_to_buffer(d, "B = back one page, H = this help, Q or other keys = exit.\n\r\n\r", 0);
      lines = -1 - (d->character->pcdata->pagelen);
      break;

   default: /*otherwise, stop the text viewing */
      if (d->showstr_head)
      {
         qdispose(d->showstr_head, strlen(d->showstr_head) + 1);
         d->showstr_head = 0;
      }
      d->showstr_point = 0;
      return;
   }

   /*
    * do any backing up necessary
    */
   if (lines < 0)
   {
      for (scan = d->showstr_point; scan > d->showstr_head; scan--)
         if ((*scan == '\n') || (*scan == '\r'))
         {
            toggle = -toggle;
            if (toggle < 0)
               if (!(++lines))
                  break;
         }
      d->showstr_point = scan;
   }

   /*
    * show a chunk
    */
   lines = 0;
   toggle = 1;

   space = MAX_STRING_LENGTH * 2 - 100;
   for (scan = buffer;; scan++, d->showstr_point++)
   {
      space--;
      if (((*scan = *d->showstr_point) == '\n' || *scan == '\r') && (toggle = -toggle) < 0 &&
          space > 0)
         lines++;
      else if (!*scan ||
               (d->character && !IS_NPC(d->character) && lines >= d->character->pcdata->pagelen) ||
               space <= 0)
      {

         *scan = '\0';
         write_to_buffer(d, buffer, strlen(buffer));

         /*
          * See if this is the end (or near the end) of the string
          */
         for (chk = d->showstr_point; isspace(*chk); chk++)
            ;
         if (!*chk)
         {
            if (d->showstr_head)
            {
               qdispose(d->showstr_head, strlen(d->showstr_head) + 1);
               d->showstr_head = 0;
            }
            d->showstr_point = 0;
         }
         return;
      }
   }

   return;
}

/*
 * The primary output interface for formatted output.
 */
void act(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type)
{
   static char *const he_she[] = {"it", "he", "she"};
   static char *const him_her[] = {"it", "him", "her"};
   static char *const his_her[] = {"its", "his", "her"};

   char buf[MAX_STRING_LENGTH];
   char fname[MAX_INPUT_LENGTH];
   char tmp_str[MSL];
   CHAR_DATA *to;
   CHAR_DATA *vch = (CHAR_DATA *)arg2;
   OBJ_DATA *obj1 = (OBJ_DATA *)arg1;
   OBJ_DATA *obj2 = (OBJ_DATA *)arg2;
   const char *str;
   const char *i = "";
   char *point;
   bool do_crlf = TRUE;
   bool can_see_message = TRUE;

   /*
    * Discard null and zero-length messages.
    */
   if (format == NULL || format[0] == '\0')
      return;

   if (ch == NULL)

      return;

   if ((ch->is_free != FALSE) || (ch->in_room == NULL))
   {
      bugf("bad ch, string=%s", format);
      return;
   }

   to = ch->in_room->first_person;
   if (type == TO_VICT)
   {
      if (vch == NULL)
      {
         bugf("Act: null vch with TO_VICT, string=%s", format);
         return;
      }
      to = vch->in_room->first_person;
   }

   for (; to != NULL; to = to->next_in_room)
   {
      if ((to->desc == NULL && (IS_NPC(to) && !(to->pIndexData->progtypes & ACT_PROG))) ||
          !IS_AWAKE(to))
         continue;

      if (type == TO_CHAR && to != ch)
         continue;
      if (type == TO_VICT && (to != vch || to == ch))
         continue;
      if (type == TO_ROOM && to == ch)
         continue;
      if (type == TO_NOTVICT && (to == ch || to == vch))
         continue;

      if (type == TO_NOTVICT && vch != NULL && (ch->fighting == vch || vch->fighting == ch) &&
          shortfight_should_suppress_watched_autoattack(
              IS_NPC(to), IS_SET(to->config, CONFIG_SHORT_FIGHT), to->fighting != NULL))
         continue;

      point = buf;
      str = format;
      while (*str != '\0')
      {
         if (*str != '$')
         {
            *point++ = *str++;
            continue;
         }
         ++str;

         if (arg2 == NULL && *str >= 'M' && *str <= 'N')
         {
            bugf("Act: missing arg2 for code $%c, string=%s", *str, format);
            i = " !!!!! ";
         }

         else if (arg2 == NULL && *str >= 'S' && *str <= 'T')
         {
            bugf("Act: missing arg2 for code $%c, string=%s", *str, format);
            i = " !!!!! ";
         }
         else if (arg2 == NULL && *str == 'E')
         {
            bugf("Act: missing arg2 for code $%c, string=%s", *str, format);
            i = " !!!!! ";
         }
         else
         {
            switch (*str)
            {
            default:
               bugf("Act: bad code $%c, string=%s", *str, format);
               i = " !!!!! ";
               break;
               /*
                * Thx alex for 't' idea
                */
            case 'L':
               can_see_message = TRUE;
               if (IS_STAFF(to))
               {
                  if (IS_SET(ch->act, PLR_WIZINVIS) && ch->invis > get_trust(to)
                      /*
                       * || ( IS_SET( ch->act, PLR_INCOG )
                       * && ch->incog > get_trust( to )
                       */
                  )
                     can_see_message = FALSE;
               }
               else
               {
                  if (IS_SET(ch->act, PLR_WIZINVIS) && get_trust(to) < ch->invis)
                     can_see_message = FALSE;
                  if ((IS_AFFECTED(ch, AFF_SNEAK) || item_has_apply(ch, ITEM_APPLY_SNEAK)) &&
                      ((get_psuedo_level(ch) - 20 + number_range(1, 30)) > get_psuedo_level(to)))
                     can_see_message = FALSE;
               }
               break;
            case 't':
               i = (char *)arg1;
               break;
            case 'T':
               i = (char *)arg2;
               break;
            case 'n':
               i = PERS(ch, to);
               break;
            case 'N':
               i = PERS(vch, to);
               break;
            case 'e':
               i = he_she[URANGE(0, ch->sex, 2)];
               break;
            case 'E':
               i = he_she[URANGE(0, vch->sex, 2)];
               break;
            case 'm':
               i = him_her[URANGE(0, ch->sex, 2)];
               break;
            case 'M':
               i = him_her[URANGE(0, vch->sex, 2)];
               break;
            case 's':
               i = his_her[URANGE(0, ch->sex, 2)];
               break;
            case 'S':
               i = his_her[URANGE(0, vch->sex, 2)];
               break;
            case 'k':
               one_argument(ch->name, tmp_str);
               i = (char *)tmp_str;
               break;
            case 'K':
               one_argument(vch->name, tmp_str);
               i = (char *)tmp_str;
               break;

            case 'p':
               if (obj1)
               {
                  i = can_see_obj(to, obj1) ? obj1->short_descr : "something";
               }
               break;

            case 'P':
               if (obj2)
               {
                  i = can_see_obj(to, obj2) ? obj2->short_descr : "something";
               }
               break;

            case 'd':
               if (arg2 == NULL || ((char *)arg2)[0] == '\0')
               {
                  i = "door";
               }
               else
               {
                  one_argument((char *)arg2, fname);
                  i = fname;
               }
               break;
            }
            if (i[0] == '^')
            {
               ++i;
            }
         }
         ++str;
         while ((*point = *i) != '\0')
            ++point, ++i;
      }

      if (do_crlf)
      {
         *point++ = '\n';
         *point++ = '\r';
      }
      buf[0] = UPPER(buf[0]);
      *point = '\0';
      if (to->desc && can_see_message)
         write_to_buffer(to->desc, buf, point - buf);
   }

   return;
}

void hang(const char *str)
{
   bug(str, 0);
   kill(getpid(), SIGQUIT);
}

void notify(char *message, int lv)
{
   /*
    * This function sends <message>
    * * to all players of level (lv) and above
    * * -- Stephen
    */
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;

   sprintf(buf, "[NOTE]: %s\n\r", message);
   for (d = first_desc; d; d = d->next)
      if ((d->connected == CON_PLAYING) && (d->character->level >= lv) && !IS_NPC(d->character) &&
          !IS_SET(d->character->deaf, CHANNEL_NOTIFY))
         send_to_char(buf, d->character);
   return;
}

void auction(char *message)
{
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;

   sprintf(buf, "[AUCTION]: %s\n\r", message);
   for (d = first_desc; d; d = d->next)
      if ((d->connected == CON_PLAYING) && !IS_NPC(d->character) &&
          !IS_SET(d->character->deaf, CHANNEL_AUCTION))
         send_to_char(buf, d->character);
   return;
}

void info(char *message, int lv)
{
   /*
    * This function sends <message>
    * * to all players of level (lv) and above
    * * Used mainly to send level gain, death info, etc to mortals.
    * * - Stephen
    */
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;

   for (d = first_desc; d; d = d->next)
      if ((d->connected == CON_PLAYING) && (d->character->level >= lv) && !IS_NPC(d->character) &&
          !IS_SET(d->character->deaf, CHANNEL_INFO))
      {
         sprintf(buf, "%s[INFO]: %s%s\n\r", color_string(d->character, "info"), message,
                 color_string(d->character, "normal"));
         send_to_char(buf, d->character);
      }
   return;
}

void log_chan(const char *message, int lv)
{
   /*
    * Used to send messages to staff.
    * * Level is used to determine WHO gets the message...
    */
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;

   sprintf(buf, "[LOG]: %s\n\r", message);
   for (d = first_desc; d; d = d->next)
      if ((d->connected == CON_PLAYING) && (get_trust(d->character) == MAX_LEVEL) &&
          (!IS_NPC(d->character)) && (d->character->level >= lv) &&
          (!IS_SET(d->character->deaf, CHANNEL_LOG)))
         send_to_char(buf, d->character);
   return;
}

bool item_has_apply(CHAR_DATA *ch, int bit)
{
   /*
    * Used to see if ch is HOLDING any object(s) with the specified
    * * ITEM_APPLY bit set.
    * * -S-
    */
   OBJ_DATA *obj;

   for (obj = ch->first_carry; obj != NULL; obj = obj->next_in_carry_list)
      if (IS_SET(obj->item_apply, bit) && obj->wear_loc != WEAR_NONE)
         return TRUE;

   return FALSE;
}

#endif /* !UNIT_TEST_COMM */
