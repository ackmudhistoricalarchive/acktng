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

void do_mindoverbody(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (!is_fighting(ch))
   {
      send_to_char("You can only perform mindoverbody when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_mindoverbody))
      return;

   if (!subtract_energy_cost(ch, gsn_mindoverbody))
      return;

   ch->chi++;

   ch->cooldown[gsn_mindoverbody] = 15;

   int base_heal = char_class_level(ch, CLASS_PUG);

   base_heal += char_class_level(ch, CLASS_MON) * 2;
   base_heal += char_class_level(ch, CLASS_BRA) * 2;
   base_heal += char_class_level(ch, CLASS_MAR) * 5;

   if (chi_should_block_mindoverbody(is_affected(ch, gsn_mindoverbody),
                                     is_affected(ch, skill_lookup("mindoverbody"))))
      return;
   af.type = gsn_mindoverbody;
   af.duration = 3;
   af.location = APPLY_HOT;
   af.duration_type = DURATION_ROUND;
   af.modifier = base_heal;
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(ch, &af);
   act("$n begins to focus on mind over body.", ch, NULL, NULL, TO_ROOM);
   send_to_char("You begin to focus on mind over body.\n\r", ch);
   WAIT_STATE(ch, skill_table[gsn_mindoverbody].beats);
}
