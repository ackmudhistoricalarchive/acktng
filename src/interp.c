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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "globals.h"

#ifndef DEC_MAGIC_H
#include "magic.h"
#endif

bool check_social args((CHAR_DATA * ch, char *command, char *argument));
bool MP_Commands args((CHAR_DATA * ch));

/*
 * Command logging types.
 */
#define LOG_NORMAL 0
#define LOG_ALWAYS 1
#define LOG_NEVER 2

/*
 * Log-all switch.
 */
bool fLogAll = FALSE;

/*
 * Convenience macros for the command table.
 * CMD            - normal command (flags = 0)
 * CMD_NINJA      - command that does not break ninja stance (flags = CMD_FLAG_NINJA_OK)
 * CMD_WAIT       - command that introduces wait state; queued while ch->wait > 0
 * CMD_NINJA_WAIT - CMD_NINJA + CMD_WAIT
 */
#define CMD(name, fun, pos, lvl, log, type, show)                                                  \
   {                                                                                               \
      name, fun, pos, lvl, log, type, show, 0                                                      \
   }
#define CMD_NINJA(name, fun, pos, lvl, log, type, show)                                            \
   {                                                                                               \
      name, fun, pos, lvl, log, type, show, CMD_FLAG_NINJA_OK                                      \
   }
#define CMD_WAIT(name, fun, pos, lvl, log, type, show)                                             \
   {                                                                                               \
      name, fun, pos, lvl, log, type, show, CMD_FLAG_WAIT                                          \
   }
#define CMD_NINJA_WAIT(name, fun, pos, lvl, log, type, show)                                       \
   {                                                                                               \
      name, fun, pos, lvl, log, type, show, CMD_FLAG_NINJA_OK | CMD_FLAG_WAIT                      \
   }

/*
 * Command table.
 */
const struct cmd_type cmd_table[] = {
    /*
     * Common movement commands.
     */

    /*
     * command table modified by Aeria
     */
    CMD_WAIT("north", do_north, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD_WAIT("east", do_east, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD_WAIT("south", do_south, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD_WAIT("west", do_west, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD_WAIT("up", do_up, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD_WAIT("down", do_down, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("map", do_mapper, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),

    /*
     * Common other commands.
     * Placed here so one and two letter abbreviations work.
     */
    CMD("buy", do_buy, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD_WAIT("cast", do_cast, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("exits", do_exits, POS_RESTING, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("get", do_get, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("gain", do_gain, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("inventory", do_inventory, POS_DEAD, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD_NINJA_WAIT("kill", do_kill, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("look", do_look, POS_RESTING, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("rest", do_rest, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("sleep", do_sleep, POS_SLEEPING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("stand", do_stand, POS_SLEEPING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("tell", do_tell, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD_NINJA("whisper", do_whisper, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("wield", do_wear, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("wizhelp", do_wizhelp, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("loot", do_loot, POS_STANDING, 20, LOG_ALWAYS, C_TYPE_OBJECT, C_SHOW_ALWAYS),

    /*
     * Informational commands.
     */

    CMD("test", do_test, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),

    CMD("affected", do_affected, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("areas", do_areas, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("appraise", do_appraise, POS_RESTING, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("bug", do_bug, POS_DEAD, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_ALWAYS),
    CMD("clist", do_clan_list, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("commands", do_commands, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("compare", do_compare, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("consider", do_consider, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("credits", do_credits, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("claneq", do_claneq, POS_STANDING, CLAN_ONLY, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("delete", do_delete, POS_STANDING, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("diagnose", do_diagnose, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("equipment", do_equipment, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("examine", do_examine, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("help", do_help, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("rhelp", do_rhelp, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("sthelp", do_stancehelp, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("rmlist", do_rmodhelp, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("shelp", do_shelp, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("lore", do_lore, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("heal", do_heal, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("idea", do_idea, POS_DEAD, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_ALWAYS),

    CMD("note", do_note, POS_RESTING, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_ALWAYS),

    CMD("report", do_report, POS_DEAD, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("pagelength", do_pagelen, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("players", do_players, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("read", do_read, POS_STANDING, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("rlist", do_race_list, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("score", do_score, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("slist", do_slist, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("socials", do_socials, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("status", do_status, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("time", do_time, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("typo", do_typo, POS_DEAD, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_ALWAYS),
    CMD("weather", do_weather, POS_RESTING, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("who", do_who, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("whois", do_whois, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("wizlist", do_wizlist, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("write", do_write, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("edit", do_edit, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),

    /*
     * Configuration commands.
     */

    CMD("alias", do_alias, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("auto", do_auto, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("autoexit", do_autoexit, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("autoloot", do_autoloot, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("autosac", do_autosac, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("blank", do_blank, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("brief", do_brief, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("channels", do_channels, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("color", do_color, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("colist", do_colist, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("combine", do_combine, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("config", do_config, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("description", do_description, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("ignore", do_ignore, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("password", do_password, POS_DEAD, 0, LOG_NEVER, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("prompt", do_prompt, POS_DEAD, 0, LOG_ALWAYS, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("title", do_title, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("wimpy", do_wimpy, POS_DEAD, 0, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),
    CMD("worth", do_worth, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD_WAIT("stance", do_stance, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("email", do_email, POS_RESTING, 20, LOG_NORMAL, C_TYPE_CONFIG, C_SHOW_ALWAYS),

    /*
     * Communication commands.
     */

    CMD("answer", do_answer, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("ask", do_ask, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("beep", do_beep, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("clan", do_clan, POS_RESTING, CLAN_ONLY, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("creator", do_creator, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD(")", do_creator, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD(".", do_gossip, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("gossip", do_gossip, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("pemote", do_pemote, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("emote", do_emote, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD(",", do_emote, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("flame", do_flame, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("finger", do_finger, POS_DEAD, 86, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("gtell", do_gtell, POS_DEAD, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD(";", do_gtell, POS_DEAD, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("music", do_music, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("newbie", do_newbie, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("pose", do_pose, POS_RESTING, 86, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    /* one or more of the poses crash us  */
    CMD("pray", do_pray, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("iinvasion", do_invasion, POS_DEAD, LEVEL_STAFF, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("invasion", do_invasion_talk, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("question", do_question, POS_SLEEPING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("race", do_race, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("reply", do_reply, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("say", do_say, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("'", do_say, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("shout", do_shout, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("tongue", do_tongue, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("yell", do_yell, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("zzz", do_zzz, POS_SLEEPING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("game", do_game, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("{", do_remorttalk, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("diplomat", do_diptalk, POS_RESTING, CLAN_ONLY, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("crusade", do_crusade, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("adept", do_adepttalk, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    CMD("ooc", do_ooc, POS_RESTING, 0, LOG_NORMAL, C_TYPE_COMM, C_SHOW_ALWAYS),
    /*
     * Combat commands.
     */
    CMD("assist", do_assist, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_NINJA_WAIT("backstab", do_backstab, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
                   C_SHOW_SKILL),
    CMD_NINJA_WAIT("bs", do_backstab, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("dirt", do_dirt, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("ashfall strike", do_ashfall_strike, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("harbor dust", do_harbor_dust, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("trip", do_trip, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("smash", do_smash, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("bash", do_bash, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("berserk", do_beserk, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("circle", do_circle, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("reach remedy", do_detox, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("disarm", do_disarm, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("flee", do_flee, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("checkpoint break", do_headbutt, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("kick", do_kick, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("road sweep", do_knee, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("leadership", do_leadership, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("morale", do_morale, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("murde", do_murde, POS_FIGHTING, 5, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_NEVER),
    CMD_NINJA_WAIT("murder", do_murder, POS_FIGHTING, 5, LOG_ALWAYS, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD_WAIT("palmstrike", do_palmstrike, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("poison:arsenic", do_poison_arsenic, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("poison:quinine", do_poison_quinine, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("poison:nightshade", do_poison_nightshade, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("punch", do_punch, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("rescue", do_rescue, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("revenant", do_revenant, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD_WAIT("bond", do_bond, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD_WAIT("stun", do_stun, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("frenzy", do_frenzy, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("charge", do_charge, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("target", do_target, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("dunmar's call", do_warcry, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("riposte", do_riposte, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("anti magic shell", do_anti_magic_shell, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("chi surge", do_chi_surge, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("breath of endurance", do_breath_of_endurance, POS_FIGHTING, 0, LOG_NORMAL,
             C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("fist of the interior form", do_fist_of_the_interior_form, POS_FIGHTING, 0, LOG_NORMAL,
             C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("momentum chain", do_momentum_chain, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("veteran's cadence", do_veterans_cadence, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("overwhelming assault", do_overwhelming_assault, POS_FIGHTING, 0, LOG_NORMAL,
             C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("oathshield", do_oathshield, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("sanctified strike", do_sanctified_strike, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("shadow reading", do_shadow_reading, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("hex ward", do_hex_ward, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("reflex disruption", do_reflex_disruption, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("holystrike", do_holystrike, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("shieldblock", do_shieldblock, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("chiblock", do_chiblock, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("chakra", do_chakra, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("spinfist", do_spinfist, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("pummel", do_pummel, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("aurabolt", do_aurabolt, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("phantomfist", do_phantomfist, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("interior discipline", do_mindoverbody, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("flurry", do_flurry, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("fleche", do_fleche, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    /* Cipher skills */
    CMD_NINJA_WAIT("reach silence", do_garrote, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
                   C_SHOW_SKILL),
    CMD_WAIT("dissolution protocol", do_feign_death, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("gap transit", do_shadow_step, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("asset flag", do_mark_target, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("gap hold", do_conceal, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("seam snare", do_set_trap, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("read intent", do_read_intent, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    /* Warden skills */
    CMD_WAIT("cleave", do_cleave, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("seven shade hold", do_fortify, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("charter challenge", do_taunt, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("rend", do_rend, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("field patch", do_field_patch, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("weapon mastery", do_weapon_mastery, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    /* Pugilist skills */
    CMD_WAIT("grapple", do_grapple, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("choke hold", do_choke_hold, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("leg sweep", do_leg_sweep, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("iron skin", do_iron_skin, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("roll with blow", do_roll_with_blow, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("applied understanding", do_pressure_point, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("feint", do_feint, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("cistern discipline", do_conditioning, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    /* Sentinel skills */
    CMD_WAIT("verdict", do_verdict, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("read opponent", do_read_opponent, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("binding strike", do_binding_strike, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("ninth descent", do_ninth_descent, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("condemn", do_condemn, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("seal testimony", do_seal_testimony, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),

    /*
     * Object manipulation commands.
     */

    CMD("adapt", do_adapt, POS_STANDING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("auction", do_auction, POS_STANDING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("bid", do_bid, POS_STANDING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD_WAIT("brandish", do_brandish, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("cdonate", do_cdonate, POS_RESTING, CLAN_ONLY, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("close", do_close, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("clutch", do_clutch, POS_STANDING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("donate", do_donate, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("drop", do_drop, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("eat", do_eat, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("enter", do_enter, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("give", do_give, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("hold", do_wear, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("list", do_list, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("lock", do_lock, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("open", do_open, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),

    CMD_WAIT("pick", do_pick, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("put", do_put, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("quaff", do_quaff, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD_WAIT("recite", do_recite, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("remove", do_remove, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("sedit", do_sedit, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_CONFIG, C_SHOW_NEVER),
    CMD("take", do_get, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("sacrifice", do_sacrifice, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("junk", do_junk, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("unlock", do_unlock, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("value", do_value, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("wear", do_wear, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("enchant", do_enchant, POS_STANDING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),
    CMD("assemble", do_connect, POS_RESTING, 0, LOG_NORMAL, C_TYPE_OBJECT, C_SHOW_ALWAYS),

    CMD("afk", do_afk, POS_DEAD, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_ALWAYS),
    CMD("assassinate", do_assassinate, POS_STANDING, 20, LOG_ALWAYS, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("follow", do_follow, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("gold", do_gold, POS_DEAD, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("group", do_group, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("guild", do_guild, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_NEVER),
    CMD("hide", do_hide, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("hunt", do_hunt, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("keep", do_keep, POS_DEAD, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("leav", do_leav, POS_RESTING, CLAN_ONLY, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_NEVER),
    CMD("leave", do_leave, POS_RESTING, CLAN_ONLY, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("qui", do_qui, POS_DEAD, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_NEVER),
    CMD("quit", do_quit, POS_DEAD, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_ALWAYS),
    CMD("recall", do_recall, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("/", do_recall, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("caravan", do_caravan, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("home", do_clan_recall, POS_STANDING, CLAN_ONLY, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("rent", do_rent, POS_DEAD, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_NEVER),
    CMD("save", do_save, POS_DEAD, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_ALWAYS),
    CMD("scan", do_scan, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("shadowform", do_shadowform, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("sneak", do_sneak, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("spells", do_spells, POS_SLEEPING, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),
    CMD("split", do_split, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD_NINJA_WAIT("steal", do_steal, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("train", do_train, POS_RESTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("visible", do_visible, POS_SLEEPING, 0, LOG_NORMAL, C_TYPE_MISC, C_SHOW_ALWAYS),
    CMD("wake", do_wake, POS_SLEEPING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("where", do_where, POS_RESTING, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),

    CMD("disguise", do_disguise, POS_STANDING, 0, LOG_ALWAYS, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("rulers", do_rulers, POS_RESTING, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),

    CMD("learned", do_learned, POS_SLEEPING, 0, LOG_NORMAL, C_TYPE_INFO, C_SHOW_ALWAYS),

    CMD("scout", do_scout, POS_STANDING, 1, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),

    CMD("qpspend", do_qpspend, POS_STANDING, 1, LOG_NORMAL, C_TYPE_MISC, C_SHOW_ALWAYS),

    /*
     * Staff commands.
     */

    CMD("alist", build_arealist, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("alink", do_alink, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("build", do_build, POS_STANDING, 2, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("mpcr", do_mpcr, POS_STANDING, L_DEI, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("halls", do_halls, POS_RESTING, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("lhunt", do_lhunt, POS_DEAD, L_SUP, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("sstat", do_sstat, POS_DEAD, L_SUP, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("cset", do_cset, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("whoname", do_whoname, POS_DEAD, L_ANG, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("sysdata", do_sysdata, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("clutchinfo", do_clutchinfo, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("advance", do_advance, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("setclass", do_setclass, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("doggy", do_dog, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("togbuild", do_togbuild, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("togleader", do_togleader, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("isnoop", do_isnoop, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("trust", do_trust, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("deathmatc", do_deathmatc, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_NEVER),
    CMD("deathmatch", do_deathmatch, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("resetpasswd", do_resetpassword, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("staffscore", do_staffscore, POS_DEAD, L_DEI, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("staffwhere", do_staffwhere, POS_DEAD, L_DEI, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("fights", do_fights, POS_DEAD, L_DEI, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("allow", do_allow, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("iban", do_ban, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("deny", do_deny, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("freeze", do_freeze, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("reboo", do_reboo, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_NEVER),
    CMD("reboot", do_reboot, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("hotreboo", do_hotreboo, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_NEVER),
    CMD("hotreboot", do_hotreboot, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("cancelhotreboot", do_cancelhotreboot, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF,
        C_SHOW_ALWAYS),
    CMD("shutdow", do_shutdow, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_NEVER),
    CMD("shutdown", do_shutdown, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("users", do_users, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("wizify", do_wizify, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("wizlock", do_wizlock, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("staffset", do_staffset, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("for", do_for, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("force", do_force, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("log", do_log, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("mload", do_mload, POS_DEAD, L_HER, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("monitor", do_monitor, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("areasave", do_areasave, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("mset", do_mset, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    /*{"mgive", do_mgive, POS_DEAD, L_DEI, LOG_ALWAYS,
     C_TYPE_STAFF, C_SHOW_ALWAYS},*/
    CMD("noemote", do_noemote, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("nopray", do_nopray, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("notell", do_notell, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("oload", do_oload, POS_DEAD, L_HER, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("oset", do_oset, POS_DEAD, L_ANG, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("owhere", do_owhere, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("pardon", do_pardon, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("peace", do_peace, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("purge", do_purge, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("staffquest", do_staffquest, POS_DEAD, L_DEI, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("restore", do_restore, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("rset", do_rset, POS_DEAD, 86, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("silence", do_silence, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("sla", do_sla, POS_DEAD, 86, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("slay", do_slay, POS_DEAD, L_HER, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("snoop", do_snoop, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("sset", do_sset, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("transfer", do_transfer, POS_DEAD, L_HER, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("transdm", do_transdm, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),

    CMD("at", do_at, POS_DEAD, L_DEI, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("bamfin", do_bamfin, POS_DEAD, L_ANG, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("bamfout", do_bamfout, POS_DEAD, L_ANG, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("echo", do_echo, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("goto", do_goto, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("holylight", do_holylight, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("invis", do_invis, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("memory", do_memory, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("mfind", do_mfind, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("mfindlev", do_mfindlev, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("mstat", do_mstat, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("mwhere", do_mwhere, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("ofind", do_ofind, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("ostat", do_ostat, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("recho", do_recho, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("return", do_return, POS_DEAD, L_DEI, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("rstat", do_rstat, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("slookup", do_slookup, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("switch", do_switch, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("stafftalk", do_stafftalk, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD(":", do_stafftalk, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("reward", do_reward, POS_DEAD, L_DEI, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("setcouncil", do_togcouncil, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("resetgain", do_gain_stat_reset, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("exlist", do_exlist, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("vlist", do_vlist, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("olmsg", do_olmsg, POS_DEAD, L_HER, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("ofindlev", do_ofindlev, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("prename", do_rename, POS_DEAD, L_GOD, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("pdelete", do_sdelete, POS_DEAD, 2, LOG_ALWAYS, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("scheck", do_scheck, POS_DEAD, L_GOD, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("staffbrand", do_staffbrand, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("otype", do_otype, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("owear", do_owear, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    CMD("findreset", do_findreset, POS_DEAD, L_HER, LOG_NORMAL, C_TYPE_STAFF, C_SHOW_ALWAYS),
    /*
     * program commands.
     */

    /* Reincarnate */
    CMD("reincarnate", do_reincarnate, POS_STANDING, 0, LOG_ALWAYS, C_TYPE_ACTION, C_SHOW_ALWAYS),

    /* Quest */
    CMD("quest", do_quest, POS_STANDING, 1, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),

    /* Public societies */
    CMD("enlist", do_enlist, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),
    CMD("society", do_society, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_ALWAYS),

    /* Public society skills -- Guard Command */
    CMD("patrol", do_patrol_sense, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("fieldmend", do_field_mend, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("rallycry", do_rally_cry, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("tactical", do_tactical_assess, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("presence", do_command_presence, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),

    /* Public society skills -- Harbor Syndics */
    CMD("audit", do_dockside_audit, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("haggle", do_haggle, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("bondcargo", do_bonded_cargo, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("anchorslam", do_anchor_slam, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),

    /* Public society skills -- Ember Wardens */
    CMD("forage", do_forage, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("fieldrepair", do_field_repair, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("embersignal", do_ember_signal, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("containward", do_containment_ward, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION,
        C_SHOW_SKILL),
    CMD("controlburn", do_controlled_burn, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
        C_SHOW_SKILL),

    /* Public society skills -- Shrine of the First Claw */
    CMD("oathmark", do_oath_mark, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("trackquarry", do_track_quarry, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("writhold", do_writ_of_hold, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("clawspursuit", do_claws_pursuit, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("judgment", do_claws_judgment, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),

    /* Public society skills -- Wall Command */
    CMD("digin", do_dig_in, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("shelter", do_shelter, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("trailblaze", do_trailblaze, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("signaltower", do_signal_tower, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("rampartslam", do_rampart_slam, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),

    /* Public society skills -- Road Wardens */
    CMD("wayfind", do_wayfind, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("roadcamp", do_road_camp, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("forcedmarch", do_forced_march, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("dispatch", do_dispatch_rider, POS_STANDING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD("waylay", do_waylay, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("duty strike", do_duty_strike, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("gap probe", do_gap_probe, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_SKILL),
    CMD_WAIT("primary motion", do_primary_motion, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),
    CMD_WAIT("vigil strike", do_vigil_strike, POS_FIGHTING, 0, LOG_NORMAL, C_TYPE_ACTION,
             C_SHOW_SKILL),

    /*
     * End of list.
     */
    CMD("", 0, POS_DEAD, 0, LOG_NORMAL, C_TYPE_ACTION, C_SHOW_NEVER)

    /*
     * Staff commands.
     */

};

/* Log all commands.. rewrite every 5 mins.. */
void comlog(CHAR_DATA *ch, int cmd, char *args)
{
   static FILE *fplog;
   static time_t ltime;

   if (!fplog || ltime + (5 * 60) <= current_time)
   {
      if (fplog != NULL)
      {
         fclose(fplog);
         fplog = NULL;
      }
      if (!(fplog = fopen("../log/comlog.txt", "w")))
         return;
      ltime = current_time;
   }
   fprintf(fplog, "%.24s :: %12.12s (%5d): %s %s\n", ctime(&current_time), ch->name,
           (IS_NPC(ch) ? ch->pIndexData->vnum : -1), cmd_table[cmd].name,
           (cmd_table[cmd].log == LOG_NEVER ? "XXX" : args));
   fflush(fplog);
}

/*
 * Returns TRUE if ch is allowed to use cmd_table[cmd].
 */
static bool cmd_allows(CHAR_DATA *ch, int cmd, int trust)
{
   if (cmd_table[cmd].level == CLAN_ONLY && !IS_NPC(ch) && ch->pcdata->clan == 0)
      return FALSE;

   if (cmd_table[cmd].level == BOSS_ONLY && !IS_NPC(ch) &&
       !IS_SET(ch->pcdata->pflags, PFLAG_CLAN_BOSS))
      return FALSE;

   return cmd_table[cmd].level <= trust || MP_Commands(ch);
}

/*
 * Searches the command table for a match.
 * exact=TRUE  requires a full name match.
 * exact=FALSE allows prefix matching.
 * Returns the table index on success, -1 if not found.
 */
static int find_command(CHAR_DATA *ch, const char *word, bool exact)
{
   int trust = get_trust(ch);
   int i;

   for (i = 0; cmd_table[i].name[0] != '\0'; i++)
   {
      if (cmd_table[i].name[0] != word[0])
         continue;
      if (!cmd_allows(ch, i, trust))
         continue;
      if (exact ? !str_cmp(word, cmd_table[i].name) : !str_prefix(word, cmd_table[i].name))
         return i;
   }
   return -1;
}

/*
 * Returns TRUE if the command in 'argument' has CMD_FLAG_WAIT set,
 * meaning it should be held in the queue while ch->wait > 0.
 */
bool command_has_wait_flag(CHAR_DATA *ch, const char *argument)
{
   char word[MAX_INPUT_LENGTH];
   int cmd;

   /* Skip leading spaces and grab first word */
   while (*argument == ' ')
      argument++;
   argument = one_argument((char *)argument, word);

   if (word[0] == '\0')
      return FALSE;

   cmd = find_command(ch, word, FALSE);
   if (cmd < 0)
      return FALSE;

   return IS_SET(cmd_table[cmd].flags, CMD_FLAG_WAIT);
}

/*
 * Position-rejection messages, indexed by POS_* constant.
 */
static const char *const pos_msgs[] = {
    /* POS_DEAD     0 */ "Lie still; you are @@dDEAD@@N.\n\r",
    /* POS_MORTAL   1 */ "You are @@Rhurt@@N far too bad for that.\n\r",
    /* POS_INCAP    2 */ "You are @@Rhurt@@N far too bad for that.\n\r",
    /* POS_STUNNED  3 */ "You are too @@estunned@@N to do that.\n\r",
    /* POS_SLEEPING 4 */ "Oh, go back to @@Wsleep!@@N\n\r",
    /* POS_RESTING  5 */ "Naaaaaah... You feel too @@brelaxed@@N...\n\r",
    /* POS_FIGHTING 6 */ "Not until you @@Rstop@@N fighting!\n\r",
};

/*
 * Internal recursive entry point for interpret().
 * alias_depth tracks alias expansion depth to prevent infinite loops.
 */
static void interpret_r(CHAR_DATA *ch, char *argument, int alias_depth)
{
   char command[MAX_INPUT_LENGTH];
   char logline[MAX_INPUT_LENGTH];
   int cmd;

   if (ch->position == POS_WRITING)
   {
      write_interpret(ch, argument);
      return;
   }

   if (ch->position == POS_BUILDING)
   {
      if (argument[0] == ':')
         argument++;
      else
      {
         build_interpret(ch, argument);
         return;
      }
   }

   /*
    * Strip leading spaces.
    */
   while (isspace(*argument))
      argument++;
   if (argument[0] == '\0')
      return;

   /*
    * No hiding.
    */
   REMOVE_BIT(ch->affected_by, AFF_HIDE);

   /*
    * Implement freeze command.
    */
   if (!IS_NPC(ch) && IS_SET(ch->act, PLR_FREEZE))
   {
      send_to_char("@@a@@fYou're totally frozen!@@N\n\r", ch);
      return;
   }

   if (ch->stunTimer > 0)
   {
      send_to_char("You are too@@aSTUNNED@@N to act!\n\r", ch);
      return;
   }

   /*
    * Grab the command word.
    * Special parsing so ' can be a command,
    *   also no spaces needed after punctuation.
    */
   snprintf(logline, sizeof(logline), "%s", argument);
   if (!isalpha(argument[0]) && !isdigit(argument[0]))
   {
      command[0] = argument[0];
      command[1] = '\0';
      argument++;
      while (isspace(*argument))
         argument++;
   }
   else
   {
      argument = one_argument(argument, command);
   }

   /*
    * Look for command in command table.
    * First pass: exact match. Second pass: prefix match.
    */
   cmd = find_command(ch, command, TRUE);
   if (cmd < 0)
      cmd = find_command(ch, command, FALSE);

   /*
    * Log and snoop.
    */
   if (cmd >= 0 && cmd_table[cmd].log == LOG_NEVER)
      snprintf(logline, sizeof(logline), "%s", "XXXXXXXX XXXXXXXX XXXXXXXX@@N");

   if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_LOG)) || fLogAll ||
       (cmd >= 0 && cmd_table[cmd].log == LOG_ALWAYS))
   {
      sprintf(log_buf, "Log %s: %s", ch->name, logline);
      log_string(log_buf);
      if (IS_SET(ch->act, PLR_LOG))
         monitor_chan(log_buf, MONITOR_BAD);
      else if (cmd >= 0 && cmd_table[cmd].level > LEVEL_HERO)
         monitor_chan(log_buf, MONITOR_GEN_STAFF);
      else
         monitor_chan(log_buf, MONITOR_GEN_MORT);
   }

   if (ch->desc != NULL && ch->desc->snoop_by != NULL)
   {
      char snp[MAX_STRING_LENGTH];
      sprintf(snp, "[Snoop:%s] %s\n\r", ch->name, logline);
      write_to_buffer(ch->desc->snoop_by, snp, 0);
   }

   /*
    * Check aliases.  Only at depth 0 to prevent infinite loops.
    */
   if (cmd < 0 && !IS_NPC(ch) && alias_depth == 0)
   {
      int cnt;
      char foo[MAX_STRING_LENGTH];

      for (cnt = 0; cnt < MAX_ALIASES; cnt++)
      {
         if (!str_cmp(ch->pcdata->alias_name[cnt], command) &&
             str_cmp(ch->pcdata->alias_name[cnt], "<none>@@N"))
         {
            sprintf(foo, "%s %s", ch->pcdata->alias[cnt], argument);
            interpret_r(ch, foo, alias_depth + 1);
            return;
         }
      }
   }

   if (cmd < 0)
   {
      if (!check_social(ch, command, argument))
         send_to_char("Huh?\n\r", ch);
      return;
   }

   /*
    * Character not in position for command?
    */
   if (ch->position < cmd_table[cmd].position)
   {
      send_to_char(pos_msgs[ch->position], ch);
      return;
   }

   /*
    * Dispatch the command.
    */
   if (!IS_NPC(ch) && ch->stance > 0 && IS_SET(stance_app[ch->stance].specials, STANCE_NINJA) &&
       !IS_SET(cmd_table[cmd].flags, CMD_FLAG_NINJA_OK))
   {
      send_to_char("You step out of the shadows.\n\r", ch);
      ch->stance = 0;
      act("$n steps out of the Shadows!", ch, NULL, NULL, TO_ROOM);
   }
   comlog(ch, cmd, argument);
   (*cmd_table[cmd].do_fun)(ch, argument);

   tail_chain();
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret(CHAR_DATA *ch, char *argument)
{
   interpret_r(ch, argument, 0);
}

bool check_social(CHAR_DATA *ch, char *command, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int cmd;
   bool found;

   found = FALSE;
   for (cmd = 0; social_table[cmd].name[0] != '\0'; cmd++)
   {
      if (command[0] == social_table[cmd].name[0] && !str_prefix(command, social_table[cmd].name))
      {
         found = TRUE;
         break;
      }
   }

   if (!found)
      return FALSE;

   if (!IS_NPC(ch) && IS_SET(ch->act, PLR_NO_EMOTE))
   {
      send_to_char("You are anti-social!\n\r", ch);
      return TRUE;
   }

   switch (ch->position)
   {
   case POS_DEAD:
      send_to_char("Lie still; you are @@dDEAD@@N.\n\r", ch);
      return TRUE;

   case POS_INCAP:
   case POS_MORTAL:
      send_to_char("You are @@Rhurt@@N far too bad for that.\n\r", ch);
      return TRUE;

   case POS_STUNNED:
      send_to_char("You are too @@estunned@@N to do that.\n\r", ch);
      return TRUE;

   case POS_SLEEPING:
      /*
       * I just know this is the path to a 12" 'if' statement.  :(
       * But two players asked for it already!  -- Furey
       */
      if (!str_cmp(social_table[cmd].name, "snore"))
         break;
      send_to_char("In your @@Wdreams@@N, or what?\n\r", ch);
      return TRUE;
   }

   one_argument(argument, arg);
   victim = NULL;
   if (arg[0] == '\0')
   {
      act(social_table[cmd].others_no_arg, ch, NULL, victim, TO_ROOM);
      act(social_table[cmd].char_no_arg, ch, NULL, victim, TO_CHAR);
   }
   else if ((victim = get_char_room(ch, arg)) == NULL)
   {
      send_to_char("They aren't here.\n\r", ch);
   }
   else if (victim == ch)
   {
      act(social_table[cmd].others_auto, ch, NULL, victim, TO_ROOM);
      act(social_table[cmd].char_auto, ch, NULL, victim, TO_CHAR);
   }
   else
   {
      act(social_table[cmd].others_found, ch, NULL, victim, TO_NOTVICT);
      act(social_table[cmd].char_found, ch, NULL, victim, TO_CHAR);
      act(social_table[cmd].vict_found, ch, NULL, victim, TO_VICT);

      if (!IS_NPC(ch) && IS_NPC(victim) && !IS_AFFECTED(victim, AFF_CHARM) && IS_AWAKE(victim))
      {
         switch (number_bits(4))
         {
         case 0:
         case 1:
         case 2:
         case 3:
         case 4:
            act(social_table[cmd].others_found, victim, NULL, ch, TO_NOTVICT);
            act(social_table[cmd].char_found, victim, NULL, ch, TO_CHAR);
            act(social_table[cmd].vict_found, victim, NULL, ch, TO_VICT);
            break;

         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
         case 11:
         case 12:
            act("$n slaps $N.", victim, NULL, ch, TO_NOTVICT);
            act("You slap $N.", victim, NULL, ch, TO_CHAR);
            act("$n slaps you.", victim, NULL, ch, TO_VICT);
            break;
         }
      }
   }

   return TRUE;
}

bool IS_SWITCHED(CHAR_DATA *ch)
{
   if (!IS_NPC(ch))
      return FALSE;

   if (ch->desc == NULL)
      return FALSE;

   return TRUE;
}

bool MP_Commands(CHAR_DATA *ch) /* Can scripted mobs
                                 * use mpcommands? TRUE if yes.
                                 * - Kahn */
{
   if (IS_SWITCHED(ch))
      return FALSE;

   if (IS_NPC(ch) && ch->pIndexData->progtypes && !IS_AFFECTED(ch, AFF_CHARM))
      return TRUE;

   return FALSE;
}
