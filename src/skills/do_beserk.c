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

void do_beserk(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!is_fighting(ch))
   {
      send_to_char("You can only go beserk when fighting!\n\r", ch);
      return;
   }

   if (is_affected(ch, gsn_beserk))
   {
      send_to_char("You are already beserk!!\n\r", ch);
      return;
   }

   if (!can_use_skill(ch, gsn_beserk))
   {
      send_to_char("You don't know how to use this skill!\n\r", ch);
      return;
   }

   WAIT_STATE(ch, skill_table[gsn_beserk].beats);

   if (!subtract_energy_cost(ch, gsn_beserk))
      return;

   raise_skill(ch, gsn_beserk);

   if (number_percent() < 25)
   {
      /*
       * Failure
       */
      act("$n calls on the Dark Powers, but they don't answer!", ch, NULL, NULL, TO_ROOM);
      send_to_char("You call on the Dark Powers.  They don't answer!", ch);
      return;
   }

   af.type = gsn_beserk;
   af.duration = -1;
   af.location = APPLY_AC;
   af.modifier = char_class_level(ch, CLASS_WAR) / 2;
   af.bitvector = 0;
   affect_to_char(ch, &af);

   af.location = APPLY_HITROLL;
   af.modifier = char_class_level(ch, CLASS_WAR) / 5;
   affect_to_char(ch, &af);

   af.location = APPLY_DAMROLL;
   af.modifier = char_class_level(ch, CLASS_WAR) / 5;
   affect_to_char(ch, &af);

   af.location = APPLY_SAVING_SPELL;
   af.modifier = char_class_level(ch, CLASS_WAR) / -10;
   affect_to_char(ch, &af);

   act("$n calls on the Dark Powers, who answer!!!", ch, NULL, NULL, TO_ROOM);
   send_to_char("You call on the Dark Powers.  They answer!!!\n\r", ch);
   return;
}
