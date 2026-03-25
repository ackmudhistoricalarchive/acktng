#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_fist_of_the_interior_form(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];
   int dam;

   one_argument(argument, arg);

   if (!is_fighting(ch))
   {
      send_to_char("You can only perform the fist of the interior form when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_fist_interior))
      return;

   if (get_chi(ch) < 20)
   {
      send_to_char("You need at least 20 chi to perform the fist of the interior form!\n\r", ch);
      return;
   }

   if (arg[0] == '\0')
      victim = ch->fighting;
   else
      victim = get_char_room(ch, arg);

   if (victim == NULL)
   {
      send_to_char("No such victim!\n\r", ch);
      return;
   }

   ch->chi -= 20;

   raise_skill(ch, gsn_fist_interior);

   WAIT_STATE(ch, skill_table[gsn_fist_interior].beats);

   ch->cooldown[gsn_fist_interior] = 30;

   dam = number_range(get_psuedo_level(ch) * get_curr_str(ch) / 30,
                      get_psuedo_level(ch) * get_curr_str(ch) / 20);

   if (dam < 1)
      dam = 1;

   dam = dam * 20 * (10 + char_class_level(ch, CLASS_MAR)) / 10;

   if (IS_NPC(ch))
      dam /= 2;

   check_killer(ch, victim);
   if (can_hit_skill(ch, victim, gsn_fist_interior))
   {
      act("@@g$n draws a single, perfect breath and strikes $N with the Fist of the Interior "
          "Form!@@N",
          ch, NULL, victim, TO_NOTVICT);
      act("@@g$n draws a single, perfect breath and strikes you with the Fist of the Interior "
          "Form!@@N",
          ch, NULL, victim, TO_VICT);
      act("@@gYou draw a single, perfect breath and strike $N with the Fist of the Interior "
          "Form!@@N",
          ch, NULL, victim, TO_CHAR);

      calculate_damage(ch, victim, dam, gsn_fist_interior, ELE_PHYSICAL, TRUE);
   }
   else
   {
      act("$n attempts the Fist of the Interior Form on $N, but misses!", ch, NULL, victim,
          TO_NOTVICT);
      act("$n attempts the Fist of the Interior Form on you, but misses!", ch, NULL, victim,
          TO_VICT);
      act("You attempt the Fist of the Interior Form on $N, but miss!", ch, NULL, victim, TO_CHAR);
      set_fighting(victim, ch, TRUE);
   }
}
