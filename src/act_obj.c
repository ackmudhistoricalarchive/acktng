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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"

/*
 * Local functions.
 */
void wear_obj(CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace);
CHAR_DATA *find_keeper(CHAR_DATA *ch);
int get_cost(CHAR_DATA *keeper, OBJ_DATA *obj);
void check_guards(CHAR_DATA *ch);
void set_obj_stat_auto(OBJ_DATA *obj);

char *format_obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch, bool fShort);

typedef struct quartermaster_emblem_option_type
{
   const char *tier_name;
   const char *style_name;
   int weight;
   int invasion_points;
   int commendations;
   int ribbons;
   int medals;
   int tier_extra_flag;
} QUARTERMASTER_EMBLEM_OPTION_TYPE;

static const QUARTERMASTER_EMBLEM_OPTION_TYPE quartermaster_emblem_options[] = {
    {"common", "caster", 3, 1, 5, 0, 0, 0},
    {"common", "melee", 8, 1, 5, 0, 0, 0},
    {"common", "tank", 13, 1, 5, 0, 0, 0},
    {"magic", "caster", 3, 10, 50, 5, 0, ITEM_MAGIC},
    {"magic", "melee", 8, 10, 50, 5, 0, ITEM_MAGIC},
    {"magic", "tank", 13, 10, 50, 5, 0, ITEM_MAGIC},
    {"rare", "caster", 3, 100, 500, 50, 5, ITEM_RARE},
    {"rare", "melee", 8, 100, 500, 50, 5, ITEM_RARE},
    {"rare", "tank", 13, 100, 500, 50, 5, ITEM_RARE},
    {NULL, NULL, 0, 0, 0, 0, 0, 0}};

static CHAR_DATA *find_quartermaster(CHAR_DATA *ch)
{
   CHAR_DATA *keeper;

   if (ch == NULL || ch->in_room == NULL)
      return NULL;

   for (keeper = ch->in_room->first_person; keeper != NULL; keeper = keeper->next_in_room)
   {
      if (IS_NPC(keeper) && IS_SET(keeper->act, ACT_QUARTERMASTER) &&
          !IS_AFFECTED(keeper, AFF_CHARM) && can_see(ch, keeper))
         return keeper;
   }

   return NULL;
}

static const QUARTERMASTER_EMBLEM_OPTION_TYPE *
find_quartermaster_emblem_option(const char *tier_name, const char *style_name)
{
   int i;

   if (tier_name == NULL || style_name == NULL)
      return NULL;

   for (i = 0; quartermaster_emblem_options[i].tier_name != NULL; i++)
   {
      if (!str_cmp(quartermaster_emblem_options[i].tier_name, tier_name) &&
          !str_cmp(quartermaster_emblem_options[i].style_name, style_name))
         return &quartermaster_emblem_options[i];
   }

   return NULL;
}

static int quartermaster_emblem_has_tokens(const QUARTERMASTER_EMBLEM_OPTION_TYPE *option)
{
   if (option == NULL)
      return FALSE;

   return (option->commendations > 0 || option->ribbons > 0 || option->medals > 0);
}

static void append_quartermaster_emblem_cost(char *dest, size_t dest_size,
                                             const QUARTERMASTER_EMBLEM_OPTION_TYPE *option)
{
   char token_buf[MSL];

   if (dest == NULL || option == NULL)
      return;

   snprintf(dest, dest_size, "%d invasion point%s", option->invasion_points,
            option->invasion_points == 1 ? "" : "s");

   if (!quartermaster_emblem_has_tokens(option))
      return;

   snprintf(token_buf, sizeof(token_buf), " (");

   if (option->commendations > 0)
      snprintf(token_buf + strlen(token_buf), sizeof(token_buf) - strlen(token_buf),
               "%d commendation%s", option->commendations, option->commendations == 1 ? "" : "s");

   if (option->ribbons > 0)
      snprintf(token_buf + strlen(token_buf), sizeof(token_buf) - strlen(token_buf),
               "%s%d ribbon%s", option->commendations > 0 ? ", " : "", option->ribbons,
               option->ribbons == 1 ? "" : "s");

   if (option->medals > 0)
      snprintf(token_buf + strlen(token_buf), sizeof(token_buf) - strlen(token_buf), "%s%d medal%s",
               (option->commendations > 0 || option->ribbons > 0) ? ", " : "", option->medals,
               option->medals == 1 ? "" : "s");

   snprintf(token_buf + strlen(token_buf), sizeof(token_buf) - strlen(token_buf), ")");
   safe_strcat(dest_size, dest, token_buf);
}

static int quartermaster_can_afford_option(const CHAR_DATA *ch,
                                           const QUARTERMASTER_EMBLEM_OPTION_TYPE *option)
{
   if (ch == NULL || option == NULL || IS_NPC(ch) || ch->pcdata == NULL)
      return FALSE;

   if (ch->pcdata->invasion_points < option->invasion_points)
      return FALSE;

   if (ch->pcdata->invasion_rewards[0] < option->commendations)
      return FALSE;

   if (ch->pcdata->invasion_rewards[1] < option->ribbons)
      return FALSE;

   if (ch->pcdata->invasion_rewards[2] < option->medals)
      return FALSE;

   return TRUE;
}

static void quartermaster_capitalize_word(const char *src, char *dest, size_t dest_size)
{
   if (dest == NULL || dest_size == 0)
      return;

   if (src == NULL)
   {
      dest[0] = '\0';
      return;
   }

   snprintf(dest, dest_size, "%s", src);
   if (dest[0] >= 'a' && dest[0] <= 'z')
      dest[0] = (char)(dest[0] - 'a' + 'A');
}

static const char *quartermaster_tier_color(const char *tier_name)
{
   if (tier_name == NULL)
      return "@@N";

   if (!str_cmp(tier_name, "magic"))
      return "@@r";

   if (!str_cmp(tier_name, "rare"))
      return "@@l";

   return "@@N";
}

static const char *quartermaster_style_color(const char *style_name)
{
   if (style_name == NULL)
      return "@@N";

   if (!str_cmp(style_name, "caster"))
      return "@@c";

   if (!str_cmp(style_name, "melee"))
      return "@@r";

   if (!str_cmp(style_name, "tank"))
      return "@@g";

   return "@@N";
}

static void show_quartermaster_list(CHAR_DATA *ch)
{
   char buf[MSL];
   char cost_summary[MSL];
   char tier_display[32];
   char style_display[32];
   const char *tier_color;
   const char *style_color;
   int i;

   if (ch == NULL)
      return;

   send_to_char("\n\r@@g[@@yTier@@g]   @@yStyle@@g   @@yWeight@@g   @@yCost@@N\n\r", ch);

   for (i = 0; quartermaster_emblem_options[i].tier_name != NULL; i++)
   {
      append_quartermaster_emblem_cost(cost_summary, sizeof(cost_summary),
                                       &quartermaster_emblem_options[i]);
      quartermaster_capitalize_word(quartermaster_emblem_options[i].tier_name, tier_display,
                                    sizeof(tier_display));
      quartermaster_capitalize_word(quartermaster_emblem_options[i].style_name, style_display,
                                    sizeof(style_display));
      tier_color = quartermaster_tier_color(quartermaster_emblem_options[i].tier_name);
      style_color = quartermaster_style_color(quartermaster_emblem_options[i].style_name);
      snprintf(buf, sizeof(buf), "@@g[@@a%s%-6s@@g] %s%-6s@@g  @@W%3d@@g      @@W%-.120s@@N\n\r",
               tier_color, tier_display, style_color, style_display,
               quartermaster_emblem_options[i].weight, cost_summary);
      send_to_char(buf, ch);
   }

   send_to_char("\n\rBuy syntax: @@Wbuy <tier> <style>@@N (styles: caster/melee/tank).\n\r", ch);
}

static OBJ_DATA *create_quartermaster_emblem(CHAR_DATA *ch,
                                             const QUARTERMASTER_EMBLEM_OPTION_TYPE *option)
{
   char buf[MSL];
   char tier_display[32];
   char style_display[32];
   OBJ_DATA *obj;
   int pseudo_level;

   if (ch == NULL || option == NULL)
      return NULL;

   pseudo_level = get_psuedo_level(ch);

   obj = create_object(get_obj_index(OBJ_VNUM_MUSHROOM), 0);
   if (obj == NULL)
      return NULL;

   obj->item_type = ITEM_ARMOR;
   /* Set level from buyer before set_obj_stat_auto() so allocation scales correctly. */
   obj->level = pseudo_level;
   obj->wear_flags = ITEM_TAKE | ITEM_WEAR_INVASION_EMBLEM;
   obj->weight = option->weight;
   obj->cost = 0;

   /* Quartermaster emblems should persist and should not bind on first equip. */
   REMOVE_BIT(obj->extra_flags, ITEM_NOSAVE);
   REMOVE_BIT(obj->extra_flags, ITEM_BIND_EQUIP);
   SET_BIT(obj->extra_flags, ITEM_NODROP);

   if (option->tier_extra_flag != 0)
      SET_BIT(obj->extra_flags, option->tier_extra_flag);

   snprintf(buf, sizeof(buf), "%s %s invasion emblem", option->tier_name, option->style_name);
   free_string(obj->name);
   obj->name = str_dup(buf);

   quartermaster_capitalize_word(option->tier_name, tier_display, sizeof(tier_display));
   quartermaster_capitalize_word(option->style_name, style_display, sizeof(style_display));
   snprintf(buf, sizeof(buf), "%s%s@@N %s%s@@N Invasion Emblem",
            quartermaster_tier_color(option->tier_name), tier_display,
            quartermaster_style_color(option->style_name), style_display);
   free_string(obj->short_descr);
   obj->short_descr = str_dup(buf);

   snprintf(buf, sizeof(buf), "A %s%s@@N %s%s@@N invasion emblem is here.",
            quartermaster_tier_color(option->tier_name), option->tier_name,
            quartermaster_style_color(option->style_name), option->style_name);
   free_string(obj->description);
   obj->description = str_dup(buf);

   set_obj_stat_auto(obj);

   return obj;
}

bool should_enforce_equip_restrictions(const CHAR_DATA *ch)
{
   return (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM));
}

static bool is_keep_chest(const OBJ_DATA *obj)
{
   return (obj != NULL && obj->item_type == ITEM_CONTAINER &&
           IS_SET(obj->value[1], CONT_KEEP_CHEST));
}

static int container_item_count(const OBJ_DATA *container)
{
   const OBJ_DATA *obj;
   int count = 0;

   if (container == NULL)
      return 0;

   for (obj = container->first_in_carry_list; obj != NULL; obj = obj->next_in_carry_list)
      count++;

   return count;
}

#define KEEP_CHEST_PUT_ALLOWED 0
#define KEEP_CHEST_PUT_ERR_FULL 1
#define KEEP_CHEST_PUT_ERR_INVALID_ITEM 2
#define KEEP_CHEST_PUT_ERR_BONDED 3

int keep_chest_put_denial_reason(const OBJ_DATA *container_obj, const OBJ_DATA *obj)
{
   int keep_chest_capacity;

   if (!is_keep_chest(container_obj))
      return KEEP_CHEST_PUT_ALLOWED;

   keep_chest_capacity = (container_obj->value[3] > 0) ? container_obj->value[3] : 50;
   if (container_item_count(container_obj) >= keep_chest_capacity)
      return KEEP_CHEST_PUT_ERR_FULL;

   if (obj != NULL && (obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC ||
                       obj->item_type == ITEM_CONTAINER))
      return KEEP_CHEST_PUT_ERR_INVALID_ITEM;

   if (obj != NULL && IS_SET(obj->extra_flags, ITEM_BONDED))
      return KEEP_CHEST_PUT_ERR_BONDED;

   return KEEP_CHEST_PUT_ALLOWED;
}

void get_obj(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container)
{

   if (!CAN_WEAR(obj, ITEM_TAKE) || obj->item_type == ITEM_CORPSE_PC)
   {
      send_to_char("You can't take that.\n\r", ch);
      return;
   }

   if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
   {
      act("$d: you can't carry that many items.", ch, NULL, obj->name, TO_CHAR);
      return;
   }

   if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch))
   {
      act("$d: you can't carry that much weight.", ch, NULL, obj->name, TO_CHAR);
      return;
   }

   if (container != NULL)
   {
      act("You get $p from $P.", ch, obj, container, TO_CHAR);
      act("$n gets $p from $P.", ch, obj, container, TO_ROOM);
      obj_from_obj(obj);
   }
   else
   {
      act("You get $p.", ch, obj, container, TO_CHAR);
      act("$n gets $p.", ch, obj, container, TO_ROOM);
      obj_from_room(obj);
   }

   if (obj->item_type == ITEM_MONEY)
   {
      if (ch->gold + obj->value[0] > 100000000)
      {
         send_to_char("You can't carry anymore gold.\n\r", ch);
         if (container != NULL)
            obj_to_obj(obj, container);
         else
            obj_to_room(obj, ch->in_room);
         return;
      }
      ch->gold += obj->value[0];
      extract_obj(obj);
   }
   else
   {
      obj_to_char(obj, ch);
      trigger_handler(ch, obj, TRIGGER_GET);
   }

   return;
}

void do_get(CHAR_DATA *ch, char *argument)
{

   char container_name[MSL];
   char object_list[MSL];
   char one_object[MSL];
   char object_number[MSL];
   char victim_name[MSL];
   char actbuf[MSL];

   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   OBJ_DATA *container = NULL;
   bool found = FALSE;
   bool get_ok = FALSE;
   bool found_money = FALSE;
   victim_name[0] = '\0';

   pre_parse(argument, victim_name, container_name, object_list);

   /*
    * Get type.
    */
   if (object_list[0] == '\0')
   {
      send_to_char("Get what?\n\r", ch);
      return;
   }
   if ((container_name[0] != '\0') && (container = get_obj_here(ch, container_name)) == NULL)
   {
      act("I see no $T here.", ch, NULL, container_name, TO_CHAR);
      return;
   }

   if ((container != NULL) && (container->item_type == ITEM_CORPSE_PC))
   {
      if (IS_NPC(ch))
      {
         send_to_char("You can't do that.\n\r", ch);
         return;
      }
      if (str_cmp(container->owner, ch->name) && !IS_STAFF(ch))
      {
         send_to_char("You can't do that.\n\r", ch);
         return;
      }
   }
   if ((container != NULL) && (container->item_type == ITEM_CORPSE_NPC))
   {
      if (!IS_NPC(ch) && container->owner != NULL && container->owner[0] != '\0' && !IS_STAFF(ch) &&
          str_cmp(container->owner, ch->name))
      {
         CHAR_DATA *owner_ch = NULL;
         CHAR_DATA *gch;
         for (gch = first_char; gch != NULL; gch = gch->next)
         {
            if (!IS_NPC(gch) && !str_cmp(container->owner, gch->name))
            {
               owner_ch = gch;
               break;
            }
         }
         if (owner_ch == NULL || !is_same_group(ch, owner_ch))
         {
            send_to_char("You can't do that.\n\r", ch);
            return;
         }
      }
   }
   if (container != NULL)
   {
      if (IS_SET(container->value[1], CONT_CLOSED))
      {
         act("The $d is closed.", ch, NULL, container->name, TO_CHAR);
         return;
      }
      if ((!str_prefix("1 all", object_list)) || (!str_cmp("coinsonly", object_list)))
      {
         /*
          * 'get all container' or 'get all.obj container'
          */
         found = FALSE;
         if (str_cmp("coinsonly", object_list))
            for (obj = container->first_in_carry_list; obj != NULL; obj = obj_next)
            {
               obj_next = obj->next_in_carry_list;
               if (can_see_obj(ch, obj))
               {
                  found = TRUE;
                  get_obj(ch, obj, container);
               }
            }
         if (!found && !found_money)
         {
            act("There is nothing at all in the $T.", ch, NULL, container_name, TO_CHAR);
         }
         else
         {
            if (container->item_type == ITEM_CORPSE_PC)
               save_corpses();
            else if (is_keep_chest(container))
               save_chest(container);
         }
         if (!IS_NPC(ch))
            do_save(ch, "");
         return;
      }
      else
      {
         char *parse;
         parse = object_list;
         sprintf(bug_buf, "Get %s, object_list: %s, container %s", argument, object_list,
                 container_name);
         monitor_chan(bug_buf, MONITOR_DEBUG);

         for (;;) /* 'get obj corpse' */
         {
            sh_int looper;

            parse = one_argument(parse, object_number);
            if (object_number[0] == '\0')
               break;
            parse = one_argument(parse, one_object);
            if (one_object[0] == '\0')
               break;
            sprintf(bug_buf, "Looking for %s %s", object_number, one_object);
            monitor_chan(bug_buf, MONITOR_DEBUG);
            for (looper = 0; looper < (is_number(object_number) ? atoi(object_number) : 0);
                 looper++)
            {
               obj = get_obj_list(ch, one_object, container->first_in_carry_list);
               if ((obj == NULL) || (!can_see_obj(ch, obj)))
               {
                  snprintf(actbuf, sizeof(actbuf), "I see no more %.128s in the %.128s.",
                           one_object, container_name);
                  act(actbuf, ch, NULL, NULL, TO_CHAR);
                  break;
               }
               get_obj(ch, obj, container);
            }
         }
         if (container->item_type == ITEM_CORPSE_PC)
            save_corpses();
         else if (is_keep_chest(container))
            save_chest(container);

         if (!IS_NPC(ch))
            do_save(ch, "");

         return;
      }
   }
   else /* getting from room */
   {
      if (!str_prefix("1 all", object_list))
      { /* 'get all' or 'get all.obj' */
         found = FALSE;
         if (get_ok)
            found = TRUE;
         for (obj = ch->in_room->first_content; obj != NULL; obj = obj_next)
         {
            obj_next = obj->next_in_room;
            if (can_see_obj(ch, obj))
            {
               found = TRUE;
               get_obj(ch, obj, NULL);
            }
         }
         if (found && ch->level > 1)
            do_save(ch, "");

         if (!found && !get_ok)
         {
            send_to_char("I see nothing here.\n\r", ch);
         }
         return;
      }
      else
      {
         bool found_one_obj = FALSE;
         char *parse;
         parse = object_list;

         for (;;)
         {
            sh_int looper;
            sprintf(bug_buf, "In get thing room, parse is %s.", parse);
            monitor_chan(bug_buf, MONITOR_DEBUG);
            parse = one_argument(parse, object_number);
            if (object_number[0] == '\0')
               break;
            parse = one_argument(parse, one_object);
            if (one_object[0] == '\0')
               break;
            sprintf(bug_buf, "Looking for %s %s.", object_number, one_object);
            monitor_chan(bug_buf, MONITOR_DEBUG);
            for (looper = 0; looper < atoi(object_number); looper++)
            {
               obj = get_obj_room(ch, one_object, ch->in_room->first_content);
               if (obj == NULL)
               {
                  if (found_one_obj)
                  {
                     snprintf(actbuf, sizeof(actbuf), "There isn't another %.128s here.",
                              one_object);
                     act(actbuf, ch, NULL, NULL, TO_CHAR);
                     break;
                  }
#ifdef LOTS
                  else if (ch->in_room->first_portal != NULL)
                  {
                     PORTAL_DATA *portal;
                     for (portal = ch->in_room->first_portal; portal; portal = portal->next)
                     {
                        if (!str_cmp(arg1, portal->keyword))
                        {
                           act("You cannot do that.", ch, NULL, NULL, TO_CHAR);
                           break;
                        }
                     }
                  }
#endif
               }

               else if (!can_see_obj(ch, obj))
               {
                  snprintf(actbuf, sizeof(actbuf), "I see no more %.128s here.", one_object);
                  act(actbuf, ch, NULL, NULL, TO_CHAR);
                  break;
               }
               else
               {
                  get_obj(ch, obj, NULL);
                  found_one_obj = TRUE;
               }
            }
         }
         if ((found == FALSE) && (found_one_obj == FALSE) && (found_money == FALSE) &&
             (get_ok == FALSE))
         {
            send_to_char("I don't see what your looking for in this room.\n\r", ch);
            return;
         }
         if (ch->level > 1)
            do_save(ch, "");

         return;
      }
   }
   if (ch->level > 1)
      do_save(ch, "");

   return;
}

void do_gold(CHAR_DATA *ch, char *argument)
{
   char buf[MSL];

   sprintf(buf, "You have %d gold.\n\r", ch->gold);

   send_to_char(buf, ch);

   return;
}

void do_put(CHAR_DATA *ch, char *argument)
{
   char object_list[MAX_INPUT_LENGTH];
   char container_name[MSL];
   char victim_name[MSL];
   char actbuf[MSL];
   OBJ_DATA *container_obj;
   OBJ_DATA *obj;

   pre_parse(argument, victim_name, container_name, object_list);

   if (object_list[0] == '\0')
   {
      send_to_char("Drop what?\n\r", ch);
      return;
   }
   if (container_name[0] == '\0')
   {
      send_to_char("What container?\n\r", ch);
      return;
   }
   else if ((container_obj = get_obj_here(ch, container_name)) == NULL)
   {
      act("I see no $T here.", ch, NULL, container_name, TO_CHAR);
      return;
   }
   if ((container_obj->item_type != ITEM_CONTAINER) &&
       (container_obj->item_type != ITEM_SPELL_MATRIX))
   {
      send_to_char("That's not a container.\n\r", ch);
      return;
   }
   if (IS_SET(container_obj->value[1], CONT_CLOSED))
   {
      act("The $d is closed.", ch, NULL, container_obj->name, TO_CHAR);
      return;
   }

   if (!str_prefix("1 all", object_list) || !str_prefix("all.", object_list))
   {
      send_to_char("You must list the items you wish to put into your container!\n\r", ch);
      return;
   }
   else
   {
      char *parse = object_list;
      char object_number[MSL];
      char one_object[MSL];
      for (;;)
      {
         sh_int looper;
         sprintf(bug_buf, "In put thing container, parse is %s.", parse);
         monitor_chan(bug_buf, MONITOR_DEBUG);
         parse = one_argument(parse, object_number);
         if (object_number[0] == '\0')
            break;
         parse = one_argument(parse, one_object);
         if (one_object[0] == '\0')
            break;
         sprintf(bug_buf, "Looking for %s %s.", object_number, one_object);
         monitor_chan(bug_buf, MONITOR_DEBUG);
         for (looper = 0; looper < atoi(object_number); looper++)
         {
            obj = get_obj_carry(ch, one_object);
            if (obj == NULL)
            {
               break;
            }
            if (!is_keep_chest(container_obj) && !can_drop_obj(ch, obj))
            {
               act("You can't let go of $d.", ch, NULL, obj->name, TO_CHAR);

               if (looper < 10)
               {
                  continue;
               }
               else
               {
                  break;
               }
            }
            if (obj == container_obj)
            {
               send_to_char("You can't fold it into itself.\n\r", ch);
               continue;
            }
            {
               int keep_chest_restriction = keep_chest_put_denial_reason(container_obj, obj);
               if (keep_chest_restriction == KEEP_CHEST_PUT_ERR_FULL)
               {
                  send_to_char("That keep chest cannot hold any more items.\n\r", ch);
                  continue;
               }

               if (keep_chest_restriction == KEEP_CHEST_PUT_ERR_INVALID_ITEM)
               {
                  send_to_char("Keep chests cannot hold corpses or containers.\n\r", ch);
                  continue;
               }

               if (keep_chest_restriction == KEEP_CHEST_PUT_ERR_BONDED)
               {
                  send_to_char("Your bonded weapon refuses to leave your side.\n\r", ch);
                  continue;
               }
            }

            if (get_obj_weight(obj) + get_obj_weight(container_obj) > container_obj->value[0])
            {
               act("$p won't fit into $P.", ch, obj, container_obj, TO_CHAR);
               continue;
            }
            if (container_obj->item_type == ITEM_SPELL_MATRIX)
            {
               if ((obj->item_type == ITEM_WEAPON) || (obj->item_type == ITEM_LIGHT) ||
                   (obj->item_type == ITEM_ARMOR))
               {
                  OBJ_DATA *one_obj;
                  bool has_item = FALSE;
                  for (one_obj = container_obj->first_in_carry_list; one_obj != NULL;
                       one_obj = one_obj->next_in_carry_list)
                  {
                     if (one_obj->item_type != ITEM_ENCHANTMENT)
                     {
                        has_item = TRUE;
                        break;
                     }
                  }
                  if (has_item)
                  {
                     send_to_char(
                         "Spell Matrix containers may hold only 1 non-enchantment item.\n\r", ch);
                     return;
                  }
               }
               else if (obj->item_type != ITEM_ENCHANTMENT)
               {
                  send_to_char("Spell Matrix containers may hold only 1 non-enchantment item.\n\r",
                               ch);
                  return;
               }
            }

            obj_from_char(obj);
            obj_to_obj(obj, container_obj);
            sprintf(actbuf, "$n puts $p into $P%s.",
                    ((container_obj->carried_by != NULL) ? "" : " ( in the room )"));
            act(actbuf, ch, obj, container_obj, TO_ROOM);
            sprintf(actbuf, "You put $p into $P%s.",
                    ((container_obj->carried_by != NULL) ? "" : " ( in the room )"));

            act(actbuf, ch, obj, container_obj, TO_CHAR);
            /*
             * trigger_handler( ch, obj, TRIGGER_DROP );
             */
         }
      }
      if (is_keep_chest(container_obj))
         save_chest(container_obj);

      if (!IS_NPC(ch))
         do_save(ch, "");
      return;
   }

   return;
}

void do_drop(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char container_name[MSL];
   char victim_name[MSL];
   char object_list[MSL];
   OBJ_DATA *obj;
   OBJ_DATA *obj_next;
   bool found = FALSE;
   char object_number[MSL];
   char one_object[MSL];

   victim_name[0] = '\0';
   pre_parse(argument, victim_name, container_name, object_list);

   if (object_list[0] == '\0')
   {
      send_to_char("Drop what?\n\r", ch);
      return;
   }

   if (str_prefix("1 all", object_list) && str_prefix("all.", arg))
   {
      char *parse;
      parse = object_list;
      for (;;)
      {
         sh_int looper;
         sprintf(bug_buf, "In drop thing room, parse is %s.", parse);
         monitor_chan(bug_buf, MONITOR_DEBUG);
         parse = one_argument(parse, object_number);
         if (object_number[0] == '\0')
            break;
         parse = one_argument(parse, one_object);
         if (one_object[0] == '\0')
            break;
         sprintf(bug_buf, "Looking for %s %s.", object_number, one_object);
         monitor_chan(bug_buf, MONITOR_DEBUG);
         for (looper = 0; looper < atoi(object_number); looper++)
         {
            obj = get_obj_carry(ch, one_object);
            if (obj == NULL)
            {
               break;
            }
            if (!can_drop_obj(ch, obj))
            {
               send_to_char("You can't let go of it.\n\r", ch);
               if (looper < 10)
               {
                  continue;
               }
               else
               {
                  break;
               }
            }
            obj_from_char(obj);
            obj_to_room(obj, ch->in_room);
            act("$n drops $p.", ch, obj, NULL, TO_ROOM);
            act("You drop $p.", ch, obj, NULL, TO_CHAR);
            /*
             * trigger_handler( ch, obj, TRIGGER_DROP );
             */
         }
      }
      if (ch->level > 1)
         do_save(ch, "");
      return;
   }

   /*
    * 'drop all' or 'drop all.obj'
    */
   found = FALSE;
   for (obj = ch->first_carry; obj != NULL; obj = obj_next)
   {
      obj_next = obj->next_in_carry_list;

      if (can_see_obj(ch, obj) && obj->wear_loc == WEAR_NONE && can_drop_obj(ch, obj))
      {
         found = TRUE;
         obj_from_char(obj);
         obj_to_room(obj, ch->in_room);
         act("You drop $p.", ch, obj, NULL, TO_CHAR);
         act("$n drops $p.", ch, obj, NULL, TO_ROOM);
         /*
          * trigger_handler( ch, obj, TRIGGER_DROP );
          */
      }
   }

   if (!found)
   {
      act("You are not carrying anything.", ch, NULL, arg, TO_CHAR);
   }

   if (found && ch->level > 1)
      do_save(ch, "");
   return;
}

void do_give(CHAR_DATA *ch, char *argument)
{
   char victim_name[MSL];
   char container_name[MSL];
   char object_list[MSL];
   char one_object[MSL];
   char object_number[MSL];
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   container_name[0] = '\0';
   pre_parse(argument, victim_name, container_name, object_list);

   if (object_list[0] == '\0' || victim_name[0] == '\0')
   {
      send_to_char("Give what to whom?\n\r", ch);
      return;
   }

   if ((victim = get_char_room(ch, victim_name)) == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
      return;
   }

   if (str_prefix("1 all", object_list) && str_prefix("all.", object_list))
   {
      char *parse;
      parse = object_list;
      for (;;)
      {
         sh_int looper;
         sprintf(bug_buf, "In give thing, parse is %s.", parse);
         monitor_chan(bug_buf, MONITOR_DEBUG);
         parse = one_argument(parse, object_number);
         if (object_number[0] == '\0')
            break;
         parse = one_argument(parse, one_object);
         if (one_object[0] == '\0')
            break;
         sprintf(bug_buf, "Looking for %s %s.", object_number, one_object);
         monitor_chan(bug_buf, MONITOR_DEBUG);
         for (looper = 0; looper < atoi(object_number); looper++)
         {
            obj = get_obj_carry(ch, one_object);
            if (obj == NULL)
            {
               break;
            }
            if (obj->wear_loc != WEAR_NONE)
            {
               send_to_char("You must remove it first.\n\r", ch);
               continue;
            }
            if (!can_drop_obj(ch, obj))
            {
               send_to_char("You can't let go of it.\n\r", ch);
               if (looper < 10)
               {
                  continue;
               }
               else
               {
                  break;
               }
            }
            if (victim->carry_number + get_obj_number(obj) > can_carry_n(victim))
            {
               act("$N has $S hands full.", ch, NULL, victim, TO_CHAR);
               continue;
            }

            if (victim->carry_weight + get_obj_weight(obj) > can_carry_w(victim))
            {
               act("$N can't carry that much weight.", ch, NULL, victim, TO_CHAR);
               continue;
            }

            if (!can_see_obj(victim, obj))
            {
               act("$N can't see it.", ch, NULL, victim, TO_CHAR);
               continue;
            }
            obj_from_char(obj);
            obj_to_char(obj, victim);
            act("$n gives $p to $N.", ch, obj, victim, TO_NOTVICT);
            act("You give $p to $N.", ch, obj, victim, TO_CHAR);
            act("$n gives you $p.", ch, obj, victim, TO_VICT);
            /*
             * trigger_handler( ch, obj, TRIGGER_DROP );
             */

            if ((quest || auto_quest) && IS_NPC(victim) && victim == quest_mob &&
                obj == quest_object)
            {
               /*
                * Then ch has recovered the quest object!!!!!
                */
               sprintf(buf, "Oh! %s you found %s for me!  Thank You!", NAME(ch), obj->short_descr);
               do_say(victim, buf);
               interpret(victim, "hop");

               if (!IS_NPC(ch))
               {
                  sprintf(buf, "%s I shall reward you well for recovering this for me!", NAME(ch));
                  do_tell(victim, buf);
                  if (obj->value[0] > 0)
                  {
                     sprintf(buf, "You receive %d quest points!\n\r", obj->value[0]);
                     send_to_char(buf, ch);
                     ch->quest_points += obj->value[0];
                  }

                  if (obj->value[2] > 0)
                  {
                     sprintf(buf, "You receive %d gold coins!\n\r", obj->value[2]);
                     send_to_char(buf, ch);
                     ch->gold += obj->value[2];
                  }
               }
               crusade_complete(ch);
            }
         }
      }
   }
   if (ch->level > 1)
      do_save(victim, "");

   return;
}

void do_eat(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;

   one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      send_to_char("Eat what?\n\r", ch);
      return;
   }

   if ((obj = get_obj_carry(ch, arg)) == NULL)
   {
      send_to_char("You do not have that item.\n\r", ch);
      return;
   }

   if (!IS_STAFF(ch))
   {
      if (obj->item_type != ITEM_PILL)
      {
         send_to_char("That's not edible.\n\r", ch);
         return;
      }
   }

   act("$n eats $p.", ch, obj, NULL, TO_ROOM);
   act("You eat $p.", ch, obj, NULL, TO_CHAR);

   switch (obj->item_type)
   {
   case ITEM_PILL:
      obj_cast_spell(obj->value[1], obj->value[0], ch, ch, NULL);
      obj_cast_spell(obj->value[2], obj->value[0], ch, ch, NULL);
      obj_cast_spell(obj->value[3], obj->value[0], ch, ch, NULL);
      break;
   }

   extract_obj(obj);
   return;
}

/*
 * Remove an object.
 */
bool remove_obj(CHAR_DATA *ch, int iWear, bool fReplace)
{
   OBJ_DATA *obj;
   AFFECT_DATA *aff;
   sh_int hp_mod = 0;
   if ((obj = get_eq_char(ch, iWear)) == NULL)
      return TRUE;

   if (!fReplace)
      return FALSE;

   if (IS_SET(obj->extra_flags, ITEM_NOREMOVE))
   {
      act("You can't remove $p.", ch, obj, NULL, TO_CHAR);
      return FALSE;
   }

   // check for hitpoints low, and hp enhancing eq
   for (aff = obj->first_apply; aff; aff = aff->next)
   {
      if (aff->location == APPLY_HIT)
         hp_mod += aff->modifier;
   }

   if ((hp_mod >= get_max_hp(ch)) || (hp_mod >= ch->hit))
   {
      send_to_char("That is the only thing keeping you alive!\n\r", ch);
      return FALSE;
   }
   unequip_char(ch, obj);
   act("$n stops using $p.", ch, obj, NULL, TO_ROOM);
   act("You stop using $p.", ch, obj, NULL, TO_CHAR);
   /*
    * trigger_handler( ch, obj, TRIGGER_REMOVE );
    */
   return TRUE;
}

/* MAG
   can_wear_at(ch,obj,location) returns true if the char can wear the object
   at the location.
   */

bool can_wear_at(CHAR_DATA *ch, OBJ_DATA *obj, int location)
{
   int loc_flag;

   switch (location)
   {
   case WEAR_NONE:
      loc_flag = 0;
      break;
   case WEAR_LIGHT:
      loc_flag = ITEM_WEAR_HOLD_HAND;
      break;
   case WEAR_FINGER_L:
   case WEAR_FINGER_R:
      loc_flag = ITEM_WEAR_FINGER;
      break;
   case WEAR_NECK_1:
   case WEAR_NECK_2:
      loc_flag = ITEM_WEAR_NECK;
      break;
   case WEAR_BODY:
      loc_flag = ITEM_WEAR_BODY;
      break;
   case WEAR_HEAD:
      loc_flag = ITEM_WEAR_HEAD;
      break;
   case WEAR_LEGS:
      loc_flag = ITEM_WEAR_LEGS;
      break;
   case WEAR_FEET:
      loc_flag = ITEM_WEAR_FEET;
      break;
   case WEAR_HANDS:
      loc_flag = ITEM_WEAR_HANDS;
      break;
   case WEAR_ARMS:
      loc_flag = ITEM_WEAR_ARMS;
      break;
   case WEAR_TAIL:
      loc_flag = ITEM_WEAR_TAIL;
      break;
   case WEAR_ABOUT:
      loc_flag = ITEM_WEAR_ABOUT;
      break;
   case WEAR_WAIST:
      loc_flag = ITEM_WEAR_WAIST;
      break;
   case WEAR_WRIST_L:
   case WEAR_WRIST_R:
      loc_flag = ITEM_WEAR_WRIST;
      break;
   case WEAR_HORNS:
      loc_flag = ITEM_WEAR_HORNS;
      break;
   case WEAR_BEAK:
      loc_flag = ITEM_WEAR_BEAK;
      break;
   case WEAR_FACE:
      loc_flag = ITEM_WEAR_FACE;
      break;
   case WEAR_EAR_L:
   case WEAR_EAR_R:
      loc_flag = ITEM_WEAR_EAR;
      break;
   case WEAR_HOOVES:
      loc_flag = ITEM_WEAR_HOOVES;
      break;
   case WEAR_AURA:
      loc_flag = ITEM_WEAR_AURA;
      break;
   case WEAR_HALO:
      loc_flag = ITEM_WEAR_HALO;
      break;
   case WEAR_WINGS:
      loc_flag = ITEM_WEAR_WINGS;
      break;
   case WEAR_SHOULDERS:
      loc_flag = ITEM_WEAR_SHOULDERS;
      break;
   case WEAR_CLAWS:
      loc_flag = ITEM_WEAR_CLAWS;
      break;
   case WEAR_CLAN_COLORS:
      loc_flag = ITEM_WEAR_CLAN_COLORS;
      break;
   case WEAR_INVASION_EMBLEM:
      loc_flag = ITEM_WEAR_INVASION_EMBLEM;
      break;
   case WEAR_HOLD_HAND_L:
   case WEAR_HOLD_HAND_R:
      loc_flag = ITEM_WEAR_HOLD_HAND;
      break;

   default:
      loc_flag = 0;
      break;
   }

   if (!((IS_NPC(ch)) && area_resetting_global) &&
       race_table[URANGE(0, ch->race, MAX_RACE)].wear_locs[location] == FALSE)
   {
      act("You don't have anywhere to wear $p!", ch, obj, NULL, TO_CHAR);
      act("$L$n fumbles about with $p, looking for a place to wear it.", ch, obj, NULL, TO_ROOM);
      return FALSE;
   }

   if (((obj->wear_flags & loc_flag) == 0) && (obj->wear_loc == -1))
   {
      send_to_char("That item can't be worn there!\n\r", ch);
      return FALSE;
   }

   return TRUE;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj(CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace)
{
   char buf[MSL];

   if (!IS_NPC(ch) && IS_SET(obj->extra_flags, ITEM_BUCKLER) && !can_use_skill(ch, gsn_two_handed))
   {
      send_to_char("You cannot wear two-handed weapons.\n\r", ch);
      return;
   }

   if (!IS_NPC(ch) && IS_SET(obj->extra_flags, ITEM_BUCKLER) &&
       !can_use_skill(ch, gsn_equip_buckler))
   {
      send_to_char("You cannot wear bucklers.\n\r", ch);
      return;
   }

   if (!IS_NPC(ch) && IS_SET(obj->extra_flags, ITEM_EXTRA_WAND) &&
       !can_use_skill(ch, gsn_equip_wand))
   {
      send_to_char("You cannot equip wands.\n\r", ch);
      return;
   }

   if (!IS_NPC(ch) && IS_SET(obj->extra_flags, ITEM_FIST) && !can_use_skill(ch, gsn_equip_fist))
   {
      send_to_char("You cannot equip fist weapons.\n\r", ch);
      return;
   }

   if (should_enforce_equip_restrictions(ch) && get_psuedo_level(ch) < obj->level)
   {
      sprintf(buf, "You must be level %d to use this object.\n\r", obj->level);
      send_to_char(buf, ch);
      act("$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM);
      return;
   }

   if (obj->item_type == ITEM_TRIGGER && obj->value[0] == 6)
   {
      /*
       * NOTE: you can't actually GET or WEAR a trigger item that triggers on get/wear. ZEN
       */
      trigger_handler(ch, obj, TRIGGER_GET);
      return;
   }

   int weight_class = (obj->weight + 4) / 5;
   int max_weight = (get_curr_str(ch) / 10) + 1;

   if (should_enforce_equip_restrictions(ch) && (weight_class > max_weight))
   {
      send_to_char("You are not of sufficient strength to use it effectively.\n\r", ch);
      act("$L$n nearly drops a $p.", ch, obj, NULL, TO_ROOM);
      return;
   }

   if ((obj->item_type == ITEM_LIGHT) && (can_wear_at(ch, obj, WEAR_LIGHT)))
   {
      if (!remove_obj(ch, WEAR_LIGHT, fReplace))
         return;
      act("$L$n lights a $p.", ch, obj, NULL, TO_ROOM);
      act("You light $p and hold it.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_LIGHT);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_FINGER)) && (can_wear_at(ch, obj, WEAR_FINGER_L)))
   {
      if (get_eq_char(ch, WEAR_FINGER_L) != NULL && get_eq_char(ch, WEAR_FINGER_R) != NULL &&
          !remove_obj(ch, WEAR_FINGER_L, fReplace) && !remove_obj(ch, WEAR_FINGER_R, fReplace))
         return;

      if (get_eq_char(ch, WEAR_FINGER_L) == NULL)
      {
         act("$n slips $p onto $s left finger.", ch, obj, NULL, TO_ROOM);
         act("You slip $p onto your left finger.", ch, obj, NULL, TO_CHAR);
         equip_char(ch, obj, WEAR_FINGER_L);
         return;
      }

      if (get_eq_char(ch, WEAR_FINGER_R) == NULL)
      {
         act("$n slips $p onto $s right finger.", ch, obj, NULL, TO_ROOM);
         act("You slip $p onto your right finger.", ch, obj, NULL, TO_CHAR);
         equip_char(ch, obj, WEAR_FINGER_R);
         return;
      }
      send_to_char("You already wear two rings.\n\r", ch);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_NECK)) && (can_wear_at(ch, obj, WEAR_NECK_1)))
   {
      if (get_eq_char(ch, WEAR_NECK_1) != NULL && get_eq_char(ch, WEAR_NECK_2) != NULL &&
          !remove_obj(ch, WEAR_NECK_1, fReplace) && !remove_obj(ch, WEAR_NECK_2, fReplace))
         return;

      if (get_eq_char(ch, WEAR_NECK_1) == NULL)
      {
         act("$n fastens $p around $s neck.", ch, obj, NULL, TO_ROOM);
         act("You fasten $p around your neck.", ch, obj, NULL, TO_CHAR);
         equip_char(ch, obj, WEAR_NECK_1);
         return;
      }

      if (get_eq_char(ch, WEAR_NECK_2) == NULL)
      {
         act("$n fastens $p around $s neck.", ch, obj, NULL, TO_ROOM);
         act("You fasten $p around your neck.", ch, obj, NULL, TO_CHAR);
         equip_char(ch, obj, WEAR_NECK_2);
         return;
      }

      bug("Wear_obj: no free neck.", 0);
      send_to_char("You already wear two neck items.\n\r", ch);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_BODY)) && (can_wear_at(ch, obj, WEAR_BODY)))
   {
      if (!remove_obj(ch, WEAR_BODY, fReplace))
         return;
      act("$n pulls $p down over $s body.", ch, obj, NULL, TO_ROOM);
      act("You pull $p down over your body.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_BODY);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_HEAD)) && (can_wear_at(ch, obj, WEAR_HEAD)))
   {
      if (!remove_obj(ch, WEAR_HEAD, fReplace))
         return;
      act("$n places $p onto $s head.", ch, obj, NULL, TO_ROOM);
      act("You place $p onto your head.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_HEAD);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_LEGS)) && (can_wear_at(ch, obj, WEAR_LEGS)))
   {
      if (!remove_obj(ch, WEAR_LEGS, fReplace))
         return;
      act("$n pushes $s legs into $p.", ch, obj, NULL, TO_ROOM);
      act("You push you legs into $p.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_LEGS);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_FEET)) && (can_wear_at(ch, obj, WEAR_FEET)))
   {
      if (!remove_obj(ch, WEAR_FEET, fReplace))
         return;
      act("$n slides $s feet into $p.", ch, obj, NULL, TO_ROOM);
      act("You slide your feet into $p.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_FEET);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_HANDS)) && (can_wear_at(ch, obj, WEAR_HANDS)))
   {
      if (!remove_obj(ch, WEAR_HANDS, fReplace))
         return;
      act("$n pulls $p over $s hands.", ch, obj, NULL, TO_ROOM);
      act("You pull $p over your hands.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_HANDS);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_ARMS)) && (can_wear_at(ch, obj, WEAR_ARMS)))
   {
      if (!remove_obj(ch, WEAR_ARMS, fReplace))
         return;
      act("$n pushes $p onto $s arms.", ch, obj, NULL, TO_ROOM);
      act("You push $p onto your arms.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_ARMS);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_ABOUT)) && (can_wear_at(ch, obj, WEAR_ABOUT)))
   {
      if (!remove_obj(ch, WEAR_ABOUT, fReplace))
         return;
      act("$n fixes $p around $s body.", ch, obj, NULL, TO_ROOM);
      act("You fix $p around your body.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_ABOUT);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_WAIST)) && (can_wear_at(ch, obj, WEAR_WAIST)))
   {
      if (!remove_obj(ch, WEAR_WAIST, fReplace))
         return;
      act("$n secures $p about $s waist.", ch, obj, NULL, TO_ROOM);
      act("You secure $p about your waist.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_WAIST);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_WRIST)) && (can_wear_at(ch, obj, WEAR_WRIST_L)))
   {
      if (get_eq_char(ch, WEAR_WRIST_L) != NULL && get_eq_char(ch, WEAR_WRIST_R) != NULL &&
          !remove_obj(ch, WEAR_WRIST_L, fReplace) && !remove_obj(ch, WEAR_WRIST_R, fReplace))
         return;

      if (get_eq_char(ch, WEAR_WRIST_L) == NULL)
      {
         act("$n slides $p onto $s left wrist.", ch, obj, NULL, TO_ROOM);
         act("You slide $p onto your left wrist.", ch, obj, NULL, TO_CHAR);
         equip_char(ch, obj, WEAR_WRIST_L);
         return;
      }

      if (get_eq_char(ch, WEAR_WRIST_R) == NULL)
      {
         act("$n slides $p onto $s right wrist.", ch, obj, NULL, TO_ROOM);
         act("You slide $p onto your right wrist.", ch, obj, NULL, TO_CHAR);
         equip_char(ch, obj, WEAR_WRIST_R);
         return;
      }

      bug("Wear_obj: no free wrist.", 0);
      send_to_char("You already wear two wrist items.\n\r", ch);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_FACE)) && (can_wear_at(ch, obj, WEAR_FACE)))
   {
      if (!remove_obj(ch, WEAR_FACE, fReplace))
         return;

      act("$n wears $p on $s face.", ch, obj, NULL, TO_ROOM);
      act("You wear $p on your face.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_FACE);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_EAR)) && (can_wear_at(ch, obj, WEAR_EAR_L)))
   {
      if (get_eq_char(ch, WEAR_EAR_L) != NULL && get_eq_char(ch, WEAR_EAR_R) != NULL &&
          !remove_obj(ch, WEAR_EAR_L, fReplace) && !remove_obj(ch, WEAR_EAR_R, fReplace))
         return;
      if (get_eq_char(ch, WEAR_EAR_L) == NULL)
      {
         act("$n clips $p on $s left ear.", ch, obj, NULL, TO_ROOM);
         act("You clip $p on your left ear.", ch, obj, NULL, TO_CHAR);
         equip_char(ch, obj, WEAR_EAR_L);
         return;
      }

      if (get_eq_char(ch, WEAR_EAR_R) == NULL)
      {
         act("$n clips $p on $s right ear.", ch, obj, NULL, TO_ROOM);
         act("You clip $p on your right ear.", ch, obj, NULL, TO_CHAR);
         equip_char(ch, obj, WEAR_EAR_R);
         return;
      }

      bug("Wear_obj: no free ear.", 0);
      send_to_char("You already wear two earrings.\n\r", ch);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_HALO)) && (can_wear_at(ch, obj, WEAR_HALO)))
   {
      if (!remove_obj(ch, WEAR_HALO, fReplace))
         return;

      act("$n places $p over $s head.", ch, obj, NULL, TO_ROOM);
      act("You place $p over your head.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_HALO);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_AURA)) && (can_wear_at(ch, obj, WEAR_AURA)))
   {
      if (!remove_obj(ch, WEAR_AURA, fReplace))
         return;

      act("$n wraps $p around $s soul.", ch, obj, NULL, TO_ROOM);
      act("You wrap $p around your soul.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_AURA);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_HORNS)) && (can_wear_at(ch, obj, WEAR_HORNS)))
   {
      if (!remove_obj(ch, WEAR_HORNS, fReplace))
         return;

      act("$n slides $p over $s horns.", ch, obj, NULL, TO_ROOM);
      act("You slide $p over your horns.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_HORNS);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_BEAK)) && (can_wear_at(ch, obj, WEAR_BEAK)))
   {
      if (!remove_obj(ch, WEAR_BEAK, fReplace))
         return;

      act("$n slides $p over $s beak.", ch, obj, NULL, TO_ROOM);
      act("You slide $p over your beak.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_BEAK);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_WINGS)) && (can_wear_at(ch, obj, WEAR_WINGS)))
   {
      if (!remove_obj(ch, WEAR_WINGS, fReplace))
         return;

      act("$n slides $p over $s wings.", ch, obj, NULL, TO_ROOM);
      act("You slide $p over your wings.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_WINGS);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_SHOULDERS)) && (can_wear_at(ch, obj, WEAR_SHOULDERS)))
   {
      if (!remove_obj(ch, WEAR_SHOULDERS, fReplace))
         return;

      act("$n slides $p onto $s shoulders.", ch, obj, NULL, TO_ROOM);
      act("You slide $p onto your shoulders.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_SHOULDERS);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_CLAWS)) && (can_wear_at(ch, obj, WEAR_CLAWS)))
   {
      if (!remove_obj(ch, WEAR_CLAWS, fReplace))
         return;

      act("$n slides $p over $s claws.", ch, obj, NULL, TO_ROOM);
      act("You slide $p over your claws.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_CLAWS);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_TAIL)) && (can_wear_at(ch, obj, WEAR_TAIL)))
   {
      if (!remove_obj(ch, WEAR_TAIL, fReplace))
         return;

      act("$n slides $p over $s tail.", ch, obj, NULL, TO_ROOM);
      act("You slide $p over your tail.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_TAIL);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_HOOVES)) && (can_wear_at(ch, obj, WEAR_HOOVES)))
   {
      if (!remove_obj(ch, WEAR_HOOVES, fReplace))
         return;

      act("$n slides $p over $s hooves.", ch, obj, NULL, TO_ROOM);
      act("You slide $p over your hooves.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_HOOVES);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_CLAN_COLORS)) && (can_wear_at(ch, obj, WEAR_CLAN_COLORS)))
   {
      if (!remove_obj(ch, WEAR_CLAN_COLORS, fReplace))
         return;

      act("$n begins using $p as $s clan colors.", ch, obj, NULL, TO_ROOM);
      act("You begin using $p as your clan colors.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_CLAN_COLORS);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_INVASION_EMBLEM)) && (can_wear_at(ch, obj, WEAR_INVASION_EMBLEM)))
   {
      if (!remove_obj(ch, WEAR_INVASION_EMBLEM, fReplace))
         return;

      act("$n wears $p as an invasion emblem.", ch, obj, NULL, TO_ROOM);
      act("You wear $p as an invasion emblem.", ch, obj, NULL, TO_CHAR);
      equip_char(ch, obj, WEAR_INVASION_EMBLEM);
      return;
   }

   if ((CAN_WEAR(obj, ITEM_WEAR_HOLD_HAND)) && (can_wear_at(ch, obj, WEAR_HOLD_HAND_L)))
   {
      if (IS_WEAPON(obj) && is_affected(ch, skill_lookup("disarm")))
      {
         send_to_char("You have recently been disarmed and can't equip a weapon yet!\n\r", ch);
         return;
      }

      if (IS_SET(obj->extra_flags, ITEM_TWO_HANDED) || IS_SET(obj->extra_flags, ITEM_BUCKLER))
      {
         if (IS_SET(obj->extra_flags, ITEM_TWO_HANDED) && !can_use_skill(ch, gsn_two_handed))
         {
            act("You cannot use two-handed weapons!", ch, obj, NULL, TO_CHAR);
            return;
         }

         if (IS_SET(obj->extra_flags, ITEM_BUCKLER) && !can_use_skill(ch, gsn_equip_buckler))
         {
            act("You cannot use bucklers!", ch, obj, NULL, TO_CHAR);
            return;
         }

         if ((get_eq_char(ch, WEAR_HOLD_HAND_L) != NULL &&
              !remove_obj(ch, WEAR_HOLD_HAND_L, fReplace)) ||
             (get_eq_char(ch, WEAR_HOLD_HAND_R) != NULL &&
              !remove_obj(ch, WEAR_HOLD_HAND_R, fReplace)))
         {
            act("You cannot remove what is in your hands!", ch, obj, NULL, TO_CHAR);
            return;
         }

         if (IS_SET(obj->extra_flags, ITEM_TWO_HANDED))
         {
            act("$n holds $p in both hands.", ch, obj, NULL, TO_ROOM);
            act("You hold $p in both of your hands.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_TWO_HANDED);
            return;
         }

         if (IS_SET(obj->extra_flags, ITEM_BUCKLER))
         {
            act("$n uses $p as $s buckler", ch, obj, NULL, TO_ROOM);
            act("You use $p as your buckler.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_BUCKLER);
            return;
         }
      }
      else
      {
         if (get_eq_char(ch, WEAR_TWO_HANDED) != NULL && !remove_obj(ch, WEAR_TWO_HANDED, fReplace))
         {
            act("You cannot remove your 2-handed weapon!", ch, obj, NULL, TO_CHAR);
            return;
         }

         if (get_eq_char(ch, WEAR_BUCKLER) != NULL && !remove_obj(ch, WEAR_BUCKLER, fReplace))
         {
            act("You cannot remove your buckler!", ch, obj, NULL, TO_CHAR);
            return;
         }

         if (get_eq_char(ch, WEAR_HOLD_HAND_L) != NULL &&
             get_eq_char(ch, WEAR_HOLD_HAND_R) != NULL &&
             !remove_obj(ch, WEAR_HOLD_HAND_L, fReplace) &&
             !remove_obj(ch, WEAR_HOLD_HAND_R, fReplace))
            return;

         if (get_eq_char(ch, WEAR_HOLD_HAND_L) == NULL)
         {
            if (!can_wield(ch, obj, WEAR_HOLD_HAND_L))
            {
               act("You are not capable of wielding two weapons!", ch, obj, NULL, TO_CHAR);
               return;
            }
            act("$n holds $p in $s left hand.", ch, obj, NULL, TO_ROOM);
            act("You hold $p in your left hand.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_HOLD_HAND_L);
            return;
         }

         if (get_eq_char(ch, WEAR_HOLD_HAND_R) == NULL)
         {
            if (!can_wield(ch, obj, WEAR_HOLD_HAND_R))
            {
               act("You are not capable of wielding two weapons!", ch, obj, NULL, TO_CHAR);
               return;
            }
            act("$n holds $p in $s right hand.", ch, obj, NULL, TO_ROOM);
            act("You hold $p in your right hand.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_HOLD_HAND_R);
            return;
         }
         bug("Wear_obj: no free hand.", 0);
         send_to_char("You already hold two items.\n\r", ch);
         return;
      }
   }

   if (fReplace)
   {
      send_to_char("Well, that didn't work!\n\r", ch);
      return;
   }
}

void do_wear(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   sh_int num_unique = 0;

   {
      OBJ_DATA *obj_next;
      for (obj = ch->first_carry; obj != NULL; obj = obj_next)
      {
         obj_next = obj->next_in_carry_list;
         if (obj->wear_loc != WEAR_NONE && IS_SET(obj->extra_flags, ITEM_UNIQUE))
            num_unique++;
      }
   }

   one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      sh_int location;
      char outbuf[MSL];
      char catbuf[MSL];
      char colbuf[MSL], eqbuf[MSL];
      OBJ_DATA *worn;
      extern char *const where_name[];
      sprintf(outbuf, "%s", "Wear slots for your race:\n\r");
      for (location = 0; location < MAX_WEAR; location++)
      {
         if (race_table[ch->race].wear_locs[location] == TRUE)
         {
            if (location == WEAR_TWO_HANDED && get_eq_char(ch, location) == NULL)
               continue;
            if (location == WEAR_BUCKLER && get_eq_char(ch, location) == NULL)
               continue;
            if ((location == WEAR_HOLD_HAND_R || location == WEAR_HOLD_HAND_L) &&
                get_eq_char(ch, WEAR_TWO_HANDED) != NULL)
               continue;
            else if ((worn = get_eq_char(ch, location)) != NULL)
            {
               sprintf(colbuf, "%s", "@@!");
               sprintf(eqbuf, "%s", format_obj_to_char(worn, ch, TRUE));
            }
            else
            {
               sprintf(colbuf, "%s", "@@.");
               sprintf(eqbuf, "%s", "@@dNothing@@N");
            }
            snprintf(catbuf, sizeof(catbuf), "%.8s%25.25s@@N %-*.*s\n\r", colbuf,
                     where_name[location], ccode_len(eqbuf, 40), 200, eqbuf);
            strcat(outbuf, catbuf);
         }
      }
      send_to_char(outbuf, ch);
      return;
   }

   if (!str_cmp(arg, "all"))
   {
      OBJ_DATA *obj_next;
      for (obj = ch->first_carry; obj != NULL; obj = obj_next)
      {
         obj_next = obj->next_in_carry_list;
         if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj))
         {
            if ((num_unique > 4) && (IS_SET(obj->extra_flags, ITEM_UNIQUE)))
               send_to_char("You may only wear 5 unique items at one time.\n\r", ch);
            else
               wear_obj(ch, obj, FALSE);
         }
         if (IS_SET(obj->extra_flags, ITEM_UNIQUE))
            num_unique++;
      }

      return;
   }
   else
   {
      if ((obj = get_obj_carry(ch, arg)) == NULL)
      {
         send_to_char("You do not have that item.\n\r", ch);
         return;
      }
      if ((num_unique > 4) && (IS_SET(obj->extra_flags, ITEM_UNIQUE)))
      {
         send_to_char("You may only wear 5 unique items at one time.\n\r", ch);
         return;
      }

      wear_obj(ch, obj, TRUE);
   }
}

void remove_all(CHAR_DATA *ch)
{
   int counter = 0;
   while (counter < MAX_WEAR)
   {
      remove_obj(ch, counter, TRUE);
      counter = counter + 1;
   }

   return;
}

void do_remove(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;

   one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      send_to_char("Remove what?\n\r", ch);
      return;
   }

   if (!str_cmp(arg, "all"))
   {
      remove_all(ch);
      return;
   }

   if ((obj = get_obj_wear(ch, arg)) == NULL)
   {
      send_to_char("You do not have that item.\n\r", ch);
      return;
   }

   remove_obj(ch, obj->wear_loc, TRUE);
   return;
}

void do_sacrifice(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   char monbuf[MSL];
   int gp;
   OBJ_DATA *obj;
   sh_int align_change = 0;
   sh_int align_direction = 0;
   bool change_align = FALSE;
   bool paying_fine = FALSE;
   int obj_value;
   argument = one_argument(argument, arg);
   argument = one_argument(argument, arg2);
   if (arg[0] == '\0' || !str_cmp(arg, ch->name))
   {
      act("@@N$n offers $mself to @@e" sacgodname "@@N, who graciously declines.", ch, NULL, NULL,
          TO_ROOM);
      send_to_char("@@e" sacgodname "@@N appreciates your offer and may accept it later.", ch);
      return;
   }

   if (arg2[0] != '\0')
   {
      if (!str_prefix(arg2, goodgodname))
      {
         align_direction = 1;
         change_align = TRUE;
      }
      else if (!str_prefix(arg2, evilgodname))
      {
         align_direction = -1;
         change_align = TRUE;
      }
      else if (!str_prefix(arg2, "judge"))
      {
         align_direction = 0;
         paying_fine = TRUE;
      }
   }

   obj = get_obj_room(ch, arg, ch->in_room->first_content);
   if (obj == NULL)
   {
      send_to_char("You can't find it.\n\r", ch);
      return;
   }

   if (!can_sac_obj(ch, obj))
   {
      act("$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR);
      return;
   }

   if (obj == quest_object)
   {
      send_to_char("Sorry, there is currently a retrieval quest running for that item.\n\r", ch);
      return;
   }
   if (!CAN_WEAR(obj, ITEM_TAKE) && (obj->item_type != ITEM_CORPSE_NPC))
   {
      act("$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR);
      return;
   }

   /*
    * Checks for payment of fines.
    */
   if (paying_fine)
   {
      sh_int plevel = get_psuedo_level(ch);
      if (ch->sentence <= 0)
      {

         send_to_char("You have no fines outstanding.\n\r", ch);
         return;
      }
      else if (!CAN_WEAR(obj, ITEM_TAKE) || plevel < obj->level || plevel > (obj->level + 30))
      {

         act("The judge will not accept $p as payment.", ch, obj, 0, TO_CHAR);
         return;
      }
      else if (IS_SET(obj->extra_flags, ITEM_CLAN_EQ))
      {

         send_to_char("You cannot use clan equipment for payment of fines.\n\r", ch);
         return;
      }
      obj_value = get_item_value(obj);
      if (plevel > 110)
         obj_value *= .2;
      else if (plevel > 100)
         obj_value *= .4;
      else if (plevel > 80)
         obj_value *= .6;
      if ((obj->item_type == ITEM_BEACON) || (obj->item_type == ITEM_SOUL))
         obj_value = 0;
      ch->sentence -= obj_value;
      if (ch->sentence > 0)
      {

         sprintf(buf, "%s: Sentence reduced to %d ( - %d ) by sacrificing %s.", ch->name,
                 ch->sentence, obj_value, obj->short_descr);
         monitor_chan(buf, MONITOR_OBJ);
         act("The judge accepts $p as partial payment of your fine.", ch, obj, 0, TO_CHAR);
      }
      else
      {

         ch->sentence = 0;
         REMOVE_BIT(ch->act, PLR_KILLER);
         REMOVE_BIT(ch->act, PLR_THIEF);
         send_to_char("Your debt to society has been paid!  Please more careful in the future.\n\r",
                      ch);
         sprintf(monbuf, "%s has had a WANTED flag removed by the judge.\n\r", ch->name);
         monitor_chan(monbuf, MONITOR_GEN_MORT);
      }

      extract_obj(obj);
      /*
       * Force a save, to prevent cheaters claiming reimb rights.
       */
      do_save(ch, "");
      return;
   }

   if (obj->item_type == ITEM_CORPSE_NPC)
   {
      gp = UMAX(1, obj->level / 5);
      change_align = FALSE;
      paying_fine = FALSE;
   }
   else
   {
      gp = 1;
   }

   if (change_align)
   {

      align_change = obj->level;
      if (align_direction == 1)
      {
         if (IS_SET(obj->extra_flags, ITEM_ANTI_GOOD))
            align_change *= 1.3;
         if (IS_SET(obj->extra_flags, ITEM_ANTI_EVIL))
            align_change *= -.3;
         sprintf(buf, "@@a" goodgodname "@@N gives you %d gold coins for your sacrifice.\n\r", gp);
         send_to_char(buf, ch);
         act("@@N$n sacrifices $p to @@a" goodgodname "@@N.", ch, obj, NULL, TO_ROOM);
      }

      if (align_direction == -1)
      {
         if (IS_SET(obj->extra_flags, ITEM_ANTI_GOOD))
            align_change *= -.3;
         if (IS_SET(obj->extra_flags, ITEM_ANTI_EVIL))
            align_change *= 1.5;
         sprintf(buf, "@@e" evilgodname "@@N gives you %d gold coins for your sacrifice.\n\r", gp);
         send_to_char(buf, ch);
         act("@@N$n sacrifices $p to @@e" evilgodname "@@N.", ch, obj, NULL, TO_ROOM);
      }
      if (obj->item_type == ITEM_BEACON || obj->item_type == ITEM_LIGHT ||
          obj->item_type == ITEM_PORTAL)
         align_change /= 10;
      ch->alignment += align_direction * align_change;
      ch->alignment = URANGE(-1000, ch->alignment, 1000);
   }

   if (!change_align)
   {
      sprintf(buf, "@@N" sacgodname "@@N gives you %d gold coins for your sacrifice.\n\r", gp);
      send_to_char(buf, ch);
      act("@@N$n sacrifices $p to @@N" sacgodname "@@N.", ch, obj, NULL, TO_ROOM);
   }

   extract_obj(obj);

   ch->gold += gp;
   return;
}

void do_junk(CHAR_DATA *ch, char *argument)
{
   OBJ_DATA *obj;
   char arg[MAX_INPUT_LENGTH];

   argument = one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("What do you want to junk?\n\r", ch);
      return;
   }

   obj = get_obj_carry(ch, arg);

   if (obj != NULL)
   {
      act("You junk $p", ch, obj, NULL, TO_CHAR);
      act("$n junks $p", ch, obj, NULL, TO_ROOM);
      extract_obj(obj);
   }
   else
      send_to_char("You can't find that in your inventory.\n\r", ch);
}

void do_quaff(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;

   one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      send_to_char("Quaff what?\n\r", ch);
      return;
   }

   if ((obj = get_obj_carry(ch, arg)) == NULL)
   {
      send_to_char("You do not have that potion.\n\r", ch);
      return;
   }

   if (obj->item_type != ITEM_POTION)
   {
      send_to_char("You can quaff only potions.\n\r", ch);
      return;
   }

   act("$n quaffs $p.", ch, obj, NULL, TO_ROOM);
   act("You quaff $p.", ch, obj, NULL, TO_CHAR);
   obj_cast_spell(obj->value[1], obj->value[0], ch, ch, NULL);
   obj_cast_spell(obj->value[2], obj->value[0], ch, ch, NULL);
   obj_cast_spell(obj->value[3], obj->value[0], ch, ch, NULL);
   extract_obj(obj);
   return;
}

void do_recite(CHAR_DATA *ch, char *argument)
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *scroll;
   OBJ_DATA *obj;

   argument = one_argument(argument, arg1);
   argument = one_argument(argument, arg2);
   if ((scroll = get_obj_carry(ch, arg1)) == NULL)
   {
      send_to_char("You do not have that scroll.\n\r", ch);
      return;
   }

   if (scroll->item_type != ITEM_SCROLL)
   {
      send_to_char("You can recite only scrolls.\n\r", ch);
      return;
   }

   obj = NULL;
   if (arg2[0] == '\0')
   {
      victim = ch;
   }
   else
   {
      if ((victim = get_char_room(ch, arg2)) == NULL && (obj = get_obj_here(ch, arg2)) == NULL)
      {
         send_to_char("You can't find it.\n\r", ch);
         return;
      }
   }

   act("$n recites $p.", ch, scroll, NULL, TO_ROOM);
   act("You recite $p.", ch, scroll, NULL, TO_CHAR);
   obj_cast_spell(scroll->value[1], scroll->value[0], ch, victim, obj);
   obj_cast_spell(scroll->value[2], scroll->value[0], ch, victim, obj);
   obj_cast_spell(scroll->value[3], scroll->value[0], ch, victim, obj);
   extract_obj(scroll);
   return;
}

void do_brandish(CHAR_DATA *ch, char *argument)
{
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   OBJ_DATA *staff;
   int sn;

   if (((staff = get_eq_char(ch, WEAR_HOLD_HAND_L)) == NULL) || (staff->item_type != ITEM_STAFF))
      if (((staff = get_eq_char(ch, WEAR_HOLD_HAND_R)) == NULL) || (staff->item_type != ITEM_STAFF))
      {
         send_to_char("You are not holding a staff!\n\r", ch);
         return;
      }

   if ((sn = staff->value[3]) < 0 || sn >= MAX_SKILL || skill_table[sn].spell_fun == 0)
   {
      bugf("Do_brandish: bad sn %d on object vnum %d", sn, staff->pIndexData->vnum);
      return;
   }

   WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
   if (staff->value[2] > 0)
   {
      act("$n brandishes $p.", ch, staff, NULL, TO_ROOM);
      act("You brandish $p.", ch, staff, NULL, TO_CHAR);
      CREF(vch_next, CHAR_NEXTROOM);
      for (vch = ch->in_room->first_person; vch; vch = vch_next)
      {
         vch_next = vch->next_in_room;
         switch (skill_table[sn].target)
         {
         default:
            bug("Do_brandish: bad target for sn %d.", sn);
            return;
         case TAR_IGNORE:
            if (vch != ch)
               continue;
            break;
         case TAR_CHAR_OFFENSIVE:
            if (IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch) ? TRUE : vch->master != NULL ? TRUE : FALSE)
               continue;
            break;
         case TAR_CHAR_DEFENSIVE:
            if (IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch))
               continue;
            break;
         case TAR_CHAR_SELF:
            if (vch != ch)
               continue;
            break;
         }

         obj_cast_spell(staff->value[3], staff->value[0], ch, vch, staff);
      }
      char_unreference(&vch_next);
   }

   if (--staff->value[2] <= 0)
   {
      act("$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM);
      act("Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR);
      extract_obj(staff);
   }

   return;
}

/*void do_zap( CHAR_DATA * ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *wand;
   OBJ_DATA *obj;

   one_argument( argument, arg );
   if( arg[0] == '\0' && !is_fighting(ch) )
   {
      send_to_char( "Zap whom or what?\n\r", ch );
      return;
   }

   if( ( ( wand = get_eq_char( ch, WEAR_HOLD_HAND_L ) ) == NULL ) || ( wand->item_type != ITEM_WAND
) ) if( ( ( wand = get_eq_char( ch, WEAR_HOLD_HAND_R ) ) == NULL ) || ( wand->item_type != ITEM_WAND
) )
      {
         send_to_char( "You are not holding a wand!\n\r", ch );
         return;
      }



   obj = NULL;
   if( arg[0] == '\0' )
   {
      if( is_fighting(ch) )
      {
         victim = ch->fighting;
      }
      else
      {
         send_to_char( "Zap whom or what?\n\r", ch );
         return;
      }
   }
   else
   {
      if( ( victim = get_char_room( ch, arg ) ) == NULL && ( obj = get_obj_here( ch, arg ) ) == NULL
)
      {
         send_to_char( "You can't find it.\n\r", ch );
         return;
      }
   }

   WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
   if( wand->value[2] > 0 )
   {
      if( victim != NULL )
      {
         act( "$n zaps $N with $p.", ch, wand, victim, TO_ROOM );
         act( "You zap $N with $p.", ch, wand, victim, TO_CHAR );
      }
      else
      {
         act( "$n zaps $P with $p.", ch, wand, obj, TO_ROOM );
         act( "You zap $P with $p.", ch, wand, obj, TO_CHAR );
      }

      obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
   }

   if( --wand->value[2] <= 0 )
   {
      act( "$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM );
      act( "Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR );
      extract_obj( wand );
   }

   return;
}*/

void do_steal(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   int chance = 0;
   /*
    * do not allow steal in LIMBO
    */

   if (!can_use_skill_message(ch, gsn_steal))
      return;

   if (ch->in_room == get_room_index(ROOM_VNUM_LIMBO))
   {
      send_to_char("You failed.\n\r", ch);
      return;
   }

   argument = one_argument(argument, arg1);
   argument = one_argument(argument, arg2);
   if (arg1[0] == '\0' || arg2[0] == '\0')
   {
      send_to_char("Steal what from whom?\n\r", ch);
      return;
   }

   if ((victim = get_char_room(ch, arg2)) == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
      return;
   }

   if (victim == ch)
   {
      send_to_char("That's pointless.\n\r", ch);
      return;
   }

   if (!IS_NPC(ch) && !IS_NPC(victim))
   {
      send_to_char("PCs can't do that to PCs\n\r", ch);
      return;
   }

   WAIT_STATE(ch, skill_table[gsn_steal].beats);
   chance = IS_NPC(ch) ? (get_psuedo_level(ch) / 4) : ((get_curr_dex(ch) * 3));
   chance = chance - ((get_psuedo_level(victim) - get_psuedo_level(ch)) / 2);
   if ((!IS_NPC(ch) && !IS_NPC(victim)) && (victim->adept_level > 0) && (ch->adept_level <= 0))
      chance = chance - 25;
   if (!IS_NPC(ch))
      chance += ch->class_level[CLASS_ASS] / 4;
   if (get_psuedo_level(ch) > (get_psuedo_level(victim) + 30))
   {
      send_to_char("Coward!!! Trying to steal from the weak..\n\r", ch);
      return;
   }
   if ((victim->position == POS_FIGHTING) || (number_percent() > chance))
   {
      /*
       * Failure.
       */
      send_to_char("Oops.\n\r", ch);
      act("$n tried to steal from you.\n\r", ch, NULL, victim, TO_VICT);
      act("$n tried to steal from $N.\n\r", ch, NULL, victim, TO_NOTVICT);
      if (!IS_NPC(ch) && !IS_NPC(victim) && IS_SET(ch->pcdata->pflags, PFLAG_PKOK) &&
          IS_SET(victim->pcdata->pflags, PFLAG_PKOK))
         return;
      sprintf(buf, "%s is a bloody thief!", ch->name);
      do_yell(victim, buf);
      if (!IS_NPC(ch))
      {
         if (IS_NPC(victim) && (victim->pIndexData->pShop == NULL))
         {
            act("$n says 'I'll get you for that, $N!'", victim, NULL, ch, TO_ROOM);
            multi_hit(victim, ch, TYPE_UNDEFINED);
         }
         else
         {
            int diff = 0;
            diff = (abs(get_psuedo_level(ch) - get_psuedo_level(victim)) + 10) * 20;
            SET_BIT(ch->act, PLR_THIEF);
            send_to_char("*** You are now a THIEF!! ***\n\r", ch);
            ch->sentence += diff;
            save_char_obj(ch);
            log_string(buf);
         }
      }
      return;
   }

   if (!str_cmp(arg1, "coin") || !str_cmp(arg1, "coins") || !str_cmp(arg1, "gold"))
   {
      int amount = 0;
      amount = victim->gold * number_range(1, 10) / 100;
      if (amount <= 0)
      {
         send_to_char("You couldn't get any gold.\n\r", ch);
         return;
      }

      ch->gold += amount;
      victim->gold -= amount;
      sprintf(buf, "Bingo!  You got %d gold coins.\n\r", amount);
      send_to_char(buf, ch);
      return;
   }

   if ((obj = get_obj_carry(victim, arg1)) == NULL)
   {
      send_to_char("You can't find it.\n\r", ch);
      return;
   }

   if (!can_drop_obj(ch, obj) || IS_SET(obj->extra_flags, ITEM_INVENTORY) ||
       obj->level > ch->level + 10 || (obj->wear_loc > -1))
   {
      send_to_char("You can't pry it away.\n\r", ch);
      return;
   }

   if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
   {
      send_to_char("You have your hands full.\n\r", ch);
      return;
   }

   if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch))
   {
      send_to_char("You can't carry that much weight.\n\r", ch);
      return;
   }

   obj_from_char(obj);
   obj_to_char(obj, ch);
   send_to_char("Yes!  Got it!!!\n\r", ch);
   return;
}

/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper(CHAR_DATA *ch)
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *keeper;
   SHOP_DATA *pShop;
   pShop = NULL;
   for (keeper = ch->in_room->first_person; keeper; keeper = keeper->next_in_room)
   {
      if (IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL)
         break;
   }

   if (pShop == NULL || IS_AFFECTED(keeper, AFF_CHARM))
   {
      send_to_char("You can't do that here.\n\r", ch);
      return NULL;
   }

   /*
    * Undesirables.
    */

   if (!IS_NPC(ch) && IS_SET(ch->act, PLR_KILLER))
   {
      do_say(keeper, "Killers are not welcome!");
      sprintf(buf, "%s the KILLER is over here!", ch->name);
      do_shout(keeper, buf);
      check_guards(ch);
      return NULL;
   }

   if (!IS_NPC(ch) && IS_SET(ch->act, PLR_THIEF))
   {
      do_say(keeper, "Thieves are not welcome!");
      sprintf(buf, "%s the THIEF is over here!", ch->name);
      do_shout(keeper, buf);
      check_guards(ch);
      return NULL;
   }

   /*
    * Shop hours.
    */
   if (time_info.hour < pShop->open_hour)
   {
      do_say(keeper, "Sorry, come back later.");
      return NULL;
   }

   if (time_info.hour > pShop->close_hour)
   {
      do_say(keeper, "Sorry, come back tomorrow.");
      return NULL;
   }

   /*
    * Invisible or hidden people.
    */
   if (!can_see(keeper, ch))
   {
      do_say(keeper, "I don't trade with folks I can't see.");
      return NULL;
   }
   if (!can_see(ch, keeper))
   {
      send_to_char("You don't seem to see anyone around.\n\r", ch);
      return NULL;
   }
   return keeper;
}

/* Called when a shopkeeper spots a wanted player...
 * calls local mobs with spec_policeman set to hunt player
 * --Stephen
 */

void check_guards(CHAR_DATA *ch)
{
   CHAR_DATA *guard;
   char buf[MAX_STRING_LENGTH];
   for (guard = first_char; guard != NULL; guard = guard->next)
      if (IS_NPC(guard) && (guard->in_room->area == ch->in_room->area) && guard->spec_fun != 0 &&
          !str_cmp("spec_policeman", rev_spec_lookup(guard->spec_fun)) && guard->hunting == NULL)
      {
         if (set_hunt(guard, NULL, ch, NULL, 0, HUNT_INFORM | HUNT_MERC | HUNT_CR))
         {
            /*	     if ( make_hunt( guard, ch ) )
                    {
                      REMOVE_BIT( guard->act_hunt, ACT_HUNT_INFORM );*/
            switch (number_range(0, 7))
            {
            case 0:
               do_yell(guard, "I'm coming!");
               break;
            case 1:
               do_yell(guard, "On my way!");
               break;
            case 2:
               do_yell(guard, "Watch out buster, here i come!");
               break;
            case 3:
               do_yell(guard, "Come make my day punk!");
               interpret(guard, "grin");
               break;
            case 4:
               sprintf(buf, "leaves in search of %s.", ch->name);
               do_emote(guard, buf);
               break;
            case 5:
               interpret(guard, "wave");
               sprintf(buf, "Laters... i'm off to get %s.", ch->name);
               do_say(guard, buf);
               break;
            case 6:
               interpret(guard, "bounce");
               do_say(guard, "Yes!  Someone to go kill!");
               break;
            case 7:
               sprintf(buf, "%s Watch it mate... i'm after you!", ch->name);
               do_tell(guard, buf);
               break;
            }
         }
      }
   return;
}

int get_cost(CHAR_DATA *keeper, OBJ_DATA *obj)
{
   SHOP_DATA *pShop;
   int cost;
   if (obj == NULL || (pShop = keeper->pIndexData->pShop) == NULL)
      return 0;
   {
      int itype;
      cost = 0;
      for (itype = 0; itype < MAX_TRADE; itype++)
      {
         if (obj->item_type == pShop->buy_type[itype])
         {
            cost = obj->cost * pShop->profit_sell / 100;
            break;
         }
      }
   }
   return cost;
}

void do_buy(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *keeper;

   keeper = find_quartermaster(ch);
   if (keeper != NULL)
   {
      const QUARTERMASTER_EMBLEM_OPTION_TYPE *option;
      OBJ_DATA *obj;

      argument = one_argument(argument, arg);
      argument = one_argument(argument, arg2);

      if (arg[0] == '\0' || arg2[0] == '\0')
      {
         send_to_char("Buy what? Syntax: buy <tier> <style>.\n\r", ch);
         return;
      }

      option = find_quartermaster_emblem_option(arg, arg2);
      if (option == NULL)
      {
         send_to_char("Quartermaster offers: common/magic/rare with caster/melee/tank styles.\n\r",
                      ch);
         return;
      }

      if (!quartermaster_can_afford_option(ch, option))
      {
         send_to_char(
             "You do not have enough invasion points or invasion rewards for that emblem.\n\r", ch);
         return;
      }

      if (ch->carry_number + 1 > can_carry_n(ch))
      {
         send_to_char("You can't carry that many items.\n\r", ch);
         return;
      }

      obj = create_quartermaster_emblem(ch, option);
      if (obj == NULL)
      {
         send_to_char(
             "The quartermaster frowns and says: 'I cannot prepare that emblem right now.'\n\r",
             ch);
         return;
      }

      if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch))
      {
         extract_obj(obj);
         send_to_char("You can't carry that much weight.\n\r", ch);
         return;
      }

      ch->pcdata->invasion_points -= option->invasion_points;
      ch->pcdata->invasion_rewards[0] -= option->commendations;
      ch->pcdata->invasion_rewards[1] -= option->ribbons;
      ch->pcdata->invasion_rewards[2] -= option->medals;

      obj_to_char(obj, ch);
      act("$n purchases $p from the quartermaster.", ch, obj, NULL, TO_ROOM);
      act("You purchase $p.", ch, obj, NULL, TO_CHAR);
      return;
   }

   argument = one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      send_to_char("Buy what?\n\r", ch);
      return;
   }

   if ((keeper = find_keeper(ch)) == NULL)
      return;

   if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP))
   {
      char buf[MAX_STRING_LENGTH];
      CHAR_DATA *pet;
      ROOM_INDEX_DATA *pRoomIndexNext;
      ROOM_INDEX_DATA *in_room;
      if (IS_NPC(ch))
         return;
      pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
      if (pRoomIndexNext == NULL)
      {
         bug("Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum);
         send_to_char("Sorry, you can't buy that here.\n\r", ch);
         return;
      }

      in_room = ch->in_room;
      ch->in_room = pRoomIndexNext;
      pet = get_char_room(ch, arg);
      ch->in_room = in_room;
      if (pet == NULL || !IS_SET(pet->act, ACT_PET))
      {
         send_to_char("Sorry, you can't buy that here.\n\r", ch);
         return;
      }

      if (IS_SET(ch->act, PLR_BOUGHT_PET))
      {
         send_to_char("You already bought one pet this level.\n\r", ch);
         return;
      }

      if (ch->gold < 10 * pet->level * pet->level)
      {
         send_to_char("You can't afford it.\n\r", ch);
         return;
      }

      if (ch->level < pet->level)
      {
         send_to_char("You're not ready for this pet.\n\r", ch);
         return;
      }

      ch->gold -= 10 * pet->level * pet->level;
      {
         do_say(keeper, "Thank you very much for your business!");
      }

      pet = create_mobile(pet->pIndexData);
      SET_BIT(ch->act, PLR_BOUGHT_PET);
      SET_BIT(pet->act, ACT_PET);
      SET_BIT(pet->affected_by, AFF_CHARM);
      argument = one_argument(argument, arg);
      if (arg[0] != '\0')
      {
         sprintf(buf, "%s %s", pet->name, arg);
         free_string(pet->name);
         pet->name = str_dup(buf);
      }

      sprintf(buf, "%sA neck tag says 'I belong to %s'.\n\r", pet->description, ch->name);
      free_string(pet->description);
      pet->description = str_dup(buf);
      char_to_room(pet, ch->in_room);
      add_follower(pet, ch);
      send_to_char("Enjoy your pet.\n\r", ch);
      act("$n bought $N as a pet.", ch, NULL, pet, TO_ROOM);
      return;
   }
   else
   {

      OBJ_DATA *obj;
      int cost;
      obj = get_obj_carry(keeper, arg);
      cost = get_cost(keeper, obj);
      if (cost <= 0 || !can_see_obj(ch, obj))
      {
         act("$n tells you 'I don't sell that -- try 'list''.", keeper, NULL, ch, TO_VICT);
         ch->reply = keeper;
         return;
      }

      if (ch->gold < cost)
      {
         act("$n tells you 'You can't afford to buy $p'.", keeper, obj, ch, TO_VICT);
         ch->reply = keeper;
         return;
      }

      if (obj->level > get_psuedo_level(ch))
      {
         act("$n tells you 'You can't use $p yet'.", keeper, obj, ch, TO_VICT);
         ch->reply = keeper;
         return;
      }

      if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
      {
         send_to_char("You can't carry that many items.\n\r", ch);
         return;
      }

      if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch))
      {
         send_to_char("You can't carry that much weight.\n\r", ch);
         return;
      }

      ch->gold -= cost;
      keeper->gold += cost;
      act("$n buys $p.", ch, obj, NULL, TO_ROOM);
      act("You buy $p.", ch, obj, NULL, TO_CHAR);
      {
         do_say(keeper, "Thank you very much for your business!");
      }

      if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
         obj = create_object(obj->pIndexData, obj->level);
      else
         obj_from_char(obj);
      obj_to_char(obj, ch);
      return;
   }
}

void do_list(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char buf1[MAX_STRING_LENGTH];
   int stopcounter = 0;
   bool found;
   int rounded_cost;
   char costbuf[MSL];
   CHAR_DATA *keeper;
   int cost;
   buf1[0] = '\0';

   keeper = find_quartermaster(ch);
   if (keeper != NULL)
   {
      show_quartermaster_list(ch);
      return;
   }

   if ((keeper = find_keeper(ch)) == NULL)
      return;
   found = FALSE;

   if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP))
   {
      ROOM_INDEX_DATA *pRoomIndexNext;
      CHAR_DATA *pet;
      bool found;
      pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
      if (pRoomIndexNext == NULL)
      {
         bug("Do_list: bad pet shop at vnum %d.", ch->in_room->vnum);
         send_to_char("You can't do that here.\n\r", ch);
         return;
      }

      found = FALSE;
      for (pet = pRoomIndexNext->first_person; pet; pet = pet->next_in_room)
      {
         if (IS_SET(pet->act, ACT_PET))
         {
            if (!found)
            {
               found = TRUE;
               safe_strcat(MAX_STRING_LENGTH, buf1,
                           "\n\r@@g[@@yLevel@@g]       @@yPet@@g                           "
                           "@@yPrice  ( Approximate )@@N \n\r");
            }
            stopcounter++;
            rounded_cost = 10 * pet->level * pet->level;
            sprintf(costbuf, "%d", rounded_cost);
            snprintf(buf, sizeof(buf), "[ @@W%3d@@g]  @@c%-*.*s@@g  @@W%.32s@@N \n\r", pet->level,
                     ccode_len(pet->short_descr, 30), 200, capitalize(pet->short_descr), costbuf);
            safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            if (stopcounter > 45)
            {
               send_to_char(buf1, ch);
               sprintf(buf1, "%s", "");
               stopcounter = 0;
            }
         }
      }
      if (!found)
         send_to_char("Sorry, we're out of pets right now.\n\r", ch);
      send_to_char(buf1, ch);
      return;
   }
   else
   {
      char arg[MAX_INPUT_LENGTH];
      OBJ_DATA *obj;
      one_argument(argument, arg);
      for (obj = keeper->first_carry; obj; obj = obj->next_in_carry_list)
      {
         if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj) &&
             (cost = get_cost(keeper, obj)) > 0 && (arg[0] == '\0' || is_name(arg, obj->name)))
         {
            if (!found)
            {
               found = TRUE;
               safe_strcat(MAX_STRING_LENGTH, buf1,
                           "\n\r@@g[@@yLvl@@g]       @@yItem@@g                           "
                           "@@yPrice@@N \n\r");
            }
            stopcounter++;
            sprintf(buf, "@@g[%s%3d@@g]  @@c%-*s@@g      @@W%d@@N\n\r", "@@a", obj->level,
                    ccode_len(obj->short_descr, 30), capitalize(obj->short_descr), cost);
            safe_strcat(MAX_STRING_LENGTH, buf1, buf);
            if (stopcounter > 45)
            {
               send_to_char(buf1, ch);
               sprintf(buf1, "%s", "");
               stopcounter = 0;
            }
         }
      }

      if (!found)
      {
         if (arg[0] == '\0')
            send_to_char("You can't buy anything here.\n\r", ch);
         else
            send_to_char("You can't buy that here.\n\r", ch);
         return;
      }

      send_to_char(buf1, ch);
      send_to_char("@@mMagenta@@N Level numbers indicate remort items.\n\rYou may also type list "
                   "<keyword>.\n\r",
                   ch);
      return;
   }
}

void do_value(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *keeper;
   OBJ_DATA *obj;
   int cost;
   one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      send_to_char("Value what?\n\r", ch);
      return;
   }

   if ((keeper = find_keeper(ch)) == NULL)
      return;

   if ((obj = get_obj_carry(ch, arg)) == NULL)
   {
      act("$n tells you 'You don't have that item'.", keeper, NULL, ch, TO_VICT);
      ch->reply = keeper;
      return;
   }

   if (!can_drop_obj(ch, obj))
   {
      send_to_char("You can't let go of it.\n\r", ch);
      return;
   }

   if ((cost = get_cost(keeper, obj)) <= 0)
   {
      act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT);
      return;
   }
   sprintf(buf, "$n tells you 'I'll give you %d gold coins for $p'.", cost);
   act(buf, keeper, obj, ch, TO_VICT);
   ch->reply = keeper;
   return;
}

void do_donate(CHAR_DATA *ch, char *argument)
/*  This function removes *one* object from a player,
 *  and instead of dropping it in the curent room, it is
 *  placed in the donation room, vnum 3206
 *  (I have donation room one east of temple)
 *  Eg: donate excalibur
 *  Donate all was not implemented to avoid unwanted eq loss :)
 *  Money may NOT be denoted - risk of too much lying around!!
 *  Three donation rooms now, used to hold armor, weapons and misc.
 *  --  Stephen
 */
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int place_to_put_it = ROOM_VNUM_MISC_DONATE;
   ROOM_INDEX_DATA *room = NULL;
   argument = one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("Donate what?\n\r", ch);
      return;
   }

   /*
    * Donating coins is NOT allowed!
    */

   if (is_number(arg))
   {
      send_to_char("Sorry, that is not possible - yet?!\n\r", ch);
      return;
   }

   /*
    * Donate all will not be allowed!
    */

   if (!str_cmp(arg, "all") || !str_prefix("all.", arg))
   {
      send_to_char("Donating *all* sounds dodgy to me!\n\r", ch);
      return;
   }

   /*
    * Check to see if item in inventory :)
    */

   if ((obj = get_obj_carry(ch, arg)) == NULL)
   {
      send_to_char("You do not have that item to donate.\n\r", ch);
      return;
   }

   /*
    * Check to see if item *can* be dropped!
    */

   if (!can_drop_obj(ch, obj))
   {
      send_to_char("You can't let go of it.\n\r", ch);
      return;
   }
   if (obj->item_type == ITEM_BEACON)
   {
      send_to_char(" @@eNice try:P@@N\n\r", ch);
      return;
   }

   /*
    * Remove object from inventory and put into donation room
    */

   if (obj->item_type == ITEM_WEAPON)
      place_to_put_it = ROOM_VNUM_WEAPON_DONATE;
   if (obj->item_type == ITEM_ARMOR)
      place_to_put_it = ROOM_VNUM_ARMOR_DONATE;
   if ((room = get_room_index(place_to_put_it)) == NULL)
   {
      sprintf(log_buf, "No valid donation room: %d", place_to_put_it);
      log_f("%s", log_buf);
      send_to_char("System Error.  Contact an Administrator.\n\r", ch);
      return;
   }
   act("$n donates $p, how kind.", ch, obj, NULL, TO_ROOM);
   act("You donate $p, thank you.", ch, obj, NULL, TO_CHAR);
   if (count_obj_room(obj->pIndexData, room->first_content) < 10)
   {
      obj_from_char(obj);
      obj_to_room(obj, room);
   }
   else
   {
      extract_obj(obj);
   }
   do_save(ch, "");
}

void do_adapt(CHAR_DATA *ch, char *argument)
{
   /*
    * Take one piece of eq, the ch's level, and reset the eq's stats
    * * and level req so that they can use it, but the eq won't be as
    * * good as it was previously...  Only do if ch->level > eq->level!
    * * Added: descrips changed to reflect adaptation.
    * * -- Stephen
    */

   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   AFFECT_DATA *paf;
   CHAR_DATA *mob;
   bool changed; /* was the eq changed?? */
   long diff;    /* ratio to change stuff by */
   int pen;      /* additional penalty */
   int cost;

   cost = (ch->level * 250);
   argument = one_argument(argument, arg);
   /*
    * Check for mob with act->adapt
    */
   for (mob = ch->in_room->first_person; mob; mob = mob->next_in_room)
   {
      if (IS_NPC(mob) && IS_SET(mob->act, ACT_ADAPT))
         break;
   }

   if (mob == NULL)
   {
      send_to_char("You can't do that here.\n\r", ch);
      return;
   }

   if (arg[0] == '\0')
   {
      sprintf(buf, "The cost for you to have a weapon adapted is: %d GP.\n\r", cost);
      send_to_char(buf, ch);
      send_to_char("Usuage: ADAPT <weapon>.  The weapon must be in your inventory.\n\r", ch);
      return;
   }

   if (!str_cmp(arg, "all") || !str_prefix("all.", arg))
   {
      send_to_char("Only one weapon can be adapted at a time.....\n\r", ch);
      return;
   }

   /*
    * Check to see if item in inventory :)
    */

   if ((obj = get_obj_carry(ch, arg)) == NULL)
   {
      send_to_char("You do not have that item to be adapted.\n\r", ch);
      return;
   }

   diff = (obj->level / ch->level);
   pen = (obj->level - ch->level);
   if (diff < 1 || pen == 0)
   {
      send_to_char("It doesn't need adapting!\n\r", ch);
      return;
   }

   if (pen == 1)
   {
      send_to_char("Just wait until you gain another level!\n\r", ch);
      return;
   }
   pen *= 2; /* Double difference in levels as penalty. */
   if (ch->gold < cost)
   {
      send_to_char("You can't afford to have it adapted!\n\r", ch);
      return;
   }

   /*
    * Now, see if the piece of eq CAN be adapted.....
    */

   switch (obj->item_type)
   {
   default:
      changed = FALSE;
      break;
   case ITEM_WEAPON:
      obj->value[1] /= diff; /* Reduce the damage the weapon does */
      obj->value[1] = UMAX(0, obj->value[1] - pen);
      obj->value[2] /= diff;
      obj->value[2] = UMAX(0, obj->value[2] - pen);
      changed = TRUE;
      if (obj->value[2] == 0) /* Then weapon is screwed */
      {
         send_to_char("The weapon crumbles into dust...It was broken!\n\r", ch);
         extract_obj(obj);
         changed = FALSE;
      }
      break;
   }

   if (!changed)
   {
      send_to_char("You can only have *weapons* adapted!\n\r", ch);
      return;
   }

   /*
    * Tone down any affects that apply to the object, if applicable
    */

   for (paf = obj->first_apply; paf != NULL; paf = paf->next)
   {
      if (paf->location != APPLY_NONE && paf->modifier != 0)
      {
         if (paf->modifier > 0)
            paf->modifier = UMAX(0, (paf->modifier / (diff * 2) - pen));
         else
            paf->modifier = UMIN(0, (paf->modifier / (diff * 2) - pen));
      }
   }

   ch->gold -= cost;       /* Take the cost from char's gold. */
   obj->level = ch->level; /* Allow ch to use the eq... */
   sprintf(buf, "%s <adapted>", obj->short_descr);
   free_string(obj->short_descr);
   obj->short_descr = str_dup(buf);
   sprintf(buf, "<adapted> %s", obj->description);
   free_string(obj->description);
   obj->description = str_dup(buf);
   send_to_char("Your weapon is now adapted.\n\r", ch);
   return;
}

void do_cdonate(CHAR_DATA *ch, char *argument)
/*  This function removes *one* object from a player,
 *  and instead of dropping it in the curent room, it is
 *  placed in the donation room.
 *  (I have donation room one east of temple)
 *  Eg: donate excalibur
 *  Donate all was not implemented to avoid unwanted eq loss :)
 *  Money may NOT be denoted - risk of too much lying around!!
 *  Three donation rooms now, used to hold armor, weapons and misc.
 *  --  Stephen
 * This just a copy of donate, except it works for clans -S-
 */
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int place_to_put_it;
   ROOM_INDEX_DATA *room;

   argument = one_argument(argument, arg);
   if (IS_NPC(ch))
   {
      send_to_char("Mobs don't have clans.\n\r", ch);
      return;
   }

   if (ch->pcdata->clan == 0)
   {
      send_to_char("You must be in a clan to use this command!\n\r", ch);
      return;
   }

   if (arg[0] == '\0')
   {
      send_to_char("Donate what?\n\r", ch);
      return;
   }

   /*
    * Donating coins is NOT allowed!
    */

   if (is_number(arg))
   {
      send_to_char("Sorry, that is not possible - yet?!\n\r", ch);
      return;
   }

   /*
    * Donate all will not be allowed!
    */

   if (!str_cmp(arg, "all") || !str_prefix("all.", arg))
   {
      send_to_char("Donating *all* sounds dodgy to me!\n\r", ch);
      return;
   }

   /*
    * Check to see if item in inventory :)
    */

   if ((obj = get_obj_carry(ch, arg)) == NULL)
   {
      send_to_char("You do not have that item to donate.\n\r", ch);
      return;
   }

   /*
    * Check to see if item *can* be dropped!
    */

   if (!can_drop_obj(ch, obj))
   {
      send_to_char("You can't let go of it.\n\r", ch);
      return;
   }
   if (obj->item_type == ITEM_BEACON)
   {
      send_to_char(" @@eNice try:P@@N\n\r", ch);
      return;
   }

   /*
    * Remove object from inventory and put into donation room
    */

   place_to_put_it = clan_table[ch->pcdata->clan].donat_room;
   if ((room = get_room_index(place_to_put_it)) == NULL)
   {
      sprintf(log_buf, "Cdonate: no valid room for vnum %d", place_to_put_it);
      log_f("%s", log_buf);
      send_to_char("Invalid Clan Donation Room.  Please contact an Administrator.\n\r", ch);
      return;
   }
   act("$n clan-donates $p, how kind.", ch, obj, NULL, TO_ROOM);
   act("You clan-donate $p, thank you.", ch, obj, NULL, TO_CHAR);
   if (count_obj_room(obj->pIndexData, room->first_content) < 10)
   {
      obj_from_char(obj);
      obj_to_room(obj, room);
   }
   else
   {
      extract_obj(obj);
   }
   do_save(ch, "");
   return;
}

void do_appraise(CHAR_DATA *ch, char *argument)
{
   OBJ_DATA *obj;
   void *vo;

   if (argument[0] == '\0')
   {
      send_to_char("Appraise what?\n\r", ch);
      return;
   }

   if ((obj = get_obj_here(ch, argument)) == NULL)
   {
      send_to_char("It would help if you tried to appraise an object!\n\r", ch);
      return;
   }

   vo = (void *)obj;
   spell_identify(0, LEVEL_HERO - 1, ch, vo, obj);
   return;
}

void do_bid(CHAR_DATA *ch, char *argument)
{
   int amount = 0;

   if (IS_NPC(ch))
   {
      send_to_char("Only players may use the auction system.\n\r", ch);
      return;
   }

   if (auction_item == NULL)
   {
      send_to_char("Nothing is being auctioned right now!\n\r", ch);
      return;
   }

   if (amount <= auction_bid)
   {
      send_to_char("That doesn't beat the latest bid.\n\r", ch);
      return;
   }

   if (amount > ch->gold)
   // This fix provided by Coldmyst
   {
      send_to_char("You don't have that much money.\n\r", ch);
      return;
   }

   if (auction_bidder == ch)
   {
      send_to_char("You made the last bid, silly!\n\r", ch);
      return;
   }

   if (auction_owner == ch)
   {
      send_to_char("You can't bid for your own item!!\n\r", ch);
      return;
   }

   /*
    * Ok, so now set the variables to reflect the new bidder
    */
   /*
    * refund gold to last bidder, take gold from new one
    */

   auction_bidder = ch;
   auction_bid = amount;
   auction_stage = 0;
   auction_update();
   auction_flop = TRUE;
   return;
}

void do_auction(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_STRING_LENGTH];
   int reserve;
   void *vo = NULL;
   if (IS_NPC(ch))
      return;

   argument = one_argument(argument, arg);
   if ((IS_STAFF(ch)) && ((!str_cmp(arg, "stop")) || (!str_cmp(arg, "take"))))
      if (auction_item != NULL)
      {
         CHAR_DATA *ach;
         bool good_seller = FALSE;
         bool good_buyer = FALSE;
         for (ach = first_char; ach != NULL; ach = ach->next)
         {
            if (auction_owner == ach)
               good_seller = TRUE;
            if (auction_bidder == ach)
               good_buyer = TRUE;
         }

         sprintf(buf, "The auction of %s has been stopped by @@mstaff@@N.\n\r",
                 auction_item->short_descr);
         if (!str_cmp(arg, "take"))
         {
            char buf2[MSL];
            sprintf(buf2, " The item has been taken by %s.\n\r", ch->name);
            safe_strcat(MSL, buf, buf2);
         }
         do_echo(ch, buf);
         if (good_seller)
         {
            if (!str_cmp(arg, "take"))
            {
               obj_to_char(auction_item, ch);
               send_to_char("Your item has been claimed by the gods.\n\r", auction_owner);
            }
            else
            {
               obj_to_char(auction_item, auction_owner);
               send_to_char("Your item has been returned.\n\r ", auction_owner);
            }
         }
         else
         {

            obj_to_char(auction_item, ch);
         }

         if (good_buyer)
         {
            send_to_char("Your bid has been returned.\n\r", auction_bidder);
         }

         auction_stage = 0;
         auction_bidder = NULL;
         auction_owner = NULL;
         auction_item = NULL;
         auction_reserve = 0;
         auction_bid = 0;
         return;
      }

   if (auction_item != NULL)
   {
      sprintf(buf, "The current object being auctioned is: %s\n\r", auction_item->short_descr);
      send_to_char(buf, ch);
      sprintf(buf, "Item was offered for sale by %s.\n\r", auction_owner->name);
      send_to_char(buf, ch);
      sprintf(buf, "The estimated value is %d, and last bid was for %d.\n\r", auction_item->cost,
              auction_bid);
      send_to_char(buf, ch);
      sprintf(buf, "The reserve price is @@W%d @@N.\n\r", auction_reserve);
      send_to_char(buf, ch);
      vo = (void *)auction_item;
      spell_identify(0, LEVEL_HERO - 1, ch, vo, auction_item);
      return;
   }

   if (arg[0] == '\0' || argument[0] == '\0')
   {
      send_to_char("USAGE:  SYNTAX <object>  [reserve_price]\n\r", ch);
      return;
   }

   if (argument[0] != '\0')
      reserve = atoi(argument);
   else
      reserve = 0;
   if (reserve * .1 > ch->gold)
   {
      send_to_char("You don't have enough gold to cover the auction fee at that reserve price!\n\r",
                   ch);
      return;
   }

   if ((auction_item = get_obj_carry(ch, arg)) == NULL)
   {
      send_to_char("You're not carrying that item!\n\r", ch);
      return;
   }

   if (IS_SET(auction_item->extra_flags, ITEM_NO_AUCTION))
   {
      send_to_char("You can't auction that.  Sorry!\n\r", ch);
      auction_item = NULL;
      return;
   }

   switch (auction_item->item_type)
   {
   case ITEM_CORPSE_PC:
   case ITEM_CORPSE_NPC:
      send_to_char("You can't auction corpses.\n\r", ch);
      auction_item = NULL;
      return;
      break;
   case ITEM_TRASH:
      send_to_char("Looks like trash.  Forget it!\n\r", ch);
      auction_item = NULL;
      return;
      break;
   default:
      break;
   }

   if (IS_SET(auction_item->extra_flags, ITEM_NODROP))
   {
      send_to_char("You can't let go of it!\n\r", ch);
      auction_item = NULL;
      return;
   }

   sprintf(buf,
           "You have placed %s up for auction.  %d @@Whas been charged for these services.\n\r",
           auction_item->name, (int)(reserve * 0.1));
   send_to_char(buf, ch);
   auction_owner = ch;
   auction_bidder = NULL;
   auction_bid = 0;
   auction_reserve = reserve;
   if (auction_reserve > 0)
      auction_bid = auction_reserve;
   auction_stage = 0;
   obj_from_char(auction_item);
   auction_update();
   auction_flop = TRUE;
   return;
}

void do_connect(CHAR_DATA *ch, char *argument)
{
   OBJ_DATA *first_ob;
   OBJ_DATA *second_ob;
   OBJ_DATA *new_ob;
   char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];

   argument = one_argument(argument, arg1);
   argument = one_argument(argument, arg2);
   if (arg1[0] == '\0' || arg2[0] == '\0')
   {
      send_to_char("You must connect one piece to another.\n\r", ch);
      return;
   }

   if ((first_ob = get_obj_carry(ch, arg1)) == NULL)
   {
      sprintf(buf, "You aren't carrying that!!\n\r");
      send_to_char(buf, ch);
      return;
   }

   if ((second_ob = get_obj_carry(ch, arg2)) == NULL)
   {
      sprintf(buf, "You aren't carrying that!!\n\r");
      send_to_char(buf, ch);
      return;
   }

   if (first_ob->item_type != ITEM_PIECE || second_ob->item_type != ITEM_PIECE)
   {
      send_to_char("Both items must be pieces of another item!\n\r", ch);
      return;
   }

   /*
    * check to see if the pieces connect
    */

   if ((first_ob->value[0] == second_ob->pIndexData->vnum) ||
       (first_ob->value[1] == second_ob->pIndexData->vnum))
   /*
    * good connection
    */
   {
      new_ob = create_object(get_obj_index(first_ob->value[2]), ch->level);
      extract_obj(first_ob);
      extract_obj(second_ob);
      obj_to_char(new_ob, ch);
      act("$n jiggles some pieces together, and suddenly they snap in place, creating $p! Perfect "
          "Fit!!",
          ch, new_ob, NULL, TO_ROOM);
      act("You jiggle the pieces together, and suddenly they snap into place, creating $p! Nice "
          "job!",
          ch, new_ob, NULL, TO_CHAR);
   }
   else
   {
      act("$n jiggles some pieces together, but can't seem to make them connect.", ch, NULL, NULL,
          TO_ROOM);
      act("You try to fit them together every which way, but they just don't want to fit together.",
          ch, NULL, NULL, TO_CHAR);
      return;
   }

   return;
}
