#include "globals.h"
#include "special.h"
#include "weapon_bond.h"

/* Autogen */
OBJ_DATA *generate_item(int level);
bool create_loot(CHAR_DATA *ch, OBJ_DATA *obj);

bool are_clans_hostile(int victim_clan, int killer_clan, const POL_DATA *politics)
{
   return victim_clan > 0 && victim_clan < MAX_CLAN && killer_clan > 0 && killer_clan < MAX_CLAN &&
          politics->diplomacy[victim_clan][killer_clan] < -450;
}

/*
 * Make a corpse out of a character.
 */
void make_corpse(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *corpse;
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   OBJ_DATA *autogen;
   CHAR_DATA *target = NULL;
   CHAR_DATA *wch;
   char *name;
   bool leave_corpse = FALSE;
   bool has_hostile_clan_killer = FALSE;
   /*
    * int counter, num;
    */
   one_argument(argument, arg);

   if (deathmatch)
      return;

   if (IS_NPC(ch))
   {
      if ((ch->in_room != NULL) && IS_SET(ch->in_room->affected_by, ROOM_BV_SOUL_NET))
      {
         ROOM_INDEX_DATA *room;
         ROOM_AFFECT_DATA *raf;
         ROOM_AFFECT_DATA *raf_next;
         corpse = create_object(get_obj_index(OBJ_VNUM_CAPTURED_SOUL), ch->level);
         corpse->level = ch->level;
         obj_to_room(corpse, ch->in_room);
         OREF(obj_next, OBJ_NEXTCONTENT);
         for (obj = ch->first_carry; obj != NULL; obj = obj_next)
         {
            obj_next = obj->next_in_carry_list;
            obj_from_char(obj);
            extract_obj(obj);
         }
         obj_unreference(&obj_next);
         act("@@eAs $n's soul attempts to fade from the room, the @@dSoul Net@@e quickly "
             "collapses, entombing the soul into a small figurine!!",
             ch, NULL, NULL, TO_ROOM);
         room = ch->in_room;
         for (raf = room->first_room_affect; raf != NULL; raf = raf_next)
         {
            raf_next = raf->next;
            if (raf->bitvector == ROOM_BV_SOUL_NET)
            {
               r_affect_remove(room, raf);
            }
         }
         return;
      }
      else /* still NPC, no soul net */
      {
         int gold;
         time_t lifetime;

         name = ch->short_descr;
         corpse = create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
         corpse->timer = number_range(3, 6);
         corpse->level = ch->level; /* for animate/revenant spell */
         if (arg[0] != '\0')
         {
            free_string(corpse->owner);
            corpse->owner = str_dup(arg);
         }
         if (ch->pIndexData != NULL)
            corpse->value[4] = ch->pIndexData->vnum; /* original mob vnum for revenant */
         /*
          * Takes a mob 2 rl hours to gain full gold.
          */
         lifetime = current_time - (ch->logon);
         gold = 5 * (ch->level) * (UMIN(100, lifetime * 100 / (2 * 3600))) / 100;
         /*
          * Then take from 1/5 of maximum (i.e. level) to maximum gold.
          */
         gold = number_range(gold / 5, gold);
         /*
          * Add special gold.
          */
         gold += ch->gold;

         if (gold > 0)
         {
            // corpse money goes here
         }
         ch->gold = 0;
      }
   }
   else /* player */
   {
      name = ch->name;
      corpse = create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
      corpse->timer = number_range(20, 30);

      sprintf(buf, "%s", ch->name);
      free_string(corpse->owner);
      corpse->owner = str_dup(buf);

      if ((arg[0] != '\0') && (!IS_NPC(ch)))
      {
         target = NULL;
         for (wch = first_char; wch != NULL; wch = wch->next)
         {
            if (!IS_NPC(wch) && is_name(arg, wch->name))
            {
               target = wch;
               break;
            }
         }
         if ((target != NULL) && !IS_NPC(target))
         {
            bool clans_in_war =
                are_clans_hostile(ch->pcdata->clan, target->pcdata->clan, &politics_data);

            if (IS_SET(ch->pcdata->pflags, PFLAG_PKOK))
               corpse->value[0] = 1;
            if (clans_in_war)
            {
               corpse->value[2] = target->pcdata->clan;
               has_hostile_clan_killer = TRUE;
            }
            else
               corpse->value[2] = -1;
            corpse->value[3] = number_range(2, 5);
         }
      }
   } /* end of player only */

   if ((IS_SET(ch->act, PLR_KILLER) || IS_SET(ch->act, PLR_THIEF)) &&
       ((target != NULL) &&
        ((!IS_NPC(target)) || (!str_cmp(rev_spec_lookup(target->spec_fun), "spec_executioner")))))

   {
      corpse->value[0] = 1;
      corpse->value[3] = number_range(3, 6);
   }
   sprintf(buf, corpse->name, name);
   free_string(corpse->name);
   corpse->name = str_dup(buf);

   sprintf(buf, corpse->short_descr, name);
   free_string(corpse->short_descr);
   corpse->short_descr = str_dup(buf);

   sprintf(buf, corpse->description, name);
   free_string(corpse->description);
   corpse->description = str_dup(buf);
   OREF(obj_next, OBJ_NEXTCONTENT);
   for (obj = ch->first_carry; obj != NULL; obj = obj_next)
   {
      obj_next = obj->next_in_carry_list;

      /* Bonded weapons stay equipped on the swordsman through death */
      if (IS_OBJ_BONDED(obj))
         continue;

      obj_from_char(obj);
      if ((obj == quest_object) && (ch == quest_target))
      {
         obj->value[0] = UMAX(1, obj->value[0] * quest_timer / 10);
         obj->value[1] = UMAX(1, obj->value[1] * quest_timer / 10);
         obj->value[2] = UMAX(1, obj->value[2] * quest_timer / 10);
      }
      if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
         extract_obj(obj);
      else
         obj_to_obj(obj, corpse);
   }
   obj_unreference(&obj_next);

   if (!IS_AFFECTED(ch, AFF_CHARM) && number_percent() < get_generation_chance())
   {
      autogen = generate_item(ch->level - 10);
      obj_to_obj(autogen, corpse);
   }

   if (ch->loot_amount > 0)
   {
      create_loot(ch, corpse);
   }

   if (!IS_NPC(ch))
   {
      if ((IS_SET(ch->pcdata->pflags, PFLAG_PKOK)) || has_hostile_clan_killer ||
          ((ch->level > 30) && (IS_SET(ch->act, PLR_KILLER) || IS_SET(ch->act, PLR_THIEF))) ||
          (leave_corpse))
         obj_to_room(corpse, ch->in_room);
      else
         obj_to_room(corpse, get_room_index(ROOM_VNUM_MORIBUND));
      {
         CORPSE_DATA *this_corpse;
         GET_FREE(this_corpse, corpse_free);
         this_corpse->next = NULL;
         this_corpse->prev = NULL;
         this_corpse->this_corpse = corpse;
         LINK(this_corpse, first_corpse, last_corpse, next, prev);
      }
      save_corpses();
      return;
   }
   else
   {
      obj_to_room(corpse, ch->in_room);
      return;
   }
}

/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry(CHAR_DATA *ch)
{
   /*
    * FIXME:  *ONLY* PKOK victims get a head loaded.
    */
   /*
    * Make this give 'cry' message, don't load anything
    */
   ROOM_INDEX_DATA *was_in_room;
   char *msg;
   int door;

   if (IS_NPC(ch))
      msg = "You hear something's death cry.";
   else
      msg = "You hear someone's death cry.";

   was_in_room = ch->in_room;
   if ((was_in_room == NULL) || (was_in_room->vnum == 0))
      return;

   for (door = 0; door <= 5; door++)
   {
      EXIT_DATA *pexit;

      if ((pexit = was_in_room->exit[door]) != NULL && pexit->to_room != NULL &&
          pexit->to_room != was_in_room)
      {
         ch->in_room = pexit->to_room;
         act(msg, ch, NULL, NULL, TO_ROOM);
      }
   }
   ch->in_room = was_in_room;

   return;
}

bool should_extract_npc_on_death(const CHAR_DATA *victim)
{
   return (victim != NULL) && IS_NPC(victim);
}

void raw_kill(CHAR_DATA *victim, char *argument)
{
   CHAR_DATA *check;
   char arg[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];

   one_argument(argument, arg);

   stop_fighting(victim, TRUE);
   if (victim == quest_target)
      quest_target = NULL;
   if (victim == quest_mob)
   {
      sprintf(buf, "Oh well, I guess the quest is over, since I am about to @@eDIE!!!!!@@N");
      do_crusade(victim, buf);

      quest_mob = NULL;
   }

   if (victim->is_free == FALSE && victim->in_room != NULL)
      make_corpse(victim, arg);

   for (check = first_char; check != NULL; check = check->next)
   {
      if (check->hunting == victim)
         end_hunt(check);
      /*        unhunt(check);*/
   }

   if (should_extract_npc_on_death(victim))
   {
      extract_char(victim, TRUE);
      return;
   }

   extract_char(victim, FALSE);
   while (victim->first_affect)
      affect_remove(victim, victim->first_affect);
   victim->affected_by = 0;
   victim->armor = 100;
   victim->overgrowth = 0;
   victim->position = POS_RESTING;
   victim->hit = UMAX(1, victim->hit);
   victim->mana = UMAX(1, victim->mana);
   victim->move = UMAX(1, victim->move);
   save_char_obj(victim);
}

void group_gain(CHAR_DATA *ch, CHAR_DATA *victim)
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *gch;
   int members;
   int huggy; /* To work out exp gained */
   int funky; /* Hope you LOVE these var names, Mag */
   int base;
   /*
    * Monsters don't get kill xp's or alignment changes.
    * P-killing doesn't help either.
    * Dying of mortal wounds or poison doesn't give xp to anyone!
    * -S- Mod: INTELLIGENT mobs *can* gain exp.  So there!
    */

   if ((IS_NPC(ch)) || (!IS_NPC(victim)) || (victim == ch))
      return;

   members = 0;
   huggy = 0;

   if (!IS_NPC(ch))
      quest_kill_notify(ch, victim);

   invasion_on_death(victim, ch);
   spec_death_handler(victim, ch);

   base = victim->exp; /* Now share this out... */

   for (gch = ch->in_room->first_person; gch != NULL; gch = gch->next_in_room)
   {
      if (is_same_group(gch, ch))
      {
         members++;
         huggy += UMAX(get_psuedo_level(gch), (get_psuedo_level(ch) - 25));
      }
   }

   if (members == 0)
   {
      bug("Group_gain: members.", members);
      members = 1;
      huggy = get_psuedo_level(ch);
   }

   /*
    * Bonus for grouping
    */

   if (members < 2)
      ; /* Changed from funky */
   else if (members < 3)
      base *= (5 / 2); /* funky wasn't initialised */
   else if (members < 4)
      base *= 3; /* anyways.. hmm MAG */
   else if (members < 5)
      base *= (7 / 2);
   else if (members < 6)
      base *= 4;
   else
      base *= (9 / 2);

   /*
    * Question is, if someone's exp is capped, do you dole out the rest
    * to the other people? or just reduce the total amount given to the
    * group (by not doling it out).
    * As it is, the total exp is reduced. I'll leave it that way.
    *
    */

   for (gch = ch->in_room->first_person; gch != NULL; gch = gch->next_in_room)
   {
      OBJ_DATA *obj;
      OBJ_DATA *obj_next;
      int align;
      int max_xp = 5000000;

      if (happy_hour)
      {
         send_to_char("Happy hour is active! Xp doubled!\n\r", gch);
         max_xp *= 2;
      }

      if (!is_same_group(gch, ch))
         continue;

      /*
       * Calc each char's xp seperately, but mult by ch->lev/tot_group_lev.
       */

      funky = (base * get_psuedo_level(gch)) / huggy; /* gch's % of exp gained */

      /*
       * Capping changed.  -S-
       */
      /*
       * Changed YET again -S-
       */
      /*
       * funky = UMIN( funky, ( 1.5 * exp_table[gch->level].mob_base ) );
       * funky = UMAX( 0, funky );
       */

      /*
       * Now the max is just 250K
       */
      if (funky < 0)
         funky = 823421;

      funky *= number_range(150, 250);
      funky /= 100;

      funky = UMIN(funky, max_xp);

      if (happy_hour)
         funky *= 2;

      if ((abs((get_psuedo_level(gch) - get_psuedo_level(victim))) > 21) ||
          (get_psuedo_level(gch) > (get_psuedo_level(victim) + 15)))
      {
         funky = 0;
      }

      if ((abs((get_psuedo_level(ch) - get_psuedo_level(victim))) > 21) ||
          (get_psuedo_level(ch) > (get_psuedo_level(victim) + 15)))
      {
         funky = 0;
      }

      if (is_adept(gch))
         funky /= 1000;

      sprintf(buf, "You Receive %d Experience Points.\n\r", funky);
      send_to_char(buf, gch);
      gain_exp(gch, funky);

      if (!IS_NPC(gch) && !can_use_skill(gch, gsn_emotion_control))
      {
         align = gch->alignment -
                 (victim->alignment * (80 - gch->pcdata->learned[gsn_emotion_control]) / 100);

         if (align > 500)
            gch->alignment = UMIN(gch->alignment + (align - 500) / 4, 1000);
         else if (align < -500)
            gch->alignment = UMAX(gch->alignment + (align + 500) / 4, -1000);
         else
            gch->alignment -= gch->alignment / 4;
      }
      for (obj = ch->first_carry; obj != NULL; obj = obj_next)
      {
         obj_next = obj->next_in_carry_list;
         if (obj->wear_loc == WEAR_NONE)
            continue;

         if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) ||
             (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)))
         {
            act("You feel $p slither out of your grasp, and back into your inventory!", ch, obj,
                NULL, TO_CHAR);
            act("$p slithers out of $n's hands and back into $s inventory!", ch, obj, NULL,
                TO_ROOM);
            unequip_char(ch, obj);
            return;
         }
      }
   }

   return;
}
