#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_oathshield(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   char arg[MAX_INPUT_LENGTH];

   one_argument(argument, arg);

   if (!is_fighting(ch))
   {
      send_to_char("You can only invoke your oathshield when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_oathshield))
      return;

   if (arg[0] == '\0')
   {
      send_to_char("Shield whom with your oath?\n\r", ch);
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
      send_to_char("You cannot shield yourself with your own oath.\n\r", ch);
      return;
   }

   if (is_affected(victim, gsn_oathshield))
   {
      send_to_char("They are already shielded by an oath!\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_oathshield))
      return;

   raise_skill(ch, gsn_oathshield);

   WAIT_STATE(ch, skill_table[gsn_oathshield].beats);

   ch->cooldown[gsn_oathshield] = 20;

   af.type = gsn_oathshield;
   af.duration = 3 + char_class_level(ch, CLASS_TEM) / 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_AC;
   af.modifier = -(char_class_level(ch, CLASS_TEM) * 4 + char_class_level(ch, CLASS_PAL) * 2);
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(victim, &af);

   act("@@a$n extends $s oath to shield $N!@@N", ch, NULL, victim, TO_NOTVICT);
   act("@@a$n extends $s oath to shield you!@@N", ch, NULL, victim, TO_VICT);
   act("@@aYou extend your oath to shield $N!@@N", ch, NULL, victim, TO_CHAR);
}
