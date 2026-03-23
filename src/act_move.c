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
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "globals.h"
#include "socket.h"
#define NOWHERE -1

char *const compass_name[] = {"north", "east", "south", "west", "up", "down"};

char *const dir_name[] = {"to the North", "to the East", "to the South",
                          "to the West",  "Up",          "Down"};

/* rev_name used to indicate where ch has come from when entering -S- */

char *const rev_name[] = {"the South", "the West", "the North", "the East", "Below", "Above"};

const sh_int rev_dir[] = {2, 3, 0, 1, 5, 4};

const sh_int movement_loss[SECT_MAX] = {1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6, 1};

/*
 * Local functions.
 */
int find_door args((CHAR_DATA * ch, char *arg));
OBJ_DATA *has_key args((CHAR_DATA * ch, int key));
/*
struct fol_data
{
  struct fol_data *next;
  CHAR_DATA *ch;
};

struct fol_data *fol_stack;
struct fol_data *fol_free;

*/

void move_char(CHAR_DATA *ch, int door)
{
   CHAR_DATA *fch;
   CHAR_DATA *fch_next;
   ROOM_INDEX_DATA *in_room;
   ROOM_INDEX_DATA *to_room;
   EXIT_DATA *pexit;
   char buf[MAX_STRING_LENGTH];
   char move_buf[MAX_STRING_LENGTH];
   /*    struct fol_data *fol = NULL;   */
   char door_name_leave[MSL];
   char door_name_enter[MSL];

   buf[0] = '\0';
   move_buf[0] = '\0';
   door_name_enter[0] = '\0';
   door_name_leave[0] = '\0';

   if (door < 0 || door > 5)
   {
      bug("Do_move: bad door %d.", door);
      ch->using_named_door = FALSE;
      return;
   }
   if (!IS_NPC(ch) && ch->stance > 0 && (IS_SET(stance_app[ch->stance].specials, STANCE_NINJA)))
   {
      send_to_char("You step out of the shadows.\n\r", ch);
      ch->stance = 0;
      act("$n steps out of the Shadows!", ch, NULL, NULL, TO_ROOM);
   }
   if (IS_NPC(ch))
      ch->using_named_door = TRUE;
   in_room = ch->in_room;
   if ((pexit = in_room->exit[door]) == NULL || (to_room = pexit->to_room) == NULL ||
       ((str_cmp(pexit->keyword, "")) && (ch->using_named_door == FALSE)))
   {
      send_to_char("Alas, you cannot go that way.\n\r", ch);
      ch->using_named_door = FALSE;
      return;
   }
   if (str_cmp(pexit->keyword, ""))
   {
      if (pexit->keyword[0] == '^')
      {
         sprintf(door_name_leave, "through the %s", pexit->keyword + 1);
         sprintf(door_name_enter, "the %s", pexit->keyword + 1);
      }
      else
      {
         sprintf(door_name_leave, "through the %s", pexit->keyword);
         sprintf(door_name_enter, "the %s", pexit->keyword);
      }
   }
   else
   {
      sprintf(door_name_leave, "%s", dir_name[door]);
      sprintf(door_name_enter, "%s", rev_name[door]);
   }

   if (IS_SET(in_room->affected_by, ROOM_BV_ENCAPS) || IS_SET(to_room->affected_by, ROOM_BV_ENCAPS))
   {
      send_to_char("A barely visible energy web stops your movement!\n\r", ch);
      ch->using_named_door = FALSE;
      return;
   }
   if (IS_SET(in_room->affected_by, ROOM_BV_HOLD))
   {
      send_to_char(
          "A fleeting vision of bars appears before the exit, and stops your movement!\n\r", ch);
      ch->using_named_door = FALSE;
      return;
   }

   if (IS_SET(pexit->exit_info, EX_CLOSED))
   {
      if (!IS_AFFECTED(ch, AFF_PASS_DOOR) && (!item_has_apply(ch, ITEM_APPLY_PASS_DOOR)))
      {
         if (IS_SET(pexit->exit_info, EX_NODETECT))
            send_to_char("Alas, you cannot go that way.\n\r", ch);
         else
            act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
         ch->using_named_door = FALSE;
         return;
      }
      else if (IS_SET(pexit->exit_info, EX_PASSPROOF))
      {
         if (IS_SET(pexit->exit_info, EX_NODETECT))
            send_to_char("Alas, you cannot go that way.\n\r", ch);
         else
            act("You don't seem to be able to pass through $d.", ch, NULL, pexit->keyword, TO_CHAR);
         ch->using_named_door = FALSE;
         return;
      }
   }

   if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL && in_room == ch->master->in_room)
   {
      send_to_char("What?  And leave your beloved master?\n\r", ch);
      ch->using_named_door = FALSE;
      return;
   }

   if (room_is_private(to_room) && !IS_STAFF(ch))
   {
      send_to_char("That room is private right now.\n\r", ch);
      ch->using_named_door = FALSE;
      return;
   }
   if (IS_SET(pexit->exit_info, EX_STAFF) && !IS_STAFF(ch))
   {
      send_to_char("Only staff may use that exit.\n\r", ch);
      ch->using_named_door = FALSE;
      return;
   }

   /*
    * Int Mobs need to be able to handle climbs
    */
   if (IS_SET(pexit->exit_info, EX_CLIMB) && IS_NPC(ch) && !IS_AFFECTED(ch, AFF_FLYING))
   {
      if (ch->mana > mana_cost(ch, skill_lookup("fly")))
      {
         do_cast(ch, "fly");
      }
      else
      {
         send_to_char("Looks like you'd have to climb up there.\n\r", ch);
         ch->using_named_door = FALSE;
         return;
      }
   }

   if (!IS_NPC(ch) && ch->pcdata->learned[gsn_climb] == 0 && IS_SET(pexit->exit_info, EX_CLIMB) &&
       !IS_AFFECTED(ch, AFF_FLYING) && !item_has_apply(ch, ITEM_APPLY_FLY))
   {
      send_to_char("You'd need to be able to fly or climb to go that way!\n\r", ch);
      ch->using_named_door = FALSE;
      return;
   }

   {
      int iClan;
      int move;

      for (iClan = 0; iClan < MAX_CLAN; iClan++)
      {
         if (to_room->vnum == clan_table[iClan].clan_room)
         {
            if ((IS_NPC(ch)) || ((!IS_STAFF(ch)) && (iClan != ch->pcdata->clan)))
            {
               send_to_char("You aren't allowed in there.\n\r", ch);
               ch->using_named_door = FALSE;
               return;
            }
         }
      }

      if (to_room->vnum == ROOM_VNUM_BUILDER && (!IS_STAFF(ch) && !IS_SET(ch->act, PLR_BUILDER)))
      {
         send_to_char("The Portal allows entrance to builders only.\n\r", ch);
         ch->using_named_door = FALSE;
         return;
      }

      if (to_room->vnum == ROOM_VNUM_CLAN &&
          (IS_NPC(ch) || ((!IS_STAFF(ch)) && (!IS_SET(ch->pcdata->pflags, PFLAG_CLAN_BOSS)))))
      {
         send_to_char("Only Clan Bosses may enter this room.\n\r", ch);
         ch->using_named_door = FALSE;
         return;
      }

      if (in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR)
      {
         if ((!IS_AFFECTED(ch, AFF_FLYING)) && (!item_has_apply(ch, ITEM_APPLY_FLY)))
         {

            if ((IS_NPC(ch)) && (ch->mana > mana_cost(ch, skill_lookup("fly"))))
            {
               do_cast(ch, "fly");
            }
            else
            {
               send_to_char("You can't fly.\n\r", ch);
               ch->using_named_door = FALSE;
               return;
            }
         }
      }

      if (in_room->sector_type == SECT_WATER_NOSWIM || to_room->sector_type == SECT_WATER_NOSWIM)
      {
         OBJ_DATA *obj;
         bool found;

         /*
          * Look for a boat.
          */
         found = FALSE;

         /*
          * Suggestion for flying above water by Sludge
          */
         if ((IS_AFFECTED(ch, AFF_FLYING)) || (item_has_apply(ch, ITEM_APPLY_FLY)))
            found = TRUE;
         for (obj = ch->first_carry; obj != NULL; obj = obj->next_in_carry_list)
         {
            if (obj->item_type == ITEM_BOAT)
            {
               found = TRUE;
               break;
            }
         }
         if (!found)
         {
            if ((IS_NPC(ch)) && (ch->mana > mana_cost(ch, skill_lookup("fly"))))
            {
               do_cast(ch, "fly");
            }
            else if (!IS_STAFF(ch))
            {
               send_to_char("You need a boat to go there.\n\r", ch);
               ch->using_named_door = FALSE;
               return;
            }
         }
      }

      move = movement_loss[UMIN(SECT_MAX - 1, in_room->sector_type)] +
             movement_loss[UMIN(SECT_MAX - 1, to_room->sector_type)];

      if (IS_AFFECTED(ch, AFF_FLYING) || item_has_apply(ch, ITEM_APPLY_FLY))
         move = 1;

      if (ch->move < move)
      {
         send_to_char("You are too exhausted.\n\r", ch);
         ch->using_named_door = FALSE;
         return;
      }

      WAIT_STATE(ch, 1);
      ch->move -= move;
   }
   /*    if ( ( !IS_AFFECTED(ch, AFF_SNEAK) && !item_has_apply( ch, ITEM_APPLY_SNEAK ) )
       && ( IS_NPC(ch) || !IS_SET(ch->act, PLR_WIZINVIS) ) )  */
   {
      if (IS_SET(pexit->exit_info, EX_CLIMB)) /* dir_name[door] */
         act("$n climbs $T.", ch, NULL, door_name_leave, TO_ROOM);
      else if (IS_SET(pexit->exit_info, EX_CLOSED)) /* using passdoor */
      {
         snprintf(buf, sizeof(buf), "$L$n floats %.64s.", door_name_leave);
         act(buf, ch, NULL, pexit->keyword, TO_ROOM);
      }
      else
      {
         if (IS_NPC(ch))
            snprintf(move_buf, sizeof(move_buf), "$L$n %s $T.", "wanders");
         else
         {
            snprintf(move_buf, sizeof(move_buf), "$L%s$n %s $T.",
                     get_ruler_title(ch->pcdata->ruler_rank, ch->login_sex), ch->pcdata->room_exit);
         }
         act(move_buf, ch, NULL, door_name_leave, TO_ROOM);
      }
   }
   char_from_room(ch);
   char_to_room(ch, to_room);
   /*
    * if ( ( !IS_AFFECTED(ch, AFF_SNEAK) && !item_has_apply( ch, ITEM_APPLY_SNEAK ) )
    * && ( IS_NPC(ch) || !IS_SET(ch->act, PLR_WIZINVIS) ) )
    */
   {
      if (IS_NPC(ch))
         sprintf(move_buf, "$L$n %s $T.", "wanders in from");
      else
      {
         sprintf(move_buf, "$L%s$n %s $T.", get_ruler_title(ch->pcdata->ruler_rank, ch->login_sex),
                 ch->pcdata->room_enter);
      }
      act(move_buf, ch, NULL, door_name_enter, TO_ROOM);
   }

   if (in_room != to_room)
   {
      for (fch = in_room->first_person; fch != NULL; fch = fch_next)
      {
         fch_next = fch->next_in_room;
         if (fch->master == ch && fch->position == POS_STANDING)
         {
            act("You follow $N $t.", fch, dir_name[door], ch, TO_CHAR);
            fch->using_named_door = TRUE;
            move_char(fch, door);
         }
      }
   }
   do_look(ch, "auto");

   if (!IS_NPC(ch))
      quest_room_notify(ch, ch->in_room);

   ch->using_named_door = FALSE;
   return;
}

void do_north(CHAR_DATA *ch, char *argument)
{
   move_char(ch, DIR_NORTH);
   return;
}

void do_east(CHAR_DATA *ch, char *argument)
{
   move_char(ch, DIR_EAST);
   return;
}

void do_south(CHAR_DATA *ch, char *argument)
{
   move_char(ch, DIR_SOUTH);
   return;
}

void do_west(CHAR_DATA *ch, char *argument)
{
   move_char(ch, DIR_WEST);
   return;
}

void do_up(CHAR_DATA *ch, char *argument)
{
   move_char(ch, DIR_UP);
   return;
}

void do_down(CHAR_DATA *ch, char *argument)
{
   move_char(ch, DIR_DOWN);
   return;
}

int find_door(CHAR_DATA *ch, char *arg)
{
   EXIT_DATA *pexit;
   int door;

   if (!str_cmp(arg, "n") || !str_cmp(arg, "north"))
      door = 0;
   else if (!str_cmp(arg, "e") || !str_cmp(arg, "east"))
      door = 1;
   else if (!str_cmp(arg, "s") || !str_cmp(arg, "south"))
      door = 2;
   else if (!str_cmp(arg, "w") || !str_cmp(arg, "west"))
      door = 3;
   else if (!str_cmp(arg, "u") || !str_cmp(arg, "up"))
      door = 4;
   else if (!str_cmp(arg, "d") || !str_cmp(arg, "down"))
      door = 5;
   else
   {
      for (door = 0; door <= 5; door++)
      {
         if ((pexit = ch->in_room->exit[door]) != NULL && IS_SET(pexit->exit_info, EX_ISDOOR) &&
             pexit->keyword != NULL && is_name(arg, pexit->keyword))
            return door;
      }
      act("I see no $T here.", ch, NULL, arg, TO_CHAR);
      return -1;
   }

   if (((pexit = ch->in_room->exit[door]) == NULL) ||
       (str_cmp(pexit->keyword, "") && !is_name(arg, pexit->keyword)))
   {
      act("I see no door $T here.", ch, NULL, arg, TO_CHAR);
      return -1;
   }

   if (!IS_SET(pexit->exit_info, EX_ISDOOR))
   {
      send_to_char("You can't do that.\n\r", ch);
      return -1;
   }

   return door;
}

void do_open(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int door;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Open what?\n\r", ch);
      return;
   }

   if ((obj = get_obj_here(ch, arg)) != NULL)
   {
      /*
       * 'open object'
       */
      if (obj->item_type != ITEM_CONTAINER)
      {
         send_to_char("That's not a container.\n\r", ch);
         return;
      }
      if (!IS_SET(obj->value[1], CONT_CLOSED))
      {
         send_to_char("It's already open.\n\r", ch);
         return;
      }
      if (!IS_SET(obj->value[1], CONT_CLOSEABLE))
      {
         send_to_char("You can't do that.\n\r", ch);
         return;
      }
      if (IS_SET(obj->value[1], CONT_LOCKED))
      {
         send_to_char("It's locked.\n\r", ch);
         return;
      }

      REMOVE_BIT(obj->value[1], CONT_CLOSED);
      send_to_char("Ok.\n\r", ch);
      act("$n opens $p.", ch, obj, NULL, TO_ROOM);
      return;
   }

   if ((door = find_door(ch, arg)) >= 0)
   {
      /*
       * 'open door'
       */
      ROOM_INDEX_DATA *to_room;
      EXIT_DATA *pexit;
      EXIT_DATA *pexit_rev;

      pexit = ch->in_room->exit[door];
      if (!IS_SET(pexit->exit_info, EX_CLOSED))
      {
         send_to_char("It's already open.\n\r", ch);
         return;
      }
      if (IS_SET(pexit->exit_info, EX_LOCKED))
      {
         send_to_char("It's locked.\n\r", ch);
         return;
      }

      REMOVE_BIT(pexit->exit_info, EX_CLOSED);
      act("$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM);
      send_to_char("Ok.\n\r", ch);

      /*
       * open the other side
       */
      if ((to_room = pexit->to_room) != NULL &&
          (pexit_rev = to_room->exit[rev_dir[door]]) != NULL && pexit_rev->to_room == ch->in_room)
      {
         CHAR_DATA *rch;

         REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
         for (rch = to_room->first_person; rch != NULL; rch = rch->next_in_room)
            act("The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR);
      }
   }

   return;
}

void do_close(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int door;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Close what?\n\r", ch);
      return;
   }

   if ((obj = get_obj_here(ch, arg)) != NULL)
   {
      /*
       * 'close object'
       */
      if (obj->item_type != ITEM_CONTAINER)
      {
         send_to_char("That's not a container.\n\r", ch);
         return;
      }
      if (IS_SET(obj->value[1], CONT_CLOSED))
      {
         send_to_char("It's already closed.\n\r", ch);
         return;
      }
      if (!IS_SET(obj->value[1], CONT_CLOSEABLE))
      {
         send_to_char("You can't do that.\n\r", ch);
         return;
      }

      SET_BIT(obj->value[1], CONT_CLOSED);
      send_to_char("Ok.\n\r", ch);
      act("$n closes $p.", ch, obj, NULL, TO_ROOM);
      return;
   }

   if ((door = find_door(ch, arg)) >= 0)
   {
      /*
       * 'close door'
       */
      ROOM_INDEX_DATA *to_room;
      EXIT_DATA *pexit;
      EXIT_DATA *pexit_rev;

      pexit = ch->in_room->exit[door];
      if (IS_SET(pexit->exit_info, EX_CLOSED))
      {
         send_to_char("It's already closed.\n\r", ch);
         return;
      }

      SET_BIT(pexit->exit_info, EX_CLOSED);
      act("$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM);
      send_to_char("Ok.\n\r", ch);

      /*
       * close the other side
       */
      if ((to_room = pexit->to_room) != NULL && (pexit_rev = to_room->exit[rev_dir[door]]) != 0 &&
          pexit_rev->to_room == ch->in_room)
      {
         CHAR_DATA *rch;

         SET_BIT(pexit_rev->exit_info, EX_CLOSED);
         for (rch = to_room->first_person; rch != NULL; rch = rch->next_in_room)
            act("The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR);
      }
   }

   return;
}

OBJ_DATA *has_key(CHAR_DATA *ch, int key)
{
   OBJ_DATA *obj;

   for (obj = ch->first_carry; obj != NULL; obj = obj->next_in_carry_list)
   {
      if (obj->pIndexData->vnum == key)
      {
         return obj;
      }
   }

   return NULL;
}

void do_lock(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int door;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Lock what?\n\r", ch);
      return;
   }

   if ((obj = get_obj_here(ch, arg)) != NULL)
   {
      /*
       * 'lock object'
       */
      if (obj->item_type != ITEM_CONTAINER)
      {
         send_to_char("That's not a container.\n\r", ch);
         return;
      }
      if (!IS_SET(obj->value[1], CONT_CLOSED))
      {
         send_to_char("It's not closed.\n\r", ch);
         return;
      }
      if (obj->value[2] < 0)
      {
         send_to_char("It can't be locked.\n\r", ch);
         return;
      }
      if (has_key(ch, obj->value[2]) == NULL)
      {
         send_to_char("You lack the key.\n\r", ch);
         return;
      }
      if (IS_SET(obj->value[1], CONT_LOCKED))
      {
         send_to_char("It's already locked.\n\r", ch);
         return;
      }

      SET_BIT(obj->value[1], CONT_LOCKED);
      send_to_char("*Click*\n\r", ch);
      act("$n locks $p.", ch, obj, NULL, TO_ROOM);
      return;
   }

   if ((door = find_door(ch, arg)) >= 0)
   {
      /*
       * 'lock door'
       */
      ROOM_INDEX_DATA *to_room;
      EXIT_DATA *pexit;
      EXIT_DATA *pexit_rev;

      pexit = ch->in_room->exit[door];
      if (!IS_SET(pexit->exit_info, EX_CLOSED))
      {
         send_to_char("It's not closed.\n\r", ch);
         return;
      }
      if (pexit->key < 0)
      {
         send_to_char("It can't be locked.\n\r", ch);
         return;
      }
      if (has_key(ch, pexit->key) == NULL)
      {
         send_to_char("You lack the key.\n\r", ch);
         return;
      }
      if (IS_SET(pexit->exit_info, EX_LOCKED))
      {
         send_to_char("It's already locked.\n\r", ch);
         return;
      }

      SET_BIT(pexit->exit_info, EX_LOCKED);
      send_to_char("*Click*\n\r", ch);
      act("$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

      /*
       * lock the other side
       */
      if ((to_room = pexit->to_room) != NULL && (pexit_rev = to_room->exit[rev_dir[door]]) != 0 &&
          pexit_rev->to_room == ch->in_room)
      {
         SET_BIT(pexit_rev->exit_info, EX_LOCKED);
      }
   }

   return;
}

void do_unlock(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj, *key;
   int door;

   one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Unlock what?\n\r", ch);
      return;
   }

   if ((obj = get_obj_here(ch, arg)) != NULL)
   {
      /*
       * 'unlock object'
       */
      if (obj->item_type != ITEM_CONTAINER)
      {
         send_to_char("That's not a container.\n\r", ch);
         return;
      }
      if (!IS_SET(obj->value[1], CONT_CLOSED))
      {
         send_to_char("It's not closed.\n\r", ch);
         return;
      }
      if (obj->value[2] < 0)
      {
         send_to_char("It can't be unlocked.\n\r", ch);
         return;
      }
      if (!IS_SET(obj->value[1], CONT_LOCKED))
      {
         send_to_char("It's already unlocked.\n\r", ch);
         return;
      }

      key = has_key(ch, obj->value[2]);

      if (key == NULL)
      {
         send_to_char("You lack the key.\n\r", ch);
         return;
      }

      REMOVE_BIT(obj->value[1], CONT_LOCKED);
      send_to_char("*Click*\n\r", ch);
      act("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
      if (IS_SET(key->extra_flags, ITEM_TRIG_DESTROY))
      {
         act("$p was consumed on use.", ch, key, NULL, TO_ROOM);
         act("You consumed $p on use.", ch, key, NULL, TO_CHAR);
         extract_obj(key);
      }
      return;
   }

   if ((door = find_door(ch, arg)) >= 0)
   {
      /*
       * 'unlock door'
       */
      ROOM_INDEX_DATA *to_room;
      EXIT_DATA *pexit;
      EXIT_DATA *pexit_rev;

      pexit = ch->in_room->exit[door];
      if (!IS_SET(pexit->exit_info, EX_CLOSED))
      {
         send_to_char("It's not closed.\n\r", ch);
         return;
      }
      if (pexit->key < 0)
      {
         send_to_char("It can't be unlocked.\n\r", ch);
         return;
      }
      key = has_key(ch, pexit->key);
      if (key == NULL)
      {
         send_to_char("You lack the key.\n\r", ch);
         return;
      }
      if (!IS_SET(pexit->exit_info, EX_LOCKED))
      {
         send_to_char("It's already unlocked.\n\r", ch);
         return;
      }

      REMOVE_BIT(pexit->exit_info, EX_LOCKED);
      send_to_char("*Click*\n\r", ch);
      act("$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM);
      if (IS_SET(key->extra_flags, ITEM_TRIG_DESTROY))
      {
         act("$p was consumed on use.", ch, key, NULL, TO_ROOM);
         act("You consumed $p on use.", ch, key, NULL, TO_CHAR);
         extract_obj(key);
      }

      /*
       * unlock the other side
       */
      if ((to_room = pexit->to_room) != NULL &&
          (pexit_rev = to_room->exit[rev_dir[door]]) != NULL && pexit_rev->to_room == ch->in_room)
      {
         REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
      }
   }

   return;
}

void do_stand(CHAR_DATA *ch, char *argument)
{
   switch (ch->position)
   {
   case POS_SLEEPING:
      if (IS_AFFECTED(ch, AFF_SLEEP))
      {
         send_to_char("You can't wake up!\n\r", ch);
         return;
      }

      send_to_char("You wake and stand up.\n\r", ch);
      act("$n wakes and stands up.", ch, NULL, NULL, TO_ROOM);
      ch->position = POS_STANDING;
      break;

   case POS_RESTING:
      send_to_char("You stand up.\n\r", ch);
      act("$n stands up.", ch, NULL, NULL, TO_ROOM);
      if (ch->sitting != NULL)
      {
         ch->sitting->value[1]--;
         ch->sitting = NULL;
      }
      ch->position = POS_STANDING;
      break;

   case POS_STANDING:
      send_to_char("You are already standing.\n\r", ch);
      break;

   case POS_FIGHTING:
      send_to_char("You are already fighting!\n\r", ch);
      break;
   }

   return;
}

void do_rest(CHAR_DATA *ch, char *argument)
{

   OBJ_DATA *furn = NULL;
   bool sit;

   sit = FALSE;

   if (argument[0] != '\0')
   {
      furn = get_obj_here(ch, argument);
      if (furn == NULL || furn->item_type != ITEM_FURNITURE)
      {
         send_to_char("You can't rest on that!\n\r", ch);
         return;
      }
      if (furn->value[1] >= furn->value[0])
      {
         send_to_char("It's full already!\n\r", ch);
         return;
      }
      sit = TRUE;
   }
   switch (ch->position)
   {
   case POS_SLEEPING:
      send_to_char("You are already sleeping.\n\r", ch);
      break;

   case POS_RESTING:
      send_to_char("You are already resting.\n\r", ch);
      break;

   case POS_STANDING:
      if (sit == FALSE)
      {
         send_to_char("You rest.\n\r", ch);
         act("$n rests.", ch, NULL, NULL, TO_ROOM);
      }
      else
      {
         ch->sitting = furn;
         act("$n rests on $p", ch, furn, NULL, TO_ROOM);
         act("You rest on $p", ch, furn, NULL, TO_CHAR);
         ch->sitting->value[1]++;
      }
      ch->position = POS_RESTING;
      break;

   case POS_FIGHTING:
      send_to_char("You are already fighting!\n\r", ch);
      break;
   }

   return;
}

void do_sleep(CHAR_DATA *ch, char *argument)
{
   switch (ch->position)
   {
   case POS_SLEEPING:
      send_to_char("You are already sleeping.\n\r", ch);
      break;

   case POS_RESTING:
   case POS_STANDING:
      send_to_char("You sleep.\n\r", ch);
      act("$n sleeps.", ch, NULL, NULL, TO_ROOM);
      ch->position = POS_SLEEPING;
      break;

   case POS_FIGHTING:
      send_to_char("You are already fighting!\n\r", ch);
      break;
   }

   return;
}

void do_wake(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      do_stand(ch, argument);
      return;
   }

   if (!IS_AWAKE(ch))
   {
      send_to_char("You are asleep yourself!\n\r", ch);
      return;
   }

   if ((victim = get_char_room(ch, arg)) == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
      return;
   }

   if (IS_AWAKE(victim))
   {
      act("$N is already awake.", ch, NULL, victim, TO_CHAR);
      return;
   }

   if (IS_AFFECTED(victim, AFF_SLEEP))
   {
      act("You can't wake $M!", ch, NULL, victim, TO_CHAR);
      return;
   }

   act("You wake $M.", ch, NULL, victim, TO_CHAR);
   act("$n wakes you.", ch, NULL, victim, TO_VICT);
   victim->position = POS_STANDING;
   return;
}

void do_shadowform(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   send_to_char("You attempt to take on shadow form.\n\r", ch);
   act("$n tries to take on shadow form.", ch, NULL, NULL, TO_ROOM);
   affect_strip(ch, gsn_sneak);

   if (IS_NPC(ch) || number_percent() < ch->pcdata->learned[gsn_shadow])
   {
      af.type = gsn_sneak;
      af.duration = ch->level;
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bitvector = AFF_SNEAK;
      affect_to_char(ch, &af);
   }

   return;
}

void do_sneak(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;

   send_to_char("You attempt to move silently.\n\r", ch);
   affect_strip(ch, gsn_sneak);

   if (IS_NPC(ch) || number_percent() < ch->pcdata->learned[gsn_sneak])
   {
      af.type = gsn_sneak;
      af.duration = ch->level;
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bitvector = AFF_SNEAK;
      affect_to_char(ch, &af);
   }

   return;
}

void do_hide(CHAR_DATA *ch, char *argument)
{
   send_to_char("You attempt to hide.\n\r", ch);

   if (IS_AFFECTED(ch, AFF_HIDE))
      REMOVE_BIT(ch->affected_by, AFF_HIDE);

   if (IS_NPC(ch) || number_percent() < ch->pcdata->learned[gsn_hide])
      SET_BIT(ch->affected_by, AFF_HIDE);

   return;
}

/*
 * Contributed by Alander.
 */
void do_visible(CHAR_DATA *ch, char *argument)
{
   affect_strip(ch, gsn_invis);
   affect_strip(ch, gsn_mass_invis);
   affect_strip(ch, gsn_sneak);
   REMOVE_BIT(ch->affected_by, AFF_HIDE);
   REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
   REMOVE_BIT(ch->affected_by, AFF_SNEAK);
   send_to_char("You become more visible again.\n\r", ch);
   return;
}

void do_clan_recall(CHAR_DATA *ch, char *argument)
{
   ROOM_INDEX_DATA *location;

   if (IS_NPC(ch))
      return;

   if (deathmatch)
   {
      send_to_char("Not during a DEATHMATCH!!!\n\r", ch);
      return;
   }

   if (ch->pcdata->clan == 0)
   {
      send_to_char("You must be in a clan to use this command!!\n\r", ch);
      return;
   }
   else
      location = get_room_index(clan_table[ch->pcdata->clan].clan_room);

   if (location == NULL)
   {
      send_to_char("You are well lost, like.\n\r", ch);
      return;
   }

   if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) || IS_AFFECTED(ch, AFF_CURSE))
   {
      send_to_char("Some strange force prevents your transport.\n\r", ch);
      return;
   }

   if (ch->in_room == location)
   {
      send_to_char("You didn't have far to go, did you?\n\r", ch);
      return;
   }

   if (is_fighting(ch))
   {
      send_to_char("You can't use this command when fighting!!\n\r", ch);
      return;
   }

   act("$n is engulfed in a stream of red light!!", ch, NULL, NULL, TO_ROOM);
   send_to_char("You are engulfed in a stream of red light, carrying you home!\n\r", ch);
   char_from_room(ch);
   char_to_room(ch, location);

   act("$n arrives, carried upon a stream of red light!!", ch, NULL, NULL, TO_ROOM);
   do_look(ch, "auto");
   return;
}

void do_recall(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *location;
   int recall_index;
   act("$n makes the Holy Sign for transportation!", ch, 0, 0, TO_ROOM);
   buf[0] = '\0';

   /*
    * FIXME: Need to check if NPC is charmed - if so find
    * * owner's recall location..... -S-
    */

   /*
    * if ( IS_NPC( ch ) || ch->level < 6 )
    */

   location = get_room_index(ROOM_VNUM_TEMPLE);

   if (!IS_NPC(ch))
   {
      if (ch->pcdata->recall_vnum == 3001 || ch->pcdata->recall_vnum == 3110 ||
          ch->pcdata->recall_vnum == 1026)
         ch->pcdata->recall_vnum = ROOM_VNUM_TEMPLE;
      location = get_room_index(ch->pcdata->recall_vnum);
   }

   if (deathmatch)
   {
      recall_index = number_range(ROOM_VNUM_DM_RECALL, ROOM_VNUM_DM_RECALL + 35);
      location = get_room_index(recall_index);

      if (ch->hit > 250)
      {
         send_to_char(
             "@@lYou wince in pain as you feel your molecules disperse, then reform.@@N\n\r", ch);
         ch->hit -= 200;
      }
      else
      {
         send_to_char(
             "Your molecules begin to disperse, then reform quickly, leaving you here.\n\r", ch);
         return;
      }
   }

   /*
    * Only recall to midgaard now...
    */
   /*
    * else
    * location = get_room_index( race_table[ch->race].recall );
    */

   if (location == NULL)
   {
      send_to_char("You are completely lost.\n\r", ch);
      return;
   }

   if (ch->in_room == location)
      return;

   if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) || IS_AFFECTED(ch, AFF_CURSE))
   {
      send_to_char("Some strange force prevents your recall.\n\r", ch);
      return;
   }

   if ((victim = ch->fighting) != NULL)
   {
      int lose;

      if (number_bits(1) == 0)
      {
         WAIT_STATE(ch, 4);
         lose = (ch->level / 4) + 1;
         lose = UMIN(lose, ch->exp);
         gain_exp(ch, 0 - lose);
         sprintf(buf, "You failed!  You lose %d exps.\n\r", lose);
         send_to_char(buf, ch);
         return;
      }

      lose = (ch->level / 4) + 25;
      lose = UMIN(lose, ch->exp);
      gain_exp(ch, 0 - lose);
      sprintf(buf, "You recall from combat!  You lose %d exps.\n\r", lose);
      send_to_char(buf, ch);
      stop_fighting(ch, TRUE);
   }

   ch->move = (ch->move > 10) ? (ch->move - 10) : 0;
   act("$n is engulfed by a stream of blue light!", ch, NULL, NULL, TO_ROOM);

   char_from_room(ch);
   char_to_room(ch, location);

   act("$n arrives, carried by a stream of blue light!", ch, NULL, NULL, TO_ROOM);
   do_look(ch, "auto");

   return;
}

#define CARAVAN_MIDGAARD_VNUM 1151 /* Caravan Square, Midgaard */

typedef struct caravan_dest CARAVAN_DEST;
struct caravan_dest
{
   const char *name;
   int dest_vnum;
   int quest_template_id; /* completed_quests[id] must be TRUE */
   const char *city_name;
};

/* City cartography quest template IDs (file N.prop -> id N-1) */
static const CARAVAN_DEST caravan_destinations[] = {
    {"kiess", 3661, 144, "Kiess"},
    {"kowloon", 3833, 145, "Kowloon"},
    {"mafdet", 3878, 146, "Mafdet"},
    {"rakuen", 4556, 148, "Rakuen"},
};
static const int num_caravan_destinations =
    (int)(sizeof(caravan_destinations) / sizeof(caravan_destinations[0]));

void do_caravan(CHAR_DATA *ch, char *argument)
{
   ROOM_INDEX_DATA *dest_room;
   const CARAVAN_DEST *dest;
   int i;

   if (IS_NPC(ch))
      return;

   if (ch->fighting != NULL)
   {
      send_to_char("You cannot board a caravan while fighting!\n\r", ch);
      return;
   }

   if (ch->in_room == NULL || ch->in_room->vnum != CARAVAN_MIDGAARD_VNUM)
   {
      send_to_char("You must be at the Caravan Square in Midgaard to use caravan travel.\n\r", ch);
      return;
   }

   if (argument[0] == '\0')
   {
      send_to_char("Caravan travel is available to: Kiess, Kowloon, Mafdet, Rakuen.\n\r", ch);
      send_to_char("Usage: caravan <city>\n\r", ch);
      return;
   }

   dest = NULL;
   for (i = 0; i < num_caravan_destinations; i++)
   {
      if (!str_prefix(argument, caravan_destinations[i].name))
      {
         dest = &caravan_destinations[i];
         break;
      }
   }

   if (dest == NULL)
   {
      send_to_char("Caravan travel is available to: Kiess, Kowloon, Mafdet, Rakuen.\n\r", ch);
      return;
   }

   if (dest->quest_template_id >= 0 && dest->quest_template_id < QUEST_MAX_TEMPLATES &&
       !ch->pcdata->completed_quests[dest->quest_template_id])
   {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf,
              "You haven't completed the cartographic survey of %s. "
              "The caravan master cannot route you there without proper navigation records.\n\r",
              dest->city_name);
      send_to_char(buf, ch);
      return;
   }

   dest_room = get_room_index(dest->dest_vnum);
   if (dest_room == NULL)
   {
      send_to_char("The caravan route to that city is not currently available.\n\r", ch);
      return;
   }

   act("$n climbs aboard a departing caravan wagon bound for the road.", ch, NULL, NULL, TO_ROOM);

   char_from_room(ch);
   char_to_room(ch, dest_room);

   act("$n steps down from a caravan wagon, having arrived from Midgaard.", ch, NULL, NULL,
       TO_ROOM);
   do_look(ch, "auto");
}

void do_train(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *mob;
   int hp_gain = 0;
   int mana_gain = 0;
   long *pAbility;
   char *pOutput;
   int cost;
   buf[0] = '\0';

   if (IS_NPC(ch))
   {
      send_to_char("Not for NPCs.  Sorry.\n\r", ch);
      return;
   }

   /*
    * Check for trainer.
    */

   for (mob = ch->in_room->first_person; mob; mob = mob->next_in_room)
   {
      if (IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN))
         break;
   }

   if (mob == NULL)
   {
      send_to_char("You can't do that here.\n\r", ch);
      return;
   }

   if (argument[0] == '\0')
   {
      sprintf(buf, "You have %d practice sessions.\n\r", ch->practice);
      send_to_char(buf, ch);
      argument = "foo";
   }

   cost = 4;
   /*
      if( !str_cmp( argument, "str" ) )
      {
         if( gclass_table[ch->class].attr_prime == APPLY_STR )
            cost = 3;
         pAbility = &ch->pcdata->perm_str;
         pMax = ch->pcdata->max_str;
         pOutput = "strength";
      }

      else if( !str_cmp( argument, "int" ) )
      {
         if( gclass_table[ch->class].attr_prime == APPLY_INT )
            cost = 3;
         pAbility = &ch->pcdata->perm_int;
         pMax = ch->pcdata->max_int;
         pOutput = "intelligence";
      }

      else if( !str_cmp( argument, "wis" ) )
      {
         if( gclass_table[ch->class].attr_prime == APPLY_WIS )
            cost = 3;
         pAbility = &ch->pcdata->perm_wis;
         pMax = ch->pcdata->max_wis;
         pOutput = "wisdom";
      }

      else if( !str_cmp( argument, "dex" ) )
      {
         if( gclass_table[ch->class].attr_prime == APPLY_DEX )
            cost = 3;
         pAbility = &ch->pcdata->perm_dex;
         pMax = ch->pcdata->max_dex;
         pOutput = "dexterity";
      }

      else if( !str_cmp( argument, "con" ) )
      {
         if( gclass_table[ch->class].attr_prime == APPLY_CON )
            cost = 3;
         pAbility = &ch->pcdata->perm_con;
         pMax = ch->pcdata->max_con;
         pOutput = "constitution";
      }*/

   if (!str_cmp(argument, "hp"))
   {
      pAbility = &ch->max_hit;
      pOutput = "number of hit points";
      cost = 4;    /* this is pracs per "train hp" */
      hp_gain = 1; /* this is hp gained per "train hp" */
   }

   else if (!str_cmp(argument, "mana"))
   {
      pAbility = &ch->max_mana;
      pOutput = "amount of mana";
      cost = 5;
      mana_gain = 1;
   }

   else
   {
      /* Work out the attribute training costs here if that option is re-enabled. */

      strcpy(buf, "You can train: hp (4)  mana (5)");

      /*   if( ch->pcdata->perm_str < ch->pcdata->max_str )
         {
            strcat( buf, "  str" );
            sprintf( buf2, " (%d)", cost1 );
            strcat( buf, buf2 );
         }
         if( ch->pcdata->perm_int < ch->pcdata->max_int )
         {
            strcat( buf, "  int" );
            sprintf( buf2, " (%d)", cost2 );
            strcat( buf, buf2 );
         }
         if( ch->pcdata->perm_wis < ch->pcdata->max_wis )
         {
            strcat( buf, "  wis" );
            sprintf( buf2, " (%d)", cost3 );
            strcat( buf, buf2 );
         }
         if( ch->pcdata->perm_dex < ch->pcdata->max_dex )
         {
            strcat( buf, "  dex" );
            sprintf( buf2, " (%d)", cost4 );
            strcat( buf, buf2 );
         }
         if( ch->pcdata->perm_con < ch->pcdata->max_con )
         {
            strcat( buf, "  con" );
            sprintf( buf2, " (%d)", cost5 );
            strcat( buf, buf2 );
         }*/
      if (buf[strlen(buf) - 1] != ':')
      {
         strcat(buf, ".\n\r");
         send_to_char(buf, ch);
      }
      return;
   }

   if (!str_cmp(argument, "hp"))
   {

      if (cost > ch->practice)
      {
         send_to_char("You don't have enough practices.\n\r", ch);
         return;
      }

      ch->practice -= cost;
      *pAbility += hp_gain;
      ch->pcdata->hp_from_gain += 1;
      act("Your $T increases!", ch, NULL, pOutput, TO_CHAR);
      act("$n's $T increases!", ch, NULL, pOutput, TO_ROOM);
      return;
   }

   if (!str_cmp(argument, "mana"))
   {

      if (cost > ch->practice)
      {
         send_to_char("You don't have enough practices.\n\r", ch);
         return;
      }

      ch->practice -= cost;
      *pAbility += mana_gain;
      ch->pcdata->mana_from_gain += 1;

      act("Your $T increases!", ch, NULL, pOutput, TO_CHAR);
      act("$n's $T increases!", ch, NULL, pOutput, TO_ROOM);
      return;
   }

   return;
   /*   if( *pAbility >= pMax )
      {
         act( "Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR );
         return;
      }*/

   if (cost > ch->practice)
   {
      send_to_char("You don't have enough practices.\n\r", ch);
      return;
   }

   /*   ch->practice -= cost;
      *pAbility += 1;
      act( "Your $T increases!", ch, NULL, pOutput, TO_CHAR );
      act( "$n's $T increases!", ch, NULL, pOutput, TO_ROOM );*/
   return;
}

void do_guild(CHAR_DATA *ch, char *argument)
{
   send_to_char("This command is no longer employed here!\n\r", ch);
   return;
}

void do_halls(CHAR_DATA *ch, char *argument)
/*
 * Quite a lot of the following code derives from the original
 * * MERC2.2 code, although the tweaks are my own.
 * * -- Stephen
 */
{

   CHAR_DATA *victim;

   /*
    * Remember to limit use of this command to staff!
    */

   if (ch->level < 81)
   {
      send_to_char("Huh?\n\r", ch);
      return;
   }

   act("$n prepares to return to the Halls of the Staff!", ch, 0, 0, TO_ROOM);

   /*
    * Check if staff member is fighting.
    * * Stop fight if they are!
    */

   if ((victim = ch->fighting) != NULL)
   {
      stop_fighting(ch, TRUE);
   }

   act("$n vanishes into a beam of energy!", ch, NULL, NULL, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, get_room_index(ROOM_VNUM_HALLS));
   act("$n steps out of the energy beam!", ch, NULL, NULL, TO_ROOM);

   do_look(ch, "auto");
   return;
}

void do_scan(CHAR_DATA *ch, char *argument)
/* Informs ch if there are any (N)PCs in the 6 adjacent rooms.
 * I'm sure this could be written better.... ;)
 * -- Stephen
 */
{
   ROOM_INDEX_DATA *location;
   int door;
   char buf[MAX_INPUT_LENGTH];
   char person[MAX_INPUT_LENGTH];
   CHAR_DATA *d;
   bool found = FALSE;
   buf[0] = '\0';
   person[0] = '\0';

   act("$n looks all around for anyone else.", ch, NULL, NULL, TO_ROOM);
   send_to_char("You look around.\n\r", ch);

   location = ch->in_room;
   for (door = 0; door < 6; door++)
   {
      EXIT_DATA *pexit;

      if ((pexit = location->exit[door]) == 0 || pexit->to_room == NULL ||
          IS_SET(pexit->exit_info, EX_CLOSED) || (str_cmp(pexit->keyword, "")) ||
          (pexit->to_room->vnum == ch->in_room->vnum))
      {
         /*
          * Then this direction is "invalid"
          */
         continue;
      }
      else
      {

         for (d = pexit->to_room->first_person; d != NULL; d = d->next_in_room)
         {

            if ((pexit->to_room->vnum == d->in_room->vnum) && can_see(ch, d))
            {
               found = TRUE;

               /*
                * bit here to set person to (N)PC name
                */

               if (IS_NPC(d))
               {
                  sprintf(person, "%s (NPC)", d->short_descr);
               }
               else
               {
                  sprintf(person, "%s", PERS(d, ch));
               }

               snprintf(buf, sizeof(buf), "%.32s : %.256s\n\r", dir_name[door], person);

               send_to_char(buf, ch);
            }
         }
         if ((pexit->to_room->affected_by != 0) &&
             ((is_affected(ch, skill_lookup("detect magic"))) ||
              (item_has_apply(ch, ITEM_APPLY_DET_MAG))))
         {
            sprintf(buf, "The room %s has a @@rMagical@@N Affect!!!\n\r", dir_name[door]);
            send_to_char(buf, ch);
         }
      }
   }
   if (!found)
   {
      send_to_char("You fail to spot anyone around you.\n\r", ch);
   }

   /* v2 WebSocket: send scan data to browser client */
   if (!IS_NPC(ch) && ch->desc && ch->desc->websocket_active)
      ws_send_map_scan(ch->desc, ch);

   return;
}

void do_enter(CHAR_DATA *ch, char *argument)
{
   OBJ_DATA *portal;
   ROOM_INDEX_DATA *to_room;
   OBJ_DATA *foo;
   OBJ_DATA *foo_next;
   int door;

   if (argument[0] == '\0')
   {
      send_to_char("Enter what?\n\r", ch);
      return;
   }

   portal = get_obj_here(ch, argument);
   if (portal == NULL || portal->item_type != ITEM_PORTAL || portal->value[2] == 0)
   {
      /* look for a keyworded door */

      if ((door = find_door(ch, argument)) >= 0)
      {
         /*
          * enterable exit
          */
         ch->using_named_door = TRUE;
         move_char(ch, door);
         return;
      }
      else
      {
         /*        send_to_char( "You can't enter that!\n\r", ch );  */
         return;
      }

      return;
   }

   if ((to_room = get_room_index(portal->value[0])) == NULL)
   {
      send_to_char("Nothing happens.\n\r", ch);
      return;
   }
   if ((IS_SET(ch->in_room->room_flags, ROOM_NO_PORTAL) ||
        IS_SET(to_room->room_flags, ROOM_NO_PORTAL)) &&
       number_range(0, 100) < 75)
   {
      act("The anti-magic zone causes the portal to @@eIMPLODE@@N!!!", ch, NULL, NULL, TO_ROOM);
      send_to_char("The anti-magic zone causes the portal to @@eIMPLODE@@N!!!", ch);

      extract_obj(portal);

      /*
       * Now check if there is a corresponding portal in to_room
       */
      for (foo = to_room->first_content; foo != NULL; foo = foo_next)
      {
         foo_next = foo->next_in_room;

         if (foo->item_type == ITEM_PORTAL && foo->value[0] == ch->in_room->vnum)
            break;
         extract_obj(foo);
      }
      return;
   }

   act("$n enters $p.", ch, portal, NULL, TO_ROOM);
   act("You enter $p.", ch, portal, NULL, TO_CHAR);

   /*
    * Now check if there is a corresponding portal in to_room
    */
   for (foo = to_room->first_content; foo != NULL; foo = foo->next_in_room)
      if (foo->item_type == ITEM_PORTAL && foo->value[0] == ch->in_room->vnum)
         break;

   char_from_room(ch);
   char_to_room(ch, to_room);

   if (foo == NULL)
      act("A glowing sphere forms, and $n steps out!", ch, NULL, NULL, TO_ROOM);
   else
      act("$n steps out of $p.", ch, foo, NULL, TO_ROOM);

   do_look(ch, "");
   return;
}

void do_scout(CHAR_DATA *ch, char *argument)
{
   ROOM_INDEX_DATA *loc;
   ROOM_INDEX_DATA *tmploc;
   int door;
   int depth = 0;
   int counter;
   CHAR_DATA *target;
   char buf[MAX_INPUT_LENGTH];
   char pre[MAX_INPUT_LENGTH];
   buf[0] = '\0';
   pre[0] = '\0';

   if (!can_use_skill(ch, gsn_scout))
   {
      send_to_char("You are not trained in this skill!\n\r", ch);
      return;
   }

   depth = 4;

   act("$n looks all around for anyone else.", ch, NULL, NULL, TO_ROOM);
   send_to_char("You look around.\n\r", ch);

   while (depth > 0)
   {
      loc = ch->in_room;
      for (door = 0; door < 6; door++)
      {
         tmploc = loc;

         /*
          * get the room
          */
         for (counter = 0; counter < depth; counter++)
         {
            if (tmploc->exit[door] == 0 || tmploc->exit[door]->to_room == NULL ||
                IS_SET(tmploc->exit[door]->exit_info, EX_CLOSED) ||
                (str_cmp(tmploc->exit[door]->keyword, "")))
               break;

            tmploc = tmploc->exit[door]->to_room;
         }

         /*
          * if room didn't exist
          */
         if (counter < depth)
            continue;

         for (target = tmploc->first_person; target != NULL; target = target->next_in_room)
         {
            if (target->in_room->vnum == ch->in_room->vnum)
               break;

            if (!can_see(ch, target))
               continue;

            if (depth == 1)
               sprintf(pre, "%s : ", dir_name[door]);
            else if (depth == 2)
               sprintf(pre, "Far %s : ", dir_name[door]);
            else if (depth == 3)
               sprintf(pre, "Very Far %s : ", dir_name[door]);
            else if (depth == 4)
               sprintf(pre, "Distant %s : ", dir_name[door]);

            if (IS_NPC(target))
               snprintf(buf, sizeof(buf), "%.64s%.256s\n\r", pre, target->short_descr);
            else
               snprintf(buf, sizeof(buf), "%.64s%.256s\n\r", pre, target->name);

            send_to_char(buf, ch);
         }
      }

      depth = depth - 1;
   }
}
