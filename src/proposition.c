/***************************************************************************
 * proposition.c  --  Personal proposition quest system for ACK! MUD
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "globals.h"

typedef struct static_prop_template_data STATIC_PROP_TEMPLATE;
struct static_prop_template_data
{
    int id;
    char *title;
    int prerequisite_static_id;
    int type;
    int num_targets;
    int target_vnum[PROP_MAX_TARGETS];
    int kill_needed;
    int min_level;
    int max_level;
    int offerer_vnum;
    int reward_gold;
    int reward_qp;
    int reward_item_vnum;
    int reward_item_count;
    char *accept_message;
    char *completion_message;
};

#define PROP_STATIC_DIR "../propositions"

static STATIC_PROP_TEMPLATE *static_prop_table = NULL;
static int static_prop_count = 0;

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

static bool load_static_prop_file(const char *path, int id)
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
    tpl.accept_message = str_dup("");
    tpl.completion_message = str_dup("");

    if (!read_prop_line(fp, line, sizeof(line)))
    {
        bugf("load_static_prop_file: missing title in %s", path);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }
    tpl.title = str_dup(line);

    if (!read_prop_line(fp, line, sizeof(line)))
    {
        bugf("load_static_prop_file: missing numeric line in %s", path);
        free_string(tpl.title);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }

    tpl.max_level = 170;
    if (sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d",
               &tpl.prerequisite_static_id,
               &tpl.type,
               &tpl.num_targets,
               &tpl.kill_needed,
               &tpl.min_level,
               &tpl.max_level,
               &tpl.offerer_vnum,
               &tpl.reward_gold,
               &tpl.reward_qp,
               &tpl.reward_item_vnum,
               &tpl.reward_item_count) != 11)
    {
        if (sscanf(line, "%d %d %d %d %d %d %d %d %d %d",
                   &tpl.prerequisite_static_id,
                   &tpl.type,
                   &tpl.num_targets,
                   &tpl.kill_needed,
                   &tpl.min_level,
                   &tpl.offerer_vnum,
                   &tpl.reward_gold,
                   &tpl.reward_qp,
                   &tpl.reward_item_vnum,
                   &tpl.reward_item_count) != 10)
        {
            bugf("load_static_prop_file: bad numeric line in %s", path);
            free_string(tpl.title);
            free_string(tpl.accept_message);
            free_string(tpl.completion_message);
            fclose(fp);
            return FALSE;
        }
    }

    if (!read_prop_line(fp, line, sizeof(line)))
    {
        bugf("load_static_prop_file: missing target line in %s", path);
        free_string(tpl.title);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }

    token = strtok(line, " \t");
    for (i = 0; i < PROP_MAX_TARGETS && token != NULL; i++)
    {
        tpl.target_vnum[i] = atoi(token);
        token = strtok(NULL, " \t");
    }

    if (tpl.num_targets < 0 || tpl.num_targets > PROP_MAX_TARGETS)
    {
        bugf("load_static_prop_file: invalid target count in %s", path);
        free_string(tpl.title);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }

    if (read_prop_line(fp, line, sizeof(line)))
        tpl.accept_message = str_dup(line);

    if (read_prop_line(fp, line, sizeof(line)))
        tpl.completion_message = str_dup(line);

    grown = realloc(static_prop_table, sizeof(*grown) * (static_prop_count + 1));
    if (grown == NULL)
    {
        bug("load_static_prop_file: out of memory growing static proposition table", 0);
        free_string(tpl.title);
        free_string(tpl.accept_message);
        free_string(tpl.completion_message);
        fclose(fp);
        return FALSE;
    }

    static_prop_table = grown;
    static_prop_table[static_prop_count++] = tpl;
    fclose(fp);
    return TRUE;
}

void proposition_load_static_templates(void)
{
    char path[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int i;

    if (static_prop_table != NULL)
    {
        for (i = 0; i < static_prop_count; i++)
        {
            free_string(static_prop_table[i].title);
            free_string(static_prop_table[i].accept_message);
            free_string(static_prop_table[i].completion_message);
        }
        free(static_prop_table);
        static_prop_table = NULL;
        static_prop_count = 0;
    }

    for (i = 1; i <= PROP_MAX_STATIC_PROPOSITIONS; i++)
    {
        snprintf(path, sizeof(path), "%s/%d.prop", PROP_STATIC_DIR, i);
        load_static_prop_file(path, i - 1);
    }

    snprintf(buf, sizeof(buf), "Loaded %d static proposition template%s from %s.",
             static_prop_count,
             static_prop_count == 1 ? "" : "s",
             PROP_STATIC_DIR);
    log_string(buf);
}

static const STATIC_PROP_TEMPLATE *find_static_prop_template(int static_id)
{
    int i;

    for (i = 0; i < static_prop_count; i++)
        if (static_prop_table[i].id == static_id)
            return &static_prop_table[i];

    return NULL;
}


#ifdef UNIT_TEST_PROPOSITION
int proposition_unit_static_count(void)
{
    return static_prop_count;
}

const char *proposition_unit_static_title(int static_id)
{
    const STATIC_PROP_TEMPLATE *tpl = find_static_prop_template(static_id);
    return tpl != NULL ? tpl->title : NULL;
}

const char *proposition_unit_static_accept_message(int static_id)
{
    const STATIC_PROP_TEMPLATE *tpl = find_static_prop_template(static_id);
    return tpl != NULL ? tpl->accept_message : NULL;
}

const char *proposition_unit_static_completion_message(int static_id)
{
    const STATIC_PROP_TEMPLATE *tpl = find_static_prop_template(static_id);
    return tpl != NULL ? tpl->completion_message : NULL;
}

int proposition_unit_static_max_level(int static_id)
{
    const STATIC_PROP_TEMPLATE *tpl = find_static_prop_template(static_id);
    return tpl != NULL ? tpl->max_level : -1;
}
#endif

static bool static_prop_prerequisite_met(CHAR_DATA *ch, const STATIC_PROP_TEMPLATE *tpl);

static AREA_DATA *find_area_by_num(int area_num)
{
    AREA_DATA *area;

    for (area = first_area; area != NULL; area = area->next)
        if (area->area_num == area_num)
            return area;

    return NULL;
}

static int area_room_count(AREA_DATA *area)
{
    BUILD_DATA_LIST *node;
    int count = 0;

    if (area == NULL)
        return 0;

    for (node = area->first_area_room; node != NULL; node = node->next)
        if (node->data != NULL)
            count++;

    return count;
}

static bool prop_cart_room_seen(PROPOSITION_DATA *prop, int room_vnum)
{
    int byte;
    int bit;

    if (room_vnum < 0 || room_vnum > PROP_CART_ROOM_MAX_VNUM)
        return TRUE;

    byte = room_vnum / 8;
    bit = room_vnum % 8;
    return (prop->prop_cart_room_seen[byte] & (1u << bit)) != 0;
}

static void prop_cart_set_room_seen(PROPOSITION_DATA *prop, int room_vnum)
{
    int byte;
    int bit;

    if (room_vnum < 0 || room_vnum > PROP_CART_ROOM_MAX_VNUM)
        return;

    byte = room_vnum / 8;
    bit = room_vnum % 8;
    prop->prop_cart_room_seen[byte] |= (1u << bit);
}

static PROPOSITION_DATA *get_prop_slot(CHAR_DATA *ch, int slot)
{
    if (IS_NPC(ch) || ch->pcdata == NULL)
        return NULL;
    if (slot < 0 || slot >= PROP_MAX_PROPOSITIONS)
        return NULL;
    return &ch->pcdata->propositions[slot];
}

static int find_free_prop_slot(CHAR_DATA *ch)
{
    int i;
    for (i = 0; i < PROP_MAX_PROPOSITIONS; i++)
    {
        PROPOSITION_DATA *prop = get_prop_slot(ch, i);
        if (prop != NULL && prop->prop_type == PROP_TYPE_NONE)
            return i;
    }
    return -1;
}

static bool proposition_active(CHAR_DATA *ch)
{
    int i;

    if (IS_NPC(ch) || ch->pcdata == NULL)
        return FALSE;

    for (i = 0; i < PROP_MAX_PROPOSITIONS; i++)
        if (ch->pcdata->propositions[i].prop_type != PROP_TYPE_NONE)
            return TRUE;

    return FALSE;
}

static int proposition_gold(int psuedo_level)
{
    int lvl = UMAX(1, UMIN(150, psuedo_level));
    return 10 + (990 * (lvl - 1) / 149);
}

static int proposition_qp(int psuedo_level)
{
    int lvl = UMAX(1, UMIN(150, psuedo_level));
    return 1 + (19 * (lvl - 1) / 149);
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

static bool check_all_done(PROPOSITION_DATA *prop)
{
    int i;
    for (i = 0; i < prop->prop_num_targets; i++)
        if (!prop->prop_target_done[i])
            return FALSE;
    return TRUE;
}

static void clear_proposition_slot(CHAR_DATA *ch, int slot)
{
    int i;
    PROPOSITION_DATA *prop = get_prop_slot(ch, slot);

    if (prop == NULL)
        return;

    prop->prop_type = PROP_TYPE_NONE;
    prop->prop_completed = FALSE;
    prop->prop_num_targets = 0;
    prop->prop_kill_needed = 0;
    prop->prop_kill_count = 0;
    prop->prop_static_id = -1;
    prop->prop_reward_gold = 0;
    prop->prop_reward_qp = 0;
    prop->prop_reward_item_vnum = 0;
    prop->prop_reward_item_count = 0;
    prop->prop_cart_area_num = 0;
    memset(prop->prop_cart_room_seen, 0, sizeof(prop->prop_cart_room_seen));

    for (i = 0; i < PROP_MAX_TARGETS; i++)
    {
        prop->prop_target_vnum[i] = 0;
        prop->prop_target_done[i] = FALSE;
    }
}

void clear_proposition(CHAR_DATA *ch)
{
    int i;

    if (IS_NPC(ch) || ch->pcdata == NULL)
        return;

    for (i = 0; i < PROP_MAX_PROPOSITIONS; i++)
        clear_proposition_slot(ch, i);
}

static void show_reward_preview(CHAR_DATA *ch, PROPOSITION_DATA *prop)
{
    char buf[MAX_STRING_LENGTH];
    int gold = prop->prop_static_id >= 0 ? prop->prop_reward_gold : proposition_gold(get_psuedo_level(ch));
    int qp = prop->prop_static_id >= 0 ? prop->prop_reward_qp : proposition_qp(get_psuedo_level(ch));

    sprintf(buf, "@@WReward on completion:@@N @@Y%d@@N gold, @@Y%d@@N quest point%s",
            gold, qp, qp == 1 ? "" : "s");
    if (prop->prop_reward_item_vnum > 0 && prop->prop_reward_item_count > 0)
    {
        OBJ_INDEX_DATA *oidx = get_obj_index(prop->prop_reward_item_vnum);
        if (oidx != NULL)
            sprintf(buf + strlen(buf), ", @@Y%d@@N x %s",
                    prop->prop_reward_item_count,
                    oidx->short_descr);
    }
    strcat(buf, ".\n\r");
    send_to_char(buf, ch);
}

static void proposition_list_static(CHAR_DATA *ch, CHAR_DATA *postman)
{
    int i;
    int ps_lvl;
    char buf[MAX_STRING_LENGTH];

    ps_lvl = get_psuedo_level(ch);
    send_to_char("@@Y=== Available Static Propositions ===@@N\n\r", ch);

    for (i = 0; i < static_prop_count; i++)
    {
        const STATIC_PROP_TEMPLATE *tpl = &static_prop_table[i];
        if (tpl->id >= 0 && tpl->id < PROP_MAX_STATIC_PROPOSITIONS &&
            ch->pcdata->completed_static_props[tpl->id])
            continue;
        if (postman != NULL && postman->pIndexData != NULL &&
            tpl->offerer_vnum != postman->pIndexData->vnum)
            continue;
        if (!static_prop_prerequisite_met(ch, tpl))
            continue;

        if (ps_lvl < tpl->min_level || (tpl->max_level > 0 && ps_lvl > tpl->max_level))
            sprintf(buf, "@@W%2d)@@N %s @@r[requires pseudo-level %d-%d]@@N\n\r", i + 1, tpl->title, tpl->min_level, tpl->max_level);
        else
            sprintf(buf, "@@W%2d)@@N %s @@g[pseudo-level %d-%d]@@N\n\r", i + 1, tpl->title, tpl->min_level, tpl->max_level);
        send_to_char(buf, ch);
    }
}

static bool static_prop_already_active(CHAR_DATA *ch, int static_id)
{
    int i;

    for (i = 0; i < PROP_MAX_PROPOSITIONS; i++)
    {
        PROPOSITION_DATA *prop = &ch->pcdata->propositions[i];
        if (prop->prop_type != PROP_TYPE_NONE && prop->prop_static_id == static_id)
            return TRUE;
    }

    return FALSE;
}

static bool static_prop_prerequisite_met(CHAR_DATA *ch, const STATIC_PROP_TEMPLATE *tpl)
{
    if (tpl->prerequisite_static_id < 0)
        return TRUE;

    if (tpl->prerequisite_static_id >= PROP_MAX_STATIC_PROPOSITIONS)
        return FALSE;

    return ch->pcdata->completed_static_props[tpl->prerequisite_static_id];
}

static void proposition_accept_static(CHAR_DATA *ch, CHAR_DATA *postman, int list_number)
{
    int slot, i;
    const STATIC_PROP_TEMPLATE *tpl;
    PROPOSITION_DATA *prop;
    char buf[MAX_STRING_LENGTH];

    if (list_number < 1 || list_number > static_prop_count)
    {
        send_to_char("That proposition number is not valid.\n\r", ch);
        return;
    }

    tpl = &static_prop_table[list_number - 1];

    if (postman == NULL || postman->pIndexData == NULL ||
        tpl->offerer_vnum != postman->pIndexData->vnum)
    {
        send_to_char("That postmaster does not offer that static proposition.\n\r", ch);
        return;
    }

    if (tpl->id >= 0 && tpl->id < PROP_MAX_STATIC_PROPOSITIONS &&
        ch->pcdata->completed_static_props[tpl->id])
    {
        send_to_char("You have already completed that static proposition.\n\r", ch);
        return;
    }

    if (!static_prop_prerequisite_met(ch, tpl))
    {
        send_to_char("You have not unlocked that static proposition yet.\n\r", ch);
        return;
    }

    if (static_prop_already_active(ch, tpl->id))
    {
        send_to_char("You already have that static proposition active.\n\r", ch);
        return;
    }

    if (get_psuedo_level(ch) < tpl->min_level)
    {
        sprintf(buf, "You must be at least pseudo-level %d for that static proposition.\n\r", tpl->min_level);
        send_to_char(buf, ch);
        return;
    }

    if (tpl->max_level > 0 && get_psuedo_level(ch) > tpl->max_level)
    {
        sprintf(buf, "You must be pseudo-level %d or lower for that static proposition.\n\r", tpl->max_level);
        send_to_char(buf, ch);
        return;
    }

    slot = find_free_prop_slot(ch);
    if (slot < 0)
    {
        send_to_char("You cannot carry any more propositions right now.\n\r", ch);
        return;
    }

    clear_proposition_slot(ch, slot);
    prop = &ch->pcdata->propositions[slot];
    prop->prop_type = tpl->type;
    prop->prop_num_targets = tpl->num_targets;
    prop->prop_kill_needed = tpl->kill_needed;
    prop->prop_static_id = tpl->id;
    prop->prop_reward_gold = tpl->reward_gold;
    prop->prop_reward_qp = tpl->reward_qp;
    prop->prop_reward_item_vnum = tpl->reward_item_vnum;
    prop->prop_reward_item_count = tpl->reward_item_count;

    if (tpl->type == PROP_TYPE_CARTOGRAPHY)
    {
        AREA_DATA *area;
        int total_rooms;

        if (tpl->num_targets < 1)
        {
            send_to_char("That static proposition is misconfigured (missing area target).\n\r", ch);
            clear_proposition_slot(ch, slot);
            return;
        }

        area = find_area_by_num(tpl->target_vnum[0]);
        total_rooms = area_room_count(area);
        if (area == NULL || total_rooms <= 0)
        {
            send_to_char("That static proposition is misconfigured (invalid or empty area target).\n\r", ch);
            clear_proposition_slot(ch, slot);
            return;
        }

        prop->prop_num_targets = 1;
        prop->prop_target_vnum[0] = tpl->target_vnum[0];
        prop->prop_target_done[0] = FALSE;
        prop->prop_cart_area_num = tpl->target_vnum[0];
        prop->prop_kill_needed = total_rooms;
        prop->prop_kill_count = 0;
    }

    for (i = 0; i < tpl->num_targets; i++)
    {
        prop->prop_target_vnum[i] = tpl->target_vnum[i];
        prop->prop_target_done[i] = FALSE;
    }

    sprintf(buf, "@@GYou accepted static proposition [%d] in slot %d:@@N %s\n\r",
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

void proposition_request(CHAR_DATA *ch, CHAR_DATA *postman)
{
    int psuedo_lvl;
    int min_lvl, max_lvl;
    int type;
    int i;
    int slot;
    PROPOSITION_DATA *prop;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    if (ch->pcdata->prop_dynamic_cooldown_until > (int)current_time)
    {
        int wait_seconds = ch->pcdata->prop_dynamic_cooldown_until - (int)current_time;
        char wait_buf[MAX_STRING_LENGTH];
        sprintf(wait_buf, "You must wait %d more second%s before requesting a new dynamic proposition.\n\r",
                wait_seconds,
                wait_seconds == 1 ? "" : "s");
        send_to_char(wait_buf, ch);
        return;
    }

    slot = find_free_prop_slot(ch);
    if (slot < 0)
    {
        send_to_char("You already have the maximum number of active propositions.\n\r", ch);
        return;
    }

    psuedo_lvl = get_psuedo_level(ch);
    min_lvl = psuedo_lvl;
    max_lvl = psuedo_lvl + 15;

    type = number_range(PROP_TYPE_KILL_VARIETY, PROP_TYPE_KILL_COUNT);

    clear_proposition_slot(ch, slot);
    prop = &ch->pcdata->propositions[slot];
    prop->prop_type = type;

    switch (type)
    {
    case PROP_TYPE_KILL_VARIETY:
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
                if (prop->prop_target_vnum[k] == midx->vnum)
                {
                    dup = TRUE;
                    break;
                }
            if (dup)
                continue;

            prop->prop_target_vnum[found] = midx->vnum;
            prop->prop_target_done[found] = FALSE;
            found++;
        }

        prop->prop_num_targets = found;

        if (found == 0)
        {
            clear_proposition_slot(ch, slot);
            send_to_char("The postman shrugs. 'Sorry, I can't find any work for you right now.'\n\r", ch);
            return;
        }

        send_to_char("\n\r@@GYou have received a new proposition!@@N\n\r\n\r", ch);
        sprintf(buf, "Assigned to slot @@Y%d@@N.\n\r", slot + 1);
        send_to_char(buf, ch);
        send_to_char("Hunt down each of the following enemies:\n\r", ch);
        for (i = 0; i < prop->prop_num_targets; i++)
        {
            MOB_INDEX_DATA *midx = get_mob_index(prop->prop_target_vnum[i]);
            if (midx != NULL)
            {
                sprintf(buf, "  @@Y- %s@@N\n\r", midx->short_descr);
                send_to_char(buf, ch);
            }
        }
        break;
    }

    case PROP_TYPE_COLLECT_ITEMS:
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
                if (prop->prop_target_vnum[k] == oidx->vnum)
                {
                    dup = TRUE;
                    break;
                }
            if (dup)
                continue;

            prop->prop_target_vnum[found] = oidx->vnum;
            prop->prop_target_done[found] = FALSE;
            found++;
        }

        prop->prop_num_targets = found;

        if (found == 0)
        {
            clear_proposition_slot(ch, slot);
            send_to_char("The postman shrugs. 'Sorry, I can't find any work for you right now.'\n\r", ch);
            return;
        }

        send_to_char("\n\r@@GYou have received a new proposition!@@N\n\r\n\r", ch);
        sprintf(buf, "Assigned to slot @@Y%d@@N.\n\r", slot + 1);
        send_to_char(buf, ch);
        send_to_char("Acquire each of the following items (credited when they enter your inventory):\n\r", ch);
        for (i = 0; i < prop->prop_num_targets; i++)
        {
            OBJ_INDEX_DATA *oidx = get_obj_index(prop->prop_target_vnum[i]);
            if (oidx != NULL)
            {
                sprintf(buf, "  @@C- %s @@N[level %d]\n\r", oidx->short_descr, oidx->level);
                send_to_char(buf, ch);
            }
        }
        break;
    }

    case PROP_TYPE_KILL_COUNT:
    {
        MOB_INDEX_DATA *midx;
        int kill_need = number_range(5, 15);

        midx = find_prop_mob_index(psuedo_lvl + 5, psuedo_lvl + 15);
        if (midx == NULL)
        {
            clear_proposition_slot(ch, slot);
            send_to_char("The postman shrugs. 'Sorry, I can't find any work for you right now.'\n\r", ch);
            return;
        }

        prop->prop_num_targets = 1;
        prop->prop_target_vnum[0] = midx->vnum;
        prop->prop_target_done[0] = FALSE;
        prop->prop_kill_needed = kill_need;
        prop->prop_kill_count = 0;

        send_to_char("\n\r@@GYou have received a new proposition!@@N\n\r\n\r", ch);
        sprintf(buf, "Assigned to slot @@Y%d@@N.\n\r", slot + 1);
        send_to_char(buf, ch);
        sprintf(buf, "Slay @@Y%d@@N of @@R%s@@N.\n\r", kill_need, midx->short_descr);
        send_to_char(buf, ch);
        break;
    }
    }

    show_reward_preview(ch, prop);
    send_to_char("Return to any postman when done to claim your reward.\n\r", ch);

    act("$N hands you a sealed note outlining your proposition.", ch, NULL, postman, TO_CHAR);
    act("$N hands $n a sealed proposition.", ch, NULL, postman, TO_ROOM);

    do_save(ch, "");
}

void proposition_status(CHAR_DATA *ch)
{
    int slot, i;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    if (!proposition_active(ch))
    {
        send_to_char("You have no active proposition. Visit a postman to request one.\n\r", ch);
        return;
    }

    send_to_char("@@Y===  Proposition Status  ===@@N\n\r\n\r", ch);

    for (slot = 0; slot < PROP_MAX_PROPOSITIONS; slot++)
    {
        PROPOSITION_DATA *prop = &ch->pcdata->propositions[slot];
        if (prop->prop_type == PROP_TYPE_NONE)
            continue;

        sprintf(buf, "@@W[Slot %d]@@N %s\n\r", slot + 1,
                prop->prop_static_id >= 0 ? "(static)" : "(dynamic)");
        send_to_char(buf, ch);

        if (prop->prop_completed)
        {
            send_to_char("@@GStatus: COMPLETE@@N - turn in at a postman.\n\r\n\r", ch);
            continue;
        }

        switch (prop->prop_type)
        {
        case PROP_TYPE_KILL_VARIETY:
            send_to_char("@@WTask:@@N Hunt down each of the following enemies:\n\r", ch);
            for (i = 0; i < prop->prop_num_targets; i++)
            {
                MOB_INDEX_DATA *midx = get_mob_index(prop->prop_target_vnum[i]);
                if (midx != NULL)
                {
                    sprintf(buf, "  %s%-30s@@N  %s\n\r",
                            prop->prop_target_done[i] ? "@@G" : "@@R",
                            midx->short_descr,
                            prop->prop_target_done[i] ? "[DONE]" : "[pending]");
                    send_to_char(buf, ch);
                }
            }
            break;

        case PROP_TYPE_COLLECT_ITEMS:
            send_to_char("@@WTask:@@N Acquire each of the following items:\n\r", ch);
            for (i = 0; i < prop->prop_num_targets; i++)
            {
                OBJ_INDEX_DATA *oidx = get_obj_index(prop->prop_target_vnum[i]);
                if (oidx != NULL)
                {
                    sprintf(buf, "  %s%-30s@@N  [lvl %-3d]  %s\n\r",
                            prop->prop_target_done[i] ? "@@G" : "@@C",
                            oidx->short_descr,
                            oidx->level,
                            prop->prop_target_done[i] ? "[OBTAINED]" : "[needed]");
                    send_to_char(buf, ch);
                }
            }
            break;

        case PROP_TYPE_KILL_COUNT:
        {
            MOB_INDEX_DATA *midx = get_mob_index(prop->prop_target_vnum[0]);
            sprintf(buf, "@@WTask:@@N Slay %s\n\r", midx ? midx->short_descr : "(unknown)");
            send_to_char(buf, ch);
            sprintf(buf, "  Progress: @@Y%d@@N / @@Y%d@@N\n\r",
                    prop->prop_kill_count,
                    prop->prop_kill_needed);
            send_to_char(buf, ch);
            break;
        }

        case PROP_TYPE_CARTOGRAPHY:
        {
            AREA_DATA *area = find_area_by_num(prop->prop_cart_area_num);
            sprintf(buf, "@@WTask:@@N Explore every room in area %s@@N.\n\r",
                    area != NULL ? area->name : "(unknown)");
            send_to_char(buf, ch);
            sprintf(buf, "  Progress: @@Y%d@@N / @@Y%d@@N rooms explored\n\r",
                    prop->prop_kill_count,
                    prop->prop_kill_needed);
            send_to_char(buf, ch);
            break;
        }
        }

        show_reward_preview(ch, prop);
        send_to_char("\n\r", ch);
    }
}

void proposition_room_notify(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
    int slot;

    if (IS_NPC(ch) || ch->pcdata == NULL || room == NULL || room->area == NULL)
        return;

    for (slot = 0; slot < PROP_MAX_PROPOSITIONS; slot++)
    {
        PROPOSITION_DATA *prop = &ch->pcdata->propositions[slot];

        if (prop->prop_type != PROP_TYPE_CARTOGRAPHY || prop->prop_completed)
            continue;
        if (prop->prop_cart_area_num != room->area->area_num)
            continue;
        if (prop_cart_room_seen(prop, room->vnum))
            continue;

        prop_cart_set_room_seen(prop, room->vnum);
        prop->prop_kill_count++;

        if (prop->prop_kill_count >= prop->prop_kill_needed)
        {
            prop->prop_completed = TRUE;
            send_to_char("\n\r@@G*** Proposition complete! Visit any postman to claim your reward. ***@@N\n\r\n\r", ch);
        }
        else
        {
            char buf[MAX_STRING_LENGTH];
            sprintf(buf, "@@GProposition slot %d progress:@@N explored %d/%d rooms.\n\r",
                    slot + 1,
                    prop->prop_kill_count,
                    prop->prop_kill_needed);
            send_to_char(buf, ch);
        }

        do_save(ch, "");
    }
}

void proposition_complete(CHAR_DATA *ch, CHAR_DATA *postman)
{
    int slot;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    for (slot = 0; slot < PROP_MAX_PROPOSITIONS; slot++)
    {
        PROPOSITION_DATA *prop = &ch->pcdata->propositions[slot];
            int gold_reward;
        int qp_reward;
        int i;
        const STATIC_PROP_TEMPLATE *tpl;

        if (prop->prop_type == PROP_TYPE_NONE || !prop->prop_completed)
            continue;

        gold_reward = prop->prop_static_id >= 0 ? prop->prop_reward_gold : proposition_gold(get_psuedo_level(ch));
        qp_reward = prop->prop_static_id >= 0 ? prop->prop_reward_qp : proposition_qp(get_psuedo_level(ch));

        act("$N reviews your completed proposition and nods approvingly.", ch, NULL, postman, TO_CHAR);
        act("$N reviews $n's completed proposition and nods approvingly.", ch, NULL, postman, TO_ROOM);

        send_to_char("\n\r@@GProposition complete! You receive:@@N\n\r", ch);
        sprintf(buf, "  @@Y%d@@N gold coins\n\r", gold_reward);
        send_to_char(buf, ch);
        sprintf(buf, "  @@Y%d@@N quest point%s\n\r", qp_reward, qp_reward == 1 ? "" : "s");
        send_to_char(buf, ch);
        send_to_char("  @@Y1@@N proposition point\n\r", ch);

        if (prop->prop_static_id >= 0)
        {
            tpl = find_static_prop_template(prop->prop_static_id);
            if (tpl != NULL && tpl->completion_message != NULL && tpl->completion_message[0] != '\0')
            {
                sprintf(buf, "@@WCompletion report:@@N %s\n\r", tpl->completion_message);
                send_to_char(buf, ch);
            }
        }

        ch->gold += gold_reward;
        ch->quest_points += qp_reward;
        ch->pcdata->proposition_points += 1;

        if (prop->prop_reward_item_vnum > 0 && prop->prop_reward_item_count > 0)
        {
            OBJ_INDEX_DATA *oidx = get_obj_index(prop->prop_reward_item_vnum);
            if (oidx != NULL)
            {
                for (i = 0; i < prop->prop_reward_item_count; i++)
                {
                    OBJ_DATA *reward = create_object(oidx, oidx->level);
                    if (reward != NULL)
                        obj_to_char(reward, ch);
                }
                sprintf(buf, "  @@Y%d@@N x %s\n\r", prop->prop_reward_item_count, oidx->short_descr);
                send_to_char(buf, ch);
            }
        }

        if (prop->prop_static_id >= 0 && prop->prop_static_id < PROP_MAX_STATIC_PROPOSITIONS)
            ch->pcdata->completed_static_props[prop->prop_static_id] = TRUE;

        clear_proposition_slot(ch, slot);
        do_save(ch, "");
        return;
    }

    if (!proposition_active(ch))
        send_to_char("You have no proposition to turn in.\n\r", ch);
    else
        send_to_char("You have no completed proposition to turn in yet.\n\r", ch);
}

void proposition_kill_notify(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int slot;

    if (IS_NPC(ch) || !IS_NPC(victim) || ch->pcdata == NULL)
        return;

    for (slot = 0; slot < PROP_MAX_PROPOSITIONS; slot++)
    {
        PROPOSITION_DATA *prop = &ch->pcdata->propositions[slot];
        int i;
        bool progress = FALSE;
        char buf[MAX_STRING_LENGTH];

        if (prop->prop_type == PROP_TYPE_NONE || prop->prop_completed)
            continue;

        switch (prop->prop_type)
        {
        case PROP_TYPE_KILL_VARIETY:
            for (i = 0; i < prop->prop_num_targets; i++)
            {
                if (!prop->prop_target_done[i] &&
                    prop->prop_target_vnum[i] == victim->pIndexData->vnum)
                {
                    prop->prop_target_done[i] = TRUE;
                    progress = TRUE;
                    sprintf(buf, "@@GProposition slot %d progress:@@N %s killed!\n\r",
                            slot + 1, victim->short_descr);
                    send_to_char(buf, ch);
                    break;
                }
            }
            if (progress && check_all_done(prop))
            {
                prop->prop_completed = TRUE;
                send_to_char("\n\r@@G*** Proposition complete! Visit any postman to claim your reward. ***@@N\n\r\n\r", ch);
            }
            break;

        case PROP_TYPE_KILL_COUNT:
            if (prop->prop_target_vnum[0] == victim->pIndexData->vnum)
            {
                prop->prop_kill_count++;
                progress = TRUE;
                sprintf(buf, "@@GProposition slot %d progress:@@N %d/%d %s slain.\n\r",
                        slot + 1,
                        prop->prop_kill_count,
                        prop->prop_kill_needed,
                        victim->short_descr);
                send_to_char(buf, ch);
                if (prop->prop_kill_count >= prop->prop_kill_needed)
                {
                    prop->prop_completed = TRUE;
                    send_to_char("\n\r@@G*** Proposition complete! Visit any postman to claim your reward. ***@@N\n\r\n\r", ch);
                }
            }
            break;
        }

        if (progress)
            do_save(ch, "");
    }
}

void proposition_obj_notify(CHAR_DATA *ch, OBJ_DATA *obj)
{
    int slot;

    if (IS_NPC(ch) || ch->pcdata == NULL)
        return;

    for (slot = 0; slot < PROP_MAX_PROPOSITIONS; slot++)
    {
        PROPOSITION_DATA *prop = &ch->pcdata->propositions[slot];
        int i;
        char buf[MAX_STRING_LENGTH];

        if (prop->prop_type != PROP_TYPE_COLLECT_ITEMS || prop->prop_completed)
            continue;

        for (i = 0; i < prop->prop_num_targets; i++)
        {
            if (!prop->prop_target_done[i] &&
                prop->prop_target_vnum[i] == obj->pIndexData->vnum)
            {
                prop->prop_target_done[i] = TRUE;
                sprintf(buf, "@@GProposition slot %d progress:@@N %s obtained!\n\r",
                        slot + 1, obj->short_descr);
                send_to_char(buf, ch);

                if (check_all_done(prop))
                {
                    prop->prop_completed = TRUE;
                    send_to_char("\n\r@@G*** Proposition complete! Visit any postman to claim your reward. ***@@N\n\r\n\r", ch);
                }

                extract_obj(obj);
                do_save(ch, "");
                return;
            }
        }
    }
}


void proposition_cancel(CHAR_DATA *ch, int slot)
{
    PROPOSITION_DATA *prop;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || ch->pcdata == NULL)
        return;

    if (slot < 0 || slot >= PROP_MAX_PROPOSITIONS)
    {
        send_to_char("Usage: proposition cancel <slot# 1-3>\n\r", ch);
        return;
    }

    prop = &ch->pcdata->propositions[slot];
    if (prop->prop_type == PROP_TYPE_NONE)
    {
        send_to_char("That proposition slot is already empty.\n\r", ch);
        return;
    }

    if (prop->prop_static_id < 0)
        ch->pcdata->prop_dynamic_cooldown_until = (int)current_time + 15 * 60;

    clear_proposition_slot(ch, slot);

    sprintf(buf, "You cancel proposition slot @@Y%d@@N.\n\r", slot + 1);
    send_to_char(buf, ch);

    if (ch->pcdata->prop_dynamic_cooldown_until > (int)current_time)
        send_to_char("Cancelling a dynamic proposition starts a 15 minute request cooldown.\n\r", ch);

    do_save(ch, "");
}

void do_proposition(CHAR_DATA *ch, char *argument)
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
        send_to_char("Usage: proposition <request|status|complete|list|accept #|cancel #>\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "status"))
    {
        proposition_status(ch);
        return;
    }

    if (!str_prefix(arg, "cancel"))
    {
        if (!is_number(arg2))
        {
            send_to_char("Usage: proposition cancel <slot# 1-3>\n\r", ch);
            return;
        }
        proposition_cancel(ch, atoi(arg2) - 1);
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

        if (postman == NULL)
        {
            send_to_char("You need to be with a postman to do that.\n\r", ch);
            return;
        }

        if (!str_prefix(arg, "request"))
            proposition_request(ch, postman);
        else if (!str_prefix(arg, "complete"))
            proposition_complete(ch, postman);
        else if (!str_prefix(arg, "list"))
            proposition_list_static(ch, postman);
        else
        {
            if (!is_number(arg2))
            {
                send_to_char("Usage: proposition accept <number>\n\r", ch);
                return;
            }
            proposition_accept_static(ch, postman, atoi(arg2));
        }

        return;
    }

    send_to_char("Usage: proposition <request|status|complete|list|accept #|cancel #>\n\r", ch);
}
