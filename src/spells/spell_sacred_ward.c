#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "tables.h"
#include "magic.h"

bool spell_sacred_ward(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   AFFECT_DATA af;
   int heal_mod;

   heal_mod = char_class_level(ch, CLASS_TEM) * 3 + char_class_level(ch, CLASS_PAL) +
              char_class_level(ch, CLASS_PRI);

   act("@@a$n calls forth a sacred ward, blessing the ground with holy power!@@N", ch, NULL, NULL,
       TO_ROOM);
   send_to_char("@@aYou call forth a sacred ward, blessing the ground with holy power!@@N\n\r", ch);

   for (vch = ch->in_room->first_person; vch != NULL; vch = vch_next)
   {
      vch_next = vch->next_in_room;

      if (!IS_NPC(vch) && is_same_group(ch, vch) && !is_affected(vch, sn))
      {
         af.type = sn;
         af.duration = 6 + char_class_level(ch, CLASS_TEM) / 4;
         af.duration_type = DURATION_ROUND;
         af.location = APPLY_HOT;
         af.modifier = heal_mod;
         af.bitvector = 0;
         af.caster = ch;
         affect_to_char(vch, &af);

         send_to_char("@@aYou feel the sacred ward's warmth restore you!@@N\n\r", vch);
      }
   }

   return TRUE;
}
