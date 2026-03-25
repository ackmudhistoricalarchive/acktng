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

bool spell_soul_thief(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *summoned = player_summon(ch, 100 + char_class_level(ch, CLASS_NEC) / 4, SOUL_THIEF);

   act("$n calls upon the @@dNegative Plane@@N!", ch, obj, NULL, TO_ROOM);
   act("You call upon the @@dNegative Plane@@N.", ch, obj, NULL, TO_CHAR);

   act("@@NThe ground opens beneath you, and a @@lSoul @@eThief@@N crawls out of the graveyard "
       "filth!!",
       ch, obj, NULL, TO_ROOM);
   act("@@NThe ground opens beneath you, and a @@lSoul @@eThief@@N crawls out of the graveyard "
       "filth!!",
       ch, obj, NULL, TO_CHAR);

   act("$n Stands erect, and bow's towards it's master.", summoned, NULL, NULL, TO_ROOM);

   return TRUE;
}
