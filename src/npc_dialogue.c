/*
 * npc_dialogue.c — LLM-powered NPC dialogue via tng-ai (POST /v1/chat).
 *
 * Architecture:
 *   Game loop thread: npc_dialogue_dispatch() enqueues requests immediately.
 *   Worker thread:    dequeues, calls tng-ai over HTTP (blocking), enqueues
 *                     responses.
 *   Game loop thread: npc_dialogue_deliver() drains responses each tick and
 *                     calls do_say() for valid NPC+player pairs.
 *
 * Knowledge blocks are loaded at init time from data/knowledge/ and stored in
 * static tables.  Area blocks are per-city; topic blocks are per-topic per-city
 * with a global fallback.  All paths relative to the area/ run directory.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "globals.h"
#include "npc_dialogue.h"
#include "lists.h"
#include "db/db_help.h"

/* =========================================================================
 * Forward declarations for game functions used here.
 * ========================================================================= */

void do_say(CHAR_DATA *ch, char *argument);
void do_crusade(CHAR_DATA *ch, char *argument);

/* =========================================================================
 * Knowledge block tables — populated at init from .kb files.
 * ========================================================================= */

static char *common_block = NULL;
static char *area_blocks[NUM_CITIES] = {NULL};
static char *topic_blocks[NUM_CITIES][NUM_KNOW_FLAGS];

/* Filename stem → KNOW_* bit index (0-based) */
static const struct
{
   const char *stem;
   int index;
} topic_file_map[] = {{"weapons", 0},    {"trade", 1},    {"magic", 2}, {"temple", 3},
                      {"underworld", 4}, {"harbor", 5},   {"guard", 6}, {"history", 7},
                      {"wilderness", 8}, {"politics", 9}, {NULL, -1}};

/* Area filename stem → city index */
static const struct
{
   const char *stem;
   int city;
} area_file_map[] = {{"midgaard", CITY_MIDGAARD}, {"kowloon", CITY_KOWLOON}, {"kiess", CITY_KIESS},
                     {"rakuen", CITY_RAKUEN},     {"mafdet", CITY_MAFDET},   {NULL, -1}};

/* -------------------------------------------------------------------------
 * Map a section header line to a city index.  Returns -1 if not recognised.
 * -------------------------------------------------------------------------*/
static int city_from_section_name(const char *name)
{
   if (!strcmp(name, "GLOBAL"))
      return CITY_GLOBAL;
   if (!strcmp(name, "MIDGAARD"))
      return CITY_MIDGAARD;
   if (!strcmp(name, "KOWLOON"))
      return CITY_KOWLOON;
   if (!strcmp(name, "KIESS"))
      return CITY_KIESS;
   if (!strcmp(name, "RAKUEN"))
      return CITY_RAKUEN;
   if (!strcmp(name, "MAFDET"))
      return CITY_MAFDET;
   return -1;
}

/* -------------------------------------------------------------------------
 * Read an entire file into a malloc'd string.  Returns NULL on failure.
 * -------------------------------------------------------------------------*/
static char *read_file_to_string(const char *path)
{
   FILE *fp;
   long size;
   char *buf;

   fp = fopen(path, "r");
   if (fp == NULL)
      return NULL;

   fseek(fp, 0, SEEK_END);
   size = ftell(fp);
   rewind(fp);

   buf = malloc(size + 1);
   if (buf == NULL)
   {
      fclose(fp);
      return NULL;
   }

   size = (long)fread(buf, 1, size, fp);
   buf[size] = '\0';
   fclose(fp);
   return buf;
}

/* -------------------------------------------------------------------------
 * Load a plain-text .kb file as a single block into *out.
 * If *out is already set it is freed first.
 * -------------------------------------------------------------------------*/
static void load_plain_kb(const char *path, char **out)
{
   char *text = read_file_to_string(path);
   if (text == NULL)
   {
      log_f("load_plain_kb: cannot open %s", path);
      return;
   }
   free(*out);
   *out = text;
}

/* -------------------------------------------------------------------------
 * Load a sectioned topic .kb file.
 *
 * Format (each section ends with a line whose first char is '~'):
 *
 *   GLOBAL
 *   text line...
 *   text line...
 *   ~
 *   MIDGAARD
 *   text line...
 *   ~
 *
 * Parsed sections are stored in topic_blocks[city][topic_index].
 * -------------------------------------------------------------------------*/
static void load_topic_kb(const char *path, int topic_index)
{
#define KB_TEXT_MAX (1024 * 32)
   FILE *fp;
   char line[4096];
   char *text;
   int text_len;
   int current_city;

   fp = fopen(path, "r");
   if (fp == NULL)
   {
      log_f("load_topic_kb: cannot open %s", path);
      return;
   }

   text = malloc(KB_TEXT_MAX);
   if (text == NULL)
   {
      fclose(fp);
      return;
   }

   text[0] = '\0';
   text_len = 0;
   current_city = -1; /* not yet in a section */

   while (fgets(line, sizeof(line), fp) != NULL)
   {
      /* Strip trailing \r\n */
      size_t ln = strlen(line);
      while (ln > 0 && (line[ln - 1] == '\n' || line[ln - 1] == '\r'))
         line[--ln] = '\0';

      /* Tilde alone = end of current section */
      if (line[0] == '~' && line[1] == '\0')
      {
         if (current_city >= 0 && current_city < NUM_CITIES)
         {
            free(topic_blocks[current_city][topic_index]);
            topic_blocks[current_city][topic_index] = strdup(text);
         }
         text[0] = '\0';
         text_len = 0;
         current_city = -1;
         continue;
      }

      /* Check if this line is a city section header */
      if (current_city < 0)
      {
         int city;
         city = city_from_section_name(line);
         if (city >= 0)
         {
            current_city = city;
            text[0] = '\0';
            text_len = 0;
            continue;
         }
         /* Not a recognised header and not in a section — skip */
         continue;
      }

      /* Append line to current section text */
      int needed = text_len + (int)ln + 2; /* +1 newline +1 null */
      if (needed < KB_TEXT_MAX)
      {
         memcpy(text + text_len, line, ln);
         text_len += ln;
         text[text_len++] = '\n';
         text[text_len] = '\0';
      }
   }

   /* Handle file that ends without a trailing ~ */
   if (current_city >= 0 && current_city < NUM_CITIES && text_len > 0)
   {
      free(topic_blocks[current_city][topic_index]);
      topic_blocks[current_city][topic_index] = strdup(text);
   }

   free(text);
   fclose(fp);
#undef KB_TEXT_MAX
}

/* -------------------------------------------------------------------------
 * Load all knowledge blocks from KNOWLEDGE_DIR.
 * -------------------------------------------------------------------------*/
static void load_knowledge_blocks(void)
{
   char path[512];
   int i;

   /* Zero the tables */
   memset(area_blocks, 0, sizeof(area_blocks));
   memset(topic_blocks, 0, sizeof(topic_blocks));
   common_block = NULL;

   /* common.kb */
   snprintf(path, sizeof(path), "%scommon.kb", KNOWLEDGE_DIR);
   load_plain_kb(path, &common_block);

   /* area city .kb files */
   for (i = 0; area_file_map[i].stem != NULL; i++)
   {
      snprintf(path, sizeof(path), "%sarea/%s.kb", KNOWLEDGE_DIR, area_file_map[i].stem);
      load_plain_kb(path, &area_blocks[area_file_map[i].city]);
   }

   /* topic .kb files */
   for (i = 0; topic_file_map[i].stem != NULL; i++)
   {
      snprintf(path, sizeof(path), "%stopic/%s.kb", KNOWLEDGE_DIR, topic_file_map[i].stem);
      load_topic_kb(path, topic_file_map[i].index);
   }

   log_f("NPC dialogue: knowledge blocks loaded.");
}

/* =========================================================================
 * City detection — map a room vnum to a city index.
 * ========================================================================= */

static int city_for_room(int vnum)
{
   if (vnum >= MIDGAARD_VNUM_MIN && vnum <= MIDGAARD_VNUM_MAX)
      return CITY_MIDGAARD;
   if (vnum >= KIESS_VNUM_MIN && vnum <= KIESS_VNUM_MAX)
      return CITY_KIESS;
   if (vnum >= KOWLOON_VNUM_MIN && vnum <= KOWLOON_VNUM_MAX)
      return CITY_KOWLOON;
   if (vnum >= MAFDET_VNUM_MIN && vnum <= MAFDET_VNUM_MAX)
      return CITY_MAFDET;
   if (vnum >= RAKUEN_VNUM_MIN && vnum <= RAKUEN_VNUM_MAX)
      return CITY_RAKUEN;
   return CITY_GLOBAL;
}

/* =========================================================================
 * Conversation history (per NPC instance, heap-allocated).
 * ========================================================================= */

typedef struct
{
   char role[16];
   char content[1024];
} DIALOGUE_TURN;

typedef struct
{
   int count;
   time_t last_time;
   DIALOGUE_TURN turns[MAX_DIALOGUE_TURNS];
} NPC_DLG_STATE;

static NPC_DLG_STATE *get_or_create_dlg_state(CHAR_DATA *npc)
{
   if (npc->dlg_state == NULL)
      npc->dlg_state = calloc(1, sizeof(NPC_DLG_STATE));
   return (NPC_DLG_STATE *)npc->dlg_state;
}

/* =========================================================================
 * Request / response structs and thread-safe queues.
 * ========================================================================= */

typedef struct
{
   char role[16];
   char content[1024];
} DIALOGUE_TURN_COPY;

typedef struct npc_dlg_req
{
   CHAR_DATA *npc;
   CHAR_DATA *player;
   char player_name[50];
   char system_prompt[16384];
   char help_context[1600]; /* injected help/shelp entries; empty string if none */
   int history_count;
   DIALOGUE_TURN_COPY history[MAX_REQUEST_TURNS];
   dlg_delivery_t delivery; /* DLG_DELIVER_SAY or DLG_DELIVER_CRUSADE */
   struct npc_dlg_req *next;
} NPC_DLG_REQ;

typedef struct npc_dlg_resp
{
   CHAR_DATA *npc;
   CHAR_DATA *player;
   char response_text[1024];
   dlg_delivery_t delivery; /* copied from the originating request */
   struct npc_dlg_resp *next;
} NPC_DLG_RESP;

/* Request queue: game loop writes, worker reads */
static pthread_mutex_t req_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t req_cond = PTHREAD_COND_INITIALIZER;
static NPC_DLG_REQ *req_head = NULL;

/* Response queue: worker writes, game loop reads */
static pthread_mutex_t resp_mutex = PTHREAD_MUTEX_INITIALIZER;
static NPC_DLG_RESP *resp_head = NULL;

/* =========================================================================
 * JSON helpers.
 * ========================================================================= */

/* Append src to dst (capacity cap), keeping null-termination. */
static void safe_append(char *dst, size_t cap, const char *src)
{
   size_t dlen = strlen(dst);
   size_t slen = strlen(src);
   if (dlen + slen + 1 > cap)
      slen = cap - dlen - 1;
   memcpy(dst + dlen, src, slen);
   dst[dlen + slen] = '\0';
}

/*
 * JSON-escape src into dst (capacity cap).
 * Escapes: \ " control chars.
 */
static void json_escape(char *dst, size_t cap, const char *src)
{
   size_t di = 0;
   const unsigned char *s = (const unsigned char *)src;

   while (*s && di + 4 < cap)
   {
      if (*s == '"' || *s == '\\')
      {
         dst[di++] = '\\';
         dst[di++] = (char)*s;
      }
      else if (*s == '\n')
      {
         dst[di++] = '\\';
         dst[di++] = 'n';
      }
      else if (*s == '\r')
      {
         dst[di++] = '\\';
         dst[di++] = 'r';
      }
      else if (*s == '\t')
      {
         dst[di++] = '\\';
         dst[di++] = 't';
      }
      else if (*s < 0x20)
      {
         /* skip other control characters */
      }
      else
      {
         dst[di++] = (char)*s;
      }
      s++;
   }
   dst[di] = '\0';
}

/*
 * Build the JSON chat completions request body into buf (capacity cap).
 */
static void build_json_request(char *buf, size_t cap, const NPC_DLG_REQ *req)
{
   char escaped[16384];
   int i;

   char header[128];

   buf[0] = '\0';
   snprintf(header, sizeof(header),
            "{\"model\":\"%s\",\"max_tokens\":%d,\"temperature\":0.7,\"messages\":[", TNGAI_MODEL,
            TNGAI_MAX_TOKENS);
   safe_append(buf, cap, header);

   /* System message */
   json_escape(escaped, sizeof(escaped), req->system_prompt);
   safe_append(buf, cap, "{\"role\":\"system\",\"content\":\"");
   safe_append(buf, cap, escaped);
   safe_append(buf, cap, "\"}");

   /* History turns */
   for (i = 0; i < req->history_count; i++)
   {
      json_escape(escaped, sizeof(escaped), req->history[i].content);
      safe_append(buf, cap, ",{\"role\":\"");
      safe_append(buf, cap, req->history[i].role);
      safe_append(buf, cap, "\",\"content\":\"");
      safe_append(buf, cap, escaped);
      safe_append(buf, cap, "\"}");
   }

   safe_append(buf, cap, "]}");
}

/*
 * Extract the assistant content string from an OpenAI-compatible response.
 * Writes into out (capacity cap).  Returns TRUE on success.
 */
static bool parse_json_response(const char *json, char *out, size_t cap)
{
   const char *p;
   const char *end;
   size_t len;

   out[0] = '\0';

   p = strstr(json, "\"response\":");
   if (p == NULL)
      return FALSE;

   p += strlen("\"response\":");
   while (*p == ' ' || *p == '\t')
      p++;
   if (*p != '"')
      return FALSE;
   p++; /* skip opening quote */

   end = p;
   while (*end && *end != '"')
   {
      if (*end == '\\')
         end++; /* skip escaped char */
      if (*end)
         end++;
   }

   len = (size_t)(end - p);
   if (len >= cap)
      len = cap - 1;

   /* Copy with basic escape handling (\n → newline, \" → ", \\ → \) */
   size_t di = 0;
   for (size_t si = 0; si < len && di + 1 < cap; si++)
   {
      if (p[si] == '\\' && si + 1 < len)
      {
         si++;
         if (p[si] == 'n')
            out[di++] = '\n';
         else if (p[si] == '"')
            out[di++] = '"';
         else if (p[si] == '\\')
            out[di++] = '\\';
         else
            out[di++] = p[si];
      }
      else
      {
         out[di++] = p[si];
      }
   }
   out[di] = '\0';

   /* Strip trailing whitespace / newlines */
   while (di > 0 && (out[di - 1] == '\n' || out[di - 1] == '\r' || out[di - 1] == ' '))
      out[--di] = '\0';

   return (di > 0);
}

/* =========================================================================
 * URL components — parsed once from TNGAI_URL at init time.
 * ========================================================================= */

static char tngai_host[256];
static char tngai_port[8];
static char tngai_path[256];

/*
 * Parse "http://host:port/path" into the static buffers above.
 * Returns TRUE on success.
 */
static bool parse_tngai_url(void)
{
   const char *env = getenv("TNGAI_URL");
   const char *url = (env != NULL && env[0] != '\0') ? env : TNGAI_URL_DEFAULT;
   const char *p;
   const char *host_start;
   const char *host_end;
   const char *port_start;
   const char *path_start;

   /* Skip "http://" */
   if (strncmp(url, "http://", 7) != 0)
      return FALSE;
   p = url + 7;
   host_start = p;

   /* Find host:port boundary or path */
   host_end = strpbrk(p, ":/");
   if (host_end == NULL)
   {
      strncpy(tngai_host, host_start, sizeof(tngai_host) - 1);
      strncpy(tngai_port, "80", sizeof(tngai_port) - 1);
      strncpy(tngai_path, "/", sizeof(tngai_path) - 1);
      return TRUE;
   }

   /* Copy host */
   {
      size_t hlen = (size_t)(host_end - host_start);
      if (hlen >= sizeof(tngai_host))
         hlen = sizeof(tngai_host) - 1;
      memcpy(tngai_host, host_start, hlen);
      tngai_host[hlen] = '\0';
   }

   if (*host_end == ':')
   {
      port_start = host_end + 1;
      path_start = strchr(port_start, '/');
      if (path_start == NULL)
      {
         strncpy(tngai_port, port_start, sizeof(tngai_port) - 1);
         strncpy(tngai_path, "/", sizeof(tngai_path) - 1);
      }
      else
      {
         size_t plen = (size_t)(path_start - port_start);
         if (plen >= sizeof(tngai_port))
            plen = sizeof(tngai_port) - 1;
         memcpy(tngai_port, port_start, plen);
         tngai_port[plen] = '\0';
         strncpy(tngai_path, path_start, sizeof(tngai_path) - 1);
      }
   }
   else
   {
      strncpy(tngai_port, "80", sizeof(tngai_port) - 1);
      strncpy(tngai_path, host_end, sizeof(tngai_path) - 1);
   }

   return TRUE;
}

/* =========================================================================
 * HTTP call to tng-ai via POSIX sockets.
 * ========================================================================= */

static void call_tngai(const NPC_DLG_REQ *req, char *response_text, size_t cap)
{
   struct addrinfo hints, *res, *rp;
   int sockfd = -1;
   char request_json[1024 * 24];
   char http_req[1024 * 26];
   char *resp_buf = NULL;
   size_t resp_len = 0;
   size_t resp_cap = 0;
   struct timeval tv;
   int json_len;
   int http_len;

   response_text[0] = '\0';

   build_json_request(request_json, sizeof(request_json), req);
   json_len = (int)strlen(request_json);

   /* Build HTTP request */
   http_len = snprintf(http_req, sizeof(http_req),
                       "POST %s HTTP/1.1\r\n"
                       "Host: %s:%s\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: %d\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "%s",
                       tngai_path, tngai_host, tngai_port, json_len, request_json);

   /* Resolve host */
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(tngai_host, tngai_port, &hints, &res) != 0)
   {
      log_f("call_tngai: getaddrinfo failed for %s:%s", tngai_host, tngai_port);
      return;
   }

   /* Connect to first available address */
   for (rp = res; rp != NULL; rp = rp->ai_next)
   {
      sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sockfd < 0)
         continue;

      /* Set timeouts */
      tv.tv_sec = TNGAI_TIMEOUT;
      tv.tv_usec = 0;
      setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
      setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

      if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
         break;

      close(sockfd);
      sockfd = -1;
   }
   freeaddrinfo(res);

   if (sockfd < 0)
   {
      log_f("call_tngai: connect failed to %s:%s", tngai_host, tngai_port);
      return;
   }

   /* Send request */
   {
      size_t sent = 0;
      while (sent < (size_t)http_len)
      {
         ssize_t n = write(sockfd, http_req + sent, (size_t)http_len - sent);
         if (n <= 0)
         {
            log_f("call_tngai: write failed: %s", strerror(errno));
            close(sockfd);
            return;
         }
         sent += (size_t)n;
      }
   }

   /* Read response into a dynamically growing buffer */
   resp_cap = 4096;
   resp_buf = malloc(resp_cap);
   if (resp_buf == NULL)
   {
      close(sockfd);
      return;
   }
   resp_len = 0;

   for (;;)
   {
      ssize_t n;
      if (resp_len + 1024 > resp_cap)
      {
         size_t new_cap = resp_cap + 4096;
         char *new_buf = realloc(resp_buf, new_cap);
         if (new_buf == NULL)
            break;
         resp_buf = new_buf;
         resp_cap = new_cap;
      }
      n = read(sockfd, resp_buf + resp_len, resp_cap - resp_len - 1);
      if (n <= 0)
         break;
      resp_len += (size_t)n;
   }
   resp_buf[resp_len] = '\0';
   close(sockfd);

   /* Find body after \r\n\r\n header terminator */
   {
      const char *body = strstr(resp_buf, "\r\n\r\n");
      if (body != NULL)
      {
         body += 4;
         parse_json_response(body, response_text, cap);
      }
      else
      {
         log_f("call_tngai: malformed HTTP response (no header terminator)");
      }
   }

   free(resp_buf);
}

/* =========================================================================
 * Worker thread.
 * ========================================================================= */

static void *npc_dialogue_worker(void *unused)
{
   (void)unused;

   for (;;)
   {
      NPC_DLG_REQ *req;

      pthread_mutex_lock(&req_mutex);
      while (req_head == NULL)
         pthread_cond_wait(&req_cond, &req_mutex);
      req = req_head;
      req_head = req->next;
      pthread_mutex_unlock(&req_mutex);

      char response_text[1024];
      call_tngai(req, response_text, sizeof(response_text));

      if (response_text[0] != '\0')
      {
         NPC_DLG_RESP *resp = malloc(sizeof(NPC_DLG_RESP));
         if (resp != NULL)
         {
            resp->npc = req->npc;
            resp->player = req->player;
            resp->delivery = req->delivery;
            strncpy(resp->response_text, response_text, sizeof(resp->response_text) - 1);
            resp->response_text[sizeof(resp->response_text) - 1] = '\0';

            pthread_mutex_lock(&resp_mutex);
            resp->next = resp_head;
            resp_head = resp;
            pthread_mutex_unlock(&resp_mutex);
         }
      }
      else
      {
         /* No response — clear the pending flag via a sentinel response with empty text */
         NPC_DLG_RESP *resp = malloc(sizeof(NPC_DLG_RESP));
         if (resp != NULL)
         {
            resp->npc = req->npc;
            resp->player = req->player;
            resp->delivery = req->delivery;
            resp->response_text[0] = '\0';

            pthread_mutex_lock(&resp_mutex);
            resp->next = resp_head;
            resp_head = resp;
            pthread_mutex_unlock(&resp_mutex);
         }
      }

      free(req);
   }
   return NULL;
}

/* =========================================================================
 * Accent post-processing.
 * ========================================================================= */

struct syl_type
{
   const char *old;
   const char *new;
};

/* Kowloon: terse, elliptic — drop filler words */
static const struct syl_type accent_kowloon[] = {{"please ", ""}, {"Please ", ""}, {" please", ""},
                                                 {"very ", ""},   {"quite ", ""},  {"just ", ""},
                                                 {"", ""}};

/* Midgaard: bureaucratic, ledger idiom */
static const struct syl_type accent_midgaard[] = {{"okay", "duly noted"},
                                                  {"Okay", "Duly noted"},
                                                  {"sure,", "it is recorded,"},
                                                  {"Sure,", "It is recorded,"},
                                                  {"", ""}};

/* Mafdet: formal titles, oath references */
static const struct syl_type accent_mafdet[] = {{"yes,", "sworn true,"},
                                                {"Yes,", "Sworn true,"},
                                                {"goodbye", "oaths kept"},
                                                {"Goodbye", "Oaths kept"},
                                                {"bye", "oaths kept"},
                                                {"Bye", "Oaths kept"},
                                                {"", ""}};

/* Kiess: measured, watchful, frontier caution */
static const struct syl_type accent_kiess[] = {{"watch out", "attend the verge"},
                                               {"Watch out", "Attend the verge"},
                                               {"be careful", "attend carefully"},
                                               {"Be careful", "Attend carefully"},
                                               {"", ""}};

/* Rakuen: warm but weary, garden metaphors */
static const struct syl_type accent_rakuen[] = {{"but ", "and yet "},
                                                {"But ", "And yet "},
                                                {"however,", "and yet,"},
                                                {"However,", "And yet,"},
                                                {"", ""}};

static const struct syl_type *accent_tables[MAX_ACCENT] = {
    NULL,            /* ACCENT_NONE */
    accent_midgaard, /* ACCENT_MIDGAARD */
    accent_kowloon,  /* ACCENT_KOWLOON */
    accent_mafdet,   /* ACCENT_MAFDET */
    accent_kiess,    /* ACCENT_KIESS */
    accent_rakuen    /* ACCENT_RAKUEN */
};

/*
 * Apply the accent table for npc->pIndexData->accent to src, writing into dst.
 * Uses the same scanning approach as slur_text in act_comm.c.
 */
static void accent_text(CHAR_DATA *npc, const char *src, char *dst, size_t cap)
{
   const struct syl_type *table;
   const char *p;
   size_t di = 0;
   int accent;

   if (npc->pIndexData == NULL)
   {
      strncpy(dst, src, cap - 1);
      dst[cap - 1] = '\0';
      return;
   }

   accent = npc->pIndexData->accent;
   if (accent <= ACCENT_NONE || accent >= MAX_ACCENT || accent_tables[accent] == NULL)
   {
      strncpy(dst, src, cap - 1);
      dst[cap - 1] = '\0';
      return;
   }

   table = accent_tables[accent];
   p = src;

   while (*p && di + 1 < cap)
   {
      int matched = 0;
      int i;

      for (i = 0; table[i].old[0] != '\0'; i++)
      {
         size_t olen = strlen(table[i].old);
         if (strncmp(p, table[i].old, olen) == 0)
         {
            /* Copy replacement */
            const char *rep = table[i].new;
            while (*rep && di + 1 < cap)
               dst[di++] = *rep++;
            p += olen;
            matched = 1;
            break;
         }
      }

      if (!matched)
         dst[di++] = *p++;
   }

   dst[di] = '\0';
}

/* =========================================================================
 * Race speech inclination strings (indexed by race slot in race_table).
 * ========================================================================= */

static const char *race_speech_inclination[10] = {
    /* 0 HUMAN    */ "Pragmatic and procedural; reference systems, records, and"
                     " institutional context.",
    /* 1 KHENARI  */
    "Formal, precise, and ritualistic; reference ledgers,"
    " death-rites, and proper procedure. Speak with bureaucratic accuracy.",
    /* 2 KHEPHARI */
    "Ancient and methodical; unhurried, grounded in physical"
    " observation, speak of cycles and enduring things."
    " May pause as if listening to the earth.",
    /* 3 ASHBORN  */
    "Direct, warm, and elemental; say what you mean without"
    " preamble, use forge and fire metaphors, value action over talk.",
    /* 4 UMBRAL   */
    "Precise and observant; emphasize what is missing rather than"
    " what is present, cool and pragmatic, slightly unsettling.",
    /* 5 RIVENNID */
    "Associative and patient; speak slowly as if consulting a"
    " shared network, describe systems intuitively, pause mid-sentence.",
    /* 6 DELTARI  */
    "Empirical and quietly confident; use water-flow metaphors,"
    " ask probing questions, observe before speaking.",
    /* 7 USHABTI  */
    "Judicious and measured; weigh words as permanent record,"
    " reference historical precedent, speak with gravitas and formal phrasing.",
    /* 8 SERATHI  */
    "Precise, direct, and altitude-aware; speak literally and"
    " expect the same in return, use sky-line and hunt metaphors,"
    " observe before committing.",
    /* 9 KETHARI  */
    "Unhurried and ancient; speak with empirical certainty,"
    " reference water, patience, and long observation,"
    " every sentence carries data.",
};

static const char *accent_instructions[MAX_ACCENT] = {
    NULL, /* ACCENT_NONE */
    "Speak with a Midgaard accent: clipped bureaucratic phrasing, ledger idiom,"
    " formal titles, reference institutions and records.",
    "Speak with a Kowloon accent: terse, elliptic, proverb-laden."
    " Drop filler words. Let silences carry weight.",
    "Speak with a Mafdet accent: formal titles, trade jargon, oath references,"
    " swift declarative sentences.",
    "Speak with a Kiess accent: measured, watchful phrasing, frontier caution."
    " Choose words carefully. Say only what you mean.",
    "Speak with a Rakuen accent: warm but weary, garden metaphors, communal framing."
    " Reference shared work and collective stakes.",
};

/* =========================================================================
 * Help/shelp context collection for KNOW_HELPS mobs.
 * ========================================================================= */

/* Common English stop-words to skip when extracting search keywords. */
static const char *help_stop_words[] = {"the",  "and", "is", "are", "you", "how", "what", "can",
                                        "does", "do",  "a",  "an",  "in",  "to",  "of",   "it",
                                        "i",    "me",  "my", "we",  "for", "on",  "at",   "be",
                                        "was",  "if",  "or", "not", NULL};

static bool is_stop_word(const char *w)
{
   int i;
   for (i = 0; help_stop_words[i] != NULL; i++)
      if (!str_cmp(w, help_stop_words[i]))
         return TRUE;
   return FALSE;
}

/*
 * Query help/shelp DB for entries whose keyword matches any word in `message`.
 * Writes up to `cap` bytes of formatted context to `out`.
 */
static void collect_help_context(const char *message, char *out, size_t cap)
{
   char msg_copy[512];
   char word[64];
   const char *p;
   /* Track matched keywords to deduplicate. */
   char seen_kw[4][128];
   int match_count = 0;
   int i;

   out[0] = '\0';

   /* Skip greet cues — they carry no question keywords. */
   if (strncmp(message, "[GREET]", 7) == 0)
      return;

   snprintf(msg_copy, sizeof(msg_copy), "%s", message);

   p = msg_copy;
   while (*p != '\0' && match_count < 4)
   {
      char kw_buf[128];
      char text_buf[512];
      int lev_out = 0;
      bool already;

      p = one_argument((char *)p, word);

      if (word[0] == '\0')
         break;
      if (strlen(word) <= 3)
         continue;
      if (is_stop_word(word))
         continue;

      /* Try help then shelp; use first hit. */
      if (!db_help_lookup(word, 100, kw_buf, sizeof(kw_buf), text_buf, sizeof(text_buf), &lev_out))
      {
         if (!db_shelp_lookup(word, 100, kw_buf, sizeof(kw_buf), text_buf, sizeof(text_buf),
                              &lev_out))
            continue;
      }

      /* Deduplicate by matched keyword. */
      already = FALSE;
      for (i = 0; i < match_count; i++)
         if (!str_cmp(seen_kw[i], kw_buf))
         {
            already = TRUE;
            break;
         }
      if (already)
         continue;

      snprintf(seen_kw[match_count], sizeof(seen_kw[match_count]), "%s", kw_buf);
      match_count++;

      /* Append to output. */
      {
         char header[128];
         char snippet[304];
         size_t tlen;

         snprintf(header, sizeof(header), "[HELP: %s]\n", kw_buf);
         if (strlen(out) + strlen(header) + 1 >= cap)
            break;
         strncat(out, header, cap - strlen(out) - 1);

         tlen = strlen(text_buf);
         if (tlen > 300)
            tlen = 300;
         memcpy(snippet, text_buf, tlen);
         snippet[tlen] = '\n';
         snippet[tlen + 1] = '\0';

         if (strlen(out) + strlen(snippet) + 1 >= cap)
            break;
         strncat(out, snippet, cap - strlen(out) - 1);
      }
   }
}

#ifdef UNIT_TEST_NPC_DIALOGUE
/* Thin wrapper exposing collect_help_context for unit tests. */
void npc_dialogue_test_collect_help_context(const char *message, char *out, size_t cap)
{
   collect_help_context(message, out, cap);
}
#endif

/* =========================================================================
 * System prompt assembly.
 * ========================================================================= */

/* Context struct and callback for lore injection into system prompt. */
struct lore_inject_ctx
{
   char *buf;
   size_t cap;
   int count;
};

static void lore_inject_cb(const char *keyword, const char *body, void *ud)
{
   struct lore_inject_ctx *ctx = (struct lore_inject_ctx *)ud;
   char lore_header[128];
   size_t tlen;

   if (ctx->count == 0)
      safe_append(ctx->buf, ctx->cap, "LORE:\n\n");
   snprintf(lore_header, sizeof(lore_header), "[LORE: %s]\n", keyword);
   safe_append(ctx->buf, ctx->cap, lore_header);
   tlen = strlen(body);
   if (tlen > 1024)
   {
      char lore_chunk[1031];
      memcpy(lore_chunk, body, 1024);
      memcpy(lore_chunk + 1024, "[...]\n", 7);
      safe_append(ctx->buf, ctx->cap, lore_chunk);
   }
   else
   {
      safe_append(ctx->buf, ctx->cap, body);
   }
   safe_append(ctx->buf, ctx->cap, "\n");
   ctx->count++;
}

static void build_system_prompt(char *buf, size_t cap, CHAR_DATA *npc, const char *help_context)
{
   int city;
   int i;
   int ai_knowledge;
   int accent;
   int race;

   buf[0] = '\0';

   if (npc->pIndexData == NULL)
      return;

   ai_knowledge = npc->pIndexData->ai_knowledge;
   accent = npc->pIndexData->accent;
   race = npc->pIndexData->race;
   city = (npc->in_room != NULL) ? city_for_room(npc->in_room->vnum) : CITY_GLOBAL;

   /* 1. Common knowledge block */
   if (common_block != NULL)
   {
      safe_append(buf, cap, common_block);
      safe_append(buf, cap, "\n");
   }

   /* 2. Area knowledge block */
   if (city > CITY_GLOBAL && area_blocks[city] != NULL)
   {
      safe_append(buf, cap, area_blocks[city]);
      safe_append(buf, cap, "\n");
   }

   /* 3. Topic blocks (one per set bit, city-specific then global fallback) */
   for (i = 0; i < NUM_KNOW_FLAGS; i++)
   {
      const char *block;

      if (!(ai_knowledge & (1 << i)))
         continue;

      block = topic_blocks[city][i];
      if (block == NULL)
         block = topic_blocks[CITY_GLOBAL][i];
      if (block != NULL)
      {
         safe_append(buf, cap, block);
         safe_append(buf, cap, "\n");
      }
   }

   /* 4. Lore injection: up to 3 entries matching this NPC's lore_flags */
   if (npc->lore_flags != 0)
   {
      struct lore_inject_ctx lctx;
      lctx.buf = buf;
      lctx.cap = cap;
      lctx.count = 0;
      db_lore_collect_by_flags(npc->lore_flags, 3, lore_inject_cb, &lctx);
   }

   /* 5. Accent instruction */
   if (accent > ACCENT_NONE && accent < MAX_ACCENT && accent_instructions[accent] != NULL)
   {
      safe_append(buf, cap, accent_instructions[accent]);
      safe_append(buf, cap, "\n");
   }

   /* 6. Race speech inclination */
   if (race >= 0 && race < 10 && race_speech_inclination[race] != NULL)
   {
      char race_line[512];
      snprintf(race_line, sizeof(race_line), "Your nature colors your speech: %s\n",
               race_speech_inclination[race]);
      safe_append(buf, cap, race_line);
   }

   /* 7. NPC persona (most critical — always appended last) */
   if (npc->pIndexData->ai_prompt != NULL)
   {
      safe_append(buf, cap, npc->pIndexData->ai_prompt);
      safe_append(buf, cap, "\n");
   }

   /* 7b. Help/shelp file context (injected per-message when KNOW_HELPS is set) */
   if (help_context != NULL && help_context[0] != '\0')
   {
      safe_append(buf, cap, "HELP FILE CONTEXT (use this to inform your answer):\n");
      safe_append(buf, cap, help_context);
      safe_append(buf, cap, "\n");
   }

   /* 8. Behavioral guardrails */
   safe_append(buf, cap,
               "Keep responses to 1-3 sentences. Stay in character at all times."
               " Never say you are an AI. Speak only of what your character would know.\n");
}

/* =========================================================================
 * Prompt injection defence.
 * ========================================================================= */

/* Tokens that indicate a structural injection attempt; stripped from input. */
static const char *STRIP_TOKENS[] = {"[INST]",       "<<SYS>>",    "<|system|>", "</s>",
                                     "<|im_start|>", "<|im_end|>", NULL};

/* Phrases that signal a persona-override attempt; trigger keyword short-circuit. */
static const char *INJECTION_TRIGGERS[] = {"ignore previous",       "ignore all previous",
                                           "disregard previous",    "forget previous",
                                           "you are now",           "act as",
                                           "pretend you are",       "pretend to be",
                                           "your new instructions", NULL};

void npc_dialogue_sanitize_input(char *dst, const char *src)
{
   const char *p = src;
   size_t di = 0;
   size_t cap = 512; /* dst is always 512 bytes per API contract */

   while (*p && di + 1 < cap)
   {
      int stripped = 0;
      int i;
      size_t tlen;

      /* Check for tokens to strip entirely (case-insensitive) */
      for (i = 0; STRIP_TOKENS[i] != NULL; i++)
      {
         tlen = strlen(STRIP_TOKENS[i]);
         if (strncasecmp(p, STRIP_TOKENS[i], tlen) == 0)
         {
            p += tlen;
            stripped = 1;
            break;
         }
      }

      if (stripped)
         continue;

      /* Replace angle brackets to prevent structural role confusion */
      if (*p == '<')
         dst[di++] = '(';
      else if (*p == '>')
         dst[di++] = ')';
      else
         dst[di++] = *p;

      p++;
   }
   dst[di] = '\0';
}

/* =========================================================================
 * Public API: dispatch.
 * ========================================================================= */

void npc_dialogue_dispatch(CHAR_DATA *npc, CHAR_DATA *player, const char *message)
{
   NPC_DLG_STATE *state;
   NPC_DLG_REQ *req;
   char sanitized[512];
   char user_turn[560]; /* player name (up to ~30) + ": " + sanitized (up to 511) + NUL */
   int i;

   /* Don't stack requests */
   if (npc->dlg_pending)
      return;

   /* Sanitize input: strip role-boundary tokens, replace angle brackets */
   npc_dialogue_sanitize_input(sanitized, message);

   state = get_or_create_dlg_state(npc);

   /* Expire history after silence */
   if (state->count > 0 && state->last_time > 0 &&
       (time(NULL) - state->last_time) > DIALOGUE_HISTORY_EXPIRY)
   {
      state->count = 0;
   }

   /* Build user turn: "PlayerName: message" */
   snprintf(user_turn, sizeof(user_turn), "%s: %s", player->name, sanitized);

   /* Keyword short-circuit: detect persona-override attempts.
    * Enqueue a fixed confused refusal directly without hitting the API. */
   for (i = 0; INJECTION_TRIGGERS[i] != NULL; i++)
   {
      if (!str_infix(INJECTION_TRIGGERS[i], sanitized))
      {
         NPC_DLG_RESP *resp = malloc(sizeof(NPC_DLG_RESP));
         log_f("npc_dialogue: injection attempt by %s blocked: %.80s", player->name, sanitized);
         if (resp != NULL)
         {
            resp->npc = npc;
            resp->player = player;
            strncpy(resp->response_text, "I am not certain I follow your meaning.",
                    sizeof(resp->response_text) - 1);
            resp->response_text[sizeof(resp->response_text) - 1] = '\0';
            pthread_mutex_lock(&resp_mutex);
            resp->next = resp_head;
            resp_head = resp;
            pthread_mutex_unlock(&resp_mutex);
         }
         return;
      }
   }

   /* Allocate request (malloc — never touches the MUD allocator from worker) */
   req = malloc(sizeof(NPC_DLG_REQ));
   if (req == NULL)
      return;
   memset(req, 0, sizeof(NPC_DLG_REQ));

   req->npc = npc;
   req->player = player;
   req->delivery = DLG_DELIVER_SAY;
   strncpy(req->player_name, player->name, sizeof(req->player_name) - 1);

   /* Collect help/shelp context when the mob has KNOW_HELPS set. */
   req->help_context[0] = '\0';
   if (npc->pIndexData != NULL && (npc->pIndexData->ai_knowledge & KNOW_HELPS))
      collect_help_context(sanitized, req->help_context, sizeof(req->help_context));

   /* Build system prompt */
   build_system_prompt(req->system_prompt, sizeof(req->system_prompt), npc, req->help_context);

   /* Serialize history snapshot */
   req->history_count = 0;
   for (i = 0; i < state->count && req->history_count < MAX_REQUEST_TURNS - 1; i++)
   {
      strncpy(req->history[req->history_count].role, state->turns[i].role,
              sizeof(req->history[0].role) - 1);
      strncpy(req->history[req->history_count].content, state->turns[i].content,
              sizeof(req->history[0].content) - 1);
      req->history_count++;
   }

   /* Append user turn as final history entry */
   strncpy(req->history[req->history_count].role, "user", sizeof(req->history[0].role) - 1);
   strncpy(req->history[req->history_count].content, user_turn,
           sizeof(req->history[0].content) - 1);
   req->history_count++;

   /* Append user turn to live history */
   if (state->count < MAX_DIALOGUE_TURNS)
   {
      strncpy(state->turns[state->count].role, "user", sizeof(state->turns[0].role) - 1);
      strncpy(state->turns[state->count].content, user_turn, sizeof(state->turns[0].content) - 1);
      state->count++;
   }
   else
   {
      /* Shift history to make room */
      memmove(&state->turns[0], &state->turns[1], (MAX_DIALOGUE_TURNS - 1) * sizeof(DIALOGUE_TURN));
      strncpy(state->turns[MAX_DIALOGUE_TURNS - 1].role, "user", sizeof(state->turns[0].role) - 1);
      strncpy(state->turns[MAX_DIALOGUE_TURNS - 1].content, user_turn,
              sizeof(state->turns[0].content) - 1);
   }
   state->last_time = time(NULL);

   req->next = NULL;

   /* Enqueue request */
   pthread_mutex_lock(&req_mutex);
   req->next = req_head;
   req_head = req;
   pthread_cond_signal(&req_cond);
   pthread_mutex_unlock(&req_mutex);

   npc->dlg_pending = TRUE;
}

/* =========================================================================
 * Public API: deliver.
 * ========================================================================= */

void npc_dialogue_deliver(void)
{
   NPC_DLG_RESP *list;
   NPC_DLG_RESP *resp;
   NPC_DLG_RESP *next;

   /* Steal the entire response list atomically */
   pthread_mutex_lock(&resp_mutex);
   list = resp_head;
   resp_head = NULL;
   pthread_mutex_unlock(&resp_mutex);

   for (resp = list; resp != NULL; resp = next)
   {
      next = resp->next;

      /* Validate pointers using the MUD's is_free flag */
      if (resp->npc == NULL || resp->npc->is_free || resp->player == NULL || resp->player->is_free)
      {
         if (resp->npc != NULL && !resp->npc->is_free)
            resp->npc->dlg_pending = FALSE;
         free(resp);
         continue;
      }

      /* Clear pending regardless of whether we deliver */
      resp->npc->dlg_pending = FALSE;

      /* For room-based delivery: drop if NPC or player left the room */
      if (resp->delivery != DLG_DELIVER_CRUSADE)
      {
         if (resp->npc->in_room == NULL || resp->player->in_room == NULL ||
             resp->npc->in_room != resp->player->in_room)
         {
            free(resp);
            continue;
         }
      }

      /* Empty text = timed-out or failed request; discard silently */
      if (resp->response_text[0] == '\0')
      {
         free(resp);
         continue;
      }

      /* Apply accent post-processing */
      {
         char accented[1024];
         accent_text(resp->npc, resp->response_text, accented, sizeof(accented));

         /* Append assistant turn to live history */
         NPC_DLG_STATE *state = (NPC_DLG_STATE *)resp->npc->dlg_state;
         if (state != NULL)
         {
            if (state->count < MAX_DIALOGUE_TURNS)
            {
               strncpy(state->turns[state->count].role, "assistant",
                       sizeof(state->turns[0].role) - 1);
               strncpy(state->turns[state->count].content, accented,
                       sizeof(state->turns[0].content) - 1);
               state->count++;
            }
            else
            {
               memmove(&state->turns[0], &state->turns[1],
                       (MAX_DIALOGUE_TURNS - 1) * sizeof(DIALOGUE_TURN));
               strncpy(state->turns[MAX_DIALOGUE_TURNS - 1].role, "assistant",
                       sizeof(state->turns[0].role) - 1);
               strncpy(state->turns[MAX_DIALOGUE_TURNS - 1].content, accented,
                       sizeof(state->turns[0].content) - 1);
            }
            state->last_time = time(NULL);
         }

         if (resp->delivery == DLG_DELIVER_CRUSADE)
            do_crusade(resp->npc, accented);
         else
            do_say(resp->npc, accented);
      }

      free(resp);
   }
}

/* =========================================================================
 * Public API: crusade-channel dispatch.
 * ========================================================================= */

void npc_dialogue_dispatch_crusade(CHAR_DATA *npc, CHAR_DATA *player, const char *message,
                                   const char *quest_context)
{
   NPC_DLG_STATE *state;
   NPC_DLG_REQ *req;
   char sanitized[512];
   char user_turn[560];
   int i;

   if (npc == NULL || player == NULL || message == NULL)
      return;

   /* Don't stack requests */
   if (npc->dlg_pending)
      return;

   npc_dialogue_sanitize_input(sanitized, message);

   state = get_or_create_dlg_state(npc);

   /* Expire history after silence */
   if (state->count > 0 && state->last_time > 0 &&
       (time(NULL) - state->last_time) > DIALOGUE_HISTORY_EXPIRY)
   {
      state->count = 0;
   }

   snprintf(user_turn, sizeof(user_turn), "%s: %s", player->name, sanitized);

   /* Keyword short-circuit for injection attempts */
   for (i = 0; INJECTION_TRIGGERS[i] != NULL; i++)
   {
      if (!str_infix(INJECTION_TRIGGERS[i], sanitized))
      {
         NPC_DLG_RESP *resp = malloc(sizeof(NPC_DLG_RESP));
         log_f("npc_dialogue: crusade injection attempt by %s blocked: %.80s", player->name,
               sanitized);
         if (resp != NULL)
         {
            resp->npc = npc;
            resp->player = player;
            resp->delivery = DLG_DELIVER_CRUSADE;
            strncpy(resp->response_text, "I am not certain I follow your meaning.",
                    sizeof(resp->response_text) - 1);
            resp->response_text[sizeof(resp->response_text) - 1] = '\0';
            pthread_mutex_lock(&resp_mutex);
            resp->next = resp_head;
            resp_head = resp;
            pthread_mutex_unlock(&resp_mutex);
         }
         return;
      }
   }

   req = malloc(sizeof(NPC_DLG_REQ));
   if (req == NULL)
      return;
   memset(req, 0, sizeof(NPC_DLG_REQ));

   req->npc = npc;
   req->player = player;
   req->delivery = DLG_DELIVER_CRUSADE;
   strncpy(req->player_name, player->name, sizeof(req->player_name) - 1);

   /* Build system prompt, then append quest context */
   build_system_prompt(req->system_prompt, sizeof(req->system_prompt), npc, NULL);
   if (quest_context != NULL && quest_context[0] != '\0')
   {
      safe_append(req->system_prompt, sizeof(req->system_prompt),
                  "\nCURRENT CRUSADE QUEST CONTEXT:\n");
      safe_append(req->system_prompt, sizeof(req->system_prompt), quest_context);
      safe_append(req->system_prompt, sizeof(req->system_prompt), "\n");
   }

   /* Serialize history snapshot */
   req->history_count = 0;
   for (i = 0; i < state->count && req->history_count < MAX_REQUEST_TURNS - 1; i++)
   {
      strncpy(req->history[req->history_count].role, state->turns[i].role,
              sizeof(req->history[0].role) - 1);
      strncpy(req->history[req->history_count].content, state->turns[i].content,
              sizeof(req->history[0].content) - 1);
      req->history_count++;
   }

   strncpy(req->history[req->history_count].role, "user", sizeof(req->history[0].role) - 1);
   strncpy(req->history[req->history_count].content, user_turn,
           sizeof(req->history[0].content) - 1);
   req->history_count++;

   /* Append user turn to live history */
   if (state->count < MAX_DIALOGUE_TURNS)
   {
      strncpy(state->turns[state->count].role, "user", sizeof(state->turns[0].role) - 1);
      strncpy(state->turns[state->count].content, user_turn, sizeof(state->turns[0].content) - 1);
      state->count++;
   }
   else
   {
      memmove(&state->turns[0], &state->turns[1], (MAX_DIALOGUE_TURNS - 1) * sizeof(DIALOGUE_TURN));
      strncpy(state->turns[MAX_DIALOGUE_TURNS - 1].role, "user", sizeof(state->turns[0].role) - 1);
      strncpy(state->turns[MAX_DIALOGUE_TURNS - 1].content, user_turn,
              sizeof(state->turns[0].content) - 1);
   }
   state->last_time = time(NULL);

   req->next = NULL;

   pthread_mutex_lock(&req_mutex);
   req->next = req_head;
   req_head = req;
   pthread_cond_signal(&req_cond);
   pthread_mutex_unlock(&req_mutex);

   npc->dlg_pending = TRUE;
}

/* =========================================================================
 * Public API: init.
 * ========================================================================= */

void npc_dialogue_init(void)
{
   pthread_t worker;
   int i, j;

   /* Zero topic table */
   for (i = 0; i < NUM_CITIES; i++)
      for (j = 0; j < NUM_KNOW_FLAGS; j++)
         topic_blocks[i][j] = NULL;

   if (!parse_tngai_url())
   {
      log_f("npc_dialogue_init: failed to parse TNGAI_URL");
      return;
   }
   log_f("npc_dialogue_init: tng-ai at %s:%s%s", tngai_host, tngai_port, tngai_path);
   load_knowledge_blocks();

   if (pthread_create(&worker, NULL, npc_dialogue_worker, NULL) != 0)
   {
      log_f("npc_dialogue_init: failed to create worker thread");
      return;
   }
   pthread_detach(worker);

   log_f("NPC dialogue system initialised.");
}
