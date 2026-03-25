/*
 * Unit tests for handler.c — specifically the stat helper refactors.
 *
 * Tests: get_max_stat (race base, class prime, reinc bonus, NPC path)
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DEC_GLOBALS_H 1
#include "ack.h"

/* ── test doubles ─────────────────────────────────────────────────── */

/* Minimal race table: index 0 = "human-like" (all stats 13),
 *                     index 1 = "warrior-race" (str=15, rest=12)     */
const struct race_type race_table[MAX_RACE] = {
    [0] = {.race_str = 13,
           .race_int = 13,
           .race_wis = 13,
           .race_dex = 13,
           .race_con = 13,
           .skill = ""},
    [1] = {.race_str = 15,
           .race_int = 12,
           .race_wis = 12,
           .race_dex = 12,
           .race_con = 14,
           .skill = ""},
};

/* Minimal class table: CLASS_MAG (0) primes INT, CLASS_WAR (1) primes STR */
const struct class_type gclass_table[MAX_TOTAL_CLASS] = {
    [CLASS_MAG] = {.attr_prime = APPLY_INT}, [CLASS_WAR] = {.attr_prime = APPLY_STR},
    [CLASS_PSI] = {.attr_prime = APPLY_INT}, [CLASS_CLE] = {.attr_prime = APPLY_WIS},
    [CLASS_CIP] = {.attr_prime = APPLY_DEX}, [CLASS_PUG] = {.attr_prime = APPLY_CON},
};

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

/* ── test: NPC stat ────────────────────────────────────────────────── */

static void test_get_curr_str_npc(void)
{
   CHAR_DATA ch;
   clear_character(&ch);
   ch.act = ACT_IS_NPC; /* IS_NPC checks act flag, not pcdata */
   ch.level = 16;

   /* NPC should get 13 + level/16 = 13 + 1 = 14 */
   assert(get_curr_str(&ch) == 14);
   printf("PASS: test_get_curr_str_npc\n");
}

/* ── test: PC with race base only ─────────────────────────────────── */

static void test_get_max_stat_race_base_only(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);

   ch.pcdata = &pcdata;
   ch.race = 0;          /* human-like: all stats = 13 */
   ch.class = CLASS_CLE; /* prime = WIS, not STR */

   /* STR max = 13 (race base) + 0 (no prime) + 0 (no reinc) = 13 */
   assert(get_max_stat(&ch, APPLY_STR) == 13);
   printf("PASS: test_get_max_stat_race_base_only\n");
}

/* ── test: PC with class prime bonus ──────────────────────────────── */

static void test_get_max_stat_class_prime(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);

   ch.pcdata = &pcdata;
   ch.race = 0;          /* all stats 13 */
   ch.class = CLASS_MAG; /* prime = INT */

   /* INT max = 13 (race) + 1 (class prime) = 14 */
   assert(get_max_stat(&ch, APPLY_INT) == 14);
   /* STR max = 13 (no prime) */
   assert(get_max_stat(&ch, APPLY_STR) == 13);
   printf("PASS: test_get_max_stat_class_prime\n");
}

/* ── test: PC with reincarnation bonus (STR from WAR reinc) ────────── */

static void test_get_max_stat_reinc_bonus(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);

   ch.pcdata = &pcdata;
   ch.race = 0;                          /* all stats 13 */
   ch.class = CLASS_CIP;                 /* prime = DEX */
   pcdata.reincarnations[CLASS_WAR] = 2; /* WAR reinc bonus for STR */

   /* STR max = 13 + 0 (no prime) + (2+1)/2=1 reinc bonus = 14 */
   assert(get_max_stat(&ch, APPLY_STR) == 14);
   /* DEX max = 13 + 1 (prime) + 0 (no reinc) = 14 */
   assert(get_max_stat(&ch, APPLY_DEX) == 14);
   printf("PASS: test_get_max_stat_reinc_bonus\n");
}

/* ── test: INT gets bonus from both MAG and PSI reincs ──────────────── */

static void test_get_max_stat_int_dual_reinc(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);

   ch.pcdata = &pcdata;
   ch.race = 0;
   ch.class = CLASS_WAR;                 /* prime = STR */
   pcdata.reincarnations[CLASS_MAG] = 2; /* (2+1)/2 = 1 */
   pcdata.reincarnations[CLASS_PSI] = 4; /* (4+1)/2 = 2 */

   /* INT max = 13 + 0 (prime) + 1 (MAG) + 2 (PSI) = 16 */
   assert(get_max_stat(&ch, APPLY_INT) == 16);
   printf("PASS: test_get_max_stat_int_dual_reinc\n");
}

/* ── test: warrior race has higher STR base ───────────────────────── */

static void test_get_max_stat_warrior_race(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);

   ch.pcdata = &pcdata;
   ch.race = 1;          /* str=15, int=12, dex=12 */
   ch.class = CLASS_WAR; /* prime = STR */

   /* STR max = 15 + 1 (prime) = 16 */
   assert(get_max_stat(&ch, APPLY_STR) == 16);
   /* INT max = 12 (no prime) */
   assert(get_max_stat(&ch, APPLY_INT) == 12);
   printf("PASS: test_get_max_stat_warrior_race\n");
}

/* ── test: STAT_MAX cap ───────────────────────────────────────────── */

static void test_get_max_stat_cap(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch);
   clear_pcdata(&pcdata);

   ch.pcdata = &pcdata;
   ch.race = 1;                           /* str=15 */
   ch.class = CLASS_WAR;                  /* prime = STR (+1) */
   pcdata.reincarnations[CLASS_WAR] = 30; /* (30+1)/2 = 15 bonus */

   /* Would be 15 + 1 + 15 = 31, but capped at STAT_MAX (30) */
   assert(get_max_stat(&ch, APPLY_STR) == STAT_MAX);
   printf("PASS: test_get_max_stat_cap\n");
}

/* ── main ─────────────────────────────────────────────────────────── */

int main(void)
{
   test_get_curr_str_npc();
   test_get_max_stat_race_base_only();
   test_get_max_stat_class_prime();
   test_get_max_stat_reinc_bonus();
   test_get_max_stat_int_dual_reinc();
   test_get_max_stat_warrior_race();
   test_get_max_stat_cap();

   printf("All handler unit tests passed.\n");
   return 0;
}
