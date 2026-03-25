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
#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_fortify(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;
   int level;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_fortify))
   {
      send_to_char("You don't know how to use this skill!\n\r", ch);
      return;
   }

   /* Toggle off if already active */
   if (is_affected(ch, gsn_fortify))
   {
      affect_strip(ch, gsn_fortify);
      send_to_char("You relax your defensive stance.\n\r", ch);
      return;
   }

   level = char_class_level(ch, CLASS_WAR);

   WAIT_STATE(ch, skill_table[gsn_fortify].beats);
   raise_skill(ch, gsn_fortify);

   af.type = gsn_fortify;
   af.duration = -1;
   af.location = APPLY_AC;
   af.modifier = -(level / 3);
   af.bitvector = 0;
   affect_to_char(ch, &af);

   af.location = APPLY_DAMROLL;
   af.modifier = -(level / 6);
   affect_to_char(ch, &af);

   send_to_char("You take a fortified defensive stance.\n\r", ch);
   act("$n takes a fortified defensive stance.", ch, NULL, NULL, TO_ROOM);
}
