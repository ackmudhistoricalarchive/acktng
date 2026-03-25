#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_sanctified_strike(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];
   int dam;
   int heal;

   one_argument(argument, arg);

   if (!is_fighting(ch))
   {
      send_to_char("You can only perform a sanctified strike when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_sanctified_strike))
      return;

   if (arg[0] == '\0')
      victim = ch->fighting;
   else
      victim = get_char_room(ch, arg);

   if (victim == NULL)
   {
      send_to_char("No such victim!\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_sanctified_strike))
      return;

   raise_skill(ch, gsn_sanctified_strike);

   WAIT_STATE(ch, skill_table[gsn_sanctified_strike].beats);

   ch->cooldown[gsn_sanctified_strike] = 15;

   dam = number_range(get_psuedo_level(ch) * get_curr_str(ch) / 5,
                      get_psuedo_level(ch) * get_curr_str(ch) / 4);

   dam = dam * (10 + char_class_level(ch, CLASS_TEM)) / 10;

   if (IS_NPC(ch))
      dam /= 2;

   check_killer(ch, victim);
   if (can_hit_skill(ch, victim, gsn_sanctified_strike))
   {
      act("@@a$n strikes $N with sanctified force, holy light mending $s wounds!@@N", ch, NULL,
          victim, TO_NOTVICT);
      act("@@a$n strikes you with sanctified force!@@N", ch, NULL, victim, TO_VICT);
      act("@@aYou strike $N with sanctified force, holy light mending your wounds!@@N", ch, NULL,
          victim, TO_CHAR);

      calculate_damage(ch, victim, dam, gsn_sanctified_strike, ELE_PHYSICAL | ELE_HOLY, TRUE);

      heal = dam / 5;
      if (heal > 0)
         heal_character(ch, ch, heal, gsn_sanctified_strike, FALSE);
   }
   else
   {
      act("$n tries to strike $N with sanctified force, but misses!", ch, NULL, victim, TO_NOTVICT);
      act("$n tries to strike you with sanctified force, but misses!", ch, NULL, victim, TO_VICT);
      act("You try to strike $N with sanctified force, but miss!", ch, NULL, victim, TO_CHAR);
      set_fighting(victim, ch, TRUE);
   }
}
