#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "tables.h"
#include "magic.h"

bool spell_convergence_shield(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   AFFECT_DATA af;

   if (is_affected(ch, sn))
   {
      send_to_char("You are already shielded by a convergence barrier.\n\r", ch);
      return FALSE;
   }

   af.type = sn;
   af.duration = level / 8;
   af.duration_type = DURATION_HOUR;
   af.location = APPLY_AC;
   af.modifier = -(char_class_level(ch, CLASS_GMA) * 5 + get_curr_int(ch) / 2);
   af.bitvector = 0;
   affect_to_char(ch, &af);

   act("@@pA rotating barrier of elemental forces forms around $n!@@N", ch, NULL, NULL, TO_ROOM);
   send_to_char("@@pA rotating barrier of elemental forces forms around you!@@N\n\r", ch);
   return TRUE;
}
