#include "globals.h"
#include "magic.h"
#include "weapon_bond.h"
#include "headers/socket.h"

void disarm(CHAR_DATA *ch, CHAR_DATA *victim);
int find_door(CHAR_DATA *ch, char *arg);

int disarm_select_weapon_slot(int left_type, int right_type, int two_handed_type)
{
   if (left_type == ITEM_WEAPON)
      return WEAR_HOLD_HAND_L;

   if (right_type == ITEM_WEAPON)
      return WEAR_HOLD_HAND_R;

   if (two_handed_type == ITEM_WEAPON)
      return WEAR_TWO_HANDED;

   return -1;
}

OBJ_DATA *disarm_find_weapon(OBJ_DATA *left, OBJ_DATA *right, OBJ_DATA *two_handed)
{
   int slot = disarm_select_weapon_slot(left == NULL ? -1 : left->item_type,
                                        right == NULL ? -1 : right->item_type,
                                        two_handed == NULL ? -1 : two_handed->item_type);

   if (slot == WEAR_HOLD_HAND_L)
      return left;

   if (slot == WEAR_HOLD_HAND_R)
      return right;

   if (slot == WEAR_TWO_HANDED)
      return two_handed;

   return NULL;
}

void disarm(CHAR_DATA *ch, CHAR_DATA *victim)
{
   OBJ_DATA *obj = NULL;
   AFFECT_DATA af;
   int chance;

   if (!can_use_skill_message(ch, gsn_disarm))
      return;

   if (IS_SET(victim->skills, MOB_DISARM))
   {
      act("$N cannot be disarmed!", ch, NULL, victim, TO_CHAR);
      return;
   }

   set_fighting(ch, victim, TRUE);

   obj = disarm_find_weapon(get_eq_char(victim, WEAR_HOLD_HAND_L),
                            get_eq_char(victim, WEAR_HOLD_HAND_R),
                            get_eq_char(victim, WEAR_TWO_HANDED));

   if (obj == NULL || IS_SET(obj->extra_flags, ITEM_FIST))
   {
      send_to_char("Your opponent is not wielding a weapon.\n\r", ch);
      return;
   }

   /* Bonded weapons cannot be disarmed */
   if (IS_OBJ_BONDED(obj))
   {
      act("$N's bonded blade resists your disarm attempt!", ch, NULL, victim, TO_CHAR);
      act("Your bonded blade holds firm against $n's disarm!", ch, NULL, victim, TO_VICT);
      act("$N's blade flares as it resists $n's disarm attempt!", ch, NULL, victim, TO_NOTVICT);
      WAIT_STATE(ch, 18);
      return;
   }

   WAIT_STATE(ch, 18);

   chance = 0;

   if (IS_SET(obj->extra_flags, ITEM_NODISARM))
      chance -= 80;

   if (can_use_skill(victim, gsn_nodisarm))
      chance -= 50;

   chance += (get_psuedo_level(ch) - get_psuedo_level(victim)) / 2;

   raise_skill(ch, gsn_disarm);

   if (!skill_success(ch, victim, gsn_disarm, chance))
   {
      act("You dodge $n's disarm attempt!", ch, NULL, victim, TO_VICT);
      act("You fail to disarm $N!", ch, NULL, victim, TO_CHAR);
      act("$N dodges $n's disarm attempt!", ch, NULL, victim, TO_NOTVICT);
      return;
   }

   act("$n DISARMS you!", ch, NULL, victim, TO_VICT);
   act("You disarm $N!", ch, NULL, victim, TO_CHAR);
   act("$n DISARMS $N!", ch, NULL, victim, TO_NOTVICT);

   unequip_char(ch, obj);
   /* v2 WebSocket: push updated equipment to disarmed player */
   if (!IS_NPC(victim) && victim->desc && victim->desc->websocket_active)
      ws_send_equipment(victim->desc, victim);

   af.type = skill_lookup("disarm");
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.duration_type = DURATION_ROUND;
   af.duration = 2;
   af.bitvector = 0;
   affect_to_char(victim, &af);
}
