#include "globals.h"
#include "magic.h"

bool do_poison(CHAR_DATA *ch, char *argument, int gsn);
void backstab(CHAR_DATA *ch, CHAR_DATA *victim, bool backstab);
bool can_hit_skill(CHAR_DATA *ch, CHAR_DATA *victim, int gsn);
bool subtract_energy_cost(CHAR_DATA *ch, int gsn);

void backstab(CHAR_DATA *ch, CHAR_DATA *victim, bool backstab)
{
   OBJ_DATA *obj;
   int dam;

   if (victim == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
      return;
   }

   if ((victim == NULL) || (victim->is_free != FALSE))
      return;

   obj = get_eq_char(ch, WEAR_HOLD_HAND_R);
   if (obj == NULL || obj->value[3] != 11)
   {
      obj = get_eq_char(ch, WEAR_HOLD_HAND_L);
      if (obj == NULL || obj->value[3] != 11)
      {
         obj = get_eq_char(ch, WEAR_TWO_HANDED);
         if (obj == NULL || obj->value[3] != 11)
         {
            send_to_char("You need to wield a piercing weapon.\n\r", ch);
            return;
         }
      }
   }

   if (IS_NPC(victim) && IS_SET(victim->act, ACT_NO_BODY))
   {
      act("$N has no body to backstab!", ch, NULL, victim, TO_CHAR);
      return;
   }

   if (!backstab && !can_hit_skill(victim, ch, gsn_circle))
   {
      act("$n tries to circle $N, but misses!", ch, NULL, victim, TO_NOTVICT);
      act("You try to circle $N, but miss!", ch, NULL, victim, TO_CHAR);
      act("$N tries to circle you, but misses!", victim, NULL, ch, TO_CHAR);
      WAIT_STATE(ch, skill_table[gsn_circle].beats);
      return;
   }

   if (victim == ch)
   {
      send_to_char("How can you sneak up on yourself?\n\r", ch);
      return;
   }

   if (is_safe(ch, victim))
      return;

   if (backstab && victim->fighting != NULL)
   {
      send_to_char("You can't backstab a fighting person.\n\r", ch);
      return;
   }

   if (backstab)
   {
      if (!subtract_energy_cost(ch, gsn_backstab))
         return;
   }
   else
   {
      if (!subtract_energy_cost(ch, gsn_circle))
         return;
   }

   /*
    * Work out damage
    */
   dam = number_range(obj->value[1], obj->value[2]);
   dam += number_range(get_curr_dex(ch) * get_psuedo_level(ch) / 5,
                       get_curr_dex(ch) * get_psuedo_level(ch) / 2);

   check_killer(ch, victim);

   if (backstab && IS_NPC(victim) && IS_AFFECTED(victim, AFF_SANCTUARY) &&
       (char_class_level(ch, CLASS_ASS) > 0 || char_class_level(ch, CLASS_WLK) > 0) &&
       (number_percent() > 50))
   {
      send_to_char("Critical Success! Your advanced training has succeeded!\n\r", ch);
      act("The white aura around $n fades.", victim, NULL, NULL, TO_ROOM);
      send_to_char("The white aura around you fades.\n\r", victim);
      REMOVE_BIT(victim->affected_by, AFF_SANCTUARY);
   }

   char actbuf[MSL];
   sprintf(actbuf, "$n places $p into the back of $N!!");
   act(actbuf, ch, obj, victim, TO_NOTVICT);
   sprintf(actbuf, "You place $p into the back of $N!!");
   act(actbuf, ch, obj, victim, TO_CHAR);
   sprintf(actbuf, "$N places $p into your back!!");
   act(actbuf, victim, obj, ch, TO_CHAR);

   if (backstab)
      raise_skill(ch, gsn_backstab);
   else
      raise_skill(ch, gsn_circle);

   if (!IS_NPC(ch) && char_class_level(ch, CLASS_CIP) > 0 && number_percent() == 1)
   {
      send_to_room("You hear a large CRACK!\n\r", ch->in_room);
      dam *= 2;
   }

   if (backstab)
   {
      if ((obj = get_eq_char(ch, WEAR_HOLD_HAND_R)) != NULL && obj->value[3] == 11)
      {
         if ((obj = get_eq_char(ch, WEAR_HOLD_HAND_L)) != NULL && obj->value[3] == 11)
         {
            if (IS_SET(stance_app[ch->stance].specials, STANCE_DUAL_BACKSTAB))
            {
               calculate_damage(ch, victim, number_range(dam * 0.95, dam * 1.05), gsn_backstab,
                                ELE_PHYSICAL, TRUE);
            }
         }
      }
      if (!calculate_damage(ch, victim, dam, gsn_backstab, ELE_PHYSICAL, TRUE))
         return;
   }
   else
   {
      if (!calculate_damage(ch, victim, dam * 0.8, gsn_circle, ELE_PHYSICAL, TRUE))
         return;
   }

   if (backstab)
      WAIT_STATE(ch, skill_table[gsn_backstab].beats);
   else
      WAIT_STATE(ch, skill_table[gsn_circle].beats);

   if (is_affected(victim, skill_lookup("poison:quinine")))
   {
      dam = number_range(dam * 1.1, dam * 1.25);
      sprintf(actbuf, "$N screams as the quinine in $M veins is consumed!");
      act(actbuf, ch, obj, victim, TO_NOTVICT);
      sprintf(actbuf, "$N screams as the quinine in $M veins is consumed!");
      act(actbuf, ch, obj, victim, TO_CHAR);
      sprintf(actbuf, "You scream as the quinine in your veins is consumed!");
      act(actbuf, ch, obj, victim, TO_VICT);
      calculate_damage(ch, victim, dam, gsn_poison_quinine, ELE_PHYSICAL | ELE_POISON, TRUE);
      affect_strip(victim, skill_lookup("poison:quinine"));
   }

   if (is_affected(victim, skill_lookup("poison:arsenic")))
   {
      dam = number_range(dam * 1.1, dam * 1.25);
      sprintf(actbuf, "$N screams as the arsenic in $M veins is consumed!");
      act(actbuf, ch, obj, victim, TO_NOTVICT);
      sprintf(actbuf, "$N screams as the arsenic in $M veins is consumed!");
      act(actbuf, ch, obj, victim, TO_CHAR);
      sprintf(actbuf, "You scream as the arsenic in your veins is consumed!");
      act(actbuf, ch, obj, victim, TO_VICT);
      calculate_damage(ch, victim, dam, gsn_poison_arsenic, ELE_PHYSICAL | ELE_POISON, TRUE);
      affect_strip(victim, skill_lookup("poison:arsenic"));
   }

   if (is_affected(victim, skill_lookup("poison:nightshade")))
   {
      dam = number_range(dam * 1.25, dam * 1.4);
      sprintf(actbuf, "$N screams as the nightshade in $M veins is consumed!");
      act(actbuf, ch, obj, victim, TO_NOTVICT);
      sprintf(actbuf, "$N screams as the nightshade in $M veins is consumed!");
      act(actbuf, ch, obj, victim, TO_CHAR);
      sprintf(actbuf, "You scream as the nightshade in your veins is consumed!");
      act(actbuf, ch, obj, victim, TO_VICT);
      calculate_damage(ch, victim, dam, gsn_poison_nightshade, ELE_PHYSICAL | ELE_POISON, TRUE);
      affect_strip(victim, skill_lookup("poison:nightshade"));
   }
}

bool do_poison(CHAR_DATA *ch, char *argument, int gsn)
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   AFFECT_DATA af;

   if (!can_use_skill_message(ch, gsn))
      return FALSE;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Poison whom?\n\r", ch);
      return FALSE;
   }

   victim = get_char_room(ch, arg);

   if (victim == NULL)
   {
      send_to_char("Poison whom?\n\r", ch);
      return FALSE;
   }

   if (!subtract_energy_cost(ch, gsn))
      return FALSE;

   if (is_affected(victim, gsn))
   {
      send_to_char("Your victim is already inflicted with that.\n\r", ch);
      return FALSE;
   }

   WAIT_STATE(ch, skill_table[gsn].beats);

   check_killer(ch, victim);
   set_fighting(victim, ch, TRUE);

   raise_skill(ch, gsn);

   if (!can_hit_skill(ch, victim, gsn))
   {
      sprintf(buf, "You attempt to inflict $N with %s, but fail.", skill_table[gsn].name);
      act(buf, ch, NULL, victim, TO_CHAR);
      sprintf(buf, "$n attempts to inflict you with %s, but fails.", skill_table[gsn].name);
      act(buf, ch, NULL, victim, TO_VICT);
      sprintf(buf, "$n attempts to inflict $N with %s, but fails.", skill_table[gsn].name);
      act(buf, ch, NULL, victim, TO_ROOM);
      return FALSE;
   }

   sprintf(buf, "You inflict $N with %s!", skill_table[gsn].name);
   act(buf, ch, NULL, victim, TO_CHAR);
   sprintf(buf, "$n inflictS you with %s!", skill_table[gsn].name);
   act(buf, ch, NULL, victim, TO_VICT);
   sprintf(buf, "$n inflictS $N with %s!", skill_table[gsn].name);
   act(buf, ch, NULL, victim, TO_ROOM);
   af.type = gsn;
   af.duration = 10;
   af.duration_type = DURATION_ROUND;
   af.location = APPLY_DOT;
   af.modifier = get_psuedo_level(ch) / 2;
   af.bitvector = 0;
   af.element = ELEMENT_PHYSICAL | ELEMENT_POISON;
   af.caster = ch;
   affect_to_char(victim, &af);
   return TRUE;
}
