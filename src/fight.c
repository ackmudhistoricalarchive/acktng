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

#include <stdio.h>
#include <stdlib.h> /* For div_t, div() */
#include <string.h>
#include <time.h>
#include "globals.h"
#include "magic.h"
#include "cloak.h"
#include "tables.h"
#include "sentinel.h"
#include <math.h>

/*
 * Local functions.
 */
bool check_avoidance args((CHAR_DATA * ch, CHAR_DATA *victim));
void check_killer args((CHAR_DATA * ch, CHAR_DATA *victim));
void dam_message args((CHAR_DATA * ch, CHAR_DATA *victim, int dam, int dt, bool critical));
void death_message args((CHAR_DATA * ch, CHAR_DATA *victim, int dt, int max_dt));
void death_cry args((CHAR_DATA * ch));
void group_gain args((CHAR_DATA * ch, CHAR_DATA *victim));
void do_knee args((CHAR_DATA * ch, char *argument));
bool do_lifesteal args((CHAR_DATA * ch, CHAR_DATA *victim, OBJ_DATA *wield, bool dual, int dam));
bool shortfight_summary_recipient_matches args((CHAR_DATA * rch, CHAR_DATA *ch, CHAR_DATA *victim,
                                                bool expected_shortfight));
bool shortfight_can_broadcast_room_summary args((CHAR_DATA * ch, CHAR_DATA *victim));
void shortfight_emit_autoattack_summary args((CHAR_DATA * ch, CHAR_DATA *victim));
bool shortfight_should_suppress_watched_autoattack args((int observer_is_npc,
                                                         int observer_has_shortfight,
                                                         int observer_is_fighting));

bool should_summon_assist_master_round args((int is_npc, int is_charmed, int has_master,
                                             int master_fighting, int same_room,
                                             int is_player_summon, int can_see_master_target));
bool should_summon_cast_round args((int is_npc, int is_player_summon, int is_fighting,
                                    int has_spec_fun, int should_cast_now));

bool is_safe args((CHAR_DATA * ch, CHAR_DATA *victim));
void make_corpse args((CHAR_DATA * ch, char *argument));
void one_hit args((CHAR_DATA * ch, CHAR_DATA *victim, int dt));
void raw_kill args((CHAR_DATA * victim, char *argument));
void set_fighting args((CHAR_DATA * ch, CHAR_DATA *victim, bool check));
void disarm args((CHAR_DATA * ch, CHAR_DATA *victim, OBJ_DATA *obj));
void trip args((CHAR_DATA * ch, CHAR_DATA *victim));
void check_adrenaline args((CHAR_DATA * ch, sh_int damage));
void obj_damage args((OBJ_DATA * obj, CHAR_DATA *victim, int dam));

int do_damage args((CHAR_DATA * ch, CHAR_DATA *victim, int dam, int dt, int element,
                    bool critical));
int damage args((CHAR_DATA * ch, CHAR_DATA *victim, int dam, int dt));
int get_counter args((CHAR_DATA * ch));
int get_evasion_piercing args((CHAR_DATA * ch));

bool shortfight_should_suppress_watched_autoattack(int observer_is_npc, int observer_has_shortfight,
                                                   int observer_is_fighting)
{
   return !observer_is_npc && observer_has_shortfight && !observer_is_fighting;
}

bool should_summon_assist_master_round(int is_npc, int is_charmed, int has_master,
                                       int master_fighting, int same_room, int is_player_summon,
                                       int can_see_master_target)
{
   return is_npc && is_charmed && has_master && master_fighting && same_room && is_player_summon &&
          can_see_master_target;
}

bool should_summon_cast_round(int is_npc, int is_player_summon, int is_fighting, int has_spec_fun,
                              int should_cast_now)
{
   return is_npc && is_player_summon && is_fighting && has_spec_fun && should_cast_now;
}

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update(void)
{
   CHAR_DATA *ch;
   CHAR_DATA *ch_next;
   CHAR_DATA *victim;
   CHAR_DATA *rch;
   CHAR_DATA *rch_next;

   CREF(ch_next, CHAR_NEXT);
   for (ch = first_char; ch; ch = ch_next)
   {
      ch_next = ch->next;

      if (ch->position == POS_STUNNED)
      {
         if (ch->wait > 0)
         {
            ch->wait -= 1;
            continue;
         }
         else
         {
            ch->position = POS_STANDING;
            send_to_char("You are no longer stunned.\n\r", ch);
            continue;
         }
      }

      /* slight damage for players in a speeded stance, simulates fatigue */

      if (!IS_NPC(ch) && (stance_app[ch->stance].speed_mod > 1))
      {
         ch->hit = UMAX(10, ch->hit - number_range(get_psuedo_level(ch) * 5 / 1000,
                                                   get_psuedo_level(ch) * 10 / 1000));
      }

      if (ch->stunTimer > 0)
      {
         ch->stunTimer--;
         continue;
      }

      area_resetting_global = TRUE;

      if (IS_NPC(ch) && ch->hit < 0)
      {
         ch->position = POS_DEAD;
         if (!is_fighting(ch))
            if (ch->in_room != NULL)
               act("Suddenly, $n is enveloped in a @@mBeam of light@@N, and is gone!", ch, NULL,
                   NULL, TO_ROOM);
         extract_char(ch, TRUE);
         continue;
      }
      area_resetting_global = FALSE;

      /*
       * Hunting mobs.
       * -S- Mod: use flags to work out what to do....
       */
      if (IS_NPC(ch) && !is_fighting(ch) && IS_AWAKE(ch))
      {
         mob_hunt(ch);
         continue;
      }
      if ((victim = ch->fighting) == NULL || ch->in_room == NULL)
      {
         continue;
      }

      if ((IS_AWAKE(ch)) && (ch->is_free == FALSE) && (victim->is_free == FALSE) &&
          (ch->in_room != NULL) && (victim->in_room != NULL) && (ch->in_room == victim->in_room))
         multi_hit(ch, victim, TYPE_UNDEFINED);
      else
         stop_fighting(ch, FALSE);

      if ((victim = ch->fighting) == NULL)
         continue;

      /*
       * Assisting
       */
      CREF(rch_next, CHAR_NEXTROOM);
      for (rch = ch->in_room->first_person; rch != NULL; rch = rch_next)
      {
         rch_next = rch->next_in_room;

         if (IS_AWAKE(rch) && rch->fighting == NULL)
         {
            if (should_summon_assist_master_round(
                    IS_NPC(rch), IS_AFFECTED(rch, AFF_CHARM), rch->master != NULL,
                    rch->master != NULL && rch->master->fighting != NULL,
                    rch->master != NULL && rch->in_room == rch->master->in_room,
                    is_player_summon_special(rch->spec_fun),
                    rch->master != NULL && rch->master->fighting != NULL &&
                        can_see(rch, rch->master->fighting)))
            {
               set_fighting(rch, rch->master->fighting, TRUE);

               if (rch->spec_fun != NULL)
                  (*rch->spec_fun)(rch);
               continue;
            }

            if (IS_AFFECTED(rch, AFF_CHARM))
               continue;
            if (!IS_NPC(rch) && !IS_NPC(ch) && IS_SET(rch->config, CONFIG_AUTOASSIST) &&
                is_same_group(rch, ch))
            {
               do_assist(rch, ch->name);
            }
            else if (!IS_NPC(rch) && !IS_NPC(victim) && IS_SET(rch->config, CONFIG_AUTOASSIST) &&
                     is_same_group(rch, victim))
            {
               do_assist(rch, victim->name);
            }

            else if (IS_NPC(rch) && !IS_SET(rch->act, ACT_NOASSIST))
            {
               if ((rch->pIndexData == victim->pIndexData) /* is it the same as a target here?  */
                   || is_same_group(rch, victim))
               {
                  if (IS_NPC(rch) && IS_NPC(victim) && can_see(rch, victim->fighting) &&
                      (get_psuedo_level(ch) - get_psuedo_level(victim)) < 10)
                  {
                     if ((victim->fighting != NULL) && (rch->fighting == NULL))
                     {
                        char actbuf[MSL];
                        sprintf(actbuf, "$n screams, 'BANZAI!! $N must be assisted!!'");
                        act(actbuf, rch, NULL, victim, TO_ROOM);
                        sprintf(actbuf, "You scream, 'BANZAI!! $N must be assisted!!'");
                        act(actbuf, rch, NULL, victim, TO_CHAR);
                        set_fighting(rch, victim->fighting, TRUE);
                     }
                  }
               }
               if ((rch->pIndexData == ch->pIndexData) /* is it the same as a target here?  */
                   || is_same_group(rch, ch))
               {
                  if (IS_NPC(rch) && IS_NPC(ch) && can_see(rch, ch->fighting) &&
                      (get_psuedo_level(ch) - get_psuedo_level(victim)) < 10)
                  {
                     if ((victim->fighting != NULL) && (rch->fighting == NULL))
                     {
                        char actbuf[MSL];
                        sprintf(actbuf, "$n screams, 'BANZAI!! $N must be assisted!!'");
                        act(actbuf, rch, NULL, ch, TO_ROOM);
                        sprintf(actbuf, "You scream, 'BANZAI!! $N must be assisted!!'");
                        act(actbuf, rch, NULL, ch, TO_CHAR);
                        set_fighting(rch, ch->fighting, TRUE);
                     }
                  }
               }
            }
         }
      }
      char_unreference(&rch_next);
   }
   char_unreference(&ch_next);
   return;
}

/*
 * Do one group of attacks.
 */
void multi_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
   int chance;
   OBJ_DATA *wield1 = NULL;
   OBJ_DATA *wield2 = NULL;
   int dual_chance = 0;

   if (IS_SET(stance_app[ch->stance].specials, STANCE_NO_HIT))
      return;

   if (should_summon_cast_round(IS_NPC(ch), is_player_summon_special(ch->spec_fun), is_fighting(ch),
                                ch->spec_fun != NULL, TRUE))
   {
      (*ch->spec_fun)(ch);
      return;
   }

   if (IS_NPC(ch) && is_player_summon_special(ch->spec_fun))
      return;

   if ((((wield1 = get_eq_char(ch, WEAR_HOLD_HAND_L)) != NULL) &&
        (wield1->item_type == ITEM_WEAPON)) &&
       (((wield2 = get_eq_char(ch, WEAR_HOLD_HAND_R)) != NULL) &&
        (wield2->item_type == ITEM_WEAPON)))
      dual_chance = 15;

   int hits = 0;

   if (IS_NPC(ch))
   {
      if (IS_SET(ch->skills, MOB_SECOND))
         hits++;
      if (IS_SET(ch->skills, MOB_THIRD))
         hits++;
      if (IS_SET(ch->skills, MOB_FOURTH))
         hits++;
      if (IS_SET(ch->skills, MOB_FIFTH))
         hits++;
      if (IS_SET(ch->skills, MOB_SIXTH))
         hits++;

      if (IS_SET(ch->act, ACT_SOLO))
         hits++;
      if (IS_SET(ch->act, ACT_BOSS))
         hits += 2;

      if (ch->level > 30)
         hits++;
      if (ch->level > 60)
         hits++;
      if (ch->level > 90)
         hits++;
      if (ch->level > 120)
         hits++;
      if (ch->level > 150)
         hits++;
   }
   else
   {
      // Remort hits
      if (char_class_level(ch, CLASS_PAL) > 0)
         hits++;
      if (char_class_level(ch, CLASS_ASS) > 0)
         hits++;
      if (char_class_level(ch, CLASS_WLK) > 0)
         hits++;
      if (char_class_level(ch, CLASS_KNI) > 0)
         hits++;
      if (char_class_level(ch, CLASS_SWO) > 0)
         hits++;

      if (char_class_level(ch, CLASS_NIG) > 0)
         hits++;
      if (char_class_level(ch, CLASS_TEM) > 0)
         hits++;
      if (char_class_level(ch, CLASS_MAR) > 0)
         hits++;
      if (char_class_level(ch, CLASS_CRU) > 0)
         hits++;

      // Mort hits
      for (int i = 0; i < 4; i++)
      {
         if (ch->mortal_class[i] < 0)
            continue;
         if (gclass_table[ch->mortal_class[i]].attr_prime == APPLY_INT ||
             gclass_table[ch->mortal_class[i]].attr_prime == APPLY_WIS)
            continue;
         if (ch->mortal_level[i] > 10)
            hits++;
      }
   }

   short_fight_round_begin(ch, victim);

   // First hit
   one_hit(ch, victim, dt);

   if (ch->fighting != victim)
   {
      shortfight_emit_autoattack_summary(ch, victim);
      return;
   }

   for (int i = 0; i < hits; i++)
   {
      chance = 80 - (i * 10);
      chance += stance_app[ch->stance].speed_mod;
      chance += dual_chance;

      chance += get_speed(ch);

      int calc_chance = number_percent();

      if (chance > calc_chance)
      {
         one_hit(ch, victim, dt);

         if (ch->fighting != victim)
            break;
      }
   }

   /* Public society: Harbor Syndics longshoreman extra attack */
   if (pub_society_extra_attack(ch) && ch->fighting == victim)
      one_hit(ch, victim, dt);

   if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_TAIL) && number_percent() < 25)
      one_hit(ch, victim, TYPE_HIT + 14);

   shortfight_emit_autoattack_summary(ch, victim);

   if (!IS_NPC(ch) && ch->stance > 0 &&
       ((IS_SET(stance_app[victim->stance].specials, STANCE_NINJA))))
   {
      send_to_char("You step out of the shadows.\n\r", ch);
      ch->stance = 0;
      act("$n steps out of the Shadows!", ch, NULL, NULL, TO_ROOM);
   }
   return;
}

#define ZERO_RATIO 20
#define ONE_STEPS 100l
#define MAX_IX 120
#define MAX_DAM_MOD 1.4

/*
 * Hit one guy once.
 */
void one_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
   OBJ_DATA *wield;
   OBJ_DATA *dualwield = NULL;
   int victim_ac;
   int remort_bonus;
   int dam = 0;
   int diceroll;
   int ix;
   float dam_mod;
   extern const float hr_damTable[121];
   /*
    * Can't beat a dead char!
    * Guard against weird room-leavings.
    */

   if (ch == NULL || victim == NULL)
      return;

   if (victim->position == POS_DEAD || ch->in_room != victim->in_room)
      return;

   /* Arbiter: contempt of court — gain testimony when testimony target attacks allies */
   if (!IS_NPC(victim) && victim->in_room != NULL)
   {
      CHAR_DATA *arb;
      for (arb = victim->in_room->first_person; arb != NULL; arb = arb->next_in_room)
      {
         if (arb == victim || arb == ch || IS_NPC(arb))
            continue;
         if (!can_use_skill(arb, gsn_contempt_of_court))
            continue;
         if (arb->testimony_target != ch)
            continue;
         if (!is_same_group(arb, victim) || arb == victim)
            continue;
         /* ch is attacking victim, and arb has testimony on ch */
         add_testimony(arb, 2);
         break;
      }
   }

   /*
    * Figure out the type of damage message.
    */
   wield = get_eq_char(ch, WEAR_HOLD_HAND_L);
   if (IS_WEAPON(wield))
      dualwield = get_eq_char(ch, WEAR_HOLD_HAND_R);
   if (!IS_WEAPON(wield))
      wield = get_eq_char(ch, WEAR_HOLD_HAND_R);
   if (!IS_WEAPON(wield))
      wield = get_eq_char(ch, WEAR_TWO_HANDED);
   if (!IS_WEAPON(wield))
      wield = NULL;

   if (dt == TYPE_UNDEFINED)
   {
      dt = TYPE_HIT;
      if (wield != NULL && wield->item_type == ITEM_WEAPON &&
          !IS_SET(wield->extra_flags, ITEM_FIST))
         dt += wield->value[3];
   }

   if (dt == TYPE_HIT && (wield == NULL || IS_SET(wield->extra_flags, ITEM_FIST)))
   {
      if (IS_NPC(ch) && IS_SET(ch->skills, MOB_MARTIAL))
      {
         if (number_percent() < 75)
            dt = TYPE_MARTIAL;
      }

      if (!IS_NPC(ch) && can_use_skill(ch, gsn_martial_arts))
      {
         if (number_percent() < (char_class_level(ch, CLASS_PUG) / 2) + 40)
            dt = TYPE_MARTIAL;
      }
   }

   victim_ac = get_ac(victim);
   if (!can_see(ch, victim))
      victim_ac -= 200;

   if (dt == gsn_backstab || dt == gsn_circle)
      victim_ac += 300;

   diceroll =
       number_range((get_psuedo_level(ch) * 5), (get_psuedo_level(ch) * 21)) + get_hitroll(ch);

   if (!IS_NPC(ch))
      diceroll += number_range(get_psuedo_level(ch), (get_psuedo_level(ch) * 1));
   if ((remort_bonus = get_psuedo_level(ch) > 100))
      diceroll += remort_bonus * 1;

   if (IS_NPC(ch))
   {
      diceroll += (get_psuedo_level(ch) * 5);
      if (IS_SET(ch->act, ACT_SOLO))
         diceroll += (get_psuedo_level(ch) * 5);
      if (IS_SET(ch->act, ACT_BOSS))
         diceroll += (get_psuedo_level(ch) * 8);
   }
   diceroll += cloak_adept_hitroll_bonus(ch);

   /* Player vs player bonus, to handle unbalanced hitroll vs ac */
   if (!IS_NPC(ch) && !IS_NPC(victim) && get_psuedo_level(ch) > 80 && get_psuedo_level(victim) > 80)
      diceroll += number_range(1000, 2000);
   if (IS_NPC(ch) && victim_ac < -3000 && get_psuedo_level(ch) > 110 && (number_range(0, 100) < 10))
      diceroll += 3000;

   if (victim_ac > -100)
   {
      if (diceroll + victim_ac < 0)
         ix = -1;
      else
         ix = ZERO_RATIO;
   }
   else
   {
      /*
       * This finds the ratio of excess hit roll to AC, and
       * breaks it into steps as defined by constants above,
       * for use in damage modifier lookup table.
       * Long int calculation avoids overflow problems.
       */
      ix = -(ONE_STEPS * (long int)(diceroll + victim_ac)) / (long int)victim_ac;
      ix += ZERO_RATIO;
   }

   if (ix < 0)
   {
      dam_mod = 0.0;

      damage(ch, victim, 0, dt);
      tail_chain();
      return;
   }
   else if (ix <= MAX_IX)
      dam_mod = hr_damTable[ix];
   else
      dam_mod = MAX_DAM_MOD;
   /*
    * Hit.
    * Calc damage.
    * Tried to make it easy for players to hit mobs... --Stephen
    */
   int dam_bonus = get_curr_str(ch) * get_psuedo_level(ch) / 100;

   if (wield != NULL && IS_SET(wield->extra_flags, ITEM_TWO_HANDED))
      dam = number_range(wield->value[1] * 3, wield->value[2] * 3) + dam_bonus;
   else if (wield != NULL && !IS_SET(wield->extra_flags, ITEM_FIST))
      dam = number_range(wield->value[1], wield->value[2]) + dam_bonus;
   else if (IS_NPC(ch))
   {
      if (dt == TYPE_MARTIAL || dt == gsn_counter)
         dam = number_range(ch->level / 3, ch->level / 2) + dam_bonus;
      else
         dam = number_range(ch->level / 3, ch->level / 2) + dam_bonus;

      if (IS_SET(ch->act, ACT_SOLO))
         dam *= 1.5;
      if (IS_SET(ch->act, ACT_BOSS))
         dam *= 2;
   }
   else
   {
      if (dt == TYPE_MARTIAL || dt == gsn_counter)
      {
         if (can_use_skill(ch, gsn_bare_hand))
         {
            if (wield != NULL)
               dam_bonus += number_range(wield->value[1], wield->value[2]);
            dam_bonus += dam_bonus * char_class_level(ch, CLASS_MON) / 75;
            dam_bonus += dam_bonus * char_class_level(ch, CLASS_BRA) / 75 * 0.75;
            dam_bonus += dam_bonus * char_class_level(ch, CLASS_MAR) / 50;

            dam += number_range(get_psuedo_level(ch) / 3, get_psuedo_level(ch) / 2) + dam_bonus;
         }
         else
            dam = number_range(2, get_psuedo_level(ch) / 4) + dam_bonus;
      }
      else
         dam += UMAX(number_range(2, 4), get_psuedo_level(ch) / 4);
   }

   dam = dam * dam_mod;

   dam = calculate_damage(ch, victim, dam, dt, ELE_PHYSICAL, TRUE);

   bool stole_life = FALSE;
   if ((wield) && (dam > 0) && ((IS_OBJ_STAT(wield, ITEM_LIFESTEALER))))
      stole_life = do_lifesteal(ch, victim, wield, FALSE, dam);
   if (!stole_life && dualwield && dam > 0 && IS_OBJ_STAT(dualwield, ITEM_LIFESTEALER))
      do_lifesteal(ch, victim, dualwield, TRUE, dam);
}

/*
 * Inflict damage from a hit.
 */
int damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt)
{
   return do_damage(ch, victim, dam, dt, ELE_PHYSICAL, FALSE);
}

bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim)
{
   if (deathmatch)
      return FALSE; /* Deathmatch? Anything goes!! */

   /*we are going to have safe rooms no attacks at all  SRZ 2 Jul 96
       if ( IS_NPC(ch) || IS_NPC(victim) )
      return FALSE;
   */

   /*
    * No PC vs. PS attacks in safe rooms
    */
   if ((victim->in_room != NULL) && IS_SET(victim->in_room->room_flags, ROOM_SAFE))
   {
      send_to_char("Not a chance!  This is a safe room.\n\r", ch);
      return TRUE;
   }

   if (IS_SET(victim->act, PLR_KILLER))
      return FALSE;
   if (IS_SET(victim->act, PLR_THIEF))
      return FALSE;
   if (!IS_NPC(ch) && !IS_NPC(victim) && IS_SET(victim->pcdata->pflags, PFLAG_PKOK) &&
       IS_SET(ch->pcdata->pflags, PFLAG_PKOK))
      return FALSE;

   if (((victim->level < 10) || (victim->level + 20 < ch->level)) && (!IS_NPC(victim)) &&
       (!IS_NPC(ch)))
   {
      send_to_char("The Gods prevent your foul deed.\n\r", ch);
      return TRUE;
   }

   return FALSE;
}

struct hunt_mobs_tp
{
   int mob_vnum;
   int room_vnum;
   int min_level;
   char *name;
}

hunt_mobs[] = {{3062, 3001, 0, "Bug"},
               {3561, 3001, 40, "Shadow Dragon"},
               {18302, 3001, 80, "Bounty Hunter"},
               {18301, 3001, 100, "The Avenger"},
               {18306, 3001, 240, "Blayze"},
               {-1, -1, -1, " "}};

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer(CHAR_DATA *ch, CHAR_DATA *victim)
{
   /*
    * Follow charm thread to responsible character.
    * Attacking someone's charmed char is hostile!
    * -S- Mod:  Set sentence according to difference in levels -
    * this is no. times the player must die before the killer flag
    * will be removed.
    */

   int diff;
   int a;
   MOB_INDEX_DATA *pMobIndex;
   ROOM_INDEX_DATA *pRoomIndex;
   CHAR_DATA *hunter;

   if ((ch->fighting == victim))
      return;
   /*
      if ( ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL )
         && ( ch->fighting != victim )  )
      victim = victim->master;   */

   /*
    * NPC's are fair game.
    * So are killers and thieves.
    */
   if (IS_NPC(victim) || IS_SET(victim->act, PLR_KILLER) ||
       IS_SET(victim->in_room->room_flags, ROOM_PK) /* -S- Mod */
       || IS_SET(victim->act, PLR_THIEF)            /*
                                                     * || ( ch->fighting == victim )  */
       || (ch == victim))
      return;

   /*
    * if in deathmatch, anything goes
    */
   if (deathmatch)
      return;
   if (ch->sentence > 5000)
      return;

   /*
    * Check to see if ch & victim are in clans, and enemies
    */

   if (!IS_NPC(ch) && !IS_NPC(victim))
   {

      if ((ch->pcdata->clan != 0) && (victim->pcdata->clan != 0) &&
          (politics_data.diplomacy[ch->pcdata->clan][victim->pcdata->clan] < -450))
         return;

      if (IS_SET(ch->pcdata->pflags, PFLAG_PKOK) && IS_SET(victim->pcdata->pflags, PFLAG_PKOK))
         return;
   }

   /*
    * NPC's are cool of course (as long as not charmed).
    * Hitting yourself is cool too (bleeding).
    * So is being staff (Alander's idea).
    * BAH.  Imms get flagged too now, unless pkok.
    * And current killers stay as they are.
    */
   if (IS_NPC(ch) || ch == victim)
      return;

   send_to_char("*** You are a PLAYER KILLER!! ***\n\r", ch);

   {
      char buf[MAX_STRING_LENGTH];

      sprintf(buf, "%s flagged as a KILLER for attack on %s.", ch->name, victim->name);
      monitor_chan(buf, MONITOR_COMBAT);
   }
   diff = 3;
   if (get_psuedo_level(ch) > get_psuedo_level(victim))
   {
      diff += (get_psuedo_level(ch) - get_psuedo_level(victim)) / 7;
      if (diff > 5)
         diff = 5;
   }
   ch->sentence += diff * get_psuedo_level(ch) * 3; /* Magic # - Ramias */
   if (ch->adept_level > 0)
      ch->sentence += diff * get_psuedo_level(ch) * 2;

   SET_BIT(ch->act, PLR_KILLER);
   save_char_obj(ch);

   /*
    * MAG Create a hunter for the person
    */
   diff = get_psuedo_level(ch);

   /*
    * Added if check back... meant to penalize for attacking lower
    * * level players -S-
    */

   if (get_psuedo_level(ch) > get_psuedo_level(victim))
      diff += get_psuedo_level(ch) - get_psuedo_level(victim);
   /*
       if (diff > MAX_LEVEL)
        diff=MAX_LEVEL;
                          SRZ just set the level of the hunter to the level of the player :) */

   /*
    * Fixed for loop so it can't become infinite -S-
    */
   for (a = 0; hunt_mobs[a].min_level != -1; a++)
   {
      if (diff <= hunt_mobs[a].min_level)
         break;
   }

   if (hunt_mobs[a].mob_vnum == -1)
   {
      notify("No killer hunter found in check_killer!", 83);
      return;
   }

   if ((pMobIndex = get_mob_index(hunt_mobs[a].mob_vnum)) == NULL)
   {
      bug("check_killer: No such mob %i.", hunt_mobs[a].mob_vnum);
      return;
   }

   if ((pRoomIndex = get_room_index(hunt_mobs[a].room_vnum)) == NULL)
   {
      bug("check_killer: No such room %i.", hunt_mobs[a].room_vnum);
      return;
   }

   hunter = create_mobile(pMobIndex);
   char_to_room(hunter, pRoomIndex);
   /*
    * do_gossip( hunter, "Well, someone's gonna get it!" );
    */
   set_hunt(hunter, NULL, ch, NULL,
            HUNT_WORLD | HUNT_OPENDOOR | HUNT_PICKDOOR | HUNT_UNLOCKDOOR | HUNT_INFORM,
            HUNT_CR | HUNT_MERC);

   return;
}

bool shortfight_summary_recipient_matches(CHAR_DATA *rch, CHAR_DATA *ch, CHAR_DATA *victim,
                                          bool expected_shortfight)
{
   bool rch_shortfight;

   if (rch == NULL || ch == NULL || victim == NULL)
      return FALSE;

   if (rch == ch || rch == victim)
      return FALSE;

   rch_shortfight = !IS_NPC(rch) && IS_SET(rch->config, CONFIG_SHORT_FIGHT);
   return rch_shortfight == expected_shortfight;
}

bool shortfight_can_broadcast_room_summary(CHAR_DATA *ch, CHAR_DATA *victim)
{
   return ch != NULL && victim != NULL && ch->in_room != NULL && victim->in_room != NULL &&
          ch->in_room == victim->in_room;
}

void shortfight_emit_autoattack_summary(CHAR_DATA *ch, CHAR_DATA *victim)
{
   int reactive_damage = 0;
   int total_damage = short_fight_round_end(ch, victim, &reactive_damage);

   if (total_damage > 0 || reactive_damage > 0)
   {
      char buf[MSL];
      bool expected_shortfight = TRUE;
      bool ch_shortfight = !IS_NPC(ch) && IS_SET(ch->config, CONFIG_SHORT_FIGHT);
      bool victim_shortfight = !IS_NPC(victim) && IS_SET(victim->config, CONFIG_SHORT_FIGHT);

      if (ch_shortfight == expected_shortfight)
      {
         sprintf(
             buf,
             "@@cYou@@N autoattack summary vs @@c$N@@N: dealt @@e%d@@N, took @@r%d@@N reactive.",
             total_damage, reactive_damage);
         act(buf, ch, NULL, victim, TO_CHAR);
      }

      if (victim_shortfight == expected_shortfight)
      {
         sprintf(
             buf,
             "@@c$n@@N autoattack summary vs @@cyou@@N: dealt @@e%d@@N, took @@r%d@@N reactive.",
             total_damage, reactive_damage);
         act(buf, ch, NULL, victim, TO_VICT);
      }

      if (shortfight_can_broadcast_room_summary(ch, victim))
      {
         for (CHAR_DATA *rch = ch->in_room->first_person; rch != NULL; rch = rch->next_in_room)
         {
            if (!shortfight_summary_recipient_matches(rch, ch, victim, expected_shortfight))
               continue;

            sprintf(buf,
                    "@@c%s@@N autoattack summary vs @@c%s@@N: dealt @@e%d@@N, took @@r%d@@N "
                    "reactive.\n\r",
                    PERS(ch, rch), PERS(victim, rch), total_damage, reactive_damage);
            send_to_char(buf, rch);
         }
      }
   }
}

static void act_avoidance_notvict(CHAR_DATA *ch, CHAR_DATA *victim, const char *verb)
{
   char buf[MAX_STRING_LENGTH];

   for (CHAR_DATA *rch = ch->in_room->first_person; rch != NULL; rch = rch->next_in_room)
   {
      if (rch == ch || rch == victim)
         continue;

      if (!IS_NPC(rch) && IS_SET(rch->config, CONFIG_SHORT_FIGHT))
         continue;

      sprintf(buf, "%s %s %s's attack.\n\r", PERS(victim, rch), verb, PERS(ch, rch));
      send_to_char(buf, rch);
   }
}

int get_level_scaled_avoidance_baseline(CHAR_DATA *ch, CHAR_DATA *victim, int base_chance)
{
   int level_diff = 0;

   if (ch == NULL || victim == NULL)
      return 0;

   level_diff = get_psuedo_level(victim) - get_psuedo_level(ch);

   /*
    * Baseline avoidance expectations:
    *  - PC vs PC and NPC vs NPC: equal levels anchor the baseline.
    *  - NPC victim vs PC attacker: victim being 20 levels higher anchors the baseline.
    *  - NPC attacker vs PC victim: attacker being 20 levels higher anchors the baseline.
    */
   if (!IS_NPC(ch) && IS_NPC(victim))
      level_diff -= 20;
   else if (IS_NPC(ch) && !IS_NPC(victim))
      level_diff += 20;

   base_chance -= abs(level_diff) / 2;

   if (base_chance < 0)
      return 0;

   return base_chance;
}

bool check_avoidance(CHAR_DATA *ch, CHAR_DATA *victim)
{
   int max_avoidance = 75;
   int chance = number_percent();

   max_avoidance += get_speed(victim) * 5;

   max_avoidance -= get_speed(ch) * 5;

   // Can't avoid when stunned!
   if (ch->position <= POS_STUNNED)
      return FALSE;

   /* Public society: Shrine marked_strike bypasses avoidance 10% of the time */
   if (pub_society_marked_strike_check(ch, victim))
      return FALSE;

   if (IS_NPC(ch) && IS_SET(ch->act, ACT_SOLO))
      max_avoidance += 10;
   if (IS_NPC(ch) && IS_SET(ch->act, ACT_BOSS))
      max_avoidance += 20;

   int parry = get_level_scaled_avoidance_baseline(ch, victim, 10) + get_parry(victim) -
               get_evasion_piercing(ch);
   if (!can_see(ch, victim) && parry > 0)
      parry += 20;
   if (!can_see(victim, ch) && parry > 0)
      parry -= 30;
   if (parry > max_avoidance && parry > 0)
   {
      parry = max_avoidance;
      max_avoidance = 0;
   }
   else if (parry > 0)
      max_avoidance -= parry;
   else
      parry = 0;

   if (chance < parry)
   {
      if (!(!IS_NPC(victim) && IS_SET(victim->config, CONFIG_SHORT_FIGHT) &&
            short_fight_round_active(ch, victim)))
         act("You parry $n's attack.", ch, NULL, victim, TO_VICT);

      if (!(!IS_NPC(ch) && IS_SET(ch->config, CONFIG_SHORT_FIGHT) &&
            short_fight_round_active(ch, victim)))
         act("$N parries your attack.", ch, NULL, victim, TO_CHAR);

      act_avoidance_notvict(ch, victim, "parries");

      if (number_percent() < (get_counter(victim) - get_evasion_piercing(ch)))
         one_hit(victim, ch, gsn_counter);

      /* Sentinel: testimony accumulation on parry */
      if (is_sentinel_class(victim) && victim->fighting == ch)
      {
         if (victim->testimony_target != ch)
         {
            /* Seal of the Tribunal: testimony cannot be lost */
            if (!is_affected(victim, gsn_seal_of_the_tribunal))
            {
               /* Seal testimony: lose 2 instead of all on target switch */
               if (is_affected(victim, gsn_seal_testimony))
                  victim->testimony = UMAX(victim->testimony - 2, 0);
               else
                  victim->testimony = 0;
            }
            set_testimony_target(victim, ch);
         }
         add_testimony(victim, 1);
      }

      /* Sentinel: measured response — counter-attack on parry */
      if (can_use_skill(victim, gsn_measured_response) &&
          number_percent() < get_curr_wis(victim) * 2)
      {
         one_hit(victim, ch, gsn_measured_response);
         if (is_sentinel_class(victim) && victim->fighting == ch)
            add_testimony(victim, 1);
      }

      /* Arbiter: cross-examination — chance to delay attacker's next skill on parry */
      if (can_use_skill(victim, gsn_cross_examination) &&
          number_percent() < get_curr_wis(victim) * 2)
      {
         if (!is_affected(ch, gsn_cross_examination))
         {
            AFFECT_DATA xaf;
            xaf.type = gsn_cross_examination;
            xaf.duration = 1;
            xaf.duration_type = DURATION_ROUND;
            xaf.location = APPLY_NONE;
            xaf.modifier = 0;
            xaf.bitvector = 0;
            affect_to_char(ch, &xaf);
            act("@@y$N's counter-positioning disrupts your timing!@@N", ch, NULL, victim, TO_CHAR);
         }
      }

      return TRUE;
   }

   int raw_block = get_block(victim);
   int block = (raw_block > 0 ? get_level_scaled_avoidance_baseline(ch, victim, 30) : 0) +
               raw_block - get_evasion_piercing(ch);
   if (!can_see(ch, victim) && block > 0)
      block += 20;
   if (!can_see(victim, ch) && block > 0)
      block -= 30;
   if (block > max_avoidance && block > 0)
   {
      block = max_avoidance;
      max_avoidance = 0;
   }
   else if (block > 0)
      max_avoidance -= block;
   else
      block = 0;

   if (chance < parry + block)
   {
      if (!(!IS_NPC(victim) && IS_SET(victim->config, CONFIG_SHORT_FIGHT) &&
            short_fight_round_active(ch, victim)))
         act("You block $n's attack.", ch, NULL, victim, TO_VICT);

      if (!(!IS_NPC(ch) && IS_SET(ch->config, CONFIG_SHORT_FIGHT) &&
            short_fight_round_active(ch, victim)))
         act("$N blocks your attack.", ch, NULL, victim, TO_CHAR);

      act_avoidance_notvict(ch, victim, "blocks");

      if (number_percent() < (get_counter(victim) - get_evasion_piercing(ch)))
         one_hit(victim, ch, gsn_counter);

      /* Sentinel: testimony accumulation on block */
      if (is_sentinel_class(victim) && victim->fighting == ch)
      {
         if (victim->testimony_target != ch)
         {
            if (!is_affected(victim, gsn_seal_of_the_tribunal))
            {
               if (is_affected(victim, gsn_seal_testimony))
                  victim->testimony = UMAX(victim->testimony - 2, 0);
               else
                  victim->testimony = 0;
            }
            set_testimony_target(victim, ch);
         }
         add_testimony(victim, 1);
      }

      return TRUE;
   }

   int dodge = get_level_scaled_avoidance_baseline(ch, victim, 10) + get_dodge(victim) -
               get_evasion_piercing(ch);
   if (!can_see(ch, victim) && dodge > 0)
      dodge += 20;
   if (!can_see(victim, ch) && dodge > 0)
      dodge -= 30;
   if (dodge > max_avoidance && dodge > 0)
   {
      dodge = max_avoidance;
      max_avoidance = 0;
   }
   else if (dodge > 0)
      max_avoidance -= dodge;
   else
      dodge = 0;

   if (chance < parry + block + dodge)
   {
      if (!(!IS_NPC(victim) && IS_SET(victim->config, CONFIG_SHORT_FIGHT) &&
            short_fight_round_active(ch, victim)))
         act("You dodge $n's attack.", ch, NULL, victim, TO_VICT);

      if (!(!IS_NPC(ch) && IS_SET(ch->config, CONFIG_SHORT_FIGHT) &&
            short_fight_round_active(ch, victim)))
         act("$N dodges your attack.", ch, NULL, victim, TO_CHAR);

      act_avoidance_notvict(ch, victim, "dodges");

      if (number_percent() < (get_counter(victim) - get_evasion_piercing(ch)))
         one_hit(victim, ch, gsn_counter);

      /* Sentinel: testimony accumulation on dodge */
      if (is_sentinel_class(victim) && victim->fighting == ch)
      {
         if (victim->testimony_target != ch)
         {
            if (!is_affected(victim, gsn_seal_of_the_tribunal))
            {
               if (is_affected(victim, gsn_seal_testimony))
                  victim->testimony = UMAX(victim->testimony - 2, 0);
               else
                  victim->testimony = 0;
            }
            set_testimony_target(victim, ch);
         }
         add_testimony(victim, 1);
      }

      /* Sentinel: measured response — counter-attack on dodge */
      if (can_use_skill(victim, gsn_measured_response) &&
          number_percent() < get_curr_wis(victim) * 2)
      {
         one_hit(victim, ch, gsn_measured_response);
         if (is_sentinel_class(victim) && victim->fighting == ch)
            add_testimony(victim, 1);
      }

      /* Arbiter: cross-examination — chance to delay attacker's next skill on dodge */
      if (can_use_skill(victim, gsn_cross_examination) &&
          number_percent() < get_curr_wis(victim) * 2)
      {
         if (!is_affected(ch, gsn_cross_examination))
         {
            AFFECT_DATA xaf;
            xaf.type = gsn_cross_examination;
            xaf.duration = 1;
            xaf.duration_type = DURATION_ROUND;
            xaf.location = APPLY_NONE;
            xaf.modifier = 0;
            xaf.bitvector = 0;
            affect_to_char(ch, &xaf);
            act("@@y$N's evasive movement disrupts your timing!@@N", ch, NULL, victim, TO_CHAR);
         }
      }

      return TRUE;
   }

   return FALSE;
}

/*
 * Check for parry.
 */
int get_parry(CHAR_DATA *ch)
{
   int chance = 0;

   if (!IS_AWAKE(ch))
      return 0;

   if (IS_NPC(ch) && !IS_SET(ch->skills, MOB_PARRY))
      return 0;

   if (!can_use_skill(ch, gsn_parry))
      return 0;

   if (IS_NPC(ch))
   {
      chance = get_curr_str(ch);
      if (IS_SET(ch->act, ACT_SOLO))
         chance += 15;
      if (IS_SET(ch->act, ACT_BOSS))
         chance += 25;
   }
   else
   {
      OBJ_DATA *weapon;
      if ((weapon = get_eq_char(ch, WEAR_HOLD_HAND_R)) == NULL || !IS_WEAPON(weapon))
         weapon = get_eq_char(ch, WEAR_HOLD_HAND_L);
      if (weapon == NULL)
         weapon = get_eq_char(ch, WEAR_TWO_HANDED);
      if ((weapon == NULL) || (!IS_WEAPON(weapon)))
      {
         return 0;
      }
      chance = get_curr_str(ch);
   }
   chance += cloak_adept_defense_bonus(ch);

   chance += stance_app[ch->stance].speed_mod;
   chance += get_speed(ch) * 5;

   chance += ch->parry_mod;

   /* Vigilance: WIS-based parry bonus */
   if (can_use_skill(ch, gsn_vigilance))
      chance += get_curr_wis(ch) * 2 / 5;

   if (chance > 50)
      chance = 50;

   return chance;
}

/*
 * Check for dodge.
 */
int get_dodge(CHAR_DATA *ch)
{
   int chance = 0;

   if (!IS_AWAKE(ch))
      return 0;

   if (IS_NPC(ch) && !IS_SET(ch->skills, MOB_DODGE))
      return 0;

   if (!can_use_skill(ch, gsn_dodge))
      return 0;

   if (IS_NPC(ch))
   {
      /*
       * Tuan was here.  :)
       */
      chance = get_curr_dex(ch);
      if (IS_SET(ch->act, ACT_SOLO))
         chance += 15;
      if (IS_SET(ch->act, ACT_BOSS))
         chance += 25;
   }
   else
   {
      chance = get_curr_dex(ch);
   }
   chance += cloak_adept_defense_bonus(ch);

   chance += stance_app[ch->stance].speed_mod;
   chance += get_speed(ch) * 5;

   chance += ch->dodge_mod;

   /* Public society: Road Wardens road_grit +5% dodge */
   chance += pub_society_dodge_bonus(ch);

   /* Vigilance: WIS-based dodge bonus */
   if (can_use_skill(ch, gsn_vigilance))
      chance += get_curr_wis(ch) * 2 / 5;

   if (chance > 50)
      chance = 50;

   return chance;
}

int get_block(CHAR_DATA *ch)
{
   int chance = 0;
   bool buckler = FALSE;

   if (!IS_AWAKE(ch))
      return 0;

   if (!can_use_skill(ch, gsn_shield_block))
      return 0;

   OBJ_DATA *shield;
   shield = get_eq_char(ch, WEAR_HOLD_HAND_R);
   if (shield == NULL || shield->item_type != ITEM_ARMOR)
      shield = get_eq_char(ch, WEAR_HOLD_HAND_L);
   if (shield == NULL || shield->item_type != ITEM_ARMOR)
   {
      shield = get_eq_char(ch, WEAR_BUCKLER);
      buckler = TRUE;
   }
   if (shield == NULL)
      return 0;

   if (IS_NPC(ch))
   {
      chance = get_curr_con(ch);
      if (IS_SET(ch->act, ACT_SOLO))
         chance += 15;
      if (IS_SET(ch->act, ACT_BOSS))
         chance += 25;
   }
   else
   {

      chance = get_curr_con(ch);
   }

   chance += cloak_adept_defense_bonus(ch);

   if (buckler)
      chance /= 2;

   chance += stance_app[ch->stance].speed_mod;
   chance += get_speed(ch) * 5;

   chance += ch->block_mod;

   if (chance > 50)
      chance = 50;

   return chance;
}

/* Calculate counter */
int get_counter(CHAR_DATA *ch)
{
   int chance = 0;

   if (!IS_AWAKE(ch))
      return chance;

   if (IS_NPC(ch) && !IS_SET(ch->skills, MOB_COUNTER))
      return chance;

   if (!IS_NPC(ch) && !can_use_skill(ch, gsn_counter))
      return chance;

   if (IS_NPC(ch))
   {
      chance = get_psuedo_level(ch) / 3.1 + get_curr_dex(ch) * 2 / 5;
      if (IS_SET(ch->act, ACT_SOLO))
         chance += 15;
      if (IS_SET(ch->act, ACT_BOSS))
         chance += 25;
   }
   else
   {
      chance = 10 + get_curr_dex(ch) * 3 / 5;
   }

   int fists = 0;

   if (get_eq_char(ch, WEAR_HOLD_HAND_L) != NULL &&
       IS_SET(get_eq_char(ch, WEAR_HOLD_HAND_L)->extra_flags, ITEM_FIST))
      fists++;

   if (get_eq_char(ch, WEAR_HOLD_HAND_R) != NULL &&
       IS_SET(get_eq_char(ch, WEAR_HOLD_HAND_R)->extra_flags, ITEM_FIST))
      fists++;

   if (char_class_level(ch, CLASS_MON) > 0 || char_class_level(ch, CLASS_BRA) > 0)
      chance += fists * 3;

   if (char_class_level(ch, CLASS_MAR) > 0)
      chance += fists * 3;

   chance += get_speed(ch) * 5;

   chance += cloak_adept_defense_bonus(ch);

   return chance;
}

/* Calculate evasion piercing */
int get_evasion_piercing(CHAR_DATA *ch)
{
   int chance = 0;

   if (!IS_AWAKE(ch))
      return 0;

   if (IS_NPC(ch))
   {
      if (IS_SET(ch->act, ACT_SOLO))
         chance += 5;
      if (IS_SET(ch->act, ACT_BOSS))
         chance += 10;
   }

   chance += get_speed(ch) * 5;

   chance += stance_app[ch->stance].speed_mod;

   chance += ch->pierce_mod;

   return chance;
}

/*
 * Start fights.
 */
void set_fighting(CHAR_DATA *ch, CHAR_DATA *victim, bool check)
{
   if (is_fighting(ch))
   {
      /*
       * bug( "Set_fighting: already fighting", 0 );
       */
      return;
   }

   /*
    * Check here for killer flag
    */
   if (check)
      check_killer(ch, victim);

   /*    if ( IS_NPC(victim) && IS_SET(victim->act, ACT_HUNTER ) )
                   make_hunt( victim, ch );*/
   /*
    * fun fun FUN!
    */

   if (IS_AFFECTED(ch, AFF_SLEEP))
      affect_strip(ch, gsn_sleep);

   ch->fighting = victim;
   ch->position = POS_FIGHTING;

   /*
    * Check if mob has ACT_REMEMBER (ch to attack) SET
    */

   if (IS_NPC(victim) && !IS_NPC(ch) && IS_SET(victim->act, ACT_REMEMBER))
   {
      /*
       * Then set victim->target to player's name...
       */
      if (victim->target != NULL)
         free_string(victim->target);
      if (ch != NULL && ch->is_free == FALSE)
         victim->target = str_dup(ch->name);
   }

   return;
}

/*
 * Stop fights.
 */
void stop_fighting(CHAR_DATA *ch, bool fBoth)
{
   CHAR_DATA *fch;
   CHAR_DATA *fch_next;

   ch->fighting = NULL;
   ch->position = POS_STANDING;
   update_pos(ch);

   if (!fBoth)
      return;

   CREF(fch_next, CHAR_NEXT);
   for (fch = first_char; fch != NULL; fch = fch_next)
   {
      fch_next = fch->next;
      if (fch->fighting == ch)
      {
         fch->fighting = NULL;
         fch->position = POS_STANDING;
         update_pos(fch);
      }
   }
   char_unreference(&fch_next);
   return;
}

void do_kill(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Kill whom?\n\r", ch);
      return;
   }

   if ((victim = get_char_room(ch, arg)) == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
      return;
   }

   if (!IS_NPC(victim) && !(deathmatch))
   {
      if (!IS_SET(victim->act, PLR_KILLER) && !IS_SET(victim->act, PLR_THIEF))
      {
         send_to_char("You must MURDER a player.\n\r", ch);
         return;
      }
   }
   else
   {
      if (IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL)
      {
         send_to_char("You must MURDER a charmed creature.\n\r", ch);
         return;
      }
   }

   if (victim == ch)
   {
      send_to_char("You hit yourself.  Ouch!\n\r", ch);
      multi_hit(ch, ch, TYPE_UNDEFINED);
      return;
   }

   if (is_safe(ch, victim))
      return;

   if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
   {
      act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
      return;
   }

   if (is_fighting(ch))
   {
      send_to_char("You do the best you can!\n\r", ch);
      return;
   }

   WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
   check_killer(ch, victim);
   multi_hit(ch, victim, TYPE_UNDEFINED);
   return;
}

void do_murde(CHAR_DATA *ch, char *argument)
{
   send_to_char("If you want to MURDER, spell it out.\n\r", ch);
   return;
}

void do_murder(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Murder whom?\n\r", ch);
      return;
   }

   if ((victim = get_char_room(ch, arg)) == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
      return;
   }

   if (victim == ch)
   {
      send_to_char("Suicide is a mortal sin.\n\r", ch);
      return;
   }

   if (is_safe(ch, victim))
   {
      send_to_char("Not here.\n\r", ch);
      return;
   }

   if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
   {
      act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
      return;
   }

   if (is_fighting(ch))
   {
      send_to_char("You do the best you can!\n\r", ch);
      return;
   }

   WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
   sprintf(buf, "%s attacked by %s.\n\r", victim->name, ch->name);
   notify(buf, MAX_LEVEL - 2);

   if (IS_NPC(ch) || IS_NPC(victim) || !IS_SET(ch->pcdata->pflags, PFLAG_PKOK) ||
       !IS_SET(victim->pcdata->pflags, PFLAG_PKOK))
   {
      /*
       * If not pkok people, do yell.
       */
      sprintf(buf, "Help! I'M BEING ATTACKED!!! ARRRGGGHHHHHH!");
      do_yell(victim, buf);
   }
   check_killer(ch, victim);
   multi_hit(ch, victim, TYPE_UNDEFINED);
   return;
}

void do_flee(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;
   ROOM_INDEX_DATA *was_in;
   ROOM_INDEX_DATA *now_in;
   CHAR_DATA *victim;
   char buf[MAX_STRING_LENGTH];
   int attempt;
   int cost; /* xp cost for a flee */

   if ((victim = ch->fighting) == NULL)
   {
      if (is_fighting(ch))
         ch->position = POS_STANDING;
      send_to_char("You aren't fighting anyone.\n\r", ch);
      return;
   }

   /*
    * Check if mob will "allow" ch to flee...
    */

   if (IS_SET(victim->act, ACT_NO_FLEE) && !IS_NPC(ch) && IS_NPC(victim))
   {
      send_to_char("You attempt to flee from battle, but fail!\n\r", ch);
      sprintf(buf, "%s tells you 'No way will you escape ME!!'\n\r", victim->short_descr);
      send_to_char(buf, ch);
      return;
   }
   if (deathmatch)
   {
      if (ch->hit < 200)
      {
         send_to_char("@@eYou are in too much pain!!@@N\n\r", ch);
         return;
      }
      else
      {
         send_to_char("@@eYour enemy nails you one more time!@@N\n\r", ch);
         ch->hit -= 150;
      }
   }

   if (IS_NPC(ch) && is_affected(ch, skill_lookup("flee timer")))
   {
      send_to_char("You can't flee again so soon, your flee timer has not expired!\n\r", ch);
      return;
   }

   was_in = ch->in_room;
   for (attempt = 0; attempt < 6; attempt++)
   {
      EXIT_DATA *pexit;
      int door;

      door = number_door();
      if ((pexit = was_in->exit[door]) == 0 || pexit->to_room == NULL ||
          IS_SET(pexit->exit_info, EX_CLOSED) ||
          (IS_NPC(ch) &&
           (IS_SET(pexit->to_room->room_flags, ROOM_NO_MOB) ||
            (IS_SET(ch->act, ACT_STAY_AREA) && pexit->to_room->area != ch->in_room->area))))
         continue;

      move_char(ch, door);
      if ((now_in = ch->in_room) == was_in)
         continue;

      ch->in_room = was_in;
      act("$n has fled!", ch, NULL, NULL, TO_ROOM);
      ch->in_room = now_in;

      if (IS_NPC(ch))
      {
         af.type = gsn_flee_timer;
         af.duration = number_range(2, 4);
         af.location = APPLY_NONE;
         af.duration_type = DURATION_ROUND;
         af.modifier = 0;
         af.bitvector = 0;
         af.caster = ch;
         affect_to_char(ch, &af);
      }

      if (!IS_NPC(ch))
      {
         cost = number_range(ch->exp / 15, ch->exp / 10);
         if (ch->adept_level > 0)
            cost /= 1000;
         cost = UMIN(cost, ch->exp);
         sprintf(buf, "You flee from combat!  You lose %d exps.\n\r", cost);
         send_to_char(buf, ch);
         gain_exp(ch, (0 - cost));
      }
      if ((is_fighting(ch)) && (IS_NPC(ch->fighting)))
      {
         ch->fighting->ngroup->state = GRP_STATE_HUNTING;
         ch->fighting->ngroup->leader->hunting = ch;
      }
      stop_fighting(ch, TRUE);
      /*
       * 75% chance that mobs will hunt fleeing people. -- Alty
       */
      if (IS_NPC(victim) && !IS_SET(victim->act, ACT_SENTINEL) && number_bits(2) > 0)
         set_hunt(victim, NULL, ch, NULL, HUNT_WORLD | HUNT_INFORM | HUNT_OPENDOOR,
                  HUNT_MERC | HUNT_CR);
      return;
   }

   cost = get_psuedo_level(ch) * 3;
   if (ch->adept_level > 0)
      cost = 0;
   cost = UMIN(cost, ch->exp);
   sprintf(buf, "You failed!  You lose %d exps.\n\r", cost);
   send_to_char(buf, ch);
   gain_exp(ch, (0 - cost));
   return;
}

void do_sla(CHAR_DATA *ch, char *argument)
{
   send_to_char("If you want to SLAY, spell it out.\n\r", ch);
   return;
}

void do_slay(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];

   one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      send_to_char("Slay whom?\n\r", ch);
      return;
   }

   if ((victim = get_char_room(ch, arg)) == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
      return;
   }

   if (ch == victim)
   {
      send_to_char("Suicide is a mortal sin.\n\r", ch);
      return;
   }

   if (IS_HERO(victim))
   {
      send_to_char("Not on other Staff / Adept players!\n\r", ch);
      return;
   }

   if (!IS_NPC(victim) && victim->level >= ch->level)
   {
      send_to_char("You failed.\n\r", ch);
      return;
   }

   act("You suck the life energy out of $M!", ch, NULL, victim, TO_CHAR);
   act("$n sucks out your life energy!", ch, NULL, victim, TO_VICT);
   act("$n sucks out $N's life energy!", ch, NULL, victim, TO_NOTVICT);
   raw_kill(victim, "");
   return;
}

void do_assist(CHAR_DATA *ch, char *argument)
{
   /*
    * Allow players to join in fight, by typing assist,
    * * or assist <name>.  Will only ever allow players to
    * * assist a group member  -- Stephen
    */

   CHAR_DATA *assist = NULL;
   CHAR_DATA *ppl;
   char actbuf[MSL];

   if (argument[0] != '\0') /* then check for assist <name> */
   {
      if ((assist = get_char_room(ch, argument)) == NULL)
      {
         send_to_char("They don't seem to be here right now...\n\r", ch);
         return;
      }
      if (assist == ch)
      {
         send_to_char("You sure need help!\n\r", ch);
         return;
      }
      if (!IS_NPC(ch) && !is_same_group(ch, assist))
      {
         act("Sorry, $N isn't in your group...", ch, NULL, assist, TO_CHAR);
         return;
      }
      if ((assist->fighting != NULL) && (!is_fighting(ch)))
      {
         sprintf(actbuf, "$n screams, '%s'", "BANZAI!! $N must be assisted!!");
         act(actbuf, ch, NULL, assist, TO_ROOM);
         sprintf(actbuf, "You scream, '%s'", "BANZAI!! $N must be assisted!!");
         act(actbuf, ch, NULL, assist, TO_CHAR);
         set_fighting(ch, assist->fighting, TRUE);
         return;
      }
      send_to_char("Doesn't look like anyone needs your help right now...\n\r", ch);
      return;
   }

   /*
    * Check if any group members in room AND fighting
    */

   for (ppl = ch->in_room->first_person; ppl != NULL; ppl = ppl->next_in_room)
      if ((ppl != ch) && (is_same_group(ch, ppl)))
         break;

   if (ppl == NULL)
   {
      send_to_char("Doesn't look like anyone needs your help right now...\n\r", ch);
      return;
   }

   /*
    * Assisting leader is main priority
    */
   if ((ch->leader != NULL) && (ch->leader->in_room == ch->in_room) &&
       (ch->leader->fighting != NULL) && (!is_fighting(ch)) && (ch->leader != ch))
   {
      sprintf(actbuf, "$n screams, '%s'", "BANZAI!! $N must be assisted!!");
      act(actbuf, ch, NULL, ch->leader, TO_ROOM);
      sprintf(actbuf, "You scream, '%s'", "BANZAI!! $N must be assisted!!");
      act(actbuf, ch, NULL, ch->leader, TO_CHAR);
      set_fighting(ch, ch->leader->fighting, TRUE);
      return;
   }

   /*
    * Ok, so no leader to help... lets look for other group members
    */
   for (ppl = ch->in_room->first_person; ppl != NULL; ppl = ppl->next_in_room)
      if ((is_same_group(ch, ppl)) && (ppl != ch) && (ppl->fighting != NULL) && (!is_fighting(ch)))
      {
         sprintf(actbuf, "$n screams, '%s'", "BANZAI!! $N must be assisted!!");
         act(actbuf, ch, NULL, ppl, TO_ROOM);
         sprintf(actbuf, "You scream, '%s'", "BANZAI!! $N must be assisted!!");
         act(actbuf, ch, NULL, ppl, TO_CHAR);
         set_fighting(ch, ppl->fighting, TRUE);
         return;
      }

   send_to_char("Doesn't look like anyone needs your help right now...\n\r", ch);
   return;
}

void check_adrenaline(CHAR_DATA *ch, sh_int damage)
{
   AFFECT_DATA af;

   if (damage > 200 && can_use_skill(ch, gsn_adrenaline))
   {

      af.type = skill_lookup("adrenaline bonus");
      af.duration = 1;
      af.location = APPLY_DAMROLL;
      af.modifier = 1;
      af.bitvector = 0;
      affect_join(ch, &af);
      send_to_char("You feel a surge of @@eadrenaline@@N!!!\n\r", ch);
   }
   return;
}
