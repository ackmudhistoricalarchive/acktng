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

void do_field_patch(CHAR_DATA *ch, char *argument)
{
   char arg[MSL];
   CHAR_DATA *victim;
   int heal_amount;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_field_patch))
   {
      send_to_char("You don't know how to use this skill!\n\r", ch);
      return;
   }

   if (is_fighting(ch))
   {
      send_to_char("You cannot apply field patches while fighting!\n\r", ch);
      return;
   }

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      victim = ch;
   }
   else
   {
      if ((victim = get_char_room(ch, arg)) == NULL)
      {
         send_to_char("They aren't here.\n\r", ch);
         return;
      }
   }

   WAIT_STATE(ch, 60);
   raise_skill(ch, gsn_field_patch);

   heal_amount = dice(3, char_class_level(ch, CLASS_WAR)) + 10;
   victim->hit = UMIN(victim->hit + heal_amount, victim->max_hit);

   if (victim == ch)
   {
      send_to_char("You quickly patch your own wounds.\n\r", ch);
   }
   else
   {
      act("You quickly patch $N's wounds.", ch, NULL, victim, TO_CHAR);
      act("$n patches your wounds.", ch, NULL, victim, TO_VICT);
   }
}
