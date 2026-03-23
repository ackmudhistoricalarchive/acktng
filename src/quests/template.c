/***************************************************************************
 * template.c  --  Quest template management.
 *
 * Provides quest template lookup and reward helpers used throughout
 * the quest module. Templates are loaded from the database at boot.
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "quest_internal.h"

void set_obj_stat_auto(OBJ_DATA *obj);

QUEST_TEMPLATE *quest_template_table = NULL;
int quest_template_count = 0;

const QUEST_TEMPLATE *find_quest_template(int template_id)
{
   int i;

   for (i = 0; i < quest_template_count; i++)
      if (quest_template_table[i].id == template_id)
         return &quest_template_table[i];

   return NULL;
}

/*
 * Raw exp reward for a quest template: 3x the mob_base for a mob at max_level.
 * Boss quests (any target has ACT_BOSS) get double; cartography quests get 10x.
 * Character modifiers (adept, happy hour) are applied by the caller.
 */
int calc_quest_exp(int max_level, bool is_boss, bool is_cartography)
{
   int lvl = UMAX(0, UMIN(MAX_MOB_LEVEL - 1, max_level));
   int exp = (int)(exp_table[lvl].mob_base * 3L);
   if (is_boss)
      exp *= 2;
   if (is_cartography)
      exp *= 10;
   return UMAX(1, exp);
}

bool quest_template_has_boss_target(const QUEST_TEMPLATE *tpl)
{
   int i;
   if (tpl == NULL)
      return FALSE;
   for (i = 0; i < tpl->num_targets; i++)
   {
      MOB_INDEX_DATA *mob = get_mob_index(tpl->target_vnum[i]);
      if (mob != NULL && IS_SET(mob->act, ACT_BOSS))
         return TRUE;
   }
   return FALSE;
}

bool quest_reward_item_is_valid(const QUEST_TEMPLATE *tpl)
{
   if (tpl == NULL)
      return FALSE;

   if (tpl->reward_obj_short == NULL || tpl->reward_obj_short[0] == '\0' ||
       tpl->reward_obj_name == NULL || tpl->reward_obj_name[0] == '\0' ||
       tpl->reward_obj_long == NULL || tpl->reward_obj_long[0] == '\0')
      return FALSE;

   if (tpl->reward_obj_wear_flags == 0)
      return FALSE;

   if (tpl->reward_obj_weight <= 0)
      return FALSE;

   return TRUE;
}

OBJ_DATA *create_quest_reward_object(CHAR_DATA *ch, const QUEST_TEMPLATE *tpl)
{
   OBJ_DATA *reward;
   int spawn_level;

   if (ch == NULL || !quest_reward_item_is_valid(tpl))
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
