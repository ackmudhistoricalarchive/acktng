#include "globals.h"
#include "magic.h"
#include "skills.h"

/*
 * Veteran's Cadence — Crusader (CLASS_CRU) Level 10
 *
 * Expresses the Crusader tradition's campaign-service origin: practitioners
 * who remained in the field through sustained engagements developed a rhythm
 * that built effectiveness over time rather than spending it.  Each activation
 * adds a stacking DAMROLL bonus (up to three stacks); the bonuses fade when
 * combat ends, representing momentum that only compounds through sustained
 * engagement.
 */

#define CADENCE_MAX_STACKS 3

void do_veterans_cadence(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;
   AFFECT_DATA *aff;
   int stacks;

   if (!is_fighting(ch))
   {
      send_to_char("Veteran's cadence only builds in the heat of battle!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_veterans_cadence))
      return;

   /* Count existing stacks */
   stacks = 0;
   for (aff = ch->first_affect; aff != NULL; aff = aff->next)
      if (aff->type == gsn_veterans_cadence)
         stacks++;

   if (stacks >= CADENCE_MAX_STACKS)
   {
      send_to_char("Your cadence is already at its peak — hold the line!\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_veterans_cadence))
      return;

   raise_skill(ch, gsn_veterans_cadence);

   WAIT_STATE(ch, skill_table[gsn_veterans_cadence].beats);

   ch->cooldown[gsn_veterans_cadence] = 12;

   af.type = gsn_veterans_cadence;
   af.duration = 6 + char_class_level(ch, CLASS_CRU) / 4;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_DAMROLL;
   af.modifier = 4 + char_class_level(ch, CLASS_CRU) + char_class_level(ch, CLASS_KNI) +
                 char_class_level(ch, CLASS_SWO);
   af.bitvector = 0;
   affect_to_char(ch, &af);

   if (stacks == 0)
   {
      act("$n settles into a steady rhythm, each blow carrying the weight of hard-won experience.",
          ch, NULL, NULL, TO_ROOM);
      send_to_char("You settle into a steady rhythm, each blow carrying the weight of hard-won "
                   "experience.\n\r",
                   ch);
   }
   else if (stacks == 1)
   {
      act("$n's cadence deepens — a veteran's pace that does not slow.", ch, NULL, NULL, TO_ROOM);
      send_to_char("Your cadence deepens — a veteran's pace that does not slow.\n\r", ch);
   }
   else
   {
      act("$n reaches full campaign tempo, unstoppable as a force that has never once broken.", ch,
          NULL, NULL, TO_ROOM);
      send_to_char(
          "You reach full campaign tempo, unstoppable as a force that has never once broken.\n\r",
          ch);
   }
}
