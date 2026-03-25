#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_shadow_reading(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (!is_fighting(ch))
   {
      send_to_char("You can only invoke shadow reading when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_shadow_reading))
      return;

   if (is_affected(ch, gsn_shadow_reading))
   {
      send_to_char("You are already reading the shadows!\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_shadow_reading))
      return;

   raise_skill(ch, gsn_shadow_reading);

   WAIT_STATE(ch, skill_table[gsn_shadow_reading].beats);

   ch->cooldown[gsn_shadow_reading] = 15;

   af.type = gsn_shadow_reading;
   af.duration = 4 + char_class_level(ch, CLASS_NIG) / 4;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_HITROLL;
   af.modifier = char_class_level(ch, CLASS_NIG) * 3 + char_class_level(ch, CLASS_ASS) * 2 +
                 char_class_level(ch, CLASS_WLK) * 2;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   act("@@d$n's eyes flicker with shadow-sight, reading $s opponent's defenses.@@N", ch, NULL, NULL,
       TO_ROOM);
   send_to_char("@@dYour eyes flicker with shadow-sight, reading your opponent's defenses.@@N\n\r",
                ch);
}
