#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_momentum_chain(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (!is_fighting(ch))
   {
      send_to_char("You can only build momentum when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_momentum_chain))
      return;

   if (is_affected(ch, gsn_momentum_chain))
   {
      send_to_char("You are already building momentum!\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_momentum_chain))
      return;

   raise_skill(ch, gsn_momentum_chain);

   WAIT_STATE(ch, skill_table[gsn_momentum_chain].beats);

   ch->cooldown[gsn_momentum_chain] = 20;

   af.type = gsn_momentum_chain;
   af.duration = 5 + char_class_level(ch, CLASS_CRU) / 4;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_DAMROLL;
   af.modifier = char_class_level(ch, CLASS_CRU) * 3 + char_class_level(ch, CLASS_KNI) * 2 +
                 char_class_level(ch, CLASS_SWO) * 2;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   act("$n builds momentum, each strike flowing into the next!", ch, NULL, NULL, TO_ROOM);
   send_to_char("You build momentum, each strike flowing into the next!\n\r", ch);
}
