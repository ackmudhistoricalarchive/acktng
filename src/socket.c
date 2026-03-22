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
 * This file contains socket-related OS-dependent code:
 *   BSD sockets, TCP/IP, WebSocket support, and the main game I/O loop.
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
 */

#include <sys/types.h>
#include <sys/time.h>
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

bool command_has_wait_flag args((CHAR_DATA * ch, const char *argument));

/*
 * Socket and TCP/IP stuff.
 */
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
#include <zlib.h>
#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
#include "socket.h"
#include "prompt.h"

const char echo_off_str[] = {IAC, WILL, TELOPT_ECHO, '\0'};
const char echo_on_str[] = {IAC, WONT, TELOPT_ECHO, '\0'};
const char go_ahead_str[] = {IAC, GA, '\0'};

/*
 * Forward declarations for functions defined in comm.c but called from socket.c
 */
extern bool merc_down;
void nanny(DESCRIPTOR_DATA *d, char *argument);
void stop_idling(CHAR_DATA *ch);

/*
 * Forward declarations for socket.c-internal functions used before their definitions
 */
void new_descriptor(int control, bool is_tls, bool do_sniff);
void init_descriptor(DESCRIPTOR_DATA *dnew, int desc);
bool read_from_descriptor(DESCRIPTOR_DATA *d);
bool write_websocket_frame(DESCRIPTOR_DATA *d, unsigned char opcode, const unsigned char *payload,
                           size_t payload_len);
void process_telnet_options(DESCRIPTOR_DATA *d);
void send_mssp_data(DESCRIPTOR_DATA *d);
void process_msdp_subneg(DESCRIPTOR_DATA *d, unsigned char *payload, int len);
void process_gmcp_subneg(DESCRIPTOR_DATA *d, unsigned char *payload, int len);
void mccp2_start(DESCRIPTOR_DATA *d);
void mccp3_init(DESCRIPTOR_DATA *d);

DESCRIPTOR_DATA *d_next; /* Next descriptor in loop      */

int global_port;
int global_ws_port = -1;
int global_wss_port = -1;
int global_tls_port = -1;
int global_sniff_port = -1;
int global_http_port = -1;

#ifdef HAVE_OPENSSL
static SSL_CTX *global_ssl_ctx = NULL;

/*
 * Peek at the first byte of the connection to detect a TLS ClientHello (0x16).
 * Waits up to 500ms for data to arrive.  Returns TRUE if TLS, FALSE otherwise.
 */
static bool sniff_is_tls(int fd)
{
   fd_set fds;
   struct timeval tv;
   unsigned char byte;
   int n;

   FD_ZERO(&fds);
   FD_SET(fd, &fds);
   tv.tv_sec = 0;
   tv.tv_usec = 500000; /* 500ms */
   if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0)
      return FALSE;
   n = recv(fd, &byte, 1, MSG_PEEK);
   if (n <= 0)
      return FALSE;
   return byte == 0x16;
}
#endif

/* MSSP boot-time counters (populated by init_mssp_counts()) */
static int mssp_area_count = 0;
static int mssp_mob_count = 0;
static int mssp_obj_count = 0;
static int mssp_room_count = 0;
static int mssp_help_count = 0;
static int mssp_race_count = 0;
static int mssp_skill_count = 0;
static time_t boot_time = 0;

static const char websocket_guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static void sha1_transform(uint32_t state[5], const unsigned char block[64])
{
   uint32_t w[80], a, b, c, d, e, t;
   int i;

   for (i = 0; i < 16; i++)
      w[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16) |
             ((uint32_t)block[i * 4 + 2] << 8) | (uint32_t)block[i * 4 + 3];
   for (i = 16; i < 80; i++)
   {
      uint32_t v = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
      w[i] = (v << 1) | (v >> 31);
   }

   a = state[0];
   b = state[1];
   c = state[2];
   d = state[3];
   e = state[4];

   for (i = 0; i < 80; i++)
   {
      uint32_t f, k;

      if (i < 20)
      {
         f = (b & c) | ((~b) & d);
         k = 0x5A827999;
      }
      else if (i < 40)
      {
         f = b ^ c ^ d;
         k = 0x6ED9EBA1;
      }
      else if (i < 60)
      {
         f = (b & c) | (b & d) | (c & d);
         k = 0x8F1BBCDC;
      }
      else
      {
         f = b ^ c ^ d;
         k = 0xCA62C1D6;
      }

      t = ((a << 5) | (a >> 27)) + f + e + k + w[i];
      e = d;
      d = c;
      c = (b << 30) | (b >> 2);
      b = a;
      a = t;
   }

   state[0] += a;
   state[1] += b;
   state[2] += c;
   state[3] += d;
   state[4] += e;
}

static void sha1_digest(const unsigned char *data, size_t len, unsigned char out[20])
{
   uint32_t state[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
   unsigned char block[64];
   uint64_t bit_len = (uint64_t)len * 8;
   size_t i, rem;

   for (i = 0; i + 64 <= len; i += 64)
      sha1_transform(state, data + i);

   rem = len - i;
   memset(block, 0, sizeof(block));
   if (rem > 0)
      memcpy(block, data + i, rem);
   block[rem] = 0x80;

   if (rem >= 56)
   {
      sha1_transform(state, block);
      memset(block, 0, sizeof(block));
   }

   for (i = 0; i < 8; i++)
      block[56 + i] = (unsigned char)(bit_len >> ((7 - i) * 8));

   sha1_transform(state, block);

   for (i = 0; i < 5; i++)
   {
      out[i * 4] = (unsigned char)(state[i] >> 24);
      out[i * 4 + 1] = (unsigned char)(state[i] >> 16);
      out[i * 4 + 2] = (unsigned char)(state[i] >> 8);
      out[i * 4 + 3] = (unsigned char)state[i];
   }
}

static const char *find_http_header_value(const char *headers, const char *header_name)
{
   const char *line_start;
   size_t name_len;

   if (headers == NULL || header_name == NULL || header_name[0] == '\0')
      return NULL;

   name_len = strlen(header_name);
   line_start = headers;

   while (*line_start != '\0')
   {
      const char *line_end = strstr(line_start, "\r\n");
      size_t line_len;

      if (line_end == NULL)
         break;

      line_len = (size_t)(line_end - line_start);
      if (line_len == 0)
         break;

      if (line_len > name_len + 1 && strncasecmp(line_start, header_name, name_len) == 0 &&
          line_start[name_len] == ':')
      {
         const char *value_start = line_start + name_len + 1;
         while (*value_start == ' ' || *value_start == '\t')
            value_start++;
         return value_start;
      }

      line_start = line_end + 2;
   }

   return NULL;
}

static bool header_value_contains_token(const char *value, const char *token)
{
   const char *cursor;
   size_t token_len;

   if (value == NULL || token == NULL || token[0] == '\0')
      return FALSE;

   token_len = strlen(token);
   cursor = value;

   while (*cursor != '\0' && *cursor != '\r' && *cursor != '\n')
   {
      const char *entry_start = cursor;
      const char *entry_end;
      size_t entry_len;

      while (*entry_start == ' ' || *entry_start == '\t' || *entry_start == ',')
         entry_start++;

      if (*entry_start == '\0' || *entry_start == '\r' || *entry_start == '\n')
         break;

      entry_end = entry_start;
      while (*entry_end != '\0' && *entry_end != '\r' && *entry_end != '\n' && *entry_end != ',')
         entry_end++;

      while (entry_end > entry_start && (entry_end[-1] == ' ' || entry_end[-1] == '\t'))
         entry_end--;

      entry_len = (size_t)(entry_end - entry_start);
      if (entry_len == token_len && strncasecmp(entry_start, token, token_len) == 0)
         return TRUE;

      cursor = (*entry_end == ',') ? entry_end + 1 : entry_end;
   }

   return FALSE;
}

static void send_music_play(DESCRIPTOR_DATA *d, const char *filename);
static void base64_encode(const unsigned char *in, size_t in_len, char *out, size_t out_size)
{
   static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   size_t i, j = 0;

   if (out_size == 0)
      return;

   for (i = 0; i < in_len && j + 4 < out_size; i += 3)
   {
      uint32_t octet_a = in[i];
      uint32_t octet_b = (i + 1 < in_len) ? in[i + 1] : 0;
      uint32_t octet_c = (i + 2 < in_len) ? in[i + 2] : 0;
      uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

      out[j++] = table[(triple >> 18) & 0x3F];
      out[j++] = table[(triple >> 12) & 0x3F];
      out[j++] = (i + 1 < in_len) ? table[(triple >> 6) & 0x3F] : '=';
      out[j++] = (i + 2 < in_len) ? table[triple & 0x3F] : '=';
   }

   out[j] = '\0';
}

void queue_login_greeting(DESCRIPTOR_DATA *d)
{
   char buf[MAX_STRING_LENGTH];
   HELP_DATA *pHelp;
   extern HELP_DATA *first_help;

   if (d == NULL || d->greeting_sent)
      return;

   d->greeting_sent = TRUE;

   /* Send telnet protocol offers on plain/TLS telnet connections */
   if (!d->websocket_active)
   {
      static const char will_echo[] = {IAC, WILL, TELOPT_ECHO, '\0'};
      static const char will_mssp[] = {IAC, WILL, TELOPT_MSSP, '\0'};
      static const char will_msdp[] = {IAC, WILL, TELOPT_MSDP, '\0'};
      static const char will_gmcp[] = {IAC, WILL, TELOPT_GMCP, '\0'};
      static const char will_mccp2[] = {IAC, WILL, TELOPT_MCCP2, '\0'};
      static const char will_mccp3[] = {IAC, WILL, TELOPT_MCCP3, '\0'};
      write_to_descriptor(d, (char *)will_echo, 3);
      write_to_descriptor(d, (char *)will_mssp, 3);
      write_to_descriptor(d, (char *)will_msdp, 3);
      write_to_descriptor(d, (char *)will_gmcp, 3);
      write_to_descriptor(d, (char *)will_mccp2, 3);
      write_to_descriptor(d, (char *)will_mccp3, 3);
   }

   sprintf(buf, "greeting%d", number_range(1, 6));

   for (pHelp = first_help; pHelp != NULL; pHelp = pHelp->next)
      if (!str_cmp(pHelp->keyword, buf))
      {
         if (pHelp->text[0] == '.')
            write_to_buffer(d, pHelp->text + 1, 0);
         else
            write_to_buffer(d, pHelp->text, 0);
         break;
      }
}

static void get_request_path(const char *request, char *out, size_t outsz)
{
   const char *start, *end;
   size_t len;

   /* request starts with "GET " */
   start = request + 4;
   end = start;
   while (*end != '\0' && *end != ' ' && *end != '\r' && *end != '\n')
      end++;
   len = (size_t)(end - start);
   if (len >= outsz)
      len = outsz - 1;
   memcpy(out, start, len);
   out[len] = '\0';
}

static void handle_http_request(int desc)
{
   char buf[4096];
   int n;
   char path[64];
   const char *body;
   char header[256];

   n = recv(desc, buf, (int)sizeof(buf) - 1, 0);
   if (n <= 0)
      return;
   buf[n] = '\0';

   if (strncmp(buf, "GET ", 4) != 0)
      return;

   get_request_path(buf, path, sizeof(path));

   if (strcmp(path, "/gsgp") == 0 || strcmp(path, "/gsgp/") == 0)
   {
      body = get_gsgp_json();
      snprintf(header, sizeof(header),
               "HTTP/1.0 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Access-Control-Allow-Origin: *\r\n"
               "Content-Length: %d\r\n"
               "\r\n",
               (int)strlen(body));
      send(desc, header, strlen(header), 0);
      send(desc, body, strlen(body), 0);
   }
   else if (strcmp(path, "/who") == 0 || strcmp(path, "/who/") == 0)
   {
      body = get_wholist_html();
      snprintf(header, sizeof(header),
               "HTTP/1.0 200 OK\r\n"
               "Content-Type: text/html; charset=utf-8\r\n"
               "Content-Length: %d\r\n"
               "\r\n",
               (int)strlen(body));
      send(desc, header, strlen(header), 0);
      send(desc, body, strlen(body), 0);
   }
   else
   {
      const char *not_found = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
      send(desc, not_found, strlen(not_found), 0);
   }
}

bool handle_websocket_handshake(DESCRIPTOR_DATA *d)
{
   char *end_headers;
   char *key_line;
   char *key_start;
   char key[256], challenge[512], accept_key[64], response[512];
   unsigned char digest[20];
   size_t key_len;

   if (d == NULL || d->websocket_handshake_complete || d->inbuf_len <= 0)
      return TRUE;

   d->inbuf[d->inbuf_len] = '\0';
   if (strncmp(d->inbuf, "GET ", 4) != 0)
   {
      d->websocket_handshake_complete = TRUE;
      queue_login_greeting(d);
      return TRUE;
   }

   end_headers = strstr(d->inbuf, "\r\n\r\n");
   if (end_headers == NULL)
      return TRUE;

   key_line = (char *)find_http_header_value(d->inbuf, "Upgrade");
   if (key_line == NULL || !header_value_contains_token(key_line, "websocket"))
      return FALSE;

   key_start = (char *)find_http_header_value(d->inbuf, "Sec-WebSocket-Key");
   if (key_start == NULL)
      return FALSE;

   while (*key_start == ' ' || *key_start == '\t')
      key_start++;

   key_len = strcspn(key_start, "\r\n");
   if (key_len == 0 || key_len >= sizeof(key))
      return FALSE;

   memcpy(key, key_start, key_len);
   key[key_len] = '\0';

   snprintf(challenge, sizeof(challenge), "%s%s", key, websocket_guid);
   sha1_digest((const unsigned char *)challenge, strlen(challenge), digest);
   base64_encode(digest, sizeof(digest), accept_key, sizeof(accept_key));

   snprintf(response, sizeof(response),
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n\r\n",
            accept_key);

   if (!write_to_descriptor(d, response, 0))
      return FALSE;

   d->websocket_active = TRUE;
   d->websocket_handshake_complete = TRUE;

   d->inbuf_len -= (int)((end_headers + 4) - d->inbuf);
   memmove(d->inbuf, end_headers + 4, d->inbuf_len);
   d->inbuf[d->inbuf_len] = '\0';

   send_music_play(d, "theme.mp3");

   queue_login_greeting(d);
   return TRUE;
}

static void send_music_play(DESCRIPTOR_DATA *d, const char *filename)
{
   char buf[256];
   snprintf(buf, sizeof(buf), "{\"type\":\"music\",\"action\":\"play\",\"url\":\"/web/mp3/%s\"}",
            filename);
   write_websocket_frame(d, 0x1, (const unsigned char *)buf, strlen(buf));
}

static void send_music_stop(DESCRIPTOR_DATA *d)
{
   const char *msg = "{\"type\":\"music\",\"action\":\"stop\"}";
   write_websocket_frame(d, 0x1, (const unsigned char *)msg, strlen(msg));
}

/* Sentinel: websocket_current_music is set to this when a stop has been sent,
 * so we can distinguish "stop already sent" from "nothing sent yet" (NULL). */
#define MUSIC_STOPPED ((const char *)1)

void send_area_music(CHAR_DATA *ch)
{
   DESCRIPTOR_DATA *d;
   const char *track;

   if (IS_NPC(ch) || (d = ch->desc) == NULL || !d->websocket_active)
      return;

   track = ch->in_room->area->music; /* NULL means no music for this area */

   if (track == NULL)
   {
      /* No music for this area — stop playback if not already stopped */
      if (d->websocket_current_music == MUSIC_STOPPED)
         return;
      send_music_stop(d);
      d->websocket_current_music = MUSIC_STOPPED;
   }
   else
   {
      if (track == d->websocket_current_music)
         return;
      send_music_play(d, track);
      d->websocket_current_music = track;
   }
}

static size_t sanitize_websocket_text_payload(const unsigned char *src, size_t src_len,
                                              unsigned char *dst, size_t dst_cap)
{
   size_t i = 0, j = 0;

   if (src == NULL || dst == NULL || dst_cap == 0)
      return 0;

   while (i < src_len && j < dst_cap)
   {
      if (src[i] == (unsigned char)IAC)
      {
         if (i + 1 >= src_len)
         {
            i++;
            continue;
         }

         if (src[i + 1] == (unsigned char)GA)
         {
            i += 2;
            continue;
         }

         if ((src[i + 1] == (unsigned char)WILL || src[i + 1] == (unsigned char)WONT ||
              src[i + 1] == (unsigned char)DO || src[i + 1] == (unsigned char)DONT) &&
             i + 2 < src_len)
         {
            i += 3;
            continue;
         }
      }

      if (src[i] == '\r')
      {
         i++;
         continue; /* strip carriage return */
      }

      dst[j++] = src[i++];
   }

   return j;
}

bool write_websocket_frame(DESCRIPTOR_DATA *d, unsigned char opcode, const unsigned char *payload,
                           size_t payload_len)
{
   unsigned char header[4];
   size_t hlen;

   if (payload_len > 65535)
      return FALSE;

   header[0] = 0x80 | (opcode & 0x0F);
   if (payload_len < 126)
   {
      header[1] = (unsigned char)payload_len;
      hlen = 2;
   }
   else
   {
      header[1] = 126;
      header[2] = (unsigned char)(payload_len >> 8);
      header[3] = (unsigned char)(payload_len & 0xFF);
      hlen = 4;
   }

   if (!write_to_descriptor(d, (char *)header, (int)hlen))
      return FALSE;
   if (payload_len > 0 && !write_to_descriptor(d, (char *)payload, (int)payload_len))
      return FALSE;

   return TRUE;
}

bool process_websocket_input(DESCRIPTOR_DATA *d)
{
   unsigned char *buf = (unsigned char *)d->inbuf;

   if (d == NULL || d->incomm[0] != '\0')
      return TRUE;

   while (d->inbuf_len >= 2)
   {
      size_t pos = 2, payload_len, frame_len;
      unsigned char opcode;
      unsigned char mask[4];
      int i, k = 0;

      buf = (unsigned char *)d->inbuf;
      opcode = buf[0] & 0x0F;
      payload_len = buf[1] & 0x7F;

      if (payload_len == 126)
      {
         if (d->inbuf_len < 4)
            return TRUE;
         payload_len = ((size_t)buf[2] << 8) | buf[3];
         pos = 4;
      }
      else if (payload_len == 127 || (buf[1] & 0x80) == 0)
         return FALSE;

      if ((size_t)d->inbuf_len < pos + 4 + payload_len)
         return TRUE;

      memcpy(mask, buf + pos, 4);
      pos += 4;
      frame_len = pos + payload_len;

      for (i = 0; i < (int)payload_len; i++)
         buf[pos + i] ^= mask[i % 4];

      if (opcode == 0x8)
         return FALSE;
      if (opcode == 0x9)
      {
         if (!write_websocket_frame(d, 0xA, buf + pos, payload_len))
            return FALSE;
      }
      else if (opcode == 0x1 || opcode == 0x0 || opcode == 0x2)
      {
         if (payload_len >= MAX_INPUT_LENGTH - 1)
            write_to_descriptor(d, "Line too long.\n\r", 0);
         else
         {
            for (i = 0; i < (int)payload_len; i++)
            {
               unsigned char c = buf[pos + i];
               if (c == '\b' && k > 0)
                  --k;
               else if (isprint(c))
                  d->incomm[k++] = (char)c;
            }
            if (k == 0)
               d->incomm[k++] = ' ';
            d->incomm[k] = '\0';
         }
      }

      d->inbuf_len -= (int)frame_len;
      memmove(d->inbuf, d->inbuf + frame_len, d->inbuf_len);
      d->inbuf[d->inbuf_len] = '\0';

      if (d->incomm[0] != '\0')
         return TRUE;
   }

   return TRUE;
}

int init_socket(int port, unsigned long bind_addr)
{
   static struct sockaddr_in sa_zero;
   struct sockaddr_in sa;
   int x = 1;
   int fd;

   if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      perror("Init_socket: socket");
      exit(1);
   }

   if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&x, sizeof(x)) < 0)
   {
      perror("Init_socket: SO_REUSEADDR");
      close(fd);
      exit(1);
   }

#if defined(SO_DONTLINGER) && !defined(SYSV)
   {
      struct linger ld;

      ld.l_onoff = 1;
      ld.l_linger = 1000;

      if (setsockopt(fd, SOL_SOCKET, SO_DONTLINGER, (char *)&ld, sizeof(ld)) < 0)
      {
         perror("Init_socket: SO_DONTLINGER");
         close(fd);
         exit(1);
      }
   }
#endif
   sa = sa_zero;
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = htonl(bind_addr);
   sa.sin_port = htons(port);

   if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
   {
      perror("Init_socket: bind");
      close(fd);
      exit(1);
   }

   if (listen(fd, 3) < 0)
   {
      perror("Init_socket: listen");
      close(fd);
      exit(1);
   }

   return fd;
}

int cur_hour = 0;
int cur_players = 0;

/* + */
int reopen_flag;
void reopen_socket(int sig)
{
   reopen_flag = 1;
   signal(SIGUSR1, reopen_socket);
}

/* + */

void game_loop(int control, int control_ws, int control_tls, int control_sniff, int control_http,
               int control_wss)
{
   static struct timeval null_time;
   struct timeval last_time;

   signal(SIGPIPE, SIG_IGN);
   /*+*/
   /*
    * On a SIGUSR1, open and close the control socket (anti-port-locking
    * *  thing) -- Spectrum
    */
   reopen_flag = 0;
   signal(SIGUSR1, reopen_socket);

   /*+*/

   gettimeofday(&last_time, NULL);
   current_time = (time_t)last_time.tv_sec;

   /*
    * Main loop
    */
   while (!merc_down)
   {
      fd_set in_set;
      fd_set out_set;
      fd_set exc_set;
      DESCRIPTOR_DATA *d;
      int maxdesc;

      /*
       * handle reopening the control socket
       * * don't worry about locking here, we assume that SIGUSR1's are
       * * relatively rare
       */
      if (reopen_flag)
      {
         log_string("SIGUSR1 received, reopening control socket");
         if (control >= 0)
         {
            close(control);
            control = init_socket(global_port, INADDR_ANY);
         }
         if (control_ws >= 0)
         {
            close(control_ws);
            control_ws = init_socket(global_ws_port, INADDR_LOOPBACK);
         }
         if (control_tls >= 0)
         {
            close(control_tls);
            control_tls = init_socket(global_tls_port, INADDR_ANY);
         }
         if (control_sniff >= 0)
         {
            close(control_sniff);
            control_sniff = init_socket(global_sniff_port, INADDR_ANY);
         }
         if (control_http >= 0)
         {
            close(control_http);
            control_http = init_socket(global_http_port, INADDR_ANY);
         }
         if (control_wss >= 0)
         {
            close(control_wss);
            control_wss = init_socket(global_wss_port, INADDR_ANY);
         }
         reopen_flag = 0;
      }

      /*
       * Poll all active descriptors.
       */
      FD_ZERO(&in_set);
      FD_ZERO(&out_set);
      FD_ZERO(&exc_set);
      maxdesc = 0;
      if (control >= 0)
      {
         FD_SET(control, &in_set);
         maxdesc = control;
      }
      if (control_ws >= 0)
      {
         FD_SET(control_ws, &in_set);
         maxdesc = UMAX(maxdesc, control_ws);
      }
      if (control_tls >= 0)
      {
         FD_SET(control_tls, &in_set);
         maxdesc = UMAX(maxdesc, control_tls);
      }
      if (control_sniff >= 0)
      {
         FD_SET(control_sniff, &in_set);
         maxdesc = UMAX(maxdesc, control_sniff);
      }
      if (control_http >= 0)
      {
         FD_SET(control_http, &in_set);
         maxdesc = UMAX(maxdesc, control_http);
      }
      if (control_wss >= 0)
      {
         FD_SET(control_wss, &in_set);
         maxdesc = UMAX(maxdesc, control_wss);
      }

      for (d = first_desc; d; d = d->next)
      {
         if ((d->flags && DESC_FLAG_PASSTHROUGH) == 0)
         {
            maxdesc = UMAX(maxdesc, d->descriptor);
            FD_SET(d->descriptor, &in_set);
            FD_SET(d->descriptor, &out_set);
            FD_SET(d->descriptor, &exc_set);
         }
         else
         {
            /*
             * Check to see if child process has terminated
             */
            if (waitpid(d->childpid, NULL, WNOHANG) != 0)
            {
               /*
                * Terminated or error
                */
               d->childpid = 0;
               REMOVE_BIT(d->flags, DESC_FLAG_PASSTHROUGH);
            }
         }
      }

      if (select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0)
      {
         perror("Game_loop: select: poll");
         exit(1);
      }

      /*
       * New connection?
       */
      if (control >= 0 && FD_ISSET(control, &in_set))
         new_descriptor(control, FALSE, TRUE);
      if (control_ws >= 0 && FD_ISSET(control_ws, &in_set))
         new_descriptor(control_ws, FALSE, FALSE);
      if (control_tls >= 0 && FD_ISSET(control_tls, &in_set))
         new_descriptor(control_tls, TRUE, FALSE);
      if (control_sniff >= 0 && FD_ISSET(control_sniff, &in_set))
         new_descriptor(control_sniff, FALSE, TRUE);
      if (control_http >= 0 && FD_ISSET(control_http, &in_set))
      {
         struct sockaddr_in sa;
         socklen_t size = sizeof(sa);
         int desc = accept(control_http, (struct sockaddr *)&sa, &size);
         if (desc >= 0)
         {
            handle_http_request(desc);
            close(desc);
         }
      }
      if (control_wss >= 0 && FD_ISSET(control_wss, &in_set))
         new_descriptor(control_wss, TRUE, FALSE);

         /*
          * Advance any pending TLS handshakes non-blockingly.
          * SSL_accept was deferred from new_descriptor to avoid blocking the
          * game loop.  Each iteration we try to complete the handshake when the
          * socket is ready, or time out after a short deadline.
          */
#ifdef HAVE_OPENSSL
      for (d = first_desc; d != NULL; d = d_next)
      {
         d_next = d->next;
         if (!d->tls_handshake_pending)
            continue;

         /* Enforce the short handshake deadline */
         if (current_time >= d->timeout)
         {
            d->outtop = 0;
            close_socket(d);
            continue;
         }

         /* Only try when the socket signals readiness */
         if (!FD_ISSET(d->descriptor, &in_set) && !FD_ISSET(d->descriptor, &out_set))
            continue;

         {
            int ret = SSL_accept((SSL *)d->ssl);
            if (ret > 0)
            {
               d->tls_handshake_pending = FALSE;
               d->tls_active = TRUE;
               d->timeout = current_time + 180;
               queue_login_greeting(d);
            }
            else
            {
               int err = SSL_get_error((SSL *)d->ssl, ret);
               if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
               {
                  ERR_print_errors_fp(stderr);
                  d->outtop = 0;
                  close_socket(d);
               }
               /* else: keep waiting until next tick */
            }
         }
      }
#endif

      /*
       * Kick out the freaky folks.
       */
      for (d = first_desc; d != NULL; d = d_next)
      {
         d_next = d->next;
         if (FD_ISSET(d->descriptor, &exc_set))
         {
            FD_CLR(d->descriptor, &in_set);
            FD_CLR(d->descriptor, &out_set);
            if (d->character)
               save_char_obj(d->character);
            d->outtop = 0;
            close_socket(d);
         }
      }

      /*
       * Process input.
       */
      for (d = first_desc; d != NULL; d = d_next)
      {
         d_next = d->next;
#ifdef HAVE_OPENSSL
         if (d->tls_handshake_pending)
            continue;
#endif
         d->fcommand = FALSE;

         if (FD_ISSET(d->descriptor, &in_set))
         {
            if (d->character != NULL)
               d->character->timer = 0;
            if (!read_from_descriptor(d))
            {
               FD_CLR(d->descriptor, &out_set);
               if (d->character != NULL)
                  save_char_obj(d->character);
               d->outtop = 0;
               close_socket(d);
               continue;
            }
         }

         /* Read the next command before checking wait, so we can inspect it. */
         read_from_buffer(d);

         if (d->character != NULL && d->character->wait > 0)
         {
            --d->character->wait;

            /*
             * If the pending command introduces a wait state, keep it queued.
             * incomm is left non-empty; read_from_buffer will not overwrite it
             * on the next pulse, so the command stays queued until wait expires.
             * Non-wait-state commands (look, score, say, etc.) fall through and
             * execute immediately.
             */
            if (d->incomm[0] != '\0' && d->connected == CON_PLAYING &&
                command_has_wait_flag(d->character, d->incomm))
               continue;
         }

         if (d->incomm[0] != '\0')
         {
            d->fcommand = TRUE;
            stop_idling(d->character);
            d->timeout = current_time + 180; /* spec: stop idling */

            if (d->connected == CON_PLAYING)
               if (d->showstr_point)
                  show_string(d, d->incomm);
               else
                  interpret(d->character, d->incomm);
            else
               nanny(d, d->incomm);

            d->incomm[0] = '\0';
         }
      }

      /*
       * Autonomous game motion.
       */
      update_handler();
      waitpid(0, 0, WNOHANG);

      /*
       * Output.
       */
      for (d = first_desc; d != NULL; d = d_next)
      {
         d_next = d->next;
#ifdef HAVE_OPENSSL
         if (d->tls_handshake_pending)
            continue;
#endif

         /*
          * spec: disconnect people idling on login
          */
         if (d->connected < 0 && d->timeout < current_time)
         {
            if (d->websocket_active)
               write_websocket_frame(d, 0x1, (const unsigned char *)"Login timeout (180s)\n\r",
                                     strlen("Login timeout (180s)\n\r"));
            else
               write_to_descriptor(d, "Login timeout (180s)\n\r", 0);
            close_socket(d);
            continue;
         }

         if ((d->fcommand || d->outtop > 0) && FD_ISSET(d->descriptor, &out_set))
         {
            if (!process_output(d, TRUE))
            {
               if (d->character != NULL)
                  save_char_obj(d->character);
               d->outtop = 0;
               close_socket(d);
            }
         }
      }

      /*
       * Synchronize to a clock.
       * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
       * Careful here of signed versus unsigned arithmetic.
       */
      {
         struct timeval now_time;
         struct tm *now_bd_time;
         FILE *out_file;
         long secDelta;
         long usecDelta;

         long temp_time;

         gettimeofday(&now_time, NULL);
         temp_time = (time_t)now_time.tv_sec;
         now_bd_time = localtime(&temp_time);

         if (now_bd_time->tm_hour != cur_hour)
         {
            cur_hour = now_bd_time->tm_hour;
            out_file = fopen("players.num", "a");
            fprintf(out_file, "%i %i %i\n", now_bd_time->tm_mday, cur_hour, max_players);
            if (out_file != NULL)
            {
               fclose(out_file);
               out_file = NULL;
            }
         }

         usecDelta =
             ((int)last_time.tv_usec) - ((int)now_time.tv_usec) + 1000000 / PULSE_PER_SECOND;
         secDelta = ((int)last_time.tv_sec) - ((int)now_time.tv_sec);
         while (usecDelta < 0)
         {
            usecDelta += 1000000;
            secDelta -= 1;
         }

         while (usecDelta >= 1000000)
         {
            usecDelta -= 1000000;
            secDelta += 1;
         }

         if (secDelta > 0 || (secDelta == 0 && usecDelta > 0))
         {
            struct timeval stall_time;

            stall_time.tv_usec = usecDelta;
            stall_time.tv_sec = secDelta;
            if (select(0, NULL, NULL, NULL, &stall_time) < 0 && errno != EINTR)
            {
               perror("Game_loop: select: stall");
               exit(1);
            }
         }
      }

      gettimeofday(&last_time, NULL);
      current_time = (time_t)last_time.tv_sec;
   }

   return;
}

void free_desc(DESCRIPTOR_DATA *d)
{
   DESCRIPTOR_DATA *sd;

   d->snoop_by = NULL;
   for (sd = first_desc; sd; sd = sd->next)
      if (sd->snoop_by == d)
         sd->snoop_by = NULL;
   if (d->original)
      do_return(d->character, "");
   if (d->character)
      free_char(d->character);
   free_string(d->host);
   close(d->descriptor);
   if (d->showstr_head)
      qdispose(d->showstr_head, strlen(d->showstr_head) + 1);
   if (d->outbuf)
      dispose(d->outbuf, d->outsize);
}

void new_descriptor(int control, bool is_tls, bool do_sniff)
{
   static DESCRIPTOR_DATA d_zero;
   char buf[MSL];
   DESCRIPTOR_DATA *dnew;
   /*
    * BAN_DATA *pban;
    */
   struct sockaddr_in sock;
   int desc;
   socklen_t size;

   size = sizeof(sock);
   getsockname(control, (struct sockaddr *)&sock, &size);
   if ((desc = accept(control, (struct sockaddr *)&sock, &size)) < 0)
   {
      perror("New_descriptor: accept");
      return;
   }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

   if (fcntl(desc, F_SETFL, FNDELAY) == -1)
   {
      perror("New_descriptor: fcntl: FNDELAY");
      return;
   }

   /*
    * Cons a new descriptor.
    */
   GET_FREE(dnew, desc_free);
   *dnew = d_zero;
   init_descriptor(dnew, desc); /* Not sure is this right? */
   dnew->descriptor = desc;
   dnew->connected = CON_GET_NAME;
   dnew->showstr_head = NULL;
   dnew->showstr_point = NULL;
   dnew->outsize = 2000;
   dnew->flags = 0;
   dnew->childpid = 0;

#ifdef HAVE_OPENSSL
   if (do_sniff)
      is_tls = sniff_is_tls(desc);
   if (is_tls && global_ssl_ctx != NULL)
   {
      SSL *ssl;
      int ret, err;

      ssl = SSL_new(global_ssl_ctx);
      SSL_set_fd(ssl, desc);
      ret = SSL_accept(ssl);
      if (ret > 0)
      {
         dnew->ssl = ssl;
         dnew->tls_active = TRUE;
      }
      else
      {
         err = SSL_get_error(ssl, ret);
         if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
         {
            /*
             * Handshake not yet complete on a non-blocking socket.
             * Store the SSL object and mark the descriptor so game_loop
             * advances the handshake without blocking the main thread.
             * Greeting is deferred until the handshake finishes.
             */
            dnew->ssl = ssl;
            dnew->tls_handshake_pending = TRUE;
            dnew->timeout = current_time + 10; /* short handshake deadline */
         }
         else
         {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(desc);
            PUT_FREE(dnew, desc_free);
            return;
         }
      }
   }
#else
   (void)is_tls;
   (void)do_sniff;
#endif

   size = sizeof(sock);
   if (getpeername(desc, (struct sockaddr *)&sock, &size) < 0)
   {
      perror("New_descriptor: getpeername");
      dnew->host = str_dup("(unknown)");
   }
   else
   {
      /*
       * Would be nice to use inet_ntoa here but it takes a struct arg,
       * which ain't very compatible between gcc and system libraries.
       */
      int addr;

      addr = ntohl(sock.sin_addr.s_addr);
      sprintf(buf, "%d.%d.%d.%d", (addr >> 24) & 0xFF, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF,
              (addr) & 0xFF);
      sprintf(log_buf, "Sock.sinaddr:  %s (%d)", buf, ntohs(sock.sin_port));
      log_string(log_buf);
      sprintf(log_buf, "Connection formed from %s.", buf);
      monitor_chan(log_buf, MONITOR_CONNECT);

      dnew->remote_port = ntohs(sock.sin_port);

      /*
       * From unused to prevent possible ns lockup
       * from = gethostbyaddr( (char*) &sock.sin_addr,
       * sizeof(sock.sin_addr), AF_INET );
       */

      dnew->host = str_dup(buf);
   }

   /*
    * Swiftest: I added the following to ban sites.  I don't
    * endorse banning of sites, but Copper has few descriptors now
    * and some people from certain sites keep abusing access by
    * using automated 'autodialers' and leaving connections hanging.
    *
    * Furey: added suffix check by request of Nickel of HiddenWorlds.
    *
    * Stephen: As we use IP address now, want to use prefix check,
    * so we can ban whole domains....
    */
   /*
    * MOVED TO LOWER FOR NEWBIEBAN
    *
    * for ( pban = first_ban; pban != NULL; pban = pban->next )
    * {
    * if ( !str_prefix( pban->name, dnew->host ) && !( pban->newbie ) )
    * {
    * char buf[MAX_STRING_LENGTH];
    * sprintf( buf, "Denying access to banned site %s", dnew->host );
    * monitor_chan( buf, MONITOR_CONNECT );
    * write_to_descriptor( desc,
    * "Your site has been banned from this Mud.  BYE BYE!\n\r", 0 );
    * free_desc(dnew);
    * PUT_FREE(dnew, desc_free);
    * return;
    * }
    * }
    *
    *
    */
   /*
    * Init descriptor data.
    */
   LINK(dnew, first_desc, last_desc, next, prev);

   /*
    * spec: set initial login timeout (may have been overridden above to a
    * short TLS handshake deadline; restored to 180s after handshake done).
    * When HAVE_OPENSSL is not defined there is no TLS, so always set now.
    */
#ifdef HAVE_OPENSSL
   if (!dnew->tls_handshake_pending)
#endif
      dnew->timeout = current_time + 180;

#ifdef HAVE_OPENSSL
   if (!dnew->tls_handshake_pending)
#endif
      queue_login_greeting(dnew);

   cur_players++;

   return;
}

void init_descriptor(DESCRIPTOR_DATA *dnew, int desc)
{
   static DESCRIPTOR_DATA d_zero;
   *dnew = d_zero;
   dnew->descriptor = desc;
   dnew->connected = CON_GET_NAME;
   dnew->showstr_head = NULL;
   dnew->showstr_point = NULL;
   dnew->outsize = 2000;
   dnew->outbuf = getmem(dnew->outsize);
   dnew->flags = 0;
   dnew->childpid = 0;
   dnew->inbuf_len = 0;
   dnew->websocket_active = FALSE;
   dnew->websocket_handshake_complete = FALSE;
   dnew->greeting_sent = FALSE;
   dnew->websocket_current_music = NULL;
   dnew->tls_active = FALSE;
#ifdef HAVE_OPENSSL
   dnew->ssl = NULL;
   dnew->tls_handshake_pending = FALSE;
#endif
   dnew->msdp_active = FALSE;
   dnew->msdp_reported = 0;
   dnew->msdp_last_room = 0;
   dnew->gmcp_active = FALSE;
   dnew->gmcp_supports = 0;
   dnew->gmcp_last_room = 0;
   dnew->mccp2_active = FALSE;
   dnew->mccp2_zout = NULL;
   dnew->mccp3_active = FALSE;
   dnew->mccp3_zin = NULL;
}

void close_socket(DESCRIPTOR_DATA *dclose)
{
   CHAR_DATA *ch;

   if (dclose->outtop > 0)
      process_output(dclose, FALSE);

   if (dclose->snoop_by != NULL)
   {
      write_to_buffer(dclose->snoop_by, "Your victim has left the game.\n\r", 0);
   }

   {
      DESCRIPTOR_DATA *d;

      for (d = first_desc; d != NULL; d = d->next)
      {
         if (d->snoop_by == dclose)
            d->snoop_by = NULL;
      }
   }

   if (dclose->original)
   {
      if (dclose->character)
         do_return(dclose->character, "");
      else
      {
         bug("Close_socket: original without ch", 0);
         dclose->character = dclose->original;
         dclose->original = NULL;
      }
   }

   if ((ch = dclose->character) != NULL)
   {
      sprintf(log_buf, "Closing link to %s.", ch->name);
      log_string(log_buf);
      monitor_chan(log_buf, MONITOR_CONNECT);
      if (dclose->connected == CON_PLAYING)
      {
         act("$n has lost $s link.", ch, NULL, NULL, TO_ROOM);
         ch->desc = NULL;
      }
      else
      {
         free_char(dclose->character);
      }
      /*      stop_fighting( ch, TRUE );
            save_char_obj( ch );
            extract_char( ch, TRUE );  */
   }

   if (d_next == dclose)
      d_next = d_next->next;

   /* Tear down compression and TLS before closing the fd */
   if (dclose->mccp2_active && dclose->mccp2_zout)
   {
      deflateEnd((z_stream *)dclose->mccp2_zout);
      free(dclose->mccp2_zout);
      dclose->mccp2_zout = NULL;
      dclose->mccp2_active = FALSE;
   }
   if (dclose->mccp3_active && dclose->mccp3_zin)
   {
      inflateEnd((z_stream *)dclose->mccp3_zin);
      free(dclose->mccp3_zin);
      dclose->mccp3_zin = NULL;
      dclose->mccp3_active = FALSE;
   }
#ifdef HAVE_OPENSSL
   if (dclose->ssl)
   {
      if (dclose->tls_active)
         SSL_shutdown((SSL *)dclose->ssl);
      SSL_free((SSL *)dclose->ssl);
      dclose->ssl = NULL;
      dclose->tls_active = FALSE;
      dclose->tls_handshake_pending = FALSE;
   }
#endif

   UNLINK(dclose, first_desc, last_desc, next, prev);
   close(dclose->descriptor);
   free_string(dclose->host);
   if (dclose->outbuf)
      dispose(dclose->outbuf, dclose->outsize);
   if (dclose->showstr_head)
      qdispose(dclose->showstr_head, strlen(dclose->showstr_head) + 1);
   PUT_FREE(dclose, desc_free);

   cur_players--;
   list_who_to_output();
   return;
}

bool read_from_descriptor(DESCRIPTOR_DATA *d)
{
   int iStart;

   /*
    * Hold horses if pending command already.
    */
   if (d->incomm[0] != '\0')
      return TRUE;

   /*
    * Check for overflow.
    */
   iStart = d->inbuf_len;
   if (iStart >= sizeof(d->inbuf) - 10)
   {
      sprintf(log_buf, "%s input overflow!", d->host);
      log_string(log_buf);
      sprintf(log_buf, "input overflow by %s (%s)",
              (d->character == NULL) ? "[login]" : d->character->name, d->host);
      monitor_chan(log_buf, MONITOR_CONNECT);
      write_to_descriptor(d, "\n\r SPAMMING IS RUDE, BYE BYE! \n\r", 0);
      return FALSE;
   }

   /*
    * Snarf input.
    */
   for (;;)
   {
      int nRead;

#ifdef HAVE_OPENSSL
      if (d->tls_active)
         nRead = SSL_read((SSL *)d->ssl, d->inbuf + iStart, sizeof(d->inbuf) - 10 - iStart);
      else
#endif
         nRead = read(d->descriptor, d->inbuf + iStart, sizeof(d->inbuf) - 10 - iStart);
      if (nRead > 0)
      {
         iStart += nRead;
         if (d->websocket_active)
            break;
         if (d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r')
            break;
      }
      else if (nRead == 0)
      {
         log_string("EOF encountered on read.");
         return FALSE;
      }
      else if (errno == EWOULDBLOCK)
         break;
      else
      {
         perror("Read_from_descriptor");
         return FALSE;
      }
   }
   d->inbuf_len = iStart;
   d->inbuf[iStart] = '\0';

   /* MCCP3: decompress client-to-server data in place */
   if (d->mccp3_active && d->mccp3_zin && d->inbuf_len > 0)
   {
      z_stream *z = (z_stream *)d->mccp3_zin;
      unsigned char dbuf[sizeof(d->inbuf) - 10];

      z->next_in = (Bytef *)d->inbuf;
      z->avail_in = (uInt)d->inbuf_len;
      z->next_out = dbuf;
      z->avail_out = sizeof(dbuf);

      int ret = inflate(z, Z_SYNC_FLUSH);
      if (ret < 0)
      {
         log_string("MCCP3: inflate error, closing connection.");
         return FALSE;
      }

      int produced = (int)(sizeof(dbuf) - z->avail_out);
      memcpy(d->inbuf, dbuf, produced);
      d->inbuf[produced] = '\0';
      d->inbuf_len = produced;
   }

   if (!d->websocket_handshake_complete)
   {
      if (!handle_websocket_handshake(d))
         return FALSE;
      if (!d->websocket_active && d->websocket_handshake_complete)
         queue_login_greeting(d);
   }

   return TRUE;
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer(DESCRIPTOR_DATA *d)
{
   int i, j, k;

   /*
    * Hold horses if pending command already.
    */
   if (d->incomm[0] != '\0')
      return;

   if (d->websocket_active)
   {
      process_websocket_input(d);
      return;
   }

   /* Strip and dispatch IAC telnet sequences before parsing player input */
   process_telnet_options(d);

   /*
    * Look for at least one new line.
    */
   for (i = 0; i < d->inbuf_len && d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
      ;
   if (i >= d->inbuf_len)
      return;

   /*
    * Canonical input processing.
    */
   for (i = 0, k = 0; i < d->inbuf_len && d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
   {
      if (k >= MAX_INPUT_LENGTH - 2)
      {
         write_to_descriptor(d, "Line too long.\n\r", 0);

         /*
          * skip the rest of the line
          */
         for (; i < d->inbuf_len; i++)
         {
            if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
               break;
         }
         break;
      }

      if (d->inbuf[i] == '\b' && k > 0)
         --k;
      else if (isascii(d->inbuf[i]) && isprint(d->inbuf[i]))
         d->incomm[k++] = d->inbuf[i];
   }

   /*
    * Finish off the line.
    */
   if (k == 0)
      d->incomm[k++] = ' ';
   d->incomm[k] = '\0';

   /*
    * Deal with bozos with #repeat 1000 ...
    */
   if (k > 1 || d->incomm[0] == '!')
   {
      if (d->incomm[0] != '!' && strcmp(d->incomm, d->inlast))
      {
         d->repeat = 0;
      }
      else
      {
         if (++d->repeat >= 30)
         {
            if (d->connected == CON_PLAYING)
            {
               sprintf(log_buf, "%s input spamming!", d->character->name);
               log_string(log_buf);
               monitor_chan(log_buf, MONITOR_CONNECT);
            }
            write_to_descriptor(d, "\n\r***** SHUT UP!! *****\n\r", 0);
            close_socket(d);
            /*
             * old way: strcpy( d->incomm, "quit" );
             */
         }
      }
   }

   /*
    * Do '!' substitution.
    */
   if (d->incomm[0] == '!')
      strcpy(d->incomm, d->inlast);
   else
      strcpy(d->inlast, d->incomm);

   /*
    * Shift the input buffer.
    */
   while (i < d->inbuf_len && (d->inbuf[i] == '\n' || d->inbuf[i] == '\r'))
      i++;
   for (j = 0; i + j < d->inbuf_len; j++)
      d->inbuf[j] = d->inbuf[i + j];
   d->inbuf_len = j;
   d->inbuf[d->inbuf_len] = '\0';
   return;
}

/*
 * Low level output function.
 */
bool process_output(DESCRIPTOR_DATA *d, bool fPrompt)
{
   extern bool merc_down;

   /*
    * Bust a prompt.
    */
   if (fPrompt && !merc_down && d->connected == CON_PLAYING)
   {
      if (d->showstr_point)
         write_to_buffer(
             d, "[Please type (c)ontinue, (r)efresh, (b)ack, (h)elp, (q)uit, or RETURN]:  ", 0);
      else
      {
         CHAR_DATA *ch;

         ch = d->original ? d->original : d->character;
         if (IS_SET(ch->config, CONFIG_BLANK))
            write_to_buffer(d, "\n\r", 2);
         if (ch->hunting || ch->hunt_obj)
            char_hunt(ch);
         bust_a_prompt(d);
         if (IS_SET(ch->config, CONFIG_TELNET_GA))
            write_to_buffer(d, go_ahead_str, 0);
      }
   }
   /*
    * Short-circuit if nothing to write.
    */
   if (d->outtop == 0)
      return TRUE;
   /*
    * Snoop-o-rama.
    */
   if (d->snoop_by != NULL)
   {
      char foo[MAX_STRING_LENGTH];
      CHAR_DATA *snoop_ch;

      snoop_ch = d->original != NULL ? d->original : d->character;
      if (snoop_ch != NULL)
         sprintf(foo, "[SNOOP:%s] ", snoop_ch->name);
      write_to_buffer(d->snoop_by, foo, 0);
      write_to_buffer(d->snoop_by, d->outbuf, d->outtop);
   }
   /*
    * OS-dependent output.
    */
   if (d->websocket_active)
   {
      unsigned char *payload;
      size_t payload_len;

      payload = (unsigned char *)malloc((size_t)d->outtop + 1);
      if (payload == NULL)
      {
         d->outtop = 0;
         return FALSE;
      }

      payload_len = sanitize_websocket_text_payload(
          (const unsigned char *)d->outbuf, (size_t)d->outtop, payload, (size_t)d->outtop + 1);
      if (!write_websocket_frame(d, 0x1, payload, payload_len))
      {
         free(payload);
         d->outtop = 0;
         return FALSE;
      }
      free(payload);
      d->outtop = 0;
      return TRUE;
   }
   else if (!write_to_descriptor(d, d->outbuf, d->outtop))
   {
      d->outtop = 0;
      return FALSE;
   }
   else
   {
      d->outtop = 0;
      return TRUE;
   }
}

/*
 * Append onto an output buffer.
 */

#define color_MARGIN 20

void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length)
{
   if (d == NULL)
      return;

   if (d->outbuf == NULL)
   {
      bugf("write_to_buffer with NULL outbuf, string=%s", txt);
      return;
   }

   /*
    * Find length in case caller didn't.
    */
   if (length <= 0)
      length = strlen(txt);

   /*
    * Initial \n\r if needed.
    */
   if (d->outtop == 0 && !d->fcommand)
   {
      d->outbuf[0] = '\n';
      d->outbuf[1] = '\r';
      d->outtop = 2;
   }

   /*
    * Expand the buffer as needed.
    */
   while (d->outtop + length + 1 + color_MARGIN >= d->outsize)
   {
      char *outbuf;

      outbuf = getmem(2 * d->outsize);
      strncpy(outbuf, d->outbuf, d->outtop);
      dispose(d->outbuf, d->outsize);
      d->outbuf = outbuf;
      d->outsize *= 2;
   }

   /*
    * Copy.
    */

   /*
    * ONLY COPY length number of Bytes, otherwise things can go WRONG
    */
   /*
    * MAG mod to strncpy
    */
   /*
    * Old was  strcpy( d->outbuf + d->outtop, txt );
    */

   /*
    * Before Mod2: strncpy( d->outbuf + d->outtop, txt, length );
    */

   /*
    * MAG Mod2: Convert color strings, while copying. Also expand buffer as neede.
    */
   {
      char c;
      char lookup;
      char *dest;
      int count = length;
      CHAR_DATA *ch;
      char *colstr;
      int collen, cnt;

      dest = d->outbuf + d->outtop;

      for (; count > 0;)
      {
         c = *(txt++);
         if (c != '@')
         {
            *(dest++) = c;
            count--;
            continue;
         }
         else
         {
            if (*txt != '@')
            {
               *(dest++) = c;
               count--;
               continue;
            }

            txt++;        /* txt now points at color code. */
            c = *(txt++); /* c is color code. */
            length = length - 3;
            count = count - 3;

            if (c == '@')
            {
               length++;
               *(dest++) = c;
               continue;
            }

            ch = d->original != NULL ? d->original : d->character;
            if (ch != NULL && !IS_SET(ch->config, CONFIG_COLOR))
               continue;
            /* set to default highlight or dim set by player */
            lookup = c;
            if (ch != NULL && !IS_NPC(ch)) /* shouldn't happen, but... */
            {
               if (lookup == '!')
                  lookup = ch->pcdata->hicol;
               else if (lookup == '.')
                  lookup = ch->pcdata->dimcol;
            }

            for (cnt = 0; cnt < MAX_ANSI; cnt++)
               if (ansi_table[cnt].letter == lookup)
                  break;

            if (cnt == MAX_ANSI)
            {
               colstr = ansi_table[10].value;
               collen = ansi_table[10].stlen;
            }
            else
            {
               colstr = ansi_table[cnt].value;
               collen = ansi_table[cnt].stlen;
            }

            while (d->outtop + length + collen + 1 >= d->outsize)
            {
               char *outbuf;

               outbuf = getmem(2 * d->outsize);
               strncpy(outbuf, d->outbuf, d->outtop + length - count);
               dispose(d->outbuf, d->outsize);
               d->outbuf = outbuf;
               d->outsize *= 2;
            }

            dest = d->outbuf + d->outtop + length - count;
            strncpy(dest, colstr, collen);
            dest += collen;

            length = length + collen;
         }
      }
   }

   /*
    * Make sure we have a \0 at the end
    */
   *(d->outbuf + d->outtop + length) = '\0';

   d->outtop += length;
   return;
}

/*
 * Raw socket write — no compression, no framing.  Used internally and by
 * write_to_descriptor when MCCP2 compression is not active.
 */
static bool write_raw_to_socket(DESCRIPTOR_DATA *d, const char *txt, int length)
{
   int iStart;
   int nWrite;
   int nBlock;

   for (iStart = 0; iStart < length; iStart += nWrite)
   {
      nBlock = UMIN(length - iStart, 4096);
#ifdef HAVE_OPENSSL
      if (d->tls_active)
      {
         nWrite = SSL_write((SSL *)d->ssl, txt + iStart, nBlock);
         if (nWrite <= 0)
         {
            ERR_print_errors_fp(stderr);
            return FALSE;
         }
      }
      else
#endif
      {
         if ((nWrite = write(d->descriptor, txt + iStart, nBlock)) < 0)
         {
            perror("Write_to_descriptor");
            return FALSE;
         }
      }
   }
   return TRUE;
}

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If MCCP2 compression is active the data is zlib-deflated before writing.
 */
bool write_to_descriptor(DESCRIPTOR_DATA *d, char *txt, int length)
{
   if (length <= 0)
      length = strlen(txt);

   if (d->mccp2_active && d->mccp2_zout)
   {
      /* Compress through the persistent deflate stream, flushing after each
       * call so the client receives a complete, decompressible chunk. */
      z_stream *z = (z_stream *)d->mccp2_zout;
      unsigned char cbuf[8192];

      z->next_in = (Bytef *)txt;
      z->avail_in = (uInt)length;

      do
      {
         z->next_out = cbuf;
         z->avail_out = sizeof(cbuf);

         int ret = deflate(z, Z_SYNC_FLUSH);
         if (ret < 0)
            return FALSE;

         int produced = (int)(sizeof(cbuf) - z->avail_out);
         if (produced > 0)
         {
            if (!write_raw_to_socket(d, (char *)cbuf, produced))
               return FALSE;
         }
      } while (z->avail_out == 0); /* output buffer was full; more may remain */

      return TRUE;
   }

   return write_raw_to_socket(d, txt, length);
}

/*
 * =========================================================================
 * TLS context initialization
 * =========================================================================
 */
#ifdef HAVE_OPENSSL
bool init_tls_context(const char *cert_file, const char *key_file)
{
   SSL_CTX *ctx;
   SSL_library_init();
   OpenSSL_add_all_algorithms();
   SSL_load_error_strings();
   ctx = SSL_CTX_new(TLS_server_method());
   if (!ctx)
   {
      ERR_print_errors_fp(stderr);
      return FALSE;
   }
   SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
   if (SSL_CTX_use_certificate_chain_file(ctx, cert_file) <= 0 ||
       SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0 ||
       SSL_CTX_check_private_key(ctx) != 1)
   {
      ERR_print_errors_fp(stderr);
      SSL_CTX_free(ctx);
      return FALSE;
   }
   global_ssl_ctx = ctx;
   return TRUE;
}
#endif /* HAVE_OPENSSL */

/*
 * =========================================================================
 * MSSP boot-time counting
 * =========================================================================
 */
void init_mssp_counts(void)
{
   extern int top_area;
   extern int top_mob_index;
   extern int top_obj_index;
   extern int top_room;
   extern int top_help;
   int i;

   mssp_area_count = top_area;
   mssp_mob_count = top_mob_index;
   mssp_obj_count = top_obj_index;
   mssp_room_count = top_room;
   mssp_help_count = top_help;
   mssp_race_count = MAX_RACE;
   mssp_skill_count = 0;
   for (i = 0; i < MAX_SKILL; i++)
   {
      extern const struct skill_type skill_table[];
      if (skill_table[i].name != NULL)
         mssp_skill_count++;
   }
   boot_time = time(NULL);
}

/*
 * =========================================================================
 * Low-level telnet write helpers
 * =========================================================================
 */
static void telnet_send_raw(DESCRIPTOR_DATA *d, const unsigned char *bytes, int len)
{
   write_to_descriptor(d, (char *)bytes, len);
}

/*
 * =========================================================================
 * MSSP (Mud Server Status Protocol, telnet option 70)
 * =========================================================================
 */

static void mssp_append_str(unsigned char *buf, int *i, const char *name, const char *val)
{
   buf[(*i)++] = MSSP_BYTE_VAR;
   while (*name)
      buf[(*i)++] = (unsigned char)*name++;
   buf[(*i)++] = MSSP_BYTE_VAL;
   while (*val)
      buf[(*i)++] = (unsigned char)*val++;
}

static void mssp_append_int(unsigned char *buf, int *i, const char *name, int val)
{
   char tmp[64];
   snprintf(tmp, sizeof(tmp), "%d", val);
   mssp_append_str(buf, i, name, tmp);
}

void send_mssp_data(DESCRIPTOR_DATA *d)
{
   unsigned char buf[4096];
   int i = 0;
   time_t uptime;
   extern int cur_players;
   extern int max_players;

   /* IAC SB MSSP */
   buf[i++] = (unsigned char)IAC;
   buf[i++] = (unsigned char)SB;
   buf[i++] = (unsigned char)TELOPT_MSSP;

   mssp_append_str(buf, &i, "NAME", mudnamenocolor);
   mssp_append_str(buf, &i, "CODEBASE", "ACK!TNG 4.3.1");
   mssp_append_str(buf, &i, "CONTACT", MSSP_CONTACT);
   mssp_append_str(buf, &i, "WEBSITE", MSSP_WEBSITE);
   mssp_append_str(buf, &i, "ICON", MSSP_ICON_URL);
   mssp_append_str(buf, &i, "LOCATION", MSSP_LOCATION);
   mssp_append_str(buf, &i, "LANGUAGE", "English");
   mssp_append_str(buf, &i, "GENRE", "Fantasy");
   mssp_append_str(buf, &i, "GAMEPLAY", "Hack and Slash");
   mssp_append_str(buf, &i, "STATUS", "Live");
   mssp_append_str(buf, &i, "FAMILY", "Diku");
   mssp_append_str(buf, &i, "SUBGENRE", "Merc");
   mssp_append_int(buf, &i, "PORT", global_port);
   mssp_append_int(buf, &i, "PLAYERS", cur_players);
   mssp_append_int(buf, &i, "MAXPLAYERS", max_players);
   mssp_append_int(buf, &i, "AREAS", mssp_area_count);
   mssp_append_int(buf, &i, "HELPFILES", mssp_help_count);
   mssp_append_int(buf, &i, "MOBILES", mssp_mob_count);
   mssp_append_int(buf, &i, "OBJECTS", mssp_obj_count);
   mssp_append_int(buf, &i, "ROOMS", mssp_room_count);
   mssp_append_int(buf, &i, "CLASSES", MAX_CLASS);
   mssp_append_int(buf, &i, "RACES", mssp_race_count);
   mssp_append_int(buf, &i, "SKILLS", mssp_skill_count);
   uptime = (boot_time > 0) ? (time(NULL) - boot_time) : 0;
   mssp_append_int(buf, &i, "UPTIME", (int)uptime);
   mssp_append_str(buf, &i, "SSL", (global_tls_port > 0) ? "1" : "0");
   mssp_append_str(buf, &i, "ANSI", "1");
   mssp_append_str(buf, &i, "MCCP", "1");
   mssp_append_str(buf, &i, "MSDP", "1");
   mssp_append_str(buf, &i, "GMCP", "1");

   /* IAC SE */
   buf[i++] = (unsigned char)IAC;
   buf[i++] = (unsigned char)SE;

   telnet_send_raw(d, buf, i);
}

/*
 * =========================================================================
 * MSDP (Mud Server Data Protocol, telnet option 69)
 * =========================================================================
 */
void msdp_send_var(DESCRIPTOR_DATA *d, const char *name, const char *value)
{
   unsigned char buf[1024];
   int i = 0;

   buf[i++] = (unsigned char)IAC;
   buf[i++] = (unsigned char)SB;
   buf[i++] = (unsigned char)TELOPT_MSDP;
   buf[i++] = MSDP_BYTE_VAR;
   while (*name)
      buf[i++] = (unsigned char)*name++;
   buf[i++] = MSDP_BYTE_VAL;
   while (*value)
      buf[i++] = (unsigned char)*value++;
   buf[i++] = (unsigned char)IAC;
   buf[i++] = (unsigned char)SE;
   telnet_send_raw(d, buf, i);
}

void msdp_send_var_int(DESCRIPTOR_DATA *d, const char *name, int value)
{
   char tmp[32];
   snprintf(tmp, sizeof(tmp), "%d", value);
   msdp_send_var(d, name, tmp);
}

void msdp_send_room_exits(DESCRIPTOR_DATA *d, CHAR_DATA *ch)
{
   unsigned char buf[1024];
   int i = 0;
   int door;
   static const char *const dir_name[] = {"north", "east", "south", "west", "up", "down"};

   if (!ch || !ch->in_room)
      return;

   buf[i++] = (unsigned char)IAC;
   buf[i++] = (unsigned char)SB;
   buf[i++] = (unsigned char)TELOPT_MSDP;
   buf[i++] = MSDP_BYTE_VAR;
   {
      const char *n = "ROOM_EXITS";
      while (*n)
         buf[i++] = (unsigned char)*n++;
   }
   buf[i++] = MSDP_BYTE_VAL;
   buf[i++] = MSDP_TABLE_OPEN;

   for (door = 0; door < 6; door++)
   {
      EXIT_DATA *pexit = ch->in_room->exit[door];
      if (pexit && pexit->to_room)
      {
         char vnum_str[16];
         const char *dname = dir_name[door];
         snprintf(vnum_str, sizeof(vnum_str), "%d", pexit->to_room->vnum);
         buf[i++] = MSDP_BYTE_VAR;
         while (*dname)
            buf[i++] = (unsigned char)*dname++;
         buf[i++] = MSDP_BYTE_VAL;
         {
            const char *v = vnum_str;
            while (*v)
               buf[i++] = (unsigned char)*v++;
         }
      }
   }

   buf[i++] = MSDP_TABLE_CLOSE;
   buf[i++] = (unsigned char)IAC;
   buf[i++] = (unsigned char)SE;
   telnet_send_raw(d, buf, i);
}

void process_msdp_subneg(DESCRIPTOR_DATA *d, unsigned char *payload, int len)
{
   int i = 0;

   if (!d->msdp_active)
   {
      d->msdp_active = TRUE;
      d->msdp_reported = 0;
   }

   while (i < len)
   {
      if (payload[i] == MSDP_BYTE_VAR && i + 1 < len)
      {
         char var_name[128];
         int vi = 0;
         i++;
         while (i < len && payload[i] != MSDP_BYTE_VAL && vi < (int)sizeof(var_name) - 1)
            var_name[vi++] = (char)payload[i++];
         var_name[vi] = '\0';
         if (i < len && payload[i] == MSDP_BYTE_VAL)
         {
            char val_str[256];
            int vsi = 0;
            i++;
            while (i < len && payload[i] != MSDP_BYTE_VAR && vsi < (int)sizeof(val_str) - 1)
               val_str[vsi++] = (char)payload[i++];
            val_str[vsi] = '\0';

            if (!strcmp(var_name, "REPORT"))
            {
               if (!strcmp(val_str, "CHARACTER_NAME"))
                  d->msdp_reported |= MSDP_BIT_CHARACTER_NAME;
               else if (!strcmp(val_str, "HEALTH"))
                  d->msdp_reported |= MSDP_BIT_HEALTH;
               else if (!strcmp(val_str, "HEALTH_MAX"))
                  d->msdp_reported |= MSDP_BIT_HEALTH_MAX;
               else if (!strcmp(val_str, "MANA"))
                  d->msdp_reported |= MSDP_BIT_MANA;
               else if (!strcmp(val_str, "MANA_MAX"))
                  d->msdp_reported |= MSDP_BIT_MANA_MAX;
               else if (!strcmp(val_str, "MOVEMENT"))
                  d->msdp_reported |= MSDP_BIT_MOVEMENT;
               else if (!strcmp(val_str, "MOVEMENT_MAX"))
                  d->msdp_reported |= MSDP_BIT_MOVEMENT_MAX;
               else if (!strcmp(val_str, "LEVEL"))
                  d->msdp_reported |= MSDP_BIT_LEVEL;
               else if (!strcmp(val_str, "EXPERIENCE"))
                  d->msdp_reported |= MSDP_BIT_EXPERIENCE;
               else if (!strcmp(val_str, "EXPERIENCE_TNL"))
                  d->msdp_reported |= MSDP_BIT_EXPERIENCE_TNL;
               else if (!strcmp(val_str, "GOLD"))
                  d->msdp_reported |= MSDP_BIT_GOLD;
               else if (!strcmp(val_str, "ALIGNMENT"))
                  d->msdp_reported |= MSDP_BIT_ALIGNMENT;
               else if (!strcmp(val_str, "HITROLL"))
                  d->msdp_reported |= MSDP_BIT_HITROLL;
               else if (!strcmp(val_str, "DAMROLL"))
                  d->msdp_reported |= MSDP_BIT_DAMROLL;
               else if (!strcmp(val_str, "AC"))
                  d->msdp_reported |= MSDP_BIT_AC;
               else if (!strcmp(val_str, "ROOM_NAME"))
                  d->msdp_reported |= MSDP_BIT_ROOM_NAME;
               else if (!strcmp(val_str, "ROOM_VNUM"))
                  d->msdp_reported |= MSDP_BIT_ROOM_VNUM;
               else if (!strcmp(val_str, "ROOM_TERRAIN"))
                  d->msdp_reported |= MSDP_BIT_ROOM_TERRAIN;
               else if (!strcmp(val_str, "ROOM_EXITS"))
                  d->msdp_reported |= MSDP_BIT_ROOM_EXITS;
            }
            else if (!strcmp(var_name, "UNREPORT"))
            {
               if (!strcmp(val_str, "CHARACTER_NAME"))
                  d->msdp_reported &= ~MSDP_BIT_CHARACTER_NAME;
               /* etc. — clear the matching bit */
            }
            else if (!strcmp(var_name, "SEND"))
            {
               /* Immediate one-shot send */
               CHAR_DATA *ch = d->character;
               if (ch != NULL && !IS_NPC(ch))
               {
                  if (!strcmp(val_str, "HEALTH"))
                     msdp_send_var_int(d, "HEALTH", ch->hit);
                  else if (!strcmp(val_str, "LEVEL"))
                     msdp_send_var_int(d, "LEVEL", ch->level);
               }
            }
            else if (!strcmp(var_name, "RESET"))
            {
               d->msdp_reported = 0;
            }
            else if (!strcmp(var_name, "LIST"))
            {
               /* Send supported variable list */
               msdp_send_var(d, "REPORTABLE_VARIABLES",
                             "CHARACTER_NAME HEALTH HEALTH_MAX MANA MANA_MAX MOVEMENT MOVEMENT_MAX "
                             "LEVEL EXPERIENCE EXPERIENCE_TNL GOLD ALIGNMENT HITROLL DAMROLL AC "
                             "ROOM_NAME ROOM_VNUM ROOM_TERRAIN ROOM_EXITS");
            }
         }
      }
      else
         i++;
   }
}

void msdp_update(void)
{
   DESCRIPTOR_DATA *d;

   for (d = first_desc; d != NULL; d = d->next)
   {
      CHAR_DATA *ch;

      if (!d->msdp_active || d->connected != CON_PLAYING)
         continue;

      ch = d->character;
      if (ch == NULL || IS_NPC(ch))
         continue;

      if (d->msdp_reported & MSDP_BIT_CHARACTER_NAME)
         msdp_send_var(d, "CHARACTER_NAME", ch->name ? ch->name : "");
      if (d->msdp_reported & MSDP_BIT_HEALTH)
         msdp_send_var_int(d, "HEALTH", ch->hit);
      if (d->msdp_reported & MSDP_BIT_HEALTH_MAX)
         msdp_send_var_int(d, "HEALTH_MAX", ch->max_hit);
      if (d->msdp_reported & MSDP_BIT_MANA)
         msdp_send_var_int(d, "MANA", ch->mana);
      if (d->msdp_reported & MSDP_BIT_MANA_MAX)
         msdp_send_var_int(d, "MANA_MAX", ch->max_mana);
      if (d->msdp_reported & MSDP_BIT_MOVEMENT)
         msdp_send_var_int(d, "MOVEMENT", ch->move);
      if (d->msdp_reported & MSDP_BIT_MOVEMENT_MAX)
         msdp_send_var_int(d, "MOVEMENT_MAX", ch->max_move);
      if (d->msdp_reported & MSDP_BIT_LEVEL)
         msdp_send_var_int(d, "LEVEL", ch->level);
      if (d->msdp_reported & MSDP_BIT_EXPERIENCE)
         msdp_send_var_int(d, "EXPERIENCE", ch->exp);
      if (d->msdp_reported & MSDP_BIT_GOLD)
         msdp_send_var_int(d, "GOLD", ch->gold);
      if (d->msdp_reported & MSDP_BIT_ALIGNMENT)
         msdp_send_var_int(d, "ALIGNMENT", ch->alignment);
      if (d->msdp_reported & MSDP_BIT_HITROLL)
         msdp_send_var_int(d, "HITROLL", ch->hitroll);
      if (d->msdp_reported & MSDP_BIT_DAMROLL)
         msdp_send_var_int(d, "DAMROLL", ch->damroll);
      if (d->msdp_reported & MSDP_BIT_AC)
         msdp_send_var_int(d, "AC", ch->armor);
      if (d->msdp_reported & MSDP_BIT_ROOM_NAME && ch->in_room)
         msdp_send_var(d, "ROOM_NAME", ch->in_room->name ? ch->in_room->name : "");
      if (d->msdp_reported & MSDP_BIT_ROOM_VNUM && ch->in_room)
         msdp_send_var_int(d, "ROOM_VNUM", ch->in_room->vnum);
      if (d->msdp_reported & MSDP_BIT_ROOM_EXITS)
         msdp_send_room_exits(d, ch);
   }
}

/*
 * =========================================================================
 * GMCP (Generic Mud Communication Protocol, telnet option 201)
 * =========================================================================
 */

/* JSON builder helpers */
static void json_append(char *buf, int *pos, int buf_size, const char *s)
{
   while (*s && *pos < buf_size - 1)
      buf[(*pos)++] = *s++;
   buf[*pos] = '\0';
}

static void json_str_escape(char *buf, int *pos, int buf_size, const char *s)
{
   json_append(buf, pos, buf_size, "\"");
   while (*s && *pos < buf_size - 2)
   {
      char c = *s++;
      if (c == '"' || c == '\\')
      {
         buf[(*pos)++] = '\\';
      }
      buf[(*pos)++] = c;
   }
   buf[*pos] = '\0';
   json_append(buf, pos, buf_size, "\"");
}

static void json_int_val(char *buf, int *pos, int buf_size, int val)
{
   char tmp[32];
   snprintf(tmp, sizeof(tmp), "%d", val);
   json_append(buf, pos, buf_size, tmp);
}

void gmcp_send(DESCRIPTOR_DATA *d, const char *package, const char *json)
{
   unsigned char buf[4096];
   int i = 0;

   buf[i++] = (unsigned char)IAC;
   buf[i++] = (unsigned char)SB;
   buf[i++] = (unsigned char)TELOPT_GMCP;
   while (*package && i < (int)sizeof(buf) - 4)
      buf[i++] = (unsigned char)*package++;
   buf[i++] = ' ';
   while (*json && i < (int)sizeof(buf) - 3)
      buf[i++] = (unsigned char)*json++;
   buf[i++] = (unsigned char)IAC;
   buf[i++] = (unsigned char)SE;
   telnet_send_raw(d, buf, i);
}

void process_gmcp_subneg(DESCRIPTOR_DATA *d, unsigned char *payload, int len)
{
   char pkg[128];
   int pi = 0;
   int i = 0;

   if (!d->gmcp_active)
      d->gmcp_active = TRUE;

   /* Extract package name (up to first space or end) */
   while (i < len && payload[i] != ' ' && pi < (int)sizeof(pkg) - 1)
      pkg[pi++] = (char)payload[i++];
   pkg[pi] = '\0';

   if (!strcmp(pkg, "Core.Supports.Set") || !strcmp(pkg, "Core.Supports.Add"))
   {
      /* Parse JSON array of "Package N" strings — very simple scan */
      char data[1024];
      int di = 0;
      while (i < len && di < (int)sizeof(data) - 1)
         data[di++] = (char)payload[i++];
      data[di] = '\0';
      if (strstr(data, "Char"))
         d->gmcp_supports |= GMCP_PKG_CHAR;
      if (strstr(data, "Room"))
         d->gmcp_supports |= GMCP_PKG_ROOM;
      if (strstr(data, "Comm"))
         d->gmcp_supports |= GMCP_PKG_COMM;
   }
   else if (!strcmp(pkg, "Core.Supports.Remove"))
   {
      char data[1024];
      int di = 0;
      while (i < len && di < (int)sizeof(data) - 1)
         data[di++] = (char)payload[i++];
      data[di] = '\0';
      if (strstr(data, "Char"))
         d->gmcp_supports &= ~GMCP_PKG_CHAR;
      if (strstr(data, "Room"))
         d->gmcp_supports &= ~GMCP_PKG_ROOM;
      if (strstr(data, "Comm"))
         d->gmcp_supports &= ~GMCP_PKG_COMM;
   }
   else if (!strcmp(pkg, "Core.Hello"))
   {
      /* Acknowledge with server info */
      char json[256];
      int jp = 0;
      json_append(json, &jp, sizeof(json), "{");
      json_append(json, &jp, sizeof(json), "\"name\":");
      json_str_escape(json, &jp, sizeof(json), mudnamenocolor);
      json_append(json, &jp, sizeof(json), ",\"version\":\"4.3.1\"}");
      gmcp_send(d, "Core.Goodbye", json);
   }
}

void gmcp_send_vitals(DESCRIPTOR_DATA *d, CHAR_DATA *ch)
{
   char json[512];
   int jp = 0;

   json_append(json, &jp, sizeof(json), "{");
   json_append(json, &jp, sizeof(json), "\"hp\":");
   json_int_val(json, &jp, sizeof(json), ch->hit);
   json_append(json, &jp, sizeof(json), ",\"hpmax\":");
   json_int_val(json, &jp, sizeof(json), ch->max_hit);
   json_append(json, &jp, sizeof(json), ",\"mp\":");
   json_int_val(json, &jp, sizeof(json), ch->mana);
   json_append(json, &jp, sizeof(json), ",\"mpmax\":");
   json_int_val(json, &jp, sizeof(json), ch->max_mana);
   json_append(json, &jp, sizeof(json), ",\"mv\":");
   json_int_val(json, &jp, sizeof(json), ch->move);
   json_append(json, &jp, sizeof(json), ",\"mvmax\":");
   json_int_val(json, &jp, sizeof(json), ch->max_move);
   json_append(json, &jp, sizeof(json), "}");
   gmcp_send(d, "Char.Vitals", json);
}

void gmcp_send_status(DESCRIPTOR_DATA *d, CHAR_DATA *ch)
{
   char json[512];
   int jp = 0;

   json_append(json, &jp, sizeof(json), "{");
   json_append(json, &jp, sizeof(json), "\"name\":");
   json_str_escape(json, &jp, sizeof(json), ch->name ? ch->name : "");
   json_append(json, &jp, sizeof(json), ",\"level\":");
   json_int_val(json, &jp, sizeof(json), ch->level);
   json_append(json, &jp, sizeof(json), ",\"gold\":");
   json_int_val(json, &jp, sizeof(json), ch->gold);
   json_append(json, &jp, sizeof(json), ",\"exp\":");
   json_int_val(json, &jp, sizeof(json), ch->exp);
   json_append(json, &jp, sizeof(json), "}");
   gmcp_send(d, "Char.Status", json);
}

void gmcp_send_room(DESCRIPTOR_DATA *d, CHAR_DATA *ch)
{
   char json[1024];
   int jp = 0;
   ROOM_INDEX_DATA *room;
   int door;
   bool first_exit = TRUE;
   static const char *const dir_name[] = {"north", "east", "south", "west", "up", "down"};

   if (!ch || !ch->in_room)
      return;
   room = ch->in_room;

   json_append(json, &jp, sizeof(json), "{");
   json_append(json, &jp, sizeof(json), "\"name\":");
   json_str_escape(json, &jp, sizeof(json), room->name ? room->name : "");
   json_append(json, &jp, sizeof(json), ",\"vnum\":");
   json_int_val(json, &jp, sizeof(json), room->vnum);
   json_append(json, &jp, sizeof(json), ",\"exits\":{");

   for (door = 0; door < 6; door++)
   {
      EXIT_DATA *pexit = room->exit[door];
      if (pexit && pexit->to_room)
      {
         if (!first_exit)
            json_append(json, &jp, sizeof(json), ",");
         first_exit = FALSE;
         json_str_escape(json, &jp, sizeof(json), dir_name[door]);
         json_append(json, &jp, sizeof(json), ":");
         json_int_val(json, &jp, sizeof(json), pexit->to_room->vnum);
      }
   }

   json_append(json, &jp, sizeof(json), "}}");
   gmcp_send(d, "Room.Info", json);
}

void gmcp_update(void)
{
   DESCRIPTOR_DATA *d;

   for (d = first_desc; d != NULL; d = d->next)
   {
      CHAR_DATA *ch;

      if (!d->gmcp_active || d->connected != CON_PLAYING)
         continue;

      ch = d->character;
      if (ch == NULL || IS_NPC(ch))
         continue;

      if (d->gmcp_supports & GMCP_PKG_CHAR)
      {
         gmcp_send_vitals(d, ch);
         gmcp_send_status(d, ch);
      }

      if (d->gmcp_supports & GMCP_PKG_ROOM)
      {
         if (ch->in_room)
         {
            unsigned short vnum = (unsigned short)ch->in_room->vnum;
            if (vnum != d->gmcp_last_room)
            {
               d->gmcp_last_room = vnum;
               gmcp_send_room(d, ch);
            }
         }
      }
   }
}

void gmcp_send_channel(DESCRIPTOR_DATA *d, const char *channel, const char *talker,
                       const char *text)
{
   char json[1024];
   int jp = 0;

   if (!d->gmcp_active || !(d->gmcp_supports & GMCP_PKG_COMM))
      return;

   json_append(json, &jp, sizeof(json), "{");
   json_append(json, &jp, sizeof(json), "\"channel\":");
   json_str_escape(json, &jp, sizeof(json), channel ? channel : "");
   json_append(json, &jp, sizeof(json), ",\"talker\":");
   json_str_escape(json, &jp, sizeof(json), talker ? talker : "");
   json_append(json, &jp, sizeof(json), ",\"text\":");
   json_str_escape(json, &jp, sizeof(json), text ? text : "");
   json_append(json, &jp, sizeof(json), "}");
   gmcp_send(d, "Comm.Channel", json);
}

/*
 * =========================================================================
 * MCCP2 (server->client zlib compression, telnet option 86)
 * =========================================================================
 */
void mccp2_start(DESCRIPTOR_DATA *d)
{
   unsigned char header[5];
   z_stream *zout;

   if (d->mccp2_active)
      return;

   /* Send IAC SB MCCP2 IAC SE to signal start of compression */
   header[0] = (unsigned char)IAC;
   header[1] = (unsigned char)SB;
   header[2] = (unsigned char)TELOPT_MCCP2;
   header[3] = (unsigned char)IAC;
   header[4] = (unsigned char)SE;
   telnet_send_raw(d, header, 5);

   zout = (z_stream *)malloc(sizeof(z_stream));
   if (!zout)
      return;
   memset(zout, 0, sizeof(z_stream));
   if (deflateInit(zout, Z_DEFAULT_COMPRESSION) != Z_OK)
   {
      free(zout);
      return;
   }
   d->mccp2_zout = zout;
   d->mccp2_active = TRUE;
}

/*
 * =========================================================================
 * MCCP3 (client->server zlib compression, telnet option 87)
 * =========================================================================
 */
void mccp3_init(DESCRIPTOR_DATA *d)
{
   z_stream *zin;

   if (d->mccp3_active)
      return;

   zin = (z_stream *)malloc(sizeof(z_stream));
   if (!zin)
      return;
   memset(zin, 0, sizeof(z_stream));
   if (inflateInit(zin) != Z_OK)
   {
      free(zin);
      return;
   }
   d->mccp3_zin = zin;
   d->mccp3_active = TRUE;
}

/*
 * =========================================================================
 * Telnet IAC option processing — strips IAC sequences from inbuf and
 * dispatches WILL/DO/DONT/WONT and subnegotiations to protocol handlers.
 * =========================================================================
 */
void process_telnet_options(DESCRIPTOR_DATA *d)
{
   unsigned char *buf = (unsigned char *)d->inbuf;
   int len = d->inbuf_len;
   int i = 0, out = 0;

   while (i < len)
   {
      if (buf[i] != (unsigned char)IAC)
      {
         buf[out++] = buf[i++];
         continue;
      }

      /* We have IAC */
      if (i + 1 >= len)
      {
         /* Incomplete — leave it */
         buf[out++] = buf[i++];
         break;
      }

      unsigned char cmd = buf[i + 1];

      if (cmd == (unsigned char)IAC)
      {
         /* Escaped IAC — pass one through */
         buf[out++] = (unsigned char)IAC;
         i += 2;
         continue;
      }

      if (cmd == (unsigned char)GA || cmd == (unsigned char)NOP)
      {
         /* Suppress — just skip */
         i += 2;
         continue;
      }

      if (cmd == (unsigned char)WILL || cmd == (unsigned char)WONT || cmd == (unsigned char)DO ||
          cmd == (unsigned char)DONT)
      {
         if (i + 2 >= len)
         {
            /* Incomplete — leave it */
            buf[out++] = buf[i++];
            break;
         }

         unsigned char opt = buf[i + 2];
         i += 3;

         if (cmd == (unsigned char)DO)
         {
            if (opt == (unsigned char)TELOPT_MSSP)
               send_mssp_data(d);
            else if (opt == (unsigned char)TELOPT_MSDP)
               d->msdp_active = TRUE;
            else if (opt == (unsigned char)TELOPT_GMCP)
               d->gmcp_active = TRUE;
            else if (opt == (unsigned char)TELOPT_MCCP2)
               mccp2_start(d);
            else if (opt == (unsigned char)TELOPT_MCCP3)
               mccp3_init(d);
         }
         /* WILL/WONT/DONT — no response needed for now */
         continue;
      }

      if (cmd == (unsigned char)SB)
      {
         /* Find IAC SE end of subnegotiation */
         int sb_start = i + 2;
         int j = sb_start;
         bool found_se = FALSE;

         while (j < len - 1)
         {
            if (buf[j] == (unsigned char)IAC && buf[j + 1] == (unsigned char)SE)
            {
               found_se = TRUE;
               break;
            }
            j++;
         }

         if (!found_se)
         {
            /* Incomplete — leave it */
            buf[out++] = buf[i++];
            break;
         }

         unsigned char opt = buf[sb_start];
         unsigned char *payload = buf + sb_start + 1;
         int plen = j - sb_start - 1;

         if (opt == (unsigned char)TELOPT_MSDP)
            process_msdp_subneg(d, payload, plen);
         else if (opt == (unsigned char)TELOPT_GMCP)
            process_gmcp_subneg(d, payload, plen);

         i = j + 2; /* skip past IAC SE */
         continue;
      }

      /* Unknown IAC command — skip just the IAC byte */
      buf[out++] = buf[i++];
   }

   /* Copy any remaining bytes that weren't processed */
   while (i < len)
      buf[out++] = buf[i++];

   d->inbuf_len = out;
   d->inbuf[out] = '\0';
}
