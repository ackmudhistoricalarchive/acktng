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

#include "globals.h"

/*
 * Unified class table (mortal 0-5, remort 6-17, adept 18-23).
 */
const struct class_type gclass_table[MAX_TOTAL_CLASS] = {
    /* Mortal (0-5): tier=MORTAL, no prerequisites */
    {"Mag", "Magi", APPLY_INT, 2, 5, MORTAL, {-1, -1}},
    {"Cle", "Cleric", APPLY_WIS, 3, 4, MORTAL, {-1, -1}},
    {"Cip", "Cipher", APPLY_DEX, 6, 0, MORTAL, {-1, -1}},
    {"War", "Warden", APPLY_STR, 7, 0, MORTAL, {-1, -1}},
    {"Psi", "Psionicist", APPLY_INT, 3, 4, MORTAL, {-1, -1}},
    {"Pug", "Pugilist", APPLY_CON, 7, 0, MORTAL, {-1, -1}},

    /* Remort (6-17): tier=REMORT, one mortal prerequisite */
    {"Sor", "Sorcerer", APPLY_INT, 4, 10, REMORT, {CLASS_MAG, -1}},
    {"Pal", "Paladin", APPLY_WIS, 8, 6, REMORT, {CLASS_CLE, -1}},
    {"Ass", "Assassin", APPLY_DEX, 12, 0, REMORT, {CLASS_CIP, -1}},
    {"Kni", "Knight", APPLY_STR, 14, 0, REMORT, {CLASS_WAR, -1}},
    {"Nec", "Necromancer", APPLY_INT, 6, 8, REMORT, {CLASS_PSI, -1}},
    {"Mon", "Monk", APPLY_CON, 14, 0, REMORT, {CLASS_PUG, -1}},
    {"Wiz", "Wizard", APPLY_INT, 4, 10, REMORT, {CLASS_MAG, -1}},
    {"Pri", "Priest", APPLY_WIS, 6, 8, REMORT, {CLASS_CLE, -1}},
    {"Wlk", "Warlock", APPLY_DEX, 8, 6, REMORT, {CLASS_CIP, -1}},
    {"Swo", "Swordsman", APPLY_STR, 14, 0, REMORT, {CLASS_WAR, -1}},
    {"Ego", "Egomancer", APPLY_INT, 6, 8, REMORT, {CLASS_PSI, -1}},
    {"Bra", "Brawler", APPLY_CON, 14, 0, REMORT, {CLASS_PUG, -1}},

    /* Adept (18-23): tier=ADEPT, either of two remort prerequisites */
    {"Gra", "Grand Magi", APPLY_INT, 8, 20, ADEPT, {CLASS_SOR, CLASS_WIZ}},
    {"Tem", "Templar", APPLY_WIS, 16, 12, ADEPT, {CLASS_PAL, CLASS_PRI}},
    {"Nig", "Nightblade", APPLY_DEX, 20, 8, ADEPT, {CLASS_ASS, CLASS_WLK}},
    {"Cru", "Crusader", APPLY_STR, 28, 0, ADEPT, {CLASS_KNI, CLASS_SWO}},
    {"Kin", "Kinetimancer", APPLY_INT, 12, 16, ADEPT, {CLASS_NEC, CLASS_EGO}},
    {"Mar", "Martial Artist", APPLY_CON, 28, 0, ADEPT, {CLASS_MON, CLASS_BRA}},

    /* Druid lineage (24-27) */
    {"Dru", "Druid", APPLY_CON, 5, 0, MORTAL, {-1, -1}},
    {"Tho", "Thornwarden", APPLY_CON, 10, 0, REMORT, {CLASS_DRU, -1}},
    {"Wil", "Wildspeaker", APPLY_CON, 8, 0, REMORT, {CLASS_DRU, -1}},
    {"Hie", "Hierophant", APPLY_CON, 16, 0, ADEPT, {CLASS_THO, CLASS_WIL}},

    /* Sentinel lineage (28-31) */
    {"Sen", "Sentinel", APPLY_WIS, 6, 1, MORTAL, {-1, -1}},
    {"Jus", "Justicar", APPLY_WIS, 12, 2, REMORT, {CLASS_SEN, -1}},
    {"Arb", "Arbiter", APPLY_WIS, 10, 4, REMORT, {CLASS_SEN, -1}},
    {"Inq", "Inquisitor", APPLY_WIS, 20, 6, ADEPT, {CLASS_JUS, CLASS_ARB}},
};
