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

#define DEC_CONFIG_H 1

/*
 *  Your mud info here :) Zen
 */

#define mudnamecolor "@@lACK!TNG@@N"
#define mudnamenocolor "ACK!TNG"

/* MSSP listing-site metadata */
#define MSSP_CONTACT "admin@ackmud.com"
#define MSSP_WEBSITE "https://ackmud.com"
#define MSSP_ICON_URL "https://ackmud.com/icon.png"
#define MSSP_LOCATION "United States"
#define sacgodname "Cyberlich"
#define goodgodname "Freyja"
#define neutralgodname "Zenithar"
#define evilgodname "Oderus"
#define UPGRADE_REVISION 15
/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH 2048
#define MAX_STRING_LENGTH 8192
#define MSL MAX_STRING_LENGTH
#define MAX_INPUT_LENGTH 640
#define MAX_AREAS 200
#define MAX_VNUM 65535

#define BOOT_DB_ABORT_THRESHOLD 25
#define RUNNING_ABORT_THRESHOLD 10
#define ALARM_FREQUENCY 20

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */

#define MAX_CHARMIE 3
#define CHARMIE_STAT_BONUS 1
#define CHARMIE_TACT_BONUS 1
#define BASE_DAMCAP 2500
#define MAX_COMBO 6
#define MAX_MORT_COMBO 4

#define MAX_CLAN_EQ 6 /* Number of clan eq items */

#define CLANEQ_WEIGHT_MELEE 8
#define CLANEQ_WEIGHT_CASTER 3
#define CLANEQ_WEIGHT_TANK 13
#define MAX_COLOR 16 /* eg look, prompt, shout */
#define MAX_ANSI 28  /* eg red, black, etc (was 11) */
#define MAX_ALIASES 10
#define MAX_IGNORES 3
#define MAX_RACE 23
#define MAX_CLAN 11    /* number of clans (0=None + 10 clans) */
#define EXP_LEVEL 1000 /* exp per level */
#define MAX_SKILL 999
#define MAX_CLASS 6
#define MAX_REMORT (MAX_CLASS * 2)
#define INDEX_WAR 3
#define INDEX_MAG 0
#define INDEX_CLE 1
#define INDEX_PSI 4
#define INDEX_PUG 5
#define INDEX_THI 2
#define MAX_PC_CLASS 4
#define MAX_LEVEL 105
#define LEVEL_HERO (MAX_LEVEL - 4)
#define LEVEL_STAFF (MAX_LEVEL - 3)
#define MAX_MORTAL (MAX_LEVEL - 5)
#define MAX_MOB_LEVEL 171
#define MAX_ADEPT 20
#define ADVANCE_ADEPT 32
#define MAX_SUPER 4

#define STAT_MAX 30

#define MAX_LOOT 9

#define PLAYER_HOUSING_START_VNUM 64800

#define LEVEL_ONE 20
#define LEVEL_TWO 200
#define LEVEL_THREE 2000
#define LEVEL_FOUR 10000
#define LEVEL_FIVE 40000
#define LEVEL_MASTER 100000
#define LEVEL_GM 200000

/* Mortal (0-5) */
#define CLASS_MAG 0
#define CLASS_CLE 1
#define CLASS_CIP 2
#define CLASS_WAR 3
#define CLASS_PSI 4
#define CLASS_PUG 5
/* Remort (6-17) */
#define CLASS_SOR 6
#define CLASS_PAL 7
#define CLASS_ASS 8
#define CLASS_KNI 9
#define CLASS_NEC 10
#define CLASS_MON 11
#define CLASS_WIZ 12
#define CLASS_PRI 13
#define CLASS_WLK 14
#define CLASS_SWO 15
#define CLASS_EGO 16
#define CLASS_BRA 17
/* Adept (18-23) */
#define CLASS_GMA 18
#define CLASS_TEM 19
#define CLASS_NIG 20
#define CLASS_CRU 21
#define CLASS_KIN 22
#define CLASS_MAR 23
/* Druid lineage (appended, non-contiguous with tier ranges) */
#define CLASS_DRU 24
#define CLASS_THO 25
#define CLASS_WIL 26
#define CLASS_HIE 27
/* Sentinel lineage (appended) */
#define CLASS_SEN 28

/* Total class count across all tiers */
#define MAX_TOTAL_CLASS 29

/* IS_MORTAL_CLASS, IS_REMORT_CLASS, IS_ADEPT_CLASS, CLASS_TIER are
 * implemented as static inline functions in inlines_globals.h. */

#define MAX_NUM_STAFF 5

#define DURATION_HOUR 0
#define DURATION_ROUND 1

/* Maximum amount of reincarnation data */
#define MAX_REINCARNATE 10

#define REINCARNATION_DAMCAP 0
#define REINCARNATION_RMOD 1
#define REINCARNATION_STATCAP 2
#define REINCARNATION_HP 3
#define REINCARNATION_MANA 4
#define REINCARNATION_MOVE 5
#define REINCARNATION_POINTS 6

#define MAX_SUPERBOSS 5
#define SUPERBOSS_PYRAMID_BLACK_SUN_SHARD 0
#define SUPERBOSS_KEEP_ELEMENTAL_CAPTAIN 1
#define SUPERBOSS_KEEP_PHYSICAL_CAPTAIN 2
#define SUPERBOSS_HERMIT_ARCHON 3
#define SUPERBOSS_SANCTUM_KEEPER 4

#define REINCARNATE_RACE BIT_1
#define REINCARNATE_CLASS BIT_2
#define REINCARNATE_CONFIRM BIT_3

/*
 * total number of supernatural types in the game
 */
#define QUORUM_NUMBER 4
/*
 * number of members to be a quorum for a council
 */

#define MAX_TRADE 5

#define MAX_STANCE 25

/* Stance specials */
#define STANCE_DUAL_CAST BIT_1
#define STANCE_MULTI_CAST BIT_2
#define STANCE_EITHER_CLASS BIT_3
#define STANCE_NINJA BIT_4
#define STANCE_NO_HIT BIT_5
#define STANCE_DUAL_BACKSTAB BIT_6

#define AREA_PAYAREA 1
#define AREA_TELEPORT 2
#define AREA_BUILDING 4
#define AREA_NOSHOW 8 /* Show area name on 'areas' command? */
#define AREA_NO_ROOM_AFF 16

/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_MARTIAL -3 /* for martial arts */
#define TYPE_UNDEFINED -2
#define TYPE_HIT 1000

/*
 *  Target types.
 */
#define TAR_IGNORE 0
#define TAR_CHAR_OFFENSIVE 1
#define TAR_CHAR_DEFENSIVE 2
#define TAR_CHAR_SELF 3
#define TAR_OBJ_INV 4
#define TAR_CHAR_NOTSELF 5

/* Class tier constants (returned by CLASS_TIER / skill_get_tier) */
#define MORTAL 1
#define REMORT 2
#define ADEPT 3

/* Used as flags in skill_table */
#define NORM 1

/*
 * Quest mob personalities
 */

#define QUEST_RUDE 1
#define QUEST_NICE 2
#define QUEST_WIMPY 4
#define QUEST_COOL 8
#define QUEST_MEAN 16

#define C_TYPE_MISC 0
#define C_TYPE_COMM 1
#define C_TYPE_CONFIG 2
#define C_TYPE_INFO 3
#define C_TYPE_ACTION 4
#define C_TYPE_OBJECT 5
#define C_TYPE_STAFF 6

#define C_SHOW_NEVER -1
#define C_SHOW_ALWAYS 0
#define C_SHOW_SKILL 1

/* Command flags (cmd_type.flags) */
#define CMD_FLAG_NINJA_OK BIT_1 /* command does not break ninja stance */
#define CMD_FLAG_WAIT BIT_2     /* command introduces wait state; queue while ch->wait > 0 */

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_CITYGUARD 976
#define MOB_VNUM_DOGGY 65327
#define MOB_VNUM_ZOMBIE 11004
#define MOB_VNUM_STALKER 65349
#define MOB_VNUM_WATERELEM 65384
#define MOB_VNUM_SKELETON 65384
#define MOB_VNUM_FIREELEM 65384
#define MOB_VNUM_TRAINER 65328
#define MOB_VNUM_EARTHELEM 65384
#define MOB_VNUM_IRON_GOLEM 65385
#define MOB_VNUM_SOUL_THIEF 65386
#define MOB_VNUM_HOLY_AVENGER 65387
#define MOB_VNUM_PEGASUS 65393
#define MOB_VNUM_NIGHTMARE 65394
#define MOB_VNUM_ELEM_BEAST 65395
#define MOB_VNUM_INT_DEVOURER 65396
#define MOB_VNUM_SHADOW_HOUND 65397

#define MOB_VNUM_DIAMOND_GOLEM 65388
#define MOB_VNUM_COMBAT_ELEMENTAL 65392

/*
 * vnums for quest items.
 * One of these will be picked at random and placed onto a random mob
 * (one the player has a chance of killing!).
 * Reserve vnums 40-60 for this.
 * We'll define a range of possible vnums here - easier to expand (:
 */

#define OBJ_VNUM_QUEST_MIN 65446
#define OBJ_VNUM_QUEST_MAX 65451
/* Single AI-described quest item template (within the MIN/MAX range) */
#define OBJ_VNUM_QUEST_ITEM 65446

#define PULSE_PER_SECOND 8
#define PULSE_VIOLENCE (2 * PULSE_PER_SECOND)
#define PULSE_MOBILE (4 * PULSE_PER_SECOND)
#define PULSE_OBJFUN (4 * PULSE_PER_SECOND)
#define PULSE_TICK (60 * PULSE_PER_SECOND)
#define PULSE_MESSAGE (12 * PULSE_PER_SECOND)
#define PULSE_ROOMS (30 * PULSE_PER_SECOND)
#define PULSE_AREA (80 * PULSE_PER_SECOND)
#define PULSE_AUCTION (30 * PULSE_PER_SECOND)
#define PULSE_RAUCTION (30 * PULSE_PER_SECOND)
#define PULSE_MAUCTION (30 * PULSE_PER_SECOND)

/* Duration of an automatically triggered happy hour in ticks (1 tick = 1 in-game hour =
 * 60 real seconds).  12 ticks = half an in-game day = ~12 real minutes. */
#define HAPPY_HOUR_AUTO_DURATION 12

#define LIQUID_BLOOD 13

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_MONEY_ONE 65402
#define OBJ_VNUM_MONEY_SOME 65403

#define OBJ_VNUM_CORPSE_NPC 65410
#define OBJ_VNUM_CORPSE_PC 65411
#define OBJ_VNUM_SEVERED_HEAD 65412
#define OBJ_VNUM_TORN_HEART 65413
#define OBJ_VNUM_SLICED_ARM 65414
#define OBJ_VNUM_SLICED_LEG 65415
#define OBJ_VNUM_FINAL_TURD 65416

#define OBJ_VNUM_MUSHROOM 65420
#define OBJ_VNUM_LIGHT_BALL 65421
#define OBJ_VNUM_SPRING 65419

#define OBJ_VNUM_BOARD 65423
#define OBJ_VNUM_FOOD 65427
#define OBJ_VNUM_FIREBLADE 65428
#define OBJ_VNUM_TOKEN 65429
#define OBJ_VNUM_WINDOW 65430
#define OBJ_VNUM_PORTAL 65431
#define OBJ_VNUM_BEACON 65432

#define OBJ_VNUM_SCHOOL_MACE 3700
#define OBJ_VNUM_SCHOOL_DAGGER 3701
#define OBJ_VNUM_SCHOOL_SWORD 3702
#define OBJ_VNUM_SCHOOL_VEST 3703
#define OBJ_VNUM_SCHOOL_SHIELD 3704
#define OBJ_VNUM_SCHOOL_BANNER 3716
#define OBJ_VNUM_SOUL_POTION 65434
#define OBJ_VNUM_CAPTURED_SOUL 65435
#define OBJ_VNUM_CONFLAGRATION 65506
#define OBJ_VNUM_MINO_PASS 6621
/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO 65269
#define ROOM_VNUM_CHAT 491
#define ROOM_VNUM_TEMPLE 1209
#define ROOM_VNUM_ALTAR 970
#define ROOM_VNUM_SCHOOL 4900
#define ROOM_VNUM_DONATION 65326
#define ROOM_VNUM_MORGUE 65323
#define ROOM_VNUM_MORIBUND 65324
#define ROOM_VNUM_BUILDER 65315
#define ROOM_VNUM_BODIES 65290
#define ROOM_VNUM_HALLS 65312 /* The Halls of the Staff */
#define ROOM_VNUM_CLAN 65325  /* for leaders only */
#define ROOM_VNUM_DM_RECALL 917
#define ROOM_VNUM_ETHEREAL_PLANE 3850
#define ROOM_VNUM_INT_HEAL 3871
#define ROOM_VNUM_MID_BOTTOM 917
#define ROOM_VNUM_MID_TOP 1116
#define ROOM_VNUM_JAIL 65269
#define ROOM_VNUM_ARMOR_DONATE 934
#define ROOM_VNUM_WEAPON_DONATE 935
#define ROOM_VNUM_MISC_DONATE 933

#define C_SHOW_NEVER -1
#define C_SHOW_ALWAYS 0
#define C_SHOW_SKILL 1

#define SUPER_NONE 0
#define SUPER_WOLF 2
#define SUPER_BLESSED 3

/* defines for enchant.c  */
#define ENCHANT_EXTRA_FLAGS -1
#define ENCHANT_APPLY_FLAGS -2
#define ENCHANT_OBJFUNS -3

#define RACE_MOD_NONE BIT_1
#define RACE_MOD_MIRACLE_HEAL BIT_2
#define RACE_MOD_EXTRAORDINARY_HEAL BIT_3
#define RACE_MOD_FAST_HEAL BIT_4
#define RACE_MOD_SLOW_HEAL BIT_5
#define RACE_MOD_STRONG_MAGIC BIT_6
#define RACE_MOD_MAGIC_ADEPT BIT_7
#define RACE_MOD_MAGIC_GENIUS BIT_8
#define RACE_MOD_WEAK_MAGIC BIT_9
#define RACE_MOD_NO_MAGIC BIT_10
#define RACE_MOD_IMMUNE_POISON BIT_11
#define RACE_MOD_RESIST_MAGIC BIT_12
#define RACE_MOD_THWART_MAGIC BIT_13
#define RACE_MOD_MAGIC_FOIL BIT_14
#define RACE_MOD_WOODLAND BIT_15
#define RACE_MOD_DARKNESS BIT_16
#define RACE_MOD_HUGE BIT_17
#define RACE_MOD_LARGE BIT_18
#define RACE_MOD_TINY BIT_19
#define RACE_MOD_SMALL BIT_20
#define RACE_MOD_TAIL BIT_21
#define RACE_MOD_TOUGH_SKIN BIT_22
#define RACE_MOD_STONE_SKIN BIT_23
#define RACE_MOD_IRON_SKIN BIT_24
#define RACE_MOD_STRONG BIT_25
#define RACE_MOD_HERCULES BIT_26
#define RACE_MOD_GOLIATH BIT_27

/*
 *   Spell Damage Types
 */

/* Magic Shield types
   -Damane 4/27/96 */
#define SHIELD_NONE 0
#define FLAME_SHIELD 1
#define SHOCK_SHIELD 2
#define SHADOW_SHIELD 3
#define ICE_SHIELD 4
#define PSI_SHIELD 5
#define HOLY_SHIELD 6
#define ARCANE_SHIELD 7

/* These are for skill_table lookup funcs... to save writing 2 functions */
#define RETURN_BEST_LEVEL 1
#define RETURN_BEST_CLASS 2

/*
 * God Levels
 */
#define L_GOD MAX_LEVEL
#define L_SUP L_GOD - 1
#define L_DEI L_SUP - 1
#define L_ANG L_DEI - 1
#define L_HER L_ANG - 1

#define CLAN_ONLY -1
#define BOSS_ONLY -2

/*
 * Time and weather stuff.
 */
#define SUN_DARK 0
#define SUN_RISE 1
#define SUN_LIGHT 2
#define SUN_SET 3

#define SKY_CLOUDLESS 0
#define SKY_CLOUDY 1
#define SKY_RAINING 2
#define SKY_LIGHTNING 3

#define MOON_DOWN 0
#define MOON_RISE 1
#define MOON_LOW 2
#define MOON_PEAK 3
#define MOON_FALL 4
#define MOON_SET 5

#define MOON_NEW 0
#define MOON_WAX_CRE 1
#define MOON_WAX_HALF 2
#define MOON_WAX_GIB 3
#define MOON_FULL 4
#define MOON_WAN_GIB 5
#define MOON_WAN_HALF 6
#define MOON_WAN_CRE 7

/*
 * Connected state for a channel.
 */

/* These values referenced by users command, BTW */

#define CON_PLAYING 0
#define CON_GET_NAME -1
/*
 * All non connected
 */
#define CON_GET_OLD_PASSWORD -2
/*
 * states changed to
 */
#define CON_CONFIRM_NEW_NAME -3
/*
 * negative so that
 */
#define CON_GET_NEW_PASSWORD -4
/*
 * Hotboot can shut out
 */
#define CON_CONFIRM_NEW_PASSWORD -5
/*
 * All nonplaying chars
 */
#define CON_GET_NEW_SEX -6
/*
 * ---Flar
 */
#define CON_GET_NEW_CLASS -7
#define CON_GET_RACE -9
#define CON_READ_MOTD -10
#define CON_GET_STATS -11
#define CON_FINISHED -12
#define CON_MENU -13
#define CON_COPYOVER_RECOVER -14
/*
 * For Hotreboot
 */
#define CON_QUITTING -15
#define CON_RECONNECTING -16
/* Descriptor is waiting for an async player load from the DB */
#define CON_LOADING_FROM_DB -17
#define CON_SETTING_STATS 1

/* values used to check a new player has selected all options.... */
#define CHECK_RACE 1
#define CHECK_CLASS 2
#define CHECK_SEX 4
#define CHECK_STATS 8

/*
 * TO types for act.
 */
#define TO_ROOM 0
#define TO_NOTVICT 1
#define TO_VICT 2
#define TO_CHAR 3

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK 1
#define ROOM_REGEN 2 /* increased regen in this room */
#define ROOM_NO_MOB 4
#define ROOM_INDOORS 8
#define ROOM_NO_MAGIC 16
#define ROOM_HOT 32    /* players lose hp each tick   */
#define ROOM_COLD 64   /* players lose hp each tick   */
#define ROOM_PK 128    /* players may freely pk here  */
#define ROOM_QUIET 256 /* room is quiet               */
#define ROOM_PRIVATE 512
#define ROOM_SAFE 1024
#define ROOM_SOLITARY 2048
#define ROOM_PET_SHOP 4096
#define ROOM_NO_RECALL 8192
#define ROOM_NO_TELEPORT 16384
#define ROOM_HUNT_MARK 32768 /* Dont ever set! */
#define ROOM_NOBLOODWALK 65536
#define ROOM_NO_PORTAL 131072
#define ROOM_NO_REPOP BIT_19
#define ROOM_MAZE BIT_20

/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH 0
#define DIR_EAST 1
#define DIR_SOUTH 2
#define DIR_WEST 3
#define DIR_UP 4
#define DIR_DOWN 5

/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR 1
#define EX_CLOSED 2
#define EX_LOCKED 4
#define EX_CLIMB 8
#define EX_STAFF 16
#define EX_PICKPROOF 32
#define EX_SMASHPROOF 64
#define EX_PASSPROOF 128
#define EX_NODETECT 256

/*
 * Sector types.
 * Used in #ROOMS.
 */

#define SECT_NULL 0
#define SECT_CITY 1
#define SECT_FIELD 2
#define SECT_FOREST 3
#define SECT_HILLS 4
#define SECT_MOUNTAIN 5
#define SECT_WATER_SWIM 6
#define SECT_WATER_NOSWIM 7
#define SECT_RECALL_OK 8
#define SECT_AIR 9
#define SECT_DESERT 10
#define SECT_INSIDE 11
#if 0
#define SECT_HERE 12
#define SECT_UNSEEN 13
#define SECT_BLOCKED 14
#define SECT_TOP 15
#endif

#define SECT_MAX 12

/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE -1
#define WEAR_LIGHT 0
#define WEAR_HALO 1
#define WEAR_AURA 2
#define WEAR_HORNS 3
#define WEAR_HEAD 4
#define WEAR_FACE 5
#define WEAR_BEAK 6
#define WEAR_EAR_L 7
#define WEAR_EAR_R 8
#define WEAR_NECK_1 9
#define WEAR_NECK_2 10
#define WEAR_WINGS 11
#define WEAR_SHOULDERS 12
#define WEAR_ARMS 13
#define WEAR_WRIST_L 14
#define WEAR_WRIST_R 15
#define WEAR_HANDS 16
#define WEAR_FINGER_L 17
#define WEAR_FINGER_R 18
#define WEAR_CLAWS 19
#define WEAR_HOLD_HAND_L 20
#define WEAR_HOLD_HAND_R 21
#define WEAR_TWO_HANDED 22
#define WEAR_BUCKLER 23
#define WEAR_ABOUT 24
#define WEAR_WAIST 25
#define WEAR_BODY 26
#define WEAR_TAIL 27
#define WEAR_LEGS 28
#define WEAR_FEET 29
#define WEAR_HOOVES 30
#define WEAR_CLAN_COLORS 31
#define WEAR_INVASION_EMBLEM 32

#define MAX_WEAR 33

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Positions.
 */
#define POS_DEAD 0
#define POS_MORTAL 1
#define POS_INCAP 2
#define POS_STUNNED 3
#define POS_SLEEPING 4
#define POS_RESTING 5
#define POS_FIGHTING 6
#define POS_STANDING 7
#define POS_WRITING 8
#define POS_BUILDING 9

/*
 *  Configuration Bits for players
 */

#define CONFIG_AUTOEXIT BIT_1
#define CONFIG_AUTOLOOT BIT_2
#define CONFIG_AUTOSAC BIT_3
#define CONFIG_BLANK BIT_4
#define CONFIG_BRIEF BIT_5
#define CONFIG_COMBINE BIT_6
#define CONFIG_PROMPT BIT_7
#define CONFIG_TELNET_GA BIT_8
#define CONFIG_COLOR BIT_9
#define CONFIG_AUTOSPLIT BIT_10
#define CONFIG_FULL_ANSI BIT_11
#define CONFIG_MAPPER BIT_12
#define CONFIG_JUSTIFY BIT_13
#define CONFIG_AUTOMONEY BIT_14
#define CONFIG_AUTOASSIST BIT_15
#define CONFIG_SHORT_FIGHT BIT_16

/*
 * ACT bits for players.
 */
#define PLR_IS_NPC 1 /* Don't EVER set.      */
#define PLR_BOUGHT_PET 2
#define PLR_CLAN_LEADER 4 /* Clan Leader!! */
/* Available
#define PLR_AUTOEXIT                 BIT_4
#define PLR_AUTOLOOT                 BIT_5
#define PLR_AUTOSAC                  BIT_6
#define PLR_BLANK                    BIT_7
#define PLR_BRIEF                    BIT_8
*/

#define PLR_NO_PRAY 256
/* Available
#define PLR_COMBINE                 BIT_10
#define PLR_PROMPT                 BIT_11
#define PLR_TELNET_GA              BIT_12
*/
#define PLR_HOLYLIGHT 4096
#define PLR_WIZINVIS 8192
#define PLR_BUILDER 16384 /* Is able to use the OLC */
#define PLR_SILENCE 32768

#define PLR_NO_EMOTE 65536
/* avaiable
#define PLR_color		 BIT_18
*/
#define PLR_NO_TELL 262144
#define PLR_LOG 524288
#define PLR_DENY 1048576
#define PLR_FREEZE 2097152
#define PLR_THIEF 4194304
#define PLR_KILLER 8388608
#define PLR_NOSUMMON 16777216
#define PLR_NOVISIT 33554432
/* available
#define PLR_QUESTING	        BIT_27
#define PLR_AUTOSPLIT         BIT_28
#define PLR_AUTOASSIST        BIT_29
*/

#define PLR_NOBLOOD 536870912

#define PLR_INCOG 1073741824

/*
 * Obsolete bits.
 */
#if 0
#define PLR_AUCTION 4       /* Obsolete     */
#define PLR_CHAT 256        /* Obsolete     */
#define PLR_NO_SHOUT 131072 /* Obsolete     */
#endif

/*
 * Player flags
 */
#define PFLAG_PKOK 1
#define PFLAG_AFK 2
#define PFLAG_AMBAS 4
#define PFLAG_CLAN_DIPLOMAT 16
#define PFLAG_CLAN_BOSS 32
#define PFLAG_CLAN_TREASURER 64
#define PFLAG_CLAN_ARMOURER 128
#define PFLAG_CLAN_LEADER 256
#define PFLAG_SUPER_COUNCIL 512
#define PFLAG_RULER BIT_14
#define PFLAG_BLIND_PLAYER BIT_15
#define PFLAG_TESTER BIT_16

/*
 * Channel bits.
 */
#define CHANNEL_AUCTION 1
#define CHANNEL_GOSSIP 2
#define CHANNEL_MUSIC 4
#define CHANNEL_STAFFTALK 8
#define CHANNEL_NEWBIE 16
#define CHANNEL_QUESTION 32
#define CHANNEL_SHOUT 64
#define CHANNEL_YELL 128
#define CHANNEL_FLAME 256
#define CHANNEL_ZZZ 512
#define CHANNEL_RACE 1024
#define CHANNEL_CLAN 2048
#define CHANNEL_NOTIFY 4096
#define CHANNEL_INFO 8192
#define CHANNEL_LOG 16384
#define CHANNEL_CREATOR 32768
#define CHANNEL_ALLCLAN 65536
#define CHANNEL_ALLRACE 131072
#define CHANNEL_HERMIT 262144 /* Turns off ALL channels */
#define CHANNEL_BEEP 524288
#define CHANNEL_DIPLOMAT 2097152
#define CHANNEL_CRUSADE 4194304
#define CHANNEL_REMORTTALK 8388608
#define CHANNEL_ADEPT BIT_26
/* #define CHANNEL_MAUCTION        67108864  */
#define CHANNEL_OOC BIT_27
#define CHANNEL_INVASION BIT_28
#define CHANNEL_LANG BIT_29
#define CHANNEL_GAME BIT_30

#define CHANNEL_PUB_SOCIETY BIT_31

/*
 * NOTE 32 is the last allowable channel ZEN
 */

/* Monitor channels - for staff to select what mud-based info they receive */
#define MONITOR_CONNECT 1
#define MONITOR_AREA_UPDATE 2
#define MONITOR_AREA_BUGS 4
#define MONITOR_AREA_SAVING 8
#define MONITOR_GEN_STAFF 16
#define MONITOR_GEN_MORT 32
#define MONITOR_COMBAT 64
#define MONITOR_HUNTING 128
#define MONITOR_BUILD 256
#define MONITOR_CLAN 512
#define MONITOR_OBJ 1024
#define MONITOR_MOB 2048
#define MONITOR_ROOM 4096
#define MONITOR_MAGIC 8192
#define MONITOR_BAD 16384
#define MONITOR_DEBUG BIT_16
#define MONITOR_SYSTEM BIT_18
/*
 * Hunt flags for mobs
 */
#define HUNT_WORLD 0x00000001      /* Search the whole world  */
#define HUNT_OPENDOOR 0x00000002   /* Can open obstructung doors */
#define HUNT_UNLOCKDOOR 0x00000004 /* Can unlock obstructing doors  */
#define HUNT_PICKDOOR 0x00000008   /* Can pick obstructing doors */
#define HUNT_INFORM 0x00000010     /* Yells while hunting     */
#define HUNT_CR 0x00000020         /* Is preforming a CR      */
#define HUNT_MERC 0x00000040       /* Is gonna assassinate someone  */
#define HUNT_ALL 0x0000001E        /* can hunt through anything    */

#define ERROR_PROG -1
#define IN_FILE_PROG 0
#define ACT_PROG 1
#define SPEECH_PROG 2
#define RAND_PROG 4
#define FIGHT_PROG 8
#define DEATH_PROG 16
#define HITPRCNT_PROG 32
#define ENTRY_PROG 64
#define GREET_PROG 128
#define ALL_GREET_PROG 256
#define GIVE_PROG 512
#define BRIBE_PROG 1024

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/********************* Define Flags for hunting *************************/
#define ACT_HUNT_CHAR 1   /* Hunting a character     */
#define ACT_HUNT_OBJ 2    /* Hunting an object       */
#define ACT_HUNT_INFORM 4 /* Mob will gossip when hunting  */
#define ACT_HUNT_CR 8     /* Mob is doing a corpse retrival   */
#define ACT_HUNT_MOVE 16  /* Just walking somewhere      */

#define NO_VNUM -1 /* For ch->move_to thingy      */

/************************************************************************/

/** Define what is calling the trigger handler for objects ******/
#define TRIGGER_OPEN 1
/*
 * Opening a container
 */
#define TRIGGER_CLOSE 2
/*
 * Closing a container
 */
#define TRIGGER_EAT 3
/*
 * Eating some food
 */
#define TRIGGER_C_DRINK 4
/*
 * Drinking from container
 */
#define TRIGGER_F_DRINK 5
/*
 * Drinking from fountain
 */
#define TRIGGER_WEAR 6
/*
 * Wearing an object
 */
#define TRIGGER_GET 7
/*
 * Picking up an object
 */
#define TRIGGER_DROP 8
/*
 * Dropping an object
 */
#define TRIGGER_EXAMINE 9
/*
 * Examining an object
 */
/** Define what actions the triggered object can do *************/
#define ACTION_TRANSPORT 1
/*
 * Transports victim to room
 */
#define ACTION_RESTORE 2
/*
 * Restores victim
 */
#define ACTION_SLAY 3
/*
 * Kills victim
 */
#define ACTION_TRANSFORM 4
/*
 * Loads mob(s) in its place
 */
/****************************************************************/

/* BitVector flags for room-affect spells.  These are used in the
   ROOM_AFFECT_DATA structure, and passes to the major handling
   functions.  They MUST be used whenever a room-affect spell is
   being cast (from within code) */

#define ROOM_BV_NONE 0
#define ROOM_BV_SILENCE 1           /* Like silence room flag  */
#define ROOM_BV_SAFE 2              /* Like safe room flag     */
#define ROOM_BV_ENCAPS 4            /* Magically blocks exits   */
#define ROOM_BV_SHADE 8             /* Room is darkened      */
#define ROOM_BV_HEAL_REGEN 16       /* room heals hits quicker     */
#define ROOM_BV_HEAL_STEAL 32       /* room takes hits instead of giving */
#define ROOM_BV_MANA_REGEN 64       /* room heals mana quicker     */
#define ROOM_BV_MANA_STEAL 128      /* room saps mana              */
#define ROOM_BV_FIRE_RUNE 256       /* room does fire damage       */
#define ROOM_BV_FIRE_TRAP 512       /* room is fire trapped        */
#define ROOM_BV_DAMAGE_TRAP 1024    /* room is physical damage trapped */
#define ROOM_BV_SHOCK_RUNE 2048     /* room is shock runed         */
#define ROOM_BV_SHOCK_TRAP 4096     /* room is shock trapped       */
#define ROOM_BV_SPELL_ON_ENTER 8192 /* room spell cast on entrance */
#define ROOM_BV_SPELL_ALWAYS 16384  /* room casts spell continuously */
#define ROOM_BV_HOLD 32768          /* room lets you in, but not out..recall works */
#define ROOM_BV_POISON_RUNE 65536   /* room gives poison to entering ch */
#define ROOM_BV_SOUL_NET 131072     /* makes soul instead of corpse */

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC 1             /* Auto set for mobs    */
#define ACT_SENTINEL 2           /* Stays in one room    */
#define ACT_SCAVENGER 4          /* Picks up objects     */
#define ACT_REMEMBER 8           /* remembers target     */
#define ACT_NO_FLEE 16           /* can't flee from mob  */
#define ACT_AGGRESSIVE 32        /* Attacks PC's         */
#define ACT_STAY_AREA 64         /* Won't leave area     */
#define ACT_WIMPY 128            /* Flees when hurt      */
#define ACT_PET 256              /* Auto set for pets    */
#define ACT_TRAIN 512            /* Can train PC's       */
#define ACT_PRACTICE 1024        /* Can practice PC's    */
#define ACT_MERCENARY 2048       /* Is a mercenary       */
#define ACT_HEAL 4096            /* Sells spells         */
#define ACT_ADAPT 8192           /* Adapts weapons       */
#define ACT_UNDEAD 16384         /* Mob is undead  */
#define ACT_BANKER 32768         /* Is a *B*anker :P     */
#define ACT_NO_BODY 65536        /* no body for damage   */
#define ACT_HUNTER 131072        /* hunts attackers      */
#define ACT_NOMIND 262144        /* Psi attack no-no     */
#define ACT_POSTMAN 524288       /* Postmaster     */
#define ACT_REWIELD 1048576      /* Uses better weapons  */
#define ACT_RE_EQUIP 2097152     /* Uses better equipment */
#define ACT_NO_HUNT 16777216     /* Mob cannot be hunted */
#define ACT_SOLO 33554432        /* Mob is buffed for solo combat */
#define ACT_BOSS BIT_27          /* Mob is buffed for boss combat */
#define ACT_QUARTERMASTER BIT_28 /* Sells invasion rewards */
#define ACT_NOBLOOD BIT_29
#define ACT_INVASION BIT_30
#define ACT_NOASSIST BIT_31
#define ACT_DAYONLY BIT_32   /* Mob only active during day */
#define ACT_NIGHTONLY BIT_33 /* Mob only active during night */
#define BIT_34 8589934592ULL
#define ACT_AI_DIALOGUE BIT_34 /* NPC responds to say via LLM */

/* NPC Dialogue AI (tng-ai service) */
#define TNGAI_URL "http://192.168.1.111:8000/v1/chat"
#define TNGAI_MODEL "llama-3.3-70b-versatile"
#define TNGAI_TIMEOUT 5L     /* socket timeout in seconds */
#define TNGAI_MAX_TOKENS 100 /* token cap for NPC responses (1-3 sentences) */
/* Crusade AI generation script (relative to area/ run directory) */
#define CRUSADE_AI_SCRIPT "../src/quests/crusade_ai_gen.py"
/* Timeout for the crusade AI child process (seconds) */
#define CRUSADE_AI_TIMEOUT 10
#define MAX_DIALOGUE_TURNS 8
#define MAX_REQUEST_TURNS 9         /* history + new user turn */
#define DIALOGUE_HISTORY_EXPIRY 300 /* seconds of silence before history resets */

/* Knowledge topic bitmask (ai_knowledge field) */
#define KNOW_WEAPONS (1 << 0)
#define KNOW_TRADE (1 << 1)
#define KNOW_MAGIC (1 << 2)
#define KNOW_TEMPLE (1 << 3)
#define KNOW_UNDERWORLD (1 << 4)
#define KNOW_HARBOR (1 << 5)
#define KNOW_GUARD (1 << 6)
#define KNOW_HISTORY (1 << 7)
#define KNOW_WILDERNESS (1 << 8)
#define KNOW_POLITICS (1 << 9)
#define KNOW_HELPS (1 << 10) /* search help/shelp files at AI dispatch time */
#define NUM_KNOW_FLAGS 10

/* City indices for knowledge lookup table */
#define CITY_GLOBAL 0
#define CITY_MIDGAARD 1
#define CITY_KOWLOON 2
#define CITY_KIESS 3
#define CITY_RAKUEN 4
#define CITY_MAFDET 5
#define NUM_CITIES 6

/* City vnum ranges (room vnums) used by city_for_room() */
#define MIDGAARD_VNUM_MIN 1100
#define MIDGAARD_VNUM_MAX 1699
#define KIESS_VNUM_MIN 3650
#define KIESS_VNUM_MAX 3749
#define KOWLOON_VNUM_MIN 3750
#define KOWLOON_VNUM_MAX 3849
#define MAFDET_VNUM_MIN 3850
#define MAFDET_VNUM_MAX 3949
#define RAKUEN_VNUM_MIN 4550
#define RAKUEN_VNUM_MAX 4749

/* City accent constants (accent field on MOB_INDEX_DATA) */
#define ACCENT_NONE 0
#define ACCENT_MIDGAARD 1 /* clipped bureaucratic, ledger idiom */
#define ACCENT_KOWLOON 2  /* terse, elliptic, proverb-laden */
#define ACCENT_MAFDET 3   /* formal titles, trade jargon, oath references */
#define ACCENT_KIESS 4    /* measured, watchful, frontier caution */
#define ACCENT_RAKUEN 5   /* warm but weary, garden metaphors */
#define MAX_ACCENT 6

/* build bits for OLC -S- */
#define ACT_BUILD_NOWT 0  /* not doing anything   */
#define ACT_BUILD_REDIT 1 /* editing rooms        */
#define ACT_BUILD_OEDIT 2 /* editing objects      */
#define ACT_BUILD_MEDIT 3 /* editing mobiles      */

#define NO_USE -999 /* this table entry can */
                    /*
                     * NOT be used, except
                     */
                    /*
                     * by a Creator
                     */

/*
 * Macros for sparse skill/spell level definitions in skill_table_data.c and
 * spell_table_data.c.  Instead of listing all MAX_TOTAL_CLASS entries, only
 * specify the classes that actually receive the skill:
 *
 *   {LEVELS_INIT, L(CLASS_MAG, 5), L(CLASS_WIZ, 3)}
 *
 * LEVELS_INIT uses a GCC range designator to fill all entries with NO_USE.
 * Each L(cls, lvl) then overrides the specific class entry.
 */
#define LEVELS_INIT [0 ... MAX_TOTAL_CLASS - 1] = NO_USE
#define L(cls, lvl) [cls] = (lvl)

/*
 * New bits to determine what skills a mob can do in combat -S-
 */

#define MOB_NONE 0
#define MOB_SECOND BIT_1
#define MOB_THIRD BIT_2
#define MOB_FOURTH BIT_3
#define MOB_FIFTH BIT_4
#define MOB_SIXTH BIT_5
#define MOB_NODISARM BIT_6
#define MOB_NOTRIP BIT_7
#define MOB_PUNCH BIT_8
#define MOB_HEADBUTT BIT_9
#define MOB_KNEE BIT_10
#define MOB_DISARM BIT_11
#define MOB_TRIP BIT_12
#define MOB_DODGE BIT_13
#define MOB_PARRY BIT_14
#define MOB_MARTIAL BIT_15
#define MOB_ENHANCED BIT_16
#define MOB_DUALWIELD BIT_17
#define MOB_DIRT BIT_18
#define MOB_CHARGE BIT_19
#define MOB_COUNTER BIT_20
#define MOB_KICK BIT_21

/*
 * New bits to determine what spells a mob will cast in combat -S-
 * These are for offensive spells.
 * Remember spec_funs may still cast as well! - but spells cast
 *  this way WILL reduce the mob's mana <g>
 */

#define CAST_NONE BIT_1
#define CAST_MAGIC_MISSILE BIT_2
#define CAST_SHOCKING_GRASP BIT_3
#define CAST_BURNING_HANDS BIT_4
#define CAST_COLOR_SPRAY BIT_5
#define CAST_FIREBALL BIT_6
#define CAST_HELLSPAWN BIT_7
#define CAST_ACID_BLAST BIT_8
#define CAST_CHAIN_LIGHTNING BIT_9
#define CAST_FLARE BIT_10
#define CAST_FLAMESTRIKE BIT_11
#define CAST_EARTHQUAKE BIT_12
#define CAST_MIND_FLAIL BIT_13
#define CAST_PLANERGY BIT_14
#define CAST_PHOBIA BIT_15
#define CAST_MIND_BOLT BIT_16
#define CAST_STATIC BIT_17
#define CAST_EGO_WHIP BIT_18
#define CAST_BLOODY_TEARS BIT_19
#define CAST_MINDFLAME BIT_20
#define CAST_SUFFOCATE BIT_21
#define CAST_NERVE_FIRE BIT_22
#define CAST_LIGHTNING_BOLT BIT_23
#define CAST_HEAT_ARMOR BIT_24
#define CAST_LAVA_BURST BIT_25

/* Mob def */
#define DEF_NONE BIT_1
#define DEF_CURE_LIGHT BIT_2
#define DEF_CURE_SERIOUS BIT_3
#define DEF_CURE_CRITIC BIT_4
#define DEF_HEAL BIT_5
#define DEF_FIRESHIELD BIT_6
#define DEF_ICESHIELD BIT_7
#define DEF_SHOCKSHIELD BIT_8

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND 1
#define AFF_INVISIBLE 2
#define AFF_DETECT_EVIL 4
#define AFF_DETECT_INVIS 8
#define AFF_DETECT_MAGIC 16
#define AFF_DETECT_HIDDEN 32
#define AFF_CLOAK_REFLECTION 64
#define AFF_SANCTUARY 128
#define AFF_FAERIE_FIRE 256
#define AFF_INFRARED 512
#define AFF_CURSE 1024
#define AFF_CLOAK_FLAMING 2048
#define AFF_POISON 4096
#define AFF_PROTECT 8192
#define AFF_CLOAK_ABSORPTION 16384
#define AFF_SNEAK 32768
#define AFF_HIDE 65536
#define AFF_SLEEP 131072
#define AFF_CHARM 262144
#define AFF_FLYING 524288
#define AFF_PASS_DOOR 1048576
#define AFF_ANTI_MAGIC 2097152 /* -S- no magic hurts ch */
#define AFF_BLASTED BIT_23
#define AFF_REMORT_CURSE BIT_24
#define AFF_CONFUSED BIT_25
#define AFF_HOLD BIT_27
#define AFF_PARALYSIS BIT_28
#define AFF_CLOAK_ADEPT BIT_29

/* VAMP_HEALING is used to show that a vamp is currently healing after death*/

/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL 0
#define SEX_MALE 1
#define SEX_FEMALE 2

/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT 1
#define ITEM_SCROLL 2
#define ITEM_STAFF 4
#define ITEM_WEAPON 5
#define ITEM_BEACON 6 /* for portals, etc */
#define ITEM_PORTAL 7
#define ITEM_TREASURE 8
#define ITEM_ARMOR 9
#define ITEM_POTION 10
#define ITEM_CLUTCH 11 /* for clutch location only! */
#define ITEM_FURNITURE 12
#define ITEM_TRASH 13
#define ITEM_TRIGGER 14
#define ITEM_CONTAINER 15
#define ITEM_QUEST 16
#define ITEM_KEY 18
#define ITEM_MONEY 20
#define ITEM_STAKE 21
#define ITEM_BOAT 22
#define ITEM_CORPSE_NPC 23
#define ITEM_CORPSE_PC 24
#define ITEM_PILL 26
#define ITEM_BOARD 27
#define ITEM_SOUL 28
#define ITEM_PIECE 29
#define ITEM_SPELL_MATRIX 30
#define ITEM_ENCHANTMENT 31
#define ITEM_WRAPPED_PRESENT 32
#define ITEM_STASH 33
/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GENERATED 1
#define ITEM_BIND_EQUIP 2
#define ITEM_NODISARM 4
#define ITEM_LOCK 8
#define ITEM_EVIL 16
#define ITEM_INVIS 32
#define ITEM_MAGIC 64
#define ITEM_NODROP 128
#define ITEM_BLESS 256
#define ITEM_ANTI_GOOD 512
#define ITEM_ANTI_EVIL 1024
#define ITEM_UNUSED_ANTI_NEUTRAL 2048 /* intentionally unused (anti-neutral removed) */
#define ITEM_NOREMOVE 4096
#define ITEM_INVENTORY 8192
#define ITEM_NOSAVE 16384       /* For "quest" items :) */
#define ITEM_CLAN_EQ 32768      /* only leaders and creators may load */
#define ITEM_TRIG_DESTROY 65536 /* Trigger object that destroy after use */
#define ITEM_NO_AUCTION 131072  /* Can't be auctioned */
#define ITEM_MYTHIC BIT_19
#define ITEM_LEGENDARY BIT_20
#define ITEM_RARE BIT_21
#define ITEM_QUEST_REWARD BIT_22
#define ITEM_NOLOOT BIT_23
#define ITEM_NOSAC BIT_24
#define ITEM_UNIQUE BIT_25
#define ITEM_LIFESTEALER BIT_26
#define ITEM_LOOT BIT_27
#define ITEM_BOSS BIT_28
#define ITEM_BUCKLER BIT_29
#define ITEM_EXTRA_WAND BIT_30
#define ITEM_FIST BIT_31
#define ITEM_TWO_HANDED BIT_32
#define ITEM_BONDED BIT_33

/* Class-restricted stuff dropped. */

#define ITEM_APPLY_NONE 1
#define ITEM_APPLY_INFRA 2
#define ITEM_APPLY_INV 4
#define ITEM_APPLY_DET_INV 8
#define ITEM_APPLY_SANC 16
#define ITEM_APPLY_SNEAK 32
#define ITEM_APPLY_HIDE 64
#define ITEM_APPLY_PROT 128
#define ITEM_APPLY_ENHANCED 256
#define ITEM_APPLY_DET_MAG 512
#define ITEM_APPLY_DET_HID 1024
#define ITEM_APPLY_DET_EVIL 2048
#define ITEM_APPLY_PASS_DOOR 4096
#define ITEM_APPLY_DET_POISON 8192
#define ITEM_APPLY_FLY 16384
#define ITEM_APPLY_KNOW_ALIGN 32768
#define ITEM_APPLY_DET_UNDEAD 65536
#define ITEM_APPLY_HEATED 131072

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_WEAR_NONE BIT_0
#define ITEM_WEAR_HALO BIT_1
#define ITEM_WEAR_AURA BIT_2
#define ITEM_WEAR_HORNS BIT_3
#define ITEM_WEAR_HEAD BIT_4
#define ITEM_WEAR_FACE BIT_5
#define ITEM_WEAR_BEAK BIT_6
#define ITEM_WEAR_EAR BIT_7
#define ITEM_WEAR_NECK BIT_8
#define ITEM_WEAR_WINGS BIT_9
#define ITEM_WEAR_SHOULDERS BIT_10
#define ITEM_WEAR_ARMS BIT_11
#define ITEM_WEAR_WRIST BIT_12
#define ITEM_WEAR_HANDS BIT_13
#define ITEM_WEAR_FINGER BIT_14
#define ITEM_WEAR_CLAWS BIT_15
#define ITEM_WEAR_HOLD_HAND BIT_16
#define ITEM_WEAR_ABOUT BIT_17
#define ITEM_WEAR_WAIST BIT_18
#define ITEM_WEAR_BODY BIT_19
#define ITEM_WEAR_TAIL BIT_20
#define ITEM_WEAR_LEGS BIT_21
#define ITEM_WEAR_FEET BIT_22
#define ITEM_WEAR_HOOVES BIT_23
#define ITEM_TAKE BIT_24
#define ITEM_WEAR_CLAN_COLORS BIT_25
#define ITEM_WEAR_INVASION_EMBLEM BIT_26
#define MAX_ITEM_BIT 26
/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE 0
#define APPLY_STR 1
#define APPLY_DEX 2
#define APPLY_INT 3
#define APPLY_WIS 4
#define APPLY_CON 5
#define APPLY_SEX 6
#define APPLY_CLASS 7
#define APPLY_LEVEL 8
#define APPLY_AGE 9
#define APPLY_HEIGHT 10
#define APPLY_WEIGHT 11
#define APPLY_MANA 12
#define APPLY_HIT 13
#define APPLY_MOVE 14
#define APPLY_GOLD 15
#define APPLY_EXP 16
#define APPLY_AC 17
#define APPLY_HITROLL 18
#define APPLY_DAMROLL 19
#define APPLY_SAVING_PARA 20
#define APPLY_SAVING_ROD 21
#define APPLY_SAVING_PETRI 22
#define APPLY_SAVING_BREATH 23
#define APPLY_SAVING_SPELL 24
#define APPLY_HOT 25
#define APPLY_DOT 26
#define APPLY_SPELLPOWER 27
#define APPLY_SPELL_CRIT 28
#define APPLY_SPELL_CRIT_MULT 29
#define APPLY_CRIT 30
#define APPLY_CRIT_MULT 31
#define APPLY_PARRY 32
#define APPLY_DODGE 33
#define APPLY_BLOCK 34
#define APPLY_PIERCE_AVOIDANCE 35
#define APPLY_DAMCAP 36
#define APPLY_SPEED 37

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE 1
#define CONT_PICKPROOF 2
#define CONT_CLOSED 4
#define CONT_LOCKED 8
#define CONT_KEEP_CHEST 16

/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */
#if defined(macintosh)
#define PLAYER_DIR ""         /* Player files                 */
#define NULL_FILE "proto.are" /* To reserve one stream        */
#define MOB_DIR ""            /* Script files                */
#endif

#if defined(MSDOS)
#define PLAYER_DIR ""   /* Player files                 */
#define NULL_FILE "nul" /* To reserve one stream        */
#define MOB_DIR ""      /* Script files                */
#endif

#if defined(unix)
#define PLAYER_DIR "../player/" /* Player files                 */
#define NULL_FILE "/dev/null"   /* To reserve one stream        */
#define MOB_DIR "Scripts/"      /* Script files                */
#endif

#if defined(linux)
#define PLAYER_DIR "../player/" /* Player files                 */
#define NULL_FILE "/dev/null"   /* To reserve one stream        */
#define MOB_DIR "Scripts/"      /* Script files                */
#endif

#define AREA_DIR "../area/"
#define AREA_LIST AREA_DIR "area.lst" /* List of areas                */
#define KNOWLEDGE_DIR "../data/knowledge/"

/*
 * Lore perspective flags -- used on both lore entries and NPCs.
 * When a player uses "lore <topic>" near an NPC with lore_flags,
 * the system picks the entry whose required flags best match the NPC's flags.
 */
/* City flags */
#define LORE_FLAG_MIDGAARD (1 << 0)
#define LORE_FLAG_KIESS (1 << 1)
#define LORE_FLAG_KOWLOON (1 << 2)
#define LORE_FLAG_RAKUEN (1 << 3)
#define LORE_FLAG_MAFDET (1 << 4)
/* Race flags */
#define LORE_FLAG_HUMAN (1 << 5)
#define LORE_FLAG_KHENARI (1 << 6)
#define LORE_FLAG_KHEPHARI (1 << 7)
#define LORE_FLAG_ASHBORN (1 << 8)
#define LORE_FLAG_UMBRAL (1 << 9)
#define LORE_FLAG_RIVENNID (1 << 10)
#define LORE_FLAG_DELTARI (1 << 11)
#define LORE_FLAG_USHABTI (1 << 12)
#define LORE_FLAG_SERATHI (1 << 13)
#define LORE_FLAG_KETHARI (1 << 14)

#define MAIL_DIR "../mail/"
#define DATA_DIR "../data/"
#define BUG_DIR "../reports/"
#define BUG_FILE BUG_DIR "bugs.txt"    /* For 'bug' and bug( )         */
#define IDEA_FILE BUG_DIR "ideas.txt"  /* For 'idea'                   */
#define TYPO_FILE BUG_DIR "typos.txt"  /* For 'typo'                   */
#define NOTE_FILE DATA_DIR "notes.txt" /* For 'notes'                  */
/* FIXME: boards.txt in here????? */
#define SHUTDOWN_FILE BUG_DIR "shutdown.txt" /* For 'shutdown'               */

#define CLAN_FILE DATA_DIR "clandata.dat" /* stores clan diplomacy data   */
#define CORPSE_FILE DATA_DIR "corpses.lst"
#define CHEST_DIR DATA_DIR "chest/"
#define MARKS_FILE DATA_DIR "roommarks.lst"
#define BANS_FILE DATA_DIR "bans.lst"
#define RULERS_FILE DATA_DIR "rulers.lst"
#define CONTROLS_FILE DATA_DIR "area_controls.lst"
#define BRANDS_FILE DATA_DIR "brands.lst"

#define SYSDAT_FILE DATA_DIR "system.dat"

/* Other Stuff - Flar */
#define COPYOVER_FILE "COPYOVER.TXT"               /* Temp data file used for copyover */
#define EXE_FILE "../src/ack"                      /* The one that runs the ACK! */
#define HOTREBOOT_FILE "../data/hotreboot.trigger" /* Touched by deploy to trigger hotreboot */
