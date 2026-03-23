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

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "weapon_bond.h"
#include "sentinel.h"
#include <signal.h>
#include "npc_dialogue.h"
#include "socket.h"

bool should_abort_for_checkpoint_timeout(int usage_now, int checkpoint, int threshold,
                                         bool disable_abort)
{
   return !disable_abort && (usage_now - checkpoint > threshold);
}

bool hotreboot_warning_30s_due(int countdown)
{
   return countdown == 30 * PULSE_PER_SECOND;
}

bool hotreboot_execute_due(int countdown)
{
   return countdown == 0;
}

#ifndef UNIT_TEST_UPDATE

/*
 * Local functions.
 */
int hit_gain args((CHAR_DATA * ch));
int mana_gain args((CHAR_DATA * ch));
int move_gain args((CHAR_DATA * ch));
void mobile_update args((void));
void weather_update args((void));
void char_update args((void));
void gain_update args((void));
void obj_update args((void));
void aggr_update args((void));
void objfun_update args((void));
void auction_update args((void));
void rooms_update args((void));
void remember_attack args((CHAR_DATA * ch, CHAR_DATA *victim));
void quest_update args((void));
void round_char_update(CHAR_DATA *ch);
void update_cooldown(CHAR_DATA *ch);
void update_heat_armor(CHAR_DATA *ch);
void update_buff_duration(CHAR_DATA *ch, int duration_type);
void round_update_hot(CHAR_DATA *ch);
void round_update_dot(CHAR_DATA *ch);
int abort_threshold = BOOT_DB_ABORT_THRESHOLD;
bool disable_timer_abort = FALSE;
int last_checkpoint;

int get_user_seconds()
{
   struct rusage rus;
   getrusage(RUSAGE_SELF, &rus);
   return rus.ru_utime.tv_sec;
}

/* Update the checkpoint */
void alarm_update()
{
   extern int ssm_dup_count;
   extern int ssm_loops;

   ssm_dup_count = 0;
   ssm_loops = 0;

   last_checkpoint = get_user_seconds();
   if (abort_threshold == BOOT_DB_ABORT_THRESHOLD)
   {
      abort_threshold = RUNNING_ABORT_THRESHOLD;
      fprintf(stderr, "Used %d user CPU seconds.\n", last_checkpoint);
   }
}

#ifndef WIN32
/* Set the virtual (CPU time) timer to the standard setting, ALARM_FREQUENCY */

void reset_itimer()
{
   struct itimerval itimer;
   itimer.it_interval.tv_usec = 0; /* miliseconds */
   itimer.it_interval.tv_sec = ALARM_FREQUENCY;
   itimer.it_value.tv_usec = 0;
   itimer.it_value.tv_sec = ALARM_FREQUENCY;

   /*
    * start the timer - in that many CPU seconds, alarm_handler will be called
    */
   if (setitimer(ITIMER_VIRTUAL, &itimer, NULL) < 0)
   {
      perror("reset_itimer:setitimer");
      exit(1);
   }
}
#endif

const char *szFrozenMessage = "Alarm_handler: Not checkpointed recently, aborting!\n";

/* Signal handler for alarm - suggested for use in MUDs by Fusion */
void alarm_handler(int signo)
{
   int usage_now = get_user_seconds();

   /*
    * Has there gone abort_threshold CPU seconds without alarm_update?
    */
   if (should_abort_for_checkpoint_timeout(usage_now, last_checkpoint, abort_threshold,
                                           disable_timer_abort))
   {
      /*
       * For the log file
       */
      char buf[MAX_STRING_LENGTH];
      extern int ssm_dup_count;
      extern int ssm_loops;
      extern int ssm_recent_loops;

      /*
       * spec: log usage values
       */
      log_f("current usage: %d, last checkpoint: %d", usage_now, last_checkpoint);
      log_f("SSM dups: %d, loops: %d, recent: %d", ssm_dup_count, ssm_loops, ssm_recent_loops);

      sprintf(buf, "%s\n\r", szFrozenMessage);
      bug(buf, 0);
      raise(SIGABRT); /* kill ourselves on return */
   }

   /*
    * The timer resets to the values specified in it_interval
    * * automatically.
    * *
    * * Spec: additionally, SIGABRT is blocked in this handler, and will
    * * only be delivered on return. This should ensure a good core.
    */
}

#ifndef WIN32
/* Install signal alarm handler */
void init_alarm_handler()
{
   struct sigaction sa;

   sa.sa_handler = alarm_handler;
   sa.sa_flags = SA_RESTART; /* Restart interrupted system calls */
   sigemptyset(&sa.sa_mask);
   sigaddset(&sa.sa_mask, SIGABRT); /* block abort() in the handler
                                     * so we can get a good coredump */

   if (sigaction(SIGVTALRM, &sa, NULL) < 0) /* setup handler for virtual timer */
   {
      perror("init_alarm_handler:sigaction");
      exit(1);
   }
   last_checkpoint = get_user_seconds();
   reset_itimer(); /* start timer */
}
#endif

/*
 * Advancement stuff.
 */
OBJ_DATA *find_clan_eq(CHAR_DATA *ch);
void claneq_adjust(OBJ_DATA *obj, CHAR_DATA *ch);

static void claneq_level_check(CHAR_DATA *ch)
{
   OBJ_DATA *obj;
   if (IS_NPC(ch) || ch->pcdata->clan == 0)
      return;
   obj = find_clan_eq(ch);
   if (obj != NULL)
   {
      bool was_worn = (obj->wear_loc != WEAR_NONE);
      if (was_worn)
         unequip_char(ch, obj);
      claneq_adjust(obj, ch);
      if (was_worn)
         equip_char(ch, obj, WEAR_CLAN_COLORS);
   }
}

void advance_level(CHAR_DATA *ch, int class, bool show)
{
   char buf[MAX_STRING_LENGTH];
   int add_hp;
   int add_mana;
   int add_move;

   if (ch->class_level[class] < 0)
      ch->class_level[class] = 1;
   else if (ch->class_level[class] < MAX_LEVEL)
      ch->class_level[class] += 1;
   else
   {
      send_to_char("Trying to advance_level past MAX_LEVEL, error!\n\r", ch);
      return;
   }
   add_hp = gclass_table[class].hp_gain;
   add_hp += get_curr_con(ch) / 8;
   add_mana = gclass_table[class].mana_gain;
   if (add_mana > 0)
      add_mana = (get_curr_int(ch) + get_curr_wis(ch)) / 8;

   add_move = (get_curr_con(ch) + get_curr_dex(ch)) / 5;

   if (class == ch->class && ch->pcdata->reincarnation_data[REINCARNATION_HP] > 0)
   {
      add_hp += get_reincarnate_hp_level(ch);
      add_mana += get_reincarnate_mana_level(ch);
      add_move += get_reincarnate_move_level(ch);
   }

   ch->pcdata->mana_from_gain += add_mana;
   ch->pcdata->hp_from_gain += add_hp;
   ch->pcdata->move_from_gain += add_move;

   if (!IS_NPC(ch))
      REMOVE_BIT(ch->act, PLR_BOUGHT_PET);

   sprintf(buf, "You gain: %d Hit Points, %d Mana, and %d Movement.\n\r", add_hp, add_mana,
           add_move);

   if (show)
      send_to_char(buf, ch);

   bond_recalculate(ch);
   claneq_level_check(ch);
   return;
}

void advance_level_remort(CHAR_DATA *ch, int class, bool show)
{
   char buf[MAX_STRING_LENGTH];
   int add_hp;
   int add_mana;
   int add_move;

   if (ch->class_level[class] < 0)
      ch->class_level[class] = 1;
   else if (ch->class_level[class] < MAX_LEVEL)
      ch->class_level[class] += 1;
   else
   {
      send_to_char("Trying to advance_level_remort past MAX_LEVEL, error!\n\r", ch);
      return;
   }
   add_hp = gclass_table[class].hp_gain;
   add_hp += get_curr_con(ch) / 4;
   add_mana = gclass_table[class].mana_gain;
   ;
   if (add_mana > 0)
      add_mana += (get_curr_int(ch) + get_curr_wis(ch)) / 4;

   add_move = (get_curr_con(ch) + get_curr_dex(ch)) / 5;

   add_hp = UMAX(1, add_hp);

   add_mana = UMAX(0, add_mana);
   add_move = UMAX(7, add_move);

   ch->pcdata->mana_from_gain += add_mana;
   ch->pcdata->hp_from_gain += add_hp;
   ch->pcdata->move_from_gain += add_move;

   if (!IS_NPC(ch))
      REMOVE_BIT(ch->act, PLR_BOUGHT_PET);

   sprintf(buf, "You gain: %d Hit Points, %d Mana, and %d Movement.\n\r", add_hp, add_mana,
           add_move);

   if (show)
      send_to_char(buf, ch);

   bond_recalculate(ch);
   claneq_level_check(ch);
   return;
}

void advance_level_adept(CHAR_DATA *ch, int class, bool show)
{
   char buf[MAX_STRING_LENGTH];
   int add_hp;
   int add_mana;
   int add_move;

   if (ch->class_level[class] < 1)
   {
      ch->exp /= 1000;
      ch->class_level[class] = 1;
   }
   else if (ch->class_level[class] < MAX_ADEPT)
      ch->class_level[class] += 1;
   else
   {
      send_to_char("Trying to advance_level_adept past MAX_ADEPT, error!\n\r", ch);
      return;
   }
   add_hp = gclass_table[class].hp_gain;
   add_hp += get_curr_con(ch) / 2;
   add_mana = gclass_table[class].mana_gain;
   if (add_mana > 0)
      add_mana += (get_curr_int(ch) + get_curr_wis(ch)) / 2;

   add_move = (get_curr_con(ch) + get_curr_dex(ch)) / 5;

   add_hp = UMAX(1, add_hp);

   add_mana = UMAX(0, add_mana);
   add_move = UMAX(7, add_move);

   ch->pcdata->mana_from_gain += add_mana;
   ch->pcdata->hp_from_gain += add_hp;
   ch->pcdata->move_from_gain += add_move;

   if (!IS_NPC(ch))
      REMOVE_BIT(ch->act, PLR_BOUGHT_PET);

   sprintf(buf, "You gain: %d Hit Points, %d Mana, and %d Movement.\n\r", add_hp, add_mana,
           add_move);

   if (show)
      send_to_char(buf, ch);

   bond_recalculate(ch);
   claneq_level_check(ch);
   return;
}

void gain_exp(CHAR_DATA *ch, long_int gain)
{
   if (IS_STAFF(ch))
      return;

   ch->exp += gain;

   return;
}

void round_update()
{
   CHAR_DATA *ch;
   CHAR_DATA *ch_next;
   for (ch = first_char; ch; ch = ch_next)
   {
      ch_next = ch->next;

      round_char_update(ch);
   }
}

void round_char_update(CHAR_DATA *ch)
{
   if (ch == NULL)
      return;

   update_heat_armor(ch);
   update_cooldown(ch);
   round_update_hot(ch);
   round_update_dot(ch);
   update_buff_duration(ch, DURATION_ROUND);

   /* Public society: Ember Wardens ember_vitality in-combat HP regen */
   if (is_fighting(ch))
   {
      int regen = pub_society_hp_regen(ch);
      if (regen > 0)
         ch->hit = UMIN(ch->hit + regen, ch->max_hit);
   }

   if (!is_fighting(ch) && ch->chi > 0)
   {
      send_to_char("Your chi has dissipated as you are not fighting.\n\r", ch);
      ch->chi = 0;
   }

   /* Sentinel: testimony cooldown tick */
   if (ch->testimony_cooldown > 0)
      ch->testimony_cooldown--;

   /* Sentinel: passive testimony accumulation while fighting same target */
   if (is_fighting(ch) && is_sentinel_class(ch) && ch->testimony_target != NULL &&
       ch->fighting == ch->testimony_target)
   {
      ch->testimony_combat_rounds++;
      if (ch->testimony_combat_rounds % TESTIMONY_PASSIVE_INTERVAL == 0)
         add_testimony(ch, 1);
   }

   /* Sentinel: out-of-combat testimony decay */
   if (!is_fighting(ch) && ch->testimony > 0)
   {
      ch->testimony--;
      if (ch->testimony > 0)
         send_to_char("@@yYour testimony fades slightly...@@N\n\r", ch);
      else
      {
         send_to_char("@@yYour testimony has fully faded.@@N\n\r", ch);
         ch->testimony_target = NULL;
         ch->testimony_combat_rounds = 0;
      }
   }

   if (!is_fighting(ch) && ch->arcane_power > 0)
      ch->arcane_power = 0;
   if (!is_fighting(ch) && ch->mental_power > 0)
      ch->mental_power = 0;
}

void update_cooldown(CHAR_DATA *ch)
{
   char buf[MSL];

   for (int i = 0; i < MAX_SKILL; i++)
   {
      if (ch->cooldown[i] > 0)
      {
         ch->cooldown[i]--;

         if (ch->cooldown[i] == 0)
         {
            sprintf(buf, "@@y%s's@@N cooldown has elapsed!\n\r", skill_table[i].name);
            send_to_char(buf, ch);
         }
      }
   }
}

void update_heat_armor(CHAR_DATA *ch)
{
   /* Heated items */
   if (ch->hit > 0 && ch->in_room != NULL && get_room_index(ch->in_room->vnum) != NULL &&
       item_has_apply(ch, ITEM_APPLY_HEATED))
   {
      OBJ_DATA *heated_item;
      int heat_damage = 0;

      for (heated_item = ch->first_carry; heated_item != NULL;
           heated_item = heated_item->next_in_carry_list)
      {
         if (IS_SET(heated_item->item_apply, ITEM_APPLY_HEATED) &&
             heated_item->wear_loc != WEAR_NONE)
         {
            heat_damage = heated_item->level;
            obj_damage(heated_item, ch, heat_damage);
            act("@@W   $p@@N you are wearing are @@eBURNING@@N you!!!", ch, heated_item, NULL,
                TO_CHAR);
            act("@@W   $p worn by $n is @@eBURNING@@N!!!", ch, heated_item, NULL, TO_ROOM);
            if (IS_NPC(ch))
               do_remove(ch, heated_item->name);
         }
      }
   }
}

void update_buff_duration(CHAR_DATA *ch, int duration_type)
{
   AFFECT_DATA *paf;
   AFFECT_DATA *paf_next;

   for (paf = ch->first_affect; paf != NULL; paf = paf_next)
   {
      paf_next = paf->next;

      if (paf->duration_type == duration_type)
      {
         if (--paf->duration < 1)
         {
            if (paf->duration < 0 && paf->duration_type == DURATION_HOUR)
            {
               paf->duration = -1;
               continue;
            }

            if (paf->type > 0 && skill_table[paf->type].msg_off)
            {
               send_to_char(skill_table[paf->type].msg_off, ch);
               send_to_char("\n\r", ch);
            }
            if (paf->type > 0 && skill_table[paf->type].room_off)
               act(skill_table[paf->type].room_off, ch, NULL, NULL, TO_ROOM);

            affect_remove(ch, paf);
         }
      }
   }
}

void round_update_hot(CHAR_DATA *ch)
{
   AFFECT_DATA *paf;

   for (paf = ch->first_affect; paf != NULL; paf = paf->next)
   {
      if (paf->location == APPLY_HOT && paf->caster != NULL && ch->hit < get_max_hp(ch) &&
          is_same_room(ch, paf->caster))
      {
         heal_character(paf->caster, ch, paf->modifier, paf->type, TRUE);
      }
   }
}

void round_update_dot(CHAR_DATA *ch)
{
   AFFECT_DATA *paf;

   for (paf = ch->first_affect; paf != NULL; paf = paf->next)
   {
      if (paf->location == APPLY_DOT && paf->caster != NULL && ch->hit > 0 &&
          is_same_room(ch, paf->caster))
      {
         do_damage(paf->caster, ch, paf->modifier, paf->type, paf->element, FALSE);
      }
   }
}

#endif

/*
 * Regeneration stuff.
 */
int hit_gain(CHAR_DATA *ch)
{
   int gain;
   if (ch->is_free != FALSE)
      return 0;

   gain = (5 + (get_psuedo_level(ch) / 20));

   switch (ch->position)
   {
   case POS_SLEEPING:
      gain += get_curr_con(ch) / 2;
      break;
   case POS_RESTING:
      gain += get_curr_con(ch) / 4;
      break;
   }
   if (!is_fighting(ch))
      gain *= 5;
   else
      gain /= 2;

   if (IS_SET(ch->in_room->room_flags, ROOM_REGEN))
      gain *= 2;

   if (IS_AFFECTED(ch, AFF_POISON))
      gain /= 4;

   if (IS_SET(ch->in_room->room_flags, ROOM_COLD) || (IS_SET(ch->in_room->room_flags, ROOM_HOT)))
   {
      if (IS_UNDEAD(ch) && IS_SET(ch->in_room->room_flags, ROOM_COLD))
         gain *= 2;
      else if (!IS_NPC(ch))
         gain *= -2;
   }

   if (IS_SET(ch->in_room->affected_by, ROOM_BV_HEAL_REGEN))
   {
      if (gain < 0)
         gain *= -2;
      else
         gain *= 2;
   }

   if (IS_SET(ch->in_room->affected_by, ROOM_BV_HEAL_STEAL))
      if (gain > 0)
         gain *= -1;
   if (!IS_NPC(ch) && (gain > 0))
   {
      if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_FAST_HEAL))
         gain = gain * 2;
      else if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_SLOW_HEAL))
         gain = gain * .75;
   }
   if (!IS_NPC(ch) && (gain > 0))
   {
      if ((IS_SET(race_table[ch->race].race_flags, RACE_MOD_WOODLAND)) && (ch->in_room != NULL))
      {
         if ((ch->in_room->sector_type == SECT_FIELD) || (ch->in_room->sector_type == SECT_FOREST))
            gain = gain * 1.3;
         else if ((ch->in_room->sector_type == SECT_CITY) ||
                  (ch->in_room->sector_type == SECT_INSIDE))
            gain = gain * .8;
      }
      else if ((IS_SET(race_table[ch->race].race_flags, RACE_MOD_DARKNESS)) &&
               (ch->in_room != NULL))
      {
         if ((ch->in_room->sector_type == SECT_FIELD) || (ch->in_room->sector_type == SECT_HILLS) ||
             (ch->in_room->sector_type == SECT_AIR) || (ch->in_room->sector_type == SECT_DESERT))
            gain = gain * .8;
         else if ((ch->in_room->sector_type == SECT_CITY) ||
                  (ch->in_room->sector_type == SECT_INSIDE))
            gain = gain * 1.3;
      }
   }

   if (is_fighting(ch) && gain > 0)
      gain = 0;

   return gain;
}

int mana_gain(CHAR_DATA *ch)
{
   int gain;
   if (ch->is_free != FALSE)
      return 0;
   gain = (5 + (get_psuedo_level(ch) / 20));

   switch (ch->position)
   {
   case POS_SLEEPING:
      gain += get_curr_int(ch);
      break;
   case POS_RESTING:
      gain += get_curr_int(ch) / 2;
      break;
   }
   if (!is_fighting(ch))
      gain *= 5;
   else
      gain /= 2;

   if (IS_SET(ch->in_room->room_flags, ROOM_REGEN))
      gain *= 2;

   if (!IS_NPC(ch) && (gain > 0))
   {
      if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_FAST_HEAL))
         gain = gain * 2;
      else if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_SLOW_HEAL))
         gain = gain * .75;
   }

   if (IS_SET(ch->in_room->affected_by, ROOM_BV_MANA_REGEN))
   {
      if (gain < 0)
         gain *= -2;
      else
         gain *= 2;
   }
   if (IS_SET(ch->in_room->affected_by, ROOM_BV_MANA_STEAL))
      if (gain > 0)
         gain *= -1;

   if (IS_AFFECTED(ch, AFF_POISON))
      gain /= 4;

   if (!IS_NPC(ch) && (gain > 0))
   {
      if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_NO_MAGIC))
         gain = gain * .5;
      else if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_WEAK_MAGIC))
         gain = gain * .75;
      else if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_STRONG_MAGIC))
         gain = gain * 1.40;
   }
   if (!IS_NPC(ch) && (gain > 0))
   {
      if ((IS_SET(race_table[ch->race].race_flags, RACE_MOD_WOODLAND)) && (ch->in_room != NULL))
      {
         if ((ch->in_room->sector_type == SECT_FIELD) || (ch->in_room->sector_type == SECT_FOREST))
            gain = gain * 1.3;
         else if ((ch->in_room->sector_type == SECT_CITY) ||
                  (ch->in_room->sector_type == SECT_INSIDE))
            gain = gain * .8;
      }
      else if ((IS_SET(race_table[ch->race].race_flags, RACE_MOD_DARKNESS)) &&
               (ch->in_room != NULL))
      {
         if ((ch->in_room->sector_type == SECT_FIELD) || (ch->in_room->sector_type == SECT_HILLS) ||
             (ch->in_room->sector_type == SECT_AIR) || (ch->in_room->sector_type == SECT_DESERT))
            gain = gain * .8;
         else if ((ch->in_room->sector_type == SECT_CITY) ||
                  (ch->in_room->sector_type == SECT_INSIDE))
            gain = gain * 1.3;
      }
   }
   if (gain > 0)
   {
      gain = gain * (get_curr_int(ch) + get_curr_wis(ch)) / 20;
   }

   if (is_fighting(ch))
      gain = 0;

   return gain;
}

int move_gain(CHAR_DATA *ch)
{
   int gain;

   if (IS_NPC(ch))
   {
      gain = ch->level;
   }
   else
   {
      gain = (10 + (get_psuedo_level(ch) / 4));

      switch (ch->position)
      {
      case POS_SLEEPING:
         gain += get_curr_dex(ch) / 2;
         break;
      case POS_RESTING:
         gain += get_curr_dex(ch) / 4;
         break;
      }
      if (!is_fighting(ch))
         gain *= 5;
      else
         gain /= 2;

      if (IS_SET(ch->in_room->room_flags, ROOM_REGEN))
         gain *= 2;
   }

   if (IS_AFFECTED(ch, AFF_POISON))
      gain /= 4;

   if (!IS_NPC(ch) && (gain > 0))
   {
      if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_FAST_HEAL))
         gain = gain * 2;
      else if (IS_SET(race_table[ch->race].race_flags, RACE_MOD_SLOW_HEAL))
         gain = gain * .75;
   }

   if (is_fighting(ch) && gain > 0)
      gain = 0;

   return gain;
}

#ifndef UNIT_TEST_UPDATE

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */

static bool mob_is_time_restricted(CHAR_DATA *ch)
{
   if (!IS_NPC(ch))
      return FALSE;

   if (IS_SET(ch->act, ACT_DAYONLY) && IS_SET(ch->act, ACT_NIGHTONLY))
      return FALSE;

   if (IS_SET(ch->act, ACT_DAYONLY) && !IS_DAYTIME())
      return TRUE;

   if (IS_SET(ch->act, ACT_NIGHTONLY) && IS_DAYTIME())
      return TRUE;

   return FALSE;
}

static void purge_time_restricted_mobs(bool keep_fighting)
{
   CHAR_DATA *ch;
   CHAR_DATA *ch_next;

   CREF(ch_next, CHAR_NEXT);
   for (ch = first_char; ch != NULL; ch = ch_next)
   {
      ch_next = ch->next;

      if (!mob_is_time_restricted(ch))
         continue;

      if (keep_fighting && ch->fighting != NULL)
         continue;

      extract_char(ch, TRUE);
   }
   char_unreference(&ch_next);
}

void mobile_update(void)
{
   CHAR_DATA *ch;
   CHAR_DATA *ch_next;
   CHAR_DATA *target;
   EXIT_DATA *pexit;
   int door;
   CHAR_DATA *quitter;

   /*
    * Examine all mobs.
    */
   CREF(ch_next, CHAR_NEXT);

   for (ch = first_char; ch != NULL; ch = ch_next)
   {
      ch_next = ch->next;

      if (!IS_NPC(ch) || ch->in_room == NULL || IS_AFFECTED(ch, AFF_CHARM))
         continue;

      if (mob_is_time_restricted(ch))
      {
         if (ch->fighting == NULL)
            extract_char(ch, TRUE);
         continue;
      }

      /*
       * Examine call for special procedure
       */
      if (ch->spec_fun != 0)
      {
         if ((*ch->spec_fun)(ch))
            continue;
      }

      /*
       * Added by Aeria for mob hunting continuously
       */
      if (ch->searching != NULL)
      {
         if ((quitter = get_char_world(ch, ch->searching)) != NULL)
            set_hunt(ch, NULL, quitter, NULL,
                     HUNT_WORLD | HUNT_OPENDOOR | HUNT_PICKDOOR | HUNT_UNLOCKDOOR | HUNT_INFORM,
                     HUNT_CR | HUNT_MERC);
      }

      /*
       * DISABLED
       */
      /*
       * Intelligent mob?
       */
      /*	if ( IS_SET( ch->act, ACT_IS_NPC ) )
            int_handler( ch ); Disabled for now, for bugs.  */

      /*
       * That's all for sleeping / busy monster
       */
      if (ch->position < POS_STANDING)
         continue;

      /*
       * Check for rewield, and re-equip (specials not used anymore)
       */
      if (IS_SET(ch->act, ACT_REWIELD))
         if (check_rewield(ch))
            continue;

      if (IS_SET(ch->act, ACT_RE_EQUIP))
         if (check_re_equip(ch))
            continue;

      /*
       * Check for remember victims
       */
      if (ch->target != NULL && (target = get_char_room(ch, ch->target)) != NULL)
      {
         remember_attack(ch, target);
         continue;
      }

      /*
       * Check to see if mob is moving somewhere
       */
      /*	if ( mob_hunt(ch) )
           continue;
         if ( IS_SET( ch->act_hunt, ACT_HUNT_MOVE )
         && ch->move_to != NO_VNUM )
         {
            hunt_move( ch );
            continue;
         }
         */

      /*
       * program random trigger
       */
      if (ch->in_room->area->nplayer > 0)
      {
         /*
          * If ch dies or changes
          * position due to it's random
          * trigger continue - Kahn
          */
         if (ch->position < POS_STANDING)
            continue;
      }

      /*
       * Scavenge
       */
      if (IS_SET(ch->act, ACT_SCAVENGER) && ch->in_room->first_content != NULL &&
          number_bits(2) == 0)
      {
         OBJ_DATA *obj;
         OBJ_DATA *obj_best;
         int max;

         max = 1;
         obj_best = 0;
         for (obj = ch->in_room->first_content; obj; obj = obj->next_in_room)
         {
            if (CAN_WEAR(obj, ITEM_TAKE) && obj->cost > max)
            {
               obj_best = obj;
               max = obj->cost;
            }
         }

         if (obj_best)
         {
            obj_from_room(obj_best);
            obj_to_char(obj_best, ch);
            act("$n gets $p.", ch, obj_best, NULL, TO_ROOM);
         }
      }

      /*
       * Don't wander while waiting to deliver a dialogue response.
       */
      if (IS_SET(ch->act, ACT_AI_DIALOGUE) && ch->dlg_pending)
         continue;

      /*
       * Wander
       */
      if (!IS_SET(ch->act, ACT_INVASION) && !IS_SET(ch->act, ACT_SENTINEL) && ch->leader == NULL &&
          (door = number_bits(5)) <= 5 && (pexit = ch->in_room->exit[door]) != NULL &&
          pexit->to_room != NULL && !IS_SET(pexit->exit_info, EX_CLOSED) &&
          !IS_SET(pexit->to_room->room_flags, ROOM_NO_MOB) &&
          (!IS_SET(ch->act, ACT_STAY_AREA) || pexit->to_room->area == ch->in_room->area))
      {
         move_char(ch, door);
         /*
          * If ch changes position due
          * to it's or someother mob's
          * movement via scripts,
          * continue - Kahn
          */
         if (ch->position < POS_STANDING)
            continue;
      }

      /*
       * Flee
       */
      if (!IS_SET(ch->act, ACT_INVASION) && IS_SET(ch->act, ACT_WIMPY) &&
          ch->hit < (get_max_hp(ch) / 2) && (door = number_bits(3)) <= 5 &&
          (pexit = ch->in_room->exit[door]) != NULL && pexit->to_room != NULL &&
          !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_SET(pexit->to_room->room_flags, ROOM_NO_MOB))
      {
         CHAR_DATA *rch;
         bool found;

         found = FALSE;
         for (rch = pexit->to_room->first_person; rch != NULL; rch = rch->next_in_room)
         {
            if (!IS_NPC(rch))
            {
               found = TRUE;
               break;
            }
         }
         if (!found)
            move_char(ch, door);
      }
   }
   char_unreference(&ch_next);
   return;
}

void clean_donate_rooms(void)
{
   ROOM_INDEX_DATA *room = NULL;
   sh_int room_count, looper, chance;
   int room_vnum;
   OBJ_DATA *object, *obj_next;

   /*
    * normal donate rooms
    */
   if ((room = get_room_index(ROOM_VNUM_MISC_DONATE)) != NULL)
   {
      for (room_count = 0, object = room->first_content; object;
           room_count++, object = object->next_in_room)
         ;
      if (room_count > 150)
      {
         OREF(obj_next, OBJ_NEXT);
         for (object = room->first_content; object; object = obj_next)
         {
            obj_next = object->next_in_room;
            if (count_obj_room(object->pIndexData, room->first_content) > 5)
               chance = 50;
            else
               chance = 10;

            if ((IS_SET(object->extra_flags, ITEM_RARE)) || (object == quest_object))
               chance = 0;
            if (number_range(0, 99) < chance)
               extract_obj(object);
         }
         obj_unreference(&obj_next);
      }
   }
   if ((room = get_room_index(ROOM_VNUM_WEAPON_DONATE)) != NULL)
   {
      for (room_count = 0, object = room->first_content; object;
           room_count++, object = object->next_in_room)
         ;
      if (room_count > 150)
      {
         OREF(obj_next, OBJ_NEXT);
         for (object = room->first_content; object; object = obj_next)
         {
            obj_next = object->next_in_room;
            if (count_obj_room(object->pIndexData, room->first_content) > 5)
               chance = 50;
            else
               chance = 10;
            if ((IS_SET(object->extra_flags, ITEM_RARE)) || (object == quest_object))
               chance = 0;
            if (number_range(0, 99) < chance)
               extract_obj(object);
         }
         obj_unreference(&obj_next);
      }
   }
   if ((room = get_room_index(ROOM_VNUM_ARMOR_DONATE)) != NULL)
   {
      for (room_count = 0, object = room->first_content; object;
           room_count++, object = object->next_in_room)
         ;
      if (room_count > 150)
      {
         OREF(obj_next, OBJ_NEXT);
         for (object = room->first_content; object; object = obj_next)
         {
            obj_next = object->next_in_room;
            if (count_obj_room(object->pIndexData, room->first_content) > 5)
               chance = 50;
            else
               chance = 10;
            if ((IS_SET(object->extra_flags, ITEM_RARE)) || (object == quest_object))
               chance = 0;
            if (number_range(0, 99) < chance)
               extract_obj(object);
         }
         obj_unreference(&obj_next);
      }
   }

   /*
    * clan donate rooms
    */
   for (looper = 1; looper < MAX_CLAN; looper++)
   {
      if ((room_vnum = clan_table[looper].donat_room) == 0)
         continue;
      if ((room = get_room_index(room_vnum)) != NULL)
      {
         for (room_count = 0, object = room->first_content; object;
              room_count++, object = object->next_in_room)
            ;
         if (room_count > 150)
         {
            OREF(obj_next, OBJ_NEXT);
            for (object = room->first_content; object; object = obj_next)
            {
               obj_next = object->next_in_room;
               if (count_obj_room(object->pIndexData, room->first_content) > 5)
                  chance = 50;
               else
                  chance = 10;
               if ((IS_SET(object->extra_flags, ITEM_RARE)) || (object == quest_object))
                  chance = 0;
               if (number_range(0, 99) < chance)
                  extract_obj(object);
            }
            obj_unreference(&obj_next);
         }
      }
   }
}
void weather_update(void)
{
   char buf[MAX_STRING_LENGTH];
   char buf2[MSL];
   DESCRIPTOR_DATA *d;
   int diff;
   sh_int x, y;

   buf[0] = '\0';
   buf2[0] = '\0';

   if (happy_hour && happy_hour_ticks_remaining > 0)
   {
      happy_hour_ticks_remaining--;
      if (happy_hour_ticks_remaining == 0)
      {
         happy_hour = FALSE;
         info("Happy hour has ended!", 1);
         info("Xp has returned to normal!", 1);
      }
   }

   switch (++time_info.hour)
   {
   case 5:
      weather_info.sunlight = SUN_LIGHT;
      strcat(buf, "The sky shows signs of daybreak.\n\r");
      break;

   case 6:
      weather_info.sunlight = SUN_RISE;
      strcat(buf, "The sun rises in the east.\n\r");
      for (x = 1; x < MAX_CLAN; x++)
         for (y = 1; y < MAX_CLAN; y++)
            politics_data.daily_negotiate_table[x][y] = FALSE;
      purge_time_restricted_mobs(TRUE);
      break;
   case 12:

      for (x = 1; x < MAX_CLAN; x++)
         for (y = 1; y < MAX_CLAN; y++)
            politics_data.daily_negotiate_table[x][y] = FALSE;
      break;

   case 19:
      weather_info.sunlight = SUN_SET;
      strcat(buf, "The sun slowly disappears in the west.\n\r");
      for (x = 1; x < MAX_CLAN; x++)
         for (y = 1; y < MAX_CLAN; y++)
            politics_data.daily_negotiate_table[x][y] = FALSE;
      break;

   case 20:
      weather_info.sunlight = SUN_DARK;
      strcat(buf, "The night has begun.\n\r");
      purge_time_restricted_mobs(TRUE);
      break;

   case 24:
      time_info.hour = 0;
      time_info.day++;
      for (x = 1; x < MAX_CLAN; x++)
         for (y = 1; y < MAX_CLAN; y++)
            politics_data.daily_negotiate_table[x][y] = FALSE;
      clean_donate_rooms();

      if (happy_hour)
      {
         happy_hour = FALSE;
         happy_hour_ticks_remaining = 0;
         info("Happy hour has ended!", 1);
         info("Xp has returned to normal!", 1);
      }
      break;
   }
   switch (time_info.moon++)
   {
   case 5:
      weather_info.moon_loc = MOON_RISE;
      sprintf(buf2, "@@NA %s @@yMoon @@Nhas risen.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;
   case 10:
      weather_info.moon_loc = MOON_LOW;
      sprintf(buf2, "@@NThe %s @@yMoon @@Nrides low on the horizon.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;
   case 15:
      weather_info.moon_loc = MOON_PEAK;
      sprintf(buf2, "@@NThe %s @@yMoon @@Nreaches it's zenith.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;
   case 20:
      weather_info.moon_loc = MOON_FALL;
      sprintf(buf2, "@@NThe %s @@yMoon @@Nfalls.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;
   case 25:
      weather_info.moon_loc = MOON_SET;
      sprintf(buf2, "@@NThe %s @@yMoon @@Nis setting.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;
   case 30:
      weather_info.moon_loc = MOON_DOWN;
      sprintf(buf2, "@@NThe %s @@yMoon @@Nhas left the sky.\n\r", get_moon_phase_name());
      safe_strcat(MSL, buf, buf2);
      break;

   default:
      break;
   }

   if (time_info.moon >= 50)
   {
      time_info.moon = 0;
      weather_info.moon_loc = MOON_DOWN;
   }

   if (time_info.day >= 20) /* now 20 days = 1 month */
   {
      time_info.day = 0;
      time_info.month++;
   }

   if (time_info.month >= 8) /* 8 months a year */
   {
      time_info.month = 0;
      time_info.year++;
   }
   if (((time_info.day) % 4) == 0)
   {
      if (!weather_info.phase_changed)
         weather_info.moon_phase++;
      if (weather_info.moon_phase > MOON_WAN_CRE)
         weather_info.moon_phase = MOON_NEW;
      weather_info.phase_changed = TRUE;
   }
   else
      weather_info.phase_changed = FALSE;

   /*
    * Weather change.
    */
   if (time_info.month >= 9 && time_info.month <= 16)
      diff = weather_info.mmhg > 985 ? -2 : 2;
   else
      diff = weather_info.mmhg > 1015 ? -2 : 2;

   weather_info.change += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
   weather_info.change = UMAX(weather_info.change, -12);
   weather_info.change = UMIN(weather_info.change, 12);

   weather_info.mmhg += weather_info.change;
   weather_info.mmhg = UMAX(weather_info.mmhg, 960);
   weather_info.mmhg = UMIN(weather_info.mmhg, 1040);

   switch (weather_info.sky)
   {
   default:
      bug("Weather_update: bad sky %d.", weather_info.sky);
      weather_info.sky = SKY_CLOUDLESS;
      break;

   case SKY_CLOUDLESS:
      if (weather_info.mmhg < 990 || (weather_info.mmhg < 1010 && number_bits(2) == 0))
      {
         strcat(buf, "The sky is getting cloudy.\n\r");
         weather_info.sky = SKY_CLOUDY;
      }
      break;

   case SKY_CLOUDY:
      if (weather_info.mmhg < 970 || (weather_info.mmhg < 990 && number_bits(2) == 0))
      {
         strcat(buf, "It starts to rain.\n\r");
         weather_info.sky = SKY_RAINING;
      }

      if (weather_info.mmhg > 1030 && number_bits(2) == 0)
      {
         strcat(buf, "The clouds disappear.\n\r");
         weather_info.sky = SKY_CLOUDLESS;
      }
      break;

   case SKY_RAINING:
      if (weather_info.mmhg < 970 && number_bits(2) == 0)
      {
         strcat(buf, "Lightning flashes in the sky.\n\r");
         weather_info.sky = SKY_LIGHTNING;
      }

      if (weather_info.mmhg > 1030 || (weather_info.mmhg > 1010 && number_bits(2) == 0))
      {
         strcat(buf, "The rain stopped.\n\r");
         weather_info.sky = SKY_CLOUDY;
      }
      break;

   case SKY_LIGHTNING:
      if (weather_info.mmhg > 1010 || (weather_info.mmhg > 990 && number_bits(2) == 0))
      {
         strcat(buf, "The lightning has stopped.\n\r");
         weather_info.sky = SKY_RAINING;
         break;
      }
      break;
   }

   if (buf[0] != '\0')
   {
      for (d = first_desc; d != NULL; d = d->next)
      {
         if (d->connected == CON_PLAYING && IS_OUTSIDE(d->character) &&
             (d->character->position != POS_WRITING) && IS_AWAKE(d->character))
            send_to_char(buf, d->character);
      }
   }

   return;
}

/* New update loop to handle gains for players => smaller 'ticks' for
   hp/mana/move gain, normal 'ticks' for objects, affects, weather, etc */
void gain_update(void)
{
   CHAR_DATA *ch;

/* send wholist to web page :) Zen  !-!-! ONLY RUN THIS ON ONE PORT OF YOUR SERVER !-! */
#if defined(SOE) && !defined(SOETEST) && !defined(SOEBLD)
   list_who_to_output();
#endif
   /* Update super_councils info  */

   {

      MEMBER_DATA *imember;
      sh_int count = 0;
      sh_int council_index;

      for (council_index = 1; council_index < MAX_SUPER; council_index++)
      {
         if (super_councils[council_index].council_time > 0)
         {
            super_councils[council_index].council_time--;

            if (super_councils[council_index].council_time == 1)
            {
               MEMBER_DATA *imember_next;
               for (imember = super_councils[council_index].first_member; imember != NULL;
                    imember = imember_next)
               {
                  imember_next = imember->next;
                  send_to_char("The current council is disbanded.\n\r", imember->this_member);
                  UNLINK(imember, super_councils[council_index].first_member,
                         super_councils[council_index].last_member, next, prev);
                  imember->this_member = NULL;
                  imember->next = NULL;
                  imember->prev = NULL;
                  PUT_FREE(imember, member_free);
               }
               super_councils[council_index].council_time = 0;
               super_councils[council_index].quorum = FALSE;
            }
         }
         if (!super_councils[council_index].quorum)
         {
            super_councils[council_index].quorum = FALSE;
            for (imember = super_councils[council_index].first_member; imember != NULL;
                 imember = imember->next)
            {
               count++;
            }
            if (count >= QUORUM_NUMBER)
            {
               super_councils[council_index].quorum = TRUE;
               super_councils[council_index].council_time = 10;
               for (imember = super_councils[council_index].first_member; imember != NULL;
                    imember = imember->next)
                  send_to_char("The Council is in Session!\n\r", imember->this_member);
            }
         }
      }
   }

   for (ch = first_char; ch != NULL; ch = ch->next)
   {
      if (ch->is_free != FALSE)
         continue;
      if (ch->position >= POS_STUNNED)
      {
         if ((ch->hit < get_max_hp(ch)) && (!IS_SET(ch->in_room->affected_by, ROOM_BV_NONE)))
            ch->hit = UMIN(get_max_hp(ch), ch->hit + hit_gain(ch));
         if (!IS_NPC(ch))
            ch->hit = UMAX(25, ch->hit);

         if ((ch->mana < get_max_mana(ch)) && (!IS_SET(ch->in_room->affected_by, ROOM_BV_NONE)))
            ch->mana = UMIN(get_max_mana(ch), ch->mana + mana_gain(ch));

         if ((ch->move < get_max_move(ch)) && (ch->carry_weight < can_carry_w(ch)))
            ch->move = UMIN(get_max_move(ch), ch->move + move_gain(ch));
         else if (ch->carry_weight >= can_carry_w(ch))
         {
            send_to_char("You are carrying so much weight that you are @@eEXHAUSTED@@N!!\n\r", ch);
            ch->move = 0;
         }
      }

      if (ch->position == POS_STUNNED)
         update_pos(ch);
   }
   return;
}

/*
 * Update all chars, including mobs.
 * This function is performance sensitive.
 */
void char_update(void)
{
   CHAR_DATA *ch;
   CHAR_DATA *ch_next;
   CHAR_DATA *ch_save;
   CHAR_DATA *ch_quit;
   time_t save_time;

   save_time = current_time;
   ch_save = NULL;
   ch_quit = NULL;

   CREF(ch_next, CHAR_NEXT);
   for (ch = first_char; ch != NULL; ch = ch_next)
   {
      ch_next = ch->next;
      if (ch->is_free != FALSE)
         continue;

      /*
       * Find dude with oldest save time.
       */
      if (!IS_NPC(ch) && (ch->desc == NULL || ch->desc->connected == CON_PLAYING) &&
          ch->level >= 2 && ch->save_time < save_time)
      {
         ch_save = ch;
         save_time = ch->save_time;
      }

      if ((!IS_NPC(ch)) && (ch->pcdata->has_exp_fix == 0) && (ch->level > 1))
      {
         send_to_char("@@eWE HAVE CHANGED THE EXP SYSTEM. YOUR EXP HAS BEEN RESET TO 0.@@N\n\r",
                      ch);
         send_to_char("@@ePLEASE CONTACT STAFF, AND YOU WILL BE ADVANCED ONE CLASS, UNLESS "
                      "YOU ARE A NEW CHARACTER.@@N\n\r",
                      ch);
         send_to_char(
             "@@mTHANK YOU!!!!@@N  NOTE: DO NOT ATTEMPT TO ABUSE THIS--WE ARE KEEPING TRACK!\n\r",
             ch);
         ch->pcdata->has_exp_fix = 1;
         ch->exp = 0;
      }
      else if (!IS_NPC(ch))
         ch->pcdata->has_exp_fix = 1;

      if ((IS_NPC(ch)) && (ch->hit < -15))
         raw_kill(ch, "");

      if (ch->sitting != NULL && ch->sitting->in_room != ch->in_room)
      {
         ch->sitting->value[1]--;
         ch->sitting = NULL;
      }

      if (ch->position >= POS_STUNNED)
      {
         /*
          * -S- mod.
          */
         if (ch->position != POS_WRITING && ch->position != POS_BUILDING)
         {
            if (IS_SET(ch->in_room->room_flags, ROOM_HOT))
               send_to_char("You feel your skin burning.\n\r", ch);
            else if (IS_SET(ch->in_room->room_flags, ROOM_COLD))
               send_to_char("You feel your skin freezing.\n\r", ch);
         }
      }

      /* Overgrowth decay: out of combat, decay 1/tick (2 if resting/sleeping) */
      if (ch->overgrowth > 0 && !is_fighting(ch) && ch->wait == 0)
      {
         int decay = (ch->position == POS_RESTING || ch->position == POS_SLEEPING) ? 2 : 1;
         ch->overgrowth -= decay;
         if (ch->overgrowth < 0)
            ch->overgrowth = 0;
      }

      /*
       * if ( ch->stunTimer > 0 )
       * {
       * ch->position = POS_STUNNED;
       * ch->stunTimer -= 1;
       * }
       * else
       * {
       * ch->stunTimer = 0;
       * ch->position = POS_STANDING;
       *
       * }
       */

      if ((!IS_NPC(ch) && ch->level < LEVEL_STAFF))
      {

         OBJ_DATA *obj;

         if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL && obj->item_type == ITEM_LIGHT)
         {
            /* Worn lights now remain lit indefinitely. */
         }

         if (++ch->timer >= 12)
         {
            if (ch->was_in_room == NULL && ch->in_room != NULL)
            {
               ch->was_in_room = ch->in_room;
               if (is_fighting(ch))
                  stop_fighting(ch, TRUE);
               act("$n disappears into the void.", ch, NULL, NULL, TO_ROOM);
               send_to_char("You disappear into the void.\n\r", ch);
               save_char_obj(ch);
               char_from_room(ch);
               char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
            }
         }

         if (ch->timer > 30)
            ch_quit = ch;
      }

      update_buff_duration(ch, DURATION_HOUR);

      /*
       * Careful with the damages here,
       *   MUST NOT refer to ch after damage taken,
       *   as it may be lethal damage (on NPC).
       */
      if (IS_NPC(ch))
      {
         if (ch->target != NULL && number_bits(4) == 0)
         {
            free_string(ch->target);
            ch->target = NULL;
         }
         if (ch->extract_timer > 1)
         {
            ch->extract_timer--;
         }
         else if (ch->extract_timer == 1)
         {
            if ((ch->master == NULL) || (ch->master->in_room == NULL) ||
                (ch->in_room != ch->master->in_room))
            {
               if (ch->in_room != NULL)
               {
                  do_say(ch, "Whaa?? Where am I? How did I get here?");
                  do_say(ch, "AHHH!!! Help me!!!! I'm MELTING......");
               }
               extract_char(ch, TRUE);
               continue;
            }
            else
            {
               if (number_range(0, 99) < get_psuedo_level(ch->master) - 25)
               {
                  CHAR_DATA *this_master;
                  this_master = ch->master;
                  do_say(ch, "Whaa?? Where am I? How did I get here?");
                  do_scan(ch, "");
                  check_social(ch, "growl", ch->master->name);
                  do_say(ch, "How dare you order me around!!!");
                  stop_follower(ch);
                  multi_hit(ch, this_master, TYPE_UNDEFINED);
                  continue;
               }
            }
         }
      }

      if (IS_AFFECTED(ch, AFF_POISON))
      {
         act("$n shivers and suffers.", ch, NULL, NULL, TO_ROOM);
         send_to_char("You shiver and suffer.\n\r", ch);
         damage(ch, ch, number_range(2, 8), gsn_poison);
      }
      else if (ch->position == POS_INCAP)
      {
         damage(ch, ch, number_range(1, 4), TYPE_UNDEFINED);
      }
      else if (ch->position == POS_MORTAL)
      {
         damage(ch, ch, number_range(2, 3), TYPE_UNDEFINED);
      }
      else if (ch->position == POS_DEAD)
      {
         damage(ch, ch, number_range(5, 10), TYPE_UNDEFINED);
      }
      else if (ch->hit < -10)
      {
         damage(ch, ch, number_range(5, 10), TYPE_UNDEFINED);
      }
   }
   char_unreference(&ch_next);

   /*
    * Autosave and autoquit.
    * Check that these chars still exist.
    */
   if (ch_save != NULL || ch_quit != NULL)
   {
      CREF(ch_next, CHAR_NEXT);

      for (ch = first_char; ch != NULL; ch = ch_next)
      {
         ch_next = ch->next;
         if (ch == ch_save)
            save_char_obj(ch);
         if (ch == ch_quit)
         {
            send_to_char("Idle for too long.  Bye Bye!\n\r", ch);
            do_quit(ch, "");
         }
      }
      char_unreference(&ch_next);
   }

   return;
}

/* Check for objfuns.... this is probably performance sensitive too. */
void objfun_update(void)
{
   OBJ_DATA *marker;
   OBJ_DATA *obj;

   /*
    * Use the same marker pattern as obj_update() so that a callback which
    * calls extract_obj() on the current object cannot invalidate the
    * iterator.  We move each object to the tail before invoking its
    * callback; the marker at the tail signals when every object has been
    * visited.
    */
   GET_FREE(marker, obj_free);
   LINK(marker, first_obj, last_obj, next, prev);

   while ((obj = first_obj) != marker)
   {
      UNLINK(obj, first_obj, last_obj, next, prev);
      LINK(obj, first_obj, last_obj, next, prev);

      if (obj->obj_fun != NULL)
      {
         (*obj->obj_fun)(obj, obj->carried_by);
      }
   }

   UNLINK(marker, first_obj, last_obj, next, prev);
   PUT_FREE(marker, obj_free);
   return;
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update(void)
{

   OBJ_DATA *marker;
   OBJ_DATA *obj;

   /*
    * Create dummy object and add to end of list.  This object is
    * only a marker, and will not actually be processed by this
    * routine.
    */
   GET_FREE(marker, obj_free);
   LINK(marker, first_obj, last_obj, next, prev);

   /*
    * Repeatedly remove object from front of list, add to tail, and process
    * until the marker is at the head of the list.  That will indicate all
    * objects have been processed.
    */
   disable_timer_abort = FALSE;
   while ((obj = first_obj) != marker)
   {

      CHAR_DATA *rch;
      char *message;

      UNLINK(obj, first_obj, last_obj, next, prev);
      LINK(obj, first_obj, last_obj, next, prev);

      if (obj == auction_item)
         continue;
      if (IS_SET(obj->item_apply, ITEM_APPLY_HEATED) && number_range(0, 100) < 33)
      {
         REMOVE_BIT(obj->item_apply, ITEM_APPLY_HEATED);
         if (obj->carried_by != NULL)
         {
            act("Your $p @@acools off@@N!!", obj->carried_by, obj, NULL, TO_CHAR);
         }
         else if (obj->in_room != NULL && (rch = obj->in_room->first_person) != NULL)
         {
            act("$p @@acools off@@N!!", rch, obj, NULL, TO_ROOM);
            act("$p @@acools off@@N!!", rch, obj, NULL, TO_CHAR);
         }
      }

      if (obj->timer <= 0 || --obj->timer > 0)
         continue;
      switch (obj->item_type)
      {
      default:
         message = "$p vanishes.";
         break;
      case ITEM_CORPSE_NPC:
         message = "$p decays into dust.";
         break;
      case ITEM_CORPSE_PC:
         message = "$p vapourises, and goes to heaven.";
         break;
      case ITEM_PORTAL:
         message = "$p implodes suddenly.";
         break;
      }

      if (obj->carried_by != NULL)
      {
         act(message, obj->carried_by, obj, NULL, TO_CHAR);
      }
      else if (obj->in_room != NULL && (rch = obj->in_room->first_person) != NULL)
      {
         act(message, rch, obj, NULL, TO_ROOM);
         act(message, rch, obj, NULL, TO_CHAR);
      }
      /*
            if ( obj->in_room == NULL )
              continue;
            if ( obj->item_type == ITEM_CORPSE_NPC )
              continue;
      */
      if (obj->value[7] > 0)
      {
         OBJ_INDEX_DATA *new_index;
         OBJ_DATA *replacer;
         if ((new_index = get_obj_index(obj->value[6])) == NULL)
         {
            sprintf(bug_buf,
                    "ERROR in expiring item %s(%s %d): item has a replace vnum set (%d), but that "
                    "is not a valid item.",
                    obj->name, obj->pIndexData->area->keyword, obj->pIndexData->vnum,
                    obj->value[6]);
            monitor_chan(bug_buf, MONITOR_OBJ);
            log_f("%s", bug_buf);
         }
         else
         {
            replacer = create_object(new_index, obj->level);
            if (obj->carried_by != NULL)
               obj_to_char(replacer, obj->carried_by);
            else if (obj->in_room != NULL)
               obj_to_room(replacer, obj->in_room);
            else if (obj->in_obj != NULL)
               obj_to_obj(replacer, obj->in_obj);
         }
      }
      extract_obj(obj);
   }

   /*
    * All objects have been processed.  Remove the marker object and
    * put it back on the free list.
    */
   UNLINK(marker, first_obj, last_obj, next, prev);
   PUT_FREE(marker, obj_free);

   disable_timer_abort = FALSE;
   return;
}

/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update(void)
{

   /*
    * Check to see if ch has encountered a mob with ACT_REMEMBER set,
    * * and with victim->target == ch->name...    tbc ;)
    * * -- Stephen
    */

   CHAR_DATA *wch;
   CHAR_DATA *wch_next;
   CHAR_DATA *ch;
   CHAR_DATA *ch_next;
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   CHAR_DATA *victim;
   OBJ_DATA *wield;

   CREF(wch_next, CHAR_NEXT);

   for (wch = first_char; wch != NULL; wch = wch_next)
   {
      wch_next = wch->next;
      if (wch->is_free != FALSE)
         continue;
      if (IS_NPC(wch) && wch->mpactnum > 0 && wch->in_room->area->nplayer > 0)
      {
         MPROG_ACT_LIST *mpact;

         while ((mpact = wch->first_mpact) != NULL)
         {
            /*
             * Lets hope this check works until something better is in place.
             * * -- Alty
             */
            if (wch->hit < -10)
               break;
            wch->first_mpact = mpact->next;
            PUT_FREE(mpact, mpact_free);
         }
         if (wch->hit < -10)
            continue;
         wch->mpactnum = 0;
         wch->first_mpact = NULL;
         wch->last_mpact = NULL;
      }

      if ((IS_NPC(wch)) || wch->level >= LEVEL_STAFF || wch->in_room == NULL)
         continue;
      CREF(ch_next, CHAR_NEXTROOM);
      for (ch = wch->in_room->first_person; ch != NULL; ch = ch_next)
      {
         int count;

         ch_next = ch->next_in_room;

         if (!IS_NPC(ch) || !IS_SET(ch->act, ACT_AGGRESSIVE) || is_fighting(ch) ||
             ch->hunting != NULL || IS_AFFECTED(ch, AFF_CHARM) || !IS_AWAKE(ch) ||
             (IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch)) || !can_see(ch, wch))
            continue;

         if ((IS_AFFECTED(wch, AFF_SNEAK) || item_has_apply(wch, ITEM_APPLY_SNEAK)) &&
             (number_percent() < 50 + (2 * (get_psuedo_level(wch) - get_psuedo_level(ch)))))
            continue;
         /*
          * Ok we have a 'wch' player character and a 'ch' npc aggressor.
          * MAG - wch can be an intelligent NPC.
          * Now make the aggressor fight a RANDOM pc victim in the room,
          *   giving each 'vch' an equal chance of selection.
          */
         count = 0;
         victim = NULL;
         CREF(vch_next, CHAR_NEXTROOM);
         for (vch = wch->in_room->first_person; vch != NULL; vch = vch_next)
         {
            vch_next = vch->next_in_room;

            if (!IS_NPC(vch) && vch->level < LEVEL_STAFF &&
                (!IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch)) && can_see(ch, vch))
            {
               if (number_range(0, count) == 0)
                  victim = vch;
               count++;
            }
         }
         char_unreference(&vch_next);
         if (victim == NULL)
         {
            /*
             * bug( "Aggr_update: null victim.", count );
             */
            continue;
         }
         if (IS_SET(victim->in_room->room_flags, ROOM_SAFE))
            continue;

         act("$n growls at $N!", victim, NULL, ch, TO_NOTVICT);
         act("$N growls at you!  Uh-oh!!", victim, NULL, ch, TO_CHAR);
         act("You growl at $N.  Get $M!!", ch, NULL, victim, TO_CHAR);

         wield = get_eq_char(ch, WEAR_HOLD_HAND_L);
         if (wield != NULL && wield->item_type == ITEM_WEAPON && wield->value[3] == 11 &&
             victim->fighting == NULL)
            do_backstab(ch, victim->name);
         else
            multi_hit(ch, victim, TYPE_UNDEFINED);
      }
      char_unreference(&ch_next);
   }
   char_unreference(&wch_next);
   return;
}

/*
 * Check ALL rooms for affects... the ratio of affects to rooms should
 * be relatively low, so this shouldn't hit performance too much.
 * -S-
 */

void rooms_update(void)
{
   ROOM_INDEX_DATA *room;
   AREA_DATA *area;
   BUILD_DATA_LIST *thing;
   ROOM_AFFECT_DATA *raf;
   ROOM_AFFECT_DATA *raf_next;
   MARK_LIST_MEMBER *this_mark;
   MARK_LIST_MEMBER *next_mark;

   for (area = first_area; area != NULL; area = area->next)
   {
      for (thing = area->first_area_room; thing != NULL; thing = thing->next)
      {
         room = thing->data;

         /*
          * if ( room->first_room_affect == NULL )
          * continue;
          */

         for (this_mark = room->first_mark_list; this_mark != NULL; this_mark = next_mark)
         {
            next_mark = this_mark->next;
            if (this_mark->mark->duration > 0)
               this_mark->mark->duration--;
            else if (this_mark->mark->duration < 0)
               ;
            else
            {
               mark_from_room(room->vnum, this_mark->mark);
            }
         }

         for (raf = room->first_room_affect; raf != NULL; raf = raf_next)
         {
            raf_next = raf->next;

            if (raf->duration > 0)
               raf->duration--;
            else if (raf->duration < 0)
               ;
            else
            {
               if (raf_next == NULL || raf_next->type != raf->type || raf_next->duration > 0)
               {
                  if (raf->type > 0 && skill_table[raf->type].msg_off)
                  {
                     send_to_room(skill_table[raf->type].msg_off, room);
                     send_to_room("\n\r", room);
                  }
                  r_affect_remove(room, raf);
               }
            }
         }
      }
   }
   return;
}

extern void build_save_flush(void);

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */
static void hotreboot_update(void)
{
   static int pulse_trigger_check = PULSE_PER_SECOND;
   CHAR_DATA *ch;
   CHAR_DATA *ch_next;

   /* Check for trigger file once per second */
   if (--pulse_trigger_check <= 0)
   {
      pulse_trigger_check = PULSE_PER_SECOND;
      if (hotreboot_countdown == 0)
      {
         FILE *fp = fopen(HOTREBOOT_FILE, "r");
         if (fp != NULL)
         {
            fclose(fp);
            unlink(HOTREBOOT_FILE);
            hotreboot_countdown = 60 * PULSE_PER_SECOND;
            for (ch = first_char; ch != NULL; ch = ch_next)
            {
               ch_next = ch->next;
               if (!IS_NPC(ch))
                  send_to_char("\n\r@@e** ACK! MUD will hotreboot in 60 seconds for a code update! "
                               "**@@N\n\r",
                               ch);
            }
            log_string("Deployment hotreboot: 60-second warning issued.");
         }
      }
   }

   if (hotreboot_countdown > 0)
   {
      hotreboot_countdown--;
      if (hotreboot_warning_30s_due(hotreboot_countdown))
      {
         for (ch = first_char; ch != NULL; ch = ch_next)
         {
            ch_next = ch->next;
            if (!IS_NPC(ch))
               send_to_char(
                   "\n\r@@e** ACK! MUD will hotreboot in 30 seconds for a code update! **@@N\n\r",
                   ch);
         }
         log_string("Deployment hotreboot: 30-second warning issued.");
      }
      else if (hotreboot_execute_due(hotreboot_countdown))
      {
         log_string("Deployment hotreboot: executing hotreboot now.");
         do_hotreboot(NULL, "");
      }
   }
}

void update_handler(void)
{
   npc_dialogue_deliver();

   static int pulse_message;
   static int objfun_check;
   static int pulse_area;
   static int pulse_rooms;
   static int pulse_gain;
   static int pulse_violence;
   static int pulse_point;
   static int pulse_auction;
   extern int saving_area;

   if (saving_area)
      build_save(); /* For incremental area saving */

   if (--pulse_area <= 0)
   {
      pulse_area = number_range(PULSE_AREA / 2, 3 * PULSE_AREA / 2);
      area_update();
      build_save_flush();
   }

   if (--pulse_rooms <= 0)
   {
      pulse_rooms = PULSE_ROOMS;
      rooms_update();
      invasion_rooms_update();
   }

   if (--pulse_message <= 0)
   {
      pulse_message = PULSE_MESSAGE;
      message_update();
   }

   if (auction_flop)
   {
      pulse_auction = PULSE_AUCTION;
      auction_flop = FALSE;
   }

   if (--pulse_auction <= 0)
   {
      pulse_auction = PULSE_AUCTION;
      auction_update();
   }

   if (--objfun_check <= 0)
   {
      objfun_check = PULSE_OBJFUN;
      objfun_update();
   }

   if (pulse_violence == PULSE_VIOLENCE / 2)
   {
      ai_update();
      mobile_update();
   }

   if (--pulse_violence <= 0)
   {
      alarm_update();
      pulse_violence = PULSE_VIOLENCE;
      log_f("update: violence_update");
      violence_update();
      log_f("update: round_update");
      round_update();
      log_f("update: msdp_update");
      msdp_update();
      log_f("update: gmcp_update");
      gmcp_update();
      log_f("update: violence block done");
   }

   if (--pulse_gain <= 0)
   {
      log_f("update: gain_update");
      gain_update();
      pulse_gain = PULSE_PER_SECOND * number_range(5, 8);
   }

   if (--pulse_point <= 0)
   {
      pulse_point = PULSE_TICK;
      weather_update();
      log_f("update: char_update");
      char_update();
      log_f("update: obj_update");
      obj_update();
      quest_update();
      invasion_update();
      caravan_update();

      /*
       * This will log the number of perms being used...
       * * fgrep the log file to get results...
       */

      /*
       * perm_update( );
       */
   }

   aggr_update();
   hotreboot_update();
   tail_chain();
   return;
}

bool check_rewield(CHAR_DATA *ch)
{
   OBJ_DATA *obj;
   OBJ_DATA *weapon = NULL;
   int dam;
   int chance;
   bool pickup;
   char buf[MAX_STRING_LENGTH];

   pickup = TRUE;
   dam = 0;

   chance = (!is_fighting(ch) ? 35 : 60);

   if (number_percent() < chance)
   {
      for (obj = ch->first_carry; obj != NULL; obj = obj->next_in_carry_list)
         if (obj->item_type == ITEM_WEAPON && dam < obj->value[2])
         {
            dam = obj->value[2];
            pickup = FALSE;
            weapon = obj;
         }

      /*
       * Then check inventory and room for any weapons
       */
      for (obj = ch->in_room->first_content; obj != NULL; obj = obj->next_in_room)
      {
         if (obj->item_type == ITEM_WEAPON)
         {
            if (obj->value[2] > dam)
            {
               dam = obj->value[2];
               weapon = obj;
               pickup = TRUE;
            }
         }
      }

      if (weapon == NULL)
         return FALSE;

      if (weapon->wear_loc == WEAR_HOLD_HAND_L)
         return FALSE;

      if (pickup)
      {
         sprintf(buf, "Great!  %s!  Just what i've always wanted!", weapon->short_descr);
         do_say(ch, buf);
      }

      if (weapon != NULL)
      {
         /*
          * Now make the mob get the weapon
          */
         if (pickup)
            get_obj(ch, weapon, NULL);

         do_wear(ch, weapon->name);

         /*
          * Check is mob wielded weapon ok...
          */
         if (weapon->wear_loc == WEAR_NONE && weapon != quest_object)
         {
            act("$n sniffs sadly.  'Baah!  It's no good to me!'", ch, NULL, NULL, TO_ROOM);
            act("$n sacrifices it.", ch, NULL, NULL, TO_ROOM);
         }
         return TRUE;
      }
   }

   return FALSE;
}

bool check_re_equip(CHAR_DATA *ch)
{
   OBJ_DATA *obj;
   OBJ_DATA *obj2;
   OBJ_DATA *armor = NULL;
   OBJ_DATA *light = NULL;
   int chance;
   bool pickup;
   bool ident;
   char buf[MAX_STRING_LENGTH];
   int objnum;
   pickup = TRUE;

   chance = (!is_fighting(ch) ? 35 : 60);
   if (number_percent() < chance)
   {
      /*
       * Check each armor in room against ch's equipment
       */

      ident = FALSE;
      for (obj = ch->in_room->first_content; obj != NULL; obj = obj->next_in_room)
      {
         if (!can_see_obj(ch, obj))
            continue;

         if ((obj->wear_flags & ~ITEM_TAKE) == 0)
            continue; /* Check to see if item cannot be worn */
         if (obj->item_type == ITEM_PIECE)
            continue;
         if (obj->item_type == ITEM_ARMOR)
         {
            /*
             * Check this object against our equiped objects
             */
            ident = FALSE;
            for (obj2 = ch->first_carry; obj2 != NULL; obj2 = obj2->next_in_carry_list)
            {
               /*
                * Only scan against worn objects.
                * * If obj2 is being worn in a position that obj can be worn in,
                * * and obj2->value[0] is better, then choose it.
                */

               if (obj2->wear_loc != WEAR_NONE && obj2->item_type == ITEM_ARMOR &&
                   can_wear_at(ch, obj, obj2->wear_loc) && obj->value[0] > obj2->value[0])
               {
                  ident = TRUE; /* identical wear_loc */
                  armor = obj;
                  break;
               }
            }

            /*
             * Found no match for locations, so get and wear.
             */
            if (!ident)
            {
               armor = obj;
               break;
            }
         }

         if (obj->item_type == ITEM_LIGHT && (get_eq_char(ch, WEAR_LIGHT) == NULL))
         {
            light = obj;
            break;
         }
      }

      /*
       * MAG Modification. Only check one item each time, against currently
       * worn object.
       */

      /*
       * Check one inv item against worn eq, incase we've picked up some nicer
       * stuff
       */

      objnum = number_percent() * ch->carry_number / 100;
      for (obj = ch->first_carry; obj != NULL && objnum > 0; obj = obj->next_in_carry_list)
         objnum--;

      if (obj != NULL && obj->wear_loc == WEAR_NONE && obj->item_type == ITEM_ARMOR)
      {
         ident = FALSE;
         for (obj2 = ch->first_carry; obj2 != NULL; obj2 = obj2->next_in_carry_list)
         {
            if (obj2->wear_loc != WEAR_NONE && can_wear_at(ch, obj, obj2->wear_loc) &&
                obj->value[0] > obj2->value[0])
            {
               ident = TRUE;
               armor = obj;
               break;
            }
         }
         if (!ident)
         {
            armor = obj;
         }
      }

      if (obj != NULL && obj->item_type == ITEM_LIGHT && (get_eq_char(ch, WEAR_LIGHT) == NULL))
      {
         light = obj;
      }
   }

   if (armor != NULL)
   {
      if (armor->carried_by != ch)
      {
         /*
          * Pick up off ground
          */
         if (pickup)
         {
            sprintf(buf, "Great!  %s!  Just what i've always wanted!", armor->short_descr);
            do_say(ch, buf);
         }

         /*
          * Now make the mob get the armor
          */
         if (pickup)
            get_obj(ch, armor, NULL);
      }

      do_wear(ch, armor->name);

      /*
       * Check is mob wielded weapon ok...
       */
      if (armor->wear_loc == WEAR_NONE && armor != quest_object)
      {
         act("$n sniffs sadly.  'Baah!  It's no good to me!'", ch, NULL, NULL, TO_ROOM);
         act("$n sacrifices it.", ch, NULL, NULL, TO_ROOM);
      }
      return TRUE;
   }

   if (light != NULL)
   {
      if (light->carried_by != ch)
      {
         /*
          * Pick up off ground
          */
         if (pickup)
         {
            sprintf(buf, "Great!  %s!  Just what i've always wanted!", light->short_descr);
            do_say(ch, buf);
         }

         /*
          * Now make the mob get the light
          */
         if (pickup)
            get_obj(ch, light, NULL);
      }

      do_wear(ch, light->name);

      /*
       * Check is mob wielded weapon ok...
       */
      if (light->wear_loc == WEAR_NONE && light != quest_object)
      {
         act("$n sniffs sadly.  'Baah!  It's no good to me!'", ch, NULL, NULL, TO_ROOM);
         act("$n sacrifices it.", ch, NULL, NULL, TO_ROOM);
      }
      return TRUE;
   }

   return FALSE;
}

void auction_update(void)
{
   char buf[MAX_STRING_LENGTH];

   CHAR_DATA *ach;
   bool good_seller = FALSE, good_buyer = FALSE;

   /*
    * Stages: 0) No/New bid.
    * 1) Waiting.  (If no bid here, then give up next time)
    * 2) Going once.
    * 3) Going Twice.
    * 4) GONE!
    */

   if (auction_item == NULL)
      return;

   switch (auction_stage)
   {
   case 0:
      if (auction_bidder == NULL)
      {
         sprintf(buf,
                 "@@N%s (level:%d, valued at %d) has been offered for auction.  A @@e10%% fee@@N "
                 "will be charged, the higher of the reserve price or highest bid.",
                 auction_item->short_descr, auction_item->level, auction_item->cost);
      }
      else
      {
         sprintf(buf, "%s has bid %d for %s.", auction_bidder->name, auction_bid,
                 auction_item->short_descr);
      }
      break;
   case 1:
      if (auction_bidder == NULL)
         sprintf(buf, "Last chance to bid for %s.", auction_item->short_descr);
      else
         sprintf(buf, "Last bid for %s was %d.  Any more offers?", auction_item->short_descr,
                 auction_bid);
      break;
   case 2:
      if (auction_bidder == NULL)
      {
         auction("No bidders.  Auction Ended.");

         for (ach = first_char; ach != NULL; ach = ach->next)
         {
            if (auction_owner == ach)
               good_seller = TRUE;
            if (auction_bidder == ach)
               good_buyer = TRUE;
         }

         if (good_seller)
         {
            obj_to_char(auction_item, auction_owner);
         }
         else
         {
            auction("Oh, well..guess they didn't want it anymore, since they LEFT!!  Well, it's "
                    "mine now! ");
            extract_obj(auction_item);
         }
         auction_item = NULL;
         return;
      }
      sprintf(buf, "%s - Going Once!", auction_item->short_descr);
      break;
   case 3:
      sprintf(buf, "%s - Going TWICE!", auction_item->short_descr);
      break;
   case 4:
      if (auction_bid < auction_reserve)
      {
         for (ach = first_char; ach != NULL; ach = ach->next)
         {
            if (auction_owner == ach)
               good_seller = TRUE;
            if (auction_bidder == ach)
               good_buyer = TRUE;
         }

         sprintf(buf, "%s - CANCELLED.  Reserve price not matched.", auction_item->short_descr);
         if (good_seller)
         {
            obj_to_char(auction_item, auction_owner);
         }
         else
            extract_obj(auction_item);
      }
      else
      {

         for (ach = first_char; ach != NULL; ach = ach->next)
         {
            if (auction_owner == ach)
               good_seller = TRUE;
            if (auction_bidder == ach)
               good_buyer = TRUE;
         }

         if (good_buyer)
         {
            sprintf(buf, "%s - SOLD! to %s.", auction_item->short_descr, auction_bidder->name);

            obj_to_char(auction_item, auction_bidder);
         }
         else
         {
            sprintf(buf, "%s - SOLD!, but the buyer has left us.  Oh Well!!!",
                    auction_item->short_descr);
            extract_obj(auction_item);
         }
      }

      auction_stage = 0;
      auction_bidder = NULL;
      auction_owner = NULL;
      auction_item = NULL;
      auction_reserve = 0;
      auction_bid = 0;
      break;
   }
   auction(buf);
   auction_stage++;
   return;
}

void remember_attack(CHAR_DATA *ch, CHAR_DATA *victim)
{
   /*
    * Called when an NPC ch encounters a PC victim, that tried to
    * * kill it previously.
    * * --Stephen
    */

   char buf[MAX_STRING_LENGTH];

   /*
    * Pick a random response for ch to give, before attacking
    */

   switch (number_range(0, 7))
   {
   case 0:
      sprintf(buf, "%s returns!  I shall have my revenge at last!", victim->name);
      do_yell(ch, buf);
      break;
   case 1:
      sprintf(buf, "%s You should never have returned.  Ye shall DIE!", victim->name);
      do_whisper(ch, buf);
      break;
   case 2:
      act("$n looks at $N, remembering $S attack", ch, NULL, victim, TO_ROOM);
      act("$n looks at you, remembering your attack", ch, NULL, victim, TO_VICT);
      act("You look at $N, remembering $S attack.", ch, NULL, victim, TO_CHAR);
      do_say(ch, "I SHALL HAVE MY REVENGE!!!");
      break;
   case 3:
      sprintf(buf, "%s has wronged me, and now I will seek my revenge!", victim->name);
      do_gossip(ch, buf);
      sprintf(buf, "Prepare to die, %s.", victim->name);
      do_say(ch, buf);
      break;
   case 4:
      sprintf(buf, "So, %s.  You have returned.  Let us finish our fight this time!", victim->name);
      do_say(ch, buf);
      break;
   case 5:
      sprintf(buf, "Only cowards flee from me, %s!", victim->name);
      do_say(ch, buf);
      break;
   case 6:
      act("$n looks at $N, and recognizes $M!!", ch, NULL, victim, TO_ROOM);
      act("$n looks at you, and recognizes you!!", ch, NULL, victim, TO_VICT);
      act("You look at $N, and recognize $M!", ch, NULL, victim, TO_CHAR);
      sprintf(buf, "There can only be one winner, %s.", victim->name);
      do_say(ch, buf);
      break;
   }

   /*
    * Check if has intelligence, and call correct attack?
    */

   multi_hit(ch, victim, TYPE_UNDEFINED);
   /*
    * spec- plug leak here
    */
   if (ch->target)
   {
      free_string(ch->target);
      ch->target = NULL;
   }
   return;
}

void quest_update()
{
   /* While AI generation is pending, poll the child and defer all other logic */
   if (quest_ai_pending)
   {
      crusade_ai_poll();
      return;
   }

   if (!quest && !auto_quest)
      return;

   if (quest)
   {
      /*
       * Make sure the mobile and obj still exist!
       */
      if (quest_mob == NULL || quest_object == NULL)
      {
         crusade_cancel();
         return;
      }

      crusade_inform();
      if (quest_timer > 15)
         crusade_cancel();
      return;
   }

   if (!quest)
   {
      if (quest_wait > 0)
      {
         quest_wait--;
         return;
      }

      if (auto_quest)
         generate_auto_crusade();
   }
}

#endif
