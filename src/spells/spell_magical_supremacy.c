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

bool spell_magical_supremacy(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;
   int buff;

   if (ch != victim)
   {
      send_to_char("You can't target someone else for supremacy!\n\r", ch);
      return FALSE;
   }

   if (is_affected(ch, skill_lookup("magical supremacy")))
      return FALSE;

   if (char_class_level(ch, CLASS_EGO) > 0)
      buff = char_class_level(ch, CLASS_EGO) * 1.5;
   else if (char_class_level(ch, CLASS_WIZ) > 0)
      buff = char_class_level(ch, CLASS_WIZ) * 1.5;
   else
      buff = 20;

   af.type = sn;
   af.duration = 15;
   af.location = APPLY_SPELLPOWER;
   af.duration_type = DURATION_ROUND;
   af.modifier = buff;
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(ch, &af);
   act("$N gains supremacy from $n's spell.", ch, NULL, victim, TO_ROOM);
   send_to_char("You gain supremacy.\n\r", victim);
   return TRUE;
}
