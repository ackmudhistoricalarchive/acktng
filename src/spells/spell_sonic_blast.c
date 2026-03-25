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

bool spell_sonic_blast(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;
   int dam;
   dam = 150 + dice(char_class_level(ch, CLASS_EGO) / 2, 20);
   if (sp_damage(obj, ch, victim, dam, ELEMENT_AIR | NO_REFLECT | NO_ABSORB, sn, TRUE))
   {
      if (is_affected(ch, sn))
         return TRUE;
      af.type = sn;
      af.duration = (level / 20);
      af.location = APPLY_DAMROLL;
      af.modifier = -1 * get_psuedo_level(ch) / 15 * get_psuedo_level(victim) / 15;
      af.bitvector = AFF_BLASTED;
      affect_to_char(victim, &af);

      af.type = sn;
      af.duration = (level / 20);
      af.location = APPLY_HITROLL;
      af.modifier = -1 * get_psuedo_level(ch) / 15 * get_psuedo_level(victim) / 15;
      af.bitvector = AFF_BLASTED;
      affect_to_char(victim, &af);
   }
   return TRUE;
}
