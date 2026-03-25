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
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/
#define DEC_GLOBALS_H 1

/*
 * Sentinel: set immediately so that ack.h's end-of-file inlines_globals.h
 * include can detect that globals.h was actually loaded (vs. pre-suppressed
 * by unit tests via `#define DEC_GLOBALS_H 1` before `#include "ack.h"`).
 */
#define DEC_GLOBALS_H_LOADED 1

#ifndef DEC_TYPEDEFS_H
#include "typedefs.h"
#endif

#ifndef DEC_CONFIG_H
#include "config.h"
#endif

#ifndef DEC_ACK_H
#include "ack.h"
#endif

/*
 * These are skill_lookup return values for common skills and spells.
 */
extern sh_int gsn_martial_arts; /* For unarmed fighting */
extern sh_int gsn_dualwield;
extern sh_int gsn_stun;
extern sh_int gsn_shadow;
extern sh_int gsn_climb;
extern sh_int gsn_find_doors;
extern sh_int gsn_smash;
extern sh_int gsn_notrip;
extern sh_int gsn_nodisarm;
extern sh_int gsn_trip;
extern sh_int gsn_dirt;
extern sh_int gsn_bash;

extern sh_int gsn_fourth_attack;
extern sh_int gsn_shield_block;
extern sh_int gsn_beserk;
extern sh_int gsn_counter;
extern sh_int gsn_enhanced_heal;
extern sh_int gsn_enhanced_purify;
extern sh_int gsn_enhanced_critical;
extern sh_int gsn_spell_critical;
extern sh_int gsn_spell_critical_damage;
extern sh_int gsn_detox;
extern sh_int gsn_enhanced_sword;
extern sh_int gsn_weapon_bond;
extern sh_int gsn_enhanced_sword_critical;
extern sh_int gsn_two_handed;
extern sh_int gsn_bare_hand;
extern sh_int gsn_palmstrike;

extern sh_int gsn_equip_buckler;
extern sh_int gsn_equip_fist;
extern sh_int gsn_dual_fist;
extern sh_int gsn_equip_wand;

extern sh_int gsn_flee_timer;

extern sh_int gsn_circle;
extern sh_int gsn_backstab;
extern sh_int gsn_poison_quinine;
extern sh_int gsn_poison_arsenic;
extern sh_int gsn_poison_nightshade;
extern sh_int gsn_dodge;
extern sh_int gsn_riposte;
extern sh_int gsn_mindoverbody;
extern sh_int gsn_flurry;
extern sh_int gsn_chakra;
extern sh_int gsn_phantomfist;
extern sh_int gsn_aurabolt;
extern sh_int gsn_cripple;
extern sh_int gsn_pummel;
extern sh_int gsn_shieldblock;
extern sh_int gsn_chiblock;
extern sh_int gsn_holystrike;
extern sh_int gsn_anti_magic_shell;
extern sh_int gsn_chi_surge;
extern sh_int gsn_breath_of_endurance;
extern sh_int gsn_fist_interior;
extern sh_int gsn_momentum_chain;
extern sh_int gsn_veterans_cadence;
extern sh_int gsn_overwhelming_assault;
extern sh_int gsn_elemental_attunement;
extern sh_int gsn_oathshield;
extern sh_int gsn_sanctified_strike;
extern sh_int gsn_shadow_reading;
extern sh_int gsn_hex_ward;
extern sh_int gsn_reflex_disruption;
extern sh_int gsn_fleche;
extern sh_int gsn_hide;
extern sh_int gsn_hunt;
extern sh_int gsn_peek;
extern sh_int gsn_pick_lock;
extern sh_int gsn_sneak;
extern sh_int gsn_steal;

extern sh_int gsn_morale;
extern sh_int gsn_leadership;

extern sh_int gsn_punch;
extern sh_int gsn_headbutt;
extern sh_int gsn_knee;

extern sh_int gsn_disarm;
extern sh_int gsn_enhanced_damage;
extern sh_int gsn_kick;
extern sh_int gsn_parry;
extern sh_int gsn_rescue;
extern sh_int gsn_second_attack;
extern sh_int gsn_third_attack;

extern sh_int gsn_blindness;
extern sh_int gsn_charm_person;
extern sh_int gsn_curse;
extern sh_int gsn_invis;
extern sh_int gsn_mass_invis;
extern sh_int gsn_poison;
extern sh_int gsn_sleep;
extern sh_int gsn_disguise;
extern sh_int gsn_teach;
extern sh_int gsn_adrenaline;
extern sh_int gsn_frenzy;
extern sh_int gsn_emotion_control;
extern sh_int gsn_fifth_attack;
extern sh_int gsn_sixth_attack;
extern sh_int gsn_target;
extern sh_int gsn_charge;
extern sh_int gsn_unit_tactics;
extern sh_int gsn_spinfist;

extern sh_int gsn_scout;
extern sh_int gsn_potency;
extern sh_int gsn_thaumatergy;
extern sh_int gsn_warcry;

/* Cipher skills */
extern sh_int gsn_garrote;
extern sh_int gsn_feign_death;
extern sh_int gsn_shadow_step;
extern sh_int gsn_mark_target;
extern sh_int gsn_conceal;
extern sh_int gsn_set_trap;
extern sh_int gsn_read_intent;

/* Warden skills */
extern sh_int gsn_cleave;
extern sh_int gsn_fortify;
extern sh_int gsn_taunt;
extern sh_int gsn_rend;
extern sh_int gsn_field_patch;
extern sh_int gsn_weapon_mastery;
extern sh_int gsn_ashfall_strike;

/* Druid skills */
extern sh_int gsn_substrate_piercing;

/* Pugilist skills */
extern sh_int gsn_grapple;
extern sh_int gsn_choke_hold;
extern sh_int gsn_leg_sweep;
extern sh_int gsn_iron_skin;
extern sh_int gsn_roll_with_blow;
extern sh_int gsn_pressure_point;
extern sh_int gsn_feint;
extern sh_int gsn_conditioning;
extern sh_int gsn_harbor_dust;

/* Sentinel skills */
extern sh_int gsn_vigilance;
extern sh_int gsn_verdict;
extern sh_int gsn_read_opponent;
extern sh_int gsn_binding_strike;
extern sh_int gsn_testimonial_guard;
extern sh_int gsn_ninth_descent;
extern sh_int gsn_measured_response;
extern sh_int gsn_condemn;
extern sh_int gsn_seal_testimony;

/* Justicar skills */
extern sh_int gsn_formal_sentencing;
extern sh_int gsn_executioners_strike;
extern sh_int gsn_second_hearing;
extern sh_int gsn_severity_escalation;
extern sh_int gsn_writ_of_execution;
extern sh_int gsn_read_weakness;
extern sh_int gsn_inevitable_verdict;

/* Arbiter skills */
extern sh_int gsn_procedural_authority;
extern sh_int gsn_sustained_objection;
extern sh_int gsn_cross_examination;
extern sh_int gsn_judicial_immunity;
extern sh_int gsn_appellate_review;
extern sh_int gsn_suppression_order;
extern sh_int gsn_contempt_of_court;

/* Inquisitor skills */
extern sh_int gsn_full_tribunal;
extern sh_int gsn_inquisition;
extern sh_int gsn_absolute_verdict;
extern sh_int gsn_precedent;
extern sh_int gsn_sovereign_authority;
extern sh_int gsn_seal_of_the_tribunal;

/* Public society skills */
extern sh_int gsn_patrol_sense;
extern sh_int gsn_field_mend;
extern sh_int gsn_rally_cry;
extern sh_int gsn_shield_wall;
extern sh_int gsn_tactical_assess;
extern sh_int gsn_command_presence;
extern sh_int gsn_dockside_audit;
extern sh_int gsn_haggle;
extern sh_int gsn_longshoreman;
extern sh_int gsn_bonded_cargo;
extern sh_int gsn_anchor_slam;
extern sh_int gsn_forage;
extern sh_int gsn_field_repair;
extern sh_int gsn_ember_signal;
extern sh_int gsn_ember_vitality;
extern sh_int gsn_containment_ward;
extern sh_int gsn_controlled_burn;
extern sh_int gsn_oath_mark;
extern sh_int gsn_track_quarry;
extern sh_int gsn_writ_of_hold;
extern sh_int gsn_marked_strike;
extern sh_int gsn_claws_pursuit;
extern sh_int gsn_claws_judgment;
extern sh_int gsn_dig_in;
extern sh_int gsn_shelter;
extern sh_int gsn_trailblaze;
extern sh_int gsn_bulwark;
extern sh_int gsn_signal_tower;
extern sh_int gsn_rampart_slam;
extern sh_int gsn_wayfind;
extern sh_int gsn_road_camp;
extern sh_int gsn_forced_march;
extern sh_int gsn_road_grit;
extern sh_int gsn_dispatch_rider;
extern sh_int gsn_waylay;

/* Level 1 active skills for mortal skill-based classes */
extern sh_int gsn_duty_strike;
extern sh_int gsn_gap_probe;
extern sh_int gsn_primary_motion;
extern sh_int gsn_vigil_strike;

#ifdef TFS
extern sh_int gsn_mana_sense;
#endif

extern bool booting_up;
extern bool area_resetting_global;
extern bool deathmatch;
extern bool wizlock;
extern char *const dir_name[];
extern char *const rev_name[];

extern int max_players;
extern int happy_hour_ticks_remaining;
extern int hotreboot_countdown;

extern bool happy_hour;

/*
 * Global constants.
 */
extern const struct color_type color_table[MAX_COLOR];
extern const struct ansi_type ansi_table[MAX_ANSI];
extern const struct class_type gclass_table[MAX_TOTAL_CLASS];
extern const struct race_type race_table[MAX_RACE];
extern const struct exp_type exp_table[MAX_MOB_LEVEL];
extern const struct clan_type clan_table[MAX_CLAN];
extern const struct cmd_type cmd_table[];

extern const struct skill_type skill_table[MAX_SKILL];
/* Lua script sources, indexed by sn; NULL means use the C spell_fun/do_fun. */
extern char *skill_scripts[MAX_SKILL];
extern struct pub_society_type
    pub_society_table[]; /* sized MAX_PUB_SOCIETY in const_pub_society.c */
extern struct social_type *social_table;
extern char *const title_table[MAX_CLASS][36][2];

/* spec: log all calls to getmem/dispose when set */
extern bool mem_log;
extern const struct stance_app_type stance_app[MAX_STANCE];

/*
 * Global variables.
 */
extern char bug_buf[];
extern time_t current_time;
extern bool fLogAll;
extern FILE *fpReserve;
extern KILL_DATA kill_table[];
extern char log_buf[];
extern char testerbuf[];
extern TIME_INFO_DATA time_info;
extern WEATHER_DATA weather_info;
extern DESCRIPTOR_DATA *descriptor_list;
extern CHAR_DATA *char_list;
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern SYS_DATA_TYPE sysdata;

/* Auction globals (defined in db.c). */
extern OBJ_DATA *auction_item;
extern CHAR_DATA *auction_owner;
extern CHAR_DATA *auction_bidder;
extern int auction_bid;
extern int auction_reserve;
extern int auction_stage;
extern bool auction_flop;

/* Quest globals (defined in db.c). */
extern bool quest;
extern bool auto_quest;
extern CHAR_DATA *quest_mob;
extern CHAR_DATA *quest_target;
extern OBJ_DATA *quest_object;
extern int quest_timer;
extern int quest_wait;
extern sh_int quest_personality;
extern int quest_level_min;
extern int quest_level_max;
extern bool quest_ai_pending;      /* AI generation in flight?        */
extern char quest_ai_tmpfile[256]; /* Temp file path for AI output    */
extern pid_t quest_ai_pid;         /* Child process PID               */
extern time_t quest_ai_start_time; /* Time child was launched         */

/* Diplomatics globals (defined in db.c). */
extern POL_DATA politics_data;
extern COUNCIL_DATA super_councils[];

/* db.c globals exposed for the save module and other subsystems. */
extern char str_empty[];
extern bool fBootDb;
extern FILE *fpArea;
extern char strArea[];
extern int area_revision;
extern AREA_DATA *area_load;
extern AREA_DATA *area_used[];
extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_exit;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_reset;
extern int top_room;
extern int top_shop;

/* Common combat/stat helpers shared across modules. */
long get_max_hp args((CHAR_DATA * ch));
long get_max_mana args((CHAR_DATA * ch));
long get_max_move args((CHAR_DATA * ch));
int get_total_reincarnations args((CHAR_DATA * ch));
bool is_fighting args((CHAR_DATA * ch));
void heal_character(CHAR_DATA *ch, CHAR_DATA *victim, int base_heal, int sn, bool hot);
void set_fighting args((CHAR_DATA * ch, CHAR_DATA *victim, bool check));
int get_reincarnate_hp_level(CHAR_DATA *ch);
int get_reincarnate_mana_level(CHAR_DATA *ch);
int get_reincarnate_move_level(CHAR_DATA *ch);
void obj_damage(OBJ_DATA *obj, CHAR_DATA *victim, int dam);

/* YUCK! */
extern char *target_name;

/* board junk */
extern BOARD_DATA *first_board;
extern BOARD_DATA *last_board;
extern BOARD_DATA *board_free;
extern MESSAGE_DATA *message_free;

/*
 * Command functions.
 * Defined in act_*.c (mostly).
 */

/**************************
 * This bunch of commands *
 * added by Stephen ;)    *
 **************************/

DECLARE_DO_FUN(do_test);

DECLARE_DO_FUN(do_exlist);
DECLARE_DO_FUN(do_vlist);
DECLARE_DO_FUN(do_rename);

DECLARE_DO_FUN(do_sdelete);

DECLARE_DO_FUN(do_adapt);
DECLARE_DO_FUN(do_affected);
DECLARE_DO_FUN(do_afk);
DECLARE_DO_FUN(do_alias);
DECLARE_DO_FUN(do_appraise);
DECLARE_DO_FUN(build_arealist);
DECLARE_DO_FUN(do_ask);
DECLARE_DO_FUN(do_assassinate);
DECLARE_DO_FUN(do_assist);
DECLARE_DO_FUN(do_bamfin);
DECLARE_DO_FUN(do_bamfout);
DECLARE_DO_FUN(do_bank);
DECLARE_DO_FUN(do_bash);
DECLARE_DO_FUN(do_beep);
DECLARE_DO_FUN(do_beserk);
DECLARE_DO_FUN(do_bid);
DECLARE_DO_FUN(do_rbid);
DECLARE_DO_FUN(do_mbid);
DECLARE_DO_FUN(do_cdonate);
DECLARE_DO_FUN(do_circle);
DECLARE_DO_FUN(do_clan);
DECLARE_DO_FUN(do_claneq);
DECLARE_DO_FUN(do_clan_list);
DECLARE_DO_FUN(do_clan_recall);
DECLARE_DO_FUN(do_clutch);
DECLARE_DO_FUN(do_clutchinfo);
DECLARE_DO_FUN(do_colist);
DECLARE_DO_FUN(do_color);
DECLARE_DO_FUN(do_creator);
DECLARE_DO_FUN(do_cset);
DECLARE_DO_FUN(do_deathmatc);
DECLARE_DO_FUN(do_deathmatch);
DECLARE_DO_FUN(do_diagnose);
DECLARE_DO_FUN(do_dirt);
DECLARE_DO_FUN(do_ashfall_strike);
DECLARE_DO_FUN(do_harbor_dust);
DECLARE_DO_FUN(do_dog);
DECLARE_DO_FUN(do_donate);
DECLARE_DO_FUN(do_edit);
DECLARE_DO_FUN(do_enter);
DECLARE_DO_FUN(do_fights);
DECLARE_DO_FUN(do_finger);
DECLARE_DO_FUN(do_flame);
DECLARE_DO_FUN(do_gain);
DECLARE_DO_FUN(do_gold);
DECLARE_DO_FUN(do_gossip);
DECLARE_DO_FUN(do_ooc);
DECLARE_DO_FUN(do_espanol);
DECLARE_DO_FUN(do_francais);
DECLARE_DO_FUN(do_invasion_talk);
DECLARE_DO_FUN(do_guild);
DECLARE_DO_FUN(do_halls);
DECLARE_DO_FUN(do_headbutt);
DECLARE_DO_FUN(do_knee);
DECLARE_DO_FUN(do_keep);
DECLARE_DO_FUN(do_detox);
DECLARE_DO_FUN(do_heal);
DECLARE_DO_FUN(do_hunt);
DECLARE_DO_FUN(do_palmstrike);
DECLARE_DO_FUN(do_resetpassword);
DECLARE_DO_FUN(do_staffscore);
DECLARE_DO_FUN(do_isnoop);
DECLARE_DO_FUN(do_staffwhere);
DECLARE_DO_FUN(do_leav);
DECLARE_DO_FUN(do_leave);
DECLARE_DO_FUN(do_lhunt);
DECLARE_DO_FUN(do_monitor);
DECLARE_DO_FUN(do_mpcr);
DECLARE_DO_FUN(do_music);
DECLARE_DO_FUN(do_newbie);
DECLARE_DO_FUN(do_nopray);
DECLARE_DO_FUN(do_pemote);
DECLARE_DO_FUN(do_players);
DECLARE_DO_FUN(do_pray);
DECLARE_DO_FUN(do_punch);
DECLARE_DO_FUN(do_quest);
DECLARE_DO_FUN(do_staffquest);
DECLARE_DO_FUN(do_race);
DECLARE_DO_FUN(do_race_list);
DECLARE_DO_FUN(do_retrieve);
DECLARE_DO_FUN(do_scan);
DECLARE_DO_FUN(do_setclass);
DECLARE_DO_FUN(do_smash);
DECLARE_DO_FUN(do_shadowform);
DECLARE_DO_FUN(do_shelp);
DECLARE_DO_FUN(do_sstat);
DECLARE_DO_FUN(do_status);
DECLARE_DO_FUN(do_stun);
DECLARE_DO_FUN(do_togbuild);
DECLARE_DO_FUN(do_togleader);
DECLARE_DO_FUN(do_tongue);
DECLARE_DO_FUN(do_trip);
DECLARE_DO_FUN(do_whisper);
DECLARE_DO_FUN(do_whois);
DECLARE_DO_FUN(do_whoname);
DECLARE_DO_FUN(do_worth);
DECLARE_DO_FUN(do_zzz);
DECLARE_DO_FUN(do_listspells);
DECLARE_DO_FUN(do_reward);
DECLARE_DO_FUN(do_togcouncil);
DECLARE_DO_FUN(do_rename);
DECLARE_DO_FUN(do_sdelete);
DECLARE_DO_FUN(do_warcry);
DECLARE_DO_FUN(do_riposte);
DECLARE_DO_FUN(do_mindoverbody);
DECLARE_DO_FUN(do_phantomfist);
DECLARE_DO_FUN(do_flurry);
DECLARE_DO_FUN(do_chakra);
DECLARE_DO_FUN(do_anti_magic_shell);
DECLARE_DO_FUN(do_chi_surge);
DECLARE_DO_FUN(do_breath_of_endurance);
DECLARE_DO_FUN(do_fist_of_the_interior_form);
DECLARE_DO_FUN(do_momentum_chain);
DECLARE_DO_FUN(do_veterans_cadence);
DECLARE_DO_FUN(do_overwhelming_assault);
DECLARE_DO_FUN(do_oathshield);
DECLARE_DO_FUN(do_sanctified_strike);
DECLARE_DO_FUN(do_shadow_reading);
DECLARE_DO_FUN(do_hex_ward);
DECLARE_DO_FUN(do_reflex_disruption);
DECLARE_DO_FUN(do_holystrike);
DECLARE_DO_FUN(do_fleche);
DECLARE_DO_FUN(do_shieldblock);
DECLARE_DO_FUN(do_chiblock);
DECLARE_DO_FUN(do_spinfist);
DECLARE_DO_FUN(do_pummel);
DECLARE_DO_FUN(do_aurabolt);

/* And by MAG
 *
 */
DECLARE_DO_FUN(build_interpret);
DECLARE_DO_FUN(do_build);
DECLARE_DO_FUN(do_delete);
DECLARE_DO_FUN(do_read);
DECLARE_DO_FUN(do_savearea);
DECLARE_DO_FUN(do_write);
DECLARE_DO_FUN(do_mfindlev);
DECLARE_DO_FUN(do_check_areas);
DECLARE_DO_FUN(do_check_area);

DECLARE_DO_FUN(do_ofindlev);

/****************************
 * Now the 'standard' stuff:*
 ****************************/

DECLARE_DO_FUN(do_advance);
DECLARE_DO_FUN(do_allow);
DECLARE_DO_FUN(do_answer);
DECLARE_DO_FUN(do_areas);
DECLARE_DO_FUN(do_at);
DECLARE_DO_FUN(do_auction);
DECLARE_DO_FUN(do_rauction);
DECLARE_DO_FUN(do_mauction);
DECLARE_DO_FUN(do_auto);
DECLARE_DO_FUN(do_autoexit);
DECLARE_DO_FUN(do_autoloot);
DECLARE_DO_FUN(do_autosac);
DECLARE_DO_FUN(do_autosplit);
DECLARE_DO_FUN(do_autoassist);
DECLARE_DO_FUN(do_backstab);
DECLARE_DO_FUN(do_poison_quinine);
DECLARE_DO_FUN(do_poison_arsenic);
DECLARE_DO_FUN(do_poison_nightshade);
DECLARE_DO_FUN(do_morale);
DECLARE_DO_FUN(do_leadership);
DECLARE_DO_FUN(do_ban);
DECLARE_DO_FUN(do_blank);
DECLARE_DO_FUN(do_brandish);
DECLARE_DO_FUN(do_brief);
DECLARE_DO_FUN(do_bug);
DECLARE_DO_FUN(do_buy);
DECLARE_DO_FUN(do_cast);
DECLARE_DO_FUN(do_channels);
DECLARE_DO_FUN(do_close);
DECLARE_DO_FUN(do_combine);
DECLARE_DO_FUN(do_commands);
DECLARE_DO_FUN(do_compare);
DECLARE_DO_FUN(do_config);
DECLARE_DO_FUN(do_consider);
DECLARE_DO_FUN(do_credits);
DECLARE_DO_FUN(do_deny);
DECLARE_DO_FUN(do_description);
DECLARE_DO_FUN(do_disarm);
DECLARE_DO_FUN(do_disconnect);
DECLARE_DO_FUN(do_down);
DECLARE_DO_FUN(do_drop);
DECLARE_DO_FUN(do_east);
DECLARE_DO_FUN(do_eat);
DECLARE_DO_FUN(do_echo);
DECLARE_DO_FUN(do_emote);
DECLARE_DO_FUN(do_equipment);
DECLARE_DO_FUN(do_examine);
DECLARE_DO_FUN(do_exits);
DECLARE_DO_FUN(do_flee);
DECLARE_DO_FUN(do_follow);
DECLARE_DO_FUN(do_force);
DECLARE_DO_FUN(do_freeze);
DECLARE_DO_FUN(do_game);
DECLARE_DO_FUN(do_get);
DECLARE_DO_FUN(do_give);
DECLARE_DO_FUN(do_goto);
DECLARE_DO_FUN(do_group);
DECLARE_DO_FUN(do_gtell);
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_rhelp);
DECLARE_DO_FUN(do_hide);
DECLARE_DO_FUN(do_holylight);
DECLARE_DO_FUN(do_idea);
DECLARE_DO_FUN(do_stafftalk);
DECLARE_DO_FUN(do_incognito);
DECLARE_DO_FUN(do_inventory);
DECLARE_DO_FUN(do_invis);
DECLARE_DO_FUN(do_kick);
DECLARE_DO_FUN(do_kill);
DECLARE_DO_FUN(do_list);
DECLARE_DO_FUN(do_lock);
DECLARE_DO_FUN(do_log);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_loot);
DECLARE_DO_FUN(do_lore);
DECLARE_DO_FUN(do_memory);
DECLARE_DO_FUN(do_mfind);
DECLARE_DO_FUN(do_mload);
DECLARE_DO_FUN(do_mset);
DECLARE_DO_FUN(do_mstat);
DECLARE_DO_FUN(do_mwhere);
DECLARE_DO_FUN(do_murde);
DECLARE_DO_FUN(do_murder);
DECLARE_DO_FUN(do_noemote);
DECLARE_DO_FUN(do_north);
DECLARE_DO_FUN(do_note);
DECLARE_DO_FUN(do_notell);
DECLARE_DO_FUN(do_ofind);
DECLARE_DO_FUN(do_oload);
DECLARE_DO_FUN(do_open);
DECLARE_DO_FUN(do_oset);
DECLARE_DO_FUN(do_ostat);
DECLARE_DO_FUN(do_owhere);
DECLARE_DO_FUN(do_pagelen);
DECLARE_DO_FUN(do_pardon);
DECLARE_DO_FUN(do_password);
DECLARE_DO_FUN(do_peace);
DECLARE_DO_FUN(do_pick);
DECLARE_DO_FUN(do_pose);
DECLARE_DO_FUN(do_practice);
DECLARE_DO_FUN(do_prompt);
DECLARE_DO_FUN(do_purge);
DECLARE_DO_FUN(do_put);
DECLARE_DO_FUN(do_quaff);
DECLARE_DO_FUN(do_question);
DECLARE_DO_FUN(do_invasion_talk);
DECLARE_DO_FUN(do_qui);
DECLARE_DO_FUN(do_quit);
DECLARE_DO_FUN(do_reboo);
DECLARE_DO_FUN(do_reboot);
DECLARE_DO_FUN(do_recall);
DECLARE_DO_FUN(do_caravan);
DECLARE_DO_FUN(do_recho);
DECLARE_DO_FUN(do_recite);
DECLARE_DO_FUN(do_remove);
DECLARE_DO_FUN(do_rent);
DECLARE_DO_FUN(do_reply);
DECLARE_DO_FUN(do_report);
DECLARE_DO_FUN(do_rescue);
DECLARE_DO_FUN(do_rest);
DECLARE_DO_FUN(do_restore);
DECLARE_DO_FUN(do_return);
DECLARE_DO_FUN(do_rset);
DECLARE_DO_FUN(do_rstat);
DECLARE_DO_FUN(do_sacrifice);
DECLARE_DO_FUN(do_junk);
DECLARE_DO_FUN(do_save);
DECLARE_DO_FUN(do_say);
DECLARE_DO_FUN(do_score);
DECLARE_DO_FUN(do_sell);
DECLARE_DO_FUN(do_shout);
DECLARE_DO_FUN(do_shutdow);
DECLARE_DO_FUN(do_shutdown);
DECLARE_DO_FUN(do_silence);
DECLARE_DO_FUN(do_sla);
DECLARE_DO_FUN(do_slay);
DECLARE_DO_FUN(do_sleep);
DECLARE_DO_FUN(do_slist);
DECLARE_DO_FUN(do_slookup);
DECLARE_DO_FUN(do_sneak);
DECLARE_DO_FUN(do_snoop);
DECLARE_DO_FUN(do_socials);
DECLARE_DO_FUN(do_south);
DECLARE_DO_FUN(do_spells);
DECLARE_DO_FUN(do_split);
DECLARE_DO_FUN(do_sset);
DECLARE_DO_FUN(do_stand);
DECLARE_DO_FUN(do_steal);
DECLARE_DO_FUN(do_switch);
DECLARE_DO_FUN(do_tell);
DECLARE_DO_FUN(do_time);
DECLARE_DO_FUN(do_title);
DECLARE_DO_FUN(do_train);
DECLARE_DO_FUN(do_transfer);
DECLARE_DO_FUN(do_transdm);
DECLARE_DO_FUN(do_trust);
DECLARE_DO_FUN(do_typo);
DECLARE_DO_FUN(do_unlock);
DECLARE_DO_FUN(do_up);
DECLARE_DO_FUN(do_users);
DECLARE_DO_FUN(do_value);
DECLARE_DO_FUN(do_visible);
DECLARE_DO_FUN(do_wake);
DECLARE_DO_FUN(do_wear);
DECLARE_DO_FUN(do_weather);
DECLARE_DO_FUN(do_west);
DECLARE_DO_FUN(do_where);
DECLARE_DO_FUN(do_who);
DECLARE_DO_FUN(do_wimpy);
DECLARE_DO_FUN(do_wizhelp);
DECLARE_DO_FUN(do_wizify);
DECLARE_DO_FUN(do_wizlist);
DECLARE_DO_FUN(do_wizlock);
DECLARE_DO_FUN(do_yell);
/*DECLARE_DO_FUN( do_zap );*/

/* from Titania@AA/Sylvana@LCN */
DECLARE_DO_FUN(do_otype);
DECLARE_DO_FUN(do_owear);

/* Added by Eligan, July 10 */

DECLARE_DO_FUN(do_ignore);

/*
      NEW SKILLS  (SRZ)

*/
DECLARE_DO_FUN(do_qpspend);
DECLARE_DO_FUN(do_revenant);
DECLARE_DO_FUN(do_bond);
DECLARE_DO_FUN(do_disguise);
DECLARE_DO_FUN(do_frenzy);
DECLARE_DO_FUN(do_adrenaline);
DECLARE_DO_FUN(do_target);
DECLARE_DO_FUN(do_charge);
DECLARE_DO_FUN(do_connect);

/* Cipher skills */
DECLARE_DO_FUN(do_garrote);
DECLARE_DO_FUN(do_feign_death);
DECLARE_DO_FUN(do_shadow_step);
DECLARE_DO_FUN(do_mark_target);
DECLARE_DO_FUN(do_conceal);
DECLARE_DO_FUN(do_set_trap);
DECLARE_DO_FUN(do_read_intent);

/* Warden skills */
DECLARE_DO_FUN(do_cleave);
DECLARE_DO_FUN(do_fortify);
DECLARE_DO_FUN(do_taunt);
DECLARE_DO_FUN(do_rend);
DECLARE_DO_FUN(do_field_patch);
DECLARE_DO_FUN(do_weapon_mastery);

/* Pugilist skills */
DECLARE_DO_FUN(do_grapple);
DECLARE_DO_FUN(do_choke_hold);
DECLARE_DO_FUN(do_leg_sweep);
DECLARE_DO_FUN(do_iron_skin);
DECLARE_DO_FUN(do_roll_with_blow);
DECLARE_DO_FUN(do_pressure_point);
DECLARE_DO_FUN(do_feint);
DECLARE_DO_FUN(do_conditioning);

/* Sentinel skills */
DECLARE_DO_FUN(do_verdict);
DECLARE_DO_FUN(do_read_opponent);
DECLARE_DO_FUN(do_binding_strike);
DECLARE_DO_FUN(do_ninth_descent);
DECLARE_DO_FUN(do_condemn);
DECLARE_DO_FUN(do_seal_testimony);

/* Justicar skills */
DECLARE_DO_FUN(do_formal_sentencing);
DECLARE_DO_FUN(do_executioners_strike);
DECLARE_DO_FUN(do_writ_of_execution);
DECLARE_DO_FUN(do_read_weakness);

/* Arbiter skills */
DECLARE_DO_FUN(do_procedural_authority);
DECLARE_DO_FUN(do_sustained_objection);
DECLARE_DO_FUN(do_judicial_immunity);
DECLARE_DO_FUN(do_appellate_review);
DECLARE_DO_FUN(do_suppression_order);

/* Inquisitor skills */
DECLARE_DO_FUN(do_inquisition);
DECLARE_DO_FUN(do_absolute_verdict);
DECLARE_DO_FUN(do_sovereign_authority);
DECLARE_DO_FUN(do_seal_of_the_tribunal);

DECLARE_DO_FUN(do_stance);
DECLARE_DO_FUN(do_enchant);

/*
DECLARE_DO_FUN( do_teach      );
*/

/*
 *   New Channels (ZEN)
 */

DECLARE_DO_FUN(do_diptalk);
DECLARE_DO_FUN(do_remorttalk);
DECLARE_DO_FUN(do_crusade);
DECLARE_DO_FUN(do_adepttalk); /* contrib by Hermes   */
DECLARE_DO_FUN(do_rulers);
DECLARE_DO_FUN(do_scout);

/*
 *  New staff commands
 */

DECLARE_DO_FUN(do_alink);
DECLARE_DO_FUN(do_for);       /* Flar */
DECLARE_DO_FUN(do_hotreboo);  /* Flar */
DECLARE_DO_FUN(do_hotreboot); /* Flar */
DECLARE_DO_FUN(do_cancelhotreboot);
DECLARE_DO_FUN(do_staffset); /* Flar */
DECLARE_DO_FUN(do_gain_stat_reset);
DECLARE_DO_FUN(do_sedit);
DECLARE_DO_FUN(do_olmsg);
DECLARE_DO_FUN(do_scheck);
DECLARE_DO_FUN(do_staffbrand);
DECLARE_DO_FUN(do_sysdata);
DECLARE_DO_FUN(do_areasave);
DECLARE_DO_FUN(do_mapper);
DECLARE_DO_FUN(do_mgive);
DECLARE_DO_FUN(do_email);
DECLARE_DO_FUN(do_findreset);

/* Reincarnate */
DECLARE_DO_FUN(do_reincarnate);

/* Public societies */
DECLARE_DO_FUN(do_enlist);
DECLARE_DO_FUN(do_society);
DECLARE_DO_FUN(do_patrol_sense);
DECLARE_DO_FUN(do_field_mend);
DECLARE_DO_FUN(do_rally_cry);
DECLARE_DO_FUN(do_tactical_assess);
DECLARE_DO_FUN(do_command_presence);
DECLARE_DO_FUN(do_dockside_audit);
DECLARE_DO_FUN(do_haggle);
DECLARE_DO_FUN(do_bonded_cargo);
DECLARE_DO_FUN(do_anchor_slam);
DECLARE_DO_FUN(do_forage);
DECLARE_DO_FUN(do_field_repair);
DECLARE_DO_FUN(do_ember_signal);
DECLARE_DO_FUN(do_containment_ward);
DECLARE_DO_FUN(do_controlled_burn);
DECLARE_DO_FUN(do_oath_mark);
DECLARE_DO_FUN(do_track_quarry);
DECLARE_DO_FUN(do_writ_of_hold);
DECLARE_DO_FUN(do_claws_pursuit);
DECLARE_DO_FUN(do_claws_judgment);
DECLARE_DO_FUN(do_dig_in);
DECLARE_DO_FUN(do_shelter);
DECLARE_DO_FUN(do_trailblaze);
DECLARE_DO_FUN(do_signal_tower);
DECLARE_DO_FUN(do_rampart_slam);
DECLARE_DO_FUN(do_wayfind);
DECLARE_DO_FUN(do_road_camp);
DECLARE_DO_FUN(do_forced_march);
DECLARE_DO_FUN(do_dispatch_rider);
DECLARE_DO_FUN(do_waylay);
DECLARE_DO_FUN(do_duty_strike);
DECLARE_DO_FUN(do_gap_probe);
DECLARE_DO_FUN(do_primary_motion);
DECLARE_DO_FUN(do_vigil_strike);

bool is_player_summon_special(SPEC_FUN *spec_fun);
