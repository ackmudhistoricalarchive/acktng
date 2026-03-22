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
 *    _/_/_/_/      _/          _/  _/             Version #: 4.3.1        *
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
#include <assert.h>
#include <stdarg.h>
#include "globals.h"
#include "hash.h"
#include "ssm.h"
#include <unistd.h> /* for execl */
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>

#ifndef DEC_ACT_MOB_H
#include "act_mob.h"
#endif

#ifdef HAVE_LIBPQ
#include "db/db_conn.h"
#include "db/db_load.h"
#include "db/db_worker.h"
#endif

#if !defined(macintosh)
extern int _filbuf args((FILE *));
#endif

void set_obj_stat_auto(OBJ_DATA *obj);
void set_aff_to_obj(OBJ_DATA *obj, int location, int modifier);
int object_spawn_level(int prototype_level, int requested_level);

/* Getstats */
int get_spellpower(CHAR_DATA *ch);
int get_spell_crit(CHAR_DATA *ch);
int get_spell_crit_mult(CHAR_DATA *ch);
int get_crit(CHAR_DATA *ch);
int get_crit_mult(CHAR_DATA *ch);

/*
 * Globals.
 */

/* -S- Mod: Some Globals for auctioning an item */
OBJ_DATA *auction_item;    /* Item being sold      */
CHAR_DATA *auction_owner;  /* Item's owner         */
CHAR_DATA *auction_bidder; /* Last bidder for item    */
int auction_bid;           /* Latest price offered    */
int auction_reserve;       /* Reserve Price     */
int auction_stage;         /* start, 1st, 2nd, gone   */
bool auction_flop;         /* Update called externally?    */

/* -S- Mod: Globals to handle questing */
bool quest;               /* Is there a quest running?    */
bool auto_quest;          /* Quests start on their own?   */
CHAR_DATA *quest_mob;     /* Mob which started quest      */
CHAR_DATA *quest_target;  /* Target of the quest     */
OBJ_DATA *quest_object;   /* Object to recover    */
int quest_timer;          /* Time left to get object */
int quest_wait = 0;       /* Min time until next quest  */
sh_int quest_personality; /* mob's crusade personality :) */
int quest_level_min;
int quest_level_max;
bool quest_ai_pending = FALSE;   /* AI generation in flight?        */
char quest_ai_tmpfile[256] = ""; /* Temp file path for AI output    */
pid_t quest_ai_pid = -1;         /* Child process PID               */
time_t quest_ai_start_time = 0;  /* Time child was launched         */

/* Zen mod: Diplomatics globals */
POL_DATA politics_data;

COUNCIL_DATA super_councils[MAX_SUPER]; /* for supernatural council meetings  */

char bug_buf[2 * MAX_INPUT_LENGTH];
char log_buf[2 * MAX_INPUT_LENGTH];
char testerbuf[MSL];
KILL_DATA kill_table[MAX_LEVEL];
TIME_INFO_DATA time_info;
WEATHER_DATA weather_info;
bool booting_up;
bool happy_hour;
int happy_hour_ticks_remaining;
bool area_resetting_global;
bool mem_log;
sh_int gsn_martial_arts;
sh_int gsn_dual_fist;
sh_int gsn_dualwield;
sh_int gsn_stun;
sh_int gsn_knee;
sh_int gsn_shadow;
sh_int gsn_climb;
sh_int gsn_find_doors;
sh_int gsn_bash;
sh_int gsn_smash;
sh_int gsn_notrip;
sh_int gsn_nodisarm;
sh_int gsn_trip;
sh_int gsn_dirt;
sh_int gsn_flee_timer;
sh_int gsn_fourth_attack;
sh_int gsn_shield_block;
sh_int gsn_beserk;
sh_int gsn_palmstrike;
sh_int gsn_counter;
sh_int gsn_enhanced_heal;
sh_int gsn_enhanced_purify;
sh_int gsn_enhanced_critical;
sh_int gsn_spell_critical;
sh_int gsn_spell_critical_damage;
sh_int gsn_enhanced_sword;
sh_int gsn_enhanced_sword_critical;
sh_int gsn_equip_buckler;
sh_int gsn_equip_fist;
sh_int gsn_equip_wand;
sh_int gsn_bare_hand;
sh_int gsn_two_handed;
sh_int gsn_detox;
sh_int gsn_morale;
sh_int gsn_leadership;
sh_int gsn_circle;
sh_int gsn_backstab;
sh_int gsn_poison_quinine;
sh_int gsn_poison_arsenic;
sh_int gsn_poison_nightshade;
sh_int gsn_dodge;
sh_int gsn_riposte;
sh_int gsn_mindoverbody;
sh_int gsn_flurry;
sh_int gsn_phantomfist;
sh_int gsn_spinfist;
sh_int gsn_aurabolt;
sh_int gsn_pummel;
sh_int gsn_chakra;
sh_int gsn_cripple;
sh_int gsn_fleche;
sh_int gsn_shieldblock;
sh_int gsn_chiblock;
sh_int gsn_holystrike;
sh_int gsn_anti_magic_shell;
sh_int gsn_chi_surge;
sh_int gsn_breath_of_endurance;
sh_int gsn_fist_interior;
sh_int gsn_momentum_chain;
sh_int gsn_veterans_cadence;
sh_int gsn_overwhelming_assault;
sh_int gsn_elemental_attunement;
sh_int gsn_oathshield;
sh_int gsn_sanctified_strike;
sh_int gsn_shadow_reading;
sh_int gsn_hex_ward;
sh_int gsn_reflex_disruption;
sh_int gsn_hide;
sh_int gsn_weapon_bond;
sh_int gsn_hunt;
sh_int gsn_peek;
sh_int gsn_pick_lock;
sh_int gsn_sneak;
sh_int gsn_steal;

sh_int gsn_punch;
sh_int gsn_headbutt;

sh_int gsn_disarm;
sh_int gsn_enhanced_damage;
sh_int gsn_kick;
sh_int gsn_parry;
sh_int gsn_rescue;
sh_int gsn_second_attack;
sh_int gsn_third_attack;

sh_int gsn_blindness;
sh_int gsn_charm_person;
sh_int gsn_curse;
sh_int gsn_invis;
sh_int gsn_mass_invis;
sh_int gsn_poison;
sh_int gsn_sleep;
sh_int gsn_disguise;
sh_int gsn_adrenaline;
sh_int gsn_frenzy;
sh_int gsn_emotion_control;
sh_int gsn_fifth_attack;
sh_int gsn_sixth_attack;
sh_int gsn_target;
sh_int gsn_charge;
sh_int gsn_unit_tactics;
sh_int gsn_scout;
sh_int gsn_potency;
sh_int gsn_thaumatergy;
sh_int gsn_warcry;

/* Cipher skills */
sh_int gsn_garrote;
sh_int gsn_feign_death;
sh_int gsn_shadow_step;
sh_int gsn_mark_target;
sh_int gsn_conceal;
sh_int gsn_set_trap;
sh_int gsn_read_intent;

/* Warden skills */
sh_int gsn_cleave;
sh_int gsn_fortify;
sh_int gsn_taunt;
sh_int gsn_rend;
sh_int gsn_field_patch;
sh_int gsn_weapon_mastery;
sh_int gsn_ashfall_strike;

/* Pugilist skills */
sh_int gsn_grapple;
sh_int gsn_choke_hold;
sh_int gsn_leg_sweep;
sh_int gsn_iron_skin;
sh_int gsn_roll_with_blow;
sh_int gsn_pressure_point;
sh_int gsn_feint;
sh_int gsn_conditioning;
sh_int gsn_harbor_dust;

/* Sentinel skills */
sh_int gsn_vigilance;
sh_int gsn_verdict;
sh_int gsn_read_opponent;
sh_int gsn_binding_strike;
sh_int gsn_testimonial_guard;
sh_int gsn_ninth_descent;
sh_int gsn_measured_response;
sh_int gsn_condemn;
sh_int gsn_seal_testimony;

/* Druid skills */
sh_int gsn_substrate_piercing;

/* Public society skills */
sh_int gsn_patrol_sense;
sh_int gsn_field_mend;
sh_int gsn_rally_cry;
sh_int gsn_shield_wall;
sh_int gsn_tactical_assess;
sh_int gsn_command_presence;
sh_int gsn_dockside_audit;
sh_int gsn_haggle;
sh_int gsn_longshoreman;
sh_int gsn_bonded_cargo;
sh_int gsn_anchor_slam;
sh_int gsn_forage;
sh_int gsn_field_repair;
sh_int gsn_ember_signal;
sh_int gsn_ember_vitality;
sh_int gsn_containment_ward;
sh_int gsn_controlled_burn;
sh_int gsn_oath_mark;
sh_int gsn_track_quarry;
sh_int gsn_writ_of_hold;
sh_int gsn_marked_strike;
sh_int gsn_claws_pursuit;
sh_int gsn_claws_judgment;
sh_int gsn_dig_in;
sh_int gsn_shelter;
sh_int gsn_trailblaze;
sh_int gsn_bulwark;
sh_int gsn_signal_tower;
sh_int gsn_rampart_slam;
sh_int gsn_wayfind;
sh_int gsn_road_camp;
sh_int gsn_forced_march;
sh_int gsn_road_grit;
sh_int gsn_dispatch_rider;
sh_int gsn_waylay;

/* Level 1 active skills for mortal skill-based classes */
sh_int gsn_duty_strike;
sh_int gsn_gap_probe;
sh_int gsn_primary_motion;
sh_int gsn_vigil_strike;

#ifdef TFS
sh_int gsn_mana_sense;
#endif

/*
 * Locals.
 */
MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
char *string_hash[MAX_KEY_HASH];

AREA_DATA *area_used[MAX_AREAS];
AREA_DATA *area_load;
/*
 * replaced for SSM
 */

#if 0
char *string_space = NULL;
char *top_string = NULL;
char str_empty[1] = { 0 };
#endif

extern char str_empty[1];
extern long sOverFlowString;
extern long nOverFlowString;
extern bool Full;

int top_affect;
int top_area;
int top_ed;
int top_exit;
int top_help;
int top_mob_index;
int top_obj_index;
int top_reset;
int top_room;
int top_shop;

/*
 * program locals
 */

/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */

#if 0

#define MAX_STRING 2097152
/*#define                 MAX_PERM_BLOCK  131072
#define                 MAX_MEM_LIST    12

void *                  rgFreeList      [MAX_MEM_LIST];
const int               rgSizeList      [MAX_MEM_LIST]  =
{
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768-64, 65536-16
};*/



int nAllocString;
int sAllocString;
#endif

extern long nAllocString;
extern long sAllocString;

int nAllocPerm;
int sAllocPerm;

extern long MAX_STRING;
void init_string_space(void);
void boot_done(void);
bool generate_ai_spawn(CHAR_DATA *ch);

/*
 * Semi-locals.
 */
bool fBootDb;
FILE *fpArea;
char strArea[MAX_INPUT_LENGTH];
int area_revision = -1;

/*
 * Local booting procedures.
 */
void init_mm args((void));

void load_help_files args((void));
void load_brands args((void));

void fix_exits args((void));
void check_resets args((void));

void reset_area args((AREA_DATA * pArea));

#define SHOW_AREA                                                                                  \
   if (!previous_bug)                                                                              \
   {                                                                                               \
      bug("      In area file:", 0);                                                               \
      log_f("      In %s.", pArea->filename);                                                      \
      previous_bug = 1;                                                                            \
   }

void db_format_status(char *dest, size_t dest_size, const char *prefix, const char *file_name)
{
   snprintf(dest, dest_size, "%s %s", prefix, file_name);
}

void db_set_area_name(const char *file_name)
{
   snprintf(strArea, sizeof(strArea), "%.*s", (int)sizeof(strArea) - 1, file_name);
}

#ifdef UNIT_TEST_DB
void db_test_format_status(char *dest, size_t dest_size, const char *prefix, const char *file_name)
{
   db_format_status(dest, dest_size, prefix, file_name);
}

void db_test_set_area_name(const char *file_name)
{
   db_set_area_name(file_name);
}

const char *db_test_get_area_name(void)
{
   return strArea;
}
#endif

/*
 * Big mama top level function.
 */
void boot_db(void)
{
   int a;

   /*
    * Init some data space stuff.
    */

#if 0
   {
      if( ( string_space = calloc( 1, MAX_STRING ) ) == NULL )
      {
         bug( "Boot_db: can't alloc %d string space.", MAX_STRING );
         exit( 1 );
      }
      top_string = string_space;
      fBootDb = TRUE;
   }
#endif

   init_string_space();
   fBootDb = TRUE;

   send_to_descrips("Initialising Ack! Mud.  Please Wait....\n\r");

   /*
    * Init random number generator.
    */
   {
      init_mm();
   }

   /*
    * Set time and weather.
    */
   {
      long lhour, lday, lmonth;

      lhour = (current_time - 650336715) / (PULSE_TICK / PULSE_PER_SECOND);
      time_info.hour = lhour % 24;
      lday = lhour / 24;
      time_info.day = lday % 35;
      lmonth = lday / 35;
      time_info.month = lmonth % 17;
      time_info.year = lmonth / 17;
      time_info.moon = 0;
      weather_info.moon_loc = MOON_DOWN;
      weather_info.moon_phase = number_range(MOON_NEW, MOON_FULL);

      happy_hour = FALSE;
      happy_hour_ticks_remaining = 0;

      if (time_info.hour < 5)
         weather_info.sunlight = SUN_DARK;
      else if (time_info.hour < 6)
         weather_info.sunlight = SUN_RISE;
      else if (time_info.hour < 19)
         weather_info.sunlight = SUN_LIGHT;
      else if (time_info.hour < 20)
         weather_info.sunlight = SUN_SET;
      else
         weather_info.sunlight = SUN_DARK;

      weather_info.change = 0;
      weather_info.mmhg = 960;
      if (time_info.month >= 7 && time_info.month <= 12)
         weather_info.mmhg += number_range(1, 50);
      else
         weather_info.mmhg += number_range(1, 80);

      if (weather_info.mmhg <= 980)
         weather_info.sky = SKY_LIGHTNING;
      else if (weather_info.mmhg <= 1000)
         weather_info.sky = SKY_RAINING;
      else if (weather_info.mmhg <= 1020)
         weather_info.sky = SKY_CLOUDY;
      else
         weather_info.sky = SKY_CLOUDLESS;
   }

   /*
    * Initialize Supernatural Councils
    */

   {
      sh_int index;
      char buf[MAX_STRING_LENGTH];
      log_f("Initializing Supernatural Councils");
      for (index = SUPER_NONE; index < MAX_SUPER; index++)
      {
         super_councils[index].first_member = NULL;
         super_councils[index].last_member = NULL;
         super_councils[index].quorum = FALSE;
         super_councils[index].council_time = 0;
         switch (index)
         {
         case SUPER_NONE:
            sprintf(buf, "%s", "ORDINARIES");
            break;
         default:
            sprintf(buf, "%s", "NONE");
            break;
         }
         super_councils[index].council_name = str_dup(buf);
      }
   }

   /*
    * Assign gsn's for skills which have them.
    */
   {
      int sn;

      for (sn = 0; sn < MAX_SKILL; sn++)
      {
         if (skill_table[sn].pgsn != 0)
            *skill_table[sn].pgsn = sn;
      }
   }

   /* Link public society skill gsns after gsn resolution */
   pub_society_boot_init();

   /*
    * Clear list of used areas
    */
   for (a = 0; a < MAX_AREAS; a++)
   {
      area_used[a] = NULL;
   }

   /*
    * Open DB connection if data/db.conf exists.  use_db == 1 means all
    * subsequent loads come from PostgreSQL; use_db == 0 falls back to the
    * flat-file loaders below.
    */
#if defined(USE_DB_LOAD) && defined(HAVE_LIBPQ)
   int use_db = db_conn_open(".");
   if (use_db)
      log_f("DB: connection established, using database loaders.");
   else
      log_f("DB: no connection (db.conf absent or unreachable), using flat-file loaders.");
#endif

      /*
       *   Read in clan data table
       */

#if defined(USE_DB_LOAD) && defined(HAVE_LIBPQ)
   if (use_db)
   {
      log_f("DB: loading clan diplomacy from database.");
      db_load_clans();
   }
   else
#endif
   {
      FILE *clanfp;
      char clan_file_name[MAX_STRING_LENGTH];
      sh_int x, y;
      char buf[MAX_STRING_LENGTH];
      log_f("Loading in Clan diplomacy info.");

      snprintf(clan_file_name, sizeof(clan_file_name), "%s", CLAN_FILE);

      db_format_status(buf, sizeof(buf), "Loading", clan_file_name);
      monitor_chan(buf, MONITOR_CLAN);

      if ((clanfp = fopen(clan_file_name, "r")) == NULL)
      {
         log_f("failed open of clan_table.dat in load_clan_table");
      }
      else
      {
         fpArea = clanfp;
         db_set_area_name(clan_file_name);

         sh_int file_max_clan = fread_number(clanfp);
         if (file_max_clan != MAX_CLAN)
         {
            log_f("WARNING: clandata.dat has MAX_CLAN=%d but code expects %d. "
                  "Skipping load, using defaults.",
                  file_max_clan, MAX_CLAN);
         }
         else
         {
            for (x = 1; x < MAX_CLAN; x++)
            {
               for (y = 1; y < MAX_CLAN; y++)
               {
                  politics_data.diplomacy[x][y] = fread_number(clanfp);
               }
            }

            for (x = 1; x < MAX_CLAN; x++)
            {
               politics_data.treasury[x] = fread_number(clanfp);
            }

            for (x = 1; x < MAX_CLAN; x++)
            {
               for (y = 1; y < MAX_CLAN; y++)
               {
                  politics_data.end_current_state[x][y] = fread_number(clanfp);
               }
            }
         }

         if (clanfp != NULL)
         {
            fclose(clanfp);
            clanfp = NULL;
         }
      }
      fpArea = NULL;
      db_format_status(buf, sizeof(buf), "Done Loading", clan_file_name);
      log_f("%s", buf);
   }

   /*
    * Read in all the socials.
    */
#if defined(USE_DB_LOAD) && defined(HAVE_LIBPQ)
   if (use_db)
   {
      log_f("DB: loading socials from database.");
      db_load_socials();
   }
   else
#endif
   {
      load_social_table();
   }

   /*
    * Read in all the area files.
    */
#if defined(USE_DB_LOAD) && defined(HAVE_LIBPQ)
   if (use_db)
   {
      log_f("DB: loading areas from database.");
      db_load_areas_from_db();
   }
   else
#endif
   {
      FILE *fpList;
      log_f("Reading Area Files...");

      if ((fpList = fopen(AREA_LIST, "r")) == NULL)
      {
         perror(AREA_LIST);
         log_f("Unable to open area.lst, aborting bootup.");
         kill(getpid(), SIGQUIT);
      }

      for (;;)
      {
         char areaBuf[MAX_STRING_LENGTH];
         strcpy(strArea, fread_word(fpList));
         if (strArea[0] == '$')
            break;

         if (strArea[0] == '-')
         {
            fpArea = stdin;
         }
         else
         {
            strcpy(areaBuf, AREA_DIR);
            strcat(areaBuf, strArea);
            if ((fpArea = fopen(areaBuf, "r")) == NULL)
            {
               log_string(areaBuf);
               kill(getpid(), SIGQUIT);
            }

            log_f("%s successfully loaded.", areaBuf);
         }

         for (;;)
         {
            char *word;

            if (fread_letter(fpArea) != '#')
            {
               bug("Boot_db: # not found.", 0);
               kill(getpid(), SIGQUIT);
            }

            word = fread_word(fpArea);

            if (word[0] == '$')
               break;
            else if (!str_cmp(word, "AREA"))
               load_area(fpArea);
            else if (!str_cmp(word, "MOBILES"))
               load_mobiles(fpArea);
            else if (!str_cmp(word, "OBJECTS"))
               load_objects(fpArea);
            else if (!str_cmp(word, "RESETS"))
               load_resets(fpArea);
            else if (!str_cmp(word, "ROOMS"))
               load_rooms(fpArea);
            else if (!str_cmp(word, "SHOPS"))
               load_shops(fpArea);
            else if (!str_cmp(word, "SPECIALS"))
               load_specials(fpArea);
            else if (!str_cmp(word, "SPEECH"))
               load_speech(fpArea);
            else if (!str_cmp(word, "OBJFUNS"))
               load_objfuns(fpArea);
            else
            {
               bug("Boot_db: bad section name.", 0);
               exit(1);
            }
         }

         if (fpArea != stdin)
            fclose(fpArea);
         fpArea = NULL;
      }
      if (fpList != NULL)
      {
         fclose(fpList);
         fpList = NULL;
      }
   }

#if defined(USE_DB_LOAD) && defined(HAVE_LIBPQ)
   if (use_db)
   {
      log_f("DB: loading help files from database.");
      db_load_helps_from_db();
      log_f("DB: loading shelp files from database.");
      db_load_shelps_from_db();
      log_f("DB: loading lore from database.");
      db_load_lore_from_db();
   }
   else
#endif
      load_help_files();
   log_f("Loading quest templates.");
   quest_load_templates();

   /*
    * Fix up exits.
    * MAG Mod: Check resets. (Allows loading objects of later areas.)
    * MAG Mod: Load gold for areas.
    * Declare db booting over.
    * Reset all areas once.
    * Load up the notes file.
    */
   /* Set pub_society fields on society hall NPCs by vnum */
   {
      MOB_INDEX_DATA *pMob;
      int vnum;
      /* Guard Command recruiter (Midgaard) */
      vnum = 7800;
      if ((pMob = get_mob_index(vnum)) != NULL)
      {
         pMob->pub_society = PUB_SOCIETY_GUARD_COMMAND;
         pMob->pub_society_npc_role = PUB_SOC_NPC_RECRUITER;
      }
      /* Wall Command recruiter (Kiess) */
      vnum = 7900;
      if ((pMob = get_mob_index(vnum)) != NULL)
      {
         pMob->pub_society = PUB_SOCIETY_WALL_COMMAND;
         pMob->pub_society_npc_role = PUB_SOC_NPC_RECRUITER;
      }
      /* Harbor Syndics recruiter (Kowloon) */
      vnum = 8000;
      if ((pMob = get_mob_index(vnum)) != NULL)
      {
         pMob->pub_society = PUB_SOCIETY_HARBOR_SYNDICS;
         pMob->pub_society_npc_role = PUB_SOC_NPC_RECRUITER;
      }
      /* Shrine of the First Claw recruiter (Mafdet) */
      vnum = 8100;
      if ((pMob = get_mob_index(vnum)) != NULL)
      {
         pMob->pub_society = PUB_SOCIETY_FIRST_CLAW;
         pMob->pub_society_npc_role = PUB_SOC_NPC_RECRUITER;
      }
      /* Ember Wardens recruiter (Rakuen) */
      vnum = 8200;
      if ((pMob = get_mob_index(vnum)) != NULL)
      {
         pMob->pub_society = PUB_SOCIETY_EMBER_WARDENS;
         pMob->pub_society_npc_role = PUB_SOC_NPC_RECRUITER;
      }
      /* Road Wardens recruiter (Midgaard crossroads) */
      vnum = 8300;
      if ((pMob = get_mob_index(vnum)) != NULL)
      {
         pMob->pub_society = PUB_SOCIETY_ROAD_WARDENS;
         pMob->pub_society_npc_role = PUB_SOC_NPC_RECRUITER;
      }
   }

   {
      log_f("Fixing Exits......");
      fix_exits();
      log_f("Done.\n\r");
      fBootDb = FALSE;
      log_f("Checking Resets...");
      check_resets();
      log_f("Done.");
      log_f("Updating Areas....");
      area_update();
      log_f("Loading notes");
      load_notes();
#if defined(USE_DB_LOAD) && defined(HAVE_LIBPQ)
      if (use_db)
      {
         log_f("DB: loading corpses from database.");
         db_load_corpses();
         log_f("DB: loading room marks from database.");
         db_load_room_marks();
         log_f("DB: loading boards from database.");
         db_load_boards();
         log_f("DB: loading bans from database.");
         db_load_bans();
         log_f("DB: loading ruler data from database.");
         db_load_rulers();
         log_f("DB: loading staff brands from database.");
         db_load_brands();
         log_f("DB: loading system data from database.");
         db_load_sysdata();
         log_f("DB: loading keep chests from database.");
         db_load_chests();
      }
      else
#endif
      {
         log_f("Loading corpses.");
         load_corpses();
         booting_up = TRUE;
         log_f("Loading room marks.");
         load_marks();
         booting_up = FALSE;
         save_marks();
         log_f("Loading banned sites.");
         load_bans();
         log_f("Loading ruler data.");
         load_rulers();
         log_f("Loading staff brands.");
         load_brands();
         log_f("Loading System Data.");
         load_sysdata();
      }
   }
#if defined(USE_DB_LOAD) && defined(HAVE_LIBPQ)
   db_conn_close();
#endif
   auto_quest = TRUE;
   return;
}

static long parse_lore_flags(const char *flag_str)
{
   long flags = 0;
   char buf[MAX_STRING_LENGTH];
   const char *p = flag_str;

   while (*p)
   {
      while (*p == ' ' || *p == '\t')
         p++;
      if (*p == '\0' || *p == '\r' || *p == '\n')
         break;

      size_t i = 0;
      while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && i < sizeof(buf) - 1)
         buf[i++] = *p++;
      buf[i] = '\0';

      if (!str_cmp(buf, "MIDGAARD"))
         flags |= LORE_FLAG_MIDGAARD;
      else if (!str_cmp(buf, "KIESS"))
         flags |= LORE_FLAG_KIESS;
      else if (!str_cmp(buf, "KOWLOON"))
         flags |= LORE_FLAG_KOWLOON;
      else if (!str_cmp(buf, "RAKUEN"))
         flags |= LORE_FLAG_RAKUEN;
      else if (!str_cmp(buf, "MAFDET"))
         flags |= LORE_FLAG_MAFDET;
      else if (!str_cmp(buf, "HUMAN"))
         flags |= LORE_FLAG_HUMAN;
      else if (!str_cmp(buf, "KHENARI"))
         flags |= LORE_FLAG_KHENARI;
      else if (!str_cmp(buf, "KHEPHARI"))
         flags |= LORE_FLAG_KHEPHARI;
      else if (!str_cmp(buf, "ASHBORN"))
         flags |= LORE_FLAG_ASHBORN;
      else if (!str_cmp(buf, "UMBRAL"))
         flags |= LORE_FLAG_UMBRAL;
      else if (!str_cmp(buf, "RIVENNID"))
         flags |= LORE_FLAG_RIVENNID;
      else if (!str_cmp(buf, "DELTARI"))
         flags |= LORE_FLAG_DELTARI;
      else if (!str_cmp(buf, "USHABTI"))
         flags |= LORE_FLAG_USHABTI;
      else if (!str_cmp(buf, "SERATHI"))
         flags |= LORE_FLAG_SERATHI;
      else if (!str_cmp(buf, "KETHARI"))
         flags |= LORE_FLAG_KETHARI;
      else
      {
         long numeric = atol(buf);
         if (numeric != 0)
            flags |= numeric;
      }
   }
   return flags;
}

static void link_help_entry(long level, long flags, const char *keywords, const char *text,
                            HELP_DATA **first, HELP_DATA **last)
{
   HELP_DATA *pHelp;
   GET_FREE(pHelp, help_free);
   pHelp->level = level;
   pHelp->flags = flags;
   pHelp->keyword = str_dup(keywords);
   pHelp->text = str_dup(text);
   LINK(pHelp, *first, *last, next, prev);
   top_help++;
}

static void link_lore_entry(long flags, const char *keywords, const char *text, HELP_DATA **first,
                            HELP_DATA **last)
{
   link_help_entry(0, flags, keywords, text, first, last);
}

#ifdef UNIT_TEST_DB
long db_test_parse_lore_flags(const char *s)
{
   return parse_lore_flags(s);
}
#endif

static void load_help_file(const char *help_path, HELP_DATA **first, HELP_DATA **last)
{
   FILE *fp;
   char line[MAX_STRING_LENGTH];
   long level;
   long flags = 0;
   char keywords[MAX_STRING_LENGTH];
   char text[MAX_STRING_LENGTH * 8];
   size_t text_len = 0;

   fp = fopen(help_path, "r");
   if (fp == NULL)
   {
      log_f("Unable to open help file: %s", help_path);
      return;
   }

   if (fgets(line, sizeof(line), fp) == NULL || sscanf(line, "level %ld", &level) != 1)
   {
      bug("load_help_file: invalid level header", 0);
      fclose(fp);
      return;
   }

   if (fgets(line, sizeof(line), fp) == NULL || strncmp(line, "keywords ", 9) != 0)
   {
      bug("load_help_file: missing keywords header", 0);
      fclose(fp);
      return;
   }
   snprintf(keywords, sizeof(keywords), "%s", line + 9);
   keywords[strcspn(keywords, "\r\n")] = '\0';

   if (fgets(line, sizeof(line), fp) == NULL)
   {
      bug("load_help_file: unexpected end of file", 0);
      fclose(fp);
      return;
   }

   if (strncmp(line, "flags ", 6) == 0)
   {
      flags = parse_lore_flags(line + 6);
      if (fgets(line, sizeof(line), fp) == NULL)
      {
         bug("load_help_file: unexpected end of file after flags", 0);
         fclose(fp);
         return;
      }
   }

   if (strncmp(line, "---", 3) != 0)
   {
      bug("load_help_file: missing separator", 0);
      fclose(fp);
      return;
   }

   text[0] = '\0';
   text_len = 0;

   while (fgets(line, sizeof(line), fp) != NULL)
   {
      /* Check if this line is exactly "level <number>\n" (new entry header) */
      long next_level;
      char extra;
      if (strncmp(line, "level ", 6) == 0 && sscanf(line, "level %ld %c", &next_level, &extra) == 1)
      {
         /* Save current entry */
         link_help_entry(level, flags, keywords, text, first, last);

         level = next_level;

         flags = 0;

         if (fgets(line, sizeof(line), fp) == NULL || strncmp(line, "keywords ", 9) != 0)
         {
            bug("load_help_file: missing keywords in multi-entry", 0);
            fclose(fp);
            return;
         }
         snprintf(keywords, sizeof(keywords), "%s", line + 9);
         keywords[strcspn(keywords, "\r\n")] = '\0';

         if (fgets(line, sizeof(line), fp) == NULL)
         {
            bug("load_help_file: unexpected end of file in multi-entry", 0);
            fclose(fp);
            return;
         }

         if (strncmp(line, "flags ", 6) == 0)
         {
            flags = parse_lore_flags(line + 6);
            if (fgets(line, sizeof(line), fp) == NULL)
            {
               bug("load_help_file: unexpected end after flags in multi-entry", 0);
               fclose(fp);
               return;
            }
         }

         if (strncmp(line, "---", 3) != 0)
         {
            bug("load_help_file: missing separator in multi-entry", 0);
            fclose(fp);
            return;
         }

         text[0] = '\0';
         text_len = 0;
         continue;
      }

      size_t line_len = strlen(line);
      if (text_len + line_len + 1 >= sizeof(text))
      {
         bug("load_help_file: help text too long", 0);
         fclose(fp);
         return;
      }
      memcpy(text + text_len, line, line_len);
      text_len += line_len;
      text[text_len] = '\0';
   }

   /* Save final entry */
   link_help_entry(level, flags, keywords, text, first, last);

   fclose(fp);
}

static void load_lore_file(const char *lore_path, HELP_DATA **first, HELP_DATA **last)
{
   FILE *fp;
   char line[MAX_STRING_LENGTH];
   long flags = 0;
   char keywords[MAX_STRING_LENGTH];
   char text[MAX_STRING_LENGTH * 8];
   size_t text_len = 0;

   fp = fopen(lore_path, "r");
   if (fp == NULL)
   {
      log_f("Unable to open lore file: %s", lore_path);
      return;
   }

   /* First line must be keywords */
   if (fgets(line, sizeof(line), fp) == NULL || strncmp(line, "keywords ", 9) != 0)
   {
      bug("load_lore_file: missing keywords header", 0);
      fclose(fp);
      return;
   }
   snprintf(keywords, sizeof(keywords), "%s", line + 9);
   keywords[strcspn(keywords, "\r\n")] = '\0';

   if (fgets(line, sizeof(line), fp) == NULL)
   {
      bug("load_lore_file: unexpected end of file", 0);
      fclose(fp);
      return;
   }

   if (strncmp(line, "flags ", 6) == 0)
   {
      flags = parse_lore_flags(line + 6);
      if (fgets(line, sizeof(line), fp) == NULL)
      {
         bug("load_lore_file: unexpected end of file after flags", 0);
         fclose(fp);
         return;
      }
   }

   if (strncmp(line, "---", 3) != 0)
   {
      bug("load_lore_file: missing separator", 0);
      fclose(fp);
      return;
   }

   text[0] = '\0';
   text_len = 0;

   while (fgets(line, sizeof(line), fp) != NULL)
   {
      /* Check if this line starts a new entry (flags line) */
      if (strncmp(line, "flags ", 6) == 0)
      {
         /* Save current entry */
         link_lore_entry(flags, keywords, text, first, last);

         flags = parse_lore_flags(line + 6);

         if (fgets(line, sizeof(line), fp) == NULL)
         {
            bug("load_lore_file: unexpected end after flags in multi-entry", 0);
            fclose(fp);
            return;
         }

         if (strncmp(line, "---", 3) != 0)
         {
            bug("load_lore_file: missing separator in multi-entry", 0);
            fclose(fp);
            return;
         }

         text[0] = '\0';
         text_len = 0;
         continue;
      }

      size_t line_len = strlen(line);
      if (text_len + line_len + 1 >= sizeof(text))
      {
         bug("load_lore_file: lore text too long", 0);
         fclose(fp);
         return;
      }
      memcpy(text + text_len, line, line_len);
      text_len += line_len;
      text[text_len] = '\0';
   }

   /* Save final entry */
   link_lore_entry(flags, keywords, text, first, last);

   fclose(fp);
}

static int compare_help_file_names(const void *a, const void *b)
{
   const char *const *sa = (const char *const *)a;
   const char *const *sb = (const char *const *)b;
   return str_cmp(*sa, *sb);
}

static void load_help_directory(const char *directory, HELP_DATA **first, HELP_DATA **last)
{
   DIR *dir;
   struct dirent *entry;
   char path[MAX_STRING_LENGTH];
   char *file_names[2048];
   int file_count = 0;
   int i;

   dir = opendir(directory);
   if (dir == NULL)
   {
      log_f("Help directory not found: %s", directory);
      return;
   }

   while ((entry = readdir(dir)) != NULL)
   {
      size_t len;

      if (entry->d_name[0] == '.')
         continue;

      len = strlen(entry->d_name);
      if (len == 0 || entry->d_name[len - 1] == '~')
         continue;
      if (file_count >= 2048)
      {
         bug("load_help_files: too many help files", 0);
         break;
      }

      file_names[file_count++] = str_dup(entry->d_name);
   }

   closedir(dir);

   qsort(file_names, file_count, sizeof(file_names[0]), compare_help_file_names);

   for (i = 0; i < file_count; i++)
   {
      snprintf(path, sizeof(path), "%s%s", directory, file_names[i]);
      db_set_area_name(path);
      load_help_file(path, first, last);
      log_f("%s successfully loaded.", path);
      free_string(file_names[i]);
   }
}

static void load_lore_directory(const char *directory, HELP_DATA **first, HELP_DATA **last)
{
   DIR *dir;
   struct dirent *entry;
   char path[MAX_STRING_LENGTH];
   char *file_names[2048];
   int file_count = 0;
   int i;

   dir = opendir(directory);
   if (dir == NULL)
   {
      log_f("Lore directory not found: %s", directory);
      return;
   }

   while ((entry = readdir(dir)) != NULL)
   {
      size_t len;

      if (entry->d_name[0] == '.')
         continue;

      len = strlen(entry->d_name);
      if (len == 0 || entry->d_name[len - 1] == '~')
         continue;
      if (file_count >= 2048)
      {
         bug("load_lore_directory: too many lore files", 0);
         break;
      }

      file_names[file_count++] = str_dup(entry->d_name);
   }

   closedir(dir);

   qsort(file_names, file_count, sizeof(file_names[0]), compare_help_file_names);

   for (i = 0; i < file_count; i++)
   {
      snprintf(path, sizeof(path), "%s%s", directory, file_names[i]);
      db_set_area_name(path);
      load_lore_file(path, first, last);
      log_f("%s successfully loaded.", path);
      free_string(file_names[i]);
   }
}

void load_help_files(void)
{
   load_help_directory(HELP_DIR, &first_help, &last_help);
   load_help_directory(SHELP_DIR, &first_shelp, &last_shelp);
   load_lore_directory(LORE_DIR, &first_lore, &last_lore);
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits(void)
{

   char buf[MAX_STRING_LENGTH];
   ROOM_INDEX_DATA *pRoomIndex;
   EXIT_DATA *pexit;
   int iHash;
   int door;

   for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
   {
      for (pRoomIndex = room_index_hash[iHash]; pRoomIndex != NULL; pRoomIndex = pRoomIndex->next)
      {
         bool fexit;

         fexit = FALSE;
         for (door = 0; door <= 5; door++)
         {
            if ((pexit = pRoomIndex->exit[door]) != NULL)
            {
               if ((pexit->vnum == 0) || (get_room_index(pexit->vnum) == NULL))
               {
                  sprintf(buf, "Bad exit vnum %d in room %d", pexit->vnum, pRoomIndex->vnum);
                  bug(buf, 0);
                  PUT_FREE(pexit, exit_free);
                  pRoomIndex->exit[door] = NULL;
                  /*			pexit->to_room = NULL;   */
               }

               else
               {
                  fexit = TRUE;
                  pexit->to_room = get_room_index(pexit->vnum);
               }
            }
         }

         if (!fexit)
            SET_BIT(pRoomIndex->room_flags, ROOM_NO_MOB);
      }
   }
   return;
}

/* Spec: see the comments on load_resets about what check_resets does */

#define VALID_RESET 0
#define INVAL_ROOM 1
#define INVAL_OBJ 2
#define INVAL_MOB 3
#define INVAL_GEN 4
void check_resets(void)
{
   AREA_DATA *pArea;
   RESET_DATA *pReset;
   RESET_DATA *nextReset;
   ROOM_INDEX_DATA *last_mob_room;
   ROOM_INDEX_DATA *last_obj_room;
   int previous_bug = 0;

   for (pArea = first_area; pArea; pArea = pArea->next)
   {
      last_mob_room = NULL;
      last_obj_room = NULL;

      for (pReset = pArea->first_reset; pReset; pReset = nextReset)
      {
         ROOM_INDEX_DATA *pRoomIndex = NULL;
         EXIT_DATA *pexit;
         int ValReset = VALID_RESET;

         nextReset = pReset->next;

         switch (pReset->command)
         {
         default:
            SHOW_AREA;
            bug("Load_resets: bad command '%c'.", pReset->command);
            ValReset = INVAL_GEN;
            break;
         case 'A': /* obsolete.. */
            if (!get_room_index(pReset->arg1))
               ValReset = INVAL_ROOM;
            break;
         case 'M':
            if (!get_mob_index(pReset->arg1))
               ValReset = INVAL_MOB;
            if (!(last_mob_room = get_room_index(pReset->arg3)))
               ValReset = INVAL_ROOM;
            break;
         case 'O':
            if (!get_obj_index(pReset->arg1))
               ValReset = INVAL_OBJ;
            if (!(last_obj_room = get_room_index(pReset->arg3)))
               ValReset = INVAL_ROOM;
            break;
         case 'P':
            if (!get_obj_index(pReset->arg1) || !get_obj_index(pReset->arg3))
               ValReset = INVAL_OBJ;
            if (!last_obj_room)
               ValReset = INVAL_ROOM;
            break;
         case 'G':
         case 'E':
            if (!get_obj_index(pReset->arg1))
               ValReset = INVAL_OBJ;
            if (!last_mob_room)
               ValReset = INVAL_ROOM;
            break;
         case 'D':
            if (!(pRoomIndex = get_room_index(pReset->arg1)))
            {
               ValReset = INVAL_ROOM;
               break;
            }
            if (pReset->arg2 < 0 || pReset->arg2 > 5 || !(pexit = pRoomIndex->exit[pReset->arg2]) ||
                !IS_SET(pexit->exit_info, EX_ISDOOR))
            {
               SHOW_AREA;
               bug("Check_resets: 'D': exit %d not door.", pReset->arg2);
               ValReset = INVAL_GEN;
            }

            if (pReset->arg3 < 0 || pReset->arg3 > 2)
            {
               SHOW_AREA;
               bug("Check_resets: 'D': bad 'locks': %d.", pReset->arg3);
               ValReset = INVAL_GEN;
            }
            break;
         case 'R':
            if (!(pRoomIndex = get_room_index(pReset->arg1)))
            {
               ValReset = INVAL_ROOM;
               break;
            }
            if (pReset->arg2 < 0 || pReset->arg2 > 5)
            {
               SHOW_AREA;
               bug("Check_resets: 'R': bad exit %d.", pReset->arg2);
               ValReset = INVAL_GEN;
            }
            break;
         }
         if (ValReset != VALID_RESET)
         {
            char buf[128];

            switch (ValReset)
            {
            case INVAL_ROOM:
               sprintf(buf, "Invalid room for reset: ");
               break;
            case INVAL_OBJ:
               sprintf(buf, "Invalid obj  for reset: ");
               break;
            case INVAL_MOB:
               sprintf(buf, "Invalid mob  for reset: ");
               break;
            case INVAL_GEN:
            default:
               sprintf(buf, "Invalid arg  for reset: ");
               break;
            }
            SHOW_AREA;
            sprintf(buf + strlen(buf), "%c %d %d %d %d.", pReset->command, pReset->ifflag,
                    pReset->arg1, pReset->arg2, pReset->arg3);
            bug(buf, 0);
            {
               int room = 0;
               ROOM_INDEX_DATA *reset_room;
               RESET_DATA *criminal;
               BUILD_DATA_LIST *guilty_reset;
               room = pReset->arg1;
               reset_room = get_room_index(room);
               for (guilty_reset = reset_room->first_room_reset; guilty_reset;
                    guilty_reset = guilty_reset->next)
               {

                  criminal = guilty_reset->data;
                  if (criminal == pReset)
                  {
                     bug("Found the reset", 0);
                     break;
                  }
               }
               if (guilty_reset == NULL)
               {
                  sprintf(bug_buf, "Uhoh, couldn't find that damn reset!");
                  monitor_chan(bug_buf, MONITOR_AREA_BUGS);
                  bug("Couldn't find the reset.", 0);
               }
               else
               {
                  UNLINK(guilty_reset, reset_room->first_room_reset, reset_room->last_room_reset,
                         next, prev);
                  guilty_reset->data = NULL;
                  PUT_FREE(guilty_reset, build_free);
               }
            }
            UNLINK(pReset, pArea->first_reset, pArea->last_reset, next, prev);
            PUT_FREE(pReset, reset_free);
         }
      }
   }
   return;
}

/*
 * Repopulate areas periodically.
 */
void area_update(void)
{
   AREA_DATA *pArea;

   for (pArea = first_area; pArea != NULL; pArea = pArea->next)
   {
      CHAR_DATA *pch;

      pArea->age++;
      /*
       * Check for PC's.
       */
      if ((pArea->nplayer > 0) && (pArea->age >= (pArea->reset_rate - 1)))
      {
         for (pch = first_char; pch != NULL; pch = pch->next)
         {
            if (!IS_NPC(pch) && IS_AWAKE(pch) && pch->in_room != NULL &&
                pch->in_room->area == pArea)
            {
               char reset_buf[MSL];
               sprintf(reset_buf, "%s\n\r", pArea->reset_msg);
               send_to_char(reset_buf, pch);
            }
         }
      }

      /*
       * Check age and reset.
       * Note: Mud School resets every 3 minutes (not 15).
       */
      if (((pArea->nplayer == 0) && (pArea->age >= 5)) || (pArea->age >= pArea->reset_rate))
      {
         reset_area(pArea);
         pArea->age = UMIN(pArea->reset_rate - 1, number_range(0, 3));
      }
   }

   return;
}

static void shuffle_maze_room_exits(ROOM_INDEX_DATA *pRoomIndex);

/*
 * Reset one area.
 */

void reset_area(AREA_DATA *pArea)
{
   RESET_DATA *pReset;
   CHAR_DATA *mob;
   OBJ_DATA *last_obj;
   char buf[MAX_STRING_LENGTH];
   bool last;
   bool just_loaded = FALSE;
   bool just_loaded_obj = FALSE;
   int level;
   int previous_bug = 0;

   sprintf(buf, "Area Reset called for %s.", pArea->name);
   monitor_chan(buf, MONITOR_AREA_UPDATE);
   area_resetting_global = TRUE;
   mob = NULL;
   last_obj = NULL;
   last = TRUE;
   (void)last;
   level = 0;
   for (pReset = pArea->first_reset; pReset != NULL; pReset = pReset->next)
   {
      ROOM_INDEX_DATA *pRoomIndex;
      MOB_INDEX_DATA *pMobIndex;
      OBJ_INDEX_DATA *pObjIndex;
      OBJ_INDEX_DATA *pObjToIndex;
      EXIT_DATA *pexit;
      OBJ_DATA *obj;
      OBJ_DATA *obj_to;
      sh_int num_allowed = 2;

      switch (pReset->command)
      {
      default:
         bug("Reset_area: bad command %c.", pReset->command);
         break;

      case 'A':
         break;

      case 'M':
         just_loaded = FALSE;
         just_loaded_obj = FALSE;
         last_obj = NULL;
         if ((pMobIndex = get_mob_index(pReset->arg1)) == NULL)
         {
            SHOW_AREA;
            bug("Reset_area: 'M': bad vnum %d.", pReset->arg1);
            continue;
         }

         if ((pRoomIndex = get_room_index(pReset->arg3)) == NULL)
         {
            SHOW_AREA;
            bug("Reset_area: 'R': bad vnum %d.", pReset->arg3);
            continue;
         }

         level = pMobIndex->level;
         if (pMobIndex->count >= pReset->arg2)
         {
            last = FALSE;
            break;
         }

         if (!((IS_SET(pMobIndex->act, ACT_DAYONLY) && IS_SET(pMobIndex->act, ACT_NIGHTONLY))) &&
             ((IS_SET(pMobIndex->act, ACT_DAYONLY) && !IS_DAYTIME()) ||
              (IS_SET(pMobIndex->act, ACT_NIGHTONLY) && IS_DAYTIME())))
         {
            last = FALSE;
            break;
         }

         mob = create_mobile(pMobIndex);
         just_loaded = TRUE;

         /*
          * Check for pet shop.
          */
         {
            ROOM_INDEX_DATA *pRoomIndexPrev;
            pRoomIndexPrev = get_room_index(pRoomIndex->vnum - 1);
            if (pRoomIndexPrev != NULL && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
               SET_BIT(mob->act, ACT_PET);
         }

         if (room_is_dark(pRoomIndex))
            SET_BIT(mob->affected_by, AFF_INFRARED);

         if (mob->in_room != NULL)
            char_to_room(mob, mob->in_room);
         else
            char_to_room(mob, pRoomIndex);

         level = URANGE(0, mob->level - 2, LEVEL_HERO);
         last = TRUE;
         break;

      case 'O':
         just_loaded_obj = FALSE;
         last_obj = NULL;
         if ((pObjIndex = get_obj_index(pReset->arg1)) == NULL)
         {
            SHOW_AREA;
            bug("Reset_area: 'O': bad vnum %d.", pReset->arg1);
            continue;
         }

         if ((pRoomIndex = get_room_index(pReset->arg3)) == NULL)
         {
            SHOW_AREA;
            bug("Reset_area: 'R': bad vnum %d.", pReset->arg3);
            continue;
         }
         num_allowed = ((pReset->arg2 == 0) ? 2 : pReset->arg2 - 1);
         if ((count_obj_room(pObjIndex, pRoomIndex->first_content) > num_allowed) ||
             ((count_obj_room(pObjIndex, pRoomIndex->first_content) > 0) &&
              ((pObjIndex->item_type == ITEM_BOARD) || (pObjIndex->item_type == ITEM_FURNITURE) ||
               (pObjIndex->item_type == ITEM_PORTAL) || (pObjIndex->item_type == ITEM_PIECE) ||
               (pObjIndex->extra_flags == ITEM_RARE))))
         {
            last = FALSE;
            break;
         }

         if ((IS_SET(pObjIndex->extra_flags, ITEM_RARE)) &&
             !((number_percent() < 2) && (number_percent() < 8)))
         {
            last = TRUE;
            break;
         }

         if ((IS_SET(pRoomIndex->room_flags, ROOM_NO_REPOP)) && (pRoomIndex->first_person != NULL))
         {
            last = TRUE;
            break;
         }

         obj = create_object(pObjIndex, number_fuzzy(level));

         obj_to_room(obj, pRoomIndex);
         just_loaded_obj = TRUE;
         last_obj = obj;

         last = TRUE;
         break;

      case 'P':
         if ((pObjIndex = get_obj_index(pReset->arg1)) == NULL)
         {
            SHOW_AREA;
            bug("Reset_area: 'P': bad vnum %d.", pReset->arg1);
            continue;
         }

         if ((pObjToIndex = get_obj_index(pReset->arg3)) == NULL)
         {
            SHOW_AREA;
            bug("Reset_area: 'P': bad vnum %d.", pReset->arg3);
            continue;
         }

         if (!just_loaded_obj || last_obj == NULL || last_obj->pIndexData == NULL ||
             last_obj->pIndexData->vnum != pReset->arg3)
         {
            last = FALSE;
            break;
         }

         obj_to = last_obj;

         if (count_obj_list(pObjIndex, obj_to->first_in_carry_list) > 0)
         {
            last = FALSE;
            break;
         }

         obj = create_object(pObjIndex, number_fuzzy(obj_to->level));

         obj_to_obj(obj, obj_to);

         last = TRUE;
         break;

      case 'G':
      case 'E':
         if ((pObjIndex = get_obj_index(pReset->arg1)) == NULL)
         {
            SHOW_AREA;
            bug("Reset_area: 'E' or 'G': bad vnum %d.", pReset->arg1);
            continue;
         }

         if (!just_loaded)
            break;

         if (mob == NULL)
         {
            SHOW_AREA;
            bug("Reset_area: 'E' or 'G': null mob for vnum %d.", pReset->arg1);
            last = FALSE;
            break;
         }

         if (mob->pIndexData->pShop != NULL)
         {
            int olevel;

            switch (pObjIndex->item_type)
            {
            default:
               olevel = 0;
               break;
            case ITEM_PILL:
               olevel = number_range(0, 10);
               break;
            case ITEM_POTION:
               olevel = number_range(0, 10);
               break;
            case ITEM_SCROLL:
               olevel = number_range(5, 15);
               break;
            case ITEM_STAFF:
               olevel = number_range(15, 25);
               break;
            case ITEM_ARMOR:
               olevel = number_range(5, 15);
               break;
            case ITEM_WEAPON:
               olevel = number_range(5, 15);
               break;
            }

            obj = create_object(pObjIndex, olevel);
            SET_BIT(obj->extra_flags, ITEM_INVENTORY);
         }
         else
         {
            bool has_obj = FALSE;
            OBJ_DATA *carried;
            for (carried = mob->first_carry; carried; carried = carried->next_in_carry_list)
            {
               if (carried->pIndexData == pObjIndex)
               {
                  has_obj = TRUE;
                  break;
               }
            }
            if (has_obj)
               break;

            obj = create_object(pObjIndex, number_fuzzy(level));
         }
         obj_to_char(obj, mob);
         if (pReset->command == 'E')
         {
            char objname[MSL];
            one_argument(obj->name, objname);
            do_wear(mob, objname);
         }
         last = TRUE;
         break;

      case 'D':
         if ((pRoomIndex = get_room_index(pReset->arg1)) == NULL)
         {
            bug("Reset_area: 'D': bad vnum %d.", pReset->arg1);
            continue;
         }

         if ((pexit = pRoomIndex->exit[pReset->arg2]) == NULL)
            break;

         switch (pReset->arg3)
         {
         case 0:
            REMOVE_BIT(pexit->exit_info, EX_CLOSED);
            REMOVE_BIT(pexit->exit_info, EX_LOCKED);
            break;

         case 1:
            SET_BIT(pexit->exit_info, EX_CLOSED);
            REMOVE_BIT(pexit->exit_info, EX_LOCKED);
            break;

         case 2:
            SET_BIT(pexit->exit_info, EX_CLOSED);
            SET_BIT(pexit->exit_info, EX_LOCKED);
            break;
         }

         last = TRUE;
         break;

      case 'R':
         if ((pRoomIndex = get_room_index(pReset->arg1)) == NULL)
         {
            SHOW_AREA;
            bug("Reset_area: 'R': bad vnum %d.", pReset->arg1);
            continue;
         }

         {
            int d0;
            int d1;

            for (d0 = 0; d0 < pReset->arg2 - 1; d0++)
            {
               d1 = number_range(d0, pReset->arg2 - 1);
               pexit = pRoomIndex->exit[d0];
               pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
               pRoomIndex->exit[d1] = pexit;
            }
         }
         break;
      }
   }

   /* Randomize exits for all ROOM_MAZE rooms in this area */
   {
      BUILD_DATA_LIST *pList;
      for (pList = pArea->first_area_room; pList != NULL; pList = pList->next)
      {
         ROOM_INDEX_DATA *pRoomIndex = (ROOM_INDEX_DATA *)pList->data;
         if (IS_SET(pRoomIndex->room_flags, ROOM_MAZE))
            shuffle_maze_room_exits(pRoomIndex);
      }
   }

   area_resetting_global = FALSE;
   return;
}

static void shuffle_maze_room_exits(ROOM_INDEX_DATA *pRoomIndex)
{
   int d0;
   int d1;
   EXIT_DATA *pexit;

   for (d0 = 0; d0 < 5; d0++)
   {
      d1 = number_range(d0, 5);
      pexit = pRoomIndex->exit[d0];
      pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
      pRoomIndex->exit[d1] = pexit;
   }
}

#ifdef UNIT_TEST_DB
void db_test_shuffle_maze_room_exits(ROOM_INDEX_DATA *pRoomIndex)
{
   shuffle_maze_room_exits(pRoomIndex);
}
#endif

/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile(MOB_INDEX_DATA *pMobIndex)
{
   CHAR_DATA *mob;
   int level;
   sh_int cnt;

   (void)cnt;

   if (pMobIndex == NULL)
   {
      bug("Create_mobile: NULL pMobIndex.", 0);
      hang("Create Mobile in db.c");
   }
   GET_FREE(mob, char_free);

   clear_char(mob);
   mob->pIndexData = pMobIndex;
   mob->name = str_dup(pMobIndex->player_name);

   mob->short_descr = str_dup(pMobIndex->short_descr);
   mob->long_descr = str_dup(pMobIndex->long_descr);
   mob->description = str_dup(pMobIndex->description);
   mob->switched = FALSE;
   mob->old_body = NULL;
   mob->spec_fun = pMobIndex->spec_fun;
   mob->speech_fun = pMobIndex->speech_fun;
   mob->sitting = NULL;
   mob->prompt = str_dup("<%h %m %v> ");
   mob->first_shield = NULL;
   mob->last_shield = NULL;
   /*
    * Changed exp value for mobs
    */

   level = UMAX(1, number_fuzzy(pMobIndex->level));

   mob->level = UMIN(level, MAX_MOB_LEVEL - 1);

   mob->act = pMobIndex->act;
   mob->affected_by = pMobIndex->affected_by;
   mob->alignment = pMobIndex->alignment;
   mob->sex = pMobIndex->sex;
   mob->ac_mod = pMobIndex->ac_mod;
   mob->hr_mod = pMobIndex->hr_mod;
   mob->dr_mod = pMobIndex->dr_mod;
   mob->hp_mod = pMobIndex->hp_mod;
   mob->spellpower_mod = pMobIndex->spellpower_mod;
   mob->crit_mod = pMobIndex->crit_mod;
   mob->crit_mult_mod = pMobIndex->crit_mult_mod;
   mob->spell_crit_mod = pMobIndex->spell_crit_mod;
   mob->parry_mod = pMobIndex->parry_mod;
   mob->dodge_mod = pMobIndex->dodge_mod;
   mob->block_mod = pMobIndex->block_mod;
   mob->pierce_mod = pMobIndex->pierce_mod;
   mob->loot_amount = pMobIndex->loot_amount;

   for (int i = 0; i < MAX_LOOT; i++)
   {
      mob->loot[i] = pMobIndex->loot[i];
      mob->loot_chance[i] = pMobIndex->loot_chance[i];
   }

   mob->armor = interpolate(mob->level / 2, 100, -100);

   int hit = mob->hp_mod + mob->level * 15 +
             number_range(mob->level * mob->level * 2, mob->level * mob->level * 4);
   mob->hp_mod = hit;
   mob->hit = get_max_hp(mob);

   mob->exp = exp_for_mobile(mob->level, mob);
   mob->intell_exp = 0;

   /*
    * mana for mobs...
    */
   mob->max_mana = level * 25;
   mob->mana = level * 25;

   mob->skills = pMobIndex->skills;
   mob->cast = pMobIndex->cast;
   mob->def = pMobIndex->def;
   mob->class = pMobIndex->class;
   mob->clan = pMobIndex->clan;
   mob->strong_magic = pMobIndex->strong_magic;
   mob->weak_magic = pMobIndex->weak_magic;
   mob->resist = pMobIndex->resist;
   mob->suscept = pMobIndex->suscept;
   mob->race_mods = pMobIndex->race_mods;
   mob->power_skills = pMobIndex->power_skills;
   mob->power_cast = pMobIndex->power_cast;
   mob->race = pMobIndex->race;
   mob->lore_flags = pMobIndex->lore_flags;
   mob->position = POS_STANDING;
   mob->is_quitting = FALSE;
   mob->extract_timer = -1;

   mob->in_room = NULL; /* to distinguish between loaded mobs */
   /*
    * and creted mobs
    */
   mob->ngroup = NULL;

   mob->gold = pMobIndex->pShop ? 10 : 0;

   /*
    * Insert in list.
    */
   mob->next = NULL;
   mob->prev = NULL;
   LINK(mob, first_char, last_char, next, prev);
   pMobIndex->count++;

   //  Create group data for mob

   if (IS_NPC(mob))
   {
      NPC_GROUP_DATA *ngroup;
      GET_FREE(ngroup, npc_group_free);
      ngroup->next = NULL;
      ngroup->prev = NULL;
      ngroup->state = GRP_STATE_IDLE;
      ngroup->first_follower = NULL;
      ngroup->last_follower = NULL;
      ngroup->leader = mob;
      ngroup->seek_room = NULL;
      ngroup->enemies = str_dup("none");
      ngroup->last_fighting = str_dup("none");
      ngroup->wants = str_dup("none");
      ngroup->needs = str_dup("none");
      mob->ngroup = ngroup;
      LINK(ngroup, first_npc_group, last_npc_group, next, prev);
   }

   generate_ai_spawn(mob);
   return mob;
}

/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object(OBJ_INDEX_DATA *pObjIndex, int level)
{
   static OBJ_DATA obj_zero;
   OBJ_DATA *obj;
   AFFECT_DATA *af;
   AFFECT_DATA *new_af;
   int new_cost;
   int looper;
   sh_int cnt;

   (void)af;
   (void)new_af;
   (void)cnt;

   if (level < 0)
      level = 1;

   if (pObjIndex == NULL)
   {
      bug("Create_object: NULL pObjIndex.", 0);
      hang("Creating Object in db.c");
   }

   GET_FREE(obj, obj_free);
   *obj = obj_zero;
   obj->pIndexData = pObjIndex;
   obj->in_room = NULL;
   obj->level = pObjIndex->level;

   obj->wear_loc = -1;

   obj->name = str_dup(pObjIndex->name);
   obj->short_descr = str_dup(pObjIndex->short_descr);
   obj->description = str_dup(pObjIndex->description);
   obj->item_type = pObjIndex->item_type;
   obj->extra_flags = pObjIndex->extra_flags;
   obj->wear_flags = pObjIndex->wear_flags;
   obj->item_apply = pObjIndex->item_apply;
   obj->obj_fun = pObjIndex->obj_fun;
   obj->first_in_room = NULL;
   obj->next_in_room = NULL;
   obj->last_in_room = NULL;
   obj->prev_in_room = NULL;
   obj->first_in_carry_list = NULL;
   obj->next_in_carry_list = NULL;
   obj->last_in_carry_list = NULL;
   obj->prev_in_carry_list = NULL;
   obj->first_content = NULL;
   obj->next_content = NULL;
   obj->last_content = NULL;
   obj->prev_content = NULL;
   obj->weight = pObjIndex->weight;

   obj->in_obj = NULL;

   set_obj_stat_auto(obj);

   for (looper = 0; looper < 10; looper++)
   {
      obj->value[looper] = pObjIndex->value[looper];
   }
   new_cost = get_item_value(obj);
   if (IS_SET(obj->extra_flags, ITEM_RARE))
      new_cost *= 10;
   obj->cost = new_cost;

   obj->cost = UMAX(10, obj->cost);

   obj->condition = 100; /* New, so in tip-top condition */

   /*
    * Mess with object properties.
    */
   switch (obj->item_type)
   {
   default:
      bugf("create_object: vnum %d bad type %d.", pObjIndex->vnum, obj->item_type);
      break;
   case ITEM_SPELL_MATRIX:
   case ITEM_ENCHANTMENT:
   case ITEM_LIGHT:
   case ITEM_BEACON:
   case ITEM_TREASURE:
   case ITEM_FURNITURE:
   case ITEM_TRASH:
   case ITEM_CLUTCH:
      break;
   case ITEM_CONTAINER:
      obj->wear_flags = ITEM_TAKE;
   case ITEM_BOARD:
   case ITEM_KEY:
   case ITEM_BOAT:
   case ITEM_CORPSE_NPC:
   case ITEM_CORPSE_PC:
   case ITEM_PORTAL:
   case ITEM_PIECE:
   case ITEM_SOUL:
   case ITEM_TRIGGER:
   case ITEM_STAKE:
   case ITEM_QUEST:
   case ITEM_WRAPPED_PRESENT:
      break;

   case ITEM_SCROLL:
      obj->value[0] = number_fuzzy(obj->value[0]);
      break;

   case ITEM_STAFF:
      obj->value[0] = number_fuzzy(obj->value[0]);
      obj->value[1] = number_fuzzy(obj->value[1]);
      obj->value[2] = obj->value[1];
      break;

   case ITEM_WEAPON:
      obj->value[1] = number_fuzzy(number_fuzzy(1 * level / 8 + 3));
      obj->value[2] = number_fuzzy(number_fuzzy(3 * level / 8 + 4));
      break;

   case ITEM_ARMOR:
      obj->value[0] = number_fuzzy(level / 12 + 4);
      break;

   case ITEM_POTION:
   case ITEM_PILL:
      obj->value[0] = number_fuzzy(number_fuzzy(obj->value[0]));
      break;

   case ITEM_MONEY:
      obj->value[0] = obj->cost;
      break;
   }

   if (pObjIndex->value[7] > 0)
      obj->timer = pObjIndex->value[7];

   LINK(obj, first_obj, last_obj, next, prev);
   pObjIndex->count++;

   if (obj->item_type == ITEM_CONTAINER && IS_SET(obj->value[1], CONT_KEEP_CHEST))
#if defined(USE_DB_LOAD) && defined(HAVE_LIBPQ)
      ; /* db_load_chests() called at end of boot_db() handles this */
#else
      load_chest(pObjIndex->vnum);
#endif

   return obj;
}

int object_spawn_level(int prototype_level, int requested_level)
{
   if (requested_level > 0)
      return requested_level;

   return prototype_level;
}

/*
 * Clear a new character.
 */
void clear_char(CHAR_DATA *ch)
{
   static CHAR_DATA ch_zero;

   *ch = ch_zero;
   ch->name = &str_empty[0];
   ch->short_descr = &str_empty[0];
   ch->long_descr = &str_empty[0];
   ch->long_descr_orig = &str_empty[0];
   ch->description = &str_empty[0];
   ch->prompt = &str_empty[0];
   ch->last_note = 0;
   ch->logon = current_time;
   ch->armor = 100;
   ch->position = POS_STANDING;
   ch->practice = 21;
   ch->hit = 20;
   ch->max_hit = 20;
   ch->mana = 100;
   ch->max_mana = 100;
   ch->move = 100;
   ch->sitting = NULL;
   ch->max_move = 100;
   ch->act_build = 0;
   ch->build_vnum = -1;
   ch->sentence = 0;
   ch->switched = FALSE;
   ch->old_body = NULL;
   ch->first_shield = NULL;
   ch->last_shield = NULL;
   ch->stunTimer = 0;
   /*    ch->pcdata->recall_vnum = 3001;     */

   return;
}

/*
 * Free a character.
 */
void free_char(CHAR_DATA *ch)
{
   MPROG_ACT_LIST *mpact;
   CHAR_DATA *rch;

   ch->is_quitting = TRUE;
   while (ch->first_carry != NULL)
      extract_obj(ch->first_carry);

   while (ch->first_affect != NULL)
      affect_remove(ch, ch->first_affect);

   while ((mpact = ch->first_mpact) != NULL)
   {
      ch->first_mpact = mpact->next;
      PUT_FREE(mpact, mpact_free);
   }

   for (rch = first_char; rch; rch = rch->next)
   {
      AFFECT_DATA *paf;

      if (rch->master == ch)
         rch->master = NULL;
      if (rch->leader == ch)
         rch->leader = NULL;
      if (rch->fighting == ch)
         rch->fighting = NULL;
      if (rch->reply == ch)
         rch->reply = NULL;
      if (rch->hunting == ch)
      {
         if (IS_NPC(rch) && !IS_NPC(ch))
         {
            free_string(rch->searching);
            rch->searching = str_dup(ch->name);
         }
         else if (!IS_NPC(rch))
         {
            send_to_char("@@RYou seem to have lost your prey.\n\r", rch);
         }
         end_hunt(rch);
      }
      if (rch->hunt_for == ch)
         rch->hunt_for = NULL;
      if (rch->old_body == ch)
      {
         do_return(rch, "");
         rch->old_body = NULL;
      }
      for (mpact = rch->first_mpact; mpact; mpact = mpact->next)
      {
         if (mpact->ch == ch)
            mpact->ch = NULL;
         if (mpact->vo == ch)
            mpact->vo = NULL;
      }
      for (paf = rch->first_affect; paf; paf = paf->next)
         if (paf->caster == ch)
            paf->caster = NULL;
   }

   {
      ROOM_INDEX_DATA *room;
      int rvnum;
      ROOM_AFFECT_DATA *raf;

      for (rvnum = 0; rvnum < MAX_KEY_HASH; rvnum++)
         for (room = room_index_hash[rvnum]; room; room = room->next)
            for (raf = room->first_room_affect; raf; raf = raf->next)
               if (raf->caster == ch)
                  raf->caster = NULL;
   }

   if (ch->pnote)
   {
      PUT_FREE(ch->pnote, note_free);
   }
   if (ch->current_brand)
   {
      PUT_FREE(ch->current_brand, brand_data_free);
   }

   if (ch->revenant != NULL)
   {
      free(ch->revenant);
      ch->revenant = NULL;
   }

   if (ch->pcdata != NULL)
   {
      PUT_FREE(ch->pcdata, pcd_free);
   }

   PUT_FREE(ch, char_free);
   return;
}

/*
 * Get an extra description from a list.
 */
char *get_extra_descr(const char *name, EXTRA_DESCR_DATA *ed)
{
   for (; ed != NULL; ed = ed->next)
   {
      if (is_name(name, ed->keyword))
         return ed->description;
   }
   return NULL;
}

/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index(int vnum)
{
   MOB_INDEX_DATA *pMobIndex;

   for (pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH]; pMobIndex != NULL;
        pMobIndex = pMobIndex->next)
   {
      if (pMobIndex->vnum == vnum)
         return pMobIndex;
   }

   if (fBootDb)
   {
      bug("Get_mob_index: bad vnum %d.", vnum);
      hang("Get mob index in db.c");
   }

   return NULL;
}

/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index(int vnum)
{
   OBJ_INDEX_DATA *pObjIndex;

   for (pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH]; pObjIndex != NULL;
        pObjIndex = pObjIndex->next)
   {
      if (pObjIndex->vnum == vnum)
         return pObjIndex;
   }

   if (fBootDb)
   {
      bug("Get_obj_index: bad vnum %d.", vnum);
      hang("Get Object Index in db.c");
   }

   return NULL;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index(int vnum)
{
   ROOM_INDEX_DATA *pRoomIndex;

   for (pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH]; pRoomIndex != NULL;
        pRoomIndex = pRoomIndex->next)
   {
      if (pRoomIndex->vnum == vnum)
         return pRoomIndex;
   }

   if (fBootDb)
   {
      bug("Get_room_index: bad vnum %d.", vnum);
      /*	exit( 1 );   */
   }

   return NULL;
}

/*
 * Read a letter from a file.
 */
char fread_letter(FILE *fp)
{
   char c;

   do
   {
      c = getc(fp);
   } while (isspace(c));

   return c;
}

/*
 * Read a number from a file.
 */
long fread_number(FILE *fp)
{
   long number;
   bool sign;
   char c;

   do
   {
      c = getc(fp);
   } while (isspace(c));

   number = 0;

   sign = FALSE;
   if (c == '+')
   {
      c = getc(fp);
   }
   else if (c == '-')
   {
      sign = TRUE;
      c = getc(fp);
   }

   if (!isdigit(c))
   {
      char error_buf[MSL];
      sprintf(error_buf, "%c", c);
      bug_string("Fread_number: looking for a digit, found a %s.", error_buf);
      hang("Error in fread_number");
   }

   while (isdigit(c))
   {
      number = number * 10 + c - '0';
      c = getc(fp);
   }

   if (sign)
      number = 0 - number;

   if (c == '|')
      number += fread_number(fp);
   else if (c != ' ')
      ungetc(c, fp);

   return number;
}

unsigned long long fread_number_ull(FILE *fp)
{
   unsigned long long number;
   bool sign;
   char c;

   do
   {
      c = getc(fp);
   } while (isspace(c));

   number = 0;

   sign = FALSE;
   if (c == '+')
   {
      c = getc(fp);
   }
   else if (c == '-')
   {
      sign = TRUE;
      c = getc(fp);
   }

   if (!isdigit(c))
   {
      char error_buf[MSL];
      sprintf(error_buf, "%c", c);
      bug_string("Fread_number_ull: looking for a digit, found a %s.", error_buf);
      hang("Error in fread_number_ull");
   }

   while (isdigit(c))
   {
      number = number * 10 + c - '0';
      c = getc(fp);
   }

   if (sign)
      number = 0 - number;

   if (c == '|')
      number += fread_number_ull(fp);
   else if (c != ' ')
      ungetc(c, fp);

   return number;
}

/*
 * Read to end of line (for comments).
 */

/* Spec: fixed to handle EOF more gracefully */

void fread_to_eol(FILE *fp)
{
   char c;

   do
   {
      c = getc(fp);
   } while (c != '\n' && c != '\r' && c != EOF);

   while (c == '\n' || c == '\r')
   {
      c = getc(fp);
   }

   if (c == EOF)
   {
      bugf("fread_to_eol: EOF");
      return;
   }

   ungetc(c, fp);
   return;
}

/* Same as above, but returns the rest of the line */

/* Spec: fixed to handle EOF more gracefully */

char *fsave_to_eol(FILE *fp)
{
   char string[MAX_INPUT_LENGTH + 1];
   int a;
   char c;

   a = 0;
   do
   {
      string[a++] = getc(fp);

   } while (a < MAX_INPUT_LENGTH && string[a - 1] != '\n' && string[a - 1] != '\r' &&
            string[a - 1] != EOF);

   if (string[a - 1] == EOF)
   {
      bugf("fsave_to_eol: EOF");
      string[a - 1] = 0;
      return str_dup(string);
   }

   if (a == MAX_INPUT_LENGTH)
      fread_to_eol(fp);
   else
   {
      c = string[a - 1];
      while (c == '\n' || c == '\r')
         c = getc(fp);

      if (c == EOF)
         bugf("fsave_to_eol: EOF");
      else
         ungetc(c, fp);
   }
   string[a] = '\0';

   return str_dup(string);
}

/*
 * Read one word (into static buffer).
 */
char *fread_word(FILE *fp)
{
   static char word[MAX_INPUT_LENGTH];
   char *pword;
   char cEnd;

   do
   {
      if (feof(fp))
      {
         bug("fread_word: EOF encountered on read.\n\r", 0);
         if (fBootDb)
            kill(getpid(), SIGQUIT);
         strcpy(word, "");
         return word;
      }
      cEnd = getc(fp);
   } while (isspace(cEnd));

   if (cEnd == '\'' || cEnd == '"')
   {
      pword = word;
   }
   else
   {
      word[0] = cEnd;
      pword = word + 1;
      cEnd = ' ';
   }

   for (; pword < word + MAX_INPUT_LENGTH; pword++)
   {
      if (feof(fp))
      {
         bug("fread_word: EOF encountered on read.\n\r", 0);
         if (fBootDb)
            kill(getpid(), SIGQUIT);
         *pword = '\0';
         return word;
      }
      *pword = getc(fp);
      if (cEnd == ' ' ? isspace(*pword) : *pword == cEnd)
      {
         if (cEnd == ' ')
            ungetc(*pword, fp);
         *pword = '\0';
         return word;
      }
   }

   bug("Fread_word: word too long", 0);
   kill(getpid(), SIGQUIT);
   return NULL;
}

void *_getmem(int size, const char *caller, int log)
{
   void *mem;

   if (!(mem = malloc(size)))
   {
      fprintf(stderr, "Out of memory.\n");
      raise(SIGSEGV);
   }
   memset(mem, 0, size);

   if (log && mem_log)
      log_f("getmem(%d)=%p from %s", size, mem, caller);

   return mem;
}

#if 0
/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
#define STRING_FREELIST
#ifdef STRING_FREELIST
#define MAX_SIZE_LIST 13
static const int sizelist[MAX_SIZE_LIST] = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };
struct text_data
{
   struct text_data *next;
   char *text;
};
static struct text_data *text_free[MAX_SIZE_LIST] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL
};
static struct text_data *tdf_free = NULL;
#endif
char *str_dup( const char *str )
{
   char *str_new;
#ifdef STRING_FREELIST
   sh_int size;
   int len;
#endif

   if( !str || !*str )
      return &str_empty[0];

   if( str >= string_space && str < top_string )
      return ( char * )str;

#ifdef STRING_FREELIST
   len = strlen( str ) + 1;
   for( size = 0; size < MAX_SIZE_LIST; size++ )
      if( len < sizelist[size] )
         break;
   if( size < MAX_SIZE_LIST )
   {
      if( text_free[size] != NULL )
      {
         struct text_data *tdf = text_free[size];

         text_free[size] = tdf->next;
         str_new = tdf->text;
         tdf->next = tdf_free;
         tdf_free = tdf;
      }
      else
         str_new = getmem( sizelist[size] );
   }
   else
      str_new = getmem( len );
#else
   str_new = getmem( strlen( str ) + 1 );
#endif
   strcpy( str_new, str );
   return str_new;
}



/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string( char *pstr )
{
#ifdef STRING_FREELIST
   sh_int size;
   int len;
#endif

   if( pstr == NULL || pstr == &str_empty[0] || ( pstr >= string_space && pstr < top_string ) )
      return;

#ifdef STRING_FREELIST
   len = strlen( pstr ) + 1;
   for( size = 0; size < MAX_SIZE_LIST; size++ )
      if( len < sizelist[size] )
         break;
   if( size < MAX_SIZE_LIST )
   {
      struct text_data *tdf;

      if( tdf_free )
      {
         tdf = tdf_free;
         tdf_free = tdf->next;
      }
      else
         tdf = getmem( sizeof( *tdf ) );
      tdf->text = pstr;
      tdf->next = text_free[size];
      text_free[size] = tdf;
   }
   else
      dispose( pstr, len );
#else
   dispose( pstr, strlen( pstr ) + 1 );
#endif
   return;
}

#endif

static int compare_area_levels(const void *left, const void *right)
{
   const AREA_DATA *left_area;
   const AREA_DATA *right_area;

   left_area = *(const AREA_DATA *const *)left;
   right_area = *(const AREA_DATA *const *)right;

   if (left_area->min_level != right_area->min_level)
      return left_area->min_level - right_area->min_level;
   if (left_area->max_level != right_area->max_level)
      return left_area->max_level - right_area->max_level;

   return strcmp(left_area->name, right_area->name);
}

void do_areas(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char msg[MAX_STRING_LENGTH];
   char arg1[MSL];
   sh_int foo;
   int area_count;
   int area_index;
   AREA_DATA *pArea;
   AREA_DATA **area_list;
   bool fall = FALSE;

   argument = one_argument(argument, arg1);

   if (!str_cmp(arg1, "all"))
      fall = TRUE;
   sprintf(buf, "@@W" mudnamecolor " AREA LISTING\n\r");
   safe_strcat(MAX_STRING_LENGTH, buf,
               "+---------+------------+------------------------------------------------+\n\r");
   safe_strcat(
       MAX_STRING_LENGTH, buf,
       "|  @@yLevel@@W  |            |                                                |\n\r");
   safe_strcat(MAX_STRING_LENGTH, buf,
               "|  @@yrange@@W  |   @@yAuthor@@W   |      @@yName of Area@@W                       "
               "       |\n\r");
   safe_strcat(MAX_STRING_LENGTH, buf,
               "+---------+------------+------------------------------------------------+\n\r");

   area_count = 0;
   for (pArea = first_area; pArea != NULL; pArea = pArea->next)
   {
      if ((IS_SET(pArea->flags, AREA_NOSHOW)) || (IS_SET(pArea->flags, AREA_BUILDING)))
         continue; /* for non-finished areas - don't show */
      if ((!fall) && ((pArea->min_level > (get_psuedo_level(ch))) ||
                      (pArea->max_level < (get_psuedo_level(ch)))))
         continue;
      area_count++;
   }

   area_list = NULL;
   if (area_count > 0)
   {
      area_list = malloc(sizeof(AREA_DATA *) * area_count);
      if (area_list == NULL)
      {
         send_to_char("Area list is currently unavailable.\n\r", ch);
         return;
      }
   }

   area_index = 0;
   for (pArea = first_area; pArea != NULL; pArea = pArea->next)
   {
      if ((IS_SET(pArea->flags, AREA_NOSHOW)) || (IS_SET(pArea->flags, AREA_BUILDING)))
         continue; /* for non-finished areas - don't show */
      if ((!fall) && ((pArea->min_level > (get_psuedo_level(ch))) ||
                      (pArea->max_level < (get_psuedo_level(ch)))))
         continue;
      area_list[area_index++] = pArea;
   }

   qsort(area_list, area_count, sizeof(AREA_DATA *), compare_area_levels);

   foo = 0;
   for (area_index = 0; area_index < area_count; area_index++)
   {
      pArea = area_list[area_index];
      foo++;
      sprintf(msg, "@@N{@@r%3d %3d@@N} %12s         %s\n\r", pArea->min_level, pArea->max_level,
              capitalize(pArea->owner), pArea->name);
      safe_strcat(MAX_STRING_LENGTH, buf, msg);
   }

   if (area_list != NULL)
      free(area_list);

   sprintf(msg,
           "@@R%d Areas listed.\n\r@@N Type areas all to list the entire " mudnamecolor
           " realm.\n\r@@N",
           foo);
   safe_strcat(MAX_STRING_LENGTH, buf, msg);
   send_to_char(buf, ch);
   return;
}

void perm_update()
{
   /*
    * output perm usage to log file
    */
   FILE *po;
   char *strtime;
   po = fopen("perm.out", "a");

   strtime = ctime(&current_time);
   strtime[strlen(strtime) - 1] = '\0';

   fprintf(po, "%s :: Perms   %5d blocks  of %7d bytes.\n\r", strtime, nAllocPerm, sAllocPerm);
   if (po != NULL)
   {
      fclose(po);
      po = NULL;
   }
   return;
}

void do_memory(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];

   if (!str_cmp(argument, "defrag"))
   {
      send_to_char("Defragmenting SSM heap.", ch);
      log_f("SSM: %s called defrag_heap.", ch->name);
      defrag_heap();
      return;
   }

   if (!str_cmp(argument, "log"))
   {
      if (get_trust(ch) < MAX_LEVEL)
      {
         send_to_char("Not at your level.\n\r", ch);
         return;
      }

      if (mem_log)
      {
         mem_log = FALSE;
         send_to_char("Memory logging is now OFF.\n\r", ch);
         log_f("%s turned off memory logging", ch->name);
         return;
      }
      else
      {
         mem_log = TRUE;
         send_to_char("Memory logging is now ON.. remember to turn it off!\n\r", ch);
         log_f("%s turned on memory logging", ch->name);
         return;
      }
   }

   sprintf(buf, "Affects %5d\n\r", top_affect);
   send_to_char(buf, ch);
   sprintf(buf, "Areas   %5d\n\r", top_area);
   send_to_char(buf, ch);
   sprintf(buf, "ExDes   %5d\n\r", top_ed);
   send_to_char(buf, ch);
   sprintf(buf, "Exits   %5d\n\r", top_exit);
   send_to_char(buf, ch);
   sprintf(buf, "Helps   %5d\n\r", top_help);
   send_to_char(buf, ch);
   sprintf(buf, "Mobs    %5d\n\r", top_mob_index);
   send_to_char(buf, ch);
   sprintf(buf, "Objs    %5d\n\r", top_obj_index);
   send_to_char(buf, ch);
   sprintf(buf, "Resets  %5d\n\r", top_reset);
   send_to_char(buf, ch);
   sprintf(buf, "Rooms   %5d\n\r", top_room);
   send_to_char(buf, ch);
   sprintf(buf, "Shops   %5d\n\r", top_shop);
   send_to_char(buf, ch);

#if 0
   sprintf( buf, "Strings %5d strings of %7d bytes (max %d).\n\r", nAllocString, sAllocString, MAX_STRING );
#endif

   sprintf(buf, "Shared String Info:\n\r");
   send_to_char(buf, ch);
   sprintf(buf, "Strings           %5ld strings of %7ld bytes (max %ld).\n\r", nAllocString,
           sAllocString, MAX_STRING);
   send_to_char(buf, ch);
   sprintf(buf, "Overflow Strings  %5ld strings of %7ld bytes.\n\r", nOverFlowString,
           sOverFlowString);
   send_to_char(buf, ch);
   if (Full)
   {
      send_to_char("Shared String Heap is full, increase MAX_STRING.\n\r", ch);
      sprintf(buf, "Overflow high-water-mark is %ld bytes.\n\r", hwOverFlow);
      send_to_char(buf, ch);
   }

   sprintf(buf, "Perms   %5d blocks  of %7d bytes.\n\r", nAllocPerm, sAllocPerm);
   send_to_char(buf, ch);

   return;
}

void do_status(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];

   send_to_char("             Status Report for Ack! Mud:\n\r", ch);
   send_to_char("             ---------------------------\n\r\n\r", ch);
   do_time(ch, "");

   send_to_char("\n\r", ch);
   send_to_char("The following counts are for *distinct* mobs/objs/rooms, not a count\n\r", ch);
   send_to_char("of how many are actually in the game at this time.\n\r", ch);
   send_to_char("NB. Areas count will include areas used as help files.\n\r\n\r", ch);
   sprintf(buf, "Areas   %5d\n\r", top_area);
   send_to_char(buf, ch);
   sprintf(buf, "Helps   %5d\n\r", top_help);
   send_to_char(buf, ch);
   sprintf(buf, "Mobs    %5d\n\r", top_mob_index);
   send_to_char(buf, ch);
   sprintf(buf, "Objs    %5d\n\r", top_obj_index);
   send_to_char(buf, ch);
   sprintf(buf, "Rooms   %5d\n\r", top_room);
   send_to_char(buf, ch);

   return;
}

/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy(int number)
{
   switch (number_bits(2))
   {
   case 0:
      number -= 1;
      break;
   case 3:
      number += 1;
      break;
   }

   return UMAX(1, number);
}

/*
 * Generate a random number.
 */
int number_range(int from, int to)
{
   int power;
   int number;

   if ((to = to - from + 1) <= 1)
      return from;

   for (power = 2; power < to; power <<= 1)
      ;

   while ((number = number_mm() & (power - 1)) >= to)
      ;

   return from + number;
}

/*
 * Generate a percentile roll.
 */
int number_percent(void)
{
   int percent;

   while ((percent = number_mm() & (128 - 1)) > 99)
      ;

   return 1 + percent;
}

/*
 * Generate a random door.
 */
int number_door(void)
{
   int door;

   while ((door = number_mm() & (8 - 1)) > 5)
      ;

   return door;
}

int number_bits(int width)
{
   return number_mm() & ((1 << width) - 1);
}

/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */
static int rgiState[2 + 55];

void init_mm()
{
   int *piState;
   int iState;

   piState = &rgiState[2];

   piState[-2] = 55 - 55;
   piState[-1] = 55 - 24;

   piState[0] = ((int)current_time) & ((1 << 30) - 1);
   piState[1] = 1;
   for (iState = 2; iState < 55; iState++)
   {
      piState[iState] = (piState[iState - 1] + piState[iState - 2]) & ((1 << 30) - 1);
   }
   return;
}

int number_mm(void)
{
   int *piState;
   int iState1;
   int iState2;
   int iRand;

   piState = &rgiState[2];
   iState1 = piState[-2];
   iState2 = piState[-1];
   iRand = (piState[iState1] + piState[iState2]) & ((1 << 30) - 1);
   piState[iState1] = iRand;
   if (++iState1 == 55)
      iState1 = 0;
   if (++iState2 == 55)
      iState2 = 0;
   piState[-2] = iState1;
   piState[-1] = iState2;
   return iRand >> 6;
}

/*
 * Roll some dice.
 */
int dice(int number, int size)
{
   int idice;
   int sum;

   switch (size)
   {
   case 0:
      return 0;
   case 1:
      return number;
   }

   for (idice = 0, sum = 0; idice < number; idice++)
      sum += number_range(1, size);

   return sum;
}

/*
 * Simple linear interpolation.
 */
int interpolate(int level, int value_00, int value_32)
{
   return value_00 + level * (value_32 - value_00) / 32;
}

/*
 * Append a string to a file.
 */
void append_file(CHAR_DATA *ch, char *file, char *str)
{
   FILE *fp;

   if (IS_NPC(ch) || str[0] == '\0')
      return;

   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }
   if ((fp = fopen(file, "a")) == NULL)
   {
      perror(file);
      send_to_char("Could not open the file!\n\r", ch);
   }
   else
   {
      fprintf(fp, "[%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, str);
      if (fp != NULL)
      {
         fclose(fp);
         fp = NULL;
      }
   }
   return;
}

/*
 * Nice little functions that limit the amount of typing you have to do with
 * each and every log File entry and bug report.---Flar
 */
void bugf(char *fmt, ...)
{
   char buf[MSL];
   va_list args;
   va_start(args, fmt);
   vsnprintf(buf, sizeof(buf), fmt, args);
   va_end(args);

   bug(buf, 0);
}

void log_f(char *fmt, ...)
{
   char buf[2 * MSL];
   va_list args;
   va_start(args, fmt);
   vsnprintf(buf, sizeof(buf), fmt, args);
   va_end(args);

   log_string(buf);
}

/*
 * Count the line number at the current position in an area file.
 * Seeks to the start, counts newlines up to the saved position,
 * and restores the file pointer. Returns the 0-based line number.
 */
int count_file_line(FILE *fp)
{
   int iLine;
   int iChar;

   iChar = ftell(fp);
   fseek(fp, 0, 0);
   for (iLine = 0; ftell(fp) < iChar; iLine++)
   {
      int ch;
      while ((ch = getc(fp)) != '\n' && ch != EOF)
         ;
      if (ch == EOF)
         break;
   }
   fseek(fp, iChar, 0);
   return iLine;
}

/*
 * Reports a bug.
 */
void bug(const char *str, int param)
{
   char buf[MAX_STRING_LENGTH];
   FILE *fp;

   if (fpArea != NULL)
   {
      int iLine;

      iLine = (fpArea == stdin) ? 0 : count_file_line(fpArea);

      sprintf(buf, "[*****] FILE: %s LINE: %d", strArea, iLine);
      log_string(buf);

      if ((fp = fopen(SHUTDOWN_FILE, "a")) != NULL)
      {
         fprintf(fp, "[*****] %s\n", buf);
         if (fp != NULL)
         {
            fclose(fp);
            fp = NULL;
         }
      }
   }

   strcpy(buf, "[*****] BUG: ");
   sprintf(buf + strlen(buf), str, param);
   log_string(buf);

   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }
   if ((fp = fopen(BUG_FILE, "a")) != NULL)
   {
      fprintf(fp, "%s\n", buf);
      fclose(fp);
      fp = NULL;
   }

   return;
}

void bug_string(const char *str, const char *str2)
{
   char buf[MAX_STRING_LENGTH];
   FILE *fp;

   if (fpArea != NULL)
   {
      int iLine;

      iLine = (fpArea == stdin) ? 0 : count_file_line(fpArea);

      sprintf(buf, "[*****] FILE: %s LINE: %d", strArea, iLine);
      log_string(buf);

      if ((fp = fopen(SHUTDOWN_FILE, "a")) != NULL)
      {
         fprintf(fp, "[*****] %s\n", buf);
         fclose(fp);
         fp = NULL;
      }
   }

   strcpy(buf, "[*****] BUG: ");
   sprintf(buf + strlen(buf), str, str2);
   log_string(buf);

   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }
   if ((fp = fopen(BUG_FILE, "a")) != NULL)
   {
      fprintf(fp, "%s\n", buf);
      fclose(fp);
   }

   return;
}

/*
 * Writes a string to the log.
 */
void log_string(const char *str)
{
   char *strtime;

   strtime = ctime(&current_time);
   strtime[strlen(strtime) - 1] = '\0';
   fprintf(stderr, "%s :: %s\n", strtime, str);
   return;
}

/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain(void)
{
   return;
}

void message_update(void)
{
   RESET_DATA *pReset;
   AREA_DATA *pArea;
   ROOM_INDEX_DATA *pRoom;
   CHAR_DATA *ch;

   for (pArea = first_area; pArea != NULL; pArea = pArea->next)
   {
      for (pReset = pArea->first_reset; pReset != NULL; pReset = pReset->next)
      {
         if (pReset->command != 'A')
            continue;

         if ((pRoom = get_room_index(pReset->arg1)) == NULL)
         {
            bug("message_update, no room %d found.", pReset->arg1);
            continue;
         }

         for (ch = pRoom->first_person; ch != NULL; ch = ch->next_in_room)
            if (ch->level >= pReset->arg2 && ch->level <= pReset->arg3)
               send_to_char(pReset->notes, ch);
      }
   }
   return;
}
