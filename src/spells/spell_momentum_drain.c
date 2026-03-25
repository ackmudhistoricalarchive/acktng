#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "tables.h"
#include "magic.h"

bool spell_momentum_drain(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
   {
      send_to_char("Their momentum is already drained!\n\r", ch);
      return FALSE;
   }

   af.type = sn;
   af.duration = 3 + char_class_level(ch, CLASS_KIN) / 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_HITROLL;
   af.modifier = -(char_class_level(ch, CLASS_KIN) * 2 + char_class_level(ch, CLASS_NEC) +
                   char_class_level(ch, CLASS_EGO));
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(victim, &af);

   af.type = sn;
   af.duration = 3 + char_class_level(ch, CLASS_KIN) / 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_DAMROLL;
   af.modifier = -(char_class_level(ch, CLASS_KIN) * 2 + char_class_level(ch, CLASS_NEC) +
                   char_class_level(ch, CLASS_EGO));
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(victim, &af);

   act("@@G$n redirects $N's combat momentum, draining $S force!@@N", ch, NULL, victim, TO_NOTVICT);
   act("@@G$n redirects your combat momentum -- your strikes feel weakened!@@N", ch, NULL, victim,
       TO_VICT);
   act("@@GYou redirect $N's combat momentum, draining $S force!@@N", ch, NULL, victim, TO_CHAR);
   return TRUE;
}
