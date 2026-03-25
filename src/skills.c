#include "globals.h"
#include "magic.h"

bool can_hit_skill(CHAR_DATA *ch, CHAR_DATA *victim, int gsn);
bool subtract_energy_cost(CHAR_DATA *ch, int gsn);
int get_energy_cost(CHAR_DATA *ch, int gsn);
void trip(CHAR_DATA *ch, CHAR_DATA *victim);

void trip(CHAR_DATA *ch, CHAR_DATA *victim)
{
   if (victim->wait == 0)
   {
      int chance;

      chance = IS_NPC(victim) ? IS_SET(victim->skills, MOB_NOTRIP) ? 75 : 0
                              : victim->pcdata->learned[gsn_notrip];

      /*
       * Check for no-trip
       */
      if (number_percent() < chance)
      {
         act("You sidestep $n's attempt to trip you!", ch, NULL, victim, TO_VICT);
         act("$N sidesteps your attempt to trip $M!", ch, NULL, victim, TO_CHAR);
         act("$N sidesteps $n's attempt to trip $m!", ch, NULL, victim, TO_NOTVICT);
         return;
      }

      act("$n trips you and you go down!", ch, NULL, victim, TO_VICT);
      act("You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR);
      act("$n trips $N and $N goes down!", ch, NULL, victim, TO_NOTVICT);

      WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
      WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
      victim->position = POS_RESTING;
   }

   return;
}

bool can_hit_skill(CHAR_DATA *ch, CHAR_DATA *victim, int gsn)
{
   int chance = 0;

   if (gsn == gsn_poison_quinine || gsn == gsn_poison_arsenic || gsn == gsn_poison_nightshade)
   {
      // Can never hit immune poison with poison
      if (IS_SET(race_table[victim->race].race_flags, RACE_MOD_IMMUNE_POISON))
         return FALSE;
   }

   if (!IS_AWAKE(victim))
      chance += 75;

   if (IS_AFFECTED(ch, AFF_SNEAK) || item_has_apply(ch, ITEM_APPLY_SNEAK))
      chance += 10;

   if (!can_see(victim, ch))
      chance += 20;

   if (!can_see(ch, victim))
      chance -= 30;

   if (IS_NPC(ch))
      chance -= 20;

   if (!skill_success(ch, victim, gsn, chance))
      return FALSE;

   return TRUE;
}

int get_energy_cost(CHAR_DATA *ch, int gsn)
{
   int base_cost = 50;

   if (skill_table[gsn].min_mana == 0)
      return 0;

   if (skill_table[gsn].min_mana > base_cost)
      base_cost = skill_table[gsn].min_mana;

   base_cost -= base_cost * (get_curr_str(ch) + get_curr_dex(ch)) / 100;

   return base_cost;
}

bool subtract_energy_cost(CHAR_DATA *ch, int gsn)
{
   int cost = get_energy_cost(ch, gsn);

   if (ch == NULL)
      return FALSE;

   if (gsn < 1 || gsn >= TYPE_HIT)
      return FALSE;

   if (cost > ch->move)
   {
      send_to_char("You don't have enough move to do this.\n\r", ch);
      return FALSE;
   }

   ch->move -= cost;

   return TRUE;
}

int skill_get_tier(int sn)
{
   int i;
   if (sn < 0 || sn >= MAX_SKILL)
      return 1;
   for (i = 0; i < MAX_TOTAL_CLASS; i++)
      if (IS_ADEPT_CLASS(i) && skill_table[sn].skill_level[i] >= 0)
         return 3;
   for (i = 0; i < MAX_TOTAL_CLASS; i++)
      if (IS_REMORT_CLASS(i) && skill_table[sn].skill_level[i] >= 0)
         return 2;
   return 1;
}

bool can_use_skill_message(CHAR_DATA *ch, int gsn)
{
   char buf[MSL];
   if (ch->cooldown[gsn] > 0)
   {
      sprintf(buf, "That skill is still on cooldown for %d.\n\r", ch->cooldown[gsn]);
      send_to_char(buf, ch);
      return FALSE;
   }

   if (!can_use_skill(ch, gsn))
   {
      send_to_char("You don't know how to do that\n\r", ch);
      return FALSE;
   }

   return TRUE;
}

bool can_use_skill_by_name_message(CHAR_DATA *ch, char *skill)
{
   char buf[MSL];
   if (ch->cooldown[skill_lookup(skill)] > 0)
   {
      sprintf(buf, "That skill is still on cooldown for %d.\n\r",
              ch->cooldown[skill_lookup(skill)]);
      send_to_char(buf, ch);
      return FALSE;
   }

   if (!can_use_skill_by_name(ch, skill))
   {
      send_to_char("You don't know how to do that\n\r", ch);
      return FALSE;
   }

   return TRUE;
}

bool can_use_skill_by_name(CHAR_DATA *ch, char *skill)
{
   int gsn = skill_lookup(skill);

   return can_use_skill(ch, gsn);
}

static bool skill_requirement_is_usable(int required_level)
{
   return required_level >= 0;
}

/*
 * Reincarnation unlock rules: a skill becomes usable when the combined
 * reincarnation count across class1 (and optionally class2) reaches min_total.
 * class2 == -1 means only class1 is checked.
 */
struct reinc_unlock_rule
{
   sh_int *gsn_ptr;
   int class1;
   int class2;
   int min_total;
};

static const struct reinc_unlock_rule reinc_rules[] = {
    {&gsn_enhanced_damage, CLASS_WAR, -1, 1},
    {&gsn_enhanced_critical, CLASS_CIP, -1, 1},
    {&gsn_counter, CLASS_PUG, -1, 1},
    {&gsn_potency, CLASS_MAG, -1, 1},
    {&gsn_spell_critical, CLASS_PSI, -1, 1},
    {&gsn_spell_critical_damage, CLASS_CLE, -1, 1},
    {&gsn_dualwield, CLASS_ASS, CLASS_WLK, 20},
    {&gsn_two_handed, CLASS_KNI, CLASS_SWO, 20},
    {&gsn_two_handed, CLASS_PAL, CLASS_PRI, 20},
    {&gsn_equip_buckler, CLASS_KNI, CLASS_SWO, 20},
    {&gsn_equip_buckler, CLASS_PAL, CLASS_PRI, 20},
    {&gsn_equip_fist, CLASS_BRA, CLASS_MON, 20},
    {&gsn_dual_fist, CLASS_BRA, CLASS_MON, 20},
    {&gsn_equip_wand, CLASS_WIZ, CLASS_SOR, 20},
    {&gsn_equip_wand, CLASS_NEC, CLASS_EGO, 20},
    {&gsn_equip_wand, CLASS_PRI, CLASS_PAL, 20},
    {&gsn_substrate_piercing, CLASS_DRU, -1, 1},
};

bool can_use_skill(CHAR_DATA *ch, int gsn)
{
   int i;

   if (gsn == -1 || gsn >= MAX_SKILL)
      return FALSE;

   if (ch->cooldown[gsn] > 0)
      return FALSE;

   if (IS_NPC(ch))
      return TRUE;

   /* Check reincarnation-based unlocks via data table */
   for (i = 0; i < (int)(sizeof reinc_rules / sizeof reinc_rules[0]); i++)
   {
      const struct reinc_unlock_rule *rule = &reinc_rules[i];
      if (gsn != *rule->gsn_ptr)
         continue;
      int total = ch->pcdata->reincarnations[rule->class1];
      if (rule->class2 >= 0)
         total += ch->pcdata->reincarnations[rule->class2];
      if (total >= rule->min_total)
         return TRUE;
   }

   /* Check all 24 class slots - each slot has the required level if valid */
   for (i = 0; i < MAX_TOTAL_CLASS; i++)
   {
      int required_level = skill_table[gsn].skill_level[i];

      if (!skill_requirement_is_usable(required_level))
         continue;

      if (char_class_level(ch, i) >= required_level)
         return TRUE;
      if (IS_MORTAL_CLASS(i) && ch->pcdata->reincarnations[i] >= 20)
         return TRUE;
   }

   {
      char race_skill[MSL];
      char *race_skill_list;
      race_skill_list = race_table[ch->race].skill;
      for (;;)
      {
         race_skill_list = one_argument(race_skill_list, race_skill);

         if (skill_lookup(race_skill) == gsn)
            return TRUE;

         if (strlen(race_skill) < 1)
            break;
      }
   }

   /* Check public society skills */
   if (can_use_pub_society_skill(ch, gsn))
      return TRUE;

   return FALSE;
}

bool raise_skill(CHAR_DATA *ch, int gsn)
{
   static const struct
   {
      int level;
      const char *message;
   } level_messages[] = {
       {LEVEL_ONE, "Congratulations! You have raised %s to level one!\n\r"},
       {LEVEL_TWO, "Congratulations! You have raised %s to level two!\n\r"},
       {LEVEL_THREE, "Congratulations! You have raised %s to level three!\n\r"},
       {LEVEL_FOUR, "Congratulations! You have raised %s to level four!\n\r"},
       {LEVEL_FIVE, "Congratulations! You have raised %s to level five!\n\r"},
       {LEVEL_MASTER, "Congratulations! You have mastered %s!\n\r"},
       {LEVEL_GM, "Congratulations! You have became a grandmaster of %s!\n\r"},
   };
   char buf[MSL];
   int i;

   if (gsn == -1 || gsn >= MAX_SKILL)
      return FALSE;

   if (IS_NPC(ch))
      return TRUE;

   if (!can_use_skill(ch, gsn))
      return FALSE;

   ch->pcdata->learned[gsn]++;

   for (i = 0; i < (int)(sizeof level_messages / sizeof level_messages[0]); i++)
   {
      if (ch->pcdata->learned[gsn] == level_messages[i].level)
      {
         sprintf(buf, level_messages[i].message, skill_table[gsn].name);
         send_to_char(buf, ch);
      }
   }

   return TRUE;
}

bool skill_success(CHAR_DATA *ch, CHAR_DATA *victim, int gsn, int bonus)
{
   static const struct
   {
      int threshold;
      int bonus;
   } skill_bonuses[] = {
       {LEVEL_ONE, 5},  {LEVEL_TWO, 5},    {LEVEL_THREE, 5}, {LEVEL_FOUR, 5},
       {LEVEL_FIVE, 5}, {LEVEL_MASTER, 1}, {LEVEL_GM, 1},
   };
   int chance = 70;
   int i;

   if (!IS_NPC(ch))
   {
      for (i = 0; i < (int)(sizeof skill_bonuses / sizeof skill_bonuses[0]); i++)
         if (ch->pcdata->learned[gsn] > skill_bonuses[i].threshold)
            chance += skill_bonuses[i].bonus;
   }

   if (victim != NULL)
      chance += (get_psuedo_level(ch) - get_psuedo_level(victim)) / 2;

   chance += bonus;

   return number_percent() < chance;
}

char *get_dt_name(int sn)
{
   static char *const attack_table[] = {
       "hit",        "slice",        "stab",           "slash",      "whip",         "claw",
       "blast",      "pound",        "crush",          "rend",       "bite",         "pierce",
       "drain",      "sear",         "tail whip",      "head punch", "high kick",    "vital kick",
       "head bash",  "side kick",    "spinning elbow", "body punch", "low kick",     "foot stomp",
       "knee smash", "kidney punch", "arm twist",      "uppercut",   "rabbit punch", "foot sweep"};
   const size_t attack_count = sizeof(attack_table) / sizeof(attack_table[0]);

   if (sn < MAX_SKILL && sn > 0)
      return skill_table[sn].name;

   if (sn >= TYPE_HIT && (size_t)(sn - TYPE_HIT) < attack_count)
      return attack_table[sn - TYPE_HIT];

   return "bugged damage type";
}
