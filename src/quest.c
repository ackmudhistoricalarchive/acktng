/***************************************************************************
 * quest.c  --  Personal quest quest system for ACK! MUD
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "globals.h"

void set_obj_stat_auto(OBJ_DATA *obj);

typedef struct static_quest_template_data STATIC_PROP_TEMPLATE;
struct static_quest_template_data
{
    int id;
    char *title;
    int prerequisite_static_id;
    int type;
    int num_targets;
    int target_vnum[QUEST_MAX_TARGETS];
    int kill_needed;
    int min_level;
    int max_level;
    int offerer_vnum;
    int reward_gold;
    int reward_qp;
    char *reward_obj_short;
    char *reward_obj_name;
    char *reward_obj_long;
    int reward_obj_wear_flags;
    int reward_obj_extra_flags;
    int reward_obj_weight;
    int reward_obj_item_apply;
    char *accept_message;
    char *completion_message;
};

#define QUEST_STATIC_DIR "../quests"

static STATIC_PROP_TEMPLATE *static_quest_table = NULL;
static int static_quest_count = 0;

static bool read_prop_line(FILE *fp, char *buf, size_t size)
{
    while (fgets(buf, (int)size, fp) != NULL)
    {
        char *p;
        size_t len;

        while (*buf != '\0' && isspace(*buf))
            memmove(buf, buf + 1, strlen(buf));
        if (buf[0] == '\0' || buf[0] == '\n' || buf[0] == '#')
            continue;

        len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            buf[--len] = '\0';

        p = buf + strlen(buf);
        while (p > buf && isspace(*(p - 1)))
            *(--p) = '\0';

        if (buf[0] == '\0')
            continue;

        return TRUE;
    }

    return FALSE;
}

static bool load_static_quest_file(const char *path, int id)
{
    FILE *fp;
    char line[MAX_STRING_LENGTH];
    STATIC_PROP_TEMPLATE tpl;
    STATIC_PROP_TEMPLATE *grown;
    int i;
    char *token;

    fp = fopen(path, "r");
    if (fp == NULL)
        return FALSE;

    memset(&tpl, 0, sizeof(tpl));
    tpl.id = id;
    tpl.prerequisite_static_id = -1;
    tpl.reward_obj_short = str_dup("");
    tpl.reward_obj_name = str_dup("");
    tpl.reward_obj_long = str_dup("");
    tpl.reward_obj_wear_flags = 0;
    tpl.reward_obj_extra_flags = 0;
    tpl.reward_obj_weight = 0;
    tpl.reward_obj_item_apply = 0;
    tpl.accept_message = str_dup("");
    tpl.completion_message = str_dup("");

    if (!read_prop_line(fp, line, sizeof(line)))
    {
        bugf("load_static_quest_file: missing title in %s", path);
        free_string(tpl.reward_obj_short);
        free_string(tpl.reward_obj_name);
        free_string(tpl.reward_obj_long);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }
    tpl.title = str_dup(line);

    if (!read_prop_line(fp, line, sizeof(line)))
    {
        bugf("load_static_quest_file: missing numeric line in %s", path);
        free_string(tpl.title);
        free_string(tpl.reward_obj_short);
        free_string(tpl.reward_obj_name);
        free_string(tpl.reward_obj_long);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }

    tpl.max_level = 170;
    if (sscanf(line, "%d %d %d %d %d %d %d %d %d %*d %*d",
               &tpl.prerequisite_static_id,
               &tpl.type,
               &tpl.num_targets,
               &tpl.kill_needed,
               &tpl.min_level,
               &tpl.max_level,
               &tpl.offerer_vnum,
               &tpl.reward_gold,
               &tpl.reward_qp) != 9)
    {
        if (sscanf(line, "%d %d %d %d %d %d %d %d %*d %*d",
                   &tpl.prerequisite_static_id,
                   &tpl.type,
                   &tpl.num_targets,
                   &tpl.kill_needed,
                   &tpl.min_level,
                   &tpl.offerer_vnum,
                   &tpl.reward_gold,
                   &tpl.reward_qp) != 8)
        {
            bugf("load_static_quest_file: bad numeric line in %s", path);
            free_string(tpl.title);
            free_string(tpl.reward_obj_short);
            free_string(tpl.reward_obj_name);
            free_string(tpl.reward_obj_long);
            free_string(tpl.accept_message);
            free_string(tpl.completion_message);
            fclose(fp);
            return FALSE;
        }
    }

    if (!read_prop_line(fp, line, sizeof(line)))
    {
        bugf("load_static_quest_file: missing target line in %s", path);
        free_string(tpl.title);
        free_string(tpl.reward_obj_short);
        free_string(tpl.reward_obj_name);
        free_string(tpl.reward_obj_long);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }

    token = strtok(line, " \t");
    for (i = 0; i < QUEST_MAX_TARGETS && token != NULL; i++)
    {
        tpl.target_vnum[i] = atoi(token);
        token = strtok(NULL, " \t");
    }

    if (tpl.num_targets < 0 || tpl.num_targets > QUEST_MAX_TARGETS)
    {
        bugf("load_static_quest_file: invalid target count in %s", path);
        free_string(tpl.title);
        free_string(tpl.reward_obj_short);
        free_string(tpl.reward_obj_name);
        free_string(tpl.reward_obj_long);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }

    if (read_prop_line(fp, line, sizeof(line)))
    {
        free_string(tpl.accept_message);
        tpl.accept_message = str_dup(line);
    }

    if (read_prop_line(fp, line, sizeof(line)))
    {
        free_string(tpl.completion_message);
        tpl.completion_message = str_dup(line);
    }

    if (read_prop_line(fp, line, sizeof(line)))
    {
        free_string(tpl.reward_obj_short);
        tpl.reward_obj_short = str_dup(line);
    }

    if (read_prop_line(fp, line, sizeof(line)))
    {
        free_string(tpl.reward_obj_name);
        tpl.reward_obj_name = str_dup(line);
    }

    if (read_prop_line(fp, line, sizeof(line)))
    {
        free_string(tpl.reward_obj_long);
        tpl.reward_obj_long = str_dup(line);
    }

    if (read_prop_line(fp, line, sizeof(line)))
        tpl.reward_obj_wear_flags = atoi(line);

    if (read_prop_line(fp, line, sizeof(line)))
        tpl.reward_obj_extra_flags = atoi(line);

    if (read_prop_line(fp, line, sizeof(line)))
        tpl.reward_obj_weight = atoi(line);

    if (read_prop_line(fp, line, sizeof(line)))
        tpl.reward_obj_item_apply = atoi(line);

    grown = realloc(static_quest_table, sizeof(*grown) * (static_quest_count + 1));
    if (grown == NULL)
    {
        bug("load_static_quest_file: out of memory growing static quest table", 0);
        free_string(tpl.title);
        free_string(tpl.reward_obj_short);
        free_string(tpl.reward_obj_name);
        free_string(tpl.reward_obj_long);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }

    static_quest_table = grown;
    static_quest_table[static_quest_count++] = tpl;
    fclose(fp);
    return TRUE;
}

void quest_load_static_templates(void)
{
    char path[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int i;

    if (static_quest_table != NULL)
    {
        for (i = 0; i < static_quest_count; i++)
        {
            free_string(static_quest_table[i].title);
            free_string(static_quest_table[i].reward_obj_short);
            free_string(static_quest_table[i].reward_obj_name);
            free_string(static_quest_table[i].reward_obj_long);
            free_string(static_quest_table[i].accept_message);
            free_string(static_quest_table[i].completion_message);
        }
        free(static_quest_table);
        static_quest_table = NULL;
        static_quest_count = 0;
    }

    for (i = 1; i <= QUEST_MAX_STATIC_QUESTS; i++)
    {
        snprintf(path, sizeof(path), "%s/%d.prop", QUEST_STATIC_DIR, i);
        load_static_quest_file(path, i - 1);
    }

    snprintf(buf, sizeof(buf), "Loaded %d static quest template%s from %s.",
             static_quest_count,
             static_quest_count == 1 ? "" : "s",
             QUEST_STATIC_DIR);
    log_string(buf);
}

static const STATIC_PROP_TEMPLATE *find_static_quest_template(int static_id)
{
    int i;

    for (i = 0; i < static_quest_count; i++)
        if (static_quest_table[i].id == static_id)
            return &static_quest_table[i];

    return NULL;
}

static int canonical_postmaster_vnum(int vnum)
{
    switch (vnum)
    {
    case 13021:
        return 13001; /* legacy Kiess postmaster vnum */
    case 14021:
    case 0:
        return 14001; /* legacy/placeholder Kowloon postmaster vnum */
    default:
        return vnum;
    }
}

static CHAR_DATA *find_visible_npc_by_canonical_vnum(CHAR_DATA *ch, int vnum)
{
    CHAR_DATA *wch;

    if (ch == NULL || ch->in_room == NULL || vnum <= 0)
        return NULL;

    for (wch = ch->in_room->first_person; wch != NULL; wch = wch->next_in_room)
    {
        if (!IS_NPC(wch) || wch->pIndexData == NULL)
            continue;
        if (!can_see(ch, wch))
            continue;
        if (canonical_postmaster_vnum(wch->pIndexData->vnum) == canonical_postmaster_vnum(vnum))
            return wch;
    }

    return NULL;
}


#ifdef UNIT_TEST_QUEST
int quest_unit_static_count(void)
{
    return static_quest_count;
}

const char *quest_unit_static_title(int static_id)
{
    const STATIC_PROP_TEMPLATE *tpl = find_static_quest_template(static_id);
    return tpl != NULL ? tpl->title : NULL;
}

const char *quest_unit_static_accept_message(int static_id)
{
    const STATIC_PROP_TEMPLATE *tpl = find_static_quest_template(static_id);
    return tpl != NULL ? tpl->accept_message : NULL;
}

const char *quest_unit_static_completion_message(int static_id)
{
    const STATIC_PROP_TEMPLATE *tpl = find_static_quest_template(static_id);
    return tpl != NULL ? tpl->completion_message : NULL;
}

int quest_unit_static_max_level(int static_id)
{
    const STATIC_PROP_TEMPLATE *tpl = find_static_quest_template(static_id);
    return tpl != NULL ? tpl->max_level : -1;
}

int quest_unit_canonical_postmaster_vnum(int vnum)
{
    return canonical_postmaster_vnum(vnum);
}
#endif

static bool static_quest_prerequisite_met(CHAR_DATA *ch, const STATIC_PROP_TEMPLATE *tpl);
static bool static_reward_item_is_valid(const STATIC_PROP_TEMPLATE *tpl);
static bool quest_cartography_record_room_visit(CHAR_DATA *ch, QUEST_DATA *prop, ROOM_INDEX_DATA *room, int slot);

static QUEST_DATA *get_prop_slot(CHAR_DATA *ch, int slot)
{
    if (IS_NPC(ch) || ch->pcdata == NULL)
        return NULL;
    if (slot < 0 || slot >= QUEST_MAX_QUESTS)
        return NULL;
    return &ch->pcdata->quests[slot];
}

static int find_free_prop_slot(CHAR_DATA *ch)
{
    int i;
    for (i = 0; i < QUEST_MAX_QUESTS; i++)
    {
        QUEST_DATA *prop = get_prop_slot(ch, i);
        if (prop != NULL && prop->quest_type == QUEST_TYPE_NONE)
            return i;
    }
    return -1;
}

static bool quest_active(CHAR_DATA *ch)
{
    int i;

    if (IS_NPC(ch) || ch->pcdata == NULL)
        return FALSE;

    for (i = 0; i < QUEST_MAX_QUESTS; i++)
        if (ch->pcdata->quests[i].quest_type != QUEST_TYPE_NONE)
            return TRUE;

    return FALSE;
}

static int quest_gold(int psuedo_level)
{
    int lvl = UMAX(1, UMIN(150, psuedo_level));
    return 10 + (990 * (lvl - 1) / 149);
}

static int quest_qp(int psuedo_level)
{
    int lvl = UMAX(1, UMIN(150, psuedo_level));
    return 1 + (19 * (lvl - 1) / 149);
}

static int quest_exp(CHAR_DATA *ch)
{
    int lvl;
    int exp;

    if (ch == NULL)
        return 0;

    /*
     * Match crusade kill EXP behavior more closely:
     * - use mob_base for the effective pseudo-level
     * - apply the 150-250% band at its midpoint (200%) for deterministic rewards
     * - apply adept reduction and happy hour bonus
     */
    lvl = UMAX(1, UMIN(MAX_MOB_LEVEL - 1, get_psuedo_level(ch)));
    exp = (int)((exp_table[lvl].mob_base * 200L) / 100L);

    if (is_adept(ch))
        exp /= 1000;

    exp = UMIN(exp, 5000000);

    if (happy_hour)
        exp *= 2;

    return UMAX(1, exp);
}

static MOB_INDEX_DATA *find_prop_mob_index(int min_level, int max_level)
{
    CHAR_DATA *wch;
    int skip;
    int tries = 0;

    if (max_level > 170)
        max_level = 170;
    skip = number_range(1, 500);

    for (wch = first_char; wch != NULL; wch = wch->next)
    {
        if (!IS_NPC(wch))
            continue;
        if (wch->level < min_level || wch->level > max_level)
            continue;
        if (IS_SET(wch->act, ACT_SENTINEL))
            continue;
        if (IS_SET(wch->act, ACT_PET))
            continue;
        if (IS_SET(wch->act, ACT_POSTMAN))
            continue;
        if (IS_SET(wch->act, ACT_TRAIN))
            continue;
        if (IS_SET(wch->act, ACT_PRACTICE))
            continue;
        if (IS_SET(wch->act, ACT_HEAL))
            continue;
        if (wch->in_room != NULL &&
            IS_SET(wch->in_room->area->flags, AREA_NOSHOW))
            continue;

        if (--skip <= 0 && number_percent() < 15)
            return wch->pIndexData;

        if (++tries > 2000)
            break;
    }

    for (wch = first_char; wch != NULL; wch = wch->next)
    {
        if (!IS_NPC(wch))
            continue;
        if (wch->level < min_level || wch->level > max_level)
            continue;
        if (IS_SET(wch->act, ACT_PET))
            continue;
        if (IS_SET(wch->act, ACT_POSTMAN))
            continue;
        if (number_percent() < 5)
            return wch->pIndexData;
    }

    return NULL;
}

static OBJ_INDEX_DATA *find_prop_obj_index(int min_level, int max_level)
{
    extern int top_obj_index;
    OBJ_INDEX_DATA *pObj;
    int vnum;
    int tries = 0;
    int skip;

    if (max_level > 160)
        max_level = 160;
    if (min_level < 1)
        min_level = 1;

    skip = number_range(1, 300);

    for (vnum = 0; tries < top_obj_index * 2; vnum++, tries++)
    {
        if (vnum > 65535)
            vnum = 0;

        pObj = get_obj_index(vnum);
        if (pObj == NULL)
            continue;
        if (pObj->level < min_level || pObj->level > max_level)
            continue;
        if (pObj->item_type == ITEM_CORPSE_NPC)
            continue;
        if (pObj->item_type == ITEM_CORPSE_PC)
            continue;
        if (pObj->item_type == ITEM_MONEY)
            continue;
        if (pObj->item_type == ITEM_TRASH)
            continue;
        if (IS_SET(pObj->extra_flags, ITEM_INVENTORY))
            continue;
        if (pObj->vnum >= OBJ_VNUM_QUEST_MIN &&
            pObj->vnum <= OBJ_VNUM_QUEST_MAX)
            continue;

        if (--skip <= 0 && number_percent() < 20)
            return pObj;
    }

    return NULL;
}

static bool check_all_done(QUEST_DATA *prop)
{
    int i;
    for (i = 0; i < prop->quest_num_targets; i++)
        if (!prop->quest_target_done[i])
            return FALSE;
    return TRUE;
}

static int count_area_rooms(AREA_DATA *area)
{
    BUILD_DATA_LIST *node;
    int count = 0;

    if (area == NULL)
        return 0;

    for (node = area->first_area_room; node != NULL; node = node->next)
    {
        ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *)node->data;

        if (room == NULL || room->area != area)
            continue;

        count++;
    }

    return count;
}

static bool quest_cartography_record_room_visit(CHAR_DATA *ch, QUEST_DATA *prop, ROOM_INDEX_DATA *room, int slot)
{
    int i;
    char buf[MAX_STRING_LENGTH];

    if (ch == NULL || prop == NULL || room == NULL)
        return FALSE;
    if (prop->quest_type != QUEST_TYPE_CARTOGRAPHY || prop->quest_completed)
        return FALSE;
    if (prop->quest_cartography_area_vnum <= 0 || room->area == NULL)
        return FALSE;
    if (room->area->area_num != prop->quest_cartography_area_vnum)
        return FALSE;

    for (i = 0; i < prop->quest_cartography_room_count; i++)
        if (prop->quest_cartography_room_vnum[i] == room->vnum)
            return FALSE;

    if (prop->quest_cartography_room_count < QUEST_MAX_CARTOGRAPHY_ROOMS)
        prop->quest_cartography_room_vnum[prop->quest_cartography_room_count++] = room->vnum;

    if (prop->quest_cartography_seen_rooms < prop->quest_cartography_total_rooms)
        prop->quest_cartography_seen_rooms++;

    sprintf(buf, "@@GQuest slot %d progress:@@N explored room %d/%d in this area.\n\r",
            slot + 1,
            prop->quest_cartography_seen_rooms,
            prop->quest_cartography_total_rooms);
    send_to_char(buf, ch);

    if (prop->quest_cartography_total_rooms > 0 &&
        prop->quest_cartography_seen_rooms >= prop->quest_cartography_total_rooms)
    {
        prop->quest_completed = TRUE;
        send_to_char("\n\r@@G*** Quest complete! Visit any postman to claim your reward. ***@@N\n\r\n\r", ch);
    }

    do_save(ch, "");
    return TRUE;
}

static void clear_quest_slot(CHAR_DATA *ch, int slot)
{
    int i;
    QUEST_DATA *prop = get_prop_slot(ch, slot);

    if (prop == NULL)
        return;

    prop->quest_type = QUEST_TYPE_NONE;
    prop->quest_completed = FALSE;
    prop->quest_num_targets = 0;
    prop->quest_kill_needed = 0;
    prop->quest_kill_count = 0;
    prop->quest_static_id = -1;
    prop->quest_reward_gold = 0;
    prop->quest_reward_qp = 0;
    prop->quest_reward_item_vnum = 0;
    prop->quest_reward_item_count = 0;
    prop->quest_static_offerer_vnum = 0;
    prop->quest_cartography_total_rooms = 0;
    prop->quest_cartography_seen_rooms = 0;
    prop->quest_cartography_area_vnum = 0;
    prop->quest_cartography_room_count = 0;

    for (i = 0; i < QUEST_MAX_TARGETS; i++)
    {
        prop->quest_target_vnum[i] = 0;
        prop->quest_target_done[i] = FALSE;
    }

    for (i = 0; i < QUEST_MAX_CARTOGRAPHY_ROOMS; i++)
        prop->quest_cartography_room_vnum[i] = 0;
}

void clear_quest(CHAR_DATA *ch)
{
    int i;

    if (IS_NPC(ch) || ch->pcdata == NULL)
        return;

    for (i = 0; i < QUEST_MAX_QUESTS; i++)
        clear_quest_slot(ch, i);
}

static void show_reward_preview(CHAR_DATA *ch, QUEST_DATA *prop)
{
    char buf[MAX_STRING_LENGTH];
    int gold = prop->quest_static_id >= 0 ? prop->quest_reward_gold : quest_gold(get_psuedo_level(ch));
    int qp = prop->quest_static_id >= 0 ? prop->quest_reward_qp : quest_qp(get_psuedo_level(ch));
    int exp = quest_exp(ch);

    sprintf(buf, "@@WReward on completion:@@N @@Y%d@@N gold, @@Y%d@@N exp, @@Y%d@@N quest point%s",
            gold, exp, qp, qp == 1 ? "" : "s");
    if (prop->quest_static_id >= 0)
    {
        const STATIC_PROP_TEMPLATE *tpl = find_static_quest_template(prop->quest_static_id);
        if (static_reward_item_is_valid(tpl))
            sprintf(buf + strlen(buf), ", @@Y1@@N x %s", tpl->reward_obj_short);
    }
    strcat(buf, ".\n\r");
    send_to_char(buf, ch);
}

static bool static_reward_item_is_valid(const STATIC_PROP_TEMPLATE *tpl)
{
    if (tpl == NULL)
        return FALSE;

    if (tpl->reward_obj_short == NULL || tpl->reward_obj_short[0] == '\0'
        || tpl->reward_obj_name == NULL || tpl->reward_obj_name[0] == '\0'
        || tpl->reward_obj_long == NULL || tpl->reward_obj_long[0] == '\0')
        return FALSE;

    if (tpl->reward_obj_wear_flags == 0)
        return FALSE;

    if (tpl->reward_obj_weight <= 0)
        return FALSE;

    return TRUE;
}

static OBJ_DATA *create_static_reward_object(CHAR_DATA *ch, const STATIC_PROP_TEMPLATE *tpl)
{
    OBJ_DATA *reward;
    int spawn_level;

    if (ch == NULL || !static_reward_item_is_valid(tpl))
        return NULL;

    spawn_level = get_psuedo_level(ch);
    if (tpl->max_level > 0 && spawn_level > tpl->max_level)
        spawn_level = tpl->max_level;
    spawn_level = UMAX(1, spawn_level);

    reward = create_object(get_obj_index(OBJ_VNUM_MUSHROOM), 0);
    if (reward == NULL)
        return NULL;

    reward->item_type = ITEM_ARMOR;
    reward->level = spawn_level;
    reward->wear_flags = ITEM_TAKE | tpl->reward_obj_wear_flags;
    reward->extra_flags = tpl->reward_obj_extra_flags;
    reward->weight = tpl->reward_obj_weight;
    reward->item_apply = tpl->reward_obj_item_apply;

    free_string(reward->short_descr);
    reward->short_descr = str_dup(tpl->reward_obj_short);

    free_string(reward->name);
    reward->name = str_dup(tpl->reward_obj_name);

    free_string(reward->description);
    reward->description = str_dup(tpl->reward_obj_long);

    set_obj_stat_auto(reward);
    return reward;
}

static void quest_list_static(CHAR_DATA *ch)
{
    int i;
    int ps_lvl;
    char buf[MAX_STRING_LENGTH];

    ps_lvl = get_psuedo_level(ch);
    send_to_char("@@Y=== Available Static Quests ===@@N\n\r", ch);

    for (i = 0; i < static_quest_count; i++)
    {
        const STATIC_PROP_TEMPLATE *tpl = &static_quest_table[i];
        if (tpl->id >= 0 && tpl->id < QUEST_MAX_STATIC_QUESTS &&
            ch->pcdata->completed_static_quests[tpl->id])
            continue;
        if (find_visible_npc_by_canonical_vnum(ch, tpl->offerer_vnum) == NULL)
            continue;
        if (!static_quest_prerequisite_met(ch, tpl))
            continue;

        if (ps_lvl < tpl->min_level || (tpl->max_level > 0 && ps_lvl > tpl->max_level))
            sprintf(buf, "@@W%2d)@@N %s @@r[requires pseudo-level %d-%d]@@N\n\r", i + 1, tpl->title, tpl->min_level, tpl->max_level);
        else
            sprintf(buf, "@@W%2d)@@N %s @@g[pseudo-level %d-%d]@@N\n\r", i + 1, tpl->title, tpl->min_level, tpl->max_level);
        send_to_char(buf, ch);
    }
}

static bool static_quest_already_active(CHAR_DATA *ch, int static_id)
{
    int i;

    for (i = 0; i < QUEST_MAX_QUESTS; i++)
    {
        QUEST_DATA *prop = &ch->pcdata->quests[i];
        if (prop->quest_type != QUEST_TYPE_NONE && prop->quest_static_id == static_id)
            return TRUE;
    }

    return FALSE;
}

static bool static_quest_prerequisite_met(CHAR_DATA *ch, const STATIC_PROP_TEMPLATE *tpl)
{
    if (tpl->prerequisite_static_id < 0)
        return TRUE;

    if (tpl->prerequisite_static_id >= QUEST_MAX_STATIC_QUESTS)
        return FALSE;

    return ch->pcdata->completed_static_quests[tpl->prerequisite_static_id];
}

static void quest_accept_static(CHAR_DATA *ch, int list_number)
{
    int slot, i;
    const STATIC_PROP_TEMPLATE *tpl;
    QUEST_DATA *prop;
    char buf[MAX_STRING_LENGTH];

    if (list_number < 1 || list_number > static_quest_count)
    {
        send_to_char("That quest number is not valid.\n\r", ch);
        return;
    }

    tpl = &static_quest_table[list_number - 1];

    if (find_visible_npc_by_canonical_vnum(ch, tpl->offerer_vnum) == NULL)
    {
        send_to_char("That static quest is not being offered here.\n\r", ch);
        return;
    }

    if (tpl->id >= 0 && tpl->id < QUEST_MAX_STATIC_QUESTS &&
        ch->pcdata->completed_static_quests[tpl->id])
    {
        send_to_char("You have already completed that static quest.\n\r", ch);
        return;
    }

    if (!static_quest_prerequisite_met(ch, tpl))
    {
        send_to_char("You have not unlocked that static quest yet.\n\r", ch);
        return;
    }

    if (static_quest_already_active(ch, tpl->id))
    {
        send_to_char("You already have that static quest active.\n\r", ch);
        return;
    }

    if (get_psuedo_level(ch) < tpl->min_level)
    {
        sprintf(buf, "You must be at least pseudo-level %d for that static quest.\n\r", tpl->min_level);
        send_to_char(buf, ch);
        return;
    }

    if (tpl->max_level > 0 && get_psuedo_level(ch) > tpl->max_level)
    {
        sprintf(buf, "You must be pseudo-level %d or lower for that static quest.\n\r", tpl->max_level);
        send_to_char(buf, ch);
        return;
    }

    slot = find_free_prop_slot(ch);
    if (slot < 0)
    {
        send_to_char("You cannot carry any more quests right now.\n\r", ch);
        return;
    }

    clear_quest_slot(ch, slot);
    prop = &ch->pcdata->quests[slot];

    if (tpl->type == QUEST_TYPE_CARTOGRAPHY)
    {
        ROOM_INDEX_DATA *anchor_room;

        if (tpl->num_targets < 1)
        {
            send_to_char("That cartography quest is misconfigured (missing area anchor room).\n\r", ch);
            return;
        }

        anchor_room = get_room_index(tpl->target_vnum[0]);
        if (anchor_room == NULL || anchor_room->area == NULL)
        {
            send_to_char("That cartography quest is misconfigured (invalid area anchor room).\n\r", ch);
            return;
        }
    }

    prop->quest_type = tpl->type;
    prop->quest_static_offerer_vnum = canonical_postmaster_vnum(tpl->offerer_vnum);
    prop->quest_num_targets = tpl->num_targets;
    prop->quest_kill_needed = tpl->kill_needed;
    prop->quest_static_id = tpl->id;
    prop->quest_reward_gold = tpl->reward_gold;
    prop->quest_reward_qp = tpl->reward_qp;
    prop->quest_reward_item_vnum = 0;
    prop->quest_reward_item_count = 0;

    for (i = 0; i < tpl->num_targets; i++)
    {
        prop->quest_target_vnum[i] = tpl->target_vnum[i];
        prop->quest_target_done[i] = FALSE;
    }

    if (tpl->type == QUEST_TYPE_CARTOGRAPHY)
    {
        ROOM_INDEX_DATA *anchor_room = get_room_index(tpl->target_vnum[0]);
        AREA_DATA *area = anchor_room != NULL ? anchor_room->area : NULL;

        prop->quest_num_targets = 0;
        prop->quest_cartography_area_vnum = area != NULL ? area->area_num : 0;
        prop->quest_cartography_total_rooms = count_area_rooms(area);
        prop->quest_cartography_seen_rooms = 0;
        prop->quest_cartography_room_count = 0;

        if (prop->quest_cartography_total_rooms <= 0)
        {
            clear_quest_slot(ch, slot);
            send_to_char("That cartography quest cannot be accepted right now (empty area).\n\r", ch);
            return;
        }

        quest_cartography_record_room_visit(ch, prop, ch->in_room, slot);
    }

    sprintf(buf, "@@GYou accepted static quest [%d] in slot %d:@@N %s\n\r",
            list_number, slot + 1, tpl->title);
    send_to_char(buf, ch);
    if (tpl->accept_message != NULL && tpl->accept_message[0] != '\0')
    {
        sprintf(buf, "@@WQuest briefing:@@N %s\n\r", tpl->accept_message);
        send_to_char(buf, ch);
    }
    show_reward_preview(ch, prop);
    do_save(ch, "");
}

void quest_request(CHAR_DATA *ch, CHAR_DATA *postman)
{
    int psuedo_lvl;
    int min_lvl, max_lvl;
    int type;
    int i;
    int slot;
    QUEST_DATA *prop;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    if (ch->pcdata->quest_dynamic_cooldown_until > (int)current_time)
    {
        int wait_seconds = ch->pcdata->quest_dynamic_cooldown_until - (int)current_time;
        char wait_buf[MAX_STRING_LENGTH];
        sprintf(wait_buf, "You must wait %d more second%s before requesting a new dynamic quest.\n\r",
                wait_seconds,
                wait_seconds == 1 ? "" : "s");
        send_to_char(wait_buf, ch);
        return;
    }

    slot = find_free_prop_slot(ch);
    if (slot < 0)
    {
        send_to_char("You already have the maximum number of active quests.\n\r", ch);
        return;
    }

    psuedo_lvl = get_psuedo_level(ch);
    min_lvl = psuedo_lvl;
    max_lvl = psuedo_lvl + 15;

    type = number_range(QUEST_TYPE_KILL_VARIETY, QUEST_TYPE_KILL_COUNT);

    clear_quest_slot(ch, slot);
    prop = &ch->pcdata->quests[slot];
    prop->quest_type = type;

    switch (type)
    {
    case QUEST_TYPE_KILL_VARIETY:
    {
        int needed = number_range(3, 5);
        int found = 0;
        int loop = 0;

        while (found < needed && loop < 500)
        {
            MOB_INDEX_DATA *midx;
            bool dup = FALSE;
            int k;

            loop++;
            midx = find_prop_mob_index(min_lvl, max_lvl);
            if (midx == NULL)
                continue;

            for (k = 0; k < found; k++)
                if (prop->quest_target_vnum[k] == midx->vnum)
                {
                    dup = TRUE;
                    break;
                }
            if (dup)
                continue;

            prop->quest_target_vnum[found] = midx->vnum;
            prop->quest_target_done[found] = FALSE;
            found++;
        }

        prop->quest_num_targets = found;

        if (found == 0)
        {
            clear_quest_slot(ch, slot);
            send_to_char("The postman shrugs. 'Sorry, I can't find any work for you right now.'\n\r", ch);
            return;
        }

        send_to_char("\n\r@@GYou have received a new quest!@@N\n\r\n\r", ch);
        sprintf(buf, "Assigned to slot @@Y%d@@N.\n\r", slot + 1);
        send_to_char(buf, ch);
        send_to_char("Hunt down each of the following enemies:\n\r", ch);
        for (i = 0; i < prop->quest_num_targets; i++)
        {
            MOB_INDEX_DATA *midx = get_mob_index(prop->quest_target_vnum[i]);
            if (midx != NULL)
            {
                sprintf(buf, "  @@Y- %s@@N\n\r", midx->short_descr);
                send_to_char(buf, ch);
            }
        }
        break;
    }

    case QUEST_TYPE_COLLECT_ITEMS:
    {
        int obj_min = UMAX(1, psuedo_lvl - 10);
        int obj_max = psuedo_lvl + 10;
        int needed = number_range(2, 4);
        int found = 0;
        int loop = 0;

        while (found < needed && loop < 500)
        {
            OBJ_INDEX_DATA *oidx;
            bool dup = FALSE;
            int k;

            loop++;
            oidx = find_prop_obj_index(obj_min, obj_max);
            if (oidx == NULL)
                continue;

            for (k = 0; k < found; k++)
                if (prop->quest_target_vnum[k] == oidx->vnum)
                {
                    dup = TRUE;
                    break;
                }
            if (dup)
                continue;

            prop->quest_target_vnum[found] = oidx->vnum;
            prop->quest_target_done[found] = FALSE;
            found++;
        }

        prop->quest_num_targets = found;

        if (found == 0)
        {
            clear_quest_slot(ch, slot);
            send_to_char("The postman shrugs. 'Sorry, I can't find any work for you right now.'\n\r", ch);
            return;
        }

        send_to_char("\n\r@@GYou have received a new quest!@@N\n\r\n\r", ch);
        sprintf(buf, "Assigned to slot @@Y%d@@N.\n\r", slot + 1);
        send_to_char(buf, ch);
        send_to_char("Acquire each of the following items (credited when they enter your inventory):\n\r", ch);
        for (i = 0; i < prop->quest_num_targets; i++)
        {
            OBJ_INDEX_DATA *oidx = get_obj_index(prop->quest_target_vnum[i]);
            if (oidx != NULL)
            {
                sprintf(buf, "  @@C- %s @@N[level %d]\n\r", oidx->short_descr, oidx->level);
                send_to_char(buf, ch);
            }
        }
        break;
    }

    case QUEST_TYPE_KILL_COUNT:
    {
        MOB_INDEX_DATA *midx;
        int kill_need = number_range(5, 15);

        midx = find_prop_mob_index(psuedo_lvl + 5, psuedo_lvl + 15);
        if (midx == NULL)
        {
            clear_quest_slot(ch, slot);
            send_to_char("The postman shrugs. 'Sorry, I can't find any work for you right now.'\n\r", ch);
            return;
        }

        prop->quest_num_targets = 1;
        prop->quest_target_vnum[0] = midx->vnum;
        prop->quest_target_done[0] = FALSE;
        prop->quest_kill_needed = kill_need;
        prop->quest_kill_count = 0;

        send_to_char("\n\r@@GYou have received a new quest!@@N\n\r\n\r", ch);
        sprintf(buf, "Assigned to slot @@Y%d@@N.\n\r", slot + 1);
        send_to_char(buf, ch);
        sprintf(buf, "Slay @@Y%d@@N of @@R%s@@N.\n\r", kill_need, midx->short_descr);
        send_to_char(buf, ch);
        break;
    }
    }

    show_reward_preview(ch, prop);
    send_to_char("Return to any postman when done to claim your reward.\n\r", ch);

    act("$N hands you a sealed note outlining your quest.", ch, NULL, postman, TO_CHAR);
    act("$N hands $n a sealed quest.", ch, NULL, postman, TO_ROOM);

    do_save(ch, "");
}

void quest_status(CHAR_DATA *ch)
{
    int slot, i;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    if (!quest_active(ch))
    {
        send_to_char("You have no active quest. Visit a postman to request one.\n\r", ch);
        return;
    }

    send_to_char("@@Y===  Quest Status  ===@@N\n\r\n\r", ch);

    for (slot = 0; slot < QUEST_MAX_QUESTS; slot++)
    {
        QUEST_DATA *prop = &ch->pcdata->quests[slot];
        if (prop->quest_type == QUEST_TYPE_NONE)
            continue;

        sprintf(buf, "@@W[Slot %d]@@N %s\n\r", slot + 1,
                prop->quest_static_id >= 0 ? "(static)" : "(dynamic)");
        send_to_char(buf, ch);

        if (prop->quest_completed)
        {
            send_to_char("@@GStatus: COMPLETE@@N - turn in at a postman.\n\r\n\r", ch);
            continue;
        }

        switch (prop->quest_type)
        {
        case QUEST_TYPE_KILL_VARIETY:
            send_to_char("@@WTask:@@N Hunt down each of the following enemies:\n\r", ch);
            for (i = 0; i < prop->quest_num_targets; i++)
            {
                MOB_INDEX_DATA *midx = get_mob_index(prop->quest_target_vnum[i]);
                if (midx != NULL)
                {
                    sprintf(buf, "  %s%-30s@@N  %s\n\r",
                            prop->quest_target_done[i] ? "@@G" : "@@R",
                            midx->short_descr,
                            prop->quest_target_done[i] ? "[DONE]" : "[pending]");
                    send_to_char(buf, ch);
                }
            }
            break;

        case QUEST_TYPE_COLLECT_ITEMS:
            send_to_char("@@WTask:@@N Acquire each of the following items:\n\r", ch);
            for (i = 0; i < prop->quest_num_targets; i++)
            {
                OBJ_INDEX_DATA *oidx = get_obj_index(prop->quest_target_vnum[i]);
                if (oidx != NULL)
                {
                    sprintf(buf, "  %s%-30s@@N  [lvl %-3d]  %s\n\r",
                            prop->quest_target_done[i] ? "@@G" : "@@C",
                            oidx->short_descr,
                            oidx->level,
                            prop->quest_target_done[i] ? "[OBTAINED]" : "[needed]");
                    send_to_char(buf, ch);
                }
            }
            break;

        case QUEST_TYPE_KILL_COUNT:
        {
            MOB_INDEX_DATA *midx = get_mob_index(prop->quest_target_vnum[0]);
            sprintf(buf, "@@WTask:@@N Slay %s\n\r", midx ? midx->short_descr : "(unknown)");
            send_to_char(buf, ch);
            sprintf(buf, "  Progress: @@Y%d@@N / @@Y%d@@N\n\r",
                    prop->quest_kill_count,
                    prop->quest_kill_needed);
            send_to_char(buf, ch);
            break;
        }

        case QUEST_TYPE_CARTOGRAPHY:
            send_to_char("@@WTask:@@N Explore every room in the designated area.\n\r", ch);
            sprintf(buf, "  Progress: @@Y%d@@N / @@Y%d@@N rooms explored\n\r",
                    prop->quest_cartography_seen_rooms,
                    prop->quest_cartography_total_rooms);
            send_to_char(buf, ch);
            break;
        }

        show_reward_preview(ch, prop);
        send_to_char("\n\r", ch);
    }
}

void quest_complete(CHAR_DATA *ch, CHAR_DATA *postman)
{
    int slot;
    bool has_completed = FALSE;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    for (slot = 0; slot < QUEST_MAX_QUESTS; slot++)
    {
        QUEST_DATA *prop = &ch->pcdata->quests[slot];
            int gold_reward;
        int exp_reward;
        int qp_reward;
        const STATIC_PROP_TEMPLATE *tpl = NULL;
        CHAR_DATA *turnin_npc = NULL;

        if (prop->quest_type == QUEST_TYPE_NONE || !prop->quest_completed)
            continue;

        has_completed = TRUE;

        if (prop->quest_static_id >= 0)
        {
            int required_vnum = prop->quest_static_offerer_vnum;
            tpl = find_static_quest_template(prop->quest_static_id);
            if (required_vnum <= 0 && tpl != NULL)
                required_vnum = canonical_postmaster_vnum(tpl->offerer_vnum);
            turnin_npc = find_visible_npc_by_canonical_vnum(ch, required_vnum);
            if (turnin_npc == NULL)
                continue;
        }
        else
        {
            turnin_npc = postman;
            if (turnin_npc == NULL)
                continue;
        }

        gold_reward = prop->quest_static_id >= 0 ? prop->quest_reward_gold : quest_gold(get_psuedo_level(ch));
        exp_reward = quest_exp(ch);
        qp_reward = prop->quest_static_id >= 0 ? prop->quest_reward_qp : quest_qp(get_psuedo_level(ch));

        act("$N reviews your completed quest and nods approvingly.", ch, NULL, turnin_npc, TO_CHAR);
        act("$N reviews $n's completed quest and nods approvingly.", ch, NULL, turnin_npc, TO_ROOM);

        send_to_char("\n\r@@GQuest complete! You receive:@@N\n\r", ch);
        sprintf(buf, "  @@Y%d@@N gold coins\n\r", gold_reward);
        send_to_char(buf, ch);
        sprintf(buf, "  @@Y%d@@N experience\n\r", exp_reward);
        send_to_char(buf, ch);
        sprintf(buf, "  @@Y%d@@N quest point%s\n\r", qp_reward, qp_reward == 1 ? "" : "s");
        send_to_char(buf, ch);
        send_to_char("  @@Y1@@N quest point\n\r", ch);

        if (prop->quest_static_id >= 0)
        {
            tpl = find_static_quest_template(prop->quest_static_id);
            if (tpl != NULL && tpl->completion_message != NULL && tpl->completion_message[0] != '\0')
            {
                sprintf(buf, "@@WCompletion report:@@N %s\n\r", tpl->completion_message);
                send_to_char(buf, ch);
            }
        }

        ch->gold += gold_reward;
        gain_exp(ch, exp_reward);
        ch->quest_points += qp_reward;
        ch->pcdata->post_quest_points += 1;

        if (prop->quest_static_id >= 0)
        {
            OBJ_DATA *reward;

            reward = create_static_reward_object(ch, tpl);
            if (reward != NULL)
            {
                obj_to_char(reward, ch);
                sprintf(buf, "  @@Y1@@N x %s\n\r", reward->short_descr);
                send_to_char(buf, ch);
            }
        }

        if (prop->quest_static_id >= 0 && prop->quest_static_id < QUEST_MAX_STATIC_QUESTS)
            ch->pcdata->completed_static_quests[prop->quest_static_id] = TRUE;

        clear_quest_slot(ch, slot);
        do_save(ch, "");
        return;
    }

    if (!quest_active(ch))
        send_to_char("You have no quest to turn in.\n\r", ch);
    else if (has_completed)
        send_to_char("You cannot turn that in here yet. Dynamic quests need a postman; static quests must be turned in to their offering mob.\n\r", ch);
    else
        send_to_char("You have no completed quest to turn in yet.\n\r", ch);
}

void quest_kill_notify(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int slot;

    if (IS_NPC(ch) || !IS_NPC(victim) || ch->pcdata == NULL)
        return;

    for (slot = 0; slot < QUEST_MAX_QUESTS; slot++)
    {
        QUEST_DATA *prop = &ch->pcdata->quests[slot];
        int i;
        bool progress = FALSE;
        char buf[MAX_STRING_LENGTH];

        if (prop->quest_type == QUEST_TYPE_NONE || prop->quest_completed)
            continue;

        switch (prop->quest_type)
        {
        case QUEST_TYPE_KILL_VARIETY:
            for (i = 0; i < prop->quest_num_targets; i++)
            {
                if (!prop->quest_target_done[i] &&
                    prop->quest_target_vnum[i] == victim->pIndexData->vnum)
                {
                    prop->quest_target_done[i] = TRUE;
                    progress = TRUE;
                    sprintf(buf, "@@GQuest slot %d progress:@@N %s killed!\n\r",
                            slot + 1, victim->short_descr);
                    send_to_char(buf, ch);
                    break;
                }
            }
            if (progress && check_all_done(prop))
            {
                prop->quest_completed = TRUE;
                send_to_char("\n\r@@G*** Quest complete! Visit any postman to claim your reward. ***@@N\n\r\n\r", ch);
            }
            break;

        case QUEST_TYPE_KILL_COUNT:
            if (prop->quest_target_vnum[0] == victim->pIndexData->vnum)
            {
                prop->quest_kill_count++;
                progress = TRUE;
                sprintf(buf, "@@GQuest slot %d progress:@@N %d/%d %s slain.\n\r",
                        slot + 1,
                        prop->quest_kill_count,
                        prop->quest_kill_needed,
                        victim->short_descr);
                send_to_char(buf, ch);
                if (prop->quest_kill_count >= prop->quest_kill_needed)
                {
                    prop->quest_completed = TRUE;
                    send_to_char("\n\r@@G*** Quest complete! Visit any postman to claim your reward. ***@@N\n\r\n\r", ch);
                }
            }
            break;
        }

        if (progress)
            do_save(ch, "");
    }
}

void quest_obj_notify(CHAR_DATA *ch, OBJ_DATA *obj)
{
    int slot;

    if (IS_NPC(ch) || ch->pcdata == NULL)
        return;

    for (slot = 0; slot < QUEST_MAX_QUESTS; slot++)
    {
        QUEST_DATA *prop = &ch->pcdata->quests[slot];
        int i;
        char buf[MAX_STRING_LENGTH];

        if (prop->quest_type != QUEST_TYPE_COLLECT_ITEMS || prop->quest_completed)
            continue;

        for (i = 0; i < prop->quest_num_targets; i++)
        {
            if (!prop->quest_target_done[i] &&
                prop->quest_target_vnum[i] == obj->pIndexData->vnum)
            {
                prop->quest_target_done[i] = TRUE;
                sprintf(buf, "@@GQuest slot %d progress:@@N %s obtained!\n\r",
                        slot + 1, obj->short_descr);
                send_to_char(buf, ch);

                if (check_all_done(prop))
                {
                    prop->quest_completed = TRUE;
                    send_to_char("\n\r@@G*** Quest complete! Visit any postman to claim your reward. ***@@N\n\r\n\r", ch);
                }

                extract_obj(obj);
                do_save(ch, "");
                return;
            }
        }
    }
}


void quest_room_notify(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
    int slot;

    if (IS_NPC(ch) || ch->pcdata == NULL || room == NULL)
        return;

    for (slot = 0; slot < QUEST_MAX_QUESTS; slot++)
    {
        QUEST_DATA *prop = &ch->pcdata->quests[slot];

        if (prop->quest_type != QUEST_TYPE_CARTOGRAPHY || prop->quest_completed)
            continue;

        quest_cartography_record_room_visit(ch, prop, room, slot);
    }
}


void quest_cancel(CHAR_DATA *ch, int slot)
{
    QUEST_DATA *prop;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || ch->pcdata == NULL)
        return;

    if (slot < 0 || slot >= QUEST_MAX_QUESTS)
    {
        send_to_char("Usage: quest cancel <slot# 1-3>\n\r", ch);
        return;
    }

    prop = &ch->pcdata->quests[slot];
    if (prop->quest_type == QUEST_TYPE_NONE)
    {
        send_to_char("That quest slot is already empty.\n\r", ch);
        return;
    }

    if (prop->quest_static_id < 0)
        ch->pcdata->quest_dynamic_cooldown_until = (int)current_time + 15 * 60;

    clear_quest_slot(ch, slot);

    sprintf(buf, "You cancel quest slot @@Y%d@@N.\n\r", slot + 1);
    send_to_char(buf, ch);

    if (ch->pcdata->quest_dynamic_cooldown_until > (int)current_time)
        send_to_char("Cancelling a dynamic quest starts a 15 minute request cooldown.\n\r", ch);

    do_save(ch, "");
}

void do_quest(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *postman = NULL;
    CHAR_DATA *wch;

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, arg);
    one_argument(argument, arg2);

    if (arg[0] == '\0')
    {
        send_to_char("Usage: quest <request|status|complete|list|accept #|cancel #>\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "status"))
    {
        quest_status(ch);
        return;
    }

    if (!str_prefix(arg, "cancel"))
    {
        if (!is_number(arg2))
        {
            send_to_char("Usage: quest cancel <slot# 1-3>\n\r", ch);
            return;
        }
        quest_cancel(ch, atoi(arg2) - 1);
        return;
    }

    if (!str_prefix(arg, "request") || !str_prefix(arg, "complete") ||
        !str_prefix(arg, "list") || !str_prefix(arg, "accept"))
    {
        for (wch = ch->in_room->first_person; wch != NULL; wch = wch->next_in_room)
        {
            if (IS_NPC(wch) && IS_SET(wch->act, ACT_POSTMAN) && can_see(ch, wch))
            {
                postman = wch;
                break;
            }
        }

        if (!str_prefix(arg, "request"))
        {
            if (postman == NULL)
            {
                send_to_char("You need to be with a postman to do that.\n\r", ch);
                return;
            }
            quest_request(ch, postman);
        }
        else if (!str_prefix(arg, "complete"))
            quest_complete(ch, postman);
        else if (!str_prefix(arg, "list"))
            quest_list_static(ch);
        else
        {
            if (!is_number(arg2))
            {
                send_to_char("Usage: quest accept <number>\n\r", ch);
                return;
            }
            quest_accept_static(ch, atoi(arg2));
        }

        return;
    }

    send_to_char("Usage: quest <request|status|complete|list|accept #|cancel #>\n\r", ch);
}
