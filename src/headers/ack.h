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

#define DEC_ACK_H 1

#ifndef DEC_TYPEDEFS_H
#include "typedefs.h"
#endif

#ifndef DEC_CONFIG_H
#include "config.h"
#endif

#ifndef DEC_UTILS_H
#include "utils.h"
#endif

#ifndef DEC_GLOBALS_H
#include "globals.h"
#endif

/* Various linked lists head/tail pointer declarations. -- Altrag */
#ifndef DEC_LISTS_H
#include "lists.h"
#endif

#ifndef DEC_STRFUNS_H
#include "strfuns.h"
#endif

#ifndef DEC_ACT_MOB_H
#include "act_mob.h"
#endif

#ifndef QUEST_H
#include "quest.h"
#endif

#ifndef PUB_SOCIETY_H
#include "pub_society.h"
#endif

#ifndef INVASION_H
#include "invasion.h"
#endif

struct str_array
{
   char *this_string;
};

struct sysdata_type
{
   bool w_lock;
   bool test_open;
   char *playtesters;
   STR_ARRAY staff[MAX_NUM_STAFF];
   bool shownumbers;
};

struct board_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   BOARD_DATA *next;
   BOARD_DATA *prev;
   int vnum;
   MESSAGE_DATA *first_message;
   MESSAGE_DATA *last_message;
   int min_read_lev;
   int min_write_lev;
   int expiry_time;
   int clan;
};

struct message_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   MESSAGE_DATA *next;
   MESSAGE_DATA *prev; /* Only used in save_board */
   BOARD_DATA *board;
   time_t datetime;
   char *author;
   char *title;
   char *message;
};

/*
 * color look-up table structure thingy.
 */
struct color_type
{
   char *name; /* eg, gossip, say, look */
   int index;  /* unique index */
};

struct ansi_type
{
   char *name;
   char *value; /* escape sequence, or whatever */
   int index;
   char letter;
   int stlen;
};

#define color_NORMAL "\033[0m"

/* Structure for material/strengths */
struct material_type
{
   char *name;    /* Name of the material */
   char *descrip; /* Descr. of how strong it is */
   int quality;   /* 0 = crap, 100 = non-breakable */
};

#define NO_MATERIAL 10 /* Number of materials */

struct dl_list
{
   bool is_free;
   DL_LIST *next;
   DL_LIST *prev;
   void *this_one;
};

/*
 * Site ban structure.
 */
struct ban_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   BAN_DATA *next;
   BAN_DATA *prev;
   char *name;
   char *banned_by;
   bool newbie;
};

struct brand_data
{
   bool is_free;
   BRAND_DATA *next;
   BRAND_DATA *prev;
   char *branded;
   char *branded_by;
   char *dt_stamp;
   char *message;
   char *priority;
};

struct time_info_data
{
   int hour;
   int day;
   int month;
   int year;
   int moon;
};

struct weather_data
{
   int mmhg;
   int change;
   int sky;
   int sunlight;
   int moon_phase;
   int moon_loc;
   bool phase_changed;
};

struct corpse_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   CORPSE_DATA *next;
   CORPSE_DATA *prev;
   OBJ_DATA *this_corpse;
};

struct member_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   CHAR_DATA *this_member;
   MEMBER_DATA *next;
   MEMBER_DATA *prev;
};

struct mark_data
{
   bool is_free;
   int room_vnum;
   char *message;
   char *author;
   sh_int duration;
   sh_int type; /* VAMP, WOLF */
   MARK_DATA *next;
   MARK_DATA *prev; /* for freelist only */
};

struct mark_list_member
{
   bool is_free;
   MARK_LIST_MEMBER *next;
   MARK_LIST_MEMBER *prev;
   MARK_DATA *mark;
};

struct council_data
{
   char *council_name;
   MEMBER_DATA *first_member;
   MEMBER_DATA *last_member;
   bool quorum;
   sh_int council_time;
};

/*
 * Descriptor (channel) structure.
 */
struct descriptor_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   DESCRIPTOR_DATA *next;
   DESCRIPTOR_DATA *prev;
   DESCRIPTOR_DATA *snoop_by;
   CHAR_DATA *character;
   CHAR_DATA *original;
   char *host;
   sh_int descriptor;
   sh_int connected;
   bool fcommand;
   char inbuf[4 * MAX_INPUT_LENGTH];
   int inbuf_len;
   char incomm[MAX_INPUT_LENGTH];
   char inlast[MAX_INPUT_LENGTH];
   int repeat;
   char *showstr_head;
   char *showstr_point;
   char *outbuf;
   int outsize;
   int outtop;
   unsigned int remote_port; /* 'Pair Port' ? -S- */
   int check;                /* For new players */
   int flags;
   int childpid; /* Child process id */
   time_t timeout;
   bool websocket_active;
   bool websocket_handshake_complete;
   bool greeting_sent;
   const char *websocket_current_music; /* mirrors area->music last sent, NULL = theme */
   unsigned int ws_score_checksum;      /* checksum of last Score push; 0 = never sent */

   /* TLS (native OpenSSL encryption) */
#ifdef HAVE_OPENSSL
   struct ssl_st *ssl;
   bool tls_handshake_pending; /* SSL_accept in progress; advanced in game loop */
   bool wss_pending;           /* came in on WSS port; defer greeting until WebSocket upgrade */
#endif
   bool tls_active;

   /* MSDP (telnet option 69) */
   bool msdp_active;
   unsigned int msdp_reported;
   unsigned short msdp_last_room;

   /* GMCP (telnet option 201) */
   bool gmcp_active;
   unsigned int gmcp_supports;
   unsigned short gmcp_last_room;

   /* MCCP2 (server->client compression, telnet option 86) */
   bool mccp2_active;
   void *mccp2_zout;

   /* MCCP3 (client->server compression, telnet option 87) */
   bool mccp3_active;
   void *mccp3_zin;
};

#define DESC_FLAG_PASSTHROUGH 1 /* Used when data is being passed to */
                                /*
                                 * Another prog.
                                 */

struct family_name_type
{
   char *name;
   sh_int clan_enemy;
};

struct politics_data_type
{

   sh_int diplomacy[MAX_CLAN][MAX_CLAN];
   bool daily_negotiate_table[MAX_CLAN][MAX_CLAN];
   long int treasury[MAX_CLAN];
   bool end_current_state[MAX_CLAN][MAX_CLAN];
};

struct stance_app_type
{
   char *name;
   int ac_mod;
   int dr_mod;
   int hr_mod;
   int speed_mod;
   int heal_mod;
   int spell_mod;
   int specials;
   int tier;
   int class_index;
   int class_level;
   int class_index2;
   int class_level2;
   int class_override;
};

/*
 * Shop types.
 */
struct shop_data
{
   bool is_free;               /* Ramias:for run-time checks of LINK/UNLINK */
   SHOP_DATA *next;            /* Next shop in list            */
   SHOP_DATA *prev;            /* Prev shop in list    */
   sh_int keeper;              /* Vnum of shop keeper mob      */
   sh_int buy_type[MAX_TRADE]; /* Item types shop will buy     */
   sh_int profit_buy;          /* Cost multiplier for buying   */
   sh_int profit_sell;         /* Cost multiplier for selling  */
   sh_int open_hour;           /* First opening hour           */
   sh_int close_hour;          /* First closing hour           */
};

/*
 * Per-class stuff.
 */
struct class_type
{
   char who_name[4];  /* Three-letter name for 'who'               */
   char *class_name;  /* Full name                                 */
   sh_int attr_prime; /* Prime attribute                           */
   sh_int hp_gain;
   sh_int mana_gain; /* Class gains mana on level                 */
   sh_int tier;      /* MORTAL / REMORT / ADEPT                   */
   sh_int prereq[2]; /* Prerequisite class IDs; -1 = none         */
};

struct obj_stat_type
{
   float hr_div;
   float dr_div;
   float spellpower_div;
   float ac_div;
   float hp_div;
   float move_div;
   float mana_div;
   float hrdr_bonus;
   float ac_bonus;
   float stat_bonus;
};

struct race_type
{
   char race_name[4]; /* Three letter name for race */
   char *race_title;  /* Full race name */
   sh_int recall;     /* Race's recall location */
   sh_int race_room;  /* vnum of race-only room */
   sh_int race_str;   /* max_str to use for race */
   sh_int race_int;
   sh_int race_wis;
   sh_int race_dex;
   sh_int race_con;
   int race_flags;           /* flags for the various racial stuff    */
   bool wear_locs[MAX_WEAR]; /* on/off for each wear loc */
   int classes;              /* Number of classes for race   */
   sh_int limit[MAX_CLASS];  /* Max for each class */
   char *comment;            /* comments shown for new players */
   char *skill;
   int strong_realms;
   int weak_realms;
   int resist_realms;
   int suscept_realms;
   bool player_allowed;
};

struct clan_type
{
   char *clan_name;     /* The name of the Clan */
   char *clan_abbr;     /* Abbrev. name - FIVE CHARS */
   sh_int donat_room;   /* clan donation */
   sh_int clan_room;    /* Clan-only room */
   char *leader;        /* Clan leader */
   char *enemies;       /* Enemies (if any) */
   int eq[MAX_CLAN_EQ]; /* vnums of objects to load */
};

struct exp_type
{
   long_int mob_base; /* Base exp for mob of level x */
};

/*
 * Data structure for notes.
 */
struct note_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   NOTE_DATA *next;
   NOTE_DATA *prev;
   char *sender;
   char *date;
   char *to_list;
   char *subject;
   char *text;
   time_t date_stamp;
};

/*
 * An affect.
 */
struct affect_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   AFFECT_DATA *next;
   AFFECT_DATA *prev;
   sh_int type;
   sh_int duration_type;
   sh_int duration;
   sh_int location;
   sh_int modifier;
   int bitvector;
   int element;
   CHAR_DATA *caster;
   int level;
};

struct room_affect_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   ROOM_AFFECT_DATA *next;
   ROOM_AFFECT_DATA *prev;
   sh_int duration;
   sh_int level;
   int type;
   int bitvector;
   int applies_spell; /* what spell is cast on a ch by the room..a sn */
   sh_int modifier;
   sh_int location;
   CHAR_DATA *caster;
};

/*
 * A kill structure (indexed by level).
 */
struct kill_data
{
   sh_int number;
   sh_int killed;
};

struct magic_shield
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   MAGIC_SHIELD *next;
   MAGIC_SHIELD *prev;
   sh_int type;       /* what kind is it? Electric, Fire, etc... */
   bool harmfull;     /* does the shield damage the attacker? */
   sh_int attack_dam; /* then hurt the attacker ;) */
   sh_int percent;    /* percent of damage it absorbs per attack */
   sh_int hits;       /* Shield hitpoints */
   int sn;
   char *absorb_message_room;
   char *absorb_message_victim;
   char *absorb_message_self;
   char *name;
   char *wearoff_room;
   char *wearoff_self;
};

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct mob_index_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   MOB_INDEX_DATA *next;
   SPEC_FUN *spec_fun;
   SPEECH_FUN *speech_fun;
   SHOP_DATA *pShop;
   AREA_DATA *area; /* MAG Mod */
   char *player_name;
   char *short_descr;
   char *long_descr;
   char *description;
   unsigned short vnum;
   sh_int count;
   sh_int killed;
   sh_int sex;
   sh_int level;
   unsigned long long int act;
   int affected_by;
   int aggro_list;
   sh_int alignment;
   long hp_mod;
   int ac_mod; /* ac modifier */
   int hr_mod; /* hitroll modifier */
   int dr_mod; /* damroll modifier */
   int spellpower_mod;
   int healing_mod;
   int crit_mod;
   int crit_mult_mod;
   int spell_crit_mod;
   int spell_mult_mod;
   int parry_mod;
   int dodge_mod;
   int block_mod;
   int pierce_mod;
   int loot_amount;
   int loot_chance[MAX_LOOT];
   int loot[MAX_LOOT];
   char *target;            /* last ch to attack */
   sh_int hitroll;          /* Unused */
   sh_int ac;               /* Unused */
   sh_int hitnodice;        /* Unused */
   sh_int hitsizedice;      /* Unused */
   sh_int hitplus;          /* Unused */
   sh_int damnodice;        /* Unused */
   sh_int damsizedice;      /* Unused */
   sh_int damplus;          /* Unused */
   int gold;                /* Unused */
   MPROG_DATA *first_mprog; /* Used by program   */
   MPROG_DATA *last_mprog;
   int progtypes; /* Used by program   */
   int skills;    /* skill flags       */
   int power_skills;
   int cast; /* casting flags  */
   int power_cast;
   int def; /* casting flags  */
   int strong_magic;
   int weak_magic;
   int resist;
   int suscept;
   int race_mods;
   sh_int class;
   sh_int clan;
   sh_int race;
   sh_int position;
   int hunt_flags;
   long lore_flags;
   sh_int pub_society;
   sh_int pub_society_npc_role;
   char *ai_prompt;  /* system prompt persona text; NULL if not AI-enabled */
   int ai_knowledge; /* KNOW_* bitmask of topic blocks to inject */
   sh_int accent;    /* ACCENT_* city accent; ACCENT_NONE = 0 */
};

/*
 * One character (PC or NPC).
 */
struct char_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   bool is_quitting;
   CHAR_DATA *next;
   CHAR_DATA *prev;
   CHAR_DATA *next_in_room;
   CHAR_DATA *prev_in_room;
   CHAR_DATA *master;
   CHAR_DATA *leader;
   CHAR_DATA *fighting;
   CHAR_DATA *reply;

   /*    CHAR_DATA *         hunting;
       char *              huntdirs;
       int                 huntdirno;
       int                 huntlastvnum;
       char *              huntname;
       int			act_hunt;
       OBJ_DATA	*	hunt_obj;
       int			move_to;
       char *		movename;*/

   CHAR_DATA *hunting;         /* For hunting PC's/mobs   */
   OBJ_DATA *hunt_obj;         /* Looking for objects     */
   CHAR_DATA *hunt_for;        /* Employer (crs, mercs)   */
   ROOM_INDEX_DATA *hunt_home; /* Return to after hunting */
   char *searching;            /* For PC's that quit      */
   int hunt_flags;             /* Action flags         */

   bool switched;       /* = not isnull(descriptor->original) */
   CHAR_DATA *old_body; /* = descriptor->original */
   int poly_level;

   SPEC_FUN *spec_fun;
   SPEECH_FUN *speech_fun;
   MOB_INDEX_DATA *pIndexData;
   DESCRIPTOR_DATA *desc;
   AFFECT_DATA *first_affect;
   AFFECT_DATA *last_affect;
   AFFECT_DATA *first_saved_aff;
   AFFECT_DATA *last_saved_aff;
   NOTE_DATA *pnote;
   OBJ_DATA *first_carry;
   OBJ_DATA *last_carry;
   ROOM_INDEX_DATA *in_room;
   ROOM_INDEX_DATA *was_in_room;
   PC_DATA *pcdata;
   char *name;
   OBJ_DATA *sitting; /* What they're resting or sleeping on */
   char *short_descr;
   char *long_descr;
   char *long_descr_orig;
   char *description;
   char *prompt;
   char *old_prompt; /* used to hold prompt when writing */
   sh_int sex;
   sh_int login_sex;
   sh_int class;
   sh_int clan; /* need to convert from pcdata to this */
   sh_int race;
   sh_int level;                     /* For m/c this = max of levels */
   int class_level[MAX_TOTAL_CLASS]; /* Unified class levels (mortal 0-5, remort 6-17, adept 18-23)
                                      */
   int adept_level;
   int combo[MAX_COMBO];
   int holy_power;
   int arcane_power;
   int mental_power;
   int spec_behavior;
   int spec_behavior2; /* secondary state for special functions (e.g., element interrupt tracking)
                        */
   int sentence;       /* For new legal system */
   sh_int invis;       /* For wizinvis staff - lvl invis to */
   sh_int incog;       /* Same as above except for incognito --Flar */
   sh_int trust;
   bool wizbit;
   int played;
   time_t logon;
   time_t save_time;
   time_t last_note;
   sh_int timer;
   sh_int wait;
   long hit;
   long max_hit;
   long mana;
   long max_mana;
   long move;
   long max_move;
   int gold;
   int balance; /* Amount of gold (if any) in bank */
   long exp;
   int intell_exp;
   unsigned long long int act;
   int config;
   int act_build;  /* for setting what ya editing */
   int build_vnum; /* the current vnum for w-y-e  */
   int affected_by;
   sh_int position;
   int practice;
   float carry_weight;
   sh_int carry_number;
   sh_int saving_throw;
   sh_int alignment;
   int hitroll;
   int damroll;
   int armor;
   long hp_mod;
   int ac_mod; /* ac modifier */
   int hr_mod; /* hitroll modifier */
   int dr_mod; /* damroll modifier */
   int spellpower_mod;
   int healing_mod;
   int crit_mod;
   int crit_mult_mod;
   int spell_crit_mod;
   int spell_mult_mod;
   int parry_mod;
   int dodge_mod;
   int block_mod;
   int pierce_mod;
   int loot_amount;
   int loot_chance[MAX_LOOT];
   int loot[MAX_LOOT];
   int chi;
   sh_int overgrowth;
   char *target; /* last ch to attack */
   sh_int wimpy;
   int deaf;
   sh_int cooldown[MAX_SKILL];
   MPROG_ACT_LIST *first_mpact; /* Used by program */
   MPROG_ACT_LIST *last_mpact;
   int mpactnum; /* Used by program */
   int skills;   /* Used for MOBs */
   int cast;
   int def;
   int strong_magic;
   int weak_magic;
   int resist;
   int suscept;
   int race_mods;
   int power_skills;
   int power_cast;

   /* Stuff to handle automatic quests for players */
   int quest_points; /* The reason to quest!  */

   MAGIC_SHIELD *first_shield;
   MAGIC_SHIELD *last_shield;
   int stunTimer;
   sh_int num_followers;
   sh_int extract_timer; /* same as object timer, only use for charmies */
   BRAND_DATA *current_brand;
   int stance;
   bool using_named_door;
   NPC_GROUP_DATA *ngroup;
   long lore_flags;
   void *dlg_state;         /* NPC_DLG_STATE* for ACT_AI_DIALOGUE mobs; NULL otherwise */
   bool dlg_pending;        /* TRUE while an NPC_DLG_REQ is outstanding */
   REVENANT_DATA *revenant; /* non-NULL if this NPC is a revenant */

   /* Sentinel testimony system */
   int testimony;               /* accumulated marks on current target, 0-9 */
   CHAR_DATA *testimony_target; /* who marks are tracked against */
   int testimony_cooldown;      /* rounds until testimony can accumulate again after verdict */
   int testimony_combat_rounds; /* rounds in combat with current target (for passive tick) */
};

/*
 * Data which only PC's have.
 */
struct quest_data
{
   int quest_type;                            /* QUEST_TYPE_* constant; 0=none */
   bool quest_completed;                      /* TRUE = ready to hand in      */
   int quest_num_targets;                     /* slots in use (1-5)           */
   int quest_target_vnum[QUEST_MAX_TARGETS];  /* mob or obj vnum per slot     */
   bool quest_target_done[QUEST_MAX_TARGETS]; /* per-slot completion flag     */
   int quest_kill_needed;                     /* type 3: kill goal            */
   int quest_kill_count;                      /* type 3: kills so far         */
   int quest_template_id;                     /* quest template id or -1       */
   int quest_reward_gold;                     /* template gold reward override */
   int quest_reward_qp;                       /* template qp reward override   */
   int quest_reward_item_vnum;                /* template item reward vnum     */
   int quest_reward_item_count;               /* template item reward quantity */
   int quest_offerer_vnum;                    /* required turn-in mob vnum     */
   int quest_cartography_area_num;            /* type 4: target area number   */
   int quest_cartography_room_count;          /* type 4: rooms to explore     */
   int quest_cartography_explored_count;      /* type 4: rooms explored       */
   unsigned char quest_cartography_bits[QUEST_CARTOGRAPHY_BYTES]; /* type 4 explored room bitmap */
};

struct pc_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   PC_DATA *next;
   PC_DATA *prev;
   int color[MAX_COLOR];
   char *pwd;
   char *bamfin;
   char *room_enter;
   char *room_exit;
   char *bamfout;
   char *title;
   char *staffskll;
   char *host;      /* Used to tell PC last login site */
   sh_int failures; /* Failed logins */
   sh_int clan;     /* will be used to denote clan membership */
   int generation;  /* Used for vamps */
   sh_int perm_str;
   sh_int perm_int;
   sh_int perm_wis;
   sh_int perm_dex;
   sh_int perm_con;
   sh_int max_str;
   sh_int max_int;
   sh_int max_wis;
   sh_int max_dex;
   sh_int max_con;
   sh_int mod_str;
   sh_int mod_int;
   sh_int mod_wis;
   sh_int mod_dex;
   sh_int mod_con;

   sh_int pagelen;
   unsigned int learned[MAX_SKILL];
   char *header;  /* header used for message */
   char *message; /* message for board in progress */
   char *alias_name[MAX_ALIASES];
   char *alias[MAX_ALIASES];
   char *who_name; /* To show on who name */
   int pkills;
   int pkilled;
   int mkills;
   int mkilled;
   int pflags;
   char *lastlogin;
   int monitor; /* monitor channel for staff */
   sh_int has_exp_fix;
   sh_int quest_points;

   char *ignore_list[MAX_IGNORES]; /* Ignore this person */
   int recall_vnum;
   int keep_vnum;
   int keep_healer_bought;
   int keep_healer_vnum;
   int reincarnations[MAX_TOTAL_CLASS]; /* Unified reincarnations (mortal 0-5, remort 6-17, adept
                                           18-23) */
   int reincarnation_data[MAX_REINCARNATE];
   int reincarnate_race;
   int reincarnate_class;
   int reincarnate_confirm;
   int mana_from_gain; /* saves non-item oriented mana total */
   int hp_from_gain;   /* same for hitpoints */
   int move_from_gain;
   char *load_msg;
   char hicol;
   char dimcol;
   sh_int ruler_rank;
   char *pedit_state;
   char *pedit_string[5];
   sh_int term_rows;
   sh_int term_columns;
   char *email_address;
   bool valid_email;
   int invasion_points;
   int invasion_rewards[3];
   int post_quest_points;
   int quest_dynamic_cooldown_until; /* earliest time for new dynamic prop */
   bool superboss_kills[MAX_SUPERBOSS];
   /* --- Quest system --- */
   QUEST_DATA quests[QUEST_MAX_QUESTS];
   bool completed_quests[QUEST_MAX_TEMPLATES];
   /* --- Weapon bond system --- */
   BOND_DATA *bond;
   /* --- Public society system --- */
   int pub_society;      /* PUB_SOCIETY_* constant */
   int pub_society_rank; /* rank score, -500 to 500 */
   int pub_society_tasks_done;
   int pub_society_joined; /* timestamp of enrollment */
   int pub_society_left;   /* timestamp of last departure (for cooldown) */
   bool is_new_player;     /* TRUE for brand-new characters; cleared after first greeting */
};

/*
 * program block
 */

struct mob_prog_act_list
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   MPROG_ACT_LIST *next;
   MPROG_ACT_LIST *prev;
   char *buf;
   CHAR_DATA *ch;
   OBJ_DATA *obj;
   void *vo;
};

struct mob_prog_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   MPROG_DATA *next;
   MPROG_DATA *prev;
   int type;
   char *arglist;
   char *comlist;
   char *filename;
};

/*
 * Extra description data for a room or object.
 */
struct extra_descr_data
{
   bool is_free;           /* Ramias:for run-time checks of LINK/UNLINK */
   EXTRA_DESCR_DATA *next; /* Next in list                     */
   EXTRA_DESCR_DATA *prev; /* Prev in list             */
   char *keyword;          /* Keyword in look/examine          */
   char *description;      /* What to see                      */
};

struct trigger_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   TRIGGER_DATA *next;
   TRIGGER_DATA *prev;
   char *message;      /* properly formatted act format string to use in a TO_ROOM */
   int trigger;        /* command used on object  */
   int event;          /* trigger function index to be called  */
   int data;           /* data used in the event call..vnum, spell index, etc. */
   int register_data;  /* for storing generic info  */
   int on_value;       /* for conditional triggers..happens when register is higher than
                        * on_value  */
   int at_vnum;        /* for at another room triggers. event will happen there */
   bool force_message; /* Always does an act message, in addition to trigger */
   char *spell_name;
};

struct obj_index_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   OBJ_INDEX_DATA *next;
   EXTRA_DESCR_DATA *first_exdesc;
   EXTRA_DESCR_DATA *last_exdesc;
   OBJ_FUN *obj_fun;
   AFFECT_DATA *first_apply;
   AFFECT_DATA *last_apply;
   /*
    * MAG Mod
    */
   AREA_DATA *area;
   char *owner;
   char *name;
   sh_int level;
   char *short_descr;
   char *description;
   unsigned short vnum;
   int item_type;
   unsigned long long extra_flags;
   int wear_flags;
   /*
    * class_flags changed to item_apply for magic apply, etc
    */
   int item_apply;
   sh_int count;
   sh_int weight;
   int cost; /* Unused */
   int value[10];
   TRIGGER_DATA *first_trigger;
   TRIGGER_DATA *last_trigger;
};

/*
 * One object.
 */
struct obj_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   OBJ_DATA *next;
   OBJ_DATA *prev;
   OBJ_DATA *next_in_carry_list; /* carry list is the list on a char, or in a container */
   OBJ_DATA *prev_in_carry_list;
   OBJ_DATA *first_in_carry_list;
   OBJ_DATA *last_in_carry_list;
   OBJ_DATA *next_in_room;
   OBJ_DATA *prev_in_room;
   OBJ_DATA *first_in_room;
   OBJ_DATA *last_in_room;
   OBJ_DATA *first_content;
   OBJ_DATA *last_content;
   OBJ_DATA *next_content;
   OBJ_DATA *prev_content;
   OBJ_DATA *in_obj;
   OBJ_FUN *obj_fun;
   CHAR_DATA *carried_by;
   EXTRA_DESCR_DATA *first_exdesc;
   EXTRA_DESCR_DATA *last_exdesc;
   AFFECT_DATA *first_apply;
   AFFECT_DATA *last_apply;
   OBJ_INDEX_DATA *pIndexData;
   ROOM_INDEX_DATA *in_room;
   char *owner;
   char *name;
   char *short_descr;
   char *description;
   int item_type;
   unsigned long long extra_flags;
   int wear_flags;
   int item_apply;
   int wear_loc;
   sh_int weight;
   int cost;
   sh_int level;
   sh_int timer;
   int value[10];
   sh_int condition; /* % value for condition */
};

/*
 * Exit data.
 */
struct exit_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   EXIT_DATA *next;
   ROOM_INDEX_DATA *to_room;
   unsigned short vnum;
   sh_int exit_info;
   sh_int key;
   char *keyword;
   char *description;
};

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct reset_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   RESET_DATA *next;
   RESET_DATA *prev;
   char command;
   sh_int ifflag;
   int arg1;
   int arg2;
   int arg3;
   char *notes;
   char *auto_message; /* Ugly - wasteful of space. */
};

/*
 * Area definition.
 */

/*
 *   Npc Interaction stuff  Zen
 */

struct queued_interact_list
{
   bool is_free;
   QUEUED_INTERACT_LIST *next;
   QUEUED_INTERACT_LIST *prev;
   CHAR_DATA *mob;
};

struct influence_list
{
   bool is_free;
   INFLUENCE_LIST *next;
   INFLUENCE_LIST *prev;
   INFLUENCE_DATA *this_one;
};

struct control_list
{
   bool is_free;
   CONTROL_LIST *next;
   CONTROL_LIST *prev;
   CONTROL_DATA *this_one;
};

struct ruler_list
{
   bool is_free;
   RULER_LIST *next;
   RULER_LIST *prev;
   RULER_DATA *this_one;
};

struct control_data
{
   bool is_free;
   char *keyword;
   AREA_DATA *area;
   RULER_DATA *ruler;               /* what entity controls the area */
   INFLUENCE_LIST *first_influence; /* what current influences are for the area */
   INFLUENCE_LIST *last_influence;
   INTERACT_DATA *first_interact; /* tells the mobs what to do */
   INTERACT_DATA *last_interact;
};

struct influence_data
{
   bool is_free;
   RULER_DATA *actor;
   int influence;
};

struct interact_data
{
   bool is_free;
   INTERACT_DATA *next;
   INTERACT_DATA *prev;
   int type;
   int min_value;
   int max_value;
   int action;
   char *say;
   CHAR_DATA *target;
};
#define RULER_NONE BIT_0
#define RULER_SOLO BIT_1
#define RULER_GROUP BIT_2
#define RULER_MALE BIT_3
#define RULER_FEMALE BIT_4
#define RULER_NEUTRAL BIT_5

struct ruler_data
{
   bool is_free;
   RULER_DATA *next;
   RULER_DATA *prev;
   char *name;
   sh_int ruler_rank;
   char *affiliation_name;
   sh_int affiliation_index;
   char *keywords;
   sh_int flags;
   CONTROL_LIST *first_control;
   CONTROL_LIST *last_control;
};

struct area_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   AREA_DATA *next;
   AREA_DATA *prev;
   RESET_DATA *first_reset;
   RESET_DATA *last_reset;
   char *name;
   sh_int age;
   sh_int nplayer;
   /* Saving to envy mod, to renumber vnums! */
   int offset;

   /* MAG mod */
   int modified;
   int min_vnum;
   int max_vnum;
   int area_num;
   char *owner;
   char *can_read;
   char *can_write;
   int gold;
   char *filename;
   char *music; /* bare mp3 filename served from /web/mp3/, or NULL for default theme */
   int flags;
   int aggro_list;
   BUILD_DATA_LIST *first_area_room;
   BUILD_DATA_LIST *last_area_room;
   BUILD_DATA_LIST *first_area_object;
   BUILD_DATA_LIST *last_area_object;
   BUILD_DATA_LIST *first_area_mobile;
   BUILD_DATA_LIST *last_area_mobile;
   BUILD_DATA_LIST *first_area_shop;
   BUILD_DATA_LIST *last_area_shop;
   BUILD_DATA_LIST *first_area_specfunc;
   BUILD_DATA_LIST *last_area_specfunc;
   BUILD_DATA_LIST *first_area_speechfun;
   BUILD_DATA_LIST *last_area_speechfun;
   BUILD_DATA_LIST *first_area_objfunc;
   BUILD_DATA_LIST *last_area_objfunc;
   /*
    * BUILD_DATA_LIST *   spec_funs; in mob index data.
    */
   /*
    * BUILD_DATA_LIST *   area_shops; in mob index data.
    */
   /*
    * BUILD_DATA_LIST *   resets; already there!
    */
   CONTROL_DATA *control;
   char *keyword;
   sh_int min_level;
   sh_int max_level;
   char *level_label;
   sh_int reset_rate;
   char *reset_msg;
};

/*
 * Room type.
 */
struct room_index_data
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   ROOM_INDEX_DATA *next;
   CHAR_DATA *first_person;
   CHAR_DATA *last_person;
   OBJ_DATA *first_content;
   OBJ_DATA *last_content;
   EXTRA_DESCR_DATA *first_exdesc;
   EXTRA_DESCR_DATA *last_exdesc;
   AREA_DATA *area;
   EXIT_DATA *exit[6];
   char *name;
   char *description;
   char *auto_message; /* If != NULL, send_to_room each tick */
   sh_int block_timer;
   unsigned short vnum;
   int room_flags;
   sh_int light;
   sh_int sector_type;
   BUILD_DATA_LIST *first_room_reset;
   BUILD_DATA_LIST *last_room_reset;
   /* -S- mod... don't save this with OLC :P */
   ROOM_AFFECT_DATA *first_room_affect;
   ROOM_AFFECT_DATA *last_room_affect;
   int affected_by;
   MARK_LIST_MEMBER *first_mark_list;
   MARK_LIST_MEMBER *last_mark_list;
   int gold;
};

/* Big MAG mod */
/* Area building stuff */

struct build_data_list /* Used for storing area file data. */
{
   bool is_free; /* Ramias:for run-time checks of LINK/UNLINK */
   BUILD_DATA_LIST *next;
   BUILD_DATA_LIST *prev;
   void *data;
};

struct lookup_type
{
   char *text;
   unsigned long long int value;
   int cost; /* if == NO_USE, only creators can set. */
};

struct lookup_extended_type
{
   char *text;
   long int value;
   long int value2;
   int cost;
   char *info;
};

/*
 * Skills include spells as a particular case.
 */
/*
 * Determine the tier of a skill (1=MORTAL, 2=REMORT, 3=ADEPT) from its
 * skill_level array.  The tier is the range (0-5, 6-17, or 18-23) that
 * has at least one non-NO_USE entry.  Returns 0 if no valid entry found.
 */
int skill_get_tier(int sn);

struct skill_type
{
   sh_int flag2;                        /* normal and/or vamp?         */
   char *name;                          /* Name of skill               */
   sh_int skill_level[MAX_TOTAL_CLASS]; /* Level needed by class       */
   SPELL_FUN *spell_fun;                /* Spell pointer (for spells)  */
   sh_int target;                       /* Legal targets               */
   sh_int minimum_position;             /* Position for caster / user   */
   sh_int *pgsn;                        /* Pointer to associated gsn   */
   sh_int slot;                         /* Slot for #OBJECT loading    */
   sh_int min_mana;                     /* Minimum mana used           */
   sh_int beats;                        /* Waiting time after use      */
   bool can_learn;
   char *noun_damage; /* Damage message              */
   char *msg_off;     /* Wear off message            */
   char *room_off;    /* Wear off msg TO_ROOM        */
   sh_int growth;     /* Overgrowth growth per cast (Druid spells) */
};

/*
 * Structure for a command in the command lookup table.
 */
struct cmd_type
{
   char *const name;
   DO_FUN *do_fun;
   sh_int position;
   sh_int level;
   sh_int log;
   sh_int type;  /* added by Aeria for do_commands */
   sh_int show;  /* added by Aeria for do_commands */
   sh_int flags; /* CMD_FLAG_* bitmask */
};

/*
 * Structure for a social in the socials table.
 */
struct social_type
{
   char *name;
   char *char_no_arg;
   char *others_no_arg;
   char *char_found;
   char *others_found;
   char *vict_found;
   char *char_auto;
   char *others_auto;
};

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD CHAR_DATA
#define MID MOB_INDEX_DATA
#define OD OBJ_DATA
#define OID OBJ_INDEX_DATA
#define RID ROOM_INDEX_DATA
#define SF SPEC_FUN
#define OF OBJ_FUN

/* act_comm.c */
bool is_note_to args((CHAR_DATA * ch, NOTE_DATA *pnote));
void add_follower args((CHAR_DATA * ch, CHAR_DATA *master));
void stop_follower args((CHAR_DATA * ch));
void die_follower args((CHAR_DATA * ch));
bool is_same_group args((CHAR_DATA * ach, CHAR_DATA *bch));
bool is_group_leader args((CHAR_DATA * ch));
void send_to_room args((char *message, ROOM_INDEX_DATA *room));
void list_who_to_output args((void));
const char *get_gsgp_json args((void));
const char *get_wholist_html args((void));

/* fight.c */
bool shortfight_should_suppress_watched_autoattack args((int observer_is_npc,
                                                         int observer_has_shortfight,
                                                         int observer_is_fighting));

/* act_info.c */
void set_title args((CHAR_DATA * ch, char *title));
char *color_string args((CHAR_DATA * CH, char *argument));
void do_learned args((CHAR_DATA * ch, char *argument));
void do_stancehelp args((CHAR_DATA * ch, char *argument));
void do_rmodhelp args((CHAR_DATA * ch, char *argument));

/* act_mob.c */
void int_combat_handler args((CHAR_DATA * ch, CHAR_DATA *victim));
void int_handler args((CHAR_DATA * ch));

/* act_move.c */
void move_char args((CHAR_DATA * ch, int door));

/* act_obj.c */

void get_obj args((CHAR_DATA * ch, OBJ_DATA *obj, OBJ_DATA *container));
bool can_wear_at(CHAR_DATA *ch, OBJ_DATA *obj, int location);

/* act_wiz.c */
ROOM_INDEX_DATA *find_location args((CHAR_DATA * ch, char *arg));

/* board.c */
BOARD_DATA *load_board(OBJ_INDEX_DATA *pObj);
void do_show_contents args((CHAR_DATA * ch, OBJ_DATA *obj));
void do_show_message args((CHAR_DATA * ch, int mess_num, OBJ_DATA *obj));
void do_edit_message args((CHAR_DATA * ch, int mess_num, OBJ_DATA *obj));
void do_add_to_message args((CHAR_DATA * ch, char *argument));
void do_start_a_message args((CHAR_DATA * ch, char *argument));
void save_message_data args((void));
void load_messages args((void));

/* comm.c */
void close_socket args((DESCRIPTOR_DATA * dclose));
void show_menu_to args((DESCRIPTOR_DATA * d));  /* Main */
void show_rmenu_to args((DESCRIPTOR_DATA * d)); /* Race */
void show_smenu_to args((DESCRIPTOR_DATA * d)); /* Sex */
void show_cmenu_to args((DESCRIPTOR_DATA * d)); /* Class */
void write_to_buffer args((DESCRIPTOR_DATA * d, const char *txt, int length));
void send_to_char args((const char *txt, CHAR_DATA *ch));
void show_string args((DESCRIPTOR_DATA * d, char *input));
void act args((const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type));
void hang args((const char *str));

/* Damage.c */
int calculate_damage args((CHAR_DATA * ch, CHAR_DATA *victim, int dam, int dt, int element,
                           bool crit_possible));
int do_damage args((CHAR_DATA * ch, CHAR_DATA *victim, int dam, int dt, int element,
                    bool critical));

/*
 * db.c
 */
void perm_update args((void));
void boot_db args((void));
void area_update args((void));
void db_format_status args((char *dest, size_t dest_size, const char *prefix,
                            const char *file_name));
void db_set_area_name args((const char *file_name));
void message_update args((void));
CD *create_mobile args((MOB_INDEX_DATA * pMobIndex));
OD *create_object args((OBJ_INDEX_DATA * pObjIndex, int level));
void clear_char args((CHAR_DATA * ch));
void free_char args((CHAR_DATA * ch));
char *get_extra_descr args((const char *name, EXTRA_DESCR_DATA *ed));
MID *get_mob_index args((int vnum));
OID *get_obj_index args((int vnum));
RID *get_room_index args((int vnum));
char fread_letter args((FILE * fp));
long fread_number args((FILE * fp));
unsigned long long fread_number_ull args((FILE * fp));
char *fread_string args((FILE * fp));
void fread_to_eol args((FILE * fp));
char *fsave_to_eol args((FILE * fp));
char *fread_word args((FILE * fp));

/* void *  alloc_mem       args( ( int sMem ) );
void    check_freed     args( ( unsigned int first, unsigned int last) );
void    check_free_mem  args( ( void ) );
void *  alloc_perm      args( ( int sMem ) );
void    free_mem        args( ( void *pMem, int sMem ) );*/

/* spec: renamed getmem -> _getmem, nuked unused alloc_perm */
/* void *  alloc_perm      args( ( int sMem ) ); */
void *_getmem args((int size, const char *caller, int log));
void dispose args((void *mem, int size));
char *str_dup args((const char *str));
void free_string args((char *pstr));
int number_fuzzy args((int number));
int number_range args((int from, int to));
int number_percent args((void));
int number_door args((void));
int number_bits args((int width));
int number_mm args((void));
int dice args((int number, int size));
int interpolate args((int level, int value_00, int value_32));
void append_file args((CHAR_DATA * ch, char *file, char *str));
void bug args((const char *str, int param));
void log_string args((const char *str));
void tail_chain args((void));
void safe_strcat args((int max_len, char *dest, char *source));
void send_to_descrips args((const char *message));
void bug_string args((const char *str, const char *str2));
/* Added stuff -Flar */
void bugf(char *fmt, ...) __attribute__((format(printf, 1, 2)));
void log_f(char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* fight.c */
void violence_update args((void));
void multi_hit args((CHAR_DATA * ch, CHAR_DATA *victim, int dt));
void one_hit args((CHAR_DATA * ch, CHAR_DATA *victim, int dt));
int damage args((CHAR_DATA * ch, CHAR_DATA *victim, int dam, int dt));
int swing args((CHAR_DATA * ch, CHAR_DATA *victim, int dam, int dt));
void update_pos args((CHAR_DATA * victim));
void stop_fighting args((CHAR_DATA * ch, bool fBoth));
void death_cry args((CHAR_DATA * ch));
void raw_kill args((CHAR_DATA * victim, char *argument));
void check_killer args((CHAR_DATA * ch, CHAR_DATA *victim));

/* ai.c */
void ai_update args((void));

/* revenant.c */
bool is_revenant args((CHAR_DATA * ch));
CHAR_DATA *find_revenant args((CHAR_DATA * ch));
CHAR_DATA *revenant_create args((CHAR_DATA * caster, OBJ_DATA *corpse));
void revenant_apply_tracks args((CHAR_DATA * rev));
void revenant_aura_tick args((CHAR_DATA * rev));
int revenant_spite_retaliate args((CHAR_DATA * rev, CHAR_DATA *attacker, int damage_taken));

/* handler.c */
bool remove_obj args((CHAR_DATA * ch, int iWear, bool fReplace));
int get_trust args((CHAR_DATA * ch));
void my_get_age args((CHAR_DATA * ch, char *buf));
int my_get_hours args((CHAR_DATA * ch));
int get_age args((CHAR_DATA * ch));
long get_cost_to_level args((CHAR_DATA * ch, int class));
long get_cost_to_level_remort args((CHAR_DATA * ch, int class));
bool can_wield args((CHAR_DATA * ch, OBJ_DATA *obj, int loc));
int get_curr_str args((CHAR_DATA * ch));
int get_max_str args((CHAR_DATA * ch));
int get_max_stat args((CHAR_DATA * ch, int apply_type));
int get_curr_int args((CHAR_DATA * ch));
int get_max_int args((CHAR_DATA * ch));
int get_curr_wis args((CHAR_DATA * ch));
int get_max_wis args((CHAR_DATA * ch));
int get_curr_dex args((CHAR_DATA * ch));
int get_max_dex args((CHAR_DATA * ch));
int get_curr_con args((CHAR_DATA * ch));
int get_max_con args((CHAR_DATA * ch));
int can_carry_n args((CHAR_DATA * ch));
int can_carry_w args((CHAR_DATA * ch));
int get_parry args((CHAR_DATA * ch));
int get_dodge args((CHAR_DATA * ch));
int get_block args((CHAR_DATA * ch));
int get_ac args((CHAR_DATA * ch));
int get_max_carry args((CHAR_DATA * ch));
int get_max_carry_weight args((CHAR_DATA * ch));
int get_speed args((CHAR_DATA * ch));
int get_spellpower args((CHAR_DATA * ch));
int get_spell_crit args((CHAR_DATA * ch));
int get_spell_crit_mult args((CHAR_DATA * ch));
int get_crit args((CHAR_DATA * ch));
int get_crit_mult args((CHAR_DATA * ch));
int get_counter args((CHAR_DATA * ch));
int get_damcap args((CHAR_DATA * ch));
int get_evasion_piercing args((CHAR_DATA * ch));
void short_fight_round_begin args((CHAR_DATA * ch, CHAR_DATA *victim));
int short_fight_round_end args((CHAR_DATA * ch, CHAR_DATA *victim, int *reactive_damage));
bool short_fight_round_active args((CHAR_DATA * ch, CHAR_DATA *victim));
void shortfight_emit_autoattack_summary args((CHAR_DATA * ch, CHAR_DATA *victim));

int get_racial_penalty_to_level args((int base, int race, int class));
int get_adept_level args((CHAR_DATA * ch));
bool is_adept args((CHAR_DATA * ch));
int get_hitroll args((CHAR_DATA * ch));
int get_damroll args((CHAR_DATA * ch));
int get_stat args((CHAR_DATA * ch, int stat));
bool raise_skill args((CHAR_DATA * ch, int gsn));
bool skill_success args((CHAR_DATA * ch, CHAR_DATA *victim, int gsn, int bonus));
char *get_dt_name args((int sn));
int get_generation_chance args(());
char *stat_to_string args((int stat));
char *class_order args((int race));
bool is_same_room args((CHAR_DATA * ch, CHAR_DATA *victim));
bool can_use_skill args((CHAR_DATA * ch, int gsn));
bool can_use_skill_message args((CHAR_DATA * ch, int gsn));
bool can_use_skill_by_name args((CHAR_DATA * ch, char *skill));
bool can_use_skill_by_name_message args((CHAR_DATA * ch, char *skill));
void affect_to_room args((ROOM_INDEX_DATA * room, ROOM_AFFECT_DATA *raf));
void r_affect_remove args((ROOM_INDEX_DATA * room, ROOM_AFFECT_DATA *raf));
char *get_item_class args((OBJ_DATA * obj));
void affect_to_char args((CHAR_DATA * ch, AFFECT_DATA *paf));
void affect_remove args((CHAR_DATA * ch, AFFECT_DATA *paf));
void affect_strip args((CHAR_DATA * ch, int sn));
bool is_affected args((CHAR_DATA * ch, int sn));
void affect_join args((CHAR_DATA * ch, AFFECT_DATA *paf));
void char_from_room args((CHAR_DATA * ch));
void char_to_room args((CHAR_DATA * ch, ROOM_INDEX_DATA *pRoomIndex));
void obj_to_char args((OBJ_DATA * obj, CHAR_DATA *ch));
void obj_from_char args((OBJ_DATA * obj));
int apply_ac args((OBJ_DATA * obj, int iWear));
OD *get_eq_char args((CHAR_DATA * ch, int iWear));
void equip_char args((CHAR_DATA * ch, OBJ_DATA *obj, int iWear));
void unequip_char args((CHAR_DATA * ch, OBJ_DATA *obj));
int count_obj_list args((OBJ_INDEX_DATA * obj, OBJ_DATA *list));
int count_obj_room args((OBJ_INDEX_DATA * obj, OBJ_DATA *list));
void obj_from_room args((OBJ_DATA * obj));
void obj_to_room args((OBJ_DATA * obj, ROOM_INDEX_DATA *pRoomIndex));
void obj_to_obj args((OBJ_DATA * obj, OBJ_DATA *obj_to));
void obj_from_obj args((OBJ_DATA * obj));
void extract_obj args((OBJ_DATA * obj));
void extract_char args((CHAR_DATA * ch, bool fPull));
CD *get_char_room args((CHAR_DATA * ch, char *argument));
CD *get_char_world args((CHAR_DATA * ch, char *argument));
CD *get_char_area args((CHAR_DATA * ch, char *argument));
OD *get_obj_type args((OBJ_INDEX_DATA * pObjIndexData));
OD *get_obj_list args((CHAR_DATA * ch, char *argument, OBJ_DATA *list));
OD *get_obj_room args((CHAR_DATA * ch, char *argument, OBJ_DATA *list));

OD *get_obj_carry args((CHAR_DATA * ch, char *argument));
OD *get_obj_wear args((CHAR_DATA * ch, char *argument));
OD *get_obj_here args((CHAR_DATA * ch, char *argument));
OD *get_obj_world args((CHAR_DATA * ch, char *argument));
OD *create_money args((int amount));
int get_obj_number args((OBJ_DATA * obj));
int get_obj_weight args((OBJ_DATA * obj));
int get_wear_weight args((CHAR_DATA * ch));
bool room_is_dark args((ROOM_INDEX_DATA * pRoomIndex));
bool room_is_private args((ROOM_INDEX_DATA * pRoomIndex));
bool can_see args((CHAR_DATA * ch, CHAR_DATA *victim));
bool can_see_obj args((CHAR_DATA * ch, OBJ_DATA *obj));
bool can_drop_obj args((CHAR_DATA * ch, OBJ_DATA *obj));
bool can_sac_obj args((CHAR_DATA * ch, OBJ_DATA *obj));
bool can_use args((CHAR_DATA * ch, OBJ_DATA *obj));
char *who_can_use args((OBJ_DATA * obj));
void notify args((char *message, int lv));
void info args((char *message, int lv));
void auction args((char *message));
void log_chan args((const char *message, int lv));
bool item_has_apply args((CHAR_DATA * ch, int bit));
CD *switch_char args((CHAR_DATA * victim, int mvnum, int poly_level));
CD *unswitch_char args((CHAR_DATA * ch));
int best_class args((CHAR_DATA * ch, int sn));
int best_level args((CHAR_DATA * ch, int sn));
void monitor_chan args((const char *message, int channel));
void set_stun args((CHAR_DATA * victim, int stunTime));
CD *get_char args((CHAR_DATA * ch));
void mark_from_room args((int this_room_vnum, MARK_DATA *mark));
void mark_to_room args((int this_room_vnum, MARK_DATA *mark));
void char_reference args((struct char_ref_type * ref));
void char_unreference args((CHAR_DATA * *var));
void obj_reference args((struct obj_ref_type * ref));
void obj_unreference args((OBJ_DATA * *var));

/* interp.c */
void interpret args((CHAR_DATA * ch, char *argument));
bool is_number args((char *arg));
bool IS_SWITCHED args((CHAR_DATA * ch));
bool is_shielded args((CHAR_DATA * ch, sh_int shield_type));
void remove_shield args((CHAR_DATA * ch, MAGIC_SHIELD *shield));
bool authorized args((CHAR_DATA * ch, char *skllnm));
bool check_social args((CHAR_DATA * ch, char *command, char *argument));

/* macros.c */
sh_int get_remort_level args((CHAR_DATA * ch));
sh_int get_psuedo_level args((CHAR_DATA * ch));
bool ok_to_use args((CHAR_DATA * ch, int value));
bool check_level_use args((CHAR_DATA * ch, int level));
long exp_to_level args((CHAR_DATA * ch, int index));
long exp_to_level_remort args((CHAR_DATA * ch, int index));
long exp_to_level_adept args((CHAR_DATA * ch));
int exp_for_mobile args((int level, CHAR_DATA *mob));
int skill_table_lookup args((CHAR_DATA * ch, int sn, int return_type));
bool is_remort args((CHAR_DATA * ch));
int exp_to_level_vamp args((int level));
long exp_to_level_adept args((CHAR_DATA * ch));
void reset_gain_stats args((CHAR_DATA * ch));
int get_item_value args((OBJ_DATA * obj));

/* magic.c */
int mana_cost args((CHAR_DATA * ch, int sn));
int skill_lookup args((const char *name));
int slot_lookup args((int slot));
bool is_arcane_spell args((int sn));
bool saves_spell args((int level, CHAR_DATA *victim));
void obj_cast_spell args((int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj));
bool spell_identify(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj);

/*--------------*\
) quests/crusade (
\*--------------*/

void crusade_inform args((void));
void crusade_complete args((CHAR_DATA * ch));
void crusade_cancel args((void));
void generate_auto_crusade args((void));
void crusade_ai_poll args((void));
void crusade_dialogue_dispatch args((CHAR_DATA * npc, CHAR_DATA *ch, const char *argument));

/*------*\
) save.c (
\*------*/
void save_char_obj args((CHAR_DATA * ch));
bool load_char_obj args((DESCRIPTOR_DATA * d, char *name, bool system_call));
void save_corpses args((void));
void load_corpses args((void));
void save_chest args((OBJ_DATA * chest));
void delete_chest_file args((int vnum));
void load_chest args((int vnum));
void fread_corpse args((FILE * fp));
void save_marks args((void));
void save_bans args((void));
void load_notes args((void));
char *initial args((const char *str));

/*---------*\
) special.c (
\*---------*/
SF *spec_lookup args((const char *name));
char *rev_spec_lookup args((void *func));
void print_spec_lookup args((char *buf));

/*---------*\
) obj_fun.c (
\*---------*/
OF *obj_fun_lookup args((const char *name));
char *rev_obj_fun_lookup args((void *func));
void print_obj_fun_lookup args((char *buf));

/*---------*\
) trigger.c (
\*---------*/

void trigger_handler args((CHAR_DATA * ch, OBJ_DATA *obj, int trigger));

/*--------*\
) update.c# (
\*--------*/
void advance_level args((CHAR_DATA * ch, int class, bool show));
void advance_level_remort args((CHAR_DATA * ch, int class, bool show));
void advance_level_adept args((CHAR_DATA * ch, int class, bool show));
void gain_exp args((CHAR_DATA * ch, int gain));

void update_handler args((void));
void caravan_update args((void));
bool check_rewield args((CHAR_DATA * ch));
bool check_re_equip args((CHAR_DATA * ch));
void auction_update args((void));

/* write.c */
void write_start args((char **dest, void *retfunc, void *retparm, CHAR_DATA *ch));
void write_interpret args((CHAR_DATA * ch, char *argument));

/* build.c */
void build_strdup(char **dest, char *src, bool freesrc, CHAR_DATA *ch);
char *build_simpstrdup(char *buf); /* A plug in alternative to str_dup */
extern const char *cDirs;
int get_dir(char);
char *show_values(const struct lookup_type *table, unsigned long long int value, bool fBit);

/* Tables.c */
const char *lookup_race_bit_value(int bit);

/* buildtab.c  */
/*
int table_lookup	args( (const struct lookup_type * table,char * name) );
char * rev_table_lookup	args( (const struct lookup_type * table,int number) );
char * bit_table_lookup	args( (const struct lookup_type * table,int number) );
*/

/* buildare.c */
/* Area manipulation funcs in buildare.c */
int build_canread(AREA_DATA *Area, CHAR_DATA *ch, int showerror);
int build_canwrite(AREA_DATA *Area, CHAR_DATA *ch, int showerror);
#define AREA_NOERROR 0
#define AREA_SHOWERROR 1

/* areasave.c */
void area_modified(AREA_DATA *);
void build_save_flush(void);

/* hunt.c */
void hunt_victim args((CHAR_DATA * ch));
void unhunt args((CHAR_DATA * ch));
int make_hunt args((CHAR_DATA * ch, CHAR_DATA *victim));
/* void	hunt_move	args( ( CHAR_DATA *ch ) ); */
void hunt_obj args((CHAR_DATA * ch));
bool make_move args((CHAR_DATA * ch, int vnum));
char *find_path args((int, int, CHAR_DATA *, int, int, int));

bool mob_hunt args((CHAR_DATA * mob));
void char_hunt args((CHAR_DATA * ch));
bool set_hunt args((CHAR_DATA * ch, CHAR_DATA *fch, CHAR_DATA *vch, OBJ_DATA *vobj, int set_flags,
                    int rem_flags));
void end_hunt args((CHAR_DATA * ch));
sh_int h_find_dir args((ROOM_INDEX_DATA * room, ROOM_INDEX_DATA *target, int h_flags));

/*
 * update.c
 */

void init_alarm_handler args((void));
void alarm_update args((void));
/*    SSM   */
void temp_fread_string args((FILE * fp, char *buf));

/*
 *      Rulers
 *
 */

void save_rulers args((void));
char *get_ruler_title args((int ruler_rank, int sex));

/*
 *	spendqp
 */
void save_brands args((void));

/*
 *  sysdat.c
 */
void save_sysdata args((void));

/*
 * Inline function definitions for character/object checks, name helpers,
 * and weather/skill/class-tier queries.  Must be included after all struct
 * definitions so function bodies can safely dereference struct fields.
 */
#ifndef DEC_INLINES_H
#include "inlines.h"
#endif

/*
 * Inline functions that reference global tables (skill_table, weather_info,
 * gclass_table).  Only included when globals.h was actually loaded — unit
 * tests that suppress globals.h and stub those tables themselves will not
 * see conflicting extern declarations.
 */
#ifdef DEC_GLOBALS_H_LOADED
#ifndef DEC_INLINES_GLOBALS_H
#include "inlines_globals.h"
#endif
#endif

#undef CD
#undef MID
#undef OD
#undef OID
#undef RID
#undef SF
#undef OF
