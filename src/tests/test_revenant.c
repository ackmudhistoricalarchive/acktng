/*
 * test_revenant.c — Unit tests for the revenant system.
 *
 * Tests: multiplier calculations, soul costs, track names,
 *        is_revenant detection, track scaling math.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEC_GLOBALS_H 1
#include "ack.h"
#include "revenant.h"

/* ── Stubs ──────────────────────────────────────────────────── */

CHAR_DATA *first_char = NULL;

sh_int get_psuedo_level(CHAR_DATA *ch)
{
   return ch->level;
}

long get_max_hp(CHAR_DATA *ch)
{
   return ch->hp_mod > 0 ? ch->hp_mod : 100;
}

long get_max_mana(CHAR_DATA *ch)
{
   (void)ch;
   return 120;
}

bool str_prefix(const char *astr, const char *bstr)
{
   if (astr == NULL || bstr == NULL)
      return TRUE;
   for (; *astr; astr++, bstr++)
   {
      if (*astr != *bstr)
         return TRUE;
   }
   return FALSE;
}

bool str_cmp(const char *astr, const char *bstr)
{
   if (astr == NULL && bstr == NULL)
      return FALSE;
   if (astr == NULL || bstr == NULL)
      return TRUE;
   return strcmp(astr, bstr) != 0;
}

void act(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type)
{
   (void)format;
   (void)ch;
   (void)arg1;
   (void)arg2;
   (void)type;
}

void send_to_char(const char *txt, CHAR_DATA *ch)
{
   (void)txt;
   (void)ch;
}

/* ── Helpers ────────────────────────────────────────────────── */

static void clear_character(CHAR_DATA *ch)
{
   memset(ch, 0, sizeof(CHAR_DATA));
   ch->mortal_class[0] = ch->mortal_class[1] = ch->mortal_class[2] = ch->mortal_class[3] = -1;
   ch->remort_class[0] = ch->remort_class[1] = -1;
   ch->adept_class = -1;
}

static MOB_INDEX_DATA test_mob_idx;

static void setup_test_revenant(CHAR_DATA *rev)
{
   clear_character(rev);
   memset(&test_mob_idx, 0, sizeof(test_mob_idx));
   rev->pIndexData = &test_mob_idx;
   rev->act = ACT_IS_NPC;
}

static REVENANT_DATA *make_revenant_data(void)
{
   REVENANT_DATA *rd = (REVENANT_DATA *)calloc(1, sizeof(REVENANT_DATA));
   rd->source_vnum = 12345;
   rd->corpse_level = 100;
   rd->soul_points = 0;
   rd->primary_track = REV_TRACK_NONE;
   rd->primary_rank = 1;
   rd->secondary_track = REV_TRACK_NONE;
   rd->secondary_rank = 1;
   rd->base_hr_mod = 50;
   rd->base_dr_mod = 40;
   rd->base_ac_mod = 100;
   rd->base_hp_mod = 5000;
   rd->base_spellpower_mod = 30;
   rd->base_healing_mod = 20;
   rd->base_crit_mod = 10;
   rd->base_crit_mult_mod = 5;
   rd->base_spell_crit_mod = 8;
   rd->base_spell_mult_mod = 3;
   rd->base_parry_mod = 15;
   rd->base_dodge_mod = 12;
   rd->base_block_mod = 18;
   rd->base_pierce_mod = 7;
   return rd;
}

/* ── Tests ──────────────────────────────────────────────────── */

static void test_primary_multiplier(void)
{
   assert(revenant_primary_multiplier(1) == 100);
   assert(revenant_primary_multiplier(2) == 110);
   assert(revenant_primary_multiplier(3) == 120);
   assert(revenant_primary_multiplier(4) == 135);
   assert(revenant_primary_multiplier(5) == 150);

   /* Edge cases */
   assert(revenant_primary_multiplier(0) == 100);
   assert(revenant_primary_multiplier(-1) == 100);
   assert(revenant_primary_multiplier(6) == 150); /* clamped */
   assert(revenant_primary_multiplier(99) == 150);

   printf("  test_primary_multiplier: PASSED\n");
}

static void test_secondary_multiplier(void)
{
   assert(revenant_secondary_multiplier(1) == 5);
   assert(revenant_secondary_multiplier(2) == 10);
   assert(revenant_secondary_multiplier(3) == 15);

   /* Edge cases */
   assert(revenant_secondary_multiplier(0) == 5);
   assert(revenant_secondary_multiplier(4) == 15); /* clamped */

   printf("  test_secondary_multiplier: PASSED\n");
}

static void test_soul_costs(void)
{
   assert(revenant_soul_cost_primary(1) == 0);
   assert(revenant_soul_cost_primary(2) == 25);
   assert(revenant_soul_cost_primary(3) == 75);
   assert(revenant_soul_cost_primary(4) == 150);
   assert(revenant_soul_cost_primary(5) == 300);

   assert(revenant_soul_cost_secondary(1) == 0);
   assert(revenant_soul_cost_secondary(2) == 50);
   assert(revenant_soul_cost_secondary(3) == 150);

   /* Edge cases */
   assert(revenant_soul_cost_primary(0) == 0);
   assert(revenant_soul_cost_primary(6) == 0);
   assert(revenant_soul_cost_secondary(0) == 0);
   assert(revenant_soul_cost_secondary(4) == 0);

   printf("  test_soul_costs: PASSED\n");
}

static void test_track_names(void)
{
   assert(strcmp(revenant_track_name(REV_TRACK_NONE), "none") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_DREAD), "dread") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_SHADOW), "shadow") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_BONE), "bone") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_SOUL), "soul") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_PLAGUE), "plague") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_BLOOD), "blood") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_WRATH), "wrath") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_WARD), "ward") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_HUNGER), "hunger") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_SPITE), "spite") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_WHISPER), "whisper") == 0);
   assert(strcmp(revenant_track_name(REV_TRACK_DECAY), "decay") == 0);

   /* Edge cases */
   assert(strcmp(revenant_track_name(-1), "unknown") == 0);
   assert(strcmp(revenant_track_name(99), "unknown") == 0);

   printf("  test_track_names: PASSED\n");
}

static void test_is_revenant(void)
{
   CHAR_DATA ch;
   MOB_INDEX_DATA mob_idx;
   REVENANT_DATA rd;

   clear_character(&ch);

   /* Not a revenant: no pIndexData, no revenant data */
   assert(is_revenant(&ch) == FALSE);

   /* NPC but no revenant data */
   memset(&mob_idx, 0, sizeof(mob_idx));
   ch.pIndexData = &mob_idx;
   ch.act = ACT_IS_NPC;
   assert(is_revenant(&ch) == FALSE);

   /* NPC with revenant data */
   memset(&rd, 0, sizeof(rd));
   ch.revenant = &rd;
   assert(is_revenant(&ch) == TRUE);

   /* NULL character */
   assert(is_revenant(NULL) == FALSE);

   printf("  test_is_revenant: PASSED\n");
}

static void test_apply_tracks_dread(void)
{
   CHAR_DATA rev;
   REVENANT_DATA *rd;

   setup_test_revenant(&rev);

   rd = make_revenant_data();
   rd->primary_track = REV_TRACK_DREAD;
   rd->primary_rank = 5; /* x1.50 */
   rev.revenant = rd;
   rev.level = 100;

   revenant_apply_tracks(&rev);

   /* Dread scales hr, dr, crit by x1.50 */
   assert(rev.hr_mod == 75);   /* 50 * 150 / 100 = 75 */
   assert(rev.dr_mod == 60);   /* 40 * 150 / 100 = 60 */
   assert(rev.crit_mod == 15); /* 10 * 150 / 100 = 15 */

   /* Non-dread stats remain at base */
   assert(rev.dodge_mod == 12);
   assert(rev.parry_mod == 15);
   assert(rev.block_mod == 18);
   assert(rev.ac_mod == 100);
   assert(rev.spellpower_mod == 30);

   free(rd);
   printf("  test_apply_tracks_dread: PASSED\n");
}

static void test_apply_tracks_bone(void)
{
   CHAR_DATA rev;
   REVENANT_DATA *rd;

   setup_test_revenant(&rev);

   rd = make_revenant_data();
   rd->primary_track = REV_TRACK_BONE;
   rd->primary_rank = 3; /* x1.20 */
   rev.revenant = rd;
   rev.level = 100;

   revenant_apply_tracks(&rev);

   /* Bone scales ac, block, hp by x1.20 */
   assert(rev.ac_mod == 120);   /* 100 * 120 / 100 = 120 */
   assert(rev.block_mod == 21); /* 18 * 120 / 100 = 21 (int truncation: 21.6 -> 21) */
   assert(rev.hp_mod == 6000);  /* 5000 * 120 / 100 = 6000 */

   /* Non-bone stats remain at base */
   assert(rev.hr_mod == 50);
   assert(rev.dr_mod == 40);

   free(rd);
   printf("  test_apply_tracks_bone: PASSED\n");
}

static void test_apply_tracks_soul(void)
{
   CHAR_DATA rev;
   REVENANT_DATA *rd;

   setup_test_revenant(&rev);

   rd = make_revenant_data();
   rd->primary_track = REV_TRACK_SOUL;
   rd->primary_rank = 4; /* x1.35 */
   rev.revenant = rd;
   rev.level = 100;

   revenant_apply_tracks(&rev);

   /* Soul scales spellpower, spell_crit, spell_mult by x1.35 */
   assert(rev.spellpower_mod == 40); /* 30 * 135 / 100 = 40 (int: 40.5 -> 40) */
   assert(rev.spell_crit_mod == 10); /* 8 * 135 / 100 = 10 (int: 10.8 -> 10) */
   assert(rev.spell_mult_mod == 4);  /* 3 * 135 / 100 = 4  (int: 4.05 -> 4) */

   /* Non-soul stats remain at base */
   assert(rev.hr_mod == 50);
   assert(rev.dr_mod == 40);
   assert(rev.dodge_mod == 12);

   free(rd);
   printf("  test_apply_tracks_soul: PASSED\n");
}

static void test_apply_tracks_shadow(void)
{
   CHAR_DATA rev;
   REVENANT_DATA *rd;

   setup_test_revenant(&rev);

   rd = make_revenant_data();
   rd->primary_track = REV_TRACK_SHADOW;
   rd->primary_rank = 5; /* x1.50 */
   rev.revenant = rd;
   rev.level = 100;

   revenant_apply_tracks(&rev);

   /* Shadow scales dodge, parry, pierce by x1.50 */
   assert(rev.dodge_mod == 18);  /* 12 * 150 / 100 = 18 */
   assert(rev.parry_mod == 22);  /* 15 * 150 / 100 = 22 (int: 22.5 -> 22) */
   assert(rev.pierce_mod == 10); /* 7 * 150 / 100 = 10  (int: 10.5 -> 10) */

   /* Non-shadow stats remain at base */
   assert(rev.hr_mod == 50);
   assert(rev.ac_mod == 100);

   free(rd);
   printf("  test_apply_tracks_shadow: PASSED\n");
}

static void test_apply_tracks_blood(void)
{
   CHAR_DATA rev;
   REVENANT_DATA *rd;

   setup_test_revenant(&rev);

   rd = make_revenant_data();
   rd->primary_track = REV_TRACK_BLOOD;
   rd->primary_rank = 2; /* x1.10 */
   rev.revenant = rd;
   rev.level = 100;

   revenant_apply_tracks(&rev);

   /* Blood scales healing, hr by x1.10 */
   assert(rev.healing_mod == 22); /* 20 * 110 / 100 = 22 */
   assert(rev.hr_mod == 55);      /* 50 * 110 / 100 = 55 */

   /* Non-blood stats remain at base */
   assert(rev.dr_mod == 40);
   assert(rev.dodge_mod == 12);

   free(rd);
   printf("  test_apply_tracks_blood: PASSED\n");
}

static void test_apply_tracks_none(void)
{
   CHAR_DATA rev;
   REVENANT_DATA *rd;

   setup_test_revenant(&rev);

   rd = make_revenant_data();
   rd->primary_track = REV_TRACK_NONE;
   rd->primary_rank = 1;
   rev.revenant = rd;
   rev.level = 100;

   revenant_apply_tracks(&rev);

   /* All stats should be at base */
   assert(rev.hr_mod == 50);
   assert(rev.dr_mod == 40);
   assert(rev.ac_mod == 100);
   assert(rev.dodge_mod == 12);
   assert(rev.parry_mod == 15);
   assert(rev.block_mod == 18);
   assert(rev.pierce_mod == 7);
   assert(rev.spellpower_mod == 30);
   assert(rev.healing_mod == 20);
   assert(rev.crit_mod == 10);

   free(rd);
   printf("  test_apply_tracks_none: PASSED\n");
}

static void test_spite_retaliate(void)
{
   CHAR_DATA rev;
   CHAR_DATA attacker;
   ROOM_INDEX_DATA room;
   REVENANT_DATA *rd;
   int retribution;

   setup_test_revenant(&rev);
   clear_character(&attacker);
   memset(&room, 0, sizeof(room));

   rev.in_room = &room;
   attacker.in_room = &room;

   rd = make_revenant_data();
   rd->secondary_track = REV_TRACK_SPITE;
   rd->secondary_rank = 3; /* 15% */
   rev.revenant = rd;

   /* Retaliation = base_hr_mod * 15 / 100 = 50 * 15 / 100 = 7 */
   /* Capped at damage_taken / 2 = 100 / 2 = 50 */
   retribution = revenant_spite_retaliate(&rev, &attacker, 100);
   assert(retribution == 7);

   /* With very small damage, cap applies */
   retribution = revenant_spite_retaliate(&rev, &attacker, 10);
   assert(retribution == 5); /* min(7, 10/2=5) = 5 */

   /* Wrong track = no retaliation */
   rd->secondary_track = REV_TRACK_WARD;
   retribution = revenant_spite_retaliate(&rev, &attacker, 100);
   assert(retribution == 0);

   /* Different room = no retaliation */
   rd->secondary_track = REV_TRACK_SPITE;
   ROOM_INDEX_DATA room2;
   memset(&room2, 0, sizeof(room2));
   attacker.in_room = &room2;
   retribution = revenant_spite_retaliate(&rev, &attacker, 100);
   assert(retribution == 0);

   free(rd);
   printf("  test_spite_retaliate: PASSED\n");
}

static void test_track_constants(void)
{
   /* Verify primary tracks are 1-6 */
   assert(REV_TRACK_DREAD == 1);
   assert(REV_TRACK_SHADOW == 2);
   assert(REV_TRACK_BONE == 3);
   assert(REV_TRACK_SOUL == 4);
   assert(REV_TRACK_PLAGUE == 5);
   assert(REV_TRACK_BLOOD == 6);

   /* Verify secondary tracks are 7-12 */
   assert(REV_TRACK_WRATH == 7);
   assert(REV_TRACK_WARD == 8);
   assert(REV_TRACK_HUNGER == 9);
   assert(REV_TRACK_SPITE == 10);
   assert(REV_TRACK_WHISPER == 11);
   assert(REV_TRACK_DECAY == 12);

   /* Verify limits */
   assert(REV_MAX_PRIMARY_RANK == 5);
   assert(REV_MAX_SECONDARY_RANK == 3);
   assert(REV_LEVEL_CAP == 150);
   assert(REV_MAX_TRACK == 12);

   printf("  test_track_constants: PASSED\n");
}

static void test_weak_corpse_vs_strong_corpse(void)
{
   CHAR_DATA rev_weak, rev_strong;
   REVENANT_DATA *rd_weak, *rd_strong;

   setup_test_revenant(&rev_weak);
   setup_test_revenant(&rev_strong);

   /* Weak corpse: dr_mod=3 */
   rd_weak = make_revenant_data();
   rd_weak->base_dr_mod = 3;
   rd_weak->primary_track = REV_TRACK_DREAD;
   rd_weak->primary_rank = 5;
   rev_weak.revenant = rd_weak;
   rev_weak.level = 30;

   /* Strong corpse: dr_mod=60 */
   rd_strong = make_revenant_data();
   rd_strong->base_dr_mod = 60;
   rd_strong->primary_track = REV_TRACK_DREAD;
   rd_strong->primary_rank = 5;
   rev_strong.revenant = rd_strong;
   rev_strong.level = 150;

   revenant_apply_tracks(&rev_weak);
   revenant_apply_tracks(&rev_strong);

   /* Weak: 3 * 150 / 100 = 4 */
   assert(rev_weak.dr_mod == 4);
   /* Strong: 60 * 150 / 100 = 90 */
   assert(rev_strong.dr_mod == 90);

   /* The strong corpse gets 22x more damroll from the same track rank */
   assert(rev_strong.dr_mod > rev_weak.dr_mod * 10);

   free(rd_weak);
   free(rd_strong);
   printf("  test_weak_corpse_vs_strong_corpse: PASSED\n");
}

int main(void)
{
   printf("test_revenant:\n");

   test_primary_multiplier();
   test_secondary_multiplier();
   test_soul_costs();
   test_track_names();
   test_is_revenant();
   test_apply_tracks_dread();
   test_apply_tracks_bone();
   test_apply_tracks_soul();
   test_apply_tracks_shadow();
   test_apply_tracks_blood();
   test_apply_tracks_none();
   test_spite_retaliate();
   test_track_constants();
   test_weak_corpse_vs_strong_corpse();

   printf("test_revenant: all tests passed\n");
   return 0;
}
