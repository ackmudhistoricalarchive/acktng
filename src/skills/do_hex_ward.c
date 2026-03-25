#include "globals.h"
#include "magic.h"
#include "skills.h"

void do_hex_ward(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   char arg[MAX_INPUT_LENGTH];

   one_argument(argument, arg);

   if (!is_fighting(ch))
   {
      send_to_char("You can only apply a hex ward when fighting!\n\r", ch);
      return;
   }

   if (!can_use_skill_message(ch, gsn_hex_ward))
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

   if (is_affected(victim, gsn_hex_ward))
   {
      send_to_char("They are already afflicted by a hex ward!\n\r", ch);
      return;
   }

   if (!subtract_energy_cost(ch, gsn_hex_ward))
      return;

   raise_skill(ch, gsn_hex_ward);

   WAIT_STATE(ch, skill_table[gsn_hex_ward].beats);

   ch->cooldown[gsn_hex_ward] = 20;

   af.type = gsn_hex_ward;
   af.duration = 3 + char_class_level(ch, CLASS_NIG) / 5;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_AC;
   af.modifier = char_class_level(ch, CLASS_NIG) * 4 + char_class_level(ch, CLASS_WLK) * 2;
   af.bitvector = 0;
   af.caster = ch;
   affect_to_char(victim, &af);

   act("@@d$n weaves a hex ward around $N, eroding $S defenses!@@N", ch, NULL, victim, TO_NOTVICT);
   act("@@d$n weaves a hex ward around you, eroding your defenses!@@N", ch, NULL, victim, TO_VICT);
   act("@@dYou weave a hex ward around $N, eroding $S defenses!@@N", ch, NULL, victim, TO_CHAR);
}
