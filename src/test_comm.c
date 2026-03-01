#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "globals.h"

extern bool check_parse_name(char *name);

MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];

bool is_name(const char *str, char *namelist)
{
   const char *scan = namelist;
   char token[MAX_INPUT_LENGTH];

   while (*scan != '\0')
   {
      size_t token_len = 0;

      while (isspace((unsigned char)*scan))
         ++scan;

      if (*scan == '\0')
         break;

      while (*scan != '\0' && !isspace((unsigned char)*scan) && token_len < sizeof(token) - 1)
      {
         token[token_len++] = *scan;
         ++scan;
      }
      token[token_len] = '\0';

      while (*scan != '\0' && !isspace((unsigned char)*scan))
         ++scan;

      if (strcasecmp(str, token) == 0)
         return TRUE;
   }

   return FALSE;
}

static void clear_mob_index(void)
{
   memset(mob_index_hash, 0, sizeof(mob_index_hash));
}

static void test_rejects_reserved_names(void)
{
   clear_mob_index();
   assert(check_parse_name("all") == FALSE);
   assert(check_parse_name("tank") == FALSE);
}

static void test_rejects_invalid_lengths(void)
{
   clear_mob_index();
   assert(check_parse_name("ab") == FALSE);
   assert(check_parse_name("abcdefghijklmn") == FALSE);
}

static void test_rejects_non_alpha_and_ill_names(void)
{
   clear_mob_index();
   assert(check_parse_name("abc123") == FALSE);
   assert(check_parse_name("Illl") == FALSE);
}

static void test_rejects_names_matching_mobs(void)
{
   MOB_INDEX_DATA guard = {0};

   clear_mob_index();
   guard.player_name = "guard captain";
   guard.next = NULL;
   mob_index_hash[7] = &guard;

   assert(check_parse_name("guard") == FALSE);
   assert(check_parse_name("captain") == FALSE);
}

static void test_accepts_valid_names(void)
{
   clear_mob_index();
   assert(check_parse_name("Zenith") == TRUE);
   assert(check_parse_name("Knight") == TRUE);
}

int main(void)
{
   test_rejects_reserved_names();
   test_rejects_invalid_lengths();
   test_rejects_non_alpha_and_ill_names();
   test_rejects_names_matching_mobs();
   test_accepts_valid_names();

   puts("test_comm: all tests passed");
   return 0;
}
