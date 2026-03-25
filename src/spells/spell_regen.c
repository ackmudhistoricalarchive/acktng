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

bool spell_regen(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   int base_heal =
       spell_regen_base_heal(char_class_level(ch, CLASS_MAG), char_class_level(ch, CLASS_SOR),
                             char_class_level(ch, CLASS_WIZ), get_spellpower(ch));

   if (is_affected(ch, sn) || is_affected(ch, skill_lookup("regen")))
      return FALSE;
   af.type = sn;
   af.duration = 15 + char_class_level(ch, CLASS_MAG) / 4;
   af.location = APPLY_HOT;
   af.duration_type = DURATION_ROUND;
   af.modifier = class_heal_character(ch, victim, base_heal, sn, INDEX_MAG, TRUE);
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(victim, &af);
   act("$N begins to quickly regenerate from $n's spell.", ch, NULL, victim, TO_ROOM);
   send_to_char("You begin to quickly regenerate.\n\r", victim);
   return TRUE;
}
