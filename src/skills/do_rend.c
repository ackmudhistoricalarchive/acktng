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

void do_rend(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;

   if (IS_NPC(ch))
      return;

   if (!can_use_skill(ch, gsn_rend))
   {
      send_to_char("You don't know how to use this skill!\n\r", ch);
      return;
   }

   if (!is_fighting(ch) || (victim = ch->fighting) == NULL)
   {
      send_to_char("You must be fighting someone!\n\r", ch);
      return;
   }

   WAIT_STATE(ch, skill_table[gsn_rend].beats);
   raise_skill(ch, gsn_rend);

   /* Apply AC debuff (positive modifier = worse AC) to victim */
   af.type = gsn_rend;
   af.duration = 24;
   af.location = APPLY_AC;
   af.modifier = char_class_level(ch, CLASS_WAR) / 4;
   af.bitvector = 0;
   affect_to_char(victim, &af);

   /* Deal a small amount of damage */
   damage(ch, victim, dice(2, char_class_level(ch, CLASS_WAR) / 4 + 1), gsn_rend);

   act("You rend through $N's defenses!", ch, NULL, victim, TO_CHAR);
   act("$n rends through $N's defenses!", ch, NULL, victim, TO_NOTVICT);
   act("$n rends through your defenses!", ch, NULL, victim, TO_VICT);
}
