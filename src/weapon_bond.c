/*
 * weapon_bond.c — Swordsman Weapon Bonding System
 *
 * A swordsman bonds to a sword, creating a soul-linked weapon that
 * auto-scales with their level.  Power budget is set by the original
 * weapon's level at bonding time.  Tracks provide stat multipliers
 * funded by bond points earned through kills.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "weapon_bond.h"

/* Forward declarations for functions defined elsewhere. */
void set_aff_to_obj(OBJ_DATA *obj, int location, int modifier);
void set_obj_stat_auto(OBJ_DATA *obj);

/* ── Lookup tables ────────────────────────────────────────────── */

static const int rank_cost[] = {
    0, BOND_COST_RANK_1, BOND_COST_RANK_2, BOND_COST_RANK_3, BOND_COST_RANK_4, BOND_COST_RANK_5};

static const int rank_mult[] = {BOND_MULT_RANK_1, BOND_MULT_RANK_1, BOND_MULT_RANK_2,
                                BOND_MULT_RANK_3, BOND_MULT_RANK_4, BOND_MULT_RANK_5};

static const int guard_ac_scale[] = {
    0, GUARD_AC_SCALE_1, GUARD_AC_SCALE_2, GUARD_AC_SCALE_3, GUARD_AC_SCALE_4, GUARD_AC_SCALE_5};

static const int guard_parry[] = {
    0, GUARD_PARRY_1, GUARD_PARRY_2, GUARD_PARRY_3, GUARD_PARRY_4, GUARD_PARRY_5};

static const int keen_crit[] = {0, KEEN_CRIT_1, KEEN_CRIT_2, KEEN_CRIT_3, KEEN_CRIT_4, KEEN_CRIT_5};

static const int keen_crit_mult[] = {
    0, KEEN_CRIT_MULT_1, KEEN_CRIT_MULT_2, KEEN_CRIT_MULT_3, KEEN_CRIT_MULT_4, KEEN_CRIT_MULT_5};

/* ── Utility ──────────────────────────────────────────────────── */

const char *bond_track_name(int track_id)
{
   switch (track_id)
   {
   case BOND_TRACK_NONE:
      return "none";
   case BOND_TRACK_EDGE:
      return "edge";
   case BOND_TRACK_GUARD:
      return "guard";
   case BOND_TRACK_KEEN:
      return "keen";
   case BOND_TRACK_SPIRIT:
      return "spirit";
   default:
      return "unknown";
   }
}

int bond_rank_cost(int rank)
{
   if (rank < 1 || rank > BOND_MAX_RANK)
      return 0;
   return rank_cost[rank];
}

int bond_rank_multiplier(int rank)
{
   if (rank < 1)
      return BOND_MULT_RANK_1;
   if (rank > BOND_MAX_RANK)
      return BOND_MULT_RANK_5;
   return rank_mult[rank];
}

int bond_guard_ac(int rank, int base_level)
{
   if (rank < 1)
      rank = 1;
   if (rank > BOND_MAX_RANK)
      rank = BOND_MAX_RANK;
   return -(base_level * guard_ac_scale[rank] / 100);
}

int bond_guard_parry(int rank)
{
   if (rank < 1)
      rank = 1;
   if (rank > BOND_MAX_RANK)
      rank = BOND_MAX_RANK;
   return guard_parry[rank];
}

int bond_keen_crit(int rank)
{
   if (rank < 1)
      rank = 1;
   if (rank > BOND_MAX_RANK)
      rank = BOND_MAX_RANK;
   return keen_crit[rank];
}

int bond_keen_crit_mult(int rank)
{
   if (rank < 1)
      rank = 1;
   if (rank > BOND_MAX_RANK)
      rank = BOND_MAX_RANK;
   return keen_crit_mult[rank];
}

void bond_free(BOND_DATA *bond)
{
   if (bond == NULL)
      return;
   if (bond->weapon_name != NULL)
      free_string(bond->weapon_name);
   if (bond->weapon_short != NULL)
      free_string(bond->weapon_short);
   free(bond);
}

bool is_bonded_weapon(CHAR_DATA *ch, OBJ_DATA *obj)
{
   if (ch == NULL || obj == NULL || IS_NPC(ch))
      return FALSE;
   if (ch->pcdata == NULL || ch->pcdata->bond == NULL)
      return FALSE;
   return IS_OBJ_BONDED(obj);
}

/* ── Find the bonded weapon object on a character ─────────────── */

static OBJ_DATA *find_bonded_obj(CHAR_DATA *ch)
{
   OBJ_DATA *obj;

   if (ch == NULL || IS_NPC(ch) || ch->pcdata->bond == NULL)
      return NULL;

   for (obj = ch->first_carry; obj != NULL; obj = obj->next_in_carry_list)
   {
      if (IS_OBJ_BONDED(obj))
         return obj;
   }
   return NULL;
}

/* ── Clear and reapply affects on the bonded weapon ───────────── */

static void bond_strip_affects(OBJ_DATA *obj)
{
   AFFECT_DATA *paf;
   AFFECT_DATA *paf_next;

   for (paf = obj->first_apply; paf != NULL; paf = paf_next)
   {
      paf_next = paf->next;
      UNLINK(paf, obj->first_apply, obj->last_apply, next, prev);
      PUT_FREE(paf, affect_free);
   }
}

void bond_recalculate(CHAR_DATA *ch)
{
   OBJ_DATA *obj;
   BOND_DATA *bond;
   int wielder_level;
   int mult;
   int base_level;
   bool was_equipped = FALSE;
   int old_wear_loc;

   if (ch == NULL || IS_NPC(ch) || ch->pcdata->bond == NULL)
      return;

   bond = ch->pcdata->bond;
   obj = find_bonded_obj(ch);
   if (obj == NULL)
      return;

   wielder_level = get_psuedo_level(ch);
   if (wielder_level < 1)
      wielder_level = 1;
   base_level = bond->base_level;

   /* Temporarily unequip so stat changes don't double-apply */
   old_wear_loc = obj->wear_loc;
   if (old_wear_loc != -1)
   {
      was_equipped = TRUE;
      unequip_char(ch, obj);
   }

   /* Strip existing affects */
   bond_strip_affects(obj);

   /* Set object level to wielder's level for stat generation */
   obj->level = wielder_level;

   /* Run the standard auto-stat generation */
   set_obj_stat_auto(obj);

   /* Scale weapon damage dice to wielder level */
   obj->value[1] = number_fuzzy(number_fuzzy(1 * wielder_level / 8 + 3));
   obj->value[2] = number_fuzzy(number_fuzzy(3 * wielder_level / 8 + 4));

   /* Apply power_multiplier: scale all affects by base_level / 150 */
   {
      AFFECT_DATA *paf;
      for (paf = obj->first_apply; paf != NULL; paf = paf->next)
      {
         /* Scale the modifier by base_level / 150 */
         paf->modifier = paf->modifier * base_level / 150;
      }
   }

   /* Apply track bonuses */
   mult = bond_rank_multiplier(bond->rank);

   switch (bond->track)
   {
   case BOND_TRACK_EDGE:
   {
      /* Scale dr_mod and damage dice by track multiplier */
      AFFECT_DATA *paf;
      for (paf = obj->first_apply; paf != NULL; paf = paf->next)
      {
         if (paf->location == APPLY_DAMROLL)
            paf->modifier = paf->modifier * mult / 100;
      }
      /* Bonus damage dice from track */
      obj->value[1] = obj->value[1] * mult / 100;
      obj->value[2] = obj->value[2] * mult / 100;
      break;
   }
   case BOND_TRACK_GUARD:
   {
      /* Static ac_mod and parry_mod bonuses (not scaled by power_multiplier) */
      int ac_bonus = bond_guard_ac(bond->rank, base_level);
      int parry_bonus = bond_guard_parry(bond->rank);
      if (ac_bonus != 0)
         set_aff_to_obj(obj, APPLY_AC, ac_bonus);
      if (parry_bonus != 0)
         set_aff_to_obj(obj, APPLY_PARRY, parry_bonus);
      break;
   }
   case BOND_TRACK_KEEN:
   {
      /* Scale hr_mod by track multiplier */
      AFFECT_DATA *paf;
      for (paf = obj->first_apply; paf != NULL; paf = paf->next)
      {
         if (paf->location == APPLY_HITROLL)
            paf->modifier = paf->modifier * mult / 100;
      }
      /* Static crit and crit_mult bonuses */
      int crit_bonus = bond_keen_crit(bond->rank);
      int crit_mult_bonus = bond_keen_crit_mult(bond->rank);
      if (crit_bonus != 0)
         set_aff_to_obj(obj, APPLY_CRIT, crit_bonus);
      if (crit_mult_bonus != 0)
         set_aff_to_obj(obj, APPLY_CRIT_MULT, crit_mult_bonus);
      break;
   }
   case BOND_TRACK_SPIRIT:
   {
      /* Scale hp and spellpower by track multiplier */
      AFFECT_DATA *paf;
      for (paf = obj->first_apply; paf != NULL; paf = paf->next)
      {
         if (paf->location == APPLY_HIT || paf->location == APPLY_SPELLPOWER)
            paf->modifier = paf->modifier * mult / 100;
      }
      break;
   }
   default:
      break;
   }

   /* Apply mastery objfun if at max rank */
   if (bond->mastery && bond->rank == BOND_MAX_RANK)
   {
      switch (bond->track)
      {
      case BOND_TRACK_EDGE:
         obj->obj_fun = objfun_bond_edge;
         break;
      case BOND_TRACK_GUARD:
         obj->obj_fun = objfun_bond_guard;
         break;
      case BOND_TRACK_KEEN:
         obj->obj_fun = objfun_bond_keen;
         break;
      case BOND_TRACK_SPIRIT:
         obj->obj_fun = objfun_bond_spirit;
         break;
      default:
         obj->obj_fun = NULL;
         break;
      }
   }
   else
   {
      obj->obj_fun = NULL;
   }

   /* Re-equip if it was equipped */
   if (was_equipped)
      equip_char(ch, obj, old_wear_loc);
}

/* ── Bond point award on kill ─────────────────────────────────── */

void bond_award_kill(CHAR_DATA *ch, CHAR_DATA *victim)
{
   OBJ_DATA *wield;
   int level_diff;

   if (ch == NULL || IS_NPC(ch) || ch->pcdata->bond == NULL)
      return;
   if (victim == NULL || !IS_NPC(victim))
      return;

   /* Must be wielding the bonded weapon */
   wield = find_bonded_obj(ch);
   if (wield == NULL)
      return;
   if (wield->wear_loc != WEAR_HOLD_HAND_L && wield->wear_loc != WEAR_HOLD_HAND_R &&
       wield->wear_loc != WEAR_TWO_HANDED)
      return;

   /* Bond points: mob must be 10+ levels above wielder for 1 pt, 20+ for 2 pts */
   level_diff = victim->level - get_psuedo_level(ch);
   if (level_diff >= 20)
      ch->pcdata->bond->bond_points += 2;
   else if (level_diff >= 10)
      ch->pcdata->bond->bond_points += 1;
}

/* ── Destroy the old bonded weapon and apply Broken Bond debuff ─ */

static void bond_destroy_old(CHAR_DATA *ch)
{
   OBJ_DATA *old_obj;
   AFFECT_DATA af;

   old_obj = find_bonded_obj(ch);
   if (old_obj != NULL)
   {
      act("@@eYour bond with $p@@e shatters as the blade crumbles to dust.@@N", ch, old_obj, NULL,
          TO_CHAR);
      act("@@e$n's bonded blade crumbles to dust before your eyes.@@N", ch, old_obj, NULL, TO_ROOM);

      if (old_obj->wear_loc != -1)
         unequip_char(ch, old_obj);
      obj_from_char(old_obj);
      extract_obj(old_obj);
   }

   /* Apply Broken Bond debuff */
   af.type = skill_lookup("weapon bond");
   af.location = APPLY_HITROLL;
   af.modifier = -10;
   af.duration_type = DURATION_HOUR;
   af.duration = BOND_BROKEN_DURATION;
   af.bitvector = 0;
   af.caster = NULL;
   affect_to_char(ch, &af);

   af.location = APPLY_DAMROLL;
   af.modifier = -10;
   affect_to_char(ch, &af);
}

/* ── Subcommand: bond <weapon> ────────────────────────────────── */

static void bond_weapon(CHAR_DATA *ch, char *argument)
{
   OBJ_DATA *obj;
   BOND_DATA *bond;

   if (IS_NPC(ch))
      return;

   /* Must be a swordsman */
   if (char_class_level(ch, CLASS_SWO) <= 0)
   {
      send_to_char("Only swordsmen can forge a weapon bond.\n\r", ch);
      return;
   }

   if (argument[0] == '\0')
   {
      send_to_char("Bond to which weapon?\n\r", ch);
      return;
   }

   obj = get_obj_carry(ch, argument);
   if (obj == NULL)
   {
      send_to_char("You aren't carrying that.\n\r", ch);
      return;
   }

   if (obj->item_type != ITEM_WEAPON)
   {
      send_to_char("That is not a weapon.\n\r", ch);
      return;
   }

   if (obj->value[3] != WEAPON_TYPE_SWORD)
   {
      send_to_char("You can only bond with a sword.\n\r", ch);
      return;
   }

   if (IS_OBJ_BONDED(obj))
   {
      send_to_char("That weapon is already bonded to you.\n\r", ch);
      return;
   }

   /* Destroy old bonded weapon if one exists */
   if (ch->pcdata->bond != NULL)
   {
      bond_destroy_old(ch);
      bond_free(ch->pcdata->bond);
      ch->pcdata->bond = NULL;
   }

   /* Create the bond data */
   bond = (BOND_DATA *)calloc(1, sizeof(BOND_DATA));
   bond->base_obj_vnum = obj->pIndexData->vnum;
   bond->base_level = obj->level;
   bond->bond_points = 0;
   bond->track = BOND_TRACK_NONE;
   bond->rank = 0;
   bond->mastery = FALSE;
   bond->weapon_name = str_dup(obj->name);
   bond->weapon_short = str_dup(obj->short_descr);
   ch->pcdata->bond = bond;

   /* Transform the existing weapon into the bonded version */
   SET_BIT(obj->extra_flags, ITEM_BONDED);
   SET_BIT(obj->extra_flags, ITEM_NODROP);
   SET_BIT(obj->extra_flags, ITEM_NOREMOVE);
   SET_BIT(obj->extra_flags, ITEM_NODISARM);

   /* Apply wait state (bonding trance) */
   WAIT_STATE(ch, 24);

   act("@@WYou press $p@@W against your forehead, closing your eyes...@@N", ch, obj, NULL, TO_CHAR);
   act("@@W$n presses $p@@W against $s forehead, eyes closing in deep concentration...@@N", ch, obj,
       NULL, TO_ROOM);
   act("@@W$p@@W pulses with inner light as your souls intertwine.@@N", ch, obj, NULL, TO_CHAR);
   act("@@W$n's eyes snap open as $p@@W pulses with a blinding light!@@N", ch, obj, NULL, TO_ROOM);

   send_to_char("@@aYou have forged a soul-bond with your weapon.@@N\n\r", ch);
   send_to_char("@@dUse 'bond status' to view your bond, 'bond track' to choose a path.@@N\n\r",
                ch);

   /* Recalculate stats for current level */
   bond_recalculate(ch);
}

/* ── Subcommand: bond status ──────────────────────────────────── */

static void bond_status(CHAR_DATA *ch)
{
   BOND_DATA *bond;
   OBJ_DATA *obj;
   char buf[MAX_STRING_LENGTH];

   if (ch->pcdata->bond == NULL)
   {
      send_to_char("You have no bonded weapon.\n\r", ch);
      return;
   }

   bond = ch->pcdata->bond;
   obj = find_bonded_obj(ch);

   send_to_char("@@W=== Weapon Bond Status ===@@N\n\r", ch);

   sprintf(buf, "@@aWeapon@@N:      %s@@N\n\r",
           bond->weapon_short ? bond->weapon_short : "Unknown");
   send_to_char(buf, ch);

   sprintf(buf, "@@aBase Level@@N:  %d\n\r", bond->base_level);
   send_to_char(buf, ch);

   sprintf(buf, "@@aBase Vnum@@N:   %d\n\r", bond->base_obj_vnum);
   send_to_char(buf, ch);

   sprintf(buf, "@@aTrack@@N:       %s\n\r", bond_track_name(bond->track));
   send_to_char(buf, ch);

   if (bond->track != BOND_TRACK_NONE)
   {
      sprintf(buf, "@@aRank@@N:        %d / %d\n\r", bond->rank, BOND_MAX_RANK);
      send_to_char(buf, ch);

      if (bond->rank < BOND_MAX_RANK)
      {
         int next_cost = bond_rank_cost(bond->rank + 1);
         sprintf(buf, "@@aNext Rank@@N:   %d bond points (you have %d)\n\r", next_cost,
                 bond->bond_points);
         send_to_char(buf, ch);
      }
      else
      {
         sprintf(buf, "@@aBond Points@@N: %d (max rank reached)\n\r", bond->bond_points);
         send_to_char(buf, ch);
      }

      if (bond->mastery)
         send_to_char("@@eMastery@@N:     @@WACTIVE@@N\n\r", ch);
      else if (bond->rank == BOND_MAX_RANK)
         send_to_char("@@eMastery@@N:     @@dAvailable (use 'bond mastery', costs 100 qp)@@N\n\r",
                      ch);
   }
   else
   {
      sprintf(buf, "@@aBond Points@@N: %d\n\r", bond->bond_points);
      send_to_char(buf, ch);
      send_to_char("@@dChoose a track with 'bond track <edge|guard|keen|spirit>'.@@N\n\r", ch);
   }

   if (obj != NULL)
   {
      AFFECT_DATA *paf;
      send_to_char("\n\r@@WCurrent weapon stats:@@N\n\r", ch);
      sprintf(buf, "  Level: %d  Damage: %dd%d  Type: slash\n\r", obj->level, obj->value[1],
              obj->value[2]);
      send_to_char(buf, ch);

      for (paf = obj->first_apply; paf != NULL; paf = paf->next)
      {
         if (paf->modifier == 0)
            continue;

         switch (paf->location)
         {
         case APPLY_HITROLL:
            sprintf(buf, "  Hitroll:    %+d\n\r", paf->modifier);
            break;
         case APPLY_DAMROLL:
            sprintf(buf, "  Damroll:    %+d\n\r", paf->modifier);
            break;
         case APPLY_AC:
            sprintf(buf, "  Armor:      %+d\n\r", paf->modifier);
            break;
         case APPLY_HIT:
            sprintf(buf, "  Hit Points: %+d\n\r", paf->modifier);
            break;
         case APPLY_MANA:
            sprintf(buf, "  Mana:       %+d\n\r", paf->modifier);
            break;
         case APPLY_SPELLPOWER:
            sprintf(buf, "  Spellpower: %+d\n\r", paf->modifier);
            break;
         case APPLY_PARRY:
            sprintf(buf, "  Parry:      %+d\n\r", paf->modifier);
            break;
         case APPLY_CRIT:
            sprintf(buf, "  Crit:       %+d\n\r", paf->modifier);
            break;
         case APPLY_CRIT_MULT:
            sprintf(buf, "  Crit Mult:  %+d\n\r", paf->modifier);
            break;
         default:
            sprintf(buf, "  Apply(%d):   %+d\n\r", paf->location, paf->modifier);
            break;
         }
         send_to_char(buf, ch);
      }
   }
}

/* ── Subcommand: bond track <name> ────────────────────────────── */

static void bond_set_track(CHAR_DATA *ch, char *argument)
{
   BOND_DATA *bond;
   int new_track = BOND_TRACK_NONE;

   if (ch->pcdata->bond == NULL)
   {
      send_to_char("You have no bonded weapon.\n\r", ch);
      return;
   }

   bond = ch->pcdata->bond;

   if (argument[0] == '\0')
   {
      send_to_char("Available tracks:\n\r", ch);
      send_to_char("  @@Wedge@@N   - Raw damage: damroll, damage dice\n\r", ch);
      send_to_char("  @@Wguard@@N  - Defense: armor class, parry\n\r", ch);
      send_to_char("  @@Wkeen@@N   - Precision: hitroll, crit chance, crit multiplier\n\r", ch);
      send_to_char("  @@Wspirit@@N - Sustain: hit points, spellpower\n\r", ch);
      return;
   }

   if (!str_prefix(argument, "edge"))
      new_track = BOND_TRACK_EDGE;
   else if (!str_prefix(argument, "guard"))
      new_track = BOND_TRACK_GUARD;
   else if (!str_prefix(argument, "keen"))
      new_track = BOND_TRACK_KEEN;
   else if (!str_prefix(argument, "spirit"))
      new_track = BOND_TRACK_SPIRIT;
   else
   {
      send_to_char("Unknown track. Choose: edge, guard, keen, spirit.\n\r", ch);
      return;
   }

   if (bond->track == new_track)
   {
      send_to_char("You are already on that track.\n\r", ch);
      return;
   }

   if (bond->track != BOND_TRACK_NONE)
   {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf,
              "@@eWARNING@@N: Switching tracks will reset your rank from %d to 1 and forfeit all "
              "invested bond points.\n\r",
              bond->rank);
      send_to_char(buf, ch);
      send_to_char("@@eType 'bond track confirm' to confirm the switch, or choose a different "
                   "track.@@N\n\r",
                   ch);

      /* Store pending track change for confirmation */
      /* We use a simple approach: if the argument ends with "confirm",
       * and they had a track, proceed. Otherwise just warn. */
      /* Actually, let's check if they appended confirm */
      return;
   }

   bond->track = new_track;
   bond->rank = 1;
   bond->mastery = FALSE;

   {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "@@aYou attune your bond to the path of %s (Rank 1).@@N\n\r",
              bond_track_name(new_track));
      send_to_char(buf, ch);
   }

   bond_recalculate(ch);
}

/* Confirm track switch */
static void bond_track_confirm(CHAR_DATA *ch, char *track_arg)
{
   BOND_DATA *bond;
   int new_track = BOND_TRACK_NONE;

   if (ch->pcdata->bond == NULL)
   {
      send_to_char("You have no bonded weapon.\n\r", ch);
      return;
   }

   bond = ch->pcdata->bond;

   if (!str_prefix(track_arg, "edge"))
      new_track = BOND_TRACK_EDGE;
   else if (!str_prefix(track_arg, "guard"))
      new_track = BOND_TRACK_GUARD;
   else if (!str_prefix(track_arg, "keen"))
      new_track = BOND_TRACK_KEEN;
   else if (!str_prefix(track_arg, "spirit"))
      new_track = BOND_TRACK_SPIRIT;
   else
   {
      send_to_char("Unknown track.\n\r", ch);
      return;
   }

   if (bond->track == new_track)
   {
      send_to_char("You are already on that track.\n\r", ch);
      return;
   }

   /* Reset rank and points */
   bond->track = new_track;
   bond->rank = 1;
   bond->bond_points = 0;
   bond->mastery = FALSE;

   {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf,
              "@@eYou sever your old attunement. Your bond reshapes to the path of %s (Rank "
              "1).@@N\n\r",
              bond_track_name(new_track));
      send_to_char(buf, ch);
   }

   bond_recalculate(ch);
}

/* ── Subcommand: bond rankup ──────────────────────────────────── */

static void bond_rankup(CHAR_DATA *ch)
{
   BOND_DATA *bond;
   int cost;
   char buf[MAX_STRING_LENGTH];

   if (ch->pcdata->bond == NULL)
   {
      send_to_char("You have no bonded weapon.\n\r", ch);
      return;
   }

   bond = ch->pcdata->bond;

   if (bond->track == BOND_TRACK_NONE)
   {
      send_to_char("Choose a track first with 'bond track <name>'.\n\r", ch);
      return;
   }

   if (bond->rank >= BOND_MAX_RANK)
   {
      send_to_char("Your bond has reached maximum rank.\n\r", ch);
      return;
   }

   cost = bond_rank_cost(bond->rank + 1);
   if (bond->bond_points < cost)
   {
      sprintf(buf, "You need %d bond points to advance (you have %d).\n\r", cost,
              bond->bond_points);
      send_to_char(buf, ch);
      return;
   }

   bond->bond_points -= cost;
   bond->rank++;

   sprintf(buf, "@@WYour bond deepens. %s Rank %d achieved!@@N\n\r", bond_track_name(bond->track),
           bond->rank);
   send_to_char(buf, ch);

   if (bond->rank == BOND_MAX_RANK)
   {
      send_to_char("@@eYou have reached the pinnacle of this path.@@N\n\r", ch);
      send_to_char(
          "@@dUse 'bond mastery' to unlock your weapon's true power (costs 100 qp).@@N\n\r", ch);
   }

   bond_recalculate(ch);
}

/* ── Subcommand: bond mastery ─────────────────────────────────── */

static void bond_mastery(CHAR_DATA *ch)
{
   BOND_DATA *bond;

   if (ch->pcdata->bond == NULL)
   {
      send_to_char("You have no bonded weapon.\n\r", ch);
      return;
   }

   bond = ch->pcdata->bond;

   if (bond->track == BOND_TRACK_NONE)
   {
      send_to_char("Choose a track first.\n\r", ch);
      return;
   }

   if (bond->rank < BOND_MAX_RANK)
   {
      send_to_char("You must reach maximum rank before unlocking mastery.\n\r", ch);
      return;
   }

   if (bond->mastery)
   {
      send_to_char("You have already unlocked mastery.\n\r", ch);
      return;
   }

   if (ch->quest_points < BOND_MASTERY_QP_COST)
   {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "Unlocking mastery costs %d quest points (you have %d).\n\r",
              BOND_MASTERY_QP_COST, ch->quest_points);
      send_to_char(buf, ch);
      return;
   }

   ch->quest_points -= BOND_MASTERY_QP_COST;
   bond->mastery = TRUE;

   switch (bond->track)
   {
   case BOND_TRACK_EDGE:
      act("@@eYour blade hungers for blood. The Edge awakens.@@N", ch, NULL, NULL, TO_CHAR);
      act("@@e$n's bonded blade shimmers with a razor-thin crimson aura.@@N", ch, NULL, NULL,
          TO_ROOM);
      break;
   case BOND_TRACK_GUARD:
      act("@@aYour blade hums with a protective resonance. The Guard stands eternal.@@N", ch, NULL,
          NULL, TO_CHAR);
      act("@@a$n's bonded blade radiates a steady, shielding glow.@@N", ch, NULL, NULL, TO_ROOM);
      break;
   case BOND_TRACK_KEEN:
      act("@@WYour blade whispers of weak points and killing strikes. The Keen eye opens.@@N", ch,
          NULL, NULL, TO_CHAR);
      act("@@W$n's bonded blade gleams with predatory precision.@@N", ch, NULL, NULL, TO_ROOM);
      break;
   case BOND_TRACK_SPIRIT:
      act("@@mYour blade breathes with living energy. The Spirit flows through steel.@@N", ch, NULL,
          NULL, TO_CHAR);
      act("@@m$n's bonded blade pulses with ethereal, life-giving light.@@N", ch, NULL, NULL,
          TO_ROOM);
      break;
   }

   bond_recalculate(ch);
}

/* ── Subcommand: bond name <new name> ─────────────────────────── */

static void bond_rename(CHAR_DATA *ch, char *argument)
{
   BOND_DATA *bond;
   OBJ_DATA *obj;
   char buf[MAX_STRING_LENGTH];

   if (ch->pcdata->bond == NULL)
   {
      send_to_char("You have no bonded weapon.\n\r", ch);
      return;
   }

   if (argument[0] == '\0')
   {
      send_to_char("Name your bonded weapon what?\n\r", ch);
      return;
   }

   bond = ch->pcdata->bond;
   obj = find_bonded_obj(ch);

   if (bond->weapon_name != NULL)
      free_string(bond->weapon_name);
   bond->weapon_name = str_dup(argument);

   if (bond->weapon_short != NULL)
      free_string(bond->weapon_short);
   bond->weapon_short = str_dup(argument);

   if (obj != NULL)
   {
      free_string(obj->name);
      obj->name = str_dup(argument);
      free_string(obj->short_descr);
      obj->short_descr = str_dup(argument);
      sprintf(buf, "%s lies here, thrumming with latent power.", argument);
      free_string(obj->description);
      obj->description = str_dup(buf);
   }

   sprintf(buf, "@@aYour bonded weapon is now known as '%s@@a'.@@N\n\r", argument);
   send_to_char(buf, ch);
}

/* ── Subcommand: bond tracks (list available tracks) ──────────── */

static void bond_tracks(CHAR_DATA *ch)
{
   BOND_DATA *bond = ch->pcdata->bond;

   send_to_char("@@W=== Bond Tracks ===@@N\n\r\n\r", ch);

   send_to_char("@@WEdge@@N   - The path of destruction. Increases damroll and raw damage.\n\r",
                ch);
   send_to_char("           Scaled by weapon base level. Mastery: blade lashes out with\n\r", ch);
   send_to_char("           bonus slash attacks in combat.\n\r\n\r", ch);

   send_to_char("@@WGuard@@N  - The path of the defender. Adds armor class and parry.\n\r", ch);
   send_to_char("           AC scales with base level; parry is flat. Mastery: blade\n\r", ch);
   send_to_char("           projects a damage-absorbing ward around the wielder.\n\r\n\r", ch);

   send_to_char("@@WKeen@@N   - The path of precision. Increases hitroll, crit, and crit\n\r", ch);
   send_to_char("           multiplier. Hitroll scales; crit bonuses are flat. Mastery:\n\r", ch);
   send_to_char("           strikes inflict bleeding wounds on foes.\n\r\n\r", ch);

   send_to_char("@@WSpirit@@N - The path of endurance. Increases hit points and spellpower.\n\r",
                ch);
   send_to_char("           Scaled by weapon base level. Mastery: weapon periodically\n\r", ch);
   send_to_char("           restores health and mana to the wielder.\n\r\n\r", ch);

   if (bond != NULL && bond->track != BOND_TRACK_NONE)
   {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "@@aCurrent track@@N: %s (Rank %d)\n\r", bond_track_name(bond->track),
              bond->rank);
      send_to_char(buf, ch);
   }
}

/* ── Master command handler ───────────────────────────────────── */

void do_bond(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];

   if (IS_NPC(ch))
      return;

   /* Must be a swordsman */
   if (char_class_level(ch, CLASS_SWO) <= 0)
   {
      send_to_char("Only swordsmen can forge weapon bonds.\n\r", ch);
      return;
   }

   argument = one_argument(argument, arg);

   if (arg[0] == '\0')
   {
      send_to_char("@@WWeapon Bond Commands:@@N\n\r", ch);
      send_to_char("  bond <weapon>          - Bond to a sword\n\r", ch);
      send_to_char("  bond status            - View bond details\n\r", ch);
      send_to_char("  bond track <name>      - Choose a track (edge/guard/keen/spirit)\n\r", ch);
      send_to_char("  bond track <name> confirm - Switch tracks (resets rank)\n\r", ch);
      send_to_char("  bond rankup            - Spend bond points to advance rank\n\r", ch);
      send_to_char("  bond mastery           - Unlock mastery at max rank (100 qp)\n\r", ch);
      send_to_char("  bond name <new name>   - Rename your bonded weapon\n\r", ch);
      send_to_char("  bond tracks            - List all available tracks\n\r", ch);
      return;
   }

   if (!str_prefix(arg, "status"))
   {
      bond_status(ch);
      return;
   }

   if (!str_prefix(arg, "track"))
   {
      argument = one_argument(argument, arg2);

      /* Check if there's a "confirm" after the track name */
      if (arg2[0] != '\0')
      {
         char confirm[MAX_INPUT_LENGTH];
         one_argument(argument, confirm);

         if (!str_cmp(confirm, "confirm"))
         {
            bond_track_confirm(ch, arg2);
            return;
         }
      }

      bond_set_track(ch, arg2);
      return;
   }

   if (!str_prefix(arg, "rankup"))
   {
      bond_rankup(ch);
      return;
   }

   if (!str_prefix(arg, "mastery"))
   {
      bond_mastery(ch);
      return;
   }

   if (!str_prefix(arg, "name"))
   {
      bond_rename(ch, argument);
      return;
   }

   if (!str_prefix(arg, "tracks"))
   {
      bond_tracks(ch);
      return;
   }

   /* Default: try to bond to a weapon by that name */
   bond_weapon(ch, arg);
}

/* ── Mastery objfuns ──────────────────────────────────────────── */

/*
 * Edge Mastery: "Bloodthirst"
 * The blade occasionally lashes out with a bonus slash attack in combat,
 * dealing damage based on the wielder's damroll.
 */
void objfun_bond_edge(OBJ_DATA *obj, CHAR_DATA *keeper)
{
   CHAR_DATA *victim;

   if (keeper == NULL || keeper->in_room == NULL)
      return;
   if (obj->item_type != ITEM_WEAPON)
      return;
   if (keeper->fighting == NULL)
      return;

   /* Must be wielding the bonded weapon */
   if (get_eq_char(keeper, WEAR_HOLD_HAND_L) != obj &&
       get_eq_char(keeper, WEAR_HOLD_HAND_R) != obj && get_eq_char(keeper, WEAR_TWO_HANDED) != obj)
      return;

   /* ~8% chance per tick */
   if (number_percent() > 8)
      return;

   for (victim = keeper->in_room->first_person; victim != NULL; victim = victim->next_in_room)
   {
      if (victim->fighting == keeper)
         break;
   }

   if (victim == NULL || victim->is_free != FALSE)
      return;

   act("@@e$p@@e thirsts for blood and lashes out at $N@@e!@@N", keeper, obj, victim, TO_CHAR);
   act("@@e$n@@e's bonded blade lashes out hungrily at $N@@e!@@N", keeper, obj, victim, TO_NOTVICT);
   act("@@e$n@@e's bonded blade lashes out hungrily at you!@@N", keeper, obj, victim, TO_VICT);

   {
      int dam = number_range(obj->value[1], obj->value[2]);
      dam += keeper->damroll / 2;
      damage(keeper, victim, dam, TYPE_HIT + obj->value[3]);
   }
}

/*
 * Guard Mastery: "Aegis Ward"
 * The blade projects a protective ward that periodically absorbs damage,
 * healing the wielder for a small amount when in combat.
 */
void objfun_bond_guard(OBJ_DATA *obj, CHAR_DATA *keeper)
{
   if (keeper == NULL || keeper->in_room == NULL)
      return;
   if (obj->item_type != ITEM_WEAPON)
      return;

   /* Must be wielding */
   if (get_eq_char(keeper, WEAR_HOLD_HAND_L) != obj &&
       get_eq_char(keeper, WEAR_HOLD_HAND_R) != obj && get_eq_char(keeper, WEAR_TWO_HANDED) != obj)
      return;

   if (keeper->fighting == NULL)
   {
      /* Out of combat: ambient flavor text */
      if (number_percent() < 2)
      {
         act("@@a$p@@a hums with a steady, protective resonance.@@N", keeper, obj, NULL, TO_CHAR);
         act("@@a$n@@a's bonded blade hums softly with a warding light.@@N", keeper, obj, NULL,
             TO_ROOM);
      }
      return;
   }

   /* ~10% chance per tick when fighting */
   if (number_percent() > 10)
      return;

   {
      long heal = get_max_hp(keeper) / 20; /* 5% of max HP */
      if (heal < 1)
         heal = 1;
      keeper->hit = UMIN(keeper->hit + heal, get_max_hp(keeper));

      act("@@a$p@@a projects a shimmering ward around you, mending your wounds.@@N", keeper, obj,
          NULL, TO_CHAR);
      act("@@a$n@@a's bonded blade flares with warding light, shielding $m.@@N", keeper, obj, NULL,
          TO_ROOM);
   }
}

/*
 * Keen Mastery: "Deep Wound"
 * Strikes occasionally inflict a bleeding wound on foes, dealing
 * damage over time via a poison-like effect.
 */
void objfun_bond_keen(OBJ_DATA *obj, CHAR_DATA *keeper)
{
   CHAR_DATA *victim;
   int sn;

   if (keeper == NULL || keeper->in_room == NULL)
      return;
   if (obj->item_type != ITEM_WEAPON)
      return;
   if (keeper->fighting == NULL)
      return;

   if (get_eq_char(keeper, WEAR_HOLD_HAND_L) != obj &&
       get_eq_char(keeper, WEAR_HOLD_HAND_R) != obj && get_eq_char(keeper, WEAR_TWO_HANDED) != obj)
      return;

   /* ~6% chance per tick */
   if (number_percent() > 6)
      return;

   for (victim = keeper->in_room->first_person; victim != NULL; victim = victim->next_in_room)
   {
      if (victim->fighting == keeper)
         break;
   }

   if (victim == NULL || victim->is_free != FALSE)
      return;

   /* Use weaken as a debuff to simulate a deep wound */
   sn = skill_lookup("weaken");
   if (sn < 0)
      return;

   act("@@W$p@@W finds a gap in $N@@W's defense, opening a deep, bleeding wound!@@N", keeper, obj,
       victim, TO_CHAR);
   act("@@W$n@@W's blade finds a gap in $N@@W's defense, leaving a bleeding gash!@@N", keeper, obj,
       victim, TO_NOTVICT);
   act("@@e$n@@e's blade slices deep, opening a bleeding wound on you!@@N", keeper, obj, victim,
       TO_VICT);

   obj_cast_spell(sn, obj->level, keeper, victim, obj);
}

/*
 * Spirit Mastery: "Living Steel"
 * The weapon periodically channels restorative energy, healing
 * HP and restoring mana to the wielder.
 */
void objfun_bond_spirit(OBJ_DATA *obj, CHAR_DATA *keeper)
{
   if (keeper == NULL || keeper->in_room == NULL)
      return;
   if (obj->item_type != ITEM_WEAPON)
      return;

   if (keeper->fighting == NULL)
      return;

   if (get_eq_char(keeper, WEAR_HOLD_HAND_L) != obj &&
       get_eq_char(keeper, WEAR_HOLD_HAND_R) != obj && get_eq_char(keeper, WEAR_TWO_HANDED) != obj)
      return;

   /* ~8% chance per tick */
   if (number_percent() > 8)
      return;

   {
      long heal = get_max_hp(keeper) / 25; /* 4% of max HP */
      long mana_restore = get_max_mana(keeper) / 25;
      if (heal < 1)
         heal = 1;
      if (mana_restore < 1)
         mana_restore = 1;

      keeper->hit = UMIN(keeper->hit + heal, get_max_hp(keeper));
      keeper->mana = UMIN(keeper->mana + mana_restore, get_max_mana(keeper));

      act("@@m$p@@m channels the spirit of steel, mending flesh and restoring focus.@@N", keeper,
          obj, NULL, TO_CHAR);
      act("@@m$n@@m's bonded blade pulses with ethereal energy.@@N", keeper, obj, NULL, TO_ROOM);
   }
}
