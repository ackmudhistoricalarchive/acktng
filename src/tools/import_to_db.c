/* import_to_db.c — ACK!TNG flat-file → PostgreSQL migration tool.
 *
 * Standalone binary (not linked with the server).  Reads all six content
 * stores from the filesystem and inserts them into PostgreSQL.
 *
 * Usage:
 *   ./tools/import_to_db [connstr]
 *
 * connstr defaults to the contents of data/db.conf (relative to the area/
 * directory, which is the CWD at runtime).  If absent, libpq PG* env vars
 * are used as a fallback.
 *
 * Build: make tools/import_to_db  (from src/)
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <libpq-fe.h>

/* Area flags (must match src/headers/config.h) */
#define IL_AREA_PAYAREA 1
#define IL_AREA_TELEPORT 2
#define IL_AREA_BUILDING 4
#define IL_AREA_NOSHOW 8
#define IL_AREA_NO_ROOM_AFF 16

/* Loot and trade table sizes (must match src/headers/config.h) */
#define IL_MAX_LOOT 9
#define IL_MAX_TRADE 5

/* Clan table (must match src/headers/config.h) */
#define IL_MAX_CLAN 11

/* -----------------------------------------------------------------------
 * Connection state
 * ----------------------------------------------------------------------- */

static PGconn *conn;
static int errors;
static FILE *errlog; /* error/warning log file (opened in main) */

/* -----------------------------------------------------------------------
 * SQL execution helpers
 * ----------------------------------------------------------------------- */

static int exec_sql(const char *sql)
{
   PGresult *res = PQexec(conn, sql);
   int ok = (PQresultStatus(res) == PGRES_COMMAND_OK || PQresultStatus(res) == PGRES_TUPLES_OK);
   if (!ok)
      fprintf(errlog, "SQL error: %s\n--- %s\n", PQresultErrorMessage(res), sql);
   PQclear(res);
   return ok;
}

static int exec_params(const char *sql, int n, const char *const *vals)
{
   PGresult *res = PQexecParams(conn, sql, n, NULL, vals, NULL, NULL, 0);
   ExecStatusType s = PQresultStatus(res);
   int ok = (s == PGRES_COMMAND_OK || s == PGRES_TUPLES_OK);
   if (!ok)
   {
      fprintf(errlog, "ERROR [params]: %s\n", PQresultErrorMessage(res));
      errors++;
   }
   PQclear(res);
   return ok;
}

/* Execute a parameterised INSERT ... RETURNING id.
 * Returns the new/updated row id, or -1 on error. */
static int exec_returning_id(const char *sql, int n, const char *const *vals)
{
   PGresult *res = PQexecParams(conn, sql, n, NULL, vals, NULL, NULL, 0);
   int id = -1;
   if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
      id = atoi(PQgetvalue(res, 0, 0));
   else
   {
      fprintf(errlog, "ERROR [returning_id]: %s\n", PQresultErrorMessage(res));
      errors++;
   }
   PQclear(res);
   return id;
}

/* -----------------------------------------------------------------------
 * Parameter-set builder
 *
 * Provides a small array of string buffers for integer→string conversion
 * so callers can build PQexecParams vals[] arrays without manual snprintf.
 * ----------------------------------------------------------------------- */

#define IL_MAX_PARAMS 72

typedef struct
{
   char bufs[IL_MAX_PARAMS][32]; /* storage for int/ull→string conversions */
   const char *vals[IL_MAX_PARAMS];
   int n;
} PSet;

static void ps_init(PSet *p)
{
   p->n = 0;
}

static void ps_int(PSet *p, long v)
{
   snprintf(p->bufs[p->n], 32, "%ld", v);
   p->vals[p->n] = p->bufs[p->n];
   p->n++;
}

static void ps_ull(PSet *p, unsigned long long v)
{
   /* PostgreSQL BIGINT is signed; cast so the bit pattern is preserved. */
   snprintf(p->bufs[p->n], 32, "%lld", (long long)v);
   p->vals[p->n] = p->bufs[p->n];
   p->n++;
}

/* Add a string parameter.  The string must outlive the PSet. */
static void ps_str(PSet *p, const char *s)
{
   p->vals[p->n] = s;
   p->n++;
}

/* -----------------------------------------------------------------------
 * Standalone fread utilities
 *
 * These replicate the behaviour of fread_letter / fread_number /
 * fread_string etc. from src/db.c and src/ssm.c but without any
 * server dependencies.  All heap strings are malloc'd; callers must free().
 * ----------------------------------------------------------------------- */

/* Skip whitespace and return the next character (like fread_letter). */
static int il_fread_letter(FILE *fp)
{
   int c;
   do
   {
      c = fgetc(fp);
   } while (c != EOF && isspace((unsigned char)c));
   return c;
}

/* Read a signed integer, with '|' meaning bitwise-OR of successive values. */
static long il_fread_number(FILE *fp)
{
   int c;
   long number;
   int negative;

   do
   {
      c = fgetc(fp);
   } while (c != EOF && isspace((unsigned char)c));

   negative = 0;
   if (c == '-')
   {
      negative = 1;
      c = fgetc(fp);
   }

   if (!isdigit((unsigned char)c))
   {
      /* Unexpected character — consume to end of line and return 0. */
      while (c != EOF && c != '\n')
         c = fgetc(fp);
      return 0;
   }

   number = 0;
   while (isdigit((unsigned char)c))
   {
      number = number * 10 + (c - '0');
      c = fgetc(fp);
   }

   if (c == '|')
      number |= il_fread_number(fp);
   else if (c != EOF)
      ungetc(c, fp);

   return negative ? -number : number;
}

/* Read an unsigned long long (for 64-bit act_flags / extra_flags). */
static unsigned long long il_fread_number_ull(FILE *fp)
{
   int c;
   unsigned long long number;

   do
   {
      c = fgetc(fp);
   } while (c != EOF && isspace((unsigned char)c));

   if (!isdigit((unsigned char)c))
   {
      while (c != EOF && c != '\n')
         c = fgetc(fp);
      return 0;
   }

   number = 0;
   while (isdigit((unsigned char)c))
   {
      number = number * 10 + (unsigned long long)(c - '0');
      c = fgetc(fp);
   }

   if (c == '|')
      number |= il_fread_number_ull(fp);
   else if (c != EOF)
      ungetc(c, fp);

   return number;
}

/* Read a tilde-terminated string.  Returns a malloc'd string (never NULL).
 * Leading whitespace is skipped.  A bare '~' produces an empty string.
 * Carriage-returns (\r) are stripped; newlines (\n) are kept as-is. */
static char *il_fread_string(FILE *fp)
{
   int c;
   char *buf;
   size_t len = 0, cap = 256;

   do
   {
      c = fgetc(fp);
   } while (c != EOF && isspace((unsigned char)c));

   if (c == '~' || c == EOF)
   {
      buf = malloc(1);
      buf[0] = '\0';
      return buf;
   }

   buf = malloc(cap);
   while (c != '~' && c != EOF)
   {
      if (len + 2 >= cap)
      {
         cap *= 2;
         buf = realloc(buf, cap);
      }
      if (c != '\r')
         buf[len++] = (char)c;
      c = fgetc(fp);
   }
   buf[len] = '\0';
   return buf;
}

/* Read a whitespace-delimited word.  Returns a malloc'd string (never NULL). */
static char *il_fread_word(FILE *fp)
{
   int c;
   char *buf;
   size_t len = 0, cap = 64;

   do
   {
      c = fgetc(fp);
   } while (c != EOF && isspace((unsigned char)c));

   buf = malloc(cap);
   while (c != EOF && !isspace((unsigned char)c))
   {
      if (len + 2 >= cap)
      {
         cap *= 2;
         buf = realloc(buf, cap);
      }
      buf[len++] = (char)c;
      c = fgetc(fp);
   }
   if (c != EOF)
      ungetc(c, fp);
   buf[len] = '\0';
   return buf;
}

/* Consume the rest of the current line (like fread_to_eol). */
static void il_fread_to_eol(FILE *fp)
{
   int c;
   do
   {
      c = fgetc(fp);
   } while (c != EOF && c != '\n');
}

/* Return the rest of the current line as a malloc'd string, with leading
 * spaces stripped and trailing whitespace trimmed (like fsave_to_eol). */
static char *il_fsave_to_eol(FILE *fp)
{
   int c;
   char *buf;
   size_t len = 0, cap = 128;

   /* Skip leading spaces/tabs (but not newline). */
   do
   {
      c = fgetc(fp);
   } while (c == ' ' || c == '\t');

   buf = malloc(cap);
   while (c != EOF && c != '\n')
   {
      if (len + 2 >= cap)
      {
         cap *= 2;
         buf = realloc(buf, cap);
      }
      if (c != '\r')
         buf[len++] = (char)c;
      c = fgetc(fp);
   }
   /* Trim trailing whitespace. */
   while (len > 0 && (buf[len - 1] == ' ' || buf[len - 1] == '\t'))
      len--;
   buf[len] = '\0';
   return buf;
}

/* -----------------------------------------------------------------------
 * Generic file utilities
 * ----------------------------------------------------------------------- */

/* Read an entire file into a heap buffer.  Caller must free().
 * Returns NULL on error. */
static char *read_file(const char *path)
{
   FILE *fp;
   long size;
   char *buf;

   fp = fopen(path, "r");
   if (!fp)
      return NULL;
   fseek(fp, 0, SEEK_END);
   size = ftell(fp);
   rewind(fp);
   buf = malloc((size_t)size + 1);
   if (!buf)
   {
      fclose(fp);
      return NULL;
   }
   size = (long)fread(buf, 1, (size_t)size, fp);
   buf[size] = '\0';
   fclose(fp);
   return buf;
}

/* Strip trailing newline / carriage return / whitespace from s in-place. */
static void strip_trailing(char *s)
{
   size_t n = strlen(s);
   while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t'))
      s[--n] = '\0';
}

/* -----------------------------------------------------------------------
 * Help / shelp file importer
 *
 * File format (one entry per file):
 *   level <N>
 *   keywords <words...>
 *   ---
 *   <body text>
 * ----------------------------------------------------------------------- */

static int import_one_helpfile(const char *path, const char *filename, const char *table)
{
   char *buf;
   char *p;
   char *line_end;
   char level_str[32] = "0";
   char keywords[256] = "";
   char *body_start;
   int in_header;
   const char *vals[4];
   char lev_copy[32];

   buf = read_file(path);
   if (!buf)
   {
      fprintf(errlog, "  WARN: cannot read %s\n", path);
      return 0;
   }

   /* Parse header lines until "---" */
   p = buf;
   in_header = 1;
   while (in_header && *p)
   {
      char *eol;
      line_end = strchr(p, '\n');
      eol = line_end ? line_end : p + strlen(p);
      *eol = '\0';
      strip_trailing(p);

      if (strncmp(p, "level ", 6) == 0)
      {
         strncpy(level_str, p + 6, sizeof(level_str) - 1);
         strip_trailing(level_str);
      }
      else if (strncmp(p, "keywords ", 9) == 0)
      {
         strncpy(keywords, p + 9, sizeof(keywords) - 1);
         strip_trailing(keywords);
      }
      else if (strcmp(p, "---") == 0)
      {
         in_header = 0;
         if (line_end)
            *line_end = '\n';
         p = line_end ? line_end + 1 : eol;
         break;
      }

      if (line_end)
         *line_end = '\n';
      p = line_end ? line_end + 1 : eol;
   }

   body_start = p;

   strncpy(lev_copy, level_str, sizeof(lev_copy) - 1);

   vals[0] = filename;
   vals[1] = lev_copy;
   vals[2] = keywords;
   vals[3] = body_start;

   {
      char sql[256];
      snprintf(sql, sizeof(sql),
               "INSERT INTO %s (filename, level, keywords, body) "
               "VALUES ($1, $2, $3, $4) "
               "ON CONFLICT (filename) DO UPDATE SET "
               "level = EXCLUDED.level, "
               "keywords = EXCLUDED.keywords, "
               "body = EXCLUDED.body",
               table);
      exec_params(sql, 4, vals);
   }

   free(buf);
   return 1;
}

static int import_helpdir(const char *dirpath, const char *table)
{
   DIR *dp;
   struct dirent *de;
   int count = 0;
   char path[512];

   dp = opendir(dirpath);
   if (!dp)
   {
      fprintf(errlog, "WARN: cannot open %s: %s\n", dirpath, strerror(errno));
      return 0;
   }

   exec_sql("BEGIN");
   while ((de = readdir(dp)) != NULL)
   {
      struct stat st;
      if (de->d_name[0] == '.')
         continue;
      snprintf(path, sizeof(path), "%s/%s", dirpath, de->d_name);
      if (stat(path, &st) != 0 || S_ISDIR(st.st_mode))
         continue;
      import_one_helpfile(path, de->d_name, table);
      count++;
   }
   exec_sql("COMMIT");
   closedir(dp);
   return count;
}

/* -----------------------------------------------------------------------
 * Lore flag parser
 *
 * Converts a space-separated list of flag names to a numeric bit mask.
 * Unknown tokens are silently ignored (forward compat).
 * Writes the decimal result into out_buf (size >= 32).
 * ----------------------------------------------------------------------- */

static void parse_lore_flags(const char *names, char *out_buf, size_t out_sz)
{
   static const struct
   {
      const char *name;
      unsigned long long bit;
   } flag_table[] = {{"MIDGAARD", 1ULL << 0},  {"KIESS", 1ULL << 1},
                     {"KOWLOON", 1ULL << 2},   {"RAKUEN", 1ULL << 3},
                     {"MAFDET", 1ULL << 4},    {"HUMAN", 1ULL << 5},
                     {"KHENARI", 1ULL << 6},   {"KHEPHARI", 1ULL << 7},
                     {"ASHBORN", 1ULL << 8},   {"UMBRAL", 1ULL << 9},
                     {"RIVENNID", 1ULL << 10}, {"DELTARI", 1ULL << 11},
                     {"USHABTI", 1ULL << 12},  {"SERATHI", 1ULL << 13},
                     {"KETHARI", 1ULL << 14},  {NULL, 0}};

   unsigned long long result = 0;
   char tmp[256];
   char *tok;
   int i;

   strncpy(tmp, names, sizeof(tmp) - 1);
   tmp[sizeof(tmp) - 1] = '\0';

   tok = strtok(tmp, " \t");
   while (tok)
   {
      /* upper-case comparison */
      char upper[64];
      int j;
      for (j = 0; tok[j] && j < (int)(sizeof(upper) - 1); j++)
         upper[j] = (char)toupper((unsigned char)tok[j]);
      upper[j] = '\0';

      for (i = 0; flag_table[i].name; i++)
      {
         if (strcmp(upper, flag_table[i].name) == 0)
         {
            result |= flag_table[i].bit;
            break;
         }
      }
      tok = strtok(NULL, " \t");
   }

   snprintf(out_buf, out_sz, "%llu", result);
}

/* -----------------------------------------------------------------------
 * Lore file importer
 *
 * File format:
 *   keywords <words...>
 *   ---
 *   <default body>
 *   [flags <FLAG> [FLAG ...]
 *   ---
 *   <flagged body>]  (may repeat)
 * ----------------------------------------------------------------------- */

static int import_one_lorefile(const char *path, const char *filename)
{
   char *buf;
   char *p;
   char *line_end;
   char keywords[512] = "";
   char topic_id_str[32];
   int seq = 1;
   int in_header = 1;
   int in_entry = 0;
   char flags_str[64] = "0";
   char *entry_start = NULL;
   PGresult *res;
   const char *vals[3];

   buf = read_file(path);
   if (!buf)
   {
      fprintf(errlog, "  WARN: cannot read lore %s\n", path);
      return 0;
   }

   /* Parse keyword header until first "---" */
   p = buf;
   while (in_header && *p)
   {
      char *eol;
      line_end = strchr(p, '\n');
      eol = line_end ? line_end : p + strlen(p);
      *eol = '\0';
      strip_trailing(p);
      if (strncmp(p, "keywords ", 9) == 0)
      {
         strncpy(keywords, p + 9, sizeof(keywords) - 1);
         strip_trailing(keywords);
      }
      else if (strcmp(p, "---") == 0)
      {
         in_header = 0;
         if (line_end)
            *line_end = '\n';
         p = line_end ? line_end + 1 : eol;
         break;
      }
      if (line_end)
         *line_end = '\n';
      p = line_end ? line_end + 1 : eol;
   }

   /* Upsert topic row */
   vals[0] = filename;
   vals[1] = keywords;
   res = PQexecParams(conn,
                      "INSERT INTO lore_topics (filename, keywords) "
                      "VALUES ($1, $2) "
                      "ON CONFLICT (filename) DO UPDATE SET "
                      "keywords = EXCLUDED.keywords "
                      "RETURNING id",
                      2, NULL, vals, NULL, NULL, 0);
   if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
   {
      fprintf(errlog, "ERROR: lore_topics upsert failed for %s: %s\n", filename,
              PQresultErrorMessage(res));
      PQclear(res);
      free(buf);
      errors++;
      return 0;
   }
   strncpy(topic_id_str, PQgetvalue(res, 0, 0), sizeof(topic_id_str) - 1);
   PQclear(res);

   /* Walk remaining text, splitting on "flags <N>\n---\n" boundaries */
   in_entry = 1;
   entry_start = p;
   strncpy(flags_str, "0", sizeof(flags_str) - 1);
   seq = 1;

   while (*p)
   {
      char line_buf[256];
      size_t linelen;

      line_end = strchr(p, '\n');
      if (!line_end)
         break;

      linelen = (size_t)(line_end - p);
      if (linelen >= sizeof(line_buf))
         linelen = sizeof(line_buf) - 1;
      memcpy(line_buf, p, linelen);
      line_buf[linelen] = '\0';
      strip_trailing(line_buf);

      if (strncmp(line_buf, "flags ", 6) == 0)
      {
         /* Flush current entry before starting next. */
         char *seg_end = p;
         if (in_entry && entry_start && entry_start < seg_end)
         {
            char *body = malloc((size_t)(seg_end - entry_start) + 1);
            if (body)
            {
               memcpy(body, entry_start, (size_t)(seg_end - entry_start));
               body[seg_end - entry_start] = '\0';
               strip_trailing(body);
               {
                  char seq_str[16];
                  const char *v4[4];
                  snprintf(seq_str, sizeof(seq_str), "%d", seq);
                  v4[0] = topic_id_str;
                  v4[1] = seq_str;
                  v4[2] = flags_str;
                  v4[3] = body;
                  exec_params("INSERT INTO lore_entries (topic_id, seq, flags, body) "
                              "VALUES ($1, $2, $3, $4) "
                              "ON CONFLICT (topic_id, seq) DO UPDATE SET "
                              "flags = EXCLUDED.flags, body = EXCLUDED.body",
                              4, v4);
                  seq++;
               }
               free(body);
            }
         }
         strip_trailing(line_buf);
         parse_lore_flags(line_buf + 6, flags_str, sizeof(flags_str));
         /* Skip the following "---" line */
         {
            char *next_p = line_end + 1;
            line_end = strchr(next_p, '\n');
            if (line_end)
               next_p = line_end + 1;
            entry_start = next_p;
            p = next_p;
         }
         in_entry = 1;
         continue;
      }

      p = line_end + 1;
   }

   /* Flush final entry */
   if (in_entry && entry_start)
   {
      char *body = strdup(entry_start);
      if (body)
      {
         strip_trailing(body);
         {
            char seq_str[16];
            const char *v4[4];
            snprintf(seq_str, sizeof(seq_str), "%d", seq);
            v4[0] = topic_id_str;
            v4[1] = seq_str;
            v4[2] = flags_str;
            v4[3] = body;
            exec_params("INSERT INTO lore_entries (topic_id, seq, flags, body) "
                        "VALUES ($1, $2, $3, $4) "
                        "ON CONFLICT (topic_id, seq) DO UPDATE SET "
                        "flags = EXCLUDED.flags, body = EXCLUDED.body",
                        4, v4);
         }
         free(body);
      }
   }

   free(buf);
   return 1;
}

static int import_loredir(const char *dirpath)
{
   DIR *dp;
   struct dirent *de;
   int count = 0;
   char path[512];

   dp = opendir(dirpath);
   if (!dp)
   {
      fprintf(errlog, "WARN: cannot open lore dir %s: %s\n", dirpath, strerror(errno));
      return 0;
   }

   exec_sql("BEGIN");
   while ((de = readdir(dp)) != NULL)
   {
      struct stat st;
      if (de->d_name[0] == '.')
         continue;
      snprintf(path, sizeof(path), "%s/%s", dirpath, de->d_name);
      if (stat(path, &st) != 0 || S_ISDIR(st.st_mode))
         continue;
      import_one_lorefile(path, de->d_name);
      count++;
   }
   exec_sql("COMMIT");
   closedir(dp);
   return count;
}

/* -----------------------------------------------------------------------
 * Area section importers
 *
 * Each function receives an already-open FILE* positioned just after the
 * #SECTIONNAME marker.  They consume the section content up to (but not
 * including) the next '#' that starts the following section.
 * ----------------------------------------------------------------------- */

/* Parse the #AREA header block.  Returns the area's database id, or -1 on
 * error.  Sets *revision_out to the area_revision (Q directive) value. */
static int import_area_header(FILE *fp, int *revision_out)
{
   char *name;
   int area_revision = -1;
   int reset_rate = 15;
   int area_number = 0;
   int min_vnum = 0, max_vnum = 65535;
   int level_min = 0, level_max = 0;
   int map_offset = 0;
   int flags = 0;
   char *keyword;
   char *level_label;
   char *owner;
   char *can_read;
   char *can_write;
   char *reset_msg;
   char *music;
   int area_id;
   PSet ps;

   /* First token after #AREA is the area name (tilde-string). */
   name = il_fread_string(fp);

   keyword = strdup("none");
   level_label = strdup("{?? ??}");
   owner = strdup("");
   can_read = strdup("all");
   can_write = strdup("all");
   reset_msg = strdup("You hear the screams of the Dead within your head.");
   music = strdup("");

   for (;;)
   {
      int letter = il_fread_letter(fp);
      if (letter == EOF)
         break;
      if (letter == '#')
      {
         ungetc(letter, fp);
         break;
      }
      switch (letter)
      {
      case 'Q':
         area_revision = (int)il_fread_number(fp);
         break;
      case 'F':
         reset_rate = (int)il_fread_number(fp);
         break;
      case 'N':
         area_number = (int)il_fread_number(fp);
         break;
      case 'V':
         min_vnum = (int)il_fread_number(fp);
         max_vnum = (int)il_fread_number(fp);
         break;
      case 'I':
         level_min = (int)il_fread_number(fp);
         level_max = (int)il_fread_number(fp);
         break;
      case 'X':
         map_offset = (int)il_fread_number(fp);
         break;
      case 'K':
         free(keyword);
         keyword = il_fread_string(fp);
         break;
      case 'L':
         free(level_label);
         level_label = il_fread_string(fp);
         break;
      case 'O':
         free(owner);
         owner = il_fread_string(fp);
         break;
      case 'R':
         free(can_read);
         can_read = il_fread_string(fp);
         break;
      case 'W':
         free(can_write);
         can_write = il_fread_string(fp);
         break;
      case 'U':
         free(reset_msg);
         reset_msg = il_fread_string(fp);
         break;
      case 'C':
         free(music);
         music = il_fread_string(fp);
         break;
      case 'P':
         flags |= IL_AREA_PAYAREA;
         il_fread_to_eol(fp);
         break;
      case 'M':
         flags |= IL_AREA_NO_ROOM_AFF;
         il_fread_to_eol(fp);
         break;
      case 'T':
         flags |= IL_AREA_TELEPORT;
         il_fread_to_eol(fp);
         break;
      case 'B':
         flags |= IL_AREA_BUILDING;
         il_fread_to_eol(fp);
         break;
      case 'S':
         flags |= IL_AREA_NOSHOW;
         il_fread_to_eol(fp);
         break;
      default:
         il_fread_to_eol(fp);
         break;
      }
   }

   if (revision_out)
      *revision_out = area_revision;

   ps_init(&ps);
   ps_str(&ps, name);                  /* $1  name         */
   ps_int(&ps, min_vnum);              /* $2  min_vnum      */
   ps_int(&ps, max_vnum);              /* $3  max_vnum      */
   ps_str(&ps, keyword);               /* $4  keyword       */
   ps_str(&ps, level_label);           /* $5  level_label   */
   ps_int(&ps, area_number);           /* $6  area_number   */
   ps_str(&ps, owner);                 /* $7  owner         */
   ps_int(&ps, level_min);             /* $8  level_min     */
   ps_int(&ps, level_max);             /* $9  level_max     */
   ps_int(&ps, map_offset);            /* $10 map_offset    */
   ps_int(&ps, reset_rate);            /* $11 reset_rate    */
   ps_str(&ps, reset_msg);             /* $12 reset_msg     */
   ps_str(&ps, can_read);              /* $13 can_read      */
   ps_str(&ps, can_write);             /* $14 can_write     */
   ps_str(&ps, music[0] ? music : ""); /* $15 music */
   ps_int(&ps, flags);                 /* $16 flags         */

   area_id =
       exec_returning_id("INSERT INTO areas "
                         "(name, min_vnum, max_vnum, keyword, level_label, area_number, owner, "
                         " level_min, level_max, map_offset, reset_rate, reset_msg, "
                         " can_read, can_write, music, flags) "
                         "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16) "
                         "ON CONFLICT (min_vnum) DO UPDATE SET "
                         "name=EXCLUDED.name, max_vnum=EXCLUDED.max_vnum, "
                         "keyword=EXCLUDED.keyword, level_label=EXCLUDED.level_label, "
                         "area_number=EXCLUDED.area_number, owner=EXCLUDED.owner, "
                         "level_min=EXCLUDED.level_min, level_max=EXCLUDED.level_max, "
                         "map_offset=EXCLUDED.map_offset, reset_rate=EXCLUDED.reset_rate, "
                         "reset_msg=EXCLUDED.reset_msg, can_read=EXCLUDED.can_read, "
                         "can_write=EXCLUDED.can_write, music=EXCLUDED.music, flags=EXCLUDED.flags "
                         "RETURNING id",
                         ps.n, ps.vals);

   free(name);
   free(keyword);
   free(level_label);
   free(owner);
   free(can_read);
   free(can_write);
   free(reset_msg);
   free(music);
   return area_id;
}

/* -----------------------------------------------------------------------
 * #ROOMS
 * ----------------------------------------------------------------------- */

static void import_rooms_section(FILE *fp, int area_id)
{
   for (;;)
   {
      int letter = il_fread_letter(fp);
      if (letter != '#')
         break;

      int vnum = (int)il_fread_number(fp);
      if (vnum == 0)
         break;

      char *name = il_fread_string(fp);
      char *description = il_fread_string(fp);
      int room_flags = (int)il_fread_number(fp);
      int sector_type = (int)il_fread_number(fp);

      {
         PSet ps;
         ps_init(&ps);
         ps_int(&ps, vnum);        /* $1 */
         ps_int(&ps, area_id);     /* $2 */
         ps_str(&ps, name);        /* $3 */
         ps_str(&ps, description); /* $4 */
         ps_int(&ps, room_flags);  /* $5 */
         ps_int(&ps, sector_type); /* $6 */
         exec_params(
             "INSERT INTO rooms (vnum, area_id, name, description, room_flags, sector_type) "
             "VALUES ($1,$2,$3,$4,$5,$6) "
             "ON CONFLICT (vnum) DO UPDATE SET "
             "area_id=EXCLUDED.area_id, name=EXCLUDED.name, "
             "description=EXCLUDED.description, room_flags=EXCLUDED.room_flags, "
             "sector_type=EXCLUDED.sector_type",
             ps.n, ps.vals);
      }

      /* Remove stale exits/extra_descs before re-inserting. */
      {
         char vnum_str[16];
         const char *v1[1];
         snprintf(vnum_str, sizeof(vnum_str), "%d", vnum);
         v1[0] = vnum_str;
         exec_params("DELETE FROM room_exits WHERE room_vnum=$1", 1, v1);
         exec_params("DELETE FROM room_extra_descs WHERE room_vnum=$1", 1, v1);
      }

      /* Sub-records: D (exit), E (extra_desc), S (end). */
      for (;;)
      {
         int sub = il_fread_letter(fp);
         if (sub == 'S')
            break;

         if (sub == 'D')
         {
            int dir = (int)il_fread_number(fp);
            char *edesc = il_fread_string(fp);
            char *ekey = il_fread_string(fp);
            int exit_flags = (int)il_fread_number(fp);
            int key_vnum = (int)il_fread_number(fp);
            int dest_vnum = (int)il_fread_number(fp);

            PSet ps;
            ps_init(&ps);
            ps_int(&ps, vnum);       /* $1 room_vnum   */
            ps_int(&ps, dir);        /* $2 direction   */
            ps_int(&ps, dest_vnum);  /* $3 dest_vnum   */
            ps_int(&ps, exit_flags); /* $4 exit_flags  */
            ps_int(&ps, key_vnum);   /* $5 key_vnum    */
            ps_str(&ps, ekey);       /* $6 keyword     */
            ps_str(&ps, edesc);      /* $7 description */
            exec_params(
                "INSERT INTO room_exits "
                "(room_vnum, direction, dest_vnum, exit_flags, key_vnum, keyword, description) "
                "VALUES ($1,$2,$3,$4,$5,$6,$7) "
                "ON CONFLICT (room_vnum, direction) DO UPDATE SET "
                "dest_vnum=EXCLUDED.dest_vnum, exit_flags=EXCLUDED.exit_flags, "
                "key_vnum=EXCLUDED.key_vnum, keyword=EXCLUDED.keyword, "
                "description=EXCLUDED.description",
                ps.n, ps.vals);
            free(edesc);
            free(ekey);
         }
         else if (sub == 'E')
         {
            char *ekey = il_fread_string(fp);
            char *edesc = il_fread_string(fp);
            PSet ps;
            ps_init(&ps);
            ps_int(&ps, vnum);  /* $1 */
            ps_str(&ps, ekey);  /* $2 */
            ps_str(&ps, edesc); /* $3 */
            exec_params("INSERT INTO room_extra_descs (room_vnum, keyword, description) "
                        "VALUES ($1,$2,$3)",
                        ps.n, ps.vals);
            free(ekey);
            free(edesc);
         }
         else
         {
            fprintf(errlog, "WARN: room %d: unexpected sub-record '%c'\n", vnum, sub);
            il_fread_to_eol(fp);
         }
      }

      free(name);
      free(description);
   }
}

/* -----------------------------------------------------------------------
 * #MOBILES
 * ----------------------------------------------------------------------- */

static void import_mobiles_section(FILE *fp, int area_id, int area_revision)
{
   for (;;)
   {
      int letter = il_fread_letter(fp);
      if (letter != '#')
         break;

      int vnum = (int)il_fread_number(fp);
      if (vnum == 0)
         break;

      char *player_name = il_fread_string(fp);
      char *short_descr = il_fread_string(fp);
      char *long_descr = il_fread_string(fp);
      char *description = il_fread_string(fp);

      unsigned long long act_flags = il_fread_number_ull(fp);
      int affected_by = (int)il_fread_number(fp);
      int alignment = (int)il_fread_number(fp);
      /* letter 'S' required before level */
      il_fread_letter(fp);
      int level = (int)il_fread_number(fp);
      int sex = (int)il_fread_number(fp);
      int hp_mod = (int)il_fread_number(fp);
      int ac_mod = (int)il_fread_number(fp);
      int hr_mod = (int)il_fread_number(fp);
      int dr_mod = (int)il_fread_number(fp);

      /* '!' extension: class clan race position skills cast def */
      int mob_class = 0, clan = 0, race = 0, position = 0;
      int skills = 0, cast_flags = 0, def = 0;
      letter = il_fread_letter(fp);
      if (letter == '!')
      {
         mob_class = (int)il_fread_number(fp);
         clan = (int)il_fread_number(fp);
         race = (int)il_fread_number(fp);
         position = (int)il_fread_number(fp);
         skills = (int)il_fread_number(fp);
         cast_flags = (int)il_fread_number(fp);
         def = (int)il_fread_number(fp);
         if (area_revision < 16 || race < 0)
            race = 0;
      }
      else
         ungetc(letter, fp);

      /* '|' extension: strong_magic weak_magic race_mods power_skills power_cast resist suscept */
      int strong_magic = 0, weak_magic = 0, race_mods = 0;
      int power_skills = 0, power_cast = 0, resist = 0, suscept = 0;
      letter = il_fread_letter(fp);
      if (letter == '|')
      {
         strong_magic = (int)il_fread_number(fp);
         weak_magic = (int)il_fread_number(fp);
         race_mods = (int)il_fread_number(fp);
         power_skills = (int)il_fread_number(fp);
         power_cast = (int)il_fread_number(fp);
         resist = (int)il_fread_number(fp);
         suscept = (int)il_fread_number(fp);
      }
      else
         ungetc(letter, fp);

      /* '+' extension: spellpower crit crit_mult spell_crit spell_mult parry dodge block pierce */
      int spellpower = 0, crit = 0, crit_mult = 0;
      int spell_crit = 0, spell_mult = 0;
      int parry = 0, dodge = 0, block = 0, pierce = 0;
      letter = il_fread_letter(fp);
      if (letter == '+')
      {
         spellpower = (int)il_fread_number(fp);
         crit = (int)il_fread_number(fp);
         crit_mult = (int)il_fread_number(fp);
         spell_crit = (int)il_fread_number(fp);
         spell_mult = (int)il_fread_number(fp);
         parry = (int)il_fread_number(fp);
         dodge = (int)il_fread_number(fp);
         block = (int)il_fread_number(fp);
         pierce = (int)il_fread_number(fp);
      }
      else
         ungetc(letter, fp);

      /* 'l' extension: loot_amount loot[0..MAX_LOOT-1] */
      int loot_amount = 0;
      int loot[IL_MAX_LOOT];
      int i;
      for (i = 0; i < IL_MAX_LOOT; i++)
         loot[i] = 0;
      letter = il_fread_letter(fp);
      if (letter == 'l')
      {
         loot_amount = (int)il_fread_number(fp);
         for (i = 0; i < IL_MAX_LOOT; i++)
            loot[i] = (int)il_fread_number(fp);
      }
      else
         ungetc(letter, fp);

      /* 'L' extension: loot_chance[0..MAX_LOOT-1] */
      int loot_chance[IL_MAX_LOOT];
      for (i = 0; i < IL_MAX_LOOT; i++)
         loot_chance[i] = 0;
      letter = il_fread_letter(fp);
      if (letter == 'L')
      {
         for (i = 0; i < IL_MAX_LOOT; i++)
            loot_chance[i] = (int)il_fread_number(fp);
      }
      else
         ungetc(letter, fp);

      /* '^' extension: lore_flags (server reads but doesn't store in schema) */
      letter = il_fread_letter(fp);
      if (letter == '^')
         il_fread_number(fp); /* discard */
      else
         ungetc(letter, fp);

      /* 'a' extension: ai_knowledge accent ai_prompt~ */
      int ai_knowledge = 0, accent = 0;
      char *ai_prompt = strdup("");
      letter = il_fread_letter(fp);
      if (letter == 'a')
      {
         ai_knowledge = (int)il_fread_number(fp);
         accent = (int)il_fread_number(fp);
         free(ai_prompt);
         ai_prompt = il_fread_string(fp);
      }
      else
         ungetc(letter, fp);

      /* Build parameter set and insert. */
      PSet ps;
      ps_init(&ps);
      ps_int(&ps, vnum);         /* $1  vnum         */
      ps_int(&ps, area_id);      /* $2  area_id      */
      ps_str(&ps, player_name);  /* $3  player_name  */
      ps_str(&ps, short_descr);  /* $4  short_descr  */
      ps_str(&ps, long_descr);   /* $5  long_descr   */
      ps_str(&ps, description);  /* $6  description  */
      ps_ull(&ps, act_flags);    /* $7  act_flags    */
      ps_int(&ps, affected_by);  /* $8  affected_by  */
      ps_int(&ps, alignment);    /* $9  alignment    */
      ps_int(&ps, level);        /* $10 level        */
      ps_int(&ps, sex);          /* $11 sex          */
      ps_int(&ps, hp_mod);       /* $12 hp_mod       */
      ps_int(&ps, ac_mod);       /* $13 ac_mod       */
      ps_int(&ps, hr_mod);       /* $14 hr_mod       */
      ps_int(&ps, dr_mod);       /* $15 dr_mod       */
      ps_int(&ps, mob_class);    /* $16 class        */
      ps_int(&ps, clan);         /* $17 clan         */
      ps_int(&ps, race);         /* $18 race         */
      ps_int(&ps, position);     /* $19 position     */
      ps_int(&ps, skills);       /* $20 skills       */
      ps_int(&ps, cast_flags);   /* $21 cast_flags   */
      ps_int(&ps, def);          /* $22 def          */
      ps_int(&ps, strong_magic); /* $23             */
      ps_int(&ps, weak_magic);   /* $24             */
      ps_int(&ps, race_mods);    /* $25             */
      ps_int(&ps, power_skills); /* $26             */
      ps_int(&ps, power_cast);   /* $27             */
      ps_int(&ps, resist);       /* $28             */
      ps_int(&ps, suscept);      /* $29             */
      ps_int(&ps, spellpower);   /* $30             */
      ps_int(&ps, crit);         /* $31             */
      ps_int(&ps, crit_mult);    /* $32             */
      ps_int(&ps, spell_crit);   /* $33             */
      ps_int(&ps, spell_mult);   /* $34             */
      ps_int(&ps, parry);        /* $35             */
      ps_int(&ps, dodge);        /* $36             */
      ps_int(&ps, block);        /* $37             */
      ps_int(&ps, pierce);       /* $38             */
      ps_int(&ps, ai_knowledge); /* $39             */
      ps_int(&ps, accent);       /* $40             */
      ps_str(&ps, ai_prompt);    /* $41 ai_prompt   */
      ps_int(&ps, loot_amount);  /* $42             */
      for (i = 0; i < IL_MAX_LOOT; i++)
         ps_int(&ps, loot[i]); /* $43..$51 */
      for (i = 0; i < IL_MAX_LOOT; i++)
         ps_int(&ps, loot_chance[i]); /* $52..$60 */

      exec_params("INSERT INTO mobiles "
                  "(vnum,area_id,player_name,short_descr,long_descr,description,"
                  " act_flags,affected_by,alignment,level,sex,hp_mod,ac_mod,hr_mod,dr_mod,"
                  " class,clan,race,position,skills,cast_flags,def,"
                  " strong_magic,weak_magic,race_mods,power_skills,power_cast,resist,suscept,"
                  " spellpower,crit,crit_mult,spell_crit,spell_mult,parry,dodge,block,pierce,"
                  " ai_knowledge,accent,ai_prompt,loot_amount,"
                  " loot_0,loot_1,loot_2,loot_3,loot_4,loot_5,loot_6,loot_7,loot_8,"
                  " loot_chance_0,loot_chance_1,loot_chance_2,loot_chance_3,loot_chance_4,"
                  " loot_chance_5,loot_chance_6,loot_chance_7,loot_chance_8) "
                  "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,"
                  " $16,$17,$18,$19,$20,$21,$22,$23,$24,$25,$26,$27,$28,$29,"
                  " $30,$31,$32,$33,$34,$35,$36,$37,$38,$39,$40,$41,$42,"
                  " $43,$44,$45,$46,$47,$48,$49,$50,$51,"
                  " $52,$53,$54,$55,$56,$57,$58,$59,$60) "
                  "ON CONFLICT (vnum) DO UPDATE SET "
                  "area_id=EXCLUDED.area_id,player_name=EXCLUDED.player_name,"
                  "short_descr=EXCLUDED.short_descr,long_descr=EXCLUDED.long_descr,"
                  "description=EXCLUDED.description,act_flags=EXCLUDED.act_flags,"
                  "affected_by=EXCLUDED.affected_by,alignment=EXCLUDED.alignment,"
                  "level=EXCLUDED.level,sex=EXCLUDED.sex,hp_mod=EXCLUDED.hp_mod,"
                  "ac_mod=EXCLUDED.ac_mod,hr_mod=EXCLUDED.hr_mod,dr_mod=EXCLUDED.dr_mod,"
                  "class=EXCLUDED.class,clan=EXCLUDED.clan,race=EXCLUDED.race,"
                  "position=EXCLUDED.position,skills=EXCLUDED.skills,"
                  "cast_flags=EXCLUDED.cast_flags,def=EXCLUDED.def,"
                  "strong_magic=EXCLUDED.strong_magic,weak_magic=EXCLUDED.weak_magic,"
                  "race_mods=EXCLUDED.race_mods,power_skills=EXCLUDED.power_skills,"
                  "power_cast=EXCLUDED.power_cast,resist=EXCLUDED.resist,"
                  "suscept=EXCLUDED.suscept,spellpower=EXCLUDED.spellpower,"
                  "crit=EXCLUDED.crit,crit_mult=EXCLUDED.crit_mult,"
                  "spell_crit=EXCLUDED.spell_crit,spell_mult=EXCLUDED.spell_mult,"
                  "parry=EXCLUDED.parry,dodge=EXCLUDED.dodge,block=EXCLUDED.block,"
                  "pierce=EXCLUDED.pierce,ai_knowledge=EXCLUDED.ai_knowledge,"
                  "accent=EXCLUDED.accent,ai_prompt=EXCLUDED.ai_prompt,"
                  "loot_amount=EXCLUDED.loot_amount,"
                  "loot_0=EXCLUDED.loot_0,loot_1=EXCLUDED.loot_1,loot_2=EXCLUDED.loot_2,"
                  "loot_3=EXCLUDED.loot_3,loot_4=EXCLUDED.loot_4,loot_5=EXCLUDED.loot_5,"
                  "loot_6=EXCLUDED.loot_6,loot_7=EXCLUDED.loot_7,loot_8=EXCLUDED.loot_8,"
                  "loot_chance_0=EXCLUDED.loot_chance_0,loot_chance_1=EXCLUDED.loot_chance_1,"
                  "loot_chance_2=EXCLUDED.loot_chance_2,loot_chance_3=EXCLUDED.loot_chance_3,"
                  "loot_chance_4=EXCLUDED.loot_chance_4,loot_chance_5=EXCLUDED.loot_chance_5,"
                  "loot_chance_6=EXCLUDED.loot_chance_6,loot_chance_7=EXCLUDED.loot_chance_7,"
                  "loot_chance_8=EXCLUDED.loot_chance_8",
                  ps.n, ps.vals);

      free(player_name);
      free(short_descr);
      free(long_descr);
      free(description);
      free(ai_prompt);
   }
}

/* -----------------------------------------------------------------------
 * #OBJECTS
 * ----------------------------------------------------------------------- */

static void import_objects_section(FILE *fp, int area_id, int area_revision)
{
   for (;;)
   {
      int letter = il_fread_letter(fp);
      if (letter != '#')
         break;

      int vnum = (int)il_fread_number(fp);
      if (vnum == 0)
         break;

      char *name = il_fread_string(fp);
      char *short_descr = il_fread_string(fp);
      char *description = il_fread_string(fp);

      int item_type = (int)il_fread_number(fp);
      unsigned long long extra_flags = il_fread_number_ull(fp);
      int wear_flags = (int)il_fread_number(fp);
      int item_apply = (int)il_fread_number(fp);

      int value[10];
      int i;
      for (i = 0; i < 4; i++)
         value[i] = (int)il_fread_number(fp);
      if (area_revision < 15)
         for (i = 4; i < 10; i++)
            value[i] = 0;
      else
         for (i = 4; i < 10; i++)
            value[i] = (int)il_fread_number(fp);

      int weight = (int)il_fread_number(fp);
      int obj_level = 0;

      /* Insert object row first so affects/extra_descs can FK to it. */
      {
         PSet ps;
         ps_init(&ps);
         ps_int(&ps, vnum);        /* $1  */
         ps_int(&ps, area_id);     /* $2  */
         ps_str(&ps, name);        /* $3  */
         ps_str(&ps, short_descr); /* $4  */
         ps_str(&ps, description); /* $5  */
         ps_int(&ps, item_type);   /* $6  */
         ps_ull(&ps, extra_flags); /* $7  */
         ps_int(&ps, wear_flags);  /* $8  */
         ps_int(&ps, item_apply);  /* $9  */
         for (i = 0; i < 10; i++)
            ps_int(&ps, value[i]); /* $10..$19 */
         ps_int(&ps, weight);      /* $20 */
         ps_int(&ps, 0);           /* $21 level placeholder */
         exec_params("INSERT INTO objects "
                     "(vnum,area_id,name,short_descr,description,"
                     " item_type,extra_flags,wear_flags,item_apply,"
                     " value_0,value_1,value_2,value_3,value_4,"
                     " value_5,value_6,value_7,value_8,value_9,"
                     " weight,level) "
                     "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,"
                     " $10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$20,$21) "
                     "ON CONFLICT (vnum) DO UPDATE SET "
                     "area_id=EXCLUDED.area_id,name=EXCLUDED.name,"
                     "short_descr=EXCLUDED.short_descr,description=EXCLUDED.description,"
                     "item_type=EXCLUDED.item_type,extra_flags=EXCLUDED.extra_flags,"
                     "wear_flags=EXCLUDED.wear_flags,item_apply=EXCLUDED.item_apply,"
                     "value_0=EXCLUDED.value_0,value_1=EXCLUDED.value_1,"
                     "value_2=EXCLUDED.value_2,value_3=EXCLUDED.value_3,"
                     "value_4=EXCLUDED.value_4,value_5=EXCLUDED.value_5,"
                     "value_6=EXCLUDED.value_6,value_7=EXCLUDED.value_7,"
                     "value_8=EXCLUDED.value_8,value_9=EXCLUDED.value_9,"
                     "weight=EXCLUDED.weight",
                     ps.n, ps.vals);
      }

      /* Remove stale affects/extra_descs for re-import. */
      {
         char vnum_str[16];
         const char *v1[1];
         snprintf(vnum_str, sizeof(vnum_str), "%d", vnum);
         v1[0] = vnum_str;
         exec_params("DELETE FROM object_affects WHERE obj_vnum=$1", 1, v1);
         exec_params("DELETE FROM object_extra_descs WHERE obj_vnum=$1", 1, v1);
      }

      /* Post-field loop: A (affect), E (extra_desc), L (level). */
      for (;;)
      {
         int sub = il_fread_letter(fp);

         if (sub == 'A')
         {
            int location = (int)il_fread_number(fp);
            int modifier = (int)il_fread_number(fp);
            PSet ps;
            ps_init(&ps);
            ps_int(&ps, vnum);
            ps_int(&ps, location);
            ps_int(&ps, modifier);
            exec_params("INSERT INTO object_affects (obj_vnum, location, modifier) "
                        "VALUES ($1,$2,$3)",
                        ps.n, ps.vals);
         }
         else if (sub == 'E')
         {
            char *ekey = il_fread_string(fp);
            char *edesc = il_fread_string(fp);
            PSet ps;
            ps_init(&ps);
            ps_int(&ps, vnum);
            ps_str(&ps, ekey);
            ps_str(&ps, edesc);
            exec_params("INSERT INTO object_extra_descs (obj_vnum, keyword, description) "
                        "VALUES ($1,$2,$3)",
                        ps.n, ps.vals);
            free(ekey);
            free(edesc);
         }
         else if (sub == 'L')
         {
            obj_level = (int)il_fread_number(fp);
         }
         else
         {
            ungetc(sub, fp);
            break;
         }
      }

      /* Update level now that we've parsed it. */
      if (obj_level != 0)
      {
         PSet ps;
         ps_init(&ps);
         ps_int(&ps, obj_level);
         ps_int(&ps, vnum);
         exec_params("UPDATE objects SET level=$1 WHERE vnum=$2", ps.n, ps.vals);
      }

      free(name);
      free(short_descr);
      free(description);
   }
}

/* -----------------------------------------------------------------------
 * #RESETS
 * ----------------------------------------------------------------------- */

static void import_resets_section(FILE *fp, int area_id)
{
   int seq = 0;

   for (;;)
   {
      int letter = il_fread_letter(fp);
      if (letter == 'S' || letter == EOF)
         break;

      if (letter == '*')
      {
         il_fread_to_eol(fp);
         continue;
      }

      int ifflag = (int)il_fread_number(fp);
      int arg1 = (int)il_fread_number(fp);
      int arg2 = (int)il_fread_number(fp);
      int arg3 = (letter == 'G' || letter == 'R') ? 0 : (int)il_fread_number(fp);
      char *notes = il_fsave_to_eol(fp);

      /* 'A' is an obsolete reset; the schema CHECK allows it but we skip it
       * to match server behaviour (it silently discards 'A' resets). */
      if (letter != 'A')
      {
         char cmd[2];
         cmd[0] = (char)letter;
         cmd[1] = '\0';
         seq++;
         PSet ps;
         ps_init(&ps);
         ps_int(&ps, area_id); /* $1 */
         ps_int(&ps, seq);     /* $2 */
         ps_str(&ps, cmd);     /* $3 */
         ps_int(&ps, ifflag);  /* $4 */
         ps_int(&ps, arg1);    /* $5 */
         ps_int(&ps, arg2);    /* $6 */
         ps_int(&ps, arg3);    /* $7 */
         ps_str(&ps, notes);   /* $8 */
         exec_params("INSERT INTO resets (area_id,seq,command,ifflag,arg1,arg2,arg3,notes) "
                     "VALUES ($1,$2,$3,$4,$5,$6,$7,$8) "
                     "ON CONFLICT (area_id,seq) DO UPDATE SET "
                     "command=EXCLUDED.command,ifflag=EXCLUDED.ifflag,"
                     "arg1=EXCLUDED.arg1,arg2=EXCLUDED.arg2,"
                     "arg3=EXCLUDED.arg3,notes=EXCLUDED.notes",
                     ps.n, ps.vals);
      }

      free(notes);
   }
}

/* -----------------------------------------------------------------------
 * #SHOPS
 * ----------------------------------------------------------------------- */

static void import_shops_section(FILE *fp)
{
   for (;;)
   {
      int keeper = (int)il_fread_number(fp);
      if (keeper == 0)
         break;

      int buy_type[IL_MAX_TRADE];
      int i;
      for (i = 0; i < IL_MAX_TRADE; i++)
         buy_type[i] = (int)il_fread_number(fp);

      int profit_buy = (int)il_fread_number(fp);
      int profit_sell = (int)il_fread_number(fp);
      int open_hour = (int)il_fread_number(fp);
      int close_hour = (int)il_fread_number(fp);
      il_fread_to_eol(fp);

      PSet ps;
      ps_init(&ps);
      ps_int(&ps, keeper);      /* $1  */
      ps_int(&ps, buy_type[0]); /* $2  */
      ps_int(&ps, buy_type[1]); /* $3  */
      ps_int(&ps, buy_type[2]); /* $4  */
      ps_int(&ps, buy_type[3]); /* $5  */
      ps_int(&ps, buy_type[4]); /* $6  */
      ps_int(&ps, profit_buy);  /* $7  */
      ps_int(&ps, profit_sell); /* $8  */
      ps_int(&ps, open_hour);   /* $9  */
      ps_int(&ps, close_hour);  /* $10 */
      exec_params("INSERT INTO shops "
                  "(keeper_vnum,buy_type_0,buy_type_1,buy_type_2,buy_type_3,buy_type_4,"
                  " profit_buy,profit_sell,open_hour,close_hour) "
                  "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10) "
                  "ON CONFLICT (keeper_vnum) DO UPDATE SET "
                  "buy_type_0=EXCLUDED.buy_type_0,buy_type_1=EXCLUDED.buy_type_1,"
                  "buy_type_2=EXCLUDED.buy_type_2,buy_type_3=EXCLUDED.buy_type_3,"
                  "buy_type_4=EXCLUDED.buy_type_4,profit_buy=EXCLUDED.profit_buy,"
                  "profit_sell=EXCLUDED.profit_sell,open_hour=EXCLUDED.open_hour,"
                  "close_hour=EXCLUDED.close_hour",
                  ps.n, ps.vals);
   }
}

/* -----------------------------------------------------------------------
 * #SPECIALS
 * ----------------------------------------------------------------------- */

static void import_specials_section(FILE *fp)
{
   for (;;)
   {
      int letter = il_fread_letter(fp);
      if (letter == 'S' || letter == EOF)
         break;
      if (letter == '*')
      {
         il_fread_to_eol(fp);
         continue;
      }
      if (letter == 'M')
      {
         int mob_vnum = (int)il_fread_number(fp);
         char *spec_name = il_fread_word(fp);
         il_fread_to_eol(fp);

         PSet ps;
         ps_init(&ps);
         ps_int(&ps, mob_vnum);
         ps_str(&ps, spec_name);
         exec_params("INSERT INTO mobile_specials (mob_vnum, spec_name) "
                     "VALUES ($1,$2) "
                     "ON CONFLICT (mob_vnum) DO UPDATE SET spec_name=EXCLUDED.spec_name",
                     ps.n, ps.vals);
         free(spec_name);
      }
      else
      {
         fprintf(errlog, "WARN: #SPECIALS: unexpected letter '%c'\n", letter);
         il_fread_to_eol(fp);
      }
   }
}

/* -----------------------------------------------------------------------
 * #OBJFUNS
 * ----------------------------------------------------------------------- */

static void import_objfuns_section(FILE *fp)
{
   for (;;)
   {
      int letter = il_fread_letter(fp);
      if (letter == 'S' || letter == EOF)
         break;
      if (letter == '*')
      {
         il_fread_to_eol(fp);
         continue;
      }
      if (letter == 'O')
      {
         int obj_vnum = (int)il_fread_number(fp);
         char *fun_name = il_fread_word(fp);
         il_fread_to_eol(fp);

         PSet ps;
         ps_init(&ps);
         ps_int(&ps, obj_vnum);
         ps_str(&ps, fun_name);
         exec_params("INSERT INTO object_functions (obj_vnum, fun_name) "
                     "VALUES ($1,$2) "
                     "ON CONFLICT (obj_vnum) DO UPDATE SET fun_name=EXCLUDED.fun_name",
                     ps.n, ps.vals);
         free(fun_name);
      }
      else
      {
         fprintf(errlog, "WARN: #OBJFUNS: unexpected letter '%c'\n", letter);
         il_fread_to_eol(fp);
      }
   }
}

/* -----------------------------------------------------------------------
 * Area file dispatcher
 *
 * Opens a single .are file and dispatches each #SECTION to the appropriate
 * importer.  Each area is wrapped in its own transaction so a failure in
 * one area does not roll back others.
 * ----------------------------------------------------------------------- */

static int import_area_file(const char *path)
{
   FILE *fp;
   char *section;
   int letter;
   int area_id = -1;
   int area_revision = -1;

   fp = fopen(path, "r");
   if (!fp)
   {
      fprintf(errlog, "WARN: cannot open area file %s: %s\n", path, strerror(errno));
      return 0;
   }

   exec_sql("BEGIN");

   for (;;)
   {
      letter = il_fread_letter(fp);
      if (letter == EOF)
         break;
      if (letter != '#')
      {
         /* Unexpected — consume to end of line and continue. */
         il_fread_to_eol(fp);
         continue;
      }

      section = il_fread_word(fp);

      if (strcmp(section, "$") == 0)
      {
         free(section);
         break;
      }
      else if (strcmp(section, "AREA") == 0)
      {
         area_id = import_area_header(fp, &area_revision);
         if (area_id < 0)
         {
            fprintf(errlog, "ERROR: area header import failed for %s\n", path);
            free(section);
            exec_sql("ROLLBACK");
            fclose(fp);
            return 0;
         }
      }
      else if (strcmp(section, "ROOMS") == 0)
      {
         if (area_id >= 0)
            import_rooms_section(fp, area_id);
      }
      else if (strcmp(section, "MOBILES") == 0)
      {
         if (area_id >= 0)
            import_mobiles_section(fp, area_id, area_revision);
      }
      else if (strcmp(section, "OBJECTS") == 0)
      {
         if (area_id >= 0)
            import_objects_section(fp, area_id, area_revision);
      }
      else if (strcmp(section, "RESETS") == 0)
      {
         if (area_id >= 0)
            import_resets_section(fp, area_id);
      }
      else if (strcmp(section, "SHOPS") == 0)
      {
         import_shops_section(fp);
      }
      else if (strcmp(section, "SPECIALS") == 0)
      {
         import_specials_section(fp);
      }
      else if (strcmp(section, "OBJFUNS") == 0)
      {
         import_objfuns_section(fp);
      }
      else
      {
         /* Unknown section — skip until the next '#' marker. */
         while ((letter = fgetc(fp)) != EOF)
         {
            if (letter == '#')
            {
               ungetc(letter, fp);
               break;
            }
         }
      }

      free(section);
   }

   exec_sql("COMMIT");
   fclose(fp);
   return 1;
}

/* Read area.lst and import every listed .are file. */
static int import_areas(const char *area_lst_path, const char *area_dir)
{
   FILE *fp;
   char line[256];
   int count = 0;
   char path[512];

   fp = fopen(area_lst_path, "r");
   if (!fp)
   {
      fprintf(errlog, "WARN: cannot open %s\n", area_lst_path);
      return 0;
   }

   while (fgets(line, sizeof(line), fp))
   {
      strip_trailing(line);
      if (line[0] == '\0')
         continue;
      if (line[0] == '$')
         break;
      snprintf(path, sizeof(path), "%s/%s", area_dir, line);
      if (import_area_file(path))
      {
         printf("  area: %s\n", line);
         count++;
      }
      else
      {
         fprintf(errlog, "  WARN: failed to import %s\n", line);
      }
   }

   fclose(fp);
   return count;
}

/* -----------------------------------------------------------------------
 * data/bans.lst
 *
 * Format:
 *   #BAN~
 *   <ban_type (int)>
 *   <address>~
 *   <banned_by>~
 *   (repeat)
 *   #END~
 * ----------------------------------------------------------------------- */

static int import_bans(const char *path)
{
   FILE *fp;
   int count = 0;

   fp = fopen(path, "r");
   if (!fp)
   {
      fprintf(errlog, "WARN: cannot open %s: %s\n", path, strerror(errno));
      return 0;
   }

   exec_sql("BEGIN");
   exec_sql("DELETE FROM bans");

   for (;;)
   {
      char *tag = il_fread_string(fp);
      if (!tag)
         break;
      if (strcmp(tag, "#END") == 0)
      {
         free(tag);
         break;
      }
      if (strcmp(tag, "#BAN") != 0)
      {
         free(tag);
         break;
      }
      free(tag);

      int ban_type = (int)il_fread_number(fp);
      char *address = il_fread_string(fp);
      char *banned_by = il_fread_string(fp);

      PSet ps;
      ps_init(&ps);
      ps_int(&ps, ban_type);
      ps_str(&ps, address);
      ps_str(&ps, banned_by);
      exec_params("INSERT INTO bans (ban_type, address, banned_by) VALUES ($1,$2,$3)", ps.n,
                  ps.vals);
      count++;

      free(address);
      free(banned_by);
   }

   exec_sql("COMMIT");
   fclose(fp);
   return count;
}

/* -----------------------------------------------------------------------
 * data/socials.txt
 *
 * Format:
 *   <count (int via fscanf)>
 *   <name>~
 *   <char_no_arg>~
 *   <others_no_arg>~
 *   <char_found>~
 *   <others_found>~
 *   <vict_found>~
 *   <char_auto>~
 *   <others_auto>~
 *   (repeat count times)
 * ----------------------------------------------------------------------- */

static int import_socials(const char *path)
{
   FILE *fp;
   int count = 0;
   int total;

   fp = fopen(path, "r");
   if (!fp)
   {
      fprintf(errlog, "WARN: cannot open %s: %s\n", path, strerror(errno));
      return 0;
   }

   if (fscanf(fp, "%d\n", &total) != 1)
   {
      fprintf(errlog, "WARN: cannot read social count from %s\n", path);
      fclose(fp);
      return 0;
   }

   exec_sql("BEGIN");
   exec_sql("DELETE FROM socials");

   for (; count < total; count++)
   {
      char *name = il_fread_string(fp);
      char *char_no_arg = il_fread_string(fp);
      char *others_no_arg = il_fread_string(fp);
      char *char_found = il_fread_string(fp);
      char *others_found = il_fread_string(fp);
      char *vict_found = il_fread_string(fp);
      char *char_auto = il_fread_string(fp);
      char *others_auto = il_fread_string(fp);

      PSet ps;
      ps_init(&ps);
      ps_str(&ps, name);
      ps_str(&ps, char_no_arg);
      ps_str(&ps, others_no_arg);
      ps_str(&ps, char_found);
      ps_str(&ps, others_found);
      ps_str(&ps, vict_found);
      ps_str(&ps, char_auto);
      ps_str(&ps, others_auto);
      exec_params("INSERT INTO socials "
                  "(name,char_no_arg,others_no_arg,char_found,others_found,"
                  " vict_found,char_auto,others_auto) "
                  "VALUES ($1,$2,$3,$4,$5,$6,$7,$8) "
                  "ON CONFLICT (name) DO UPDATE SET "
                  "char_no_arg=EXCLUDED.char_no_arg,others_no_arg=EXCLUDED.others_no_arg,"
                  "char_found=EXCLUDED.char_found,others_found=EXCLUDED.others_found,"
                  "vict_found=EXCLUDED.vict_found,char_auto=EXCLUDED.char_auto,"
                  "others_auto=EXCLUDED.others_auto",
                  ps.n, ps.vals);

      free(name);
      free(char_no_arg);
      free(others_no_arg);
      free(char_found);
      free(others_found);
      free(vict_found);
      free(char_auto);
      free(others_auto);
   }

   exec_sql("COMMIT");
   fclose(fp);
   return count;
}

/* -----------------------------------------------------------------------
 * data/rulers.lst
 *
 * Format:
 *   #RULER~
 *   <name>~
 *   <affiliation_index (int)>
 *   <flags (int)>
 *   <ruler_rank (int)>
 *   <keywords>~
 *   (repeat)
 *   #END~
 *
 * Only the name is stored in the schema (rulers table has name only).
 * ----------------------------------------------------------------------- */

static int import_rulers(const char *path)
{
   FILE *fp;
   int count = 0;

   fp = fopen(path, "r");
   if (!fp)
   {
      fprintf(errlog, "WARN: cannot open %s: %s\n", path, strerror(errno));
      return 0;
   }

   exec_sql("BEGIN");
   exec_sql("DELETE FROM rulers");

   for (;;)
   {
      char *tag = il_fread_string(fp);
      if (!tag)
         break;
      if (strcmp(tag, "#END") == 0)
      {
         free(tag);
         break;
      }
      if (strcmp(tag, "#RULER") != 0)
      {
         free(tag);
         break;
      }
      free(tag);

      char *ruler_name = il_fread_string(fp);
      il_fread_number(fp);                  /* affiliation_index — not in schema */
      il_fread_number(fp);                  /* flags             — not in schema */
      il_fread_number(fp);                  /* ruler_rank        — not in schema */
      char *keywords = il_fread_string(fp); /* keywords — not in schema */
      free(keywords);

      PSet ps;
      ps_init(&ps);
      ps_str(&ps, ruler_name);
      exec_params("INSERT INTO rulers (name) VALUES ($1) ON CONFLICT (name) DO NOTHING", ps.n,
                  ps.vals);
      count++;

      free(ruler_name);
   }

   exec_sql("COMMIT");
   fclose(fp);
   return count;
}

/* -----------------------------------------------------------------------
 * data/brands.lst
 *
 * Format:
 *   #BRAND~
 *   <branded_by>~      (who applied the brand — person's name)
 *   <item_name>~       (display name of branded item)
 *   <brand_date>~      (ctime(3) string, e.g. "Tue Jun 17 06:59:13 2025")
 *   <description>~     (brand message body)
 *   <priority>~        (internal ordering hint — not stored in schema)
 *   (repeat)
 *   #END~
 * ----------------------------------------------------------------------- */

static int import_brands(const char *path)
{
   FILE *fp;
   int count = 0;

   fp = fopen(path, "r");
   if (!fp)
   {
      fprintf(errlog, "WARN: cannot open %s: %s\n", path, strerror(errno));
      return 0;
   }

   exec_sql("BEGIN");
   exec_sql("DELETE FROM brands");

   for (;;)
   {
      char *tag = il_fread_string(fp);
      if (!tag)
         break;
      if (strcmp(tag, "#END") == 0)
      {
         free(tag);
         break;
      }
      if (strcmp(tag, "#BRAND") != 0)
      {
         free(tag);
         break;
      }
      free(tag);

      char *branded_by = il_fread_string(fp);
      char *item_name = il_fread_string(fp);
      char *brand_date = il_fread_string(fp);
      char *description = il_fread_string(fp);
      char *priority = il_fread_string(fp); /* discard */
      free(priority);

      PSet ps;
      ps_init(&ps);
      ps_str(&ps, branded_by);
      ps_str(&ps, item_name);
      ps_str(&ps, brand_date);
      ps_str(&ps, description);
      exec_params("INSERT INTO brands (branded_by, item_name, brand_date, description) "
                  "VALUES ($1,$2,$3,$4)",
                  ps.n, ps.vals);
      count++;

      free(branded_by);
      free(item_name);
      free(brand_date);
      free(description);
   }

   exec_sql("COMMIT");
   fclose(fp);
   return count;
}

/* -----------------------------------------------------------------------
 * Clans
 *
 * Names are hardcoded in src/const_clan.c (not in any file).
 * The diplomacy/treasury data comes from data/clandata.dat.
 *
 * Clan ids are 0..MAX_CLAN-1.  The clandata.dat loop uses 1..MAX_CLAN-1
 * (clan 0 = "None" has no diplomacy rows).
 * ----------------------------------------------------------------------- */

static const char *clan_names[IL_MAX_CLAN] = {
    "None",                  /* 0 */
    "The Lantern Accord",    /* 1 */
    "The Open Ledger",       /* 2 */
    "The Jade Compact",      /* 3 */
    "The Ember Speakers",    /* 4 */
    "The Red Sand Accounts", /* 5 */
    "The Charter Wardens",   /* 6 */
    "The Jackal Synod",      /* 7 */
    "The Root Covenant",     /* 8 */
    "The Waystone Circle",   /* 9 */
    "The Oathbound March",   /* 10 */
};

static int import_clans(const char *clandata_path)
{
   FILE *fp;
   int diplomacy[IL_MAX_CLAN][IL_MAX_CLAN];
   int treasury[IL_MAX_CLAN];
   int i, j;

   for (i = 0; i < IL_MAX_CLAN; i++)
   {
      treasury[i] = 0;
      for (j = 0; j < IL_MAX_CLAN; j++)
         diplomacy[i][j] = 0;
   }

   fp = fopen(clandata_path, "r");
   if (fp)
   {
      int file_max = (int)il_fread_number(fp);
      if (file_max == IL_MAX_CLAN)
      {
         for (i = 1; i < IL_MAX_CLAN; i++)
            for (j = 1; j < IL_MAX_CLAN; j++)
               diplomacy[i][j] = (int)il_fread_number(fp);

         for (i = 1; i < IL_MAX_CLAN; i++)
            treasury[i] = (int)il_fread_number(fp);

         /* end_current_state[i][j] — no schema column, discard */
         for (i = 1; i < IL_MAX_CLAN; i++)
            for (j = 1; j < IL_MAX_CLAN; j++)
               il_fread_number(fp);
      }
      else
      {
         fprintf(errlog, "WARN: clandata.dat MAX_CLAN=%d != %d, using defaults\n", file_max,
                 IL_MAX_CLAN);
      }
      fclose(fp);
   }
   else
   {
      fprintf(errlog, "WARN: cannot open %s, using defaults\n", clandata_path);
   }

   exec_sql("BEGIN");

   for (i = 0; i < IL_MAX_CLAN; i++)
   {
      /* Build diplomacy row i as a PostgreSQL integer array literal. */
      char arr[512];
      int pos = 0;
      arr[pos++] = '{';
      for (j = 0; j < IL_MAX_CLAN; j++)
      {
         pos += snprintf(arr + pos, sizeof(arr) - (size_t)pos, "%d", diplomacy[i][j]);
         if (j < IL_MAX_CLAN - 1)
            arr[pos++] = ',';
      }
      arr[pos++] = '}';
      arr[pos] = '\0';

      PSet ps;
      ps_init(&ps);
      ps_int(&ps, i);             /* $1 id          */
      ps_str(&ps, clan_names[i]); /* $2 name        */
      ps_int(&ps, treasury[i]);   /* $3 gold        */
      ps_str(&ps, arr);           /* $4 war_matrix  */
      exec_params("INSERT INTO clans (id, name, gold, war_matrix) "
                  "VALUES ($1,$2,$3,$4::integer[]) "
                  "ON CONFLICT (id) DO UPDATE SET "
                  "name=EXCLUDED.name, gold=EXCLUDED.gold, war_matrix=EXCLUDED.war_matrix",
                  ps.n, ps.vals);
   }

   exec_sql("COMMIT");
   return IL_MAX_CLAN;
}

/* -----------------------------------------------------------------------
 * area/boards/board.<vnum>
 *
 * Header format (keyword=value lines until "Messages"):
 *   ExpiryTime  <days>
 *   MinReadLev  <level>
 *   MaxWriteLev <level>      (file writes MaxWriteLev; code column = min_write_lev)
 *   Clan        <clan_id>
 *
 * Message format (after "Messages" line):
 *   M<timestamp>\n
 *   <author>~
 *   <title>~
 *   <body>~
 *   S
 * ----------------------------------------------------------------------- */

static int import_one_board(const char *path)
{
   FILE *fp;
   int expiry_days = 10;
   int min_read_lev = 0;
   int min_write_lev = 0;
   int clan = 0;
   int vnum;
   int board_id;
   int seq = 0;

   /* Extract vnum from filename "board.<vnum>". */
   const char *basename = strrchr(path, '/');
   basename = basename ? basename + 1 : path;
   if (strncmp(basename, "board.", 6) != 0)
      return 0;
   vnum = atoi(basename + 6);
   if (vnum <= 0)
      return 0;

   fp = fopen(path, "r");
   if (!fp)
      return 0;

   /* Read header keyword=value pairs. */
   for (;;)
   {
      if (feof(fp))
         break;
      char *word = il_fread_word(fp);
      if (strcmp(word, "ExpiryTime") == 0)
      {
         expiry_days = (int)il_fread_number(fp);
         il_fread_to_eol(fp);
      }
      else if (strcmp(word, "MinReadLev") == 0)
      {
         min_read_lev = (int)il_fread_number(fp);
         il_fread_to_eol(fp);
      }
      else if (strcmp(word, "MaxWriteLev") == 0 || strcmp(word, "MinWriteLev") == 0)
      {
         /* The save code writes "MaxWriteLev" but the field is min_write_lev. */
         min_write_lev = (int)il_fread_number(fp);
         il_fread_to_eol(fp);
      }
      else if (strcmp(word, "Clan") == 0)
      {
         clan = (int)il_fread_number(fp);
         il_fread_to_eol(fp);
      }
      else if (strcmp(word, "Messages") == 0)
      {
         il_fread_to_eol(fp);
         free(word);
         break;
      }
      else
      {
         il_fread_to_eol(fp);
      }
      free(word);
   }

   /* Upsert board row. */
   {
      PSet ps;
      ps_init(&ps);
      ps_int(&ps, vnum);
      ps_int(&ps, expiry_days);
      ps_int(&ps, min_read_lev);
      ps_int(&ps, min_write_lev);
      ps_int(&ps, clan);
      board_id = exec_returning_id(
          "INSERT INTO boards (vnum, expiry_days, min_read_lev, min_write_lev, clan) "
          "VALUES ($1,$2,$3,$4,$5) "
          "ON CONFLICT (vnum) DO UPDATE SET "
          "expiry_days=EXCLUDED.expiry_days, min_read_lev=EXCLUDED.min_read_lev, "
          "min_write_lev=EXCLUDED.min_write_lev, clan=EXCLUDED.clan "
          "RETURNING id",
          ps.n, ps.vals);
   }

   if (board_id < 0)
   {
      fclose(fp);
      return 0;
   }

   /* Delete old messages for a clean re-import. */
   {
      PSet ps;
      ps_init(&ps);
      ps_int(&ps, board_id);
      exec_params("DELETE FROM board_messages WHERE board_id=$1", ps.n, ps.vals);
   }

   /* Read messages. */
   for (;;)
   {
      int letter;
      if (feof(fp))
         break;

      letter = il_fread_letter(fp);
      if (letter == 'S' || letter == EOF)
         break;
      if (letter != 'M')
      {
         fprintf(errlog, "WARN: board %d: expected M, got '%c'\n", vnum, letter);
         break;
      }

      long posted_at = il_fread_number(fp);
      char *author = il_fread_string(fp);
      char *title = il_fread_string(fp);
      char *body = il_fread_string(fp);
      seq++;

      PSet ps;
      ps_init(&ps);
      ps_int(&ps, board_id);
      ps_int(&ps, posted_at);
      ps_str(&ps, author);
      ps_str(&ps, title);
      ps_str(&ps, body);
      ps_int(&ps, seq);
      exec_params("INSERT INTO board_messages (board_id, posted_at, author, title, body, seq) "
                  "VALUES ($1,$2,$3,$4,$5,$6)",
                  ps.n, ps.vals);

      free(author);
      free(title);
      free(body);
   }

   fclose(fp);
   return 1;
}

static int import_boards(const char *boards_dir)
{
   DIR *dp;
   struct dirent *de;
   int count = 0;
   char path[512];

   dp = opendir(boards_dir);
   if (!dp)
   {
      fprintf(errlog, "WARN: cannot open boards dir %s: %s\n", boards_dir, strerror(errno));
      return 0;
   }

   exec_sql("BEGIN");
   while ((de = readdir(dp)) != NULL)
   {
      struct stat st;
      if (de->d_name[0] == '.')
         continue;
      if (strncmp(de->d_name, "board.", 6) != 0)
         continue;
      snprintf(path, sizeof(path), "%s/%s", boards_dir, de->d_name);
      if (stat(path, &st) != 0 || S_ISDIR(st.st_mode))
         continue;
      if (import_one_board(path))
         count++;
   }
   exec_sql("COMMIT");
   closedir(dp);
   return count;
}

/* -----------------------------------------------------------------------
 * Connection string helper
 * ----------------------------------------------------------------------- */

static char *read_db_conf(const char *base_dir)
{
   char path[512];
   FILE *fp;
   char buf[1024];
   size_t n;

   snprintf(path, sizeof(path), "%s/../data/db.conf", base_dir);
   fp = fopen(path, "r");
   if (!fp)
      return NULL;
   n = fread(buf, 1, sizeof(buf) - 1, fp);
   fclose(fp);
   if (!n)
      return NULL;
   buf[n] = '\0';
   strip_trailing(buf);
   return strdup(buf);
}

/* -----------------------------------------------------------------------
 * Quest template import  (quests/*.prop)
 * ----------------------------------------------------------------------- */

#define IL_QUEST_MAX_TARGETS 10

/* Read next non-empty, non-comment line; strip leading/trailing whitespace. */
static int quest_read_line(FILE *fp, char *buf, size_t size)
{
   while (fgets(buf, (int)size, fp) != NULL)
   {
      char *p = buf;
      size_t len;
      while (*p && (*p == ' ' || *p == '\t'))
         memmove(p, p + 1, strlen(p));
      len = strlen(buf);
      while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' || buf[len - 1] == ' '))
         buf[--len] = '\0';
      if (buf[0] == '\0' || buf[0] == '#')
         continue;
      return 1;
   }
   return 0;
}

static int import_quest_file(const char *path, int id)
{
   FILE *fp;
   char line[4096];
   const char *params[22];
   char id_str[16], prereq_str[16], type_str[16], ntargets_str[16];
   char kill_str[16], minlv_str[16], maxlv_str[16], offerer_str[16];
   char gold_str[16], qp_str[16], exp_str[16];
   char wear_str[16], extra_str[16], weight_str[16], apply_str[16];
   char vnums_pg[256]; /* PostgreSQL array literal, e.g. {1234,5678} */
   int prereq = -1, type = 0, ntargets = 0, kill_needed = 0;
   int minlv = 0, maxlv = 170, offerer = 0;
   int reward_gold = 0, reward_qp = 0, reward_exp = 0;
   int target_vnums[IL_QUEST_MAX_TARGETS];
   char *title = "", *accept = "", *completion = "";
   char *obj_short = "", *obj_name = "", *obj_long = "";
   int obj_wear = 0, obj_extra = 0, obj_weight = 0, obj_apply = 0;
   char title_buf[4096], accept_buf[4096], completion_buf[4096];
   char short_buf[4096], name_buf[4096], long_buf[4096];
   int i;
   PGresult *res;

   fp = fopen(path, "r");
   if (!fp)
      return 0;

   memset(target_vnums, 0, sizeof(target_vnums));
   title_buf[0] = accept_buf[0] = completion_buf[0] = '\0';
   short_buf[0] = name_buf[0] = long_buf[0] = '\0';

   /* Line 1: title */
   if (!quest_read_line(fp, title_buf, sizeof(title_buf)))
   {
      fclose(fp);
      return 0;
   }
   title = title_buf;

   /* Line 2: numeric fields */
   if (!quest_read_line(fp, line, sizeof(line)))
   {
      fclose(fp);
      return 0;
   }
   maxlv = 170;
   reward_exp = 0;
   if (sscanf(line, "%d %d %d %d %d %d %d %d %d %d %*d", &prereq, &type, &ntargets, &kill_needed,
              &minlv, &maxlv, &offerer, &reward_gold, &reward_qp, &reward_exp) < 9)
   {
      fprintf(errlog, "WARN: bad numeric line in %s\n", path);
      fclose(fp);
      return 0;
   }

   /* Line 3: target vnums */
   if (!quest_read_line(fp, line, sizeof(line)))
   {
      fclose(fp);
      return 0;
   }
   {
      char *tok = strtok(line, " \t");
      for (i = 0; i < IL_QUEST_MAX_TARGETS && tok; i++)
      {
         target_vnums[i] = atoi(tok);
         tok = strtok(NULL, " \t");
      }
   }

   /* Line 4: accept message */
   if (quest_read_line(fp, accept_buf, sizeof(accept_buf)))
      accept = accept_buf;

   /* Line 5: completion message */
   if (quest_read_line(fp, completion_buf, sizeof(completion_buf)))
      completion = completion_buf;

   /* Lines 6-12: optional reward object fields */
   if (quest_read_line(fp, short_buf, sizeof(short_buf)))
      obj_short = short_buf;
   if (quest_read_line(fp, name_buf, sizeof(name_buf)))
      obj_name = name_buf;
   if (quest_read_line(fp, long_buf, sizeof(long_buf)))
      obj_long = long_buf;
   if (quest_read_line(fp, line, sizeof(line)))
      obj_wear = atoi(line);
   if (quest_read_line(fp, line, sizeof(line)))
      obj_extra = atoi(line);
   if (quest_read_line(fp, line, sizeof(line)))
      obj_weight = atoi(line);
   if (quest_read_line(fp, line, sizeof(line)))
      obj_apply = atoi(line);

   fclose(fp);

   /* Build PostgreSQL integer array literal */
   {
      char *p = vnums_pg;
      *p++ = '{';
      for (i = 0; i < ntargets && i < IL_QUEST_MAX_TARGETS; i++)
      {
         if (i > 0)
            *p++ = ',';
         p += sprintf(p, "%d", target_vnums[i]);
      }
      *p++ = '}';
      *p = '\0';
   }

   /* Format numeric strings; use NULL for absent FK values */
   sprintf(id_str, "%d", id);
   if (prereq >= 0)
      sprintf(prereq_str, "%d", prereq);
   sprintf(type_str, "%d", type);
   sprintf(ntargets_str, "%d", ntargets);
   sprintf(kill_str, "%d", kill_needed);
   sprintf(minlv_str, "%d", minlv);
   sprintf(maxlv_str, "%d", maxlv);
   if (offerer > 0)
      sprintf(offerer_str, "%d", offerer);
   sprintf(gold_str, "%d", reward_gold);
   sprintf(qp_str, "%d", reward_qp);
   sprintf(exp_str, "%d", reward_exp);
   sprintf(wear_str, "%d", obj_wear);
   sprintf(extra_str, "%d", obj_extra);
   sprintf(weight_str, "%d", obj_weight);
   sprintf(apply_str, "%d", obj_apply);

   params[0] = id_str;
   params[1] = title;
   params[2] = (prereq >= 0) ? prereq_str : NULL;
   params[3] = type_str;
   params[4] = ntargets_str;
   params[5] = vnums_pg;
   params[6] = kill_str;
   params[7] = minlv_str;
   params[8] = maxlv_str;
   params[9] = (offerer > 0) ? offerer_str : NULL;
   params[10] = gold_str;
   params[11] = qp_str;
   params[12] = exp_str;
   params[13] = accept;
   params[14] = completion;
   params[15] = obj_short;
   params[16] = obj_name;
   params[17] = obj_long;
   params[18] = wear_str;
   params[19] = extra_str;
   params[20] = weight_str;
   params[21] = apply_str;

   res = PQexecParams(conn,
                      "INSERT INTO quest_templates "
                      "(id, title, prerequisite_template_id, type, num_targets, target_vnums, "
                      " kill_needed, min_level, max_level, offerer_vnum, "
                      " reward_gold, reward_qp, reward_exp, "
                      " accept_message, completion_message, "
                      " reward_obj_short, reward_obj_name, reward_obj_long, "
                      " reward_obj_wear_flags, reward_obj_extra_flags, "
                      " reward_obj_weight, reward_obj_item_apply) "
                      "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,"
                      "        $11,$12,$13,$14,$15,$16,$17,$18,$19,$20,$21,$22) "
                      "ON CONFLICT (id) DO NOTHING",
                      22, NULL, params, NULL, NULL, 0);

   if (PQresultStatus(res) != PGRES_COMMAND_OK)
   {
      fprintf(errlog, "quest %d (%s): %s", id, path, PQresultErrorMessage(res));
      errors++;
      PQclear(res);
      return 0;
   }
   PQclear(res);
   return 1;
}

static int import_quests(const char *dirpath)
{
   DIR *d;
   struct dirent *ent;
   int count = 0;
   char path[512];

   d = opendir(dirpath);
   if (!d)
   {
      fprintf(errlog, "WARN: cannot open quest directory: %s\n", dirpath);
      return 0;
   }

   while ((ent = readdir(d)) != NULL)
   {
      int id;
      size_t len = strlen(ent->d_name);
      if (len < 6 || strcmp(ent->d_name + len - 5, ".prop") != 0)
         continue;
      id = atoi(ent->d_name);
      if (id <= 0)
         continue;
      snprintf(path, sizeof(path), "%s/%s", dirpath, ent->d_name);
      if (import_quest_file(path, id))
         count++;
   }
   closedir(d);
   return count;
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
   char *connstr = NULL;
   const char *area_dir = "../area";
   int n;

   printf("import_to_db — ACK!TNG database migration tool\n");

   errlog = fopen("import_errors.log", "w");
   if (!errlog)
   {
      fprintf(stderr, "WARN: cannot open import_errors.log for writing, falling back to stderr\n");
      errlog = stderr;
   }
   else
      printf("Errors and warnings will be written to: import_errors.log\n");

   if (argc > 1)
      connstr = argv[1];
   else
      connstr = read_db_conf(area_dir);

   conn = PQconnectdb(connstr ? connstr : "");
   if (connstr && argc <= 1)
      free(connstr);

   if (PQstatus(conn) != CONNECTION_OK)
   {
      fprintf(stderr, "Connection failed: %s\n", PQerrorMessage(conn));
      PQfinish(conn);
      return 1;
   }
   printf("Connected to database.\n");

   /* ------------------------------------------------------------------ */
   printf("\nImporting help files...\n");
   n = import_helpdir("../help", "help_entries");
   printf("  %d help file(s) imported.\n", n);

   printf("\nImporting shelp files...\n");
   n = import_helpdir("../shelp", "shelp_entries");
   printf("  %d shelp file(s) imported.\n", n);

   printf("\nImporting lore files...\n");
   n = import_loredir("../lore");
   printf("  %d lore file(s) imported.\n", n);

   /* ------------------------------------------------------------------ */
   printf("\nImporting area files from area.lst...\n");
   n = import_areas("../area/area.lst", "../area");
   printf("  %d area file(s) imported.\n", n);

   /* ------------------------------------------------------------------ */
   printf("\nImporting bans...\n");
   n = import_bans("../data/bans.lst");
   printf("  %d ban(s) imported.\n", n);

   printf("\nImporting socials...\n");
   n = import_socials("../data/socials.txt");
   printf("  %d social(s) imported.\n", n);

   printf("\nImporting rulers...\n");
   n = import_rulers("../data/rulers.lst");
   printf("  %d ruler(s) imported.\n", n);

   printf("\nImporting brands...\n");
   n = import_brands("../data/brands.lst");
   printf("  %d brand(s) imported.\n", n);

   printf("\nImporting clans...\n");
   n = import_clans("../data/clandata.dat");
   printf("  %d clan(s) imported.\n", n);

   printf("\nImporting boards...\n");
   n = import_boards("../area/boards");
   printf("  %d board file(s) imported.\n", n);

   printf("\nImporting quest templates...\n");
   n = import_quests("../quests");
   printf("  %d quest template(s) imported.\n", n);

   /* ------------------------------------------------------------------ */
   if (errors > 0)
      printf("\n%d error(s) encountered — see import_errors.log for details.\n", errors);
   else
      printf("\nAll imports completed successfully.\n");

   if (errlog != stderr)
      fclose(errlog);

   PQfinish(conn);
   return errors > 0 ? 1 : 0;
}
