#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_breath_of_endurance(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   if (!is_fighting(ch))
   {
      send_to_char("You can only focus your breath of endurance when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_breath_of_endurance))
      return;

   if (is_affected(ch, gsn_breath_of_endurance))
   {
      send_to_char("You are already sustaining your breath of endurance!\n\r", ch);
      return;
   }

   int cost = chi_skill_cost(8, ch->cooldown[gsn_breath_of_endurance]);

   if (get_chi(ch) < cost)
   {
      send_to_char("You do not have sufficient chi to sustain your breath.\n\r", ch);
      return;
   }

   ch->chi -= cost;

   raise_skill(ch, gsn_breath_of_endurance);

   WAIT_STATE(ch, skill_table[gsn_breath_of_endurance].beats);

   ch->cooldown[gsn_breath_of_endurance] = 15;

   int base_heal = char_class_level(ch, CLASS_MON) + char_class_level(ch, CLASS_BRA) +
                   char_class_level(ch, CLASS_MAR) * 3;

   af.type = gsn_breath_of_endurance;
   af.duration = 2 + char_class_level(ch, CLASS_MAR) / 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_HOT;
   af.modifier = base_heal;
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(ch, &af);

   af.type = gsn_breath_of_endurance;
   af.duration = 2 + char_class_level(ch, CLASS_MAR) / 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_AC;
   af.modifier = -(char_class_level(ch, CLASS_MAR) * 5 + char_class_level(ch, CLASS_MON) * 2);
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(ch, &af);

   act("$n steadies $s breathing, drawing on deep reserves of endurance.", ch, NULL, NULL, TO_ROOM);
   send_to_char("You steady your breathing, drawing on deep reserves of endurance.\n\r", ch);
}
