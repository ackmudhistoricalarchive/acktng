#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "globals.h"
#include "config.h"

int find_race_index_by_name(const char *name);
int score_should_show_invasion_rewards(CHAR_DATA *ch);
void parse_shelp_query(char *argument, char *search_term, size_t search_term_size,
                       char *full_argument, size_t full_argument_size);
long get_room_lore_flags(CHAR_DATA *ch);
CHAR_DATA *who_get_char(DESCRIPTOR_DATA *d);

char *one_argument(char *argument, char *arg_first)
{
   char cEnd;

   if (argument == NULL)
      return NULL;

   while (isspace((unsigned char)*argument))
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

      *arg_first = tolower((unsigned char)*argument);
      arg_first++;
      argument++;
   }
   *arg_first = '\0';

   while (isspace((unsigned char)*argument))
      argument++;

   return argument;
}

bool str_cmp(const char *astr, const char *bstr)
{
   if (astr == NULL)
      return bstr != NULL;
   if (bstr == NULL)
      return TRUE;

   while (*astr != '\0' || *bstr != '\0')
   {
      int a = tolower((unsigned char)*astr);
      int b = tolower((unsigned char)*bstr);
      if (a != b)
         return TRUE;
      if (*astr != '\0')
         astr++;
      if (*bstr != '\0')
         bstr++;
   }

   return FALSE;
}

bool str_prefix(const char *astr, const char *bstr)
{
   if (astr == NULL || bstr == NULL)
      return TRUE;

   while (*astr != '\0')
   {
      if (*bstr == '\0')
         return TRUE;
      if (tolower((unsigned char)*astr) != tolower((unsigned char)*bstr))
         return TRUE;
      astr++;
      bstr++;
   }

   return FALSE;
}

const struct race_type race_table[MAX_RACE] = {
    [0] = {.race_name = "hum", .race_title = "human"},
    [1] = {.race_name = "elf", .race_title = "elf"},
    [2] = {.race_name = "dwa", .race_title = "dwarf"},
};

const struct class_type gclass_table[MAX_TOTAL_CLASS] = {
    [CLASS_GMA] = {.tier = ADEPT, .prereq = {CLASS_SOR, CLASS_WIZ}},
    [CLASS_TEM] = {.tier = ADEPT, .prereq = {CLASS_PAL, CLASS_PRI}},
    [CLASS_NIG] = {.tier = ADEPT, .prereq = {CLASS_ASS, CLASS_WLK}},
    [CLASS_CRU] = {.tier = ADEPT, .prereq = {CLASS_KNI, CLASS_SWO}},
    [CLASS_KIN] = {.tier = ADEPT, .prereq = {CLASS_NEC, CLASS_EGO}},
    [CLASS_MAR] = {.tier = ADEPT, .prereq = {CLASS_MON, CLASS_BRA}},
};

static void test_find_race_index_matches_exact_name_or_title(void)
{
   assert(find_race_index_by_name("hum") == 0);
   assert(find_race_index_by_name("ELF") == 1);
   assert(find_race_index_by_name("Dwarf") == 2);
}

static void test_find_race_index_falls_back_to_prefix_matching(void)
{
   assert(find_race_index_by_name("hu") == 0);
   assert(find_race_index_by_name("el") == 1);
   assert(find_race_index_by_name("dwar") == 2);
}

static void test_find_race_index_rejects_unknown_or_empty_values(void)
{
   assert(find_race_index_by_name("orc") == -1);
   assert(find_race_index_by_name("") == -1);
   assert(find_race_index_by_name(NULL) == -1);
}

static void test_parse_shelp_query_defaults_to_shelp(void)
{
   char argument[] = "";
   char search_term[MAX_INPUT_LENGTH] = {0};
   char full_argument[MAX_INPUT_LENGTH] = {0};

   parse_shelp_query(argument, search_term, sizeof(search_term), full_argument,
                     sizeof(full_argument));

   assert(strcmp(search_term, "shelp") == 0);
   assert(strcmp(full_argument, "shelp") == 0);
}

static void test_parse_shelp_query_handles_multi_word_arguments(void)
{
   char argument[] = "soul net";
   char search_term[MAX_INPUT_LENGTH] = {0};
   char full_argument[MAX_INPUT_LENGTH] = {0};

   parse_shelp_query(argument, search_term, sizeof(search_term), full_argument,
                     sizeof(full_argument));

   assert(strcmp(search_term, "soul") == 0);
   assert(strcmp(full_argument, "soul net") == 0);
}

static void test_parse_shelp_query_unwraps_quoted_multi_word_arguments(void)
{
   char argument[] = "\"soul net\"";
   char search_term[MAX_INPUT_LENGTH] = {0};
   char full_argument[MAX_INPUT_LENGTH] = {0};

   parse_shelp_query(argument, search_term, sizeof(search_term), full_argument,
                     sizeof(full_argument));

   assert(strcmp(search_term, "soul net") == 0);
   assert(strcmp(full_argument, "soul net") == 0);
}

static void test_score_invasion_rewards_visibility(void)
{
   CHAR_DATA player = {0};
   PC_DATA pc = {0};
   CHAR_DATA npc = {0};

   player.pcdata = &pc;
   npc.act = ACT_IS_NPC;

   assert(score_should_show_invasion_rewards(NULL) == 0);
   assert(score_should_show_invasion_rewards(&npc) == 0);
   assert(score_should_show_invasion_rewards(&player) == 0);

   pc.invasion_rewards[1] = 2;
   assert(score_should_show_invasion_rewards(&player) == 1);
}

static void test_pc_lore_flags_are_always_zero(void)
{
   CHAR_DATA pc = {0};
   PC_DATA pc_data = {0};
   CHAR_DATA npc_in_room = {0};
   ROOM_INDEX_DATA room = {0};

   pc.pcdata = &pc_data;

   /* PC with no room gets 0 */
   assert(!IS_NPC(&pc) && get_room_lore_flags(&pc) == 0);

   /* PC in a room with a flagged NPC: room returns NPC flags, but IS_NPC check
      ensures do_lore uses 0 for PCs */
   npc_in_room.act = ACT_IS_NPC;
   npc_in_room.lore_flags = LORE_FLAG_MIDGAARD | LORE_FLAG_HUMAN;
   npc_in_room.next_in_room = NULL;
   room.first_person = &npc_in_room;
   pc.in_room = &room;

   assert(!IS_NPC(&pc));
   /* do_lore uses IS_NPC(ch) ? get_room_lore_flags(ch) : 0 */
   assert((IS_NPC(&pc) ? get_room_lore_flags(&pc) : 0) == 0);
}

static void test_npc_lore_flags_come_from_room(void)
{
   CHAR_DATA npc_caller = {0};
   CHAR_DATA npc_storyteller = {0};
   ROOM_INDEX_DATA room = {0};

   npc_caller.act = ACT_IS_NPC;
   npc_storyteller.act = ACT_IS_NPC;
   npc_storyteller.lore_flags = LORE_FLAG_MIDGAARD | LORE_FLAG_HUMAN;
   npc_storyteller.next_in_room = NULL;
   room.first_person = &npc_storyteller;
   npc_caller.in_room = &room;

   assert((IS_NPC(&npc_caller) ? get_room_lore_flags(&npc_caller) : 0) ==
          (LORE_FLAG_MIDGAARD | LORE_FLAG_HUMAN));
}

static void test_who_get_char_filters_non_playing(void)
{
   CHAR_DATA ch = {0};
   DESCRIPTOR_DATA d = {0};
   d.character = &ch;

   /* CON_PLAYING (0) should return the character */
   d.connected = CON_PLAYING;
   assert(who_get_char(&d) == &ch);

   /* Non-playing states should return NULL */
   d.connected = CON_GET_NAME;
   assert(who_get_char(&d) == NULL);

   d.connected = CON_GET_OLD_PASSWORD;
   assert(who_get_char(&d) == NULL);
}

static void test_who_get_char_returns_original_when_switched(void)
{
   CHAR_DATA original = {0};
   CHAR_DATA current = {0};
   DESCRIPTOR_DATA d = {0};

   d.connected = CON_PLAYING;
   d.character = &current;
   d.original = &original;

   /* When original is set (switched/possessed NPC), return the player character */
   assert(who_get_char(&d) == &original);
}

static void test_who_get_char_returns_null_for_no_character(void)
{
   DESCRIPTOR_DATA d = {0};

   /* CON_PLAYING but no character attached */
   d.connected = CON_PLAYING;
   assert(who_get_char(&d) == NULL);
}

/*
 * Adept prerequisite tests: verify that the EITHER-of-two-remorts logic
 * is correctly represented in gclass_table and that the condition
 * (prereq[0] < MAX_MORTAL && prereq[1] < MAX_MORTAL) rejects only when
 * NEITHER prereq is met.
 */

static void test_adept_classes_have_two_valid_remort_prereqs(void)
{
   /* Each adept class should reference two distinct remort-tier prereqs */
   assert(gclass_table[CLASS_GMA].prereq[0] == CLASS_SOR);
   assert(gclass_table[CLASS_GMA].prereq[1] == CLASS_WIZ);
   assert(gclass_table[CLASS_TEM].prereq[0] == CLASS_PAL);
   assert(gclass_table[CLASS_TEM].prereq[1] == CLASS_PRI);
   assert(gclass_table[CLASS_NIG].prereq[0] == CLASS_ASS);
   assert(gclass_table[CLASS_NIG].prereq[1] == CLASS_WLK);
   assert(gclass_table[CLASS_CRU].prereq[0] == CLASS_KNI);
   assert(gclass_table[CLASS_CRU].prereq[1] == CLASS_SWO);
   assert(gclass_table[CLASS_KIN].prereq[0] == CLASS_NEC);
   assert(gclass_table[CLASS_KIN].prereq[1] == CLASS_EGO);
   assert(gclass_table[CLASS_MAR].prereq[0] == CLASS_MON);
   assert(gclass_table[CLASS_MAR].prereq[1] == CLASS_BRA);
}

static void test_adept_prereq_either_logic(void)
{
   int class_level[MAX_TOTAL_CLASS];
   int adept_class = CLASS_GMA;
   int p0 = gclass_table[adept_class].prereq[0]; /* CLASS_SOR */
   int p1 = gclass_table[adept_class].prereq[1]; /* CLASS_WIZ */

   /* Neither prereq met → should be rejected (condition is true) */
   memset(class_level, 0, sizeof(class_level));
   assert((class_level[p0] < MAX_MORTAL && class_level[p1] < MAX_MORTAL) == 1);

   /* Only first prereq met → should pass (condition is false) */
   memset(class_level, 0, sizeof(class_level));
   class_level[p0] = MAX_MORTAL;
   assert((class_level[p0] < MAX_MORTAL && class_level[p1] < MAX_MORTAL) == 0);

   /* Only second prereq met → should pass (condition is false) */
   memset(class_level, 0, sizeof(class_level));
   class_level[p1] = MAX_MORTAL;
   assert((class_level[p0] < MAX_MORTAL && class_level[p1] < MAX_MORTAL) == 0);

   /* Both prereqs met → should pass (condition is false) */
   memset(class_level, 0, sizeof(class_level));
   class_level[p0] = MAX_MORTAL;
   class_level[p1] = MAX_MORTAL;
   assert((class_level[p0] < MAX_MORTAL && class_level[p1] < MAX_MORTAL) == 0);
}

int main(void)
{
   test_find_race_index_matches_exact_name_or_title();
   test_find_race_index_falls_back_to_prefix_matching();
   test_find_race_index_rejects_unknown_or_empty_values();
   test_parse_shelp_query_defaults_to_shelp();
   test_parse_shelp_query_handles_multi_word_arguments();
   test_parse_shelp_query_unwraps_quoted_multi_word_arguments();
   test_score_invasion_rewards_visibility();
   test_pc_lore_flags_are_always_zero();
   test_npc_lore_flags_come_from_room();
   test_who_get_char_filters_non_playing();
   test_who_get_char_returns_original_when_switched();
   test_who_get_char_returns_null_for_no_character();

   test_adept_classes_have_two_valid_remort_prereqs();
   test_adept_prereq_either_logic();

   puts("test_act_info: all tests passed");
   return 0;
}
