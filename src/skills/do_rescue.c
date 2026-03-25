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

void do_rescue(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   CHAR_DATA *fch;
   int best;
   int cnt;

   best = -1;

   if (!IS_NPC(ch))
   {
      for (cnt = 0; cnt < 4; cnt++)
      {
         int cl = ch->mortal_class[cnt];
         if (cl < 0)
            continue;
         if (ch->mortal_level[cnt] >= skill_table[gsn_rescue].skill_level[cl] &&
             ch->mortal_level[cnt] >= best)
            best = cl;
      }
   }
   else
      best = ch->level;

   if (!can_use_skill_message(ch, gsn_rescue))
      return;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Rescue whom?\n\r", ch);
      return;
   }

   if ((victim = get_char_room(ch, arg)) == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
      return;
   }

   if (victim == ch)
   {
      send_to_char("What about fleeing instead?\n\r", ch);
      return;
   }

   if (!IS_NPC(ch) && IS_NPC(victim))
   {
      send_to_char("Doesn't need your help!\n\r", ch);
      return;
   }

   if (ch->fighting == victim)
   {
      send_to_char("Too late.\n\r", ch);
      return;
   }

   if ((fch = victim->fighting) == NULL)
   {
      send_to_char("That person is not fighting right now.\n\r", ch);
      return;
   }

   WAIT_STATE(ch, skill_table[gsn_rescue].beats);
   if (!IS_NPC(ch) && !skill_success(ch, victim, gsn_rescue, 0))
   {
      send_to_char("You fail the rescue.\n\r", ch);
      return;
   }

   act("You rescue $N!", ch, NULL, victim, TO_CHAR);
   act("$n rescues you!", ch, NULL, victim, TO_VICT);
   act("$n rescues $N!", ch, NULL, victim, TO_NOTVICT);

   stop_fighting(fch, FALSE);
   stop_fighting(victim, FALSE);

   set_fighting(ch, fch, TRUE);
   set_fighting(fch, ch, FALSE);
   return;
}
