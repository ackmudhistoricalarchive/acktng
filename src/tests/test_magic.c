#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#define DEC_GLOBALS_H 1
#include "ack.h"

void magic_test_format_spell_utterance(char *dest, size_t dest_size, const char *words);
bool magic_test_npc_remort_cast_blocked_flags(int flag1, int act_bits, int affected_by_bits);
bool is_arcane_spell(int sn);
bool is_holy_power_spell(int sn);
void aoe_damage(CHAR_DATA *ch, OBJ_DATA *obj, int sn, int level, int min_dam, int max_dam,
                int element, int flags);
int spell_regen_base_heal(int mage_level, int sorcerer_level, int wizard_level, int spellpower);

#define AOE_SAVES (1 << 0)
#define AOE_SKIP_GROUP (1 << 1)

struct skill_type skill_table[MAX_SKILL];

/* ------------------------------------------------------------------ */
/* Stubs for external functions                                        */
/* ------------------------------------------------------------------ */

static int sp_damage_call_count;
static CHAR_DATA *sp_damage_last_victim;
static int sp_damage_last_dam;

static void reset_tracking(void)
{
   sp_damage_call_count = 0;
   sp_damage_last_victim = NULL;
   sp_damage_last_dam = 0;
}

bool sp_damage(OBJ_DATA *obj, CHAR_DATA *ch, CHAR_DATA *victim, int dam, int type, int sn,
               bool show_msg)
{
   sp_damage_call_count++;
   sp_damage_last_victim = victim;
   sp_damage_last_dam = dam;
   return TRUE;
}

static bool stub_saves_spell_result = FALSE;

bool saves_spell(int level, CHAR_DATA *victim)
{
   return stub_saves_spell_result;
}

int number_range(int from, int to)
{
   return to;
}

static bool stub_is_same_group_result = FALSE;

bool is_same_group(CHAR_DATA *ach, CHAR_DATA *bch)
{
   return stub_is_same_group_result;
}

void char_reference(struct char_ref_type *ref)
{
   if (ref->type == CHAR_NEXTROOM && *ref->var != NULL)
      ;
}

void char_unreference(CHAR_DATA **var)
{
   (void)var;
}

/* ------------------------------------------------------------------ */
/* Room / character helpers                                            */
/* ------------------------------------------------------------------ */

static bool dummy_spell(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   return TRUE;
}

static void clear_character(CHAR_DATA *ch)
{
   memset(ch, 0, sizeof(*ch));
   ch->mortal_class[0] = ch->mortal_class[1] = ch->mortal_class[2] = ch->mortal_class[3] = -1;
   ch->remort_class[0] = ch->remort_class[1] = -1;
   ch->adept_class = -1;
}

static void make_npc(CHAR_DATA *ch)
{
   clear_character(ch);
   ch->act = ACT_IS_NPC;
}

static void make_pc(CHAR_DATA *ch)
{
   clear_character(ch);
}

static void build_room(ROOM_INDEX_DATA *room, CHAR_DATA **chars, int n)
{
   memset(room, 0, sizeof(*room));
   room->first_person = (n > 0) ? chars[0] : NULL;
   for (int i = 0; i < n; i++)
   {
      chars[i]->in_room = room;
      chars[i]->next_in_room = (i + 1 < n) ? chars[i + 1] : NULL;
   }
}

/* ------------------------------------------------------------------ */
/* magic.c tests                                                       */
/* ------------------------------------------------------------------ */

static void test_spell_utterance_format(void)
{
   char buf[64];

   magic_test_format_spell_utterance(buf, sizeof(buf), "abra");
   assert(strcmp(buf, "$n utters the words, 'abra'.") == 0);
}

static void test_spell_utterance_truncates_safely(void)
{
   char words[200];
   char buf[32];

   memset(words, 'x', sizeof(words) - 1);
   words[sizeof(words) - 1] = '\0';

   magic_test_format_spell_utterance(buf, sizeof(buf), words);
   assert(buf[sizeof(buf) - 1] == '\0');
}

static void test_is_arcane_spell_matches_arcane_classes(void)
{
   memset(skill_table, 0, sizeof(skill_table));

   skill_table[1].name = "arcane";
   skill_table[1].spell_fun = dummy_spell;
   for (int i = 0; i < MAX_TOTAL_CLASS; i++)
      skill_table[1].skill_level[i] = NO_USE;
   skill_table[1].skill_level[CLASS_MAG] = 1;
   assert(is_arcane_spell(1) == TRUE);

   skill_table[2].name = "not_arcane";
   skill_table[2].spell_fun = dummy_spell;
   for (int i = 0; i < MAX_TOTAL_CLASS; i++)
      skill_table[2].skill_level[i] = NO_USE;
   skill_table[2].skill_level[CLASS_CLE] = 1;
   assert(is_arcane_spell(2) == FALSE);
}

static void test_is_arcane_spell_rejects_invalid_or_non_spell_entries(void)
{
   memset(skill_table, 0, sizeof(skill_table));

   assert(is_arcane_spell(-1) == FALSE);
   assert(is_arcane_spell(MAX_SKILL) == FALSE);

   skill_table[3].name = "no_spell_fun";
   for (int i = 0; i < MAX_TOTAL_CLASS; i++)
      skill_table[3].skill_level[i] = NO_USE;
   skill_table[3].skill_level[CLASS_MAG] = 1;
   assert(is_arcane_spell(3) == FALSE);
}

static void test_is_holy_power_spell_matches_divine_classes(void)
{
   memset(skill_table, 0, sizeof(skill_table));

   skill_table[4].name = "holy";
   skill_table[4].spell_fun = dummy_spell;
   for (int i = 0; i < MAX_TOTAL_CLASS; i++)
      skill_table[4].skill_level[i] = NO_USE;
   skill_table[4].skill_level[CLASS_PRI] = 1;
   assert(is_holy_power_spell(4) == TRUE);

   skill_table[5].name = "not_holy";
   skill_table[5].spell_fun = dummy_spell;
   for (int i = 0; i < MAX_TOTAL_CLASS; i++)
      skill_table[5].skill_level[i] = NO_USE;
   skill_table[5].skill_level[CLASS_MAG] = 1;
   assert(is_holy_power_spell(5) == FALSE);
}

static void test_npc_remort_cast_blocked_only_for_pet_or_charm(void)
{
   assert(magic_test_npc_remort_cast_blocked_flags(REMORT, 0, 0) == FALSE);
   assert(magic_test_npc_remort_cast_blocked_flags(REMORT, ACT_PET, 0) == TRUE);
   assert(magic_test_npc_remort_cast_blocked_flags(REMORT, 0, AFF_CHARM) == TRUE);
   assert(magic_test_npc_remort_cast_blocked_flags(ADEPT, ACT_PET, 0) == TRUE);
   assert(magic_test_npc_remort_cast_blocked_flags(MORTAL, ACT_PET, AFF_CHARM) == FALSE);
}

/* ------------------------------------------------------------------ */
/* aoe_damage tests (from test_magic2.c)                              */
/* ------------------------------------------------------------------ */

static void test_npc_hits_all_pcs(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA npc, pc1, pc2;
   CHAR_DATA *people[] = {&npc, &pc1, &pc2};

   make_npc(&npc);
   make_pc(&pc1);
   make_pc(&pc2);
   build_room(&room, people, 3);

   reset_tracking();
   stub_is_same_group_result = FALSE;
   stub_saves_spell_result = FALSE;
   aoe_damage(&npc, NULL, 1, 30, 50, 50, 0, 0);

   assert(sp_damage_call_count == 2);
}

static void test_pc_hits_all_npcs(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA pc, npc1, npc2;
   CHAR_DATA *people[] = {&pc, &npc1, &npc2};

   make_pc(&pc);
   make_npc(&npc1);
   make_npc(&npc2);
   build_room(&room, people, 3);

   reset_tracking();
   stub_is_same_group_result = FALSE;
   stub_saves_spell_result = FALSE;
   aoe_damage(&pc, NULL, 1, 30, 50, 50, 0, 0);

   assert(sp_damage_call_count == 2);
}

static void test_caster_not_hit(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA npc;
   CHAR_DATA *people[] = {&npc};

   make_npc(&npc);
   build_room(&room, people, 1);

   reset_tracking();
   aoe_damage(&npc, NULL, 1, 30, 50, 50, 0, 0);

   assert(sp_damage_call_count == 0);
}

static void test_npc_does_not_hit_npc(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA npc1, npc2;
   CHAR_DATA *people[] = {&npc1, &npc2};

   make_npc(&npc1);
   make_npc(&npc2);
   build_room(&room, people, 2);

   reset_tracking();
   aoe_damage(&npc1, NULL, 1, 30, 50, 50, 0, 0);

   assert(sp_damage_call_count == 0);
}

static void test_pc_does_not_hit_pc(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA pc1, pc2;
   CHAR_DATA *people[] = {&pc1, &pc2};

   make_pc(&pc1);
   make_pc(&pc2);
   build_room(&room, people, 2);

   reset_tracking();
   aoe_damage(&pc1, NULL, 1, 30, 50, 50, 0, 0);

   assert(sp_damage_call_count == 0);
}

static void test_skip_group_flag_skips_grouped_targets(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA npc, pc1, pc2;
   CHAR_DATA *people[] = {&npc, &pc1, &pc2};

   make_npc(&npc);
   make_pc(&pc1);
   make_pc(&pc2);
   build_room(&room, people, 3);

   reset_tracking();
   stub_is_same_group_result = TRUE;
   stub_saves_spell_result = FALSE;
   aoe_damage(&npc, NULL, 1, 30, 50, 50, 0, AOE_SKIP_GROUP);

   assert(sp_damage_call_count == 0);
}

static void test_no_skip_group_flag_hits_grouped_targets(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA npc, pc1;
   CHAR_DATA *people[] = {&npc, &pc1};

   make_npc(&npc);
   make_pc(&pc1);
   build_room(&room, people, 2);

   reset_tracking();
   stub_is_same_group_result = TRUE;
   stub_saves_spell_result = FALSE;
   aoe_damage(&npc, NULL, 1, 30, 50, 50, 0, 0);

   assert(sp_damage_call_count == 1);
}

static void test_saves_halves_damage(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA npc, pc1;
   CHAR_DATA *people[] = {&npc, &pc1};

   make_npc(&npc);
   make_pc(&pc1);
   build_room(&room, people, 2);

   reset_tracking();
   stub_saves_spell_result = TRUE;
   stub_is_same_group_result = FALSE;
   aoe_damage(&npc, NULL, 1, 30, 100, 100, 0, AOE_SAVES);

   assert(sp_damage_call_count == 1);
   assert(sp_damage_last_dam == 50);
}

static void test_no_saves_flag_deals_full_damage(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA npc, pc1;
   CHAR_DATA *people[] = {&npc, &pc1};

   make_npc(&npc);
   make_pc(&pc1);
   build_room(&room, people, 2);

   reset_tracking();
   stub_saves_spell_result = TRUE;
   stub_is_same_group_result = FALSE;
   aoe_damage(&npc, NULL, 1, 30, 100, 100, 0, 0);

   assert(sp_damage_call_count == 1);
   assert(sp_damage_last_dam == 100);
}

static void test_fixed_damage_exact(void)
{
   ROOM_INDEX_DATA room;
   CHAR_DATA npc, pc1;
   CHAR_DATA *people[] = {&npc, &pc1};

   make_npc(&npc);
   make_pc(&pc1);
   build_room(&room, people, 2);

   reset_tracking();
   stub_saves_spell_result = FALSE;
   stub_is_same_group_result = FALSE;
   aoe_damage(&npc, NULL, 1, 0, 77, 77, 0, 0);

   assert(sp_damage_call_count == 1);
   assert(sp_damage_last_dam == 77);
}

static void test_null_caster_is_safe(void)
{
   reset_tracking();
   aoe_damage(NULL, NULL, 1, 0, 50, 50, 0, 0);
   assert(sp_damage_call_count == 0);
}

/* ------------------------------------------------------------------ */
/* spell_regen_base_heal tests (from test_magic4.c)                   */
/* ------------------------------------------------------------------ */

static void test_base_and_mage_level_scaling(void)
{
   assert(spell_regen_base_heal(0, 0, 0, 0) == 10);
   assert(spell_regen_base_heal(11, 0, 0, 0) == 15);
}

static void test_remort_bonus_uses_higher_sorcerer_or_wizard(void)
{
   assert(spell_regen_base_heal(20, 4, 0, 0) == 27);
   assert(spell_regen_base_heal(20, 4, 9, 0) == 32);
}

static void test_spellpower_quarter_added(void)
{
   assert(spell_regen_base_heal(20, 0, 0, 19) == 24);
   assert(spell_regen_base_heal(20, 0, 0, 20) == 25);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
   test_spell_utterance_format();
   test_spell_utterance_truncates_safely();
   test_npc_remort_cast_blocked_only_for_pet_or_charm();
   test_is_arcane_spell_matches_arcane_classes();
   test_is_arcane_spell_rejects_invalid_or_non_spell_entries();
   test_is_holy_power_spell_matches_divine_classes();

   test_caster_not_hit();
   test_npc_hits_all_pcs();
   test_pc_hits_all_npcs();
   test_npc_does_not_hit_npc();
   test_pc_does_not_hit_pc();
   test_skip_group_flag_skips_grouped_targets();
   test_no_skip_group_flag_hits_grouped_targets();
   test_saves_halves_damage();
   test_no_saves_flag_deals_full_damage();
   test_fixed_damage_exact();
   test_null_caster_is_safe();

   test_base_and_mage_level_scaling();
   test_remort_bonus_uses_higher_sorcerer_or_wizard();
   test_spellpower_quarter_added();

   puts("test_magic: all tests passed");
   return 0;
}
