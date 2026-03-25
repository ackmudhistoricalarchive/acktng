#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DEC_GLOBALS_H 1
#include "ack.h"
#include "special.h"

struct skill_type skill_table[MAX_SKILL];

/* Stubs for spec_fun pointers used in death handler tests */
bool spec_pyramid_black_sun_shard(CHAR_DATA *ch)
{
   (void)ch;
   return FALSE;
}
bool spec_keep_elemental_captain(CHAR_DATA *ch)
{
   (void)ch;
   return FALSE;
}
bool spec_keep_physical_captain(CHAR_DATA *ch)
{
   (void)ch;
   return FALSE;
}
bool spec_hermit_archon(CHAR_DATA *ch)
{
   (void)ch;
   return FALSE;
}
bool spec_sanctum_keeper(CHAR_DATA *ch)
{
   (void)ch;
   return FALSE;
}
bool spec_breath_any(CHAR_DATA *ch)
{
   (void)ch;
   return FALSE;
}

static int g_cast_count = 0;
static int g_send_count = 0;

void send_to_char(const char *txt, CHAR_DATA *ch)
{
   (void)txt;
   (void)ch;
   g_send_count++;
}

bool is_same_group(CHAR_DATA *ach, CHAR_DATA *bch)
{
   (void)ach;
   (void)bch;
   return TRUE;
}

int get_max_hp(CHAR_DATA *ch)
{
   return ch != NULL ? ch->max_hit : 0;
}

int number_range(int from, int to)
{
   (void)to;
   return from;
}

static bool test_spell_fun(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)sn;
   (void)level;
   (void)ch;
   (void)vo;
   (void)obj;

   g_cast_count++;
   return TRUE;
}

int skill_lookup(const char *name)
{
   static const char *const known[] = {
       "chill touch", "acid blast",   "weaken",         "fireball",     "high explosive",
       "curse",       "earthquake",   "harm",           "poison",       "holy wrath",
       "dispel evil", "energy drain", "lightning bolt", "thought vise", "mind bolt",
       "feeble mind", "heal"};

   for (int i = 0; i < (int)(sizeof(known) / sizeof(known[0])); i++)
   {
      if (strcmp(name, known[i]) == 0)
         return i + 1;
   }

   return -1;
}

#define SUMMON_SPECIAL_COUNT 9

static bool summon_special_casts_in_combat(CHAR_DATA *ch, int index)
{
   switch (index)
   {
   case 0:
      return spec_summon_water(ch);
   case 1:
      return spec_summon_fire(ch);
   case 2:
      return spec_summon_earth(ch);
   case 3:
      return spec_summon_undead(ch);
   case 4:
      return spec_summon_holy(ch);
   case 5:
      return spec_summon_shadow(ch);
   case 6:
      return spec_summon_metal(ch);
   case 7:
      return spec_summon_animate(ch);
   case 8:
      return spec_summon_thought(ch);
   default:
      return FALSE;
   }
}

static void clear_character(CHAR_DATA *ch)
{
   memset(ch, 0, sizeof(*ch));
   ch->mortal_class[0] = ch->mortal_class[1] = ch->mortal_class[2] = ch->mortal_class[3] = -1;
   ch->remort_class[0] = ch->remort_class[1] = -1;
   ch->adept_class = -1;
}

static void initialize_spell_table(void)
{
   for (int i = 0; i < MAX_SKILL; i++)
      skill_table[i].spell_fun = test_spell_fun;

   for (int i = 1; i <= 17; i++)
      skill_table[i].spell_fun = test_spell_fun;
}

static void test_no_chance_when_master_full_or_invalid(void)
{
   assert(summon_master_heal_chance(100, 100, 30) == 0);
   assert(summon_master_heal_chance(50, 0, 30) == 0);
}

static void test_chance_increases_as_master_hp_drops(void)
{
   int high_hp = summon_master_heal_chance(90, 100, 20);
   int low_hp = summon_master_heal_chance(25, 100, 20);

   assert(high_hp == 30);
   assert(low_hp == 95);
   assert(low_hp > high_hp);
}

static void test_chance_respects_bounds(void)
{
   assert(summon_master_heal_chance(99, 100, -50) == 5);
   assert(summon_master_heal_chance(-10, 100, 0) == 95);
}

static void test_all_summon_specs_cast_in_combat(void)
{
   CHAR_DATA summon;
   CHAR_DATA target;

   clear_character(&summon);
   clear_character(&target);

   initialize_spell_table();

   summon.position = POS_FIGHTING;
   summon.level = 80;
   summon.fighting = &target;

   for (int i = 0; i < SUMMON_SPECIAL_COUNT; i++)
   {
      g_cast_count = 0;
      assert(summon_special_casts_in_combat(&summon, i) == TRUE);
      assert(g_cast_count == 1);
   }
}

static void test_superboss_index_identifies_superbosses(void)
{
   CHAR_DATA mob;
   clear_character(&mob);

   mob.spec_fun = spec_pyramid_black_sun_shard;
   assert(spec_superboss_index(&mob) == SUPERBOSS_PYRAMID_BLACK_SUN_SHARD);

   mob.spec_fun = spec_keep_elemental_captain;
   assert(spec_superboss_index(&mob) == SUPERBOSS_KEEP_ELEMENTAL_CAPTAIN);

   mob.spec_fun = spec_keep_physical_captain;
   assert(spec_superboss_index(&mob) == SUPERBOSS_KEEP_PHYSICAL_CAPTAIN);

   mob.spec_fun = spec_hermit_archon;
   assert(spec_superboss_index(&mob) == SUPERBOSS_HERMIT_ARCHON);

   mob.spec_fun = spec_sanctum_keeper;
   assert(spec_superboss_index(&mob) == SUPERBOSS_SANCTUM_KEEPER);

   mob.spec_fun = spec_breath_any;
   assert(spec_superboss_index(&mob) == -1);

   mob.spec_fun = NULL;
   assert(spec_superboss_index(&mob) == -1);
}

static void test_death_handler_awards_reincarnation_point(void)
{
   CHAR_DATA victim;
   CHAR_DATA killer;
   PC_DATA pcdata;
   ROOM_INDEX_DATA room;

   clear_character(&victim);
   clear_character(&killer);
   memset(&pcdata, 0, sizeof(pcdata));
   memset(&room, 0, sizeof(room));

   victim.spec_fun = spec_pyramid_black_sun_shard;
   victim.short_descr = "the Black Sun Shard";
   victim.in_room = &room;
   room.first_person = &killer;

   killer.next_in_room = NULL;
   killer.pcdata = &pcdata;
   killer.act = 0; /* not NPC */

   g_send_count = 0;
   spec_death_handler(&victim, &killer);

   assert(pcdata.superboss_kills[SUPERBOSS_PYRAMID_BLACK_SUN_SHARD] == TRUE);
   assert(pcdata.reincarnation_data[REINCARNATION_POINTS] == 1);
   assert(g_send_count == 1);
}

static void test_death_handler_no_double_award(void)
{
   CHAR_DATA victim;
   CHAR_DATA killer;
   PC_DATA pcdata;
   ROOM_INDEX_DATA room;

   clear_character(&victim);
   clear_character(&killer);
   memset(&pcdata, 0, sizeof(pcdata));
   memset(&room, 0, sizeof(room));

   victim.spec_fun = spec_keep_elemental_captain;
   victim.short_descr = "the Elemental Captain";
   victim.in_room = &room;
   room.first_person = &killer;

   killer.next_in_room = NULL;
   killer.pcdata = &pcdata;
   killer.act = 0;

   pcdata.superboss_kills[SUPERBOSS_KEEP_ELEMENTAL_CAPTAIN] = TRUE;
   pcdata.reincarnation_data[REINCARNATION_POINTS] = 5;

   g_send_count = 0;
   spec_death_handler(&victim, &killer);

   assert(pcdata.superboss_kills[SUPERBOSS_KEEP_ELEMENTAL_CAPTAIN] == TRUE);
   assert(pcdata.reincarnation_data[REINCARNATION_POINTS] == 5);
   assert(g_send_count == 0);
}

static void test_death_handler_ignores_non_superboss(void)
{
   CHAR_DATA victim;
   CHAR_DATA killer;
   PC_DATA pcdata;
   ROOM_INDEX_DATA room;

   clear_character(&victim);
   clear_character(&killer);
   memset(&pcdata, 0, sizeof(pcdata));
   memset(&room, 0, sizeof(room));

   victim.spec_fun = spec_breath_any;
   victim.short_descr = "a dragon";
   victim.in_room = &room;
   room.first_person = &killer;

   killer.next_in_room = NULL;
   killer.pcdata = &pcdata;
   killer.act = 0;

   g_send_count = 0;
   spec_death_handler(&victim, &killer);

   assert(pcdata.reincarnation_data[REINCARNATION_POINTS] == 0);
   assert(g_send_count == 0);
}

static void test_death_handler_skips_npcs(void)
{
   CHAR_DATA victim;
   CHAR_DATA npc;
   CHAR_DATA killer;
   ROOM_INDEX_DATA room;

   clear_character(&victim);
   clear_character(&npc);
   clear_character(&killer);
   memset(&room, 0, sizeof(room));

   victim.spec_fun = spec_pyramid_black_sun_shard;
   victim.short_descr = "the Black Sun Shard";
   victim.in_room = &room;
   room.first_person = &npc;

   npc.next_in_room = NULL;
   npc.act = ACT_IS_NPC;
   npc.pcdata = NULL;

   g_send_count = 0;
   spec_death_handler(&victim, &killer);

   assert(g_send_count == 0);
}

int main(void)
{
   test_no_chance_when_master_full_or_invalid();
   test_chance_increases_as_master_hp_drops();
   test_chance_respects_bounds();
   test_all_summon_specs_cast_in_combat();
   test_superboss_index_identifies_superbosses();
   test_death_handler_awards_reincarnation_point();
   test_death_handler_no_double_award();
   test_death_handler_ignores_non_superboss();
   test_death_handler_skips_npcs();

   puts("test_special: all tests passed");
   return 0;
}
