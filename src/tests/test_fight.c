#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Skip globals.h from ack.h to avoid incomplete-type extern arrays in unit test TU. */
#define DEC_GLOBALS_H 1
#include "ack.h"

/* Test doubles for external dependencies used by fight.c avoidance helpers. */
sh_int gsn_parry = 1;
sh_int gsn_dodge = 2;
sh_int gsn_shield_block = 3;
sh_int gsn_counter = 4;
sh_int gsn_vigilance = 5;
sh_int gsn_measured_response = 6;
sh_int gsn_ninth_descent = 7;

const struct stance_app_type stance_app[MAX_STANCE] = {
    [2] = {.speed_mod = 3},
    [3] = {.speed_mod = 1},
};

static bool skill_available = TRUE;
static OBJ_DATA *left_hand = NULL;
static OBJ_DATA *right_hand = NULL;
static OBJ_DATA *two_handed = NULL;
static OBJ_DATA *buckler_slot = NULL;
static PC_DATA default_pcdata;

int get_curr_str(CHAR_DATA *ch)
{
   return ch->damroll;
}
int get_curr_dex(CHAR_DATA *ch)
{
   return ch->hitroll;
}
int get_curr_con(CHAR_DATA *ch)
{
   return ch->saving_throw;
}
int get_curr_wis(CHAR_DATA *ch)
{
   return ch->armor;
}

int char_class_level(const CHAR_DATA *ch, int class_idx)
{
   if (ch->act & ACT_IS_NPC)
      return (ch->class == class_idx) ? ch->level : 0;
   for (int i = 0; i < 4; i++)
      if (ch->mortal_class[i] == class_idx)
         return ch->mortal_level[i];
   for (int i = 0; i < 2; i++)
      if (ch->remort_class[i] == class_idx)
         return ch->remort_level[i];
   if (ch->adept_class == class_idx)
      return ch->adept_level;
   return 0;
}

/* Sentinel stubs for fight.c */
bool is_sentinel_class(CHAR_DATA *ch)
{
   return char_class_level(ch, CLASS_SEN) > 0;
}
void set_testimony_target(CHAR_DATA *ch, CHAR_DATA *victim)
{
   ch->testimony_target = victim;
   ch->testimony_combat_rounds = 0;
}
void add_testimony(CHAR_DATA *ch, int amount)
{
   ch->testimony = ch->testimony + amount;
   if (ch->testimony > 9)
      ch->testimony = 9;
}
bool is_affected(CHAR_DATA *ch, int sn)
{
   (void)ch;
   (void)sn;
   return FALSE;
}
int get_speed(CHAR_DATA *ch)
{
   return ch->wait;
}

bool can_use_skill(CHAR_DATA *ch, int gsn)
{
   (void)ch;
   (void)gsn;
   return skill_available;
}

OBJ_DATA *get_eq_char(CHAR_DATA *ch, int iWear)
{
   (void)ch;

   if (iWear == WEAR_HOLD_HAND_L)
      return left_hand;
   if (iWear == WEAR_HOLD_HAND_R)
      return right_hand;
   if (iWear == WEAR_TWO_HANDED)
      return two_handed;
   if (iWear == WEAR_BUCKLER)
      return buckler_slot;

   return NULL;
}

sh_int get_psuedo_level(CHAR_DATA *ch)
{
   return ch->level;
}

/* Public society passive hook stubs */
int pub_society_dodge_bonus(CHAR_DATA *ch)
{
   (void)ch;
   return 0;
}
int pub_society_extra_attack(CHAR_DATA *ch)
{
   (void)ch;
   return 0;
}
bool pub_society_marked_strike_check(CHAR_DATA *ch, CHAR_DATA *victim)
{
   (void)ch;
   (void)victim;
   return FALSE;
}

int get_parry(CHAR_DATA *ch);
int get_dodge(CHAR_DATA *ch);
int get_block(CHAR_DATA *ch);
int get_counter(CHAR_DATA *ch);
int get_evasion_piercing(CHAR_DATA *ch);
bool shortfight_summary_recipient_matches(CHAR_DATA *rch, CHAR_DATA *ch, CHAR_DATA *victim,
                                          bool expected_shortfight);
bool shortfight_can_broadcast_room_summary(CHAR_DATA *ch, CHAR_DATA *victim);
bool should_summon_assist_master_round(int is_npc, int is_charmed, int has_master,
                                       int master_fighting, int same_room, int is_player_summon,
                                       int can_see_master_target);
bool should_summon_cast_round(int is_npc, int is_player_summon, int is_fighting, int has_spec_fun,
                              int should_cast_now);
int get_level_scaled_avoidance_baseline(CHAR_DATA *ch, CHAR_DATA *victim, int base_chance);

static void clear_character(CHAR_DATA *ch)
{
   memset(ch, 0, sizeof(*ch));
   ch->mortal_class[0] = ch->mortal_class[1] = ch->mortal_class[2] = ch->mortal_class[3] = -1;
   ch->remort_class[0] = ch->remort_class[1] = -1;
   ch->adept_class = -1;
   memset(&default_pcdata, 0, sizeof(default_pcdata));
   ch->pcdata = &default_pcdata;
   ch->position = POS_STANDING;
}

static void reset_equipment(void)
{
   left_hand = NULL;
   right_hand = NULL;
   two_handed = NULL;
   buckler_slot = NULL;
}

static void test_get_parry_guard_clauses(void)
{
   CHAR_DATA ch;

   clear_character(&ch);
   ch.position = POS_SLEEPING;
   assert(get_parry(&ch) == 0);

   clear_character(&ch);
   ch.act = ACT_IS_NPC;
   assert(get_parry(&ch) == 0);

   clear_character(&ch);
   skill_available = FALSE;
   assert(get_parry(&ch) == 0);
   skill_available = TRUE;

   reset_equipment();
}

static void test_get_parry_composition_and_cap(void)
{
   CHAR_DATA ch;
   OBJ_DATA weapon;

   clear_character(&ch);
   memset(&weapon, 0, sizeof(weapon));
   weapon.item_type = ITEM_WEAPON;
   right_hand = &weapon;

   ch.damroll = 20; /* get_curr_str */
   ch.stance = 2;   /* speed_mod = 3 */
   ch.wait = 2;     /* speed contribution = 10 */
   ch.parry_mod = 9;
   ch.affected_by = AFF_CLOAK_ADEPT;

   assert(get_parry(&ch) == 47);

   ch.parry_mod = 30;
   assert(get_parry(&ch) == 50);

   reset_equipment();
}

static void test_get_parry_weapon_slot_fallbacks(void)
{
   CHAR_DATA ch;
   OBJ_DATA left_weapon;
   OBJ_DATA two_hand_weapon;

   clear_character(&ch);
   memset(&left_weapon, 0, sizeof(left_weapon));
   left_weapon.item_type = ITEM_WEAPON;
   left_hand = &left_weapon;

   ch.damroll = 12;
   ch.wait = 1;
   ch.stance = 3;
   assert(get_parry(&ch) == 18);

   reset_equipment();
   clear_character(&ch);
   memset(&two_hand_weapon, 0, sizeof(two_hand_weapon));
   two_hand_weapon.item_type = ITEM_WEAPON;
   two_handed = &two_hand_weapon;

   ch.damroll = 14;
   ch.wait = 0;
   ch.stance = 2;
   assert(get_parry(&ch) == 17);

   reset_equipment();
}

static void test_get_dodge_guard_clauses(void)
{
   CHAR_DATA ch;

   clear_character(&ch);
   ch.position = POS_SLEEPING;
   assert(get_dodge(&ch) == 0);

   clear_character(&ch);
   ch.act = ACT_IS_NPC;
   assert(get_dodge(&ch) == 0);

   clear_character(&ch);
   skill_available = FALSE;
   assert(get_dodge(&ch) == 0);
   skill_available = TRUE;
}

static void test_get_dodge_npc_and_cap(void)
{
   CHAR_DATA ch;

   clear_character(&ch);
   ch.act = ACT_IS_NPC | ACT_SOLO;
   ch.skills = MOB_DODGE;
   ch.hitroll = 40;
   ch.wait = 2;
   ch.stance = 2;

   assert(get_dodge(&ch) == 50);
}

static void test_get_block_shield_requirements_and_buckler(void)
{
   CHAR_DATA ch;
   OBJ_DATA buckler;

   clear_character(&ch);
   assert(get_block(&ch) == 0);

   clear_character(&ch);
   skill_available = FALSE;
   assert(get_block(&ch) == 0);
   skill_available = TRUE;

   clear_character(&ch);
   memset(&buckler, 0, sizeof(buckler));
   buckler.item_type = ITEM_ARMOR;
   buckler_slot = &buckler;
   ch.saving_throw = 20;
   ch.wait = 1;
   ch.stance = 2;
   assert(get_block(&ch) == 18);
   reset_equipment();
}

static void test_get_block_standard_shield(void)
{
   CHAR_DATA ch;
   OBJ_DATA shield;

   clear_character(&ch);
   memset(&shield, 0, sizeof(shield));
   shield.item_type = ITEM_ARMOR;
   right_hand = &shield;

   ch.saving_throw = 30;
   ch.wait = 1;
   ch.stance = 3;
   ch.block_mod = 4;
   ch.affected_by = AFF_CLOAK_ADEPT;
   assert(get_block(&ch) == 45);

   reset_equipment();
}

static void test_get_block_npc_left_shield_and_cap(void)
{
   CHAR_DATA ch;
   OBJ_DATA shield;

   clear_character(&ch);
   memset(&shield, 0, sizeof(shield));
   shield.item_type = ITEM_ARMOR;
   left_hand = &shield;

   ch.act = ACT_IS_NPC | ACT_SOLO;
   ch.saving_throw = 40;
   ch.wait = 2;
   ch.stance = 2;
   ch.block_mod = 8;

   assert(get_block(&ch) == 50);

   reset_equipment();
}

static void test_get_counter_requires_awake_and_training(void)
{
   CHAR_DATA ch;

   clear_character(&ch);
   ch.position = POS_SLEEPING;
   assert(get_counter(&ch) == 0);

   clear_character(&ch);
   ch.act = ACT_IS_NPC;
   assert(get_counter(&ch) == 0);

   clear_character(&ch);
   skill_available = FALSE;
   assert(get_counter(&ch) == 0);
   skill_available = TRUE;
}

static void test_get_counter_applies_modifiers(void)
{
   CHAR_DATA ch;
   OBJ_DATA fist_left;
   OBJ_DATA fist_right;

   clear_character(&ch);
   ch.act = ACT_IS_NPC | ACT_SOLO;
   ch.skills = MOB_COUNTER;
   ch.level = 62;
   ch.class = CLASS_MON; /* NPC has single class; gives fist bonus via MON/BRA check */
   ch.hitroll = 25;
   ch.wait = 2;
   ch.affected_by = AFF_CLOAK_ADEPT;

   memset(&fist_left, 0, sizeof(fist_left));
   memset(&fist_right, 0, sizeof(fist_right));
   fist_left.extra_flags = ITEM_FIST;
   fist_right.extra_flags = ITEM_FIST;

   left_hand = &fist_left;
   right_hand = &fist_right;

   assert(get_counter(&ch) == 66); /* 20+10 base + 15 solo + 6 fist(MON) + 10 speed + 5 cloak */

   reset_equipment();
}

static void test_get_counter_npc_ignores_can_use_skill(void)
{
   CHAR_DATA ch;

   clear_character(&ch);
   ch.act = ACT_IS_NPC | ACT_SOLO;
   ch.skills = MOB_COUNTER;
   ch.level = 31;
   ch.hitroll = 10;
   ch.wait = 1;

   skill_available = FALSE;
   assert(get_counter(&ch) == 34);
   skill_available = TRUE;
}

static void test_shortfight_can_broadcast_room_summary_requires_room(void)
{
   CHAR_DATA ch;
   CHAR_DATA victim;
   ROOM_INDEX_DATA room;

   clear_character(&ch);
   clear_character(&victim);
   memset(&room, 0, sizeof(room));

   assert(shortfight_can_broadcast_room_summary(NULL, &victim) == FALSE);
   assert(shortfight_can_broadcast_room_summary(&ch, NULL) == FALSE);

   ch.in_room = NULL;
   victim.in_room = &room;
   assert(shortfight_can_broadcast_room_summary(&ch, &victim) == FALSE);

   ch.in_room = &room;
   victim.in_room = NULL;
   assert(shortfight_can_broadcast_room_summary(&ch, &victim) == FALSE);
}

static void test_shortfight_can_broadcast_room_summary_requires_same_room(void)
{
   CHAR_DATA ch;
   CHAR_DATA victim;
   ROOM_INDEX_DATA room1;
   ROOM_INDEX_DATA room2;

   clear_character(&ch);
   clear_character(&victim);
   memset(&room1, 0, sizeof(room1));
   memset(&room2, 0, sizeof(room2));

   ch.in_room = &room1;
   victim.in_room = &room2;
   assert(shortfight_can_broadcast_room_summary(&ch, &victim) == FALSE);

   victim.in_room = &room1;
   assert(shortfight_can_broadcast_room_summary(&ch, &victim) == TRUE);
}

static void test_shortfight_summary_recipient_null_inputs(void)
{
   CHAR_DATA attacker;
   CHAR_DATA victim;
   CHAR_DATA observer;

   clear_character(&attacker);
   clear_character(&victim);
   clear_character(&observer);

   assert(shortfight_summary_recipient_matches(&observer, NULL, &victim, TRUE) == FALSE);
   assert(shortfight_summary_recipient_matches(&observer, &attacker, NULL, TRUE) == FALSE);
   assert(shortfight_summary_recipient_matches(NULL, NULL, NULL, TRUE) == FALSE);
}

static void test_shortfight_summary_recipient_expected_toggle(void)
{
   CHAR_DATA attacker;
   CHAR_DATA victim;
   CHAR_DATA observer;

   clear_character(&attacker);
   clear_character(&victim);
   clear_character(&observer);

   assert(shortfight_summary_recipient_matches(&observer, &attacker, &victim, TRUE) == FALSE);
   assert(shortfight_summary_recipient_matches(&observer, &attacker, &victim, FALSE) == TRUE);

   SET_BIT(observer.config, CONFIG_SHORT_FIGHT);
   assert(shortfight_summary_recipient_matches(&observer, &attacker, &victim, TRUE) == TRUE);
   assert(shortfight_summary_recipient_matches(&observer, &attacker, &victim, FALSE) == FALSE);
}

static void test_shortfight_summary_recipient_npc_is_never_shortfight(void)
{
   CHAR_DATA attacker;
   CHAR_DATA victim;
   CHAR_DATA npc_observer;

   clear_character(&attacker);
   clear_character(&victim);
   clear_character(&npc_observer);

   npc_observer.act = ACT_IS_NPC;
   SET_BIT(npc_observer.config, CONFIG_SHORT_FIGHT);

   assert(shortfight_summary_recipient_matches(&npc_observer, &attacker, &victim, TRUE) == FALSE);
   assert(shortfight_summary_recipient_matches(&npc_observer, &attacker, &victim, FALSE) == TRUE);
}

static void test_shortfight_summary_recipient_matching(void)
{
   CHAR_DATA attacker;
   CHAR_DATA victim;
   CHAR_DATA observer_short;
   CHAR_DATA observer_long;
   CHAR_DATA npc_observer;

   clear_character(&attacker);
   clear_character(&victim);
   clear_character(&observer_short);
   clear_character(&observer_long);
   clear_character(&npc_observer);

   attacker.name = "attacker";
   victim.name = "victim";

   SET_BIT(observer_short.config, CONFIG_SHORT_FIGHT);

   assert(shortfight_summary_recipient_matches(NULL, &attacker, &victim, TRUE) == FALSE);
   assert(shortfight_summary_recipient_matches(&attacker, &attacker, &victim, TRUE) == FALSE);
   assert(shortfight_summary_recipient_matches(&victim, &attacker, &victim, TRUE) == FALSE);

   assert(shortfight_summary_recipient_matches(&observer_short, &attacker, &victim, TRUE) == TRUE);
   assert(shortfight_summary_recipient_matches(&observer_short, &attacker, &victim, FALSE) ==
          FALSE);

   assert(shortfight_summary_recipient_matches(&observer_long, &attacker, &victim, TRUE) == FALSE);
   assert(shortfight_summary_recipient_matches(&observer_long, &attacker, &victim, FALSE) == TRUE);

   npc_observer.act = ACT_IS_NPC;
   assert(shortfight_summary_recipient_matches(&npc_observer, &attacker, &victim, TRUE) == FALSE);
   assert(shortfight_summary_recipient_matches(&npc_observer, &attacker, &victim, FALSE) == TRUE);
}

static void test_should_summon_assist_master_round(void)
{
   assert(should_summon_assist_master_round(true, true, true, true, true, true, true) == true);

   assert(should_summon_assist_master_round(false, true, true, true, true, true, true) == false);
   assert(should_summon_assist_master_round(true, false, true, true, true, true, true) == false);
   assert(should_summon_assist_master_round(true, true, false, true, true, true, true) == false);
   assert(should_summon_assist_master_round(true, true, true, false, true, true, true) == false);
   assert(should_summon_assist_master_round(true, true, true, true, false, true, true) == false);
   assert(should_summon_assist_master_round(true, true, true, true, true, false, true) == false);
   assert(should_summon_assist_master_round(true, true, true, true, true, true, false) == false);
}

static void test_should_summon_cast_round(void)
{
   assert(should_summon_cast_round(true, true, true, true, true) == true);

   assert(should_summon_cast_round(false, true, true, true, true) == false);
   assert(should_summon_cast_round(true, false, true, true, true) == false);
   assert(should_summon_cast_round(true, true, false, true, true) == false);
   assert(should_summon_cast_round(true, true, true, false, true) == false);
   assert(should_summon_cast_round(true, true, true, true, false) == false);
}
static void test_get_evasion_piercing_composition(void)
{
   CHAR_DATA ch;

   clear_character(&ch);
   ch.act = ACT_IS_NPC | ACT_SOLO;
   ch.wait = 2;
   ch.stance = 2;
   ch.pierce_mod = 7;

   assert(get_evasion_piercing(&ch) == 25);

   ch.position = POS_SLEEPING;
   assert(get_evasion_piercing(&ch) == 0);
}

static void test_level_scaled_avoidance_baseline_baselines(void)
{
   CHAR_DATA attacker;
   CHAR_DATA victim;

   clear_character(&attacker);
   clear_character(&victim);

   attacker.level = 50;
   victim.level = 50;
   assert(get_level_scaled_avoidance_baseline(&attacker, &victim, 10) == 10);

   attacker.act = ACT_IS_NPC;
   victim.act = ACT_IS_NPC;
   assert(get_level_scaled_avoidance_baseline(&attacker, &victim, 10) == 10);

   attacker.act = 0;
   victim.act = ACT_IS_NPC;
   victim.level = 70;
   assert(get_level_scaled_avoidance_baseline(&attacker, &victim, 10) == 10);

   attacker.act = ACT_IS_NPC;
   attacker.level = 70;
   victim.act = 0;
   victim.level = 50;
   assert(get_level_scaled_avoidance_baseline(&attacker, &victim, 10) == 10);
}

static void test_level_scaled_avoidance_baseline_level_gap_scaling(void)
{
   CHAR_DATA attacker;
   CHAR_DATA victim;

   clear_character(&attacker);
   clear_character(&victim);

   attacker.level = 50;
   victim.level = 60;
   assert(get_level_scaled_avoidance_baseline(&attacker, &victim, 10) == 5);

   victim.level = 40;
   assert(get_level_scaled_avoidance_baseline(&attacker, &victim, 30) == 25);

   victim.act = ACT_IS_NPC;
   victim.level = 100;
   assert(get_level_scaled_avoidance_baseline(&attacker, &victim, 10) == 0);
}

static void test_boss_avoidance_bonuses(void)
{
   CHAR_DATA ch;

   clear_character(&ch);
   ch.act = ACT_IS_NPC | ACT_BOSS;
   ch.skills = MOB_DODGE | MOB_PARRY | MOB_COUNTER;
   ch.hitroll = 18;
   ch.damroll = 19;
   ch.wait = 1;

   assert(get_dodge(&ch) == 48);
   assert(get_parry(&ch) == 49);
   assert(get_counter(&ch) == 37);
   assert(get_evasion_piercing(&ch) == 15);
}

/* Moved from comm.c — now lives in fight.c */
bool shortfight_should_suppress_watched_autoattack(int observer_is_npc, int observer_has_shortfight,
                                                   int observer_is_fighting);

static void test_shortfight_watched_autoattack_suppression(void)
{
   /* Non-NPC observer with shortfight ON and NOT currently fighting: suppress */
   assert(shortfight_should_suppress_watched_autoattack(false, true, false));

   /* Observer is currently fighting: do NOT suppress */
   assert(!shortfight_should_suppress_watched_autoattack(false, true, true));

   /* Observer doesn't have shortfight config: do NOT suppress */
   assert(!shortfight_should_suppress_watched_autoattack(false, false, false));

   /* Observer is NPC: do NOT suppress */
   assert(!shortfight_should_suppress_watched_autoattack(true, true, false));
}

int main(void)
{
   test_get_parry_guard_clauses();
   test_get_parry_composition_and_cap();
   test_get_parry_weapon_slot_fallbacks();
   test_get_dodge_guard_clauses();
   test_get_dodge_npc_and_cap();
   test_get_block_shield_requirements_and_buckler();
   test_get_block_standard_shield();
   test_get_block_npc_left_shield_and_cap();
   test_get_counter_requires_awake_and_training();
   test_get_counter_applies_modifiers();
   test_get_counter_npc_ignores_can_use_skill();
   test_shortfight_can_broadcast_room_summary_requires_room();
   test_shortfight_can_broadcast_room_summary_requires_same_room();
   test_shortfight_summary_recipient_null_inputs();
   test_shortfight_summary_recipient_expected_toggle();
   test_shortfight_summary_recipient_npc_is_never_shortfight();
   test_shortfight_summary_recipient_matching();
   test_should_summon_assist_master_round();
   test_should_summon_cast_round();
   test_get_evasion_piercing_composition();
   test_level_scaled_avoidance_baseline_baselines();
   test_level_scaled_avoidance_baseline_level_gap_scaling();
   test_boss_avoidance_bonuses();
   test_shortfight_watched_autoattack_suppression();

   puts("test_fight: all tests passed");
   return 0;
}
