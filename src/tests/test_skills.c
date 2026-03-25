/*
 * Unit tests for skills.c — can_use_skill (reinc table), raise_skill
 * (level messages), and skill_success (probability table).
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DEC_GLOBALS_H 1
#include "ack.h"

/* ── GSN stubs ─────────────────────────────────────────────────────── */
sh_int gsn_enhanced_damage = 1;
sh_int gsn_enhanced_critical = 2;
sh_int gsn_counter = 3;
sh_int gsn_potency = 4;
sh_int gsn_spell_critical = 5;
sh_int gsn_spell_critical_damage = 6;
sh_int gsn_dualwield = 7;
sh_int gsn_two_handed = 8;
sh_int gsn_equip_buckler = 9;
sh_int gsn_equip_fist = 10;
sh_int gsn_dual_fist = 11;
sh_int gsn_equip_wand = 12;
sh_int gsn_enhanced_sword_critical = 13;
sh_int gsn_notrip = 14;
sh_int gsn_substrate_piercing = 16;

/* ── minimal skill_table ───────────────────────────────────────────── */
struct skill_type skill_table[MAX_SKILL];

/* ── minimal race_table (no race skills needed for these tests) ────── */
const struct race_type race_table[MAX_RACE] = {
    [0] = {.skill = ""},
};

/* ── minimal gclass_table: set tier for IS_MORTAL/REMORT/ADEPT_CLASS ── */
const struct class_type gclass_table[MAX_TOTAL_CLASS] = {
    [0] = {.tier = MORTAL},  [1] = {.tier = MORTAL},  [2] = {.tier = MORTAL},
    [3] = {.tier = MORTAL},  [4] = {.tier = MORTAL},  [5] = {.tier = MORTAL},
    [6] = {.tier = REMORT},  [7] = {.tier = REMORT},  [8] = {.tier = REMORT},
    [9] = {.tier = REMORT},  [10] = {.tier = REMORT}, [11] = {.tier = REMORT},
    [12] = {.tier = REMORT}, [13] = {.tier = REMORT}, [14] = {.tier = REMORT},
    [15] = {.tier = REMORT}, [16] = {.tier = REMORT}, [17] = {.tier = REMORT},
    [18] = {.tier = ADEPT},  [19] = {.tier = ADEPT},  [20] = {.tier = ADEPT},
    [21] = {.tier = ADEPT},  [22] = {.tier = ADEPT},  [23] = {.tier = ADEPT},
    [24] = {.tier = MORTAL}, [25] = {.tier = REMORT}, [26] = {.tier = REMORT},
    [27] = {.tier = ADEPT},
};

/* ── test doubles ─────────────────────────────────────────────────── */

static char last_sent[4096];
static int send_count;

void send_to_char(const char *txt, CHAR_DATA *ch)
{
   (void)ch;
   strncpy(last_sent, txt, sizeof(last_sent) - 1);
   last_sent[sizeof(last_sent) - 1] = '\0';
   send_count++;
}

static int fixed_percent = 50;

int number_percent(void)
{
   return fixed_percent;
}

sh_int get_psuedo_level(CHAR_DATA *ch)
{
   return ch->level;
}

char *one_argument(char *argument, char *arg_first)
{
   /* stub: returns empty string immediately */
   arg_first[0] = '\0';
   return argument;
}

bool can_use_pub_society_skill(CHAR_DATA *ch, int gsn)
{
   (void)ch;
   (void)gsn;
   return FALSE;
}

int skill_lookup(const char *name)
{
   (void)name;
   return -1;
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

/* ── helpers ──────────────────────────────────────────────────────── */

static void clear_character(CHAR_DATA *ch)
{
   memset(ch, 0, sizeof *ch);
   ch->mortal_class[0] = ch->mortal_class[1] = ch->mortal_class[2] = ch->mortal_class[3] = -1;
   ch->remort_class[0] = ch->remort_class[1] = -1;
   ch->adept_class = -1;
}

static void clear_pcdata(PC_DATA *pcdata)
{
   memset(pcdata, 0, sizeof *pcdata);
}

/* Initialise all skill_level entries to -1 (not available) so that
 * skills unlock only via explicit class assignment or reinc rules. */
static void init_skill_table(void)
{
   int i, j;
   for (i = 0; i < MAX_SKILL; i++)
      for (j = 0; j < MAX_TOTAL_CLASS; j++)
         skill_table[i].skill_level[j] = -1;
}

/* ── tests: can_use_skill via reinc table ─────────────────────────── */

static void test_can_use_skill_npc_always_true(void)
{
   CHAR_DATA ch;
   clear_character(&ch);
   ch.act = ACT_IS_NPC; /* IS_NPC checks act flag */

   assert(can_use_skill(&ch, gsn_enhanced_damage) == TRUE);
   printf("PASS: test_can_use_skill_npc_always_true\n");
}

static void test_can_use_skill_cooldown_blocks(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);
   ch.pcdata = &pcdata;
   ch.cooldown[gsn_potency] = 5;

   assert(can_use_skill(&ch, gsn_potency) == FALSE);
   printf("PASS: test_can_use_skill_cooldown_blocks\n");
}

static void test_can_use_skill_single_reinc(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);
   ch.pcdata = &pcdata;

   /* Without reinc: FALSE */
   assert(can_use_skill(&ch, gsn_enhanced_damage) == FALSE);

   /* With one WAR reincarnation: TRUE */
   pcdata.reincarnations[CLASS_WAR] = 1;
   assert(can_use_skill(&ch, gsn_enhanced_damage) == TRUE);
   printf("PASS: test_can_use_skill_single_reinc\n");
}

static void test_can_use_skill_dual_reinc_threshold(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);
   ch.pcdata = &pcdata;

   /* dualwield needs ASS + WLK >= 20 */
   pcdata.reincarnations[CLASS_ASS] = 10;
   pcdata.reincarnations[CLASS_WLK] = 9;
   assert(can_use_skill(&ch, gsn_dualwield) == FALSE);

   pcdata.reincarnations[CLASS_WLK] = 10;
   assert(can_use_skill(&ch, gsn_dualwield) == TRUE);
   printf("PASS: test_can_use_skill_dual_reinc_threshold\n");
}

static void test_can_use_skill_invalid_gsn(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);
   ch.pcdata = &pcdata;

   assert(can_use_skill(&ch, -1) == FALSE);
   assert(can_use_skill(&ch, MAX_SKILL) == FALSE);
   printf("PASS: test_can_use_skill_invalid_gsn\n");
}

/* ── tests: raise_skill level messages ─────────────────────────────── */

static void test_raise_skill_sends_level_message(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);
   ch.pcdata = &pcdata;

   int gsn = 15; /* use a gsn not in reinc_rules */
   skill_table[gsn].name = "slash";
   /* Give the PC the skill at the relevant class level */
   skill_table[gsn].skill_level[CLASS_WAR] = 1;
   ch.mortal_class[0] = CLASS_WAR;
   ch.mortal_level[0] = 5;

   /* Set learned to LEVEL_ONE - 1 so raise_skill will hit LEVEL_ONE */
   pcdata.learned[gsn] = LEVEL_ONE - 1;

   send_count = 0;
   bool result = raise_skill(&ch, gsn);
   assert(result == TRUE);
   assert(send_count == 1);
   assert(strstr(last_sent, "level one") != NULL);
   printf("PASS: test_raise_skill_sends_level_message\n");
}

static void test_raise_skill_master_message(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);
   ch.pcdata = &pcdata;

   int gsn = 15;
   skill_table[gsn].name = "slash";
   skill_table[gsn].skill_level[CLASS_WAR] = 1;
   ch.mortal_class[0] = CLASS_WAR;
   ch.mortal_level[0] = 5;

   pcdata.learned[gsn] = LEVEL_MASTER - 1;
   send_count = 0;
   raise_skill(&ch, gsn);
   assert(send_count == 1);
   assert(strstr(last_sent, "mastered") != NULL);
   printf("PASS: test_raise_skill_master_message\n");
}

static void test_raise_skill_npc_always_true(void)
{
   CHAR_DATA ch;
   clear_character(&ch);
   ch.act = ACT_IS_NPC;

   /* NPC should always return TRUE and not crash */
   assert(raise_skill(&ch, 15) == TRUE);
   printf("PASS: test_raise_skill_npc_always_true\n");
}

/* ── tests: skill_success probability table ─────────────────────────── */

static void test_skill_success_base_probability(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);
   ch.pcdata = &pcdata;
   ch.level = 10;

   int gsn = 15;
   pcdata.learned[gsn] = 0; /* no learning */

   /* base chance = 70, no bonuses; with fixed_percent=50 → success */
   fixed_percent = 50;
   assert(skill_success(&ch, NULL, gsn, 0) == TRUE);

   /* with fixed_percent=80 → failure */
   fixed_percent = 80;
   assert(skill_success(&ch, NULL, gsn, 0) == FALSE);

   printf("PASS: test_skill_success_base_probability\n");
}

static void test_skill_success_learned_bonus(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);
   ch.pcdata = &pcdata;
   ch.level = 10;

   int gsn = 15;
   /* Set to LEVEL_GM (highest): should get all 5+5+5+5+5+1+1 = 27 bonus → chance = 97 */
   pcdata.learned[gsn] = LEVEL_GM + 1;

   fixed_percent = 96;
   assert(skill_success(&ch, NULL, gsn, 0) == TRUE);

   fixed_percent = 97;
   assert(skill_success(&ch, NULL, gsn, 0) == FALSE);

   printf("PASS: test_skill_success_learned_bonus\n");
}

/* ── main ─────────────────────────────────────────────────────────── */

int main(void)
{
   init_skill_table();
   test_can_use_skill_npc_always_true();
   test_can_use_skill_cooldown_blocks();
   test_can_use_skill_single_reinc();
   test_can_use_skill_dual_reinc_threshold();
   test_can_use_skill_invalid_gsn();
   test_raise_skill_sends_level_message();
   test_raise_skill_master_message();
   test_raise_skill_npc_always_true();
   test_skill_success_base_probability();
   test_skill_success_learned_bonus();

   printf("All skills unit tests passed.\n");
   return 0;
}
