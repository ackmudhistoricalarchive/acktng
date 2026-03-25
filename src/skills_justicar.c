#include "globals.h"
#include "magic.h"
#include "sentinel.h"
#include "skills.h"

/*
 * Helper: Check if character has any Sentinel-lineage class.
 */
static int get_justicar_level(CHAR_DATA *ch)
{
   return char_class_level(ch, CLASS_JUS);
}

/*
 * do_formal_sentencing: Mark target for amplified verdict damage.
 * Requires 3+ testimony, does not consume testimony.
 */
void do_formal_sentencing(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_formal_sentencing))
   {
      send_to_char("You don't know how to formally sentence a target.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   if (ch->testimony < 3 || ch->testimony_target != victim)
   {
      send_to_char("You need at least 3 testimony marks against this target.\n\r", ch);
      return;
   }

   if (is_affected(victim, gsn_formal_sentencing))
   {
      send_to_char("They are already formally sentenced.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_formal_sentencing))
      return;

   WAIT_STATE(ch, skill_table[gsn_formal_sentencing].beats);
   raise_skill(ch, gsn_formal_sentencing);

   af.type = gsn_formal_sentencing;
   af.duration = 6;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = 0;
   affect_to_char(victim, &af);

   act("@@eYou formally sentence $N. All verdicts against them carry amplified weight.@@N", ch,
       NULL, victim, TO_CHAR);
   act("@@e$n formally sentences you! You feel the weight of impending judgment.@@N", ch, NULL,
       victim, TO_VICT);
   act("@@e$n formally sentences $N.@@N", ch, NULL, victim, TO_NOTVICT);
}

/*
 * do_executioners_strike: WIS-scaled melee attack, bonus vs. sentenced targets.
 */
void do_executioners_strike(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   int dam;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_executioners_strike))
   {
      send_to_char("You don't know the executioner's strike.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_executioners_strike))
      return;

   WAIT_STATE(ch, skill_table[gsn_executioners_strike].beats);
   raise_skill(ch, gsn_executioners_strike);

   dam = ch->level * 3 + get_curr_wis(ch) * 2;

   /* Bonus damage against formally sentenced targets */
   if (is_affected(victim, gsn_formal_sentencing))
      dam = dam * 3 / 2;

   act("@@yYou deliver an executioner's strike against $N!@@N", ch, NULL, victim, TO_CHAR);
   act("@@y$n delivers an executioner's strike against you!@@N", ch, NULL, victim, TO_VICT);
   act("@@y$n delivers an executioner's strike against $N!@@N", ch, NULL, victim, TO_NOTVICT);

   sp_damage(NULL, ch, victim, dam, ELE_PHYSICAL, gsn_executioners_strike, TRUE);
}

/*
 * do_writ_of_execution: Consume all testimony (min 5) for devastating strike.
 * Bypasses armor for 1 round. 30-round cooldown.
 */
void do_writ_of_execution(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   int marks, dam, wis_bonus;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_writ_of_execution))
   {
      send_to_char("You don't know how to issue a writ of execution.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   marks = ch->testimony;
   if (marks < 5 || ch->testimony_target != victim)
   {
      send_to_char("You need at least 5 testimony marks against this target.\n\r", ch);
      return;
   }

   if (ch->cooldown[gsn_writ_of_execution] > 0)
   {
      send_to_char("You are not ready to issue another writ.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_writ_of_execution))
      return;

   WAIT_STATE(ch, skill_table[gsn_writ_of_execution].beats);
   raise_skill(ch, gsn_writ_of_execution);

   /* Calculate verdict damage * 1.5 */
   wis_bonus = get_curr_wis(ch) * ch->level / 80;
   dam = number_range(ch->level * 2, ch->level * 4);

   /* Scale by mark count (use verdict tier scaling) */
   if (marks <= 2)
      dam = dam * 3 / 2;
   else if (marks <= 4)
      dam = dam * 5 / 2;
   else if (marks <= 6)
      dam = dam * 4;
   else if (marks <= 8)
      dam = dam * 6;
   else
      dam = dam * 9;

   dam += wis_bonus;
   dam = dam * 3 / 2; /* Writ multiplier */

   /* Formal sentencing bonus */
   if (is_affected(victim, gsn_formal_sentencing))
      dam = dam * 5 / 4;

   act("@@R$n issues a WRIT OF EXECUTION against $N!@@N", ch, NULL, victim, TO_NOTVICT);
   act("@@RYou issue a WRIT OF EXECUTION against $N! The final instrument is delivered!@@N", ch,
       NULL, victim, TO_CHAR);
   act("@@R$n issues a WRIT OF EXECUTION against you!@@N", ch, NULL, victim, TO_VICT);

   sp_damage(NULL, ch, victim, dam, ELE_PHYSICAL, gsn_writ_of_execution, TRUE);

   /* Consume all testimony and set cooldown */
   int retain = can_use_skill(ch, gsn_second_hearing) ? 2 : 0;
   if (can_use_skill(ch, gsn_full_tribunal))
      retain = 3;
   ch->testimony = UMIN(retain, MAX_TESTIMONY);
   ch->testimony_cooldown = VERDICT_COOLDOWN_ROUNDS;
   ch->cooldown[gsn_writ_of_execution] = 30;
}

/*
 * do_read_weakness: Self-buff for +hit/+dam vs. current target for 5 rounds.
 */
void do_read_weakness(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_read_weakness))
   {
      send_to_char("You don't know how to read weaknesses.\n\r", ch);
      return;
   }

   if (ch->fighting == NULL)
   {
      send_to_char("You must be fighting to read weaknesses.\n\r", ch);
      return;
   }

   if (is_affected(ch, gsn_read_weakness))
   {
      send_to_char("You are already reading your target's weaknesses.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_read_weakness))
      return;

   WAIT_STATE(ch, skill_table[gsn_read_weakness].beats);
   raise_skill(ch, gsn_read_weakness);

   af.type = gsn_read_weakness;
   af.duration = 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_HITROLL;
   af.modifier = get_curr_wis(ch) / 4;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   af.location = APPLY_DAMROLL;
   af.modifier = get_curr_wis(ch) / 3;
   affect_to_char(ch, &af);

   act("@@yYou read $N's structural weaknesses with judicial precision.@@N", ch, NULL, ch->fighting,
       TO_CHAR);
   act("@@y$n's eyes narrow, reading your every weakness.@@N", ch, NULL, ch->fighting, TO_VICT);
   act("@@y$n reads $N's weaknesses with cold precision.@@N", ch, NULL, ch->fighting, TO_NOTVICT);
}
