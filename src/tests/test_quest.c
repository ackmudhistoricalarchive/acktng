#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "globals.h"
#include "quest_unit.h"

time_t current_time;

void quest_obj_notify(CHAR_DATA *ch, OBJ_DATA *obj);
void quest_kill_notify(CHAR_DATA *ch, CHAR_DATA *victim);
void quest_room_notify(CHAR_DATA *ch, ROOM_INDEX_DATA *room);
void clear_quest(CHAR_DATA *ch);
void quest_cancel(CHAR_DATA *ch, int slot);
int quest_unit_canonical_postmaster_vnum(int vnum)
{
   return canonical_postmaster_vnum(vnum);
}

int quest_unit_calc_exp(int max_level, int is_boss, int is_cartography)
{
   return calc_quest_exp(max_level, is_boss ? TRUE : FALSE, is_cartography ? TRUE : FALSE);
}

char *_str_dup(const char *str, const char *func)
{
   (void)func;
   if (str == NULL)
      return NULL;
   {
      size_t len = strlen(str) + 1;
      char *out = malloc(len);
      assert(out != NULL);
      memcpy(out, str, len);
      return out;
   }
}

void _free_string(char *pstr, const char *func)
{
   (void)func;
   free(pstr);
}

void log_string(const char *str)
{
   (void)str;
}

void bug(const char *str, int param)
{
   (void)str;
   (void)param;
}

void bugf(char *fmt, ...)
{
   (void)fmt;
}

static int extracted_calls;
static int save_calls;
static OBJ_DATA *last_extracted;

void extract_obj(OBJ_DATA *obj)
{
   extracted_calls++;
   last_extracted = obj;
}

void do_save(CHAR_DATA *ch, char *argument)
{
   (void)ch;
   (void)argument;
   save_calls++;
}

void send_to_char(const char *txt, CHAR_DATA *ch)
{
   (void)txt;
   (void)ch;
}

static void reset_counters(void)
{
   extracted_calls = 0;
   save_calls = 0;
   last_extracted = NULL;
}

static CHAR_DATA make_char(PC_DATA *pc)
{
   CHAR_DATA ch;
   memset(&ch, 0, sizeof(ch));
   memset(pc, 0, sizeof(*pc));
   ch.pcdata = pc;
   ch.act = 0;
   ch.pcdata->quests[0].quest_type = QUEST_TYPE_COLLECT_ITEMS;
   return ch;
}

static OBJ_DATA make_obj(OBJ_INDEX_DATA *index, int vnum, const char *short_descr)
{
   OBJ_DATA obj;
   memset(&obj, 0, sizeof(obj));
   memset(index, 0, sizeof(*index));
   index->vnum = vnum;
   obj.pIndexData = index;
   obj.short_descr = (char *)short_descr;
   return obj;
}

static CHAR_DATA make_victim(MOB_INDEX_DATA *index, int vnum, const char *short_descr)
{
   CHAR_DATA victim;
   memset(&victim, 0, sizeof(victim));
   memset(index, 0, sizeof(*index));
   index->vnum = vnum;
   victim.act = ACT_IS_NPC;
   victim.pIndexData = index;
   victim.short_descr = (char *)short_descr;
   return victim;
}

static void test_extracts_and_saves_when_target_matches(void)
{
   PC_DATA pc;
   OBJ_INDEX_DATA index;
   CHAR_DATA ch = make_char(&pc);
   OBJ_DATA obj = make_obj(&index, 1234, "a target item");

   ch.pcdata->quests[0].quest_num_targets = 1;
   ch.pcdata->quests[0].quest_target_vnum[0] = 1234;
   ch.pcdata->quests[0].quest_target_done[0] = FALSE;

   reset_counters();
   quest_obj_notify(&ch, &obj);

   assert(ch.pcdata->quests[0].quest_target_done[0] == TRUE);
   assert(extracted_calls == 1);
   assert(last_extracted == &obj);
   assert(save_calls == 1);
}

static void test_no_extract_when_not_a_target(void)
{
   PC_DATA pc;
   OBJ_INDEX_DATA index;
   CHAR_DATA ch = make_char(&pc);
   OBJ_DATA obj = make_obj(&index, 5678, "other item");

   ch.pcdata->quests[0].quest_num_targets = 1;
   ch.pcdata->quests[0].quest_target_vnum[0] = 1234;

   reset_counters();
   quest_obj_notify(&ch, &obj);

   assert(ch.pcdata->quests[0].quest_target_done[0] == FALSE);
   assert(extracted_calls == 0);
   assert(save_calls == 0);
}

static void test_marks_completed_when_final_item_collected(void)
{
   PC_DATA pc;
   OBJ_INDEX_DATA index;
   CHAR_DATA ch = make_char(&pc);
   OBJ_DATA obj = make_obj(&index, 2222, "final target");

   ch.pcdata->quests[0].quest_num_targets = 2;
   ch.pcdata->quests[0].quest_target_vnum[0] = 1111;
   ch.pcdata->quests[0].quest_target_done[0] = TRUE;
   ch.pcdata->quests[0].quest_target_vnum[1] = 2222;

   reset_counters();
   quest_obj_notify(&ch, &obj);

   assert(ch.pcdata->quests[0].quest_target_done[1] == TRUE);
   assert(ch.pcdata->quests[0].quest_completed == TRUE);
   assert(extracted_calls == 1);
   assert(save_calls == 1);
}

static void test_collect_progress_works_for_nonzero_slot(void)
{
   PC_DATA pc;
   OBJ_INDEX_DATA index;
   CHAR_DATA ch = make_char(&pc);
   OBJ_DATA obj = make_obj(&index, 3333, "slot one target");

   ch.pcdata->quests[0].quest_type = QUEST_TYPE_NONE;
   ch.pcdata->quests[1].quest_type = QUEST_TYPE_COLLECT_ITEMS;
   ch.pcdata->quests[1].quest_num_targets = 1;
   ch.pcdata->quests[1].quest_target_vnum[0] = 3333;

   reset_counters();
   quest_obj_notify(&ch, &obj);

   assert(ch.pcdata->quests[1].quest_target_done[0] == TRUE);
   assert(extracted_calls == 1);
   assert(save_calls == 1);
}

static void test_kill_progress_works_for_nonzero_slot(void)
{
   PC_DATA pc;
   MOB_INDEX_DATA index;
   CHAR_DATA ch = make_char(&pc);
   CHAR_DATA victim = make_victim(&index, 4321, "a wanted brigand");

   ch.pcdata->quests[0].quest_type = QUEST_TYPE_NONE;
   ch.pcdata->quests[1].quest_type = QUEST_TYPE_KILL_COUNT;
   ch.pcdata->quests[1].quest_num_targets = 1;
   ch.pcdata->quests[1].quest_target_vnum[0] = 4321;
   ch.pcdata->quests[1].quest_kill_needed = 1;

   reset_counters();
   quest_kill_notify(&ch, &victim);

   assert(ch.pcdata->quests[1].quest_kill_count == 1);
   assert(ch.pcdata->quests[1].quest_completed == TRUE);
   assert(save_calls == 1);
}

static void test_kill_notify_ignores_non_matching_target(void)
{
   PC_DATA pc;
   MOB_INDEX_DATA index;
   CHAR_DATA ch = make_char(&pc);
   CHAR_DATA victim = make_victim(&index, 9999, "an unrelated foe");

   ch.pcdata->quests[0].quest_type = QUEST_TYPE_KILL_COUNT;
   ch.pcdata->quests[0].quest_num_targets = 1;
   ch.pcdata->quests[0].quest_target_vnum[0] = 1234;
   ch.pcdata->quests[0].quest_kill_needed = 2;

   reset_counters();
   quest_kill_notify(&ch, &victim);

   assert(ch.pcdata->quests[0].quest_kill_count == 0);
   assert(ch.pcdata->quests[0].quest_completed == FALSE);
   assert(save_calls == 0);
}

static void test_clear_quest_resets_all_slots(void)
{
   PC_DATA pc;
   CHAR_DATA ch = make_char(&pc);

   ch.pcdata->quests[0].quest_type = QUEST_TYPE_KILL_COUNT;
   ch.pcdata->quests[0].quest_template_id = 4;
   ch.pcdata->quests[0].quest_reward_item_vnum = 123;
   ch.pcdata->quests[1].quest_type = QUEST_TYPE_COLLECT_ITEMS;
   ch.pcdata->quests[1].quest_num_targets = 2;
   ch.pcdata->quests[1].quest_target_vnum[0] = 88;
   ch.pcdata->quests[2].quest_type = QUEST_TYPE_KILL_VARIETY;
   ch.pcdata->quests[2].quest_target_done[0] = TRUE;

   clear_quest(&ch);

   for (int i = 0; i < QUEST_MAX_QUESTS; i++)
   {
      assert(ch.pcdata->quests[i].quest_type == QUEST_TYPE_NONE);
      assert(ch.pcdata->quests[i].quest_num_targets == 0);
      assert(ch.pcdata->quests[i].quest_completed == FALSE);
      assert(ch.pcdata->quests[i].quest_template_id == -1);
      assert(ch.pcdata->quests[i].quest_reward_item_vnum == 0);
      assert(ch.pcdata->quests[i].quest_cartography_area_num == -1);
      assert(ch.pcdata->quests[i].quest_cartography_room_count == 0);
      assert(ch.pcdata->quests[i].quest_cartography_explored_count == 0);
   }
}

static void test_cartography_marks_room_once_and_completes(void)
{
   PC_DATA pc;
   CHAR_DATA ch = make_char(&pc);
   AREA_DATA area;
   ROOM_INDEX_DATA room1;
   ROOM_INDEX_DATA room2;
   BUILD_DATA_LIST node1;
   BUILD_DATA_LIST node2;

   memset(&area, 0, sizeof(area));
   memset(&room1, 0, sizeof(room1));
   memset(&room2, 0, sizeof(room2));
   memset(&node1, 0, sizeof(node1));
   memset(&node2, 0, sizeof(node2));

   area.area_num = 77;
   area.first_area_room = &node1;
   room1.area = &area;
   room2.area = &area;

   node1.data = &room1;
   node1.next = &node2;
   node2.data = &room2;

   ch.pcdata->quests[0].quest_type = QUEST_TYPE_CARTOGRAPHY;
   ch.pcdata->quests[0].quest_cartography_area_num = 77;
   ch.pcdata->quests[0].quest_cartography_room_count = 2;
   ch.pcdata->quests[0].quest_cartography_explored_count = 0;

   reset_counters();
   quest_room_notify(&ch, &room1);
   assert(ch.pcdata->quests[0].quest_cartography_explored_count == 1);
   assert(ch.pcdata->quests[0].quest_completed == FALSE);
   assert(save_calls == 1);

   quest_room_notify(&ch, &room1);
   assert(ch.pcdata->quests[0].quest_cartography_explored_count == 1);
   assert(save_calls == 1);

   quest_room_notify(&ch, &room2);
   assert(ch.pcdata->quests[0].quest_cartography_explored_count == 2);
   assert(ch.pcdata->quests[0].quest_completed == TRUE);
   assert(save_calls == 2);
}

static void test_cancel_dynamic_sets_cooldown_and_clears_slot(void)
{
   PC_DATA pc;
   CHAR_DATA ch = make_char(&pc);

   current_time = 1000;
   ch.pcdata->quests[0].quest_type = QUEST_TYPE_KILL_COUNT;
   ch.pcdata->quests[0].quest_template_id = -1;

   reset_counters();
   quest_cancel(&ch, 0);

   assert(ch.pcdata->quests[0].quest_type == QUEST_TYPE_NONE);
   assert(ch.pcdata->quest_dynamic_cooldown_until == 1900);
   assert(save_calls == 1);
}

static void test_cancel_template_does_not_set_cooldown(void)
{
   PC_DATA pc;
   CHAR_DATA ch = make_char(&pc);

   current_time = 2000;
   ch.pcdata->quests[1].quest_type = QUEST_TYPE_COLLECT_ITEMS;
   ch.pcdata->quests[1].quest_template_id = 2;

   reset_counters();
   quest_cancel(&ch, 1);

   assert(ch.pcdata->quests[1].quest_type == QUEST_TYPE_NONE);
   assert(ch.pcdata->quest_dynamic_cooldown_until == 0);
   assert(save_calls == 1);
}

static void test_postmaster_aliases_map_to_active_city_vnums(void)
{
   assert(quest_unit_canonical_postmaster_vnum(3340) == 3340);
   assert(quest_unit_canonical_postmaster_vnum(3360) == 3340);
   assert(quest_unit_canonical_postmaster_vnum(3440) == 3440);
   assert(quest_unit_canonical_postmaster_vnum(3460) == 3440);
   assert(quest_unit_canonical_postmaster_vnum(0) == 3440);
   assert(quest_unit_canonical_postmaster_vnum(931) == 931);
}

static void test_quest_exp_calculation(void)
{
   /* exp_table[20].mob_base == 5800 (from const.c) */
   int base = 3 * 5800; /* 17400 */
   assert(quest_unit_calc_exp(20, 0, 0) == base);
   assert(quest_unit_calc_exp(20, 1, 0) == base * 2);  /* boss: double */
   assert(quest_unit_calc_exp(20, 0, 1) == base * 10); /* cartography: 10x */
}

static void test_dynamic_quest_exp_uses_3x_mob_base(void)
{
   /* Dynamic quest exp = 3 * exp_table[pseudo_level].mob_base. */
   /* exp_table[20].mob_base == 5800; 3x = 17400 */
   assert(quest_unit_calc_exp(20, 0, 0) == 3 * 5800);
   /* exp_table[30].mob_base == 13000; 3x = 39000 */
   assert(quest_unit_calc_exp(30, 0, 0) == 3 * 13000);
}

int main(void)
{
   test_extracts_and_saves_when_target_matches();
   test_no_extract_when_not_a_target();
   test_marks_completed_when_final_item_collected();
   test_collect_progress_works_for_nonzero_slot();
   test_kill_progress_works_for_nonzero_slot();
   test_kill_notify_ignores_non_matching_target();
   test_clear_quest_resets_all_slots();
   test_cartography_marks_room_once_and_completes();
   test_cancel_dynamic_sets_cooldown_and_clears_slot();
   test_cancel_template_does_not_set_cooldown();
   test_postmaster_aliases_map_to_active_city_vnums();
   test_quest_exp_calculation();
   test_dynamic_quest_exp_uses_3x_mob_base();

   puts("test_quest: all tests passed");
   return 0;
}
