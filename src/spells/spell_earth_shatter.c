/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  Ack 2.2 improvements copyright (C) 1994 by Stephen Dooley              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *       _/          _/_/_/     _/    _/     _/    ACK! MUD is modified    *
 *      _/_/        _/          _/  _/       _/    Merc2.0/2.1/2.2 code    *
 *     _/  _/      _/           _/_/         _/    (c)Stephen Zepp 1998    *
 *    _/_/_/_/      _/          _/  _/             Version #: 4.3          *
 *   _/      _/      _/_/_/     _/    _/     _/                            *
 *                                                                         *
 *                        http://ackmud.nuc.net/                           *
 *                        zenithar@ackmud.nuc.net                          *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "tables.h"
#include "magic.h"

bool spell_earth_shatter(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam;

   if (obj == NULL)
   {
      int base = char_class_level(ch, CLASS_SOR);
      if (char_class_level(ch, CLASS_WIZ) > base)
         base = char_class_level(ch, CLASS_WIZ);
      if (char_class_level(ch, CLASS_NEC) > base)
         base = char_class_level(ch, CLASS_NEC);

      dam = 150 + dice(base / 2, 20);
      act("@@gShattering @@bearth@@g erupts from $n's hands!@@N", ch, NULL, NULL, TO_ROOM);
      send_to_char("@@gShattering @@bearth@@g erupts from your hands!@@N\n\r", ch);
   }
   else
   {
      dam = dice(level / 4, 20);
      act("@@gShattering @@bearth@@g erupts from $p!@@N", ch, obj, NULL, TO_ROOM);
      act("@@gShattering @@bearth@@g erupts from $p!@@N", ch, obj, NULL, TO_CHAR);
   }
   act("@@g$n is shattered by jagged @@bearth@@g!!@@N", victim, NULL, NULL, TO_ROOM);
   send_to_char("@@gYou are shattered by jagged @@bearth@@g!!@@N\n\r", victim);
   sp_damage(obj, ch, victim, dam, ELE_EARTH, sn, TRUE);
   if (!trigger_elemental_spell_combo(ch, victim, obj, sn, level))
      apply_elemental_spell_debuff(ch, victim, sn, "@@aYou are rimed with deep frost.@@N\n\r");
   return TRUE;
}
