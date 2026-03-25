/*
 * Druid spell functions for the Druid class lineage.
 *
 * All Druid spells use the Overgrowth mechanic: they cost HP instead of mana,
 * and their growth field drives the Overgrowth accumulation in do_cast().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"
#include "tables.h"
#include "magic.h"

/* ========================================================================
 * Helper: get the caster's Druid level for scaling.
 * ======================================================================== */
static int druid_level(CHAR_DATA *ch)
{
   int best = char_class_level(ch, CLASS_DRU);
   if (char_class_level(ch, CLASS_THO) > best)
      best = char_class_level(ch, CLASS_THO);
   if (char_class_level(ch, CLASS_WIL) > best)
      best = char_class_level(ch, CLASS_WIL);
   if (char_class_level(ch, CLASS_HIE) > best)
      best = char_class_level(ch, CLASS_HIE);
   return UMAX(best, 1);
}

/* ========================================================================
 * OFFENSIVE SPELLS
 * ======================================================================== */

/*
 * Root Touch — the Druid's level 1 combat spell.
 *
 * The most basic Overgrowth expression: channelling living-world vitality
 * into a physical contact strike. Costs HP (growth mechanic), not mana.
 * Does not scale with level — the Druid's control is limited at first.
 */
bool spell_root_touch(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam = dice(2, 4);
   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_EARTH, sn, TRUE);
   return TRUE;
}

bool spell_thornlash(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam = dice(druid_level(ch) / 2, 6) + 10;
   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_EARTH, sn, TRUE);
   return TRUE;
}

bool spell_nettlesting(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam = dice(druid_level(ch) / 2, 6) + 12;
   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_POISON, sn, TRUE);
   return TRUE;
}

bool spell_rockburst(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam = dice(druid_level(ch) / 2, 8) + 20;
   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_EARTH, sn, TRUE);
   return TRUE;
}

bool spell_wither(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam = dice(druid_level(ch) / 2, 8) + 25;
   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_POISON, sn, TRUE);
   return TRUE;
}

bool spell_briar_volley(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam = dice(druid_level(ch) / 2, 10) + 30;
   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_EARTH, sn, TRUE);
   return TRUE;
}

bool spell_blight_touch(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;
   int dam = dice(druid_level(ch) / 2, 8) + 20;

   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_POISON, sn, TRUE);

   /* Poison DoT */
   if (!is_affected(victim, sn) && !saves_spell(level, victim))
   {
      af.type = sn;
      af.duration = 4;
      af.duration_type = DURATION_ROUND;
      af.location = APPLY_DOT;
      af.modifier = dam / 4;
      af.bitvector = 0;
      af.caster = ch;
      af.element = ELEMENT_POISON;
      affect_to_char(victim, &af);
      send_to_char("@@eA creeping blight spreads across your skin!@@N\n\r", victim);
   }
   return TRUE;
}

bool spell_tremor(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam = dice(druid_level(ch) / 2, 12) + 40;
   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_EARTH, sn, TRUE);
   return TRUE;
}

bool spell_creeping_rot(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;
   int dam = dice(druid_level(ch) / 2, 10) + 30;

   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_POISON, sn, TRUE);

   /* DoT + healing reduction marker */
   if (!is_affected(victim, sn))
   {
      af.type = sn;
      af.duration = 5;
      af.duration_type = DURATION_ROUND;
      af.location = APPLY_DOT;
      af.modifier = dam / 5;
      af.bitvector = 0;
      af.caster = ch;
      af.element = ELEMENT_POISON;
      affect_to_char(victim, &af);
      send_to_char("@@eDecay spreads through your body, sapping your vitality!@@N\n\r", victim);
   }
   return TRUE;
}

bool spell_fissure(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam = dice(druid_level(ch) / 2, 14) + 60;
   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_EARTH, sn, TRUE);
   return TRUE;
}

bool spell_fungal_eruption(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)vo;
   int lvl = druid_level(ch);
   int min_dam = dice(lvl / 3, 8) + 30;
   int max_dam = dice(lvl / 3, 12) + 50;
   aoe_damage(ch, obj, sn, level, min_dam, max_dam, ELEMENT_POISON, AOE_SAVES | AOE_SKIP_GROUP);
   return TRUE;
}

bool spell_quake(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)vo;
   int lvl = druid_level(ch);
   int min_dam = dice(lvl / 3, 10) + 40;
   int max_dam = dice(lvl / 3, 14) + 60;
   aoe_damage(ch, obj, sn, level, min_dam, max_dam, ELEMENT_EARTH, AOE_SAVES | AOE_SKIP_GROUP);
   return TRUE;
}

bool spell_strangling_vines(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;
   int dam = dice(druid_level(ch) / 2, 12) + 50;

   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_EARTH, sn, TRUE);

   /* Snare effect: lower DEX */
   if (!is_affected(victim, sn) && !saves_spell(level, victim))
   {
      af.type = sn;
      af.duration = 3;
      af.location = APPLY_DEX;
      af.modifier = -3;
      af.bitvector = 0;
      affect_to_char(victim, &af);
      act("@@aVines wrap tightly around $n's legs!@@N", victim, NULL, NULL, TO_ROOM);
      send_to_char("@@aVines wrap tightly around your legs, slowing you!@@N\n\r", victim);
   }
   return TRUE;
}

bool spell_rot(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int dam = dice(druid_level(ch) / 2, 16) + 80;
   if (saves_spell(level, victim))
      dam /= 2;
   sp_damage(obj, ch, victim, dam, ELEMENT_POISON, sn, TRUE);
   return TRUE;
}

bool spell_upheaval(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)vo;
   int lvl = druid_level(ch);
   int min_dam = dice(lvl / 3, 14) + 60;
   int max_dam = dice(lvl / 3, 18) + 90;
   aoe_damage(ch, obj, sn, level, min_dam, max_dam, ELEMENT_EARTH, AOE_SAVES | AOE_SKIP_GROUP);
   return TRUE;
}

/* ========================================================================
 * HEALING SPELLS (7)
 * ======================================================================== */

bool spell_sap_mend(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int heal = number_range(20, 35) + druid_level(ch) / 2;
   heal_character(ch, victim, heal, sn, FALSE);
   send_to_char("@@aWarm sap flows through your veins, mending your wounds.@@N\n\r", victim);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_mend_wounds(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int heal = number_range(25, 45) + druid_level(ch);
   heal_character(ch, victim, heal, sn, FALSE);
   send_to_char("@@aNatural energy knits your wounds together.@@N\n\r", victim);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_purge_toxin(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)sn;
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   if (is_affected(victim, gsn_poison))
   {
      affect_strip(victim, gsn_poison);
      act("$N looks healthier as the toxins drain away.", ch, NULL, victim, TO_NOTVICT);
      send_to_char("@@aA surge of natural energy purges the poison from your body.@@N\n\r", victim);
      send_to_char("Ok.\n\r", ch);
   }
   else
      send_to_char("They are not poisoned.\n\r", ch);
   return TRUE;
}

bool spell_verdant_mend(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int heal = number_range(50, 80) + druid_level(ch) * 2;
   heal_character(ch, victim, heal, sn, FALSE);
   send_to_char("@@aVerdant energy washes over you, restoring vitality.@@N\n\r", victim);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_heartwood_balm(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)sn;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   /* Cure blind + cure poison */
   spell_cure_blindness(sn, level, ch, vo, obj);
   if (is_affected(victim, gsn_poison))
   {
      affect_strip(victim, gsn_poison);
      send_to_char("@@aThe balm of heartwood purges toxins from your body.@@N\n\r", victim);
   }
   return TRUE;
}

bool spell_primal_restoration(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int heal = number_range(100, 160) + druid_level(ch) * 3;
   heal_character(ch, victim, heal, sn, FALSE);
   send_to_char("@@aPrimal energy surges through you, restoring body and spirit.@@N\n\r", victim);

   /* Also cure poison and blindness */
   spell_cure_blindness(sn, level, ch, vo, obj);
   if (is_affected(victim, gsn_poison))
      affect_strip(victim, gsn_poison);
   if (is_affected(victim, gsn_curse))
      affect_strip(victim, gsn_curse);

   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_sap_transfusion(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   int transfer;

   if (victim == ch)
   {
      send_to_char("You cannot transfuse life to yourself.\n\r", ch);
      return FALSE;
   }

   /* Transfer HP from caster to target, 1:1 ratio */
   transfer = UMIN(ch->hit - 1, (int)(get_max_hp(victim) - victim->hit));
   transfer = UMIN(transfer, 200 + druid_level(ch) * 2);
   transfer = UMAX(transfer, 0);

   if (transfer <= 0)
   {
      send_to_char("You have no life force to spare, or they are already at full health.\n\r", ch);
      return FALSE;
   }

   ch->hit -= transfer;
   heal_character(ch, victim, transfer, sn, FALSE);
   send_to_char("@@aYou channel your life force into another.@@N\n\r", ch);
   send_to_char("@@aWarm sap-like energy flows into you from another's sacrifice.@@N\n\r", victim);
   return TRUE;
}

/* ========================================================================
 * BUFF SPELLS (8)
 * ======================================================================== */

bool spell_barkskin(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return FALSE;
   af.type = sn;
   af.duration = (ch == victim) ? -1 : 24;
   af.modifier = -15 - druid_level(ch) / 5;
   af.location = APPLY_AC;
   af.bitvector = 0;
   affect_to_char(victim, &af);
   send_to_char("@@aYour skin hardens into rough bark.@@N\n\r", victim);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_roothold(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return FALSE;
   af.type = sn;
   af.duration = (ch == victim) ? -1 : 12;
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bitvector = 0;
   affect_to_char(victim, &af);
   send_to_char("@@aRoots grow from your feet, anchoring you firmly in place.@@N\n\r", victim);
   return TRUE;
}

bool spell_thornhide(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return FALSE;
   af.type = sn;
   af.duration = (ch == victim) ? -1 : 12;
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bitvector = 0;
   affect_to_char(victim, &af);
   send_to_char("@@aSharp thorns sprout from your skin!@@N\n\r", victim);
   act("Sharp thorns sprout from $n's skin!", victim, NULL, NULL, TO_ROOM);
   return TRUE;
}

/* stone skin is already defined in spell_stone_skin.c -- shared with Magi */

bool spell_ironpine_vigor(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return FALSE;
   af.type = sn;
   af.duration = (ch == victim) ? -1 : 24;
   af.modifier = get_max_hp(victim) / 10;
   af.location = APPLY_HIT;
   af.bitvector = 0;
   affect_to_char(victim, &af);
   send_to_char("@@aThe vigor of ironpine surges through you, bolstering your health.@@N\n\r",
                victim);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_gift_of_the_forest(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return FALSE;

   af.type = sn;
   af.duration = (ch == victim) ? -1 : 24;
   af.bitvector = 0;

   af.location = APPLY_STR;
   af.modifier = 2;
   affect_to_char(victim, &af);
   af.location = APPLY_INT;
   affect_to_char(victim, &af);
   af.location = APPLY_WIS;
   affect_to_char(victim, &af);
   af.location = APPLY_DEX;
   affect_to_char(victim, &af);
   af.location = APPLY_CON;
   affect_to_char(victim, &af);

   send_to_char("@@aThe forest's gift fills you with power!@@N\n\r", victim);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_natures_ward(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return FALSE;
   af.type = sn;
   af.duration = (ch == victim) ? -1 : 24;
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bitvector = 0;
   affect_to_char(victim, &af);
   send_to_char("@@aNature's ward surrounds you, granting resistance to poison and earth.@@N\n\r",
                victim);
   return TRUE;
}

bool spell_ancient_bark(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)level;
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return FALSE;
   af.type = sn;
   af.duration = (ch == victim) ? -1 : 24;
   af.modifier = -30 - druid_level(ch) / 3;
   af.location = APPLY_AC;
   af.bitvector = 0;
   affect_to_char(victim, &af);
   send_to_char("@@aAncient bark encases your body, thick with thorns and hardened by ages.@@N\n\r",
                victim);
   act("Ancient bark encases $n, thorns jutting outward!", victim, NULL, NULL, TO_ROOM);
   return TRUE;
}

/* ========================================================================
 * DEBUFF SPELLS (7)
 * ======================================================================== */

bool spell_tangling_roots(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (IS_AFFECTED(victim, AFF_FAERIE_FIRE) || is_affected(victim, sn))
      return (ch == victim ? FALSE : TRUE);
   af.type = sn;
   af.duration = (ch == victim) ? -1 : 5;
   af.location = APPLY_AC;
   af.modifier = 2 * druid_level(ch);
   af.bitvector = AFF_FAERIE_FIRE;
   affect_to_char(victim, &af);
   send_to_char("@@aRoots burst from the ground and tangle around you!@@N\n\r", victim);
   act("@@aRoots burst from the ground and tangle around $n!@@N", victim, NULL, NULL, TO_ROOM);
   return TRUE;
}

bool spell_sap_strength(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return TRUE;
   if (saves_spell(level, victim))
      return TRUE;
   af.type = sn;
   af.duration = 5;
   af.location = APPLY_STR;
   af.modifier = -2;
   af.bitvector = 0;
   affect_to_char(victim, &af);
   send_to_char("@@eYou feel your strength being sapped away!@@N\n\r", victim);
   act("$n looks noticeably weaker.", victim, NULL, NULL, TO_ROOM);
   return TRUE;
}

bool spell_spore_cloud(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (IS_AFFECTED(victim, AFF_BLIND) || saves_spell(level, victim))
   {
      send_to_char("The spores fail to take hold.\n\r", ch);
      return TRUE;
   }
   af.type = sn;
   af.location = APPLY_HITROLL;
   af.modifier = -4;
   af.duration = 3;
   af.bitvector = AFF_BLIND;
   affect_to_char(victim, &af);
   send_to_char("@@eA cloud of spores blinds you!@@N\n\r", victim);
   act("$n is blinded by a cloud of spores!", victim, NULL, NULL, TO_ROOM);
   return TRUE;
}

bool spell_wither_limbs(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return TRUE;
   if (saves_spell(level, victim))
      return TRUE;
   af.type = sn;
   af.duration = 5;
   af.location = APPLY_DEX;
   af.modifier = -2;
   af.bitvector = 0;
   affect_to_char(victim, &af);
   send_to_char("@@eYour limbs wither and slow!@@N\n\r", victim);
   act("$n's limbs wither visibly.", victim, NULL, NULL, TO_ROOM);
   return TRUE;
}

bool spell_entangling_growth(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return TRUE;
   if (saves_spell(level, victim))
   {
      send_to_char("The vines fail to take hold.\n\r", ch);
      return TRUE;
   }
   af.type = sn;
   af.duration = 4;
   af.location = APPLY_DEX;
   af.modifier = -4;
   af.bitvector = 0;
   affect_to_char(victim, &af);
   send_to_char("@@aThick growth entangles you, holding you fast!@@N\n\r", victim);
   act("@@aThick growth entangles $n!@@N", victim, NULL, NULL, TO_ROOM);
   return TRUE;
}

bool spell_curse_of_decay(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return TRUE;
   if (saves_spell(level, victim))
      return TRUE;

   /* Curse effect */
   af.type = sn;
   af.duration = 6;
   af.location = APPLY_HITROLL;
   af.modifier = -2;
   af.bitvector = AFF_CURSE;
   affect_to_char(victim, &af);

   /* Poison effect */
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = AFF_POISON;
   affect_to_char(victim, &af);

   send_to_char("@@eA terrible curse of decay settles upon you!@@N\n\r", victim);
   act("$n is wracked by a curse of decay!", victim, NULL, NULL, TO_ROOM);
   return TRUE;
}

bool spell_creeping_blight(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return TRUE;
   if (saves_spell(level, victim))
      return TRUE;

   af.type = sn;
   af.duration = 6;
   af.bitvector = 0;

   af.location = APPLY_STR;
   af.modifier = -3;
   affect_to_char(victim, &af);

   af.location = APPLY_CON;
   af.modifier = -3;
   affect_to_char(victim, &af);

   send_to_char("@@eA creeping blight withers your body, sapping strength and constitution!@@N\n\r",
                victim);
   act("$n is ravaged by a creeping blight!", victim, NULL, NULL, TO_ROOM);
   return TRUE;
}

/* ========================================================================
 * UTILITY SPELLS (8)
 * ======================================================================== */

bool spell_forestsight(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (IS_AFFECTED(victim, AFF_DETECT_HIDDEN))
      return FALSE;
   af.type = sn;
   af.duration = (ch == victim) ? -1 : 4 + (level / 8);
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = AFF_DETECT_HIDDEN;
   affect_to_char(victim, &af);
   send_to_char("@@aYour eyes attune to the forest, revealing hidden things.@@N\n\r", victim);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_sense_life(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (IS_AFFECTED(victim, AFF_DETECT_INVIS))
      return FALSE;
   af.type = sn;
   af.duration = (ch == victim) ? -1 : 4 + (level / 8);
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bitvector = AFF_DETECT_INVIS;
   affect_to_char(victim, &af);
   send_to_char("@@aYou sense the pulse of all living things around you.@@N\n\r", victim);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_know_terrain(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   /* Works like know alignment */
   return spell_know_alignment(sn, level, ch, vo, obj);
}

bool spell_passage(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   /* Works like pass door */
   return spell_pass_door(sn, level, ch, vo, obj);
}

bool spell_greensight(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   (void)obj;
   CHAR_DATA *victim = (CHAR_DATA *)vo;
   AFFECT_DATA af;

   if (is_affected(victim, sn))
      return FALSE;

   af.type = sn;
   af.duration = (ch == victim) ? -1 : 12;
   af.location = APPLY_NONE;
   af.modifier = 0;

   /* Grant infravision */
   af.bitvector = AFF_INFRARED;
   affect_to_char(victim, &af);

   /* Grant detect hidden */
   af.bitvector = AFF_DETECT_HIDDEN;
   affect_to_char(victim, &af);

   send_to_char("@@aYour eyes glow with verdant light, piercing darkness and deception.@@N\n\r",
                victim);
   if (ch != victim)
      send_to_char("Ok.\n\r", ch);
   return TRUE;
}

bool spell_rootway(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   /* Works like teleport */
   return spell_teleport(sn, level, ch, vo, obj);
}

bool spell_call_of_the_wild(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   /* Works like summon */
   return spell_summon(sn, level, ch, vo, obj);
}

bool spell_living_bridge(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj)
{
   /* Works like portal */
   return spell_portal(sn, level, ch, vo, obj);
}
