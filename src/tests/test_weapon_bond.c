/*
 * test_weapon_bond.c — Unit tests for the weapon bonding system.
 *
 * Tests: rank costs, multipliers, track names, guard ac/parry,
 *        keen crit/crit_mult, bond_award_kill, is_bonded_weapon.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEC_GLOBALS_H 1
#include "ack.h"
#include "weapon_bond.h"

/* ── Stubs ──────────────────────────────────────────────────── */

CHAR_DATA *first_char = NULL;
AFFECT_DATA *affect_free = NULL;

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

int get_curr_str(CHAR_DATA *ch)
{
   (void)ch;
   return 18;
}

void _free_string(char *pstr, const char *caller)
{
   (void)pstr;
   (void)caller;
}

#undef str_dup
char *_str_dup(const char *str, const char *caller)
{
   (void)caller;
   if (str == NULL)
      return strdup("");
   return strdup(str);
}

/* ── Helpers ────────────────────────────────────────────────── */

static void clear_character(CHAR_DATA *ch)
{
   memset(ch, 0, sizeof(CHAR_DATA));
   ch->mortal_class[0] = ch->mortal_class[1] = ch->mortal_class[2] = ch->mortal_class[3] = -1;
   ch->remort_class[0] = ch->remort_class[1] = -1;
   ch->adept_class = -1;
}

/* ── Tests ──────────────────────────────────────────────────── */

static void test_rank_costs(void)
{
   assert(bond_rank_cost(1) == 0);
   assert(bond_rank_cost(2) == 125);
   assert(bond_rank_cost(3) == 375);
   assert(bond_rank_cost(4) == 750);
   assert(bond_rank_cost(5) == 1500);

   /* Edge cases */
   assert(bond_rank_cost(0) == 0);
   assert(bond_rank_cost(6) == 0);
   assert(bond_rank_cost(-1) == 0);

   printf("  test_rank_costs: PASSED\n");
}

static void test_rank_multipliers(void)
{
   assert(bond_rank_multiplier(1) == 100);
   assert(bond_rank_multiplier(2) == 110);
   assert(bond_rank_multiplier(3) == 120);
   assert(bond_rank_multiplier(4) == 135);
   assert(bond_rank_multiplier(5) == 150);

   /* Edge cases */
   assert(bond_rank_multiplier(0) == 100);
   assert(bond_rank_multiplier(-1) == 100);
   assert(bond_rank_multiplier(6) == 150);

   printf("  test_rank_multipliers: PASSED\n");
}

static void test_track_names(void)
{
   assert(strcmp(bond_track_name(BOND_TRACK_NONE), "none") == 0);
   assert(strcmp(bond_track_name(BOND_TRACK_EDGE), "edge") == 0);
   assert(strcmp(bond_track_name(BOND_TRACK_GUARD), "guard") == 0);
   assert(strcmp(bond_track_name(BOND_TRACK_KEEN), "keen") == 0);
   assert(strcmp(bond_track_name(BOND_TRACK_SPIRIT), "spirit") == 0);

   /* Edge cases */
   assert(strcmp(bond_track_name(-1), "unknown") == 0);
   assert(strcmp(bond_track_name(99), "unknown") == 0);

   printf("  test_track_names: PASSED\n");
}

static void test_guard_ac(void)
{
   /* Level 150 */
   assert(bond_guard_ac(1, 150) == -(150 * 33 / 100));  /* -49 */
   assert(bond_guard_ac(2, 150) == -(150 * 66 / 100));  /* -99 */
   assert(bond_guard_ac(3, 150) == -(150 * 100 / 100)); /* -150 */
   assert(bond_guard_ac(4, 150) == -(150 * 150 / 100)); /* -225 */
   assert(bond_guard_ac(5, 150) == -(150 * 200 / 100)); /* -300 */

   /* Level 75 */
   assert(bond_guard_ac(1, 75) == -(75 * 33 / 100));  /* -24 */
   assert(bond_guard_ac(5, 75) == -(75 * 200 / 100)); /* -150 */

   /* Level 1 */
   assert(bond_guard_ac(5, 1) == -(1 * 200 / 100)); /* -2 */

   printf("  test_guard_ac: PASSED\n");
}

static void test_guard_parry(void)
{
   assert(bond_guard_parry(1) == 1);
   assert(bond_guard_parry(2) == 2);
   assert(bond_guard_parry(3) == 3);
   assert(bond_guard_parry(4) == 4);
   assert(bond_guard_parry(5) == 5);

   printf("  test_guard_parry: PASSED\n");
}

static void test_keen_crit(void)
{
   assert(bond_keen_crit(1) == 1);
   assert(bond_keen_crit(2) == 2);
   assert(bond_keen_crit(3) == 3);
   assert(bond_keen_crit(4) == 4);
   assert(bond_keen_crit(5) == 5);

   printf("  test_keen_crit: PASSED\n");
}

static void test_keen_crit_mult(void)
{
   assert(bond_keen_crit_mult(1) == 2);
   assert(bond_keen_crit_mult(2) == 4);
   assert(bond_keen_crit_mult(3) == 6);
   assert(bond_keen_crit_mult(4) == 9);
   assert(bond_keen_crit_mult(5) == 12);

   printf("  test_keen_crit_mult: PASSED\n");
}

static void test_is_bonded_weapon(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   OBJ_DATA obj;

   clear_character(&ch);
   memset(&pcdata, 0, sizeof(pcdata));
   memset(&obj, 0, sizeof(obj));

   ch.pcdata = &pcdata;

   /* No bond data */
   assert(is_bonded_weapon(&ch, &obj) == FALSE);

   /* NPC can't have bonded weapons */
   ch.act = ACT_IS_NPC;
   assert(is_bonded_weapon(&ch, &obj) == FALSE);

   /* PC with bond data but obj not marked */
   ch.act = 0;
   BOND_DATA bond;
   memset(&bond, 0, sizeof(bond));
   pcdata.bond = &bond;
   assert(is_bonded_weapon(&ch, &obj) == FALSE);

   /* PC with bond data and obj marked with ITEM_BONDED */
   SET_BIT(obj.extra_flags, ITEM_BONDED);
   assert(is_bonded_weapon(&ch, &obj) == TRUE);

   printf("  test_is_bonded_weapon: PASSED\n");
}

static void test_bond_free(void)
{
   /* bond_free with NULL should not crash */
   bond_free(NULL);

   printf("  test_bond_free: PASSED\n");
}

int main(void)
{
   printf("test_weapon_bond:\n");

   test_rank_costs();
   test_rank_multipliers();
   test_track_names();
   test_guard_ac();
   test_guard_parry();
   test_keen_crit();
   test_keen_crit_mult();
   test_is_bonded_weapon();
   test_bond_free();

   printf("All weapon_bond tests PASSED.\n");
   return 0;
}
