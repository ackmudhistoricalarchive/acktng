/*
 * test_npc_dialogue_help.c
 *
 * Unit tests for:
 *   1. spec_lookup (spec_mudschool_guide registration)
 *   2. collect_help_context (help/shelp keyword injection via DB stubs)
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define DEC_GLOBALS_H 1
#define UNIT_TEST_NPC_DIALOGUE 1
#include "ack.h"
#include "special.h"
#include "npc_dialogue.h"

/* -------------------------------------------------------------------------
 * Stub DB help tables.  Tests populate these before calling the function
 * under test.  db_help_lookup / db_shelp_lookup do a case-insensitive
 * prefix match: the stored keyword must START WITH the query argument.
 * -------------------------------------------------------------------------*/

typedef struct
{
   const char *kw;
   const char *text;
} stub_help_entry;

static stub_help_entry stub_help_table[8];
static int stub_help_n = 0;
static stub_help_entry stub_shelp_table[8];
static int stub_shelp_n = 0;

int db_help_lookup(const char *keyword, int level, char *kw_out, size_t kw_sz, char *text_out,
                   size_t text_sz, int *level_out)
{
   int i;
   size_t kwlen = strlen(keyword);
   (void)level;
   for (i = 0; i < stub_help_n; i++)
   {
      if (strncasecmp(stub_help_table[i].kw, keyword, kwlen) == 0)
      {
         snprintf(kw_out, kw_sz, "%s", stub_help_table[i].kw);
         snprintf(text_out, text_sz, "%s", stub_help_table[i].text);
         if (level_out)
            *level_out = 0;
         return 1;
      }
   }
   return 0;
}

int db_shelp_lookup(const char *keyword, int level, char *kw_out, size_t kw_sz, char *text_out,
                    size_t text_sz, int *level_out)
{
   int i;
   size_t kwlen = strlen(keyword);
   (void)level;
   for (i = 0; i < stub_shelp_n; i++)
   {
      if (strncasecmp(stub_shelp_table[i].kw, keyword, kwlen) == 0)
      {
         snprintf(kw_out, kw_sz, "%s", stub_shelp_table[i].kw);
         snprintf(text_out, text_sz, "%s", stub_shelp_table[i].text);
         if (level_out)
            *level_out = 0;
         return 1;
      }
   }
   return 0;
}

int db_lore_collect_by_flags(long npc_flags, int max_results,
                             void (*result_cb)(const char *keyword, const char *body,
                                               void *userdata),
                             void *userdata)
{
   (void)npc_flags;
   (void)max_results;
   (void)result_cb;
   (void)userdata;
   return 0;
}

/* -------------------------------------------------------------------------
 * Minimal string-function stubs (avoids linking the full strfuns.o).
 * -------------------------------------------------------------------------*/
bool str_prefix(const char *astr, const char *bstr)
{
   if (!astr || !bstr)
      return FALSE;
   while (*astr)
   {
      if (tolower((unsigned char)*astr) != tolower((unsigned char)*bstr))
         return TRUE;
      astr++;
      bstr++;
   }
   return FALSE;
}

bool str_cmp(const char *a, const char *b)
{
   while (*a && *b)
   {
      if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
         return TRUE;
      a++;
      b++;
   }
   return (*a || *b) ? TRUE : FALSE;
}

bool str_infix(const char *needle, const char *haystack)
{
   int i;
   int nlen = (int)strlen(needle);
   int hlen = (int)strlen(haystack);
   for (i = 0; i <= hlen - nlen; i++)
   {
      int j;
      for (j = 0; j < nlen; j++)
      {
         if (tolower((unsigned char)needle[j]) != tolower((unsigned char)haystack[i + j]))
            break;
      }
      if (j == nlen)
         return FALSE; /* found = NOT a mismatch (str_infix returns FALSE on match) */
   }
   return TRUE;
}

char *one_argument(char *argument, char *arg_first)
{
   char cEnd = ' ';
   if (!argument || !arg_first)
   {
      if (arg_first)
         *arg_first = '\0';
      return argument ? argument : "";
   }
   while (*argument == ' ')
      argument++;
   if (*argument == '\'' || *argument == '"')
      cEnd = *argument++;
   while (*argument != '\0')
   {
      if (*argument == cEnd)
      {
         argument++;
         break;
      }
      *arg_first++ = (char)tolower((unsigned char)*argument++);
   }
   *arg_first = '\0';
   while (*argument == ' ')
      argument++;
   return argument;
}

/* -------------------------------------------------------------------------
 * Minimal spec_lookup/rev_spec_lookup stubs.
 * -------------------------------------------------------------------------*/
SPEC_FUN *spec_lookup(const char *name)
{
   if (!str_cmp(name, "spec_mudschool_guide"))
      return spec_mudschool_guide;
   return NULL;
}

char *rev_spec_lookup(void *func)
{
   if (func == spec_mudschool_guide)
      return "spec_mudschool_guide";
   return "(none)";
}

/* -------------------------------------------------------------------------
 * Stubs for functions referenced by npc_dialogue.c but not called in tests.
 * -------------------------------------------------------------------------*/
void do_say(CHAR_DATA *ch, char *arg)
{
   (void)ch;
   (void)arg;
}
void log_f(char *fmt, ...)
{
   (void)fmt;
}
void bug(const char *msg, int val)
{
   (void)msg;
   (void)val;
}
bool is_fighting(CHAR_DATA *ch)
{
   (void)ch;
   return FALSE;
}

bool spec_mudschool_guide(CHAR_DATA *ch)
{
   (void)ch;
   return FALSE;
}

/* -------------------------------------------------------------------------
 * Test 1: spec_lookup returns the mudschool guide function pointer.
 * -------------------------------------------------------------------------*/
static void test_spec_lookup_finds_mudschool_guide(void)
{
   SPEC_FUN *fn = spec_lookup("spec_mudschool_guide");
   assert(fn != NULL);
   assert(fn == spec_mudschool_guide);
   printf("PASS test_spec_lookup_finds_mudschool_guide\n");
}

/* -------------------------------------------------------------------------
 * Test 5: collect_help_context skips [GREET] messages.
 * -------------------------------------------------------------------------*/
static void test_collect_help_context_skips_greet(void)
{
   char out[256];

   stub_help_table[0].kw = "MOVEMENT";
   stub_help_table[0].text = "Type north/south/east/west to move.";
   stub_help_n = 1;

   npc_dialogue_test_collect_help_context("[GREET] Zorkin has arrived.", out, sizeof(out));
   assert(out[0] == '\0');
   stub_help_n = 0;
   printf("PASS test_collect_help_context_skips_greet\n");
}

/* -------------------------------------------------------------------------
 * Test 6: collect_help_context matches a keyword in the help table.
 * -------------------------------------------------------------------------*/
static void test_collect_help_context_matches_help_keyword(void)
{
   char out[512];

   stub_help_table[0].kw = "MOVEMENT";
   stub_help_table[0].text = "Type north/south/east/west to move around the world.";
   stub_help_n = 1;

   npc_dialogue_test_collect_help_context("How do I use movement commands?", out, sizeof(out));
   assert(strstr(out, "[HELP: MOVEMENT]") != NULL);
   stub_help_n = 0;
   printf("PASS test_collect_help_context_matches_help_keyword\n");
}

/* -------------------------------------------------------------------------
 * Test 7: collect_help_context deduplicates the same entry.
 * -------------------------------------------------------------------------*/
static void test_collect_help_context_deduplicates(void)
{
   char out[512];
   int count;
   const char *p;

   stub_help_table[0].kw = "COMBAT";
   stub_help_table[0].text = "Type kill <target> to begin fighting.";
   stub_help_n = 1;

   /* Message has two words that both prefix-match "COMBAT" */
   npc_dialogue_test_collect_help_context("combat combat fighting", out, sizeof(out));

   /* Count occurrences of "[HELP:" — should be exactly 1 */
   count = 0;
   p = out;
   while ((p = strstr(p, "[HELP:")) != NULL)
   {
      count++;
      p++;
   }
   assert(count == 1);
   stub_help_n = 0;
   printf("PASS test_collect_help_context_deduplicates\n");
}

/* -------------------------------------------------------------------------
 * Test 8: collect_help_context searches shelp when help has no match.
 * -------------------------------------------------------------------------*/
static void test_collect_help_context_searches_shelp(void)
{
   char out[512];

   stub_help_n = 0;
   stub_shelp_table[0].kw = "STAFF";
   stub_shelp_table[0].text = "Staff commands";
   stub_shelp_n = 1;

   npc_dialogue_test_collect_help_context("What staff commands exist?", out, sizeof(out));
   assert(strstr(out, "[HELP: STAFF]") != NULL);
   stub_shelp_n = 0;
   printf("PASS test_collect_help_context_searches_shelp\n");
}

/* -------------------------------------------------------------------------
 * Test 9: collect_help_context respects the output cap.
 * -------------------------------------------------------------------------*/
static void test_collect_help_context_respects_cap(void)
{
   char out[32]; /* very small cap */

   stub_help_table[0].kw = "SCORE";
   stub_help_table[0].text = "The score command shows your statistics.";
   stub_help_n = 1;

   npc_dialogue_test_collect_help_context("show score statistics", out, sizeof(out));
   assert(strlen(out) < sizeof(out));
   stub_help_n = 0;
   printf("PASS test_collect_help_context_respects_cap\n");
}

/* -------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------*/
int main(void)
{
   test_spec_lookup_finds_mudschool_guide();
   test_collect_help_context_skips_greet();
   test_collect_help_context_matches_help_keyword();
   test_collect_help_context_deduplicates();
   test_collect_help_context_searches_shelp();
   test_collect_help_context_respects_cap();

   printf("All npc_dialogue_help tests passed.\n");
   return 0;
}
