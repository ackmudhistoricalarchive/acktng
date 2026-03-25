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

void do_morale(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;
   CHAR_DATA *gch;

   int stat = char_class_level(ch, CLASS_KNI);

   if (char_class_level(ch, CLASS_PAL) > stat)
      stat = char_class_level(ch, CLASS_PAL);

   if (!can_use_skill_message(ch, gsn_morale))
      return;

   if (!is_fighting(ch))
   {
      send_to_char("You must be fighting to do this!\n\r", ch);
      return;
   }

   raise_skill(ch, gsn_morale);

   int combo_count = get_combo_count(ch);
   int bonus = combo_count > 0 ? combo_count : 1;

   if (combo_count > 0)
      reset_combo(ch);

   for (gch = ch->in_room->first_person; gch != NULL; gch = gch->next_in_room)
   {
      if (is_affected(gch, gsn_morale) || !is_same_group(ch, gch))
         continue;

      act("$n seems much more willing to fight.", gch, NULL, NULL, TO_ROOM);
      send_to_char("You are inspired to fight better!\n\r", gch);
      af.type = gsn_morale;
      af.duration_type = DURATION_ROUND;
      af.duration = 50;
      af.location = APPLY_DAMROLL;
      af.modifier = (stat / 2) * bonus;
      af.bitvector = 0;
      affect_to_char(gch, &af);
   }

   if (combo_count > 0)
      send_to_char("@@yYour combo empowers your rally cry!@@N\n\r", ch);

   send_to_char("You inspire the troops!\n\r", ch);
   WAIT_STATE(ch, skill_table[gsn_morale].beats);
}
