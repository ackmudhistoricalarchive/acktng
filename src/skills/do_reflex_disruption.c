#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_reflex_disruption(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   char arg[MAX_INPUT_LENGTH];

   one_argument(argument, arg);

   if (!is_fighting(ch))
   {
      send_to_char("You can only disrupt reflexes when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_reflex_disruption))
      return;

   if (arg[0] == '\0')
      victim = ch->fighting;
   else
      victim = get_char_room(ch, arg);

   if (victim == NULL)
   {
      send_to_char("No such victim!\n\r", ch);
      return;
   }

   if (is_affected(victim, gsn_reflex_disruption))
   {
      send_to_char("Their reflexes are already disrupted!\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_reflex_disruption))
      return;

   raise_skill(ch, gsn_reflex_disruption);

   WAIT_STATE(ch, skill_table[gsn_reflex_disruption].beats);

   ch->cooldown[gsn_reflex_disruption] = 20;

   af.type = gsn_reflex_disruption;
   af.duration = 2 + char_class_level(ch, CLASS_NIG) / 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_HITROLL;
   af.modifier = -(char_class_level(ch, CLASS_NIG) * 3 + char_class_level(ch, CLASS_ASS) * 2);
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(victim, &af);

   act("@@d$n disrupts $N's combat reflexes with targeted hex interference!@@N", ch, NULL, victim,
       TO_NOTVICT);
   act("@@d$n disrupts your combat reflexes -- your reactions feel sluggish!@@N", ch, NULL, victim,
       TO_VICT);
   act("@@dYou disrupt $N's combat reflexes with targeted hex interference!@@N", ch, NULL, victim,
       TO_CHAR);
}
