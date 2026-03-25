#include "globals.h"
#include "magic.h"

int get_max_combo(CHAR_DATA *ch);
bool can_hit_skill(CHAR_DATA *ch, CHAR_DATA *victim, int gsn);
void war_attack(CHAR_DATA *ch, char *argument, int gsn);
bool subtract_energy_cost(CHAR_DATA *ch, int gsn);
void pug_attack(CHAR_DATA *ch, char *argument, int gsn);

int chi_skill_cost(int base_cost, int cooldown)
{
   if (cooldown > 0)
      return base_cost * 2;

   return base_cost;
}

bool chi_should_block_mindoverbody(bool has_mindoverbody, bool has_named_mindoverbody)
{
   return has_mindoverbody || has_named_mindoverbody;
}

int get_chi(CHAR_DATA *ch)
{
   int max_chi = 0;

   if (char_class_level(ch, CLASS_PUG) > 0)
      max_chi = 15;

   if (!IS_NPC(ch) && ch->pcdata->reincarnations[CLASS_PUG] >= 20)
      max_chi = 15;

   if (max_chi == 0)
      return max_chi;

   if (char_class_level(ch, CLASS_MON) > 0 || char_class_level(ch, CLASS_BRA) > 0)
      max_chi = 20;

   if (!IS_NPC(ch) && (ch->pcdata->reincarnations[MAX_CLASS + CLASS_MON] +
                           ch->pcdata->reincarnations[MAX_CLASS + CLASS_BRA] >=
                       20))
      max_chi = 20;

   if (char_class_level(ch, CLASS_MAR) > 0)
      max_chi = 25;

   if (!IS_NPC(ch) && ch->pcdata->reincarnations[MAX_CLASS + MAX_REMORT + CLASS_MAR] >= 20)
      max_chi = 25;

   if (ch->chi > max_chi)
      ch->chi = max_chi;

   return ch->chi;
}

void pug_attack(CHAR_DATA *ch, char *argument, int gsn)
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];
   char actbuf[MAX_STRING_LENGTH];
   int dam;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      strcpy(arg, "enemy");
   }

   if (!can_use_skill_message(ch, gsn))
      return;

   if (((victim = get_char_room(ch, argument)) == NULL) && !is_fighting(ch))
   {
      send_to_char("No such victim!\n\r", ch);
      return;
   }

   if (victim == NULL)
      victim = ch->fighting;

   if (!subtract_energy_cost(ch, gsn))
      return;

   dam = number_range(get_psuedo_level(ch) * get_curr_str(ch) / 30,
                      get_psuedo_level(ch) * get_curr_str(ch) / 20);

   if (dam < 1)
      dam = 1;

   if (gsn != gsn_spinfist)
      ch->chi++;

   if (gsn == gsn_palmstrike)
   {
      dam *= 4;
      ch->cooldown[gsn_palmstrike] = 5;
   }

   WAIT_STATE(ch, skill_table[gsn].beats);

   raise_skill(ch, gsn);

   int element = ELE_PHYSICAL;

   if (gsn == gsn_aurabolt)
      element = ELE_PHYSICAL | ELE_HOLY;

   if (IS_NPC(ch))
      dam /= 2;

   check_killer(ch, victim);
   if (can_hit_skill(ch, victim, gsn))
   {
      char actbuf[MSL];
      sprintf(actbuf, "$n %ss $N!!", skill_table[gsn].name);
      act(actbuf, ch, NULL, victim, TO_NOTVICT);
      sprintf(actbuf, "$n %ss you really hard!!", skill_table[gsn].name);
      act(actbuf, ch, NULL, victim, TO_VICT);
      sprintf(actbuf, "You %s $N!!", skill_table[gsn].name);
      act(actbuf, ch, NULL, victim, TO_CHAR);

      calculate_damage(ch, victim, dam, gsn, element, TRUE);
   }
   else
   {
      sprintf(actbuf, "$n tries to %s $N, but misses!", skill_table[gsn].name);
      act(actbuf, ch, NULL, victim, TO_NOTVICT);
      sprintf(actbuf, "$N tries to %s you, but misses!", skill_table[gsn].name);
      act(actbuf, ch, NULL, victim, TO_VICT);
      sprintf(actbuf, "You try to %s $N, but miss!", skill_table[gsn].name);
      act(actbuf, ch, NULL, victim, TO_CHAR);
      set_fighting(victim, ch, TRUE);
   }
}
