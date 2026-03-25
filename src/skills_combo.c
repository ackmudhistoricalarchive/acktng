#include "globals.h"
#include "magic.h"

bool combo(CHAR_DATA *ch, CHAR_DATA *victim, int gsn);
bool is_valid_finisher(CHAR_DATA *ch);
bool is_ready_finisher(CHAR_DATA *ch);
void reset_combo(CHAR_DATA *ch);
int get_max_combo(CHAR_DATA *ch);
bool subtract_energy_cost(CHAR_DATA *ch, int gsn);
bool can_hit_skill(CHAR_DATA *ch, CHAR_DATA *victim, int gsn);
void stun(CHAR_DATA *ch, CHAR_DATA *victim);
void disarm(CHAR_DATA *ch, CHAR_DATA *victim);

int combo_damage_multiplier_for_max_combo(int max)
{
   if (max == 5)
      return 18;

   if (max == 6)
      return 15;

   return 23;
}

bool combo_has_duplicate_skill(const int *combo_values, int max, int gsn)
{
   int i;

   for (i = 0; i < max; i++)
   {
      if (combo_values[i] == gsn)
         return TRUE;
   }

   return FALSE;
}

const char *war_attack_miss_to_victim_format(void)
{
   return "$n tries to %s you, but misses!";
}

void war_attack(CHAR_DATA *ch, char *argument, int gsn)
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

   dam = number_range(get_psuedo_level(ch) * get_curr_str(ch) / 5,
                      get_psuedo_level(ch) * get_curr_str(ch) / 4);

   if (dam < 1)
      dam = 1;

   WAIT_STATE(ch, skill_table[gsn].beats);

   raise_skill(ch, gsn);

   int element = ELE_PHYSICAL;

   if (gsn == gsn_holystrike)
   {
      element = element | ELE_HOLY;
      dam *= 1.25;
   }

   if (gsn == gsn_fleche)
      dam *= 1.5;

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
      combo(ch, victim, gsn);
   }
   else
   {
      sprintf(actbuf, "$n tries to %s $N, but misses!", skill_table[gsn].name);
      act(actbuf, ch, NULL, victim, TO_NOTVICT);
      sprintf(actbuf, war_attack_miss_to_victim_format(), skill_table[gsn].name);
      act(actbuf, ch, NULL, victim, TO_VICT);
      sprintf(actbuf, "You try to %s $N, but miss!", skill_table[gsn].name);
      act(actbuf, ch, NULL, victim, TO_CHAR);
      set_fighting(victim, ch, TRUE);
   }
}

void stun(CHAR_DATA *ch, CHAR_DATA *victim)
{
   if (ch == NULL || victim == NULL)
      return;

   /*if( ( number_percent(  ) + number_percent(  ) ) < ( chance + chance2 ) )
   {*/
   /*
    * Success
    */
   act("You slam into $N, leaving $M stunned.", ch, NULL, victim, TO_CHAR);
   act("$n slams into you, leaving you stunned.", ch, NULL, victim, TO_VICT);
   act("$n slams into $N, leaving $M stunned.", ch, NULL, victim, TO_NOTVICT);

   victim->stunTimer += number_range(2, 4);
   victim->position = POS_STUNNED;

   /*}
   else
   {
      *
       * Ooops!
       *
      act( "You try to slam into $N, but miss and fall onto your face!", ch, NULL, victim, TO_CHAR
   ); act( "$n tries to slam into you, but misses and falls onto $s face!", ch, NULL, victim,
   TO_VICT ); act( "$n tries to slam into $N, but misses and falls onto $s face!", ch, NULL, victim,
   TO_NOTVICT ); return;
   }*/
   return;
}

bool combo(CHAR_DATA *ch, CHAR_DATA *victim, int gsn)
{
   int i;
   int max = get_max_combo(ch);
   int punch_cnt = 0, knee_cnt = 0, headbutt_cnt = 0, kick_cnt = 0, disarm_cnt = 0, dirt_cnt = 0,
       bash_cnt = 0, charge_cnt = 0, fleche_cnt = 0, holystrike_cnt = 0;
   int mult;

   if (max < 4)
      return FALSE;

   if (combo_has_duplicate_skill(ch->combo, max, gsn))
   {
      send_to_char("Bad combo.\n\r", ch);
      reset_combo(ch);
   }

   for (i = 0; i < max; i++)
   {
      if (ch->combo[i] == -1 || ch->combo[i] == 0)
      {
         ch->combo[i] = gsn;
         break;
      }
   }

   if (is_valid_finisher(ch))
      send_to_char("@@yYou are ready to perform a finisher!@@N\n\r", ch);

   if (!is_ready_finisher(ch))
      return FALSE;

   mult = combo_damage_multiplier_for_max_combo(max);

   // Calc our chances
   for (int i = 0; i < max; i++)
   {
      if (ch->combo[i] == gsn_punch)
         punch_cnt += mult;

      if (ch->combo[i] == gsn_knee)
         knee_cnt += mult;

      if (ch->combo[i] == gsn_headbutt)
         headbutt_cnt += mult;

      if (ch->combo[i] == gsn_kick)
         kick_cnt += mult;

      if (ch->combo[i] == gsn_disarm)
         disarm_cnt += mult;

      if (ch->combo[i] == gsn_dirt)
         dirt_cnt += mult;

      if (ch->combo[i] == gsn_bash)
         bash_cnt += mult;

      if (ch->combo[i] == gsn_charge)
         charge_cnt += mult;

      if (ch->combo[i] == gsn_fleche)
         fleche_cnt += mult;

      if (ch->combo[i] == gsn_holystrike)
         holystrike_cnt += mult;
   }

   send_to_char("@@yCombo triggered!@@N\n\r", ch);
   act("You begin a combo attack!", ch, NULL, victim, TO_CHAR);
   act("$n begins a combo attack!", ch, NULL, victim, TO_ROOM);

   int max_attacks = max - 3;

   if (ch->combo[0] == gsn_fleche)
      max_attacks++;

   int max_combo = 6;

   if (char_class_level(ch, CLASS_KNI) > 0 || char_class_level(ch, CLASS_SWO) > 0)
      max_combo += 2;

   if (char_class_level(ch, CLASS_CRU) > 0)
      max_combo += 2;

   int combo_chance = 25;
   if (gsn == gsn_holystrike || gsn == gsn_fleche)
      combo_chance += 10;

   for (int i = 0; i < max_combo && i < max_attacks; i++)
   {
      reset_combo(ch);

      if (!is_fighting(ch))
         break;

      if (i == 0 && number_percent() < combo_chance)
         max_attacks++;
      if (number_percent() < combo_chance)
         max_attacks++;

      int roll = number_percent();
      int chance = 0;

      if (roll < chance + punch_cnt)
      {
         war_attack(ch, victim->name, gsn_punch);
         continue;
      }

      chance += punch_cnt;

      if (roll < chance + kick_cnt)
      {
         war_attack(ch, victim->name, gsn_kick);
         continue;
      }

      chance += kick_cnt;

      if (roll < chance + knee_cnt)
      {
         war_attack(ch, victim->name, gsn_knee);
         continue;
      }

      chance += knee_cnt;

      if (roll < chance + headbutt_cnt)
      {
         war_attack(ch, victim->name, gsn_headbutt);
         continue;
      }

      chance += headbutt_cnt;

      if (roll < chance + disarm_cnt)
      {
         disarm(ch, victim);
         continue;
      }

      chance += disarm_cnt;

      if (roll < chance + dirt_cnt)
      {
         if (can_see(victim, ch))
            do_dirt(ch, victim->name);
         else
            i--;
         continue;
      }

      chance += dirt_cnt;

      if (roll < chance + bash_cnt)
      {
         war_attack(ch, victim->name, gsn_bash);
         continue;
      }

      chance += bash_cnt;

      if (roll < chance + charge_cnt)
      {
         war_attack(ch, victim->name, gsn_charge);
         continue;
      }

      chance += charge_cnt;

      if (roll < chance + fleche_cnt)
      {
         war_attack(ch, victim->name, gsn_fleche);
         continue;
      }

      chance += fleche_cnt;

      if (roll < chance + holystrike_cnt)
      {
         war_attack(ch, victim->name, gsn_holystrike);
         continue;
      }

      if (roll < 95)
      {
         stun(ch, ch->fighting);
         continue;
      }

      // 95+
      max_attacks += 2;
   }

   reset_combo(ch);

   return TRUE;
}

int get_combo_count(CHAR_DATA *ch)
{
   int cnt;

   for (cnt = 0; cnt < get_max_combo(ch); cnt++)
   {
      if (ch->combo[cnt] < 1 || ch->combo[cnt] > MAX_SKILL)
      {
         return cnt;
      }
   }

   return cnt;
}

bool is_valid_finisher(CHAR_DATA *ch)
{
   if (get_combo_count(ch) == (get_max_combo(ch) - 1))
      return TRUE;

   return FALSE;
}

bool is_ready_finisher(CHAR_DATA *ch)
{
   if (get_combo_count(ch) >= get_max_combo(ch))
      return TRUE;

   return FALSE;
}

void reset_combo(CHAR_DATA *ch)
{
   for (int i = 0; i < MAX_COMBO; i++)
      ch->combo[i] = -1;
}

int get_max_combo(CHAR_DATA *ch)
{
   int max;

   if (char_class_level(ch, CLASS_WAR) > 0)
      max = 4;

   if (!IS_NPC(ch) && ch->pcdata->reincarnations[CLASS_WAR] >= 20)
      max = 4;

   if (char_class_level(ch, CLASS_CRU) > 0 && max > 0)
      max++;
   else if (!IS_NPC(ch) &&
            (ch->pcdata->reincarnations[MAX_CLASS + MAX_REMORT + CLASS_CRU] >= 20 ||
             ch->pcdata->reincarnations[MAX_CLASS + MAX_REMORT + CLASS_MAR] >= 20) &&
            max > 0)
      max++;

   if ((char_class_level(ch, CLASS_KNI) > 0 || char_class_level(ch, CLASS_SWO) > 0) && max > 0)
      max++;
   else if (max > 0 && (!IS_NPC(ch) && (ch->pcdata->reincarnations[MAX_CLASS + CLASS_KNI] +
                                            ch->pcdata->reincarnations[MAX_CLASS + CLASS_SWO] >=
                                        20)))
      max++;

   return max;
}
