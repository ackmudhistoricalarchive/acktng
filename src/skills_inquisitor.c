#include "globals.h"
#include "magic.h"
#include "sentinel.h"
#include "skills.h"

/*
 * do_inquisition: Open formal inquisition against target.
 * +15% damage from all sources, reduced saves, doubled testimony gain.
 * Requires 5+ testimony, consumes 3 marks. 40-round cooldown.
 */
void do_inquisition(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_inquisition))
   {
      send_to_char("You don't know how to open a formal inquisition.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   if (ch->testimony < 5 || ch->testimony_target != victim)
   {
      send_to_char("You need at least 5 testimony marks against this target.\n\r", ch);
      return;
   }

   if (is_affected(victim, gsn_inquisition))
   {
      send_to_char("An inquisition is already active against this target.\n\r", ch);
      return;
   }

   if (ch->cooldown[gsn_inquisition] > 0)
   {
      send_to_char("You are not ready to open another inquisition.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_inquisition))
      return;

   WAIT_STATE(ch, skill_table[gsn_inquisition].beats);
   raise_skill(ch, gsn_inquisition);

   /* Consume 3 testimony marks */
   ch->testimony = UMAX(ch->testimony - 3, 0);

   /* Apply inquisition debuff to victim */
   af.type = gsn_inquisition;
   af.duration = 8;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_SAVING_SPELL;
   af.modifier = get_curr_wis(ch) / 3;
   af.bitvector = 0;
   affect_to_char(victim, &af);

   /* Apply testimony doubling buff to self */
   af.type = gsn_inquisition;
   af.duration = 8;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   ch->cooldown[gsn_inquisition] = 40;

   act("@@R$n opens a formal INQUISITION against $N!@@N", ch, NULL, victim, TO_NOTVICT);
   act("@@RYou open a formal INQUISITION against $N! The full weight of the Tribunal bears "
       "down.@@N",
       ch, NULL, victim, TO_CHAR);
   act("@@R$n opens a formal INQUISITION against you! You feel the Tribunal's scrutiny.@@N", ch,
       NULL, victim, TO_VICT);
}

/*
 * do_absolute_verdict: Enhanced Final Verdict at 9 testimony.
 * 12x damage, 2-round stun, strip all buffs. 50-round cooldown.
 */
void do_absolute_verdict(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   int dam, wis_bonus;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_absolute_verdict))
   {
      send_to_char("You don't know how to deliver an absolute verdict.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   if (ch->testimony < 9 || ch->testimony_target != victim)
   {
      send_to_char("You need exactly 9 testimony marks against this target.\n\r", ch);
      return;
   }

   if (ch->cooldown[gsn_absolute_verdict] > 0)
   {
      send_to_char("You are not ready to deliver another absolute verdict.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_absolute_verdict))
      return;

   WAIT_STATE(ch, skill_table[gsn_absolute_verdict].beats);
   raise_skill(ch, gsn_absolute_verdict);

   /* 12x base damage + WIS bonus */
   wis_bonus = get_curr_wis(ch) * ch->level / 80;
   dam = number_range(ch->level * 2, ch->level * 4);
   dam = dam * 12 + wis_bonus;

   /* Formal sentencing bonus */
   if (is_affected(victim, gsn_formal_sentencing))
      dam = dam * 5 / 4;

   /* Inquisition bonus */
   if (is_affected(victim, gsn_inquisition))
      dam = dam * 115 / 100;

   act("@@R$n delivers the ABSOLUTE VERDICT against $N! No appeal. No reprieve.@@N", ch, NULL,
       victim, TO_NOTVICT);
   act("@@RYou deliver the @@eABSOLUTE VERDICT@@R against $N! The irrevocable sentence!@@N", ch,
       NULL, victim, TO_CHAR);
   act("@@R$n delivers the ABSOLUTE VERDICT against you! There is no appeal!@@N", ch, NULL, victim,
       TO_VICT);

   /* Strip all beneficial affects from victim */
   while (victim->first_affect != NULL)
   {
      AFFECT_DATA *paf = victim->first_affect;
      if (paf->modifier > 0 || paf->location == APPLY_NONE)
         affect_remove(victim, paf);
      else
         break;
   }

   /* Apply debuffs */
   af.type = gsn_absolute_verdict;
   af.duration = 4;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_HITROLL;
   af.modifier = -6;
   af.bitvector = 0;
   affect_to_char(victim, &af);

   af.location = APPLY_DEX;
   af.modifier = -4;
   affect_to_char(victim, &af);

   /* 2-round stun */
   WAIT_STATE(victim, 48);

   sp_damage(NULL, ch, victim, dam, ELE_PHYSICAL, gsn_absolute_verdict, TRUE);

   /* Consume testimony */
   int retain = can_use_skill(ch, gsn_full_tribunal) ? 3 : 0;
   if (!retain && can_use_skill(ch, gsn_second_hearing))
      retain = 2;
   ch->testimony = UMIN(retain, MAX_TESTIMONY);
   ch->testimony_cooldown = VERDICT_COOLDOWN_ROUNDS;
   ch->cooldown[gsn_absolute_verdict] = 50;
}

/*
 * do_sovereign_authority: Party buff for hit/dam/saves.
 * 30-round cooldown.
 */
void do_sovereign_authority(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *gch;
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_sovereign_authority))
   {
      send_to_char("You don't know how to invoke sovereign authority.\n\r", ch);
      return;
   }

   if (ch->fighting == NULL)
   {
      send_to_char("You must be fighting to invoke sovereign authority.\n\r", ch);
      return;
   }

   if (ch->cooldown[gsn_sovereign_authority] > 0)
   {
      send_to_char("You are not ready to invoke sovereign authority again.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_sovereign_authority))
      return;

   WAIT_STATE(ch, skill_table[gsn_sovereign_authority].beats);
   raise_skill(ch, gsn_sovereign_authority);

   for (gch = ch->in_room->first_person; gch != NULL; gch = gch->next_in_room)
   {
      if (!is_same_group(gch, ch))
         continue;

      if (is_affected(gch, gsn_sovereign_authority))
         affect_strip(gch, gsn_sovereign_authority);

      af.type = gsn_sovereign_authority;
      af.duration = 6;
      af.duration_type = DURATION_ROUND;
      af.location = APPLY_HITROLL;
      af.modifier = get_curr_wis(ch) / 3;
      af.bitvector = 0;
      affect_to_char(gch, &af);

      af.location = APPLY_DAMROLL;
      af.modifier = get_curr_wis(ch) / 4;
      affect_to_char(gch, &af);

      af.location = APPLY_SAVING_SPELL;
      af.modifier = -(get_curr_wis(ch) / 5);
      affect_to_char(gch, &af);

      if (gch != ch)
         send_to_char("@@yThe Inquisitor's sovereign authority empowers you.@@N\n\r", gch);
   }

   ch->cooldown[gsn_sovereign_authority] = 30;

   send_to_char(
       "@@yYou invoke sovereign authority over all who serve the Tribunal's purpose.@@N\n\r", ch);
   act("@@y$n invokes sovereign authority! The Tribunal's power extends to all.@@N", ch, NULL, NULL,
       TO_ROOM);
}

/*
 * do_seal_of_the_tribunal: Self-buff preventing testimony loss,
 * reducing verdict cooldown, and enhancing avoidance testimony gain.
 * 60-round cooldown.
 */
void do_seal_of_the_tribunal(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_seal_of_the_tribunal))
   {
      send_to_char("You don't know how to invoke the Tribunal's seal.\n\r", ch);
      return;
   }

   if (ch->fighting == NULL)
   {
      send_to_char("You must be fighting to invoke the Tribunal's seal.\n\r", ch);
      return;
   }

   if (is_affected(ch, gsn_seal_of_the_tribunal))
   {
      send_to_char("The Tribunal's seal is already active.\n\r", ch);
      return;
   }

   if (ch->cooldown[gsn_seal_of_the_tribunal] > 0)
   {
      send_to_char("You are not ready to invoke the Tribunal's seal again.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_seal_of_the_tribunal))
      return;

   WAIT_STATE(ch, skill_table[gsn_seal_of_the_tribunal].beats);
   raise_skill(ch, gsn_seal_of_the_tribunal);

   af.type = gsn_seal_of_the_tribunal;
   af.duration = 6;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   ch->cooldown[gsn_seal_of_the_tribunal] = 60;

   send_to_char("@@RThe Tribunal's seal burns into existence. The proceeding will continue to "
                "completion.@@N\n\r",
                ch);
   act("@@R$n invokes the Seal of the Tribunal! An ancient authority manifests.@@N", ch, NULL, NULL,
       TO_ROOM);
}
