#include "globals.h"
#include "magic.h"
#include "sentinel.h"
#include "skills.h"

/*
 * Sentinel testimony management functions.
 */

bool is_sentinel_class(CHAR_DATA *ch)
{
   return char_class_level(ch, CLASS_SEN) > 0 || char_class_level(ch, CLASS_JUS) > 0 ||
          char_class_level(ch, CLASS_ARB) > 0 || char_class_level(ch, CLASS_INQ) > 0;
}

int get_sentinel_level(CHAR_DATA *ch)
{
   int best = char_class_level(ch, CLASS_SEN);
   int jus = char_class_level(ch, CLASS_JUS);
   int arb = char_class_level(ch, CLASS_ARB);
   int inq = char_class_level(ch, CLASS_INQ);
   if (jus > best)
      best = jus;
   if (arb > best)
      best = arb;
   if (inq > best)
      best = inq;
   return best;
}

void set_testimony_target(CHAR_DATA *ch, CHAR_DATA *victim)
{
   ch->testimony_target = victim;
   ch->testimony_combat_rounds = 0;
}

void reset_testimony(CHAR_DATA *ch)
{
   ch->testimony = 0;
   ch->testimony_target = NULL;
   ch->testimony_cooldown = 0;
   ch->testimony_combat_rounds = 0;
}

void add_testimony(CHAR_DATA *ch, int amount)
{
   char buf[MAX_STRING_LENGTH];

   if (ch->testimony_cooldown > 0)
      return;

   /* Seal of the Tribunal: +2 base on avoidance */
   if (is_affected(ch, gsn_seal_of_the_tribunal) && amount == 1)
      amount = 2;

   /* Apply ninth descent doubling */
   if (is_affected(ch, gsn_ninth_descent))
      amount *= 2;

   /* Inquisition: testimony gain doubled (stacks with ninth descent) */
   if (is_affected(ch, gsn_inquisition))
      amount *= 2;

   ch->testimony = UMIN(ch->testimony + amount, MAX_TESTIMONY);

   if (amount > 0 && ch->testimony > 0)
   {
      sprintf(buf, "@@y[Testimony: %d/9]@@N\n\r", ch->testimony);
      send_to_char(buf, ch);
   }
}

/*
 * do_verdict: Consume all testimony marks for a scaled melee strike.
 */
void do_verdict(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   int marks, dam, wis_bonus;
   char buf[MAX_STRING_LENGTH];

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_verdict))
   {
      send_to_char("You don't know how to deliver a verdict.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   marks = ch->testimony;
   if (marks < 1)
   {
      send_to_char("You have no testimony marks to spend.\n\r", ch);
      return;
   }

   if (ch->testimony_target != victim)
   {
      send_to_char("Your testimony marks are against a different target.\n\r", ch);
      return;
   }

   WAIT_STATE(ch, skill_table[gsn_verdict].beats);
   raise_skill(ch, gsn_verdict);

   /* Calculate WIS-scaled damage bonus */
   wis_bonus = get_curr_wis(ch) * ch->level / 80;

   /* Base damage from a normal hit */
   dam = number_range(ch->level * 2, ch->level * 4);

   /* Severity escalation: reduce thresholds by 1 */
   int rebuke_max = 2;
   int censure_max = 4;
   int binding_max = 6;
   int sealing_max = 8;
   if (can_use_skill(ch, gsn_severity_escalation))
   {
      rebuke_max = 1;
      censure_max = 3;
      binding_max = 5;
      sealing_max = 7;
   }

   /* Scale by verdict tier */
   if (marks <= rebuke_max)
   {
      /* Rebuke: 1.5x */
      dam = dam * 3 / 2 + wis_bonus;
      sprintf(buf, "@@eYou deliver a @@RREBUKE@@e against $N!@@N [%d marks]", marks);
      act(buf, ch, NULL, victim, TO_CHAR);
      act("@@e$n delivers a stern rebuke against you!@@N", ch, NULL, victim, TO_VICT);
      act("@@e$n delivers a rebuke against $N!@@N", ch, NULL, victim, TO_NOTVICT);
   }
   else if (marks <= censure_max)
   {
      AFFECT_DATA af;
      /* Censure: 2.5x + HR debuff */
      dam = dam * 5 / 2 + wis_bonus;
      sprintf(buf, "@@eYou deliver a @@RCENSURE@@e against $N!@@N [%d marks]", marks);
      act(buf, ch, NULL, victim, TO_CHAR);
      act("@@e$n delivers a formal censure against you!@@N", ch, NULL, victim, TO_VICT);
      act("@@e$n delivers a censure against $N!@@N", ch, NULL, victim, TO_NOTVICT);

      af.type = gsn_verdict;
      af.duration = 3;
      af.duration_type = DURATION_ROUND;
      af.location = APPLY_HITROLL;
      af.modifier = -2;
      af.bitvector = 0;
      affect_to_char(victim, &af);
   }
   else if (marks <= binding_max)
   {
      AFFECT_DATA af;
      /* Binding Verdict: 4x + speed debuff */
      dam = dam * 4 + wis_bonus;
      sprintf(buf, "@@eYou deliver a @@RBINDING VERDICT@@e against $N!@@N [%d marks]", marks);
      act(buf, ch, NULL, victim, TO_CHAR);
      act("@@e$n delivers a binding verdict against you!@@N", ch, NULL, victim, TO_VICT);
      act("@@e$n delivers a binding verdict against $N!@@N", ch, NULL, victim, TO_NOTVICT);

      af.type = gsn_verdict;
      af.duration = 4;
      af.duration_type = DURATION_ROUND;
      af.location = APPLY_HITROLL;
      af.modifier = -4;
      af.bitvector = 0;
      affect_to_char(victim, &af);

      af.location = APPLY_DEX;
      af.modifier = -3;
      affect_to_char(victim, &af);
   }
   else if (marks <= sealing_max)
   {
      AFFECT_DATA af;
      /* Sealing Verdict: 6x + silence + all above */
      dam = dam * 6 + wis_bonus;
      sprintf(buf, "@@eYou deliver a @@RSEALING VERDICT@@e against $N!@@N [%d marks]", marks);
      act(buf, ch, NULL, victim, TO_CHAR);
      act("@@e$n delivers a sealing verdict against you! Your options are sealed!@@N", ch, NULL,
          victim, TO_VICT);
      act("@@e$n delivers a sealing verdict against $N!@@N", ch, NULL, victim, TO_NOTVICT);

      af.type = gsn_verdict;
      af.duration = 4;
      af.duration_type = DURATION_ROUND;
      af.location = APPLY_HITROLL;
      af.modifier = -4;
      af.bitvector = 0;
      affect_to_char(victim, &af);

      af.location = APPLY_DEX;
      af.modifier = -3;
      affect_to_char(victim, &af);

      /* Silence effect for 2 rounds */
      af.duration = 2;
      af.location = APPLY_SAVING_SPELL;
      af.modifier = 10;
      affect_to_char(victim, &af);
   }
   else
   {
      AFFECT_DATA af;
      /* Final Verdict: 9x + stun + everything */
      dam = dam * 9 + wis_bonus;
      act("@@R$n delivers the FINAL VERDICT against $N!@@N", ch, NULL, victim, TO_NOTVICT);
      act("@@RYou deliver the @@eFINAL VERDICT@@R against $N! The judicial process is complete!@@N",
          ch, NULL, victim, TO_CHAR);
      act("@@R$n delivers the FINAL VERDICT against you!@@N", ch, NULL, victim, TO_VICT);

      af.type = gsn_verdict;
      af.duration = 4;
      af.duration_type = DURATION_ROUND;
      af.location = APPLY_HITROLL;
      af.modifier = -6;
      af.bitvector = 0;
      affect_to_char(victim, &af);

      af.location = APPLY_DEX;
      af.modifier = -4;
      affect_to_char(victim, &af);

      /* Stun for 1 round */
      WAIT_STATE(victim, 24);
   }

   /* Formal sentencing bonus: +25% verdict damage */
   if (is_affected(victim, gsn_formal_sentencing))
      dam = dam * 5 / 4;

   /* Inquisition bonus: +15% damage from all sources */
   if (is_affected(victim, gsn_inquisition))
      dam = dam * 115 / 100;

   /* Deal the damage */
   sp_damage(NULL, ch, victim, dam, ELE_PHYSICAL, gsn_verdict, TRUE);

   /* Retain testimony based on passives */
   int retain = 0;
   if (can_use_skill(ch, gsn_full_tribunal))
      retain = 3;
   else if (can_use_skill(ch, gsn_second_hearing))
      retain = 2;
   ch->testimony = UMIN(retain, MAX_TESTIMONY);

   /* Seal of the Tribunal: reduced cooldown */
   int cd = VERDICT_COOLDOWN_ROUNDS;
   if (is_affected(ch, gsn_seal_of_the_tribunal))
      cd = 1;
   ch->testimony_cooldown = cd;
}

/*
 * do_read_opponent: Instantly grants +2 testimony marks.
 */
void do_read_opponent(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_read_opponent))
   {
      send_to_char("You don't know how to read opponents.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   if (ch->cooldown[gsn_read_opponent] > 0)
   {
      send_to_char("You are not ready to read your opponent again yet.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_read_opponent))
      return;

   WAIT_STATE(ch, skill_table[gsn_read_opponent].beats);
   raise_skill(ch, gsn_read_opponent);

   /* Set or maintain testimony target */
   if (ch->testimony_target != victim)
   {
      ch->testimony = 0;
      set_testimony_target(ch, victim);
   }

   /* Inevitable verdict: +3 instead of +2 */
   int gain = can_use_skill(ch, gsn_inevitable_verdict) ? 3 : 2;
   add_testimony(ch, gain);

   act("@@yYou study $N's stance, breath, and movement, building your case.@@N", ch, NULL, victim,
       TO_CHAR);
   act("@@y$n studies you with unnerving judicial precision.@@N", ch, NULL, victim, TO_VICT);
   act("@@y$n studies $N with careful precision.@@N", ch, NULL, victim, TO_NOTVICT);

   /* Cooldown: 8 rounds, reduced by 1 at WIS 22 */
   int cd = 8;
   if (get_curr_wis(ch) >= 22)
      cd = 7;
   ch->cooldown[gsn_read_opponent] = cd;
}

/*
 * do_binding_strike: Melee attack + dodge/parry debuff.
 */
void do_binding_strike(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   int dam;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_binding_strike))
   {
      send_to_char("You don't know how to use binding strike.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_binding_strike))
      return;

   WAIT_STATE(ch, skill_table[gsn_binding_strike].beats);
   raise_skill(ch, gsn_binding_strike);

   /* 1.5x weapon damage */
   dam = number_range(ch->level * 2, ch->level * 4);
   dam = dam * 3 / 2;
   dam += get_curr_wis(ch) * get_sentinel_level(ch) / 100;

   if (!is_affected(victim, gsn_binding_strike))
   {
      af.type = gsn_binding_strike;
      af.duration = 3;
      af.duration_type = DURATION_ROUND;
      af.location = APPLY_DEX;
      af.modifier = -2;
      af.bitvector = 0;
      affect_to_char(victim, &af);
   }

   act("@@yYou strike $N's structural weakness with binding precision!@@N", ch, NULL, victim,
       TO_CHAR);
   act("@@y$n strikes your weak point with binding precision!@@N", ch, NULL, victim, TO_VICT);
   act("@@y$n strikes $N with binding precision!@@N", ch, NULL, victim, TO_NOTVICT);

   sp_damage(NULL, ch, victim, dam, ELE_PHYSICAL, gsn_binding_strike, TRUE);
}

/*
 * do_ninth_descent: Buff that doubles testimony gain for 4 rounds.
 */
void do_ninth_descent(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_ninth_descent))
   {
      send_to_char("You don't know the ninth descent technique.\n\r", ch);
      return;
   }

   if (ch->fighting == NULL)
   {
      send_to_char("You must be fighting to enter the ninth descent.\n\r", ch);
      return;
   }

   if (is_affected(ch, gsn_ninth_descent))
   {
      send_to_char("You are already in a state of heightened awareness.\n\r", ch);
      return;
   }

   if (ch->cooldown[gsn_ninth_descent] > 0)
   {
      send_to_char("You are not ready to enter the ninth descent again.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_ninth_descent))
      return;

   WAIT_STATE(ch, skill_table[gsn_ninth_descent].beats);
   raise_skill(ch, gsn_ninth_descent);

   af.type = gsn_ninth_descent;
   af.duration = 4;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   ch->cooldown[gsn_ninth_descent] = 20;

   send_to_char(
       "@@yYou descend into the silence-court's deepest focus. Your awareness sharpens.@@N\n\r",
       ch);
   act("@@y$n's eyes narrow with intense judicial focus.@@N", ch, NULL, NULL, TO_ROOM);
}

/*
 * do_condemn: WIS-scaled saves debuff, requires 3+ testimony.
 */
void do_condemn(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_condemn))
   {
      send_to_char("You don't know how to formally condemn a target.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   if (ch->testimony < 3 || ch->testimony_target != victim)
   {
      send_to_char("You need at least 3 testimony marks against this target to condemn.\n\r", ch);
      return;
   }

   if (is_affected(victim, gsn_condemn))
   {
      send_to_char("They are already condemned.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_condemn))
      return;

   WAIT_STATE(ch, skill_table[gsn_condemn].beats);
   raise_skill(ch, gsn_condemn);

   af.type = gsn_condemn;
   af.duration = 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_SAVING_SPELL;
   af.modifier = get_curr_wis(ch) / 3;
   af.bitvector = 0;
   affect_to_char(victim, &af);

   af.location = APPLY_AC;
   af.modifier = get_curr_wis(ch) / 2;
   affect_to_char(victim, &af);

   act("@@eYou formally condemn $N! The weight of judgment falls upon them.@@N", ch, NULL, victim,
       TO_CHAR);
   act("@@e$n formally condemns you! You feel the weight of judgment.@@N", ch, NULL, victim,
       TO_VICT);
   act("@@e$n formally condemns $N!@@N", ch, NULL, victim, TO_NOTVICT);
}

/*
 * do_seal_testimony: Anti-disruption buff for 3 rounds.
 */
void do_seal_testimony(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_seal_testimony))
   {
      send_to_char("You don't know how to seal testimony.\n\r", ch);
      return;
   }

   if (ch->fighting == NULL)
   {
      send_to_char("You must be fighting to seal your testimony.\n\r", ch);
      return;
   }

   if (ch->testimony < 1)
   {
      send_to_char("You have no testimony marks to seal.\n\r", ch);
      return;
   }

   if (is_affected(ch, gsn_seal_testimony))
   {
      send_to_char("Your testimony is already sealed.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_seal_testimony))
      return;

   WAIT_STATE(ch, skill_table[gsn_seal_testimony].beats);
   raise_skill(ch, gsn_seal_testimony);

   af.type = gsn_seal_testimony;
   af.duration = 3;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   send_to_char("@@yYou seal your accumulated testimony against tampering.@@N\n\r", ch);
   act("@@y$n's focus hardens, sealing away accumulated observations.@@N", ch, NULL, NULL, TO_ROOM);
}
