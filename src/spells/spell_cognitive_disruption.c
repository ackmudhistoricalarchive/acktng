#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "tables.h"
#include "magic.h"

bool spell_cognitive_disruption(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;
   int dam;

   if (is_affected(victim, sn))
   {
      send_to_char("Their cognition is already disrupted!\n\r", ch);
      return FALSE;
   }

   dam = dice(char_class_level(ch, CLASS_KIN) + char_class_level(ch, CLASS_EGO), 8);

   af.type = sn;
   af.duration = 1 + char_class_level(ch, CLASS_KIN) / 10;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_SAVING_SPELL;
   af.modifier = char_class_level(ch, CLASS_KIN) * 3;
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(victim, &af);

   act("@@G$n disrupts $N's cognitive focus, scattering $S concentration!@@N", ch, NULL, victim,
       TO_NOTVICT);
   act("@@G$n disrupts your cognitive focus -- your thoughts scatter!@@N", ch, NULL, victim,
       TO_VICT);
   act("@@GYou disrupt $N's cognitive focus, scattering $S concentration!@@N", ch, NULL, victim,
       TO_CHAR);

   sp_damage(obj, ch, victim, dam, ELEMENT_MENTAL, sn, TRUE);
   return TRUE;
}
