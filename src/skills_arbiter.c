#include "globals.h"
#include "magic.h"
#include "sentinel.h"
#include "skills.h"

/*
 * do_procedural_authority: Party buff granting saves + dodge bonus.
 */
void do_procedural_authority(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *gch;
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_procedural_authority))
   {
      send_to_char("You don't know how to invoke procedural authority.\n\r", ch);
      return;
   }

   if (ch->fighting == NULL)
   {
      send_to_char("You must be fighting to invoke procedural authority.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_procedural_authority))
      return;

   WAIT_STATE(ch, skill_table[gsn_procedural_authority].beats);
   raise_skill(ch, gsn_procedural_authority);

   for (gch = ch->in_room->first_person; gch != NULL; gch = gch->next_in_room)
   {
      if (!is_same_group(gch, ch))
         continue;

      /* Remove old instance before reapplying */
      if (is_affected(gch, gsn_procedural_authority))
         affect_strip(gch, gsn_procedural_authority);

      af.type = gsn_procedural_authority;
      af.duration = 6;
      af.duration_type = DURATION_ROUND;
      af.location = APPLY_SAVING_SPELL;
      af.modifier = -(get_curr_wis(ch) / 4);
      af.bitvector = 0;
      affect_to_char(gch, &af);

      af.location = APPLY_DEX;
      af.modifier = get_curr_wis(ch) / 5;
      affect_to_char(gch, &af);

      if (gch != ch)
         send_to_char(
             "@@yYou feel the Arbiter's procedural authority bolster your defenses.@@N\n\r", gch);
   }

   send_to_char(
       "@@yYou invoke procedural authority, extending your protection to the group.@@N\n\r", ch);
   act("@@y$n invokes procedural authority over the battlefield.@@N", ch, NULL, NULL, TO_ROOM);
}

/*
 * do_sustained_objection: Suppress target's skills for 2 rounds.
 * Requires 2+ testimony, does not consume testimony.
 */
void do_sustained_objection(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_sustained_objection))
   {
      send_to_char("You don't know how to raise a sustained objection.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   if (ch->testimony < 2 || ch->testimony_target != victim)
   {
      send_to_char("You need at least 2 testimony marks against this target.\n\r", ch);
      return;
   }

   if (is_affected(victim, gsn_sustained_objection))
   {
      send_to_char("They are already under a sustained objection.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_sustained_objection))
      return;

   WAIT_STATE(ch, skill_table[gsn_sustained_objection].beats);
   raise_skill(ch, gsn_sustained_objection);

   af.type = gsn_sustained_objection;
   af.duration = 2;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = 0;
   affect_to_char(victim, &af);

   act("@@eYou raise a sustained objection against $N, silencing their trained responses!@@N", ch,
       NULL, victim, TO_CHAR);
   act("@@e$n raises a sustained objection against you! Your skills are suppressed!@@N", ch, NULL,
       victim, TO_VICT);
   act("@@e$n raises a sustained objection against $N!@@N", ch, NULL, victim, TO_NOTVICT);
}

/*
 * do_judicial_immunity: Self-buff granting mental effect immunity
 * and testimony protection for 4 rounds.
 */
void do_judicial_immunity(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_judicial_immunity))
   {
      send_to_char("You don't know how to invoke judicial immunity.\n\r", ch);
      return;
   }

   if (ch->fighting == NULL)
   {
      send_to_char("You must be fighting to invoke judicial immunity.\n\r", ch);
      return;
   }

   if (is_affected(ch, gsn_judicial_immunity))
   {
      send_to_char("You are already under judicial immunity.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_judicial_immunity))
      return;

   WAIT_STATE(ch, skill_table[gsn_judicial_immunity].beats);
   raise_skill(ch, gsn_judicial_immunity);

   af.type = gsn_judicial_immunity;
   af.duration = 4;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   send_to_char(
       "@@yYou invoke judicial immunity. Fear, charm, and interference cannot touch you.@@N\n\r",
       ch);
   act("@@y$n invokes judicial immunity, hardening against all interference.@@N", ch, NULL, NULL,
       TO_ROOM);
}

/*
 * do_appellate_review: Remove one negative affect from an ally.
 * Cannot be used on self.
 */
void do_appellate_review(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_appellate_review))
   {
      send_to_char("You don't know how to conduct an appellate review.\n\r", ch);
      return;
   }

   one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      send_to_char("Review whose case?\n\r", ch);
      return;
   }

   victim = get_char_room(ch, arg);
   if (victim == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
      return;
   }

   if (victim == ch)
   {
      send_to_char("You cannot review your own case.\n\r", ch);
      return;
   }

   if (!is_same_group(victim, ch))
   {
      send_to_char("They are not in your group.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_appellate_review))
      return;

   WAIT_STATE(ch, skill_table[gsn_appellate_review].beats);
   raise_skill(ch, gsn_appellate_review);

   /* Remove one negative affect in priority order */
   bool removed = FALSE;
   if (is_affected(victim, gsn_charm_person))
   {
      affect_strip(victim, gsn_charm_person);
      removed = TRUE;
   }
   else if (is_affected(victim, gsn_curse))
   {
      affect_strip(victim, gsn_curse);
      removed = TRUE;
   }
   else if (is_affected(victim, gsn_poison))
   {
      affect_strip(victim, gsn_poison);
      removed = TRUE;
   }
   else if (is_affected(victim, gsn_blindness))
   {
      affect_strip(victim, gsn_blindness);
      removed = TRUE;
   }

   if (removed)
   {
      act("@@yYou conduct an appellate review and overturn the affliction on $N.@@N", ch, NULL,
          victim, TO_CHAR);
      act("@@y$n reviews your case and overturns an affliction!@@N", ch, NULL, victim, TO_VICT);
      act("@@y$n conducts an appellate review on $N.@@N", ch, NULL, victim, TO_NOTVICT);
   }
   else
   {
      act("@@yYou review $N's case but find no affliction to overturn.@@N", ch, NULL, victim,
          TO_CHAR);
   }
}

/*
 * do_suppression_order: Reduce target's damage output by 20% for 5 rounds.
 * Requires 4+ testimony, does not consume testimony.
 */
void do_suppression_order(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_suppression_order))
   {
      send_to_char("You don't know how to issue a suppression order.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) == NULL)
   {
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   if (ch->testimony < 4 || ch->testimony_target != victim)
   {
      send_to_char("You need at least 4 testimony marks against this target.\n\r", ch);
      return;
   }

   if (is_affected(victim, gsn_suppression_order))
   {
      send_to_char("They are already under a suppression order.\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_suppression_order))
      return;

   WAIT_STATE(ch, skill_table[gsn_suppression_order].beats);
   raise_skill(ch, gsn_suppression_order);

   af.type = gsn_suppression_order;
   af.duration = 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_DAMROLL;
   af.modifier = -(victim->level / 5);
   af.bitvector = 0;
   affect_to_char(victim, &af);

   act("@@eYou issue a suppression order against $N, limiting the scope of their violence.@@N", ch,
       NULL, victim, TO_CHAR);
   act("@@e$n issues a suppression order against you! Your strikes feel weakened.@@N", ch, NULL,
       victim, TO_VICT);
   act("@@e$n issues a suppression order against $N.@@N", ch, NULL, victim, TO_NOTVICT);
}
