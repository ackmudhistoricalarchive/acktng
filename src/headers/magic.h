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

#ifndef DEC_ACK_H
#include "ack.h"
#endif

#define DEC_MAGIC_H

CHAR_DATA *player_summon(CHAR_DATA *ch, int level, int element);
int class_heal_character(CHAR_DATA *ch, CHAR_DATA *victim, int base_heal, int sn, int class_index,
                         bool hot);
void heal_character(CHAR_DATA *ch, CHAR_DATA *victim, int base_heal, int sn, bool hot);
bool sp_damage(OBJ_DATA *obj, CHAR_DATA *ch, CHAR_DATA *victim, int dam, int type, int sn,
               bool show_msg);
void group_gain args((CHAR_DATA * ch, CHAR_DATA *victim));
bool is_safe args((CHAR_DATA * ch, CHAR_DATA *victim));
void set_fighting args((CHAR_DATA * ch, CHAR_DATA *victim, bool check));
void check_adrenaline args((CHAR_DATA * ch, sh_int damage));
void do_spell_heal args((CHAR_DATA * ch, CHAR_DATA *victim, int sn));
void gain_holy_power_from_healing_spell(CHAR_DATA *ch, int sn);
void apply_elemental_spell_debuff(CHAR_DATA *ch, CHAR_DATA *victim, int sn, const char *debuff_msg);
bool trigger_elemental_spell_combo(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int sn,
                                   int level);
void apply_necromancer_damage_debuff(CHAR_DATA *ch, CHAR_DATA *victim, int sn, int direct_damage,
                                     OBJ_DATA *obj);

/* magic.c */
int mana_cost(CHAR_DATA *ch, int sn);
bool is_arcane_spell(int sn);
bool is_holy_power_spell(int sn);
bool is_mental_power_spell(int sn);

void breath_damage(CHAR_DATA *ch, int sn, int element);
void aoe_damage(CHAR_DATA *ch, OBJ_DATA *obj, int sn, int level, int min_dam, int max_dam,
                int element, int flags);
#define AOE_SAVES (1 << 0)
#define AOE_SKIP_GROUP (1 << 1)
int spell_regen_base_heal(int mage_level, int sorcerer_level, int wizard_level, int spellpower);
bool cast_wizard_elemental_dot_spell(int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim,
                                     OBJ_DATA *obj, const char *cast_msg, const char *hit_msg,
                                     int element);

#define ELE_NONE BIT_0
#define ELE_PHYSICAL BIT_1
#define ELE_MENTAL BIT_2
#define ELE_HOLY BIT_3
#define ELE_AIR BIT_4
#define ELE_EARTH BIT_5
#define ELE_WATER BIT_6
#define ELE_FIRE BIT_7
#define ELE_SHADOW BIT_8
#define ELE_POISON BIT_9
#define NO_STAT_BONUS BIT_10

#define MAX_ELEMENT 9
#define MAX_REALM MAX_ELEMENT

#define ELEMENT_NONE ELE_NONE
#define ELEMENT_PHYSICAL ELE_PHYSICAL
#define ELEMENT_MENTAL ELE_MENTAL
#define ELEMENT_HOLY ELE_HOLY
#define ELEMENT_AIR ELE_AIR
#define ELEMENT_EARTH ELE_EARTH
#define ELEMENT_WATER ELE_WATER
#define ELEMENT_FIRE ELE_FIRE
#define ELEMENT_SHADOW ELE_SHADOW
#define ELEMENT_POISON ELE_POISON

#define SECOND_DIVISOR BIT_25
#define THIRD_DIVISOR BIT_26
#define FOURTH_DIVISOR BIT_27
#define FIFTH_DIVISOR BIT_28
#define SIXTH_DIVISOR BIT_29
#define NO_REFLECT BIT_30
#define NO_ABSORB BIT_31

#define WATER_ELEMENTAL 1
#define FIRE_ELEMENTAL 2
#define EARTH_ELEMENTAL 3
#define IRON_GOLEM 4
#define DIAMOND_GOLEM 5
#define SKELETON 6
#define SOUL_THIEF 7
#define HOLY_AVENGER 8
#define THOUGHT_DEVOURER 9

struct sp_dam_str_type
{
   int realm;
   char *basecol;
   char *invertcol;
   char *stresscol;
   char *backcol;
   char *formatter;
   char *vs;
   char *vs_stress;
   char *vp;
   char *vp_stress;
};

extern struct sp_dam_str_type sp_dam_str[];

/*
 * Spell functions.
 * Defined in magicX.c.
 */
DECLARE_SPELL_FUN(spell_null);

DECLARE_SPELL_FUN(spell_ego_whip);
DECLARE_SPELL_FUN(spell_physic_thrust);
DECLARE_SPELL_FUN(spell_physic_crush);
DECLARE_SPELL_FUN(spell_mind_flail);

DECLARE_SPELL_FUN(spell_acid_blast);
DECLARE_SPELL_FUN(spell_animate);
DECLARE_SPELL_FUN(spell_revenant);
DECLARE_SPELL_FUN(spell_armor);
DECLARE_SPELL_FUN(spell_badbreath);
DECLARE_SPELL_FUN(spell_bark_skin);
DECLARE_SPELL_FUN(spell_bless);
DECLARE_SPELL_FUN(spell_blindness);
DECLARE_SPELL_FUN(spell_bloody_tears);
DECLARE_SPELL_FUN(spell_burning_hands);
DECLARE_SPELL_FUN(spell_shard_ward);
DECLARE_SPELL_FUN(spell_calm);
DECLARE_SPELL_FUN(spell_cause_critical);
DECLARE_SPELL_FUN(spell_cause_light);
DECLARE_SPELL_FUN(spell_cause_serious);
DECLARE_SPELL_FUN(spell_change_sex);
DECLARE_SPELL_FUN(spell_charm_person);
DECLARE_SPELL_FUN(spell_chill_touch);
DECLARE_SPELL_FUN(spell_crystal_burst);
DECLARE_SPELL_FUN(spell_continual_light);
DECLARE_SPELL_FUN(spell_seal_calculus);
DECLARE_SPELL_FUN(spell_cure_blindness);
DECLARE_SPELL_FUN(spell_cure_critical);
DECLARE_SPELL_FUN(spell_cure_light);
DECLARE_SPELL_FUN(spell_cure_poison);
DECLARE_SPELL_FUN(spell_purify);
DECLARE_SPELL_FUN(spell_cure_serious);
DECLARE_SPELL_FUN(spell_curse);
DECLARE_SPELL_FUN(spell_detect_evil);
DECLARE_SPELL_FUN(spell_detect_hidden);
DECLARE_SPELL_FUN(spell_detect_invis);
DECLARE_SPELL_FUN(spell_detect_magic);
DECLARE_SPELL_FUN(spell_detect_poison);
DECLARE_SPELL_FUN(spell_detect_undead);
DECLARE_SPELL_FUN(spell_detection);
DECLARE_SPELL_FUN(spell_dimension_blade);
DECLARE_SPELL_FUN(spell_dispel_evil);
DECLARE_SPELL_FUN(spell_dispel_magic);
DECLARE_SPELL_FUN(spell_dispel_object);
DECLARE_SPELL_FUN(spell_jackal_verdict);
DECLARE_SPELL_FUN(spell_enchant_weapon);
DECLARE_SPELL_FUN(spell_encumber);
DECLARE_SPELL_FUN(spell_enhance_weapon);
DECLARE_SPELL_FUN(spell_energy_drain);
DECLARE_SPELL_FUN(spell_prism_mark);
DECLARE_SPELL_FUN(spell_ledger_mark);
DECLARE_SPELL_FUN(spell_fighting_trance);
DECLARE_SPELL_FUN(spell_fireball);
DECLARE_SPELL_FUN(spell_fire_blade);
DECLARE_SPELL_FUN(spell_flamestrike);
DECLARE_SPELL_FUN(spell_fly);
DECLARE_SPELL_FUN(spell_gate);
DECLARE_SPELL_FUN(spell_general_purpose);
DECLARE_SPELL_FUN(spell_giant_strength);
DECLARE_SPELL_FUN(spell_harm);
DECLARE_SPELL_FUN(spell_heal);
DECLARE_SPELL_FUN(spell_group_heal);
DECLARE_SPELL_FUN(spell_high_explosive);
DECLARE_SPELL_FUN(spell_hypnosis);
DECLARE_SPELL_FUN(spell_identify);
DECLARE_SPELL_FUN(spell_influx);
DECLARE_SPELL_FUN(spell_infravision);
DECLARE_SPELL_FUN(spell_invis);
DECLARE_SPELL_FUN(spell_know_alignment);
DECLARE_SPELL_FUN(spell_know_weakness);
DECLARE_SPELL_FUN(spell_know_critical);
DECLARE_SPELL_FUN(spell_know_item);
DECLARE_SPELL_FUN(spell_lightning_bolt);
DECLARE_SPELL_FUN(spell_locate_object);
DECLARE_SPELL_FUN(spell_magic_missile);
DECLARE_SPELL_FUN(spell_mass_invis);
DECLARE_SPELL_FUN(spell_mind_bolt);
DECLARE_SPELL_FUN(spell_nerve_fire);
DECLARE_SPELL_FUN(spell_night_vision);
DECLARE_SPELL_FUN(spell_pass_door);
DECLARE_SPELL_FUN(spell_phase);
DECLARE_SPELL_FUN(spell_poison);
DECLARE_SPELL_FUN(spell_protection);
DECLARE_SPELL_FUN(spell_psionic_recovery);
DECLARE_SPELL_FUN(spell_refresh);
DECLARE_SPELL_FUN(spell_refresh_mana);
DECLARE_SPELL_FUN(spell_regen);
DECLARE_SPELL_FUN(spell_remove_curse);
DECLARE_SPELL_FUN(spell_sanctuary);
DECLARE_SPELL_FUN(spell_see_magic);
DECLARE_SPELL_FUN(spell_pale_imprint);
DECLARE_SPELL_FUN(spell_shocking_grasp);
DECLARE_SPELL_FUN(spell_shield);
DECLARE_SPELL_FUN(spell_sleep);
DECLARE_SPELL_FUN(spell_stalker);
DECLARE_SPELL_FUN(spell_stone_skin);
DECLARE_SPELL_FUN(spell_suffocate);
DECLARE_SPELL_FUN(spell_summon);
DECLARE_SPELL_FUN(spell_teleport);
DECLARE_SPELL_FUN(spell_ventriloquate);
DECLARE_SPELL_FUN(spell_weaken);
DECLARE_SPELL_FUN(spell_window);
DECLARE_SPELL_FUN(spell_portal);
DECLARE_SPELL_FUN(spell_beacon);
DECLARE_SPELL_FUN(spell_word_of_recall);
DECLARE_SPELL_FUN(spell_acid_breath);
DECLARE_SPELL_FUN(spell_fire_breath);
DECLARE_SPELL_FUN(spell_frost_breath);
DECLARE_SPELL_FUN(spell_gas_breath);
DECLARE_SPELL_FUN(spell_lightning_breath);
DECLARE_SPELL_FUN(spell_planergy);
DECLARE_SPELL_FUN(spell_static);
DECLARE_SPELL_FUN(spell_visit);
DECLARE_SPELL_FUN(spell_chain_lightning);
DECLARE_SPELL_FUN(spell_phobia);
DECLARE_SPELL_FUN(spell_barrier);
DECLARE_SPELL_FUN(spell_recurrence_loop);
DECLARE_SPELL_FUN(spell_laserbolt);
DECLARE_SPELL_FUN(spell_hellspawn);
DECLARE_SPELL_FUN(spell_travel);
DECLARE_SPELL_FUN(spell_flare);
DECLARE_SPELL_FUN(spell_mystic_armor);
DECLARE_SPELL_FUN(spell_seal_room);
DECLARE_SPELL_FUN(spell_deflect_weapon);          /* slot 600 */
DECLARE_SPELL_FUN(spell_black_hand);              /* slot 601 */
DECLARE_SPELL_FUN(spell_throw_needle);            /* slot 602 */
DECLARE_SPELL_FUN(spell_morale);                  /* slot 603 */
DECLARE_SPELL_FUN(spell_leadership);              /* slot 604 */
DECLARE_SPELL_FUN(spell_ice_bolt);                /* slot 605 */
DECLARE_SPELL_FUN(spell_waterelem);               /* slot 606 */
DECLARE_SPELL_FUN(spell_skeleton);                /* slot 607 */
DECLARE_SPELL_FUN(spell_poison_weapon);           /* slot 608 */
DECLARE_SPELL_FUN(spell_fireelem);                /* slot 612 */
DECLARE_SPELL_FUN(spell_rune_fire);               /* slot 613 */
DECLARE_SPELL_FUN(spell_rune_shock);              /* slot 614 */
DECLARE_SPELL_FUN(spell_rune_poison);             /* slot 615 */
DECLARE_SPELL_FUN(spell_healing_light);           /* slot 616 */
DECLARE_SPELL_FUN(spell_wither_shadow);           /* slot 617 */
DECLARE_SPELL_FUN(spell_mana_flare);              /* slot 618 */
DECLARE_SPELL_FUN(spell_mana_drain);              /* slot 619 */
DECLARE_SPELL_FUN(spell_cage);                    /* slot 620 */
DECLARE_SPELL_FUN(spell_cloak_absorb);            /* slot 621 */
DECLARE_SPELL_FUN(spell_cloak_reflect);           /* slot 622 */
DECLARE_SPELL_FUN(spell_cloak_flaming);           /* slot 623 */
DECLARE_SPELL_FUN(spell_cloak_elements);          /* slot 624 */
DECLARE_SPELL_FUN(spell_room_dispel);             /* slot 625 */
DECLARE_SPELL_FUN(spell_cloak_adept);             /* slot 626 */
DECLARE_SPELL_FUN(spell_cloak_regen);             /* slot 627 */
DECLARE_SPELL_FUN(spell_throw_star);              /* slot 628 */
DECLARE_SPELL_FUN(spell_soul_net);                /* slot 629 */
DECLARE_SPELL_FUN(spell_condense_soul);           /* slot 630 */
DECLARE_SPELL_FUN(spell_restoration);             /* slot 632 */
DECLARE_SPELL_FUN(spell_infuse);                  /* slot 633 */
DECLARE_SPELL_FUN(spell_holy_light);              /* slot 634 */
DECLARE_SPELL_FUN(spell_divine_intervention);     /* slot 635 */
DECLARE_SPELL_FUN(spell_holy_armor);              /* slot 636 */
DECLARE_SPELL_FUN(spell_earthelem);               /* slot 637 */
DECLARE_SPELL_FUN(spell_iron_golem);              /* slot 638 */
DECLARE_SPELL_FUN(spell_diamond_golem);           /* slot 639 */
DECLARE_SPELL_FUN(spell_soul_thief);              /* slot 640 */
DECLARE_SPELL_FUN(spell_holy_avenger);            /* slot 641 */
DECLARE_SPELL_FUN(spell_heat_armor);              /* slot 642 */
DECLARE_SPELL_FUN(spell_retri_strike);            /* slot 643 */
DECLARE_SPELL_FUN(spell_lava_burst);              /* slot 644 */
DECLARE_SPELL_FUN(spell_fireshield);              /* slot 645 */
DECLARE_SPELL_FUN(spell_iceshield);               /* slot 646 */
DECLARE_SPELL_FUN(spell_shockshield);             /* slot 647 */
DECLARE_SPELL_FUN(spell_shadowshield);            /* slot 648 */
DECLARE_SPELL_FUN(spell_thoughtshield);           /* slot 649 */
DECLARE_SPELL_FUN(spell_tribe_claw);              /* slot 655 */
DECLARE_SPELL_FUN(spell_wolf_mark);               /* slot 656 */
DECLARE_SPELL_FUN(spell_summon_pegasus);          /* slot 659 */
DECLARE_SPELL_FUN(spell_summon_nightmare);        /* slot 660 */
DECLARE_SPELL_FUN(spell_summon_beast);            /* 661 */
DECLARE_SPELL_FUN(spell_summon_devourer);         /* 662 */
DECLARE_SPELL_FUN(spell_summon_shadow);           /* 663 */
DECLARE_SPELL_FUN(spell_summon_thought_devourer); /* 754 */
DECLARE_SPELL_FUN(spell_creature_bond);           /* 664 */
DECLARE_SPELL_FUN(spell_corrupt_bond);            /* 665 */
DECLARE_SPELL_FUN(spell_fireblast);               /* 666 */
DECLARE_SPELL_FUN(spell_shockstorm);
DECLARE_SPELL_FUN(spell_cone_cold);
DECLARE_SPELL_FUN(spell_earth_shatter);
DECLARE_SPELL_FUN(spell_holy_wrath);
DECLARE_SPELL_FUN(spell_wraith_touch);
DECLARE_SPELL_FUN(spell_thought_vise); /* 670 */
DECLARE_SPELL_FUN(spell_black_curse);
DECLARE_SPELL_FUN(spell_spirit_curse);
DECLARE_SPELL_FUN(spell_rictus_curse);
DECLARE_SPELL_FUN(spell_predictive_collapse);
DECLARE_SPELL_FUN(spell_cloak_misery);
DECLARE_SPELL_FUN(spell_cloak_drain);
DECLARE_SPELL_FUN(spell_cloak_iron);
DECLARE_SPELL_FUN(spell_cloak_mental);
DECLARE_SPELL_FUN(spell_cloak_oathbreaker);
DECLARE_SPELL_FUN(spell_cloak_transcendence);
DECLARE_SPELL_FUN(spell_sonic_blast);
DECLARE_SPELL_FUN(spell_haste);
DECLARE_SPELL_FUN(spell_slow);
DECLARE_SPELL_FUN(spell_overdrive);
DECLARE_SPELL_FUN(spell_mystical_focus); /* 676 */
DECLARE_SPELL_FUN(spell_magical_supremacy);
DECLARE_SPELL_FUN(spell_refuge);
DECLARE_SPELL_FUN(spell_feeble_mind);
DECLARE_SPELL_FUN(spell_feeble_body);
DECLARE_SPELL_FUN(spell_arc_lightning);
DECLARE_SPELL_FUN(spell_terra_rend);
DECLARE_SPELL_FUN(spell_tidal_lash);
DECLARE_SPELL_FUN(spell_phoenix_flare);
DECLARE_SPELL_FUN(spell_elemental_inferno);
DECLARE_SPELL_FUN(spell_elemental_shock);
DECLARE_SPELL_FUN(spell_elemental_deluge);
DECLARE_SPELL_FUN(spell_elemental_rupture);
DECLARE_SPELL_FUN(spell_wall_of_fire);         /* slot 700 */
DECLARE_SPELL_FUN(spell_mana_barrier);         /* slot 701 */
DECLARE_SPELL_FUN(spell_elemental_ward);       /* slot 702 */
DECLARE_SPELL_FUN(spell_arcane_lock);          /* slot 703 */
DECLARE_SPELL_FUN(spell_mirror_image);         /* slot 704 */
DECLARE_SPELL_FUN(spell_divine_strike);        /* slot 705 */
DECLARE_SPELL_FUN(spell_turn_undead);          /* slot 706 */
DECLARE_SPELL_FUN(spell_consecrate);           /* slot 708 */
DECLARE_SPELL_FUN(spell_solar_flare);          /* slot 709 */
DECLARE_SPELL_FUN(spell_holy_word);            /* slot 710 */
DECLARE_SPELL_FUN(spell_exorcism);             /* slot 711 */
DECLARE_SPELL_FUN(spell_prayer);               /* slot 712 */
DECLARE_SPELL_FUN(spell_astral_projection);    /* slot 713 */
DECLARE_SPELL_FUN(spell_pain);                 /* slot 714 */
DECLARE_SPELL_FUN(spell_thoughtsteal);         /* slot 715 */
DECLARE_SPELL_FUN(spell_precognition);         /* slot 716 */
DECLARE_SPELL_FUN(spell_mindwall);             /* slot 717 */
DECLARE_SPELL_FUN(spell_elemental_confluence); /* slot 760 */
DECLARE_SPELL_FUN(spell_convergence_shield);   /* slot 761 */
DECLARE_SPELL_FUN(spell_sacred_ward);          /* slot 762 */
DECLARE_SPELL_FUN(spell_momentum_drain);       /* slot 763 */
DECLARE_SPELL_FUN(spell_entropic_shield);      /* slot 764 */
DECLARE_SPELL_FUN(spell_cognitive_disruption); /* slot 765 */
DECLARE_SPELL_FUN(spell_predictive_collapse);  /* slot 766 */
DECLARE_SPELL_FUN(spell_psishield);            /* slot 767 */
DECLARE_SPELL_FUN(spell_arcaneshield);         /* slot 768 */
DECLARE_SPELL_FUN(spell_cloak_precision);      /* slot 769 */
DECLARE_SPELL_FUN(spell_holyshield);           /* slot 770 */
DECLARE_SPELL_FUN(spell_cloak_valor);          /* slot 771 */

/* Druid spells (slots 800+) */
DECLARE_SPELL_FUN(spell_thornlash);
DECLARE_SPELL_FUN(spell_nettlesting);
DECLARE_SPELL_FUN(spell_rockburst);
DECLARE_SPELL_FUN(spell_wither);
DECLARE_SPELL_FUN(spell_briar_volley);
DECLARE_SPELL_FUN(spell_blight_touch);
DECLARE_SPELL_FUN(spell_tremor);
DECLARE_SPELL_FUN(spell_creeping_rot);
DECLARE_SPELL_FUN(spell_fissure);
DECLARE_SPELL_FUN(spell_fungal_eruption);
DECLARE_SPELL_FUN(spell_quake);
DECLARE_SPELL_FUN(spell_strangling_vines);
DECLARE_SPELL_FUN(spell_rot);
DECLARE_SPELL_FUN(spell_upheaval);
DECLARE_SPELL_FUN(spell_sap_mend);
DECLARE_SPELL_FUN(spell_mend_wounds);
DECLARE_SPELL_FUN(spell_purge_toxin);
DECLARE_SPELL_FUN(spell_verdant_mend);
DECLARE_SPELL_FUN(spell_heartwood_balm);
DECLARE_SPELL_FUN(spell_primal_restoration);
DECLARE_SPELL_FUN(spell_sap_transfusion);
DECLARE_SPELL_FUN(spell_barkskin);
DECLARE_SPELL_FUN(spell_roothold);
DECLARE_SPELL_FUN(spell_thornhide);
DECLARE_SPELL_FUN(spell_ironpine_vigor);
DECLARE_SPELL_FUN(spell_gift_of_the_forest);
DECLARE_SPELL_FUN(spell_natures_ward);
DECLARE_SPELL_FUN(spell_ancient_bark);
DECLARE_SPELL_FUN(spell_tangling_roots);
DECLARE_SPELL_FUN(spell_sap_strength);
DECLARE_SPELL_FUN(spell_spore_cloud);
DECLARE_SPELL_FUN(spell_wither_limbs);
DECLARE_SPELL_FUN(spell_entangling_growth);
DECLARE_SPELL_FUN(spell_curse_of_decay);
DECLARE_SPELL_FUN(spell_creeping_blight);
DECLARE_SPELL_FUN(spell_forestsight);
DECLARE_SPELL_FUN(spell_sense_life);
DECLARE_SPELL_FUN(spell_know_terrain);
DECLARE_SPELL_FUN(spell_passage);
DECLARE_SPELL_FUN(spell_greensight);
DECLARE_SPELL_FUN(spell_rootway);
DECLARE_SPELL_FUN(spell_call_of_the_wild);
DECLARE_SPELL_FUN(spell_living_bridge);

/* Level 1 mortal class active spells (slots 843-846) */
DECLARE_SPELL_FUN(spell_ember_spark);        /* slot 843 — Magi */
DECLARE_SPELL_FUN(spell_solar_brand);        /* slot 844 — Cleric */
DECLARE_SPELL_FUN(spell_cognitive_pressure); /* slot 845 — Psionicist */
DECLARE_SPELL_FUN(spell_root_touch);         /* slot 846 — Druid */
