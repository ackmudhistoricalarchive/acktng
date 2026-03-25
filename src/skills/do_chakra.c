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

void do_chakra(CHAR_DATA *ch, char *argument)
{
   if (!is_fighting(ch))
   {
      send_to_char("You can only prepare for a chakra when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_chakra))
      return;

   int cost = chi_skill_cost(5, ch->cooldown[gsn_chiblock]);

   if (get_chi(ch) < cost)
   {
      send_to_char("You do not have sufficient chi to use chakra.\n\r", ch);
      return;
   }

   ch->chi -= cost;

   raise_skill(ch, gsn_chakra);

   WAIT_STATE(ch, skill_table[gsn_chakra].beats);

   act("$n focuses their chakra, and $n looks better.", ch, NULL, NULL, TO_ROOM);
   act("You focus your chakra, and you feel noticably better.", ch, NULL, NULL, TO_CHAR);

   ch->cooldown[gsn_chakra] = 10;

   int base_heal = 5;

   if (char_class_level(ch, CLASS_MAR) > 0)
      base_heal = 7;

   int heal = class_heal_character(
       ch, ch, (char_class_level(ch, CLASS_MON) + char_class_level(ch, CLASS_MAR)) * base_heal,
       gsn_chakra, CLASS_MON, FALSE);

   heal_character(ch, ch, heal, gsn_chakra, FALSE);

   int dur = 1;
   if (char_class_level(ch, CLASS_MAR) > 0)
      dur = 2;

   AFFECT_DATA af;

   af.type = gsn_chakra;
   af.duration = dur;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_DAMROLL;
   af.modifier = char_class_level(ch, CLASS_MON) * 5 + char_class_level(ch, CLASS_MAR) * 5;
   af.bitvector = 0;
   affect_to_char(ch, &af);
}
