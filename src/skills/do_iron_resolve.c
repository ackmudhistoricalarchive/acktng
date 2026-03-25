#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_iron_resolve(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (!is_fighting(ch))
   {
      send_to_char("You can only steel your resolve when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_iron_resolve))
      return;

   if (is_affected(ch, gsn_iron_resolve))
   {
      send_to_char("Your resolve is already unshakeable!\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_iron_resolve))
      return;

   raise_skill(ch, gsn_iron_resolve);

   WAIT_STATE(ch, skill_table[gsn_iron_resolve].beats);

   ch->cooldown[gsn_iron_resolve] = 15;

   af.type = gsn_iron_resolve;
   af.duration = 3 + char_class_level(ch, CLASS_CRU) / 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_AC;
   af.modifier = -(char_class_level(ch, CLASS_CRU) * 3);
   af.bitvector = 0;
   affect_to_char(ch, &af);

   act("$n plants $s feet and steels $s resolve, immovable as iron!", ch, NULL, NULL, TO_ROOM);
   send_to_char("You plant your feet and steel your resolve, immovable as iron!\n\r", ch);
}
