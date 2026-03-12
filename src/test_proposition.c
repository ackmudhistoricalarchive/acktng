#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "globals.h"

time_t current_time;
AREA_DATA *first_area;
AREA_DATA *last_area;

void proposition_obj_notify(CHAR_DATA *ch, OBJ_DATA *obj);
void proposition_kill_notify(CHAR_DATA *ch, CHAR_DATA *victim);
void proposition_room_notify(CHAR_DATA *ch, ROOM_INDEX_DATA *room);
void clear_proposition(CHAR_DATA *ch);
void proposition_cancel(CHAR_DATA *ch, int slot);
void proposition_load_static_templates(void);
int proposition_unit_static_count(void);
const char *proposition_unit_static_title(int static_id);
const char *proposition_unit_static_accept_message(int static_id);
const char *proposition_unit_static_completion_message(int static_id);
int proposition_unit_static_max_level(int static_id);


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
    ch.pcdata->propositions[0].prop_type = PROP_TYPE_COLLECT_ITEMS;
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

    ch.pcdata->propositions[0].prop_num_targets = 1;
    ch.pcdata->propositions[0].prop_target_vnum[0] = 1234;
    ch.pcdata->propositions[0].prop_target_done[0] = FALSE;

    reset_counters();
    proposition_obj_notify(&ch, &obj);

    assert(ch.pcdata->propositions[0].prop_target_done[0] == TRUE);
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

    ch.pcdata->propositions[0].prop_num_targets = 1;
    ch.pcdata->propositions[0].prop_target_vnum[0] = 1234;

    reset_counters();
    proposition_obj_notify(&ch, &obj);

    assert(ch.pcdata->propositions[0].prop_target_done[0] == FALSE);
    assert(extracted_calls == 0);
    assert(save_calls == 0);
}

static void test_marks_completed_when_final_item_collected(void)
{
    PC_DATA pc;
    OBJ_INDEX_DATA index;
    CHAR_DATA ch = make_char(&pc);
    OBJ_DATA obj = make_obj(&index, 2222, "final target");

    ch.pcdata->propositions[0].prop_num_targets = 2;
    ch.pcdata->propositions[0].prop_target_vnum[0] = 1111;
    ch.pcdata->propositions[0].prop_target_done[0] = TRUE;
    ch.pcdata->propositions[0].prop_target_vnum[1] = 2222;

    reset_counters();
    proposition_obj_notify(&ch, &obj);

    assert(ch.pcdata->propositions[0].prop_target_done[1] == TRUE);
    assert(ch.pcdata->propositions[0].prop_completed == TRUE);
    assert(extracted_calls == 1);
    assert(save_calls == 1);
}

static void test_collect_progress_works_for_nonzero_slot(void)
{
    PC_DATA pc;
    OBJ_INDEX_DATA index;
    CHAR_DATA ch = make_char(&pc);
    OBJ_DATA obj = make_obj(&index, 3333, "slot one target");

    ch.pcdata->propositions[0].prop_type = PROP_TYPE_NONE;
    ch.pcdata->propositions[1].prop_type = PROP_TYPE_COLLECT_ITEMS;
    ch.pcdata->propositions[1].prop_num_targets = 1;
    ch.pcdata->propositions[1].prop_target_vnum[0] = 3333;

    reset_counters();
    proposition_obj_notify(&ch, &obj);

    assert(ch.pcdata->propositions[1].prop_target_done[0] == TRUE);
    assert(extracted_calls == 1);
    assert(save_calls == 1);
}

static void test_kill_progress_works_for_nonzero_slot(void)
{
    PC_DATA pc;
    MOB_INDEX_DATA index;
    CHAR_DATA ch = make_char(&pc);
    CHAR_DATA victim = make_victim(&index, 4321, "a wanted brigand");

    ch.pcdata->propositions[0].prop_type = PROP_TYPE_NONE;
    ch.pcdata->propositions[1].prop_type = PROP_TYPE_KILL_COUNT;
    ch.pcdata->propositions[1].prop_num_targets = 1;
    ch.pcdata->propositions[1].prop_target_vnum[0] = 4321;
    ch.pcdata->propositions[1].prop_kill_needed = 1;

    reset_counters();
    proposition_kill_notify(&ch, &victim);

    assert(ch.pcdata->propositions[1].prop_kill_count == 1);
    assert(ch.pcdata->propositions[1].prop_completed == TRUE);
    assert(save_calls == 1);
}

static void test_kill_notify_ignores_non_matching_target(void)
{
    PC_DATA pc;
    MOB_INDEX_DATA index;
    CHAR_DATA ch = make_char(&pc);
    CHAR_DATA victim = make_victim(&index, 9999, "an unrelated foe");

    ch.pcdata->propositions[0].prop_type = PROP_TYPE_KILL_COUNT;
    ch.pcdata->propositions[0].prop_num_targets = 1;
    ch.pcdata->propositions[0].prop_target_vnum[0] = 1234;
    ch.pcdata->propositions[0].prop_kill_needed = 2;

    reset_counters();
    proposition_kill_notify(&ch, &victim);

    assert(ch.pcdata->propositions[0].prop_kill_count == 0);
    assert(ch.pcdata->propositions[0].prop_completed == FALSE);
    assert(save_calls == 0);
}

static void test_clear_proposition_resets_all_slots(void)
{
    PC_DATA pc;
    CHAR_DATA ch = make_char(&pc);

    ch.pcdata->propositions[0].prop_type = PROP_TYPE_KILL_COUNT;
    ch.pcdata->propositions[0].prop_static_id = 4;
    ch.pcdata->propositions[0].prop_reward_item_vnum = 123;
    ch.pcdata->propositions[1].prop_type = PROP_TYPE_COLLECT_ITEMS;
    ch.pcdata->propositions[1].prop_num_targets = 2;
    ch.pcdata->propositions[1].prop_target_vnum[0] = 88;
    ch.pcdata->propositions[2].prop_type = PROP_TYPE_KILL_VARIETY;
    ch.pcdata->propositions[2].prop_target_done[0] = TRUE;

    clear_proposition(&ch);

    for (int i = 0; i < PROP_MAX_PROPOSITIONS; i++)
    {
        assert(ch.pcdata->propositions[i].prop_type == PROP_TYPE_NONE);
        assert(ch.pcdata->propositions[i].prop_num_targets == 0);
        assert(ch.pcdata->propositions[i].prop_completed == FALSE);
        assert(ch.pcdata->propositions[i].prop_static_id == -1);
        assert(ch.pcdata->propositions[i].prop_reward_item_vnum == 0);
    }
}


static void test_cancel_dynamic_sets_cooldown_and_clears_slot(void)
{
    PC_DATA pc;
    CHAR_DATA ch = make_char(&pc);

    current_time = 1000;
    ch.pcdata->propositions[0].prop_type = PROP_TYPE_KILL_COUNT;
    ch.pcdata->propositions[0].prop_static_id = -1;

    reset_counters();
    proposition_cancel(&ch, 0);

    assert(ch.pcdata->propositions[0].prop_type == PROP_TYPE_NONE);
    assert(ch.pcdata->prop_dynamic_cooldown_until == 1900);
    assert(save_calls == 1);
}

static void test_cancel_static_does_not_set_cooldown(void)
{
    PC_DATA pc;
    CHAR_DATA ch = make_char(&pc);

    current_time = 2000;
    ch.pcdata->propositions[1].prop_type = PROP_TYPE_COLLECT_ITEMS;
    ch.pcdata->propositions[1].prop_static_id = 2;

    reset_counters();
    proposition_cancel(&ch, 1);

    assert(ch.pcdata->propositions[1].prop_type == PROP_TYPE_NONE);
    assert(ch.pcdata->prop_dynamic_cooldown_until == 0);
    assert(save_calls == 1);
}

static void test_loads_static_propositions_with_messages_from_files(void)
{
    const char *title;
    const char *accept_message;
    const char *completion_message;
    int max_level;

    proposition_load_static_templates();

    assert(proposition_unit_static_count() >= 5);

    title = proposition_unit_static_title(0);
    accept_message = proposition_unit_static_accept_message(0);
    completion_message = proposition_unit_static_completion_message(0);
    max_level = proposition_unit_static_max_level(0);

    assert(title != NULL);
    assert(strcmp(title, "Route reconnaissance: Forest of Confusion") == 0);

    assert(accept_message != NULL);
    assert(strstr(accept_message, "Kiess route clerks") != NULL);

    assert(completion_message != NULL);
    assert(strstr(completion_message, "dispatchers") != NULL);
    assert(max_level == 39);
}

static void test_cartography_progress_counts_unique_rooms_in_target_area(void)
{
    PC_DATA pc;
    CHAR_DATA ch = make_char(&pc);
    AREA_DATA area;
    ROOM_INDEX_DATA room_a;
    ROOM_INDEX_DATA room_b;
    ROOM_INDEX_DATA room_other;
    BUILD_DATA_LIST node_a;
    BUILD_DATA_LIST node_b;

    memset(&area, 0, sizeof(area));
    memset(&room_a, 0, sizeof(room_a));
    memset(&room_b, 0, sizeof(room_b));
    memset(&room_other, 0, sizeof(room_other));
    memset(&node_a, 0, sizeof(node_a));
    memset(&node_b, 0, sizeof(node_b));

    area.area_num = 77;
    area.name = "Test Area";
    node_a.data = &room_a;
    node_b.data = &room_b;
    node_a.next = &node_b;
    area.first_area_room = &node_a;
    area.last_area_room = &node_b;
    first_area = &area;

    room_a.vnum = 100;
    room_b.vnum = 101;
    room_other.vnum = 202;
    room_a.area = &area;
    room_b.area = &area;
    room_other.area = NULL;

    ch.pcdata->propositions[0].prop_type = PROP_TYPE_CARTOGRAPHY;
    ch.pcdata->propositions[0].prop_cart_area_num = 77;
    ch.pcdata->propositions[0].prop_kill_needed = 2;
    ch.pcdata->propositions[0].prop_kill_count = 0;

    reset_counters();
    proposition_room_notify(&ch, &room_a);
    proposition_room_notify(&ch, &room_a);
    proposition_room_notify(&ch, &room_other);

    assert(ch.pcdata->propositions[0].prop_kill_count == 1);
    assert(ch.pcdata->propositions[0].prop_completed == FALSE);
    assert(save_calls == 1);

    proposition_room_notify(&ch, &room_b);
    assert(ch.pcdata->propositions[0].prop_kill_count == 2);
    assert(ch.pcdata->propositions[0].prop_completed == TRUE);
    assert(save_calls == 2);
}

int main(void)
{
    test_extracts_and_saves_when_target_matches();
    test_no_extract_when_not_a_target();
    test_marks_completed_when_final_item_collected();
    test_collect_progress_works_for_nonzero_slot();
    test_kill_progress_works_for_nonzero_slot();
    test_kill_notify_ignores_non_matching_target();
    test_clear_proposition_resets_all_slots();
    test_cancel_dynamic_sets_cooldown_and_clears_slot();
    test_cancel_static_does_not_set_cooldown();
    test_loads_static_propositions_with_messages_from_files();
    test_cartography_progress_counts_unique_rooms_in_target_area();

    puts("test_proposition: all tests passed");
    return 0;
}
