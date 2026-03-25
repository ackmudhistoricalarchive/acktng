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

bool spell_wraith_touch(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam;
   float drain_mod = 1.0;

   if (obj == NULL)
   {
      dam = 150 + dice(char_class_level(ch, CLASS_NEC) / 2, 20);
      act("@@RA @@dwraithlike hand @@Rleaps forth from $n!@@N", ch, NULL, NULL, TO_ROOM);
      send_to_char("@@RA @@dwraithlike hand @@Rleaps forth from your hands!@@N\n\r", ch);
   }
   else
   {
      dam = 150 + dice(char_class_level(ch, CLASS_NEC) / 2, 20);
      act("@@RA @@dwraithlike hand @@Rleaps forth from $p!@@N", ch, obj, NULL, TO_ROOM);
      act("@@RA @@dwraithlike hand @@Rleaps forth from $p!@@N", ch, obj, NULL, TO_CHAR);
   }
   act("@@R$n is struck by the @@dwraithlike hand @@R!!@@N", victim, NULL, NULL, TO_ROOM);
   send_to_char("@@RYou are struck by a @@dwraithlike hand @@R!!@@N\n\r", victim);
   drain_mod = char_class_level(ch, CLASS_NEC) * dam / 130;
   if (sp_damage(obj, ch, victim, dam, ELEMENT_SHADOW | NO_REFLECT | NO_ABSORB, sn, TRUE))
   {
      ch->hit = UMIN(get_max_hp(ch), (ch->hit + drain_mod));
      apply_necromancer_damage_debuff(ch, victim, sn, dam, obj);
   }
   return TRUE;
}
