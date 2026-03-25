#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "tables.h"
#include "magic.h"

bool spell_entropic_shield(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   AFFECT_DATA af;

   if (is_affected(ch, sn))
   {
      send_to_char("You are already shielded by an entropic field.\n\r", ch);
      return FALSE;
   }

   af.type = sn;
   af.duration = level / 8;
   af.duration_type = DURATION_HOUR;
   af.location = APPLY_AC;
   af.modifier = -(char_class_level(ch, CLASS_KIN) * 4 + get_curr_int(ch) / 3);
   af.bitvector = 0;
   affect_to_char(ch, &af);

   act("@@GAn entropic field shimmers into existence around $n, degrading incoming attacks.@@N", ch,
       NULL, NULL, TO_ROOM);
   send_to_char("@@GAn entropic field shimmers into existence around you, degrading incoming "
                "attacks.@@N\n\r",
                ch);
   return TRUE;
}
