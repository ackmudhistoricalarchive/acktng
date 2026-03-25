#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_overwhelming_assault(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   int dam;
   int max;
   int mult;

   if (!is_fighting(ch))
   {
      send_to_char("You can only perform an overwhelming assault when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_overwhelming_assault))
      return;

   if (!is_valid_finisher(ch))
   {
      send_to_char("You are not prepared for a finisher!\n\r", ch);
      return;
   }

   victim = ch->fighting;

   if (victim == NULL)
   {
      send_to_char("You are not fighting anyone!\n\r", ch);
      return;
   }

   max = get_max_combo(ch);
   mult = combo_damage_multiplier_for_max_combo(max);

   reset_combo(ch);

   raise_skill(ch, gsn_overwhelming_assault);

   WAIT_STATE(ch, skill_table[gsn_overwhelming_assault].beats);

   ch->cooldown[gsn_overwhelming_assault] = 20;

   dam = number_range(get_psuedo_level(ch) * get_curr_str(ch) / 5,
                      get_psuedo_level(ch) * get_curr_str(ch) / 4);

   if (dam < 1)
      dam = 1;

   dam = dam * mult * (10 + char_class_level(ch, CLASS_CRU)) / 10;

   if (IS_NPC(ch))
      dam /= 2;

   check_killer(ch, victim);
   if (can_hit_skill(ch, victim, gsn_overwhelming_assault))
   {
      act("@@c$n unleashes an overwhelming assault on $N, channeling all $s momentum into a single "
          "devastating blow!@@N",
          ch, NULL, victim, TO_NOTVICT);
      act("@@c$n unleashes an overwhelming assault, channeling all $s momentum into a devastating "
          "blow "
          "that crashes into you!@@N",
          ch, NULL, victim, TO_VICT);
      act("@@cYou unleash an overwhelming assault on $N, channeling all your momentum into a "
          "single "
          "devastating blow!@@N",
          ch, NULL, victim, TO_CHAR);

      calculate_damage(ch, victim, dam, gsn_overwhelming_assault, ELE_PHYSICAL, TRUE);
   }
   else
   {
      act("$n tries to unleash an overwhelming assault on $N, but misses!", ch, NULL, victim,
          TO_NOTVICT);
      act("$n tries to unleash an overwhelming assault on you, but misses!", ch, NULL, victim,
          TO_VICT);
      act("You try to unleash an overwhelming assault on $N, but miss!", ch, NULL, victim, TO_CHAR);
      set_fighting(victim, ch, TRUE);
   }
}
