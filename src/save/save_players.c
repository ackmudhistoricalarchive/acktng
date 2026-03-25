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

/*
 * save_players.c — Player character file save and load.
 *
 * Handles serialisation and deserialisation of player characters:
 *   save_char_obj / load_char_obj  — top-level entry points
 *   fwrite_char / fread_char       — character field serialisation
 */

#include "save.h"
#include "weapon_bond.h"
#include "../db/db_worker.h"

/*
 * skill_name_legacy -- maps a pre-rename skill name to its current name.
 * Returns the new name string if old_name is a legacy name, NULL otherwise.
 * Used during character file load to silently migrate old save files.
 */
const char *skill_name_legacy(const char *old_name)
{
   static const char *aliases[][2] = {{"shadow step", "gap transit"},
                                      {"garrote", "reach silence"},
                                      {"pressure point", "applied understanding"},
                                      {"fortify", "seven shade hold"},
                                      {"iron resolve", "veteran's cadence"},
                                      /* Mortal class skill/spell renames */
                                      {"warcry", "dunmar's call"},
                                      {"headbutt", "checkpoint break"},
                                      {"knee", "road sweep"},
                                      {"detox", "reach remedy"},
                                      {"taunt", "charter challenge"},
                                      {"feign death", "dissolution protocol"},
                                      {"mark target", "asset flag"},
                                      {"conceal", "gap hold"},
                                      {"set trap", "seam snare"},
                                      {"mindoverbody", "interior discipline"},
                                      {"conditioning", "cistern discipline"},
                                      {"mystical potency", "compact amplification"},
                                      {"spell critical", "cognitive fault"},
                                      {"spell critical damage", "solar precision"},
                                      {"Mind Over Body", "interior discipline"},
                                      {"see magic", "architecture scan"},
                                      {"fighting trance", "assertion form"},
                                      {"night vision", "peripheral extension"},
                                      {"acid blast", "dissolution"},
                                      {"call lightning", "shard ward"},
                                      {"cause critical", "tribunal's hand"},
                                      {"cause serious", "solar censure"},
                                      {"control weather", "seal calculus"},
                                      {"purge", "cognitive flush"},
                                      {"earthquake", "jackal's verdict"},
                                      {"faerie fire", "prism mark"},
                                      {"faerie fog", "ledger mark"},
                                      {"sense evil", "pale imprint"},
                                      {"mindflame", "recurrence loop"},
                                      {"color spray", "crystal burst"},
                                      {"holy word", "disk's word"},
                                      {"prayer", "solar invocation"},
                                      {NULL, NULL}};
   int i;
   for (i = 0; aliases[i][0] != NULL; i++)
      if (!strcmp(old_name, aliases[i][0]))
         return aliases[i][1];
   return NULL;
}

char *cap_nocol(const char *str)
{
   static char strcap[MAX_STRING_LENGTH];
   int i;

   for (i = 0; str[i] != '\0'; i++)
      strcap[i] = LOWER(str[i]);
   strcap[i] = '\0';
   strcap[0] = UPPER(strcap[0]);
   return strcap;
}

/* Courtesy of Yaz of 4th Realm */
char *initial(const char *str)
{
   static char strint[MAX_STRING_LENGTH];

   strint[0] = LOWER(str[0]);
   return strint;
}

/*
 * Save a character and inventory.
 */
void save_char_obj(CHAR_DATA *ch)
{
   char strsave[MAX_INPUT_LENGTH];
   char tempstrsave[MAX_INPUT_LENGTH]; /* Hold temp filename here.. */
   char buf[MAX_INPUT_LENGTH];         /* hold misc stuff here.. */
   FILE *fp;
   char *nmptr, *bufptr;

   if (deathmatch)
   {
      /*
       * Then a deathmatch is in progress...
       */
      /*
       * And therefore don't save...
       */
      return;
   }

   if (IS_NPC(ch) || (ch->level < 2 && get_total_reincarnations(ch) < 1))
      return;

   if (!IS_NPC(ch) && ch->desc != NULL && ch->desc->original != NULL)
      ch = ch->desc->original;

   ch->save_time = current_time;
   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }

   /*
    * player files parsed directories by Yaz 4th Realm
    */
#if !defined(machintosh) && !defined(MSDOS)
   if (IS_NPC(ch)) /* convert spaces to . */
   {
      for (nmptr = ch->name, bufptr = buf; *nmptr != 0; nmptr++)
      {
         if (*nmptr == ' ')
            *(bufptr++) = '.';
         else
            *(bufptr++) = *nmptr;
      }
      *(bufptr) = *nmptr;
   }
   else
      strcpy(buf, ch->name);
   sprintf(strsave, "%s%s%s%s", PLAYER_DIR, initial(buf), "/", cap_nocol(buf));
#else
   /*
    * Convert npc names to dos compat name.... yuk
    */
   if (IS_NPC(ch))
   {
      for (nmptr = ch->name, bufptr = buf; *nmptr != 0; nmptr++)
      {
         if (*nmptr != ' ' && *nmptr != '.')
            *(bufptr++) = *nmptr;
         if (bufptr - buf == 8)
            break;
      }
      *(bufptr) = 0;
   }
   else
      strcpy(buf, ch->name);

   sprintf(strsave, "%s%s", PLAYER_DIR, cap_nocol(buf));
#endif
   /*
    * Tack on a .temp to strsave, use as tempstrsave
    */

   {
      size_t strsave_len = strlen(strsave);
      if (strsave_len + sizeof(".temp") <= sizeof(tempstrsave))
      {
         memcpy(tempstrsave, strsave, strsave_len + 1);
         memcpy(tempstrsave + strsave_len, ".temp", sizeof(".temp"));
      }
      else
         tempstrsave[0] = '\0';
   }

   if ((fp = fopen(tempstrsave, "w")) == NULL)
   {
      monitor_chan("Save_char_obj: fopen", MONITOR_BAD);
      perror(strsave);
   }
   else
   {
      fwrite_char(ch, fp);
      loop_counter = 0;
      if (ch->first_carry != NULL)
         fwrite_obj(ch, ch->first_carry, fp, 0);
      fprintf(fp, "#END\n");
   }
   fflush(fp);
   if (fp != NULL)
   {
      fclose(fp);
      fp = NULL;
   }

   /*
    * Now make temp file the actual pfile...
    */

   {
      char *raw = NULL;
      size_t rawlen = 0;
      FILE *memfp = open_memstream(&raw, &rawlen);
      if (memfp != NULL)
      {
         fwrite_char(ch, memfp);
         loop_counter = 0;
         if (ch->first_carry != NULL)
            fwrite_obj(ch, ch->first_carry, memfp, 0);
         fprintf(memfp, "#END\n");
         fclose(memfp);
         db_worker_enqueue_write(DB_WRITE_PLAYER, raw, rawlen, ch->name);
         free(raw);
         /* DB is authoritative; remove the temp flat file and return. */
         if (tempstrsave[0])
            remove(tempstrsave);
         return;
      }
   }

   rename(tempstrsave, strsave);
}

/*
 * Write the char.
 */
void fwrite_char(CHAR_DATA *ch, FILE *fp)
{
   /*
    * UUURRRGGGGHHHHHH!  When writing out ch->class_level[x] no loop used,
    * * instead, the values are just done 0,1,2,etc.. yuck.  -S-
    */

   AFFECT_DATA *paf;
   int cnt;
   int sn;
   int foo;

   fprintf(fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER");

   fprintf(fp, "Revision     %d\n", SAVE_REVISION);
   fprintf(fp, "Name         %s~\n", ch->name);
   fprintf(fp, "ShortDescr   %s~\n", ch->short_descr);
   fprintf(fp, "LongDescr    %s~\n", ch->long_descr_orig);
   fprintf(fp, "Description  %s~\n", ch->description);
   fprintf(fp, "Prompt       %s~\n", ch->prompt);
   fprintf(fp, "Sex          %d\n", ch->sex);
   fprintf(fp, "LoginSex     %d\n", ch->login_sex);
   fprintf(fp, "Class        %d\n", ch->class);
   fprintf(fp, "Race         %d\n", ch->race);
   fprintf(fp, "Level        %d\n", ch->level);
   fprintf(fp, "Sentence     %d\n", ch->sentence);
   fprintf(fp, "Invis        %d\n", ch->invis);
   fprintf(fp, "Incog		 %d\n", ch->incog);

   fprintf(fp, "MortalClass  ");
   for (cnt = 0; cnt < 4; cnt++)
      fprintf(fp, "%2d ", ch->mortal_class[cnt]);
   fprintf(fp, "\n");

   fprintf(fp, "MortalLevel  ");
   for (cnt = 0; cnt < 4; cnt++)
      fprintf(fp, "%2d ", ch->mortal_level[cnt]);
   fprintf(fp, "\n");

   fprintf(fp, "RemortClass  ");
   for (cnt = 0; cnt < 2; cnt++)
      fprintf(fp, "%2d ", ch->remort_class[cnt]);
   fprintf(fp, "\n");

   fprintf(fp, "RemortLevel  ");
   for (cnt = 0; cnt < 2; cnt++)
      fprintf(fp, "%2d ", ch->remort_level[cnt]);
   fprintf(fp, "\n");

   fprintf(fp, "AdeptClass   %2d\n", ch->adept_class);

   fprintf(fp, "Druidreinc   ");
   for (cnt = CLASS_DRU; cnt <= CLASS_HIE; cnt++)
      fprintf(fp, "%2d ", ch->pcdata->reincarnations[cnt]);
   fprintf(fp, "\n");

   fprintf(fp, "Overgrowth   %d\n", ch->overgrowth);

   fprintf(fp, "Reincarnations ");
   for (cnt = 0; cnt < MAX_CLASS; cnt++)
      fprintf(fp, "%2d ", ch->pcdata->reincarnations[cnt]);
   fprintf(fp, "\n");

   fprintf(fp, "Remortreincarnations ");
   for (cnt = 0; cnt < MAX_REMORT; cnt++)
      fprintf(fp, "%2d ", ch->pcdata->reincarnations[MAX_CLASS + cnt]);
   fprintf(fp, "\n");

   fprintf(fp, "Adeptreincarnations ");
   for (cnt = 0; cnt < MAX_CLASS; cnt++)
      fprintf(fp, "%2d ", ch->pcdata->reincarnations[MAX_CLASS + MAX_REMORT + cnt]);
   fprintf(fp, "\n");

   fprintf(fp, "Reinc_data ");
   for (cnt = 0; cnt < MAX_REINCARNATE; cnt++)
      fprintf(fp, "%2d ", ch->pcdata->reincarnation_data[cnt]);
   fprintf(fp, "\n");

   fprintf(fp, "Adeptlevel   ");
   fprintf(fp, "%2d\n ", ch->adept_level);

   fprintf(fp, "Trust        %d\n", ch->trust);
   fprintf(fp, "Wizbit       %d\n", ch->wizbit);
   fprintf(fp, "Played       %d\n", ch->played + (int)(current_time - ch->logon));
   fprintf(fp, "Note         %ld\n", ch->last_note);
   fprintf(fp, "Room         %d\n",
           (ch->in_room == get_room_index(ROOM_VNUM_LIMBO) && ch->was_in_room != NULL)
               ? ch->was_in_room->vnum
               : ch->in_room->vnum);

   fprintf(fp, "HpManaMove   %ld %ld %ld %ld %ld %ld\n", ch->hit, ch->hit, ch->mana, ch->max_mana,
           ch->move, ch->max_move);

   fprintf(fp, "Exp          %ld\n", ch->exp);
   fprintf(fp, "Act          %llu\n", ch->act);
   fprintf(fp, "Config       %d\n", ch->config);
   fprintf(fp, "Gold         %d\n", ch->gold);
   fprintf(fp, "AffectedBy   %d\n", ch->affected_by);
   /*
    * Bug fix from Alander
    */
   fprintf(fp, "Position     %d\n", is_fighting(ch) ? POS_STANDING : ch->position);

   fprintf(fp, "Practice     %d\n", ch->practice);
   fprintf(fp, "SavingThrow  %d\n", ch->saving_throw);
   fprintf(fp, "Alignment    %d\n", ch->alignment);
   fprintf(fp, "Hitroll      %d\n", ch->hitroll);
   fprintf(fp, "Damroll      %d\n", ch->damroll);
   fprintf(fp, "Armor        %d\n", ch->armor);
   fprintf(fp, "Wimpy        %d\n", ch->wimpy);
   fprintf(fp, "Deaf         %d\n", ch->deaf);

   if (IS_NPC(ch))
   {
      fprintf(fp, "Vnum         %d\n", ch->pIndexData->vnum);
   }
   else
   {
      fprintf(fp, "Generation   %d\n", ch->pcdata->generation);
      fprintf(fp, "Clan         %d\n", ch->pcdata->clan);
      fprintf(fp, "Mkills	   %d\n", ch->pcdata->mkills);
      fprintf(fp, "Mkilled	   %d\n", ch->pcdata->mkilled);
      fprintf(fp, "Pkills	   %d\n", ch->pcdata->pkills);
      fprintf(fp, "Pkilled	   %d\n", ch->pcdata->pkilled);

      fprintf(fp, "Password     %s~\n", ch->pcdata->pwd);
      fprintf(fp, "Bamfin       %s~\n", ch->pcdata->bamfin);
      fprintf(fp, "Bamfout      %s~\n", ch->pcdata->bamfout);
      fprintf(fp, "Roomenter    %s~\n", ch->pcdata->room_enter);
      fprintf(fp, "Roomexit     %s~\n", ch->pcdata->room_exit);
      fprintf(fp, "Title        %s~\n", ch->pcdata->title);
      fprintf(fp, "Immskll      %s~\n", ch->pcdata->staffskll);
      fprintf(fp, "Keep         %d\n", ch->pcdata->keep_vnum);
      fprintf(fp, "KeepHealerBought %d\n", ch->pcdata->keep_healer_bought);
      fprintf(fp, "KeepHealerVnum %d\n", ch->pcdata->keep_healer_vnum);
      /*
       * We add a '*' to preserve leading spaces... strip * on load
       */
      fprintf(fp, "Whoname      W%s~\n", ch->pcdata->who_name);
      fprintf(fp, "Monitor	   %d\n", ch->pcdata->monitor);
      fprintf(fp, "Host         %s~\n", ch->pcdata->host);
      fprintf(fp, "Failures     %d\n", ch->pcdata->failures);
      fprintf(fp, "LastLogin    %s~\n", (char *)ctime(&current_time));
      fprintf(fp, "HiCol	   %c~\n", ch->pcdata->hicol);
      fprintf(fp, "DimCol	   %c~\n", ch->pcdata->dimcol);
      fprintf(fp, "TermRows    %i\n", ch->pcdata->term_rows);
      fprintf(fp, "TermColumns    %i\n", ch->pcdata->term_columns);
      fprintf(fp, "Email   %s~\n", ch->pcdata->email_address);
      fprintf(fp, "EmailValid    %i\n", ch->pcdata->valid_email);
      for (cnt = 0; cnt < MAX_ALIASES; cnt++)
      {
         fprintf(fp, "Alias_Name%d %s~\n", cnt, ch->pcdata->alias_name[cnt]);
         fprintf(fp, "Alias%d      %s~\n", cnt, ch->pcdata->alias[cnt]);
      }

      fprintf(fp, "colors\n");
      for (foo = 0; foo < MAX_COLOR; foo++)
         fprintf(fp, "%d\n", ch->pcdata->color[foo]);

      fprintf(fp, "AttrPerm     %d %d %d %d %d\n", ch->pcdata->perm_str, ch->pcdata->perm_int,
              ch->pcdata->perm_wis, ch->pcdata->perm_dex, ch->pcdata->perm_con);

      fprintf(fp, "AttrMod      %d %d %d %d %d\n", 0, 0, 0, 0, 0);

      fprintf(fp, "AttrMax      %d %d %d %d %d\n", ch->pcdata->max_str, ch->pcdata->max_int,
              ch->pcdata->max_wis, ch->pcdata->max_dex, ch->pcdata->max_con);

      fprintf(fp, "Hasexpfix     %d\n", ch->pcdata->has_exp_fix);
      fprintf(fp, "Questpoints   %d\n", ch->quest_points);
      fprintf(fp, "InvasionPoints %d\n", ch->pcdata->invasion_points);
      fprintf(fp, "InvasionRewards %d %d %d\n", ch->pcdata->invasion_rewards[0],
              ch->pcdata->invasion_rewards[1], ch->pcdata->invasion_rewards[2]);
      fprintf(fp, "QuestPoints %d\n", ch->pcdata->post_quest_points);
      {
         int sb;
         fprintf(fp, "SuperbossKills");
         for (sb = 0; sb < MAX_SUPERBOSS; sb++)
            fprintf(fp, " %d", ch->pcdata->superboss_kills[sb] ? 1 : 0);
         fprintf(fp, "\n");
      }
      fprintf(fp, "PropDynCooldown %d\n", ch->pcdata->quest_dynamic_cooldown_until);
      {
         int i, k;
         for (i = 0; i < QUEST_MAX_QUESTS; i++)
         {
            QUEST_DATA *prop = &ch->pcdata->quests[i];
            fprintf(fp, "PropType%d    %d\n", i, prop->quest_type);
            fprintf(fp, "PropDone%d    %d\n", i, prop->quest_completed ? 1 : 0);
            fprintf(fp, "PropTargets%d %d\n", i, prop->quest_num_targets);
            for (k = 0; k < prop->quest_num_targets; k++)
               fprintf(fp, "PropT%d       %d %d\n", i, prop->quest_target_vnum[k],
                       prop->quest_target_done[k] ? 1 : 0);
            fprintf(fp, "PropKillNeed%d %d\n", i, prop->quest_kill_needed);
            fprintf(fp, "PropKillGot%d  %d\n", i, prop->quest_kill_count);
            fprintf(fp, "PropStaticId%d %d\n", i, prop->quest_template_id);
            fprintf(fp, "PropRewardGold%d %d\n", i, prop->quest_reward_gold);
            fprintf(fp, "PropRewardQp%d %d\n", i, prop->quest_reward_qp);
            fprintf(fp, "PropRewardItem%d %d %d\n", i, prop->quest_reward_item_vnum,
                    prop->quest_reward_item_count);
            fprintf(fp, "PropStaticOfferer%d %d\n", i, prop->quest_offerer_vnum);
            fprintf(fp, "PropCartArea%d %d\n", i, prop->quest_cartography_area_num);
            fprintf(fp, "PropCartRooms%d %d\n", i, prop->quest_cartography_room_count);
            fprintf(fp, "PropCartSeen%d %d\n", i, prop->quest_cartography_explored_count);
            {
               int b;
               fprintf(fp, "PropCartBits%d", i);
               for (b = 0; b < QUEST_CARTOGRAPHY_BYTES; b++)
                  fprintf(fp, " %u", (unsigned int)prop->quest_cartography_bits[b]);
               fprintf(fp, "\n");
            }
         }
         fprintf(fp, "PropStaticDoneCap %d\n", QUEST_MAX_TEMPLATES);
         for (i = 0; i < QUEST_MAX_TEMPLATES; i++)
            if (ch->pcdata->completed_quests[i])
               fprintf(fp, "PropStaticDone %d\n", i);
      }
      fprintf(fp, "RecallVnum    %d\n", ch->pcdata->recall_vnum);
      fprintf(fp, "GainMana      %d\n", ch->pcdata->mana_from_gain);
      fprintf(fp, "GainHp        %d\n", ch->pcdata->hp_from_gain);
      fprintf(fp, "GainMove      %d\n", ch->pcdata->move_from_gain);
      fprintf(fp, "RulerRank    %d\n", ch->pcdata->ruler_rank);

      fprintf(fp, "Pagelen      %d\n", ch->pcdata->pagelen);
      fprintf(fp, "Pflags       %d\n", ch->pcdata->pflags);

      /* Weapon bond data */
      if (ch->pcdata->bond != NULL)
      {
         BOND_DATA *bond = ch->pcdata->bond;
         fprintf(fp, "BondVnum     %d\n", bond->base_obj_vnum);
         fprintf(fp, "BondLevel    %d\n", bond->base_level);
         fprintf(fp, "BondPoints   %d\n", bond->bond_points);
         fprintf(fp, "BondTrack    %d\n", bond->track);
         fprintf(fp, "BondRank     %d\n", bond->rank);
         fprintf(fp, "BondMastery  %d\n", bond->mastery ? 1 : 0);
         fprintf(fp, "BondName     %s~\n", bond->weapon_name ? bond->weapon_name : "");
         fprintf(fp, "BondShort    %s~\n", bond->weapon_short ? bond->weapon_short : "");
      }

      /* Public society data */
      if (ch->pcdata->pub_society > 0)
      {
         fprintf(fp, "PubSociety   %d\n", ch->pcdata->pub_society);
         fprintf(fp, "PubSocRank   %d\n", ch->pcdata->pub_society_rank);
         fprintf(fp, "PubSocTasks  %d\n", ch->pcdata->pub_society_tasks_done);
         fprintf(fp, "PubSocJoined %d\n", ch->pcdata->pub_society_joined);
         fprintf(fp, "PubSocLeft   %d\n", ch->pcdata->pub_society_left);
      }

      for (sn = 0; sn < MAX_SKILL; sn++)
      {
         if (skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0)
         {
            fprintf(fp, "Skill        %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn].name);
         }
      }
   }

   for (paf = ch->first_affect; paf != NULL; paf = paf->next)
   {
      fprintf(fp, "Affect %3d %3d %3d %3d %3d %10d\n", paf->type, paf->duration, paf->duration_type,
              paf->modifier, paf->location, paf->bitvector);
   }
   fprintf(fp, "End\n\n");
   return;
}

/*
 * alloc_char_for_login -- allocate and zero-initialise a CHAR_DATA/PC_DATA
 * for a freshly connecting player.  Sets d->character.
 * Called from both load_char_obj() (synchronous path) and
 * db_worker_poll_results() (async DB login path).
 */
void alloc_char_for_login(DESCRIPTOR_DATA *d, const char *name)
{
   static PC_DATA pcdata_zero;
   CHAR_DATA *ch;
   int cnt, foo;

   GET_FREE(ch, char_free);
   clear_char(ch);

   {
      GET_FREE(ch->pcdata, pcd_free);
      *ch->pcdata = pcdata_zero;
      ch->pcdata->reincarnate_race = -1;
      ch->pcdata->reincarnate_class = -1;

      d->character = ch;

      ch->pcdata->host = str_dup("Unknown!");
      ch->pcdata->lastlogin = str_dup("Unknown!");
      ch->pcdata->who_name = str_dup("off");
      ch->pcdata->pwd = str_dup("");
      ch->pcdata->bamfin = str_dup("");
      ch->pcdata->bamfout = str_dup("");
      ch->pcdata->room_enter = str_dup("");
      ch->pcdata->room_exit = str_dup("");
      ch->pcdata->title = str_dup("");
      ch->pcdata->staffskll = str_dup("");
      ch->pcdata->perm_str = 13;
      ch->pcdata->perm_int = 13;
      ch->pcdata->perm_wis = 13;
      ch->pcdata->perm_dex = 13;
      ch->pcdata->perm_con = 13;
      ch->pcdata->keep_vnum = 0;
      ch->pcdata->keep_healer_bought = 0;
      ch->pcdata->keep_healer_vnum = 0;
      for (int i = 0; i < MAX_CLASS; i++)
         ch->pcdata->reincarnations[i] = 0;
      for (int i = 0; i < MAX_REMORT; i++)
         ch->pcdata->reincarnations[MAX_CLASS + i] = 0;
      for (int i = 0; i < MAX_CLASS; i++)
         ch->pcdata->reincarnations[MAX_CLASS + MAX_REMORT + i] = 0;
      for (int i = 0; i < MAX_REINCARNATE; i++)
         ch->pcdata->reincarnation_data[i] = 0;
      ch->pcdata->pagelen = 20;
      ch->pcdata->pkills = 0;
      ch->pcdata->pkilled = 0;
      ch->pcdata->mkills = 0;
      ch->pcdata->mkilled = 0;
      ch->pcdata->pflags = 0;
      ch->pcdata->has_exp_fix = 0;
      ch->pcdata->recall_vnum = ROOM_VNUM_TEMPLE;
      ch->pcdata->mana_from_gain = -1;
      ch->pcdata->hp_from_gain = -1;
      ch->pcdata->move_from_gain = -1;
      ch->pcdata->hicol = 'y';
      ch->pcdata->dimcol = 'b';
      ch->pcdata->ruler_rank = 0;
      for (foo = 0; foo < 5; foo++)
         ch->pcdata->pedit_string[foo] = str_dup("none");
      ch->pcdata->pedit_state = str_dup("none");
      ch->pcdata->term_rows = 25;
      ch->pcdata->term_columns = 80;
      ch->pcdata->valid_email = FALSE;
      ch->pcdata->email_address = str_dup("not set");
      ch->quest_points = 0;
      ch->pcdata->invasion_points = 0;
      ch->pcdata->invasion_rewards[0] = 0;
      ch->pcdata->invasion_rewards[1] = 0;
      ch->pcdata->invasion_rewards[2] = 0;
      ch->pcdata->post_quest_points = 0;
      ch->pcdata->quest_dynamic_cooldown_until = 0;
      for (foo = 0; foo < QUEST_MAX_QUESTS; foo++)
      {
         memset(&ch->pcdata->quests[foo], 0, sizeof(QUEST_DATA));
         ch->pcdata->quests[foo].quest_template_id = -1;
         ch->pcdata->quests[foo].quest_offerer_vnum = 0;
      }
      for (foo = 0; foo < QUEST_MAX_TEMPLATES; foo++)
         ch->pcdata->completed_quests[foo] = FALSE;
      for (foo = 0; foo < MAX_SUPERBOSS; foo++)
         ch->pcdata->superboss_kills[foo] = FALSE;
      for (foo = 0; foo < 4; foo++)
      {
         ch->mortal_class[foo] = -1;
         ch->mortal_level[foo] = 0;
      }
      for (foo = 0; foo < 2; foo++)
      {
         ch->remort_class[foo] = -1;
         ch->remort_level[foo] = 0;
      }
      ch->adept_class = -1;
      for (foo = CLASS_DRU; foo <= CLASS_HIE; foo++)
         ch->pcdata->reincarnations[foo] = 0;
      ch->overgrowth = 0;
      ch->pcdata->reincarnate_race = -1;
      ch->pcdata->reincarnate_class = -1;
      ch->pcdata->reincarnate_confirm = FALSE;
      ch->adept_level = 0;
      for (cnt = 0; cnt < MAX_ALIASES; cnt++)
      {
         ch->pcdata->alias_name[cnt] = str_dup("<none>");
         ch->pcdata->alias[cnt] = str_dup("<none>");
      }
   }

   ch->stunTimer = 0;
   ch->first_shield = NULL;
   ch->last_shield = NULL;
   ch->switched = FALSE;
   ch->old_body = NULL;

   ch->deaf = 0;
   ch->desc = d;
   if (ch->name != NULL)
      free_string(ch->name);
   ch->name = str_dup(name);
   ch->prompt = str_dup("");
   ch->old_prompt = str_dup("");
   ch->prompt = str_dup("TYPE HELP PROMPT ");
   ch->last_note = 0;
   ch->config = CONFIG_BLANK | CONFIG_COMBINE | CONFIG_PROMPT | CONFIG_MAPPER;
   ch->sex = SEX_NEUTRAL;
   ch->login_sex = -1;
   ch->current_brand = NULL;
   ch->stance = 0;
   ch->carry_weight = 0.0;
   ch->carry_number = 0;
   ch->ngroup = NULL;
}

/*
 * Load a char and inventory into a new ch structure.
 */

bool load_char_obj(DESCRIPTOR_DATA *d, char *name, bool system_call)
{
   char strsave[MAX_INPUT_LENGTH];
   char tempstrsave[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   FILE *fp;
   bool found;
   CHAR_DATA *ch;

   init_changed_vnum_hash();

   if (d == NULL)
   {
      bug("Load_char_obj: NULL descriptor.", 0);
      return FALSE;
   }

   alloc_char_for_login(d, name);
   ch = d->character;

   found = FALSE;
   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }

   {
      char *raw = db_worker_fetch_player_raw_save(name);
      if (raw)
      {
         found = load_char_from_raw(ch, raw);
         free(raw);
         return found;
      }
      /* raw == NULL: player not in DB yet — fall through to flat file. */
   }

   /*
    * parsed player file directories by Yaz of 4th Realm
    */
   /*
    * decompress if .gz file exists - Thx Alander
    */

#if !defined(machintosh) && !defined(MSDOS)
   strcpy(buf, name);
   sprintf(strsave, "%s%s%s%s", PLAYER_DIR, initial(buf), "/", cap_nocol(buf));
#else
   strcpy(buf, name);

   sprintf(strsave, "%s%s", PLAYER_DIR, cap_nocol(buf));
#endif

#if !defined(macintosh) && !defined(MSDOS)
   {
      size_t strsave_len = strlen(strsave);
      if (strsave_len + sizeof(".gz") <= sizeof(tempstrsave))
      {
         memcpy(tempstrsave, strsave, strsave_len + 1);
         memcpy(tempstrsave + strsave_len, ".gz", sizeof(".gz"));
      }
      else
         tempstrsave[0] = '\0';
   }
   if ((fp = fopen(tempstrsave, "r")) != NULL)
   {
      char buf[MAX_STRING_LENGTH];
      fclose(fp);
      fp = NULL;
      snprintf(buf, sizeof(buf), "gzip -dfq %s", tempstrsave);
      {
         int system_result = system(buf);
         (void)system_result;
      }
   }
#endif

   if ((fp = fopen(strsave, "r")) != NULL)
   {
      int iNest;

      for (iNest = 0; iNest < MAX_NEST; iNest++)
         rgObjNest[iNest] = NULL;

      found = TRUE;
      for (;;)
      {
         char letter;
         char *word;

         letter = fread_letter(fp);
         if (letter == '*')
         {
            fread_to_eol(fp);
            continue;
         }

         if (letter != '#')
         {
            monitor_chan("Load_char_obj: # not found.", MONITOR_BAD);
            break;
         }

         word = fread_word(fp);
         if (!str_cmp(word, "PLAYER"))
            fread_char(ch, fp);
         else if (!str_cmp(word, "OBJECT"))
            fread_obj(ch, fp);
         else if (!str_cmp(word, "END"))
            break;
         else
         {
            monitor_chan("Load_char_obj: bad section.", MONITOR_BAD);
            break;
         }
      }
      if (fp != NULL)
      {
         fclose(fp);
         fp = NULL;
      }
   }

   if (found && (ch->race < 0 || ch->race >= MAX_RACE))
   {
      char buf[MAX_STRING_LENGTH];
      snprintf(buf, sizeof(buf), "load_char_obj: %s had invalid race %d, resetting to Human.",
               ch->name ? ch->name : "(unknown)", ch->race);
      monitor_chan(buf, MONITOR_BAD);
      ch->race = 0;
   }

   return found;
}

/*
 * load_char_from_raw -- parse a raw_save text blob into an already-allocated
 * and initialised CHAR_DATA.  Returns TRUE if the parse succeeded.
 * Used by the async DB login path (db_worker_poll_results) and Phase 3's
 * synchronous DB load path.
 */
bool load_char_from_raw(CHAR_DATA *ch, const char *raw)
{
   int iNest;
   FILE *fp;
   bool found = FALSE;

   if (!raw || !raw[0])
      return FALSE;

   fp = fmemopen((void *)raw, strlen(raw), "r");
   if (!fp)
      return FALSE;

   for (iNest = 0; iNest < MAX_NEST; iNest++)
      rgObjNest[iNest] = NULL;

   found = TRUE;
   for (;;)
   {
      char letter;
      char *word;

      letter = fread_letter(fp);
      if (letter == '*')
      {
         fread_to_eol(fp);
         continue;
      }

      if (letter != '#')
      {
         monitor_chan("load_char_from_raw: # not found.", MONITOR_BAD);
         found = FALSE;
         break;
      }

      word = fread_word(fp);
      if (!str_cmp(word, "PLAYER"))
         fread_char(ch, fp);
      else if (!str_cmp(word, "OBJECT"))
         fread_obj(ch, fp);
      else if (!str_cmp(word, "END"))
         break;
      else
      {
         monitor_chan("load_char_from_raw: bad section.", MONITOR_BAD);
         found = FALSE;
         break;
      }
   }
   fclose(fp);

   if (found && (ch->race < 0 || ch->race >= MAX_RACE))
   {
      char buf[MAX_STRING_LENGTH];
      snprintf(buf, sizeof(buf), "load_char_from_raw: %s had invalid race %d, resetting to Human.",
               ch->name ? ch->name : "(unknown)", ch->race);
      monitor_chan(buf, MONITOR_BAD);
      ch->race = 0;
   }

   return found;
}

/*
 * Read in a char.
 */

static void apply_prop_static_done_cap(PC_DATA *pcdata, int saved_cap)
{
   int i;
   int start = URANGE(0, saved_cap, QUEST_MAX_TEMPLATES);

   for (i = start; i < QUEST_MAX_TEMPLATES; i++)
      pcdata->completed_quests[i] = FALSE;
}

#ifdef UNIT_TEST_SAVE
int quest_static_done_cap_true_count_for_test(int saved_cap)
{
   PC_DATA pc;
   int i;
   int count = 0;

   memset(&pc, 0, sizeof(pc));
   for (i = 0; i < QUEST_MAX_TEMPLATES; i++)
      pc.completed_quests[i] = TRUE;

   apply_prop_static_done_cap(&pc, saved_cap);

   for (i = 0; i < QUEST_MAX_TEMPLATES; i++)
      if (pc.completed_quests[i])
         count++;

   return count;
}
#endif

/*
 * Migrate an old class_level[] array (revision <= 15) into the new explicit
 * mortal_class/remort_class/adept_class fields.
 */
static void migrate_legacy_class_levels(CHAR_DATA *ch, const int *cl)
{
   int i, slot;

   /* Mortal classes: prime first, then remaining in descending level order. */
   slot = 0;
   /* Prime class is canonical; put it first regardless of level. */
   if (ch->class >= 0 && cl[ch->class] >= 0 && slot < 4)
   {
      ch->mortal_class[slot] = ch->class;
      ch->mortal_level[slot] = cl[ch->class];
      slot++;
   }
   /* Collect non-prime mortal classes that have a recorded level. */
   {
      int cand_class[MAX_TOTAL_CLASS];
      int cand_level[MAX_TOTAL_CLASS];
      int ncand = 0, a, b, tmp;
      for (i = 0; i < MAX_TOTAL_CLASS; i++)
      {
         if (!IS_MORTAL_CLASS(i) || i == ch->class)
            continue;
         if (cl[i] >= 0)
         {
            cand_class[ncand] = i;
            cand_level[ncand] = cl[i];
            ncand++;
         }
      }
      /* Sort descending by level (insertion sort; small array). */
      for (a = 1; a < ncand; a++)
      {
         for (b = a; b > 0 && cand_level[b] > cand_level[b - 1]; b--)
         {
            tmp = cand_level[b];
            cand_level[b] = cand_level[b - 1];
            cand_level[b - 1] = tmp;
            tmp = cand_class[b];
            cand_class[b] = cand_class[b - 1];
            cand_class[b - 1] = tmp;
         }
      }
      for (a = 0; a < ncand && slot < 4; a++, slot++)
      {
         ch->mortal_class[slot] = cand_class[a];
         ch->mortal_level[slot] = cand_level[a];
      }
   }

   /* Remort classes: up to 2, in index order. */
   slot = 0;
   for (i = 0; i < MAX_TOTAL_CLASS && slot < 2; i++)
   {
      if (!IS_REMORT_CLASS(i))
         continue;
      if (cl[i] > 0)
      {
         ch->remort_class[slot] = i;
         ch->remort_level[slot] = cl[i];
         slot++;
      }
   }

   /* Adept class: whichever one has a positive level. */
   ch->adept_class = -1;
   for (i = 0; i < MAX_TOTAL_CLASS; i++)
   {
      if (!IS_ADEPT_CLASS(i))
         continue;
      if (cl[i] > 0)
      {
         ch->adept_class = i;
         /* adept_level already set from "Adeptlevel" keyword */
         break;
      }
   }
}

void fread_char(CHAR_DATA *ch, FILE *fp)
{
   char buf[MAX_STRING_LENGTH];
   char *word;
   bool fMatch;
   int cnt;
   int legacy_class_level[MAX_TOTAL_CLASS];
   bool legacy_loaded = FALSE;

   for (cnt = 0; cnt < MAX_TOTAL_CLASS; cnt++)
      legacy_class_level[cnt] = -1;

   /* Initialise new class fields so they are safe if keywords are absent. */
   for (cnt = 0; cnt < 4; cnt++)
   {
      ch->mortal_class[cnt] = -1;
      ch->mortal_level[cnt] = 0;
   }
   for (cnt = 0; cnt < 2; cnt++)
   {
      ch->remort_class[cnt] = -1;
      ch->remort_level[cnt] = 0;
   }
   ch->adept_class = -1;

   ch->balance = 0;

   for (;;)
   {
      word = feof(fp) ? "End" : fread_word(fp);
      fMatch = FALSE;

      switch (UPPER(word[0]))
      {
      case '*':
         fMatch = TRUE;
         fread_to_eol(fp);
         break;

      case 'A':
         KEY("Act", ch->act, fread_number(fp));
         if (!str_cmp(word, "AdeptClass"))
         {
            ch->adept_class = fread_number(fp);
            fMatch = TRUE;
            break;
         }
         if (!str_cmp(word, "Adept"))
         {
            /* Legacy rev <= 15: adept class levels by index. */
            for (cnt = CLASS_GMA; cnt < CLASS_GMA + MAX_CLASS; cnt++)
               legacy_class_level[cnt] = fread_number(fp);
            legacy_loaded = TRUE;
            fMatch = TRUE;
            break;
         }
         KEY("AffectedBy", ch->affected_by, fread_number(fp));
         KEY("Alignment", ch->alignment, fread_number(fp));
         KEY("Armor", ch->armor, fread_number(fp));
         KEY("Adeptlevel", ch->adept_level, fread_number(fp));
         if (!IS_NPC(ch))
         {
            /* Alias_NameN — prefix match so MAX_ALIASES can be bumped freely */
            if (!str_prefix("Alias_Name", word))
            {
               int idx = atoi(word + 10);
               if (idx >= 0 && idx < MAX_ALIASES)
               {
                  if (ch->pcdata->alias_name[idx] != NULL)
                     free_string(ch->pcdata->alias_name[idx]);
                  ch->pcdata->alias_name[idx] = fread_string(fp);
                  fMatch = TRUE;
                  break;
               }
            }
            /* AliasN (not Alias_Name) */
            if (!str_prefix("Alias", word) && word[5] != '_')
            {
               int idx = atoi(word + 5);
               if (idx >= 0 && idx < MAX_ALIASES)
               {
                  if (ch->pcdata->alias[idx] != NULL)
                     free_string(ch->pcdata->alias[idx]);
                  ch->pcdata->alias[idx] = fread_string(fp);
                  fMatch = TRUE;
                  break;
               }
            }
         }

         if (!str_cmp(word, "Adeptreincarnations"))
         {
            for (cnt = 0; cnt < MAX_CLASS; cnt++)
               ch->pcdata->reincarnations[MAX_CLASS + MAX_REMORT + cnt] = fread_number(fp);
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "Affect"))
         {
            AFFECT_DATA *paf;

            GET_FREE(paf, affect_free);
            paf->type = fread_number(fp);
            paf->duration = fread_number(fp);
            paf->duration_type = fread_number(fp);
            paf->modifier = fread_number(fp);
            paf->location = fread_number(fp);
            paf->bitvector = fread_number(fp);
            paf->caster = NULL;
            if (paf->type != -1)
               LINK(paf, ch->first_saved_aff, ch->last_saved_aff, next, prev);
            else
               PUT_FREE(paf, affect_free);
            fMatch = TRUE;
            break;
         }

         if (!IS_NPC(ch))
         {
            if (!str_cmp(word, "AttrMod"))
            {
               ch->pcdata->mod_str = fread_number(fp);
               ch->pcdata->mod_int = fread_number(fp);
               ch->pcdata->mod_wis = fread_number(fp);
               ch->pcdata->mod_dex = fread_number(fp);
               ch->pcdata->mod_con = fread_number(fp);
               fMatch = TRUE;
               break;
            }

            if (!str_cmp(word, "AttrMax"))
            {
               ch->pcdata->max_str = fread_number(fp);
               ch->pcdata->max_int = fread_number(fp);
               ch->pcdata->max_wis = fread_number(fp);
               ch->pcdata->max_dex = fread_number(fp);
               ch->pcdata->max_con = fread_number(fp);
               fMatch = TRUE;
               break;
            }

            if (!str_cmp(word, "AttrPerm"))
            {
               ch->pcdata->perm_str = fread_number(fp);
               ch->pcdata->perm_int = fread_number(fp);
               ch->pcdata->perm_wis = fread_number(fp);
               ch->pcdata->perm_dex = fread_number(fp);
               ch->pcdata->perm_con = fread_number(fp);
               fMatch = TRUE;
               break;
            }
         }
         break;

      case 'B':
         KEY("Balance", ch->balance, fread_number(fp));
         if (!IS_NPC(ch))
         {
            SKEY("Bamfin", ch->pcdata->bamfin, fread_string(fp));
            SKEY("Bamfout", ch->pcdata->bamfout, fread_string(fp));

            if (!str_cmp(word, "BondVnum"))
            {
               if (ch->pcdata->bond == NULL)
                  ch->pcdata->bond = (BOND_DATA *)calloc(1, sizeof(BOND_DATA));
               ch->pcdata->bond->base_obj_vnum = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "BondLevel"))
            {
               if (ch->pcdata->bond == NULL)
                  ch->pcdata->bond = (BOND_DATA *)calloc(1, sizeof(BOND_DATA));
               ch->pcdata->bond->base_level = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "BondPoints"))
            {
               if (ch->pcdata->bond == NULL)
                  ch->pcdata->bond = (BOND_DATA *)calloc(1, sizeof(BOND_DATA));
               ch->pcdata->bond->bond_points = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "BondTrack"))
            {
               if (ch->pcdata->bond == NULL)
                  ch->pcdata->bond = (BOND_DATA *)calloc(1, sizeof(BOND_DATA));
               ch->pcdata->bond->track = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "BondRank"))
            {
               if (ch->pcdata->bond == NULL)
                  ch->pcdata->bond = (BOND_DATA *)calloc(1, sizeof(BOND_DATA));
               ch->pcdata->bond->rank = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "BondMastery"))
            {
               if (ch->pcdata->bond == NULL)
                  ch->pcdata->bond = (BOND_DATA *)calloc(1, sizeof(BOND_DATA));
               ch->pcdata->bond->mastery = (fread_number(fp) != 0);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "BondName"))
            {
               if (ch->pcdata->bond == NULL)
                  ch->pcdata->bond = (BOND_DATA *)calloc(1, sizeof(BOND_DATA));
               if (ch->pcdata->bond->weapon_name != NULL)
                  free_string(ch->pcdata->bond->weapon_name);
               ch->pcdata->bond->weapon_name = fread_string(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "BondShort"))
            {
               if (ch->pcdata->bond == NULL)
                  ch->pcdata->bond = (BOND_DATA *)calloc(1, sizeof(BOND_DATA));
               if (ch->pcdata->bond->weapon_short != NULL)
                  free_string(ch->pcdata->bond->weapon_short);
               ch->pcdata->bond->weapon_short = fread_string(fp);
               fMatch = TRUE;
               break;
            }
         }
         break;

      case 'C':
         if (!IS_NPC(ch))
         {
            KEY("Clan", ch->pcdata->clan, fread_number(fp));
         }
         KEY("Class", ch->class, fread_number(fp));
         KEY("Config", ch->config, fread_number(fp));
         if (!str_cmp(word, "colors") && !IS_NPC(ch))
         {
            int foo;
            for (foo = 0; foo < MAX_COLOR; foo++)
               ch->pcdata->color[foo] = fread_number(fp);
            fMatch = TRUE;
            break;
         }

         break;

      case 'D':
         KEY("Damroll", ch->damroll, fread_number(fp));
         KEY("Deaf", ch->deaf, fread_number(fp));
         SKEY("Description", ch->description, fread_string(fp));

         if (!str_cmp(word, "DimCol"))
         {
            char *temp;
            temp = fread_string(fp);
            ch->pcdata->dimcol = temp[0];
            /*              fread_to_eol( fp );   */
            free_string(temp);
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "Druidlevels"))
         {
            /* Legacy rev <= 15. */
            for (cnt = CLASS_DRU; cnt <= CLASS_HIE; cnt++)
               legacy_class_level[cnt] = fread_number(fp);
            legacy_loaded = TRUE;
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "Druidreinc"))
         {
            for (cnt = CLASS_DRU; cnt <= CLASS_HIE; cnt++)
               ch->pcdata->reincarnations[cnt] = fread_number(fp);
            fMatch = TRUE;
            break;
         }

         break;

      case 'E':
         if (!str_cmp(word, "End"))
         {
            if (ch->login_sex < 0)
               ch->login_sex = ch->sex;
            if (legacy_loaded)
               migrate_legacy_class_levels(ch, legacy_class_level);
            /* Keep ch->class in sync with the prime mortal class. */
            if (!IS_NPC(ch) && ch->mortal_class[0] >= 0)
               ch->class = (sh_int)ch->mortal_class[0];
            return;
         }
         KEY("Exp", ch->exp, fread_number(fp));
         KEY("EmailValid", ch->pcdata->valid_email, fread_number(fp));
         SKEY("Email", ch->pcdata->email_address, fread_string(fp));
         break;

      case 'F':
         if (!IS_NPC(ch))
         {
            KEY("Failures", ch->pcdata->failures, fread_number(fp));
         }
         break;

      case 'G':
         KEY("GainMana", ch->pcdata->mana_from_gain, fread_number(fp));
         KEY("GainHp", ch->pcdata->hp_from_gain, fread_number(fp));
         KEY("GainMove", ch->pcdata->move_from_gain, fread_number(fp));
         KEY("Gold", ch->gold, fread_number(fp));
         KEY("Generation", ch->pcdata->generation, fread_number(fp));
         break;

      case 'H':
         KEY("Hitroll", ch->hitroll, fread_number(fp));
         /*	    if (!IS_NPC(ch))
              {  */
         SKEY("Host", ch->pcdata->host, fread_string(fp));
         /*            }  */
         KEY("Hasexpfix", ch->pcdata->has_exp_fix, fread_number(fp));

         if (!str_cmp(word, "HiCol"))
         {
            char *temp;
            temp = fread_string(fp);
            ch->pcdata->hicol = temp[0];
            /*              fread_to_eol( fp );   */
            free_string(temp);
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "HpManaMove"))
         {
            ch->hit = fread_number(fp);
            ch->hit = fread_number(fp);
            ch->mana = fread_number(fp);
            ch->max_mana = fread_number(fp);
            ch->move = fread_number(fp);
            ch->max_move = fread_number(fp);
            fMatch = TRUE;
            break;
         }
         break;

      case 'I':
         if (!IS_NPC(ch))
         {
            KEY("InvasionPoints", ch->pcdata->invasion_points, fread_number(fp));
            if (!str_cmp(word, "InvasionRewards"))
            {
               ch->pcdata->invasion_rewards[0] = fread_number(fp);
               ch->pcdata->invasion_rewards[1] = fread_number(fp);
               ch->pcdata->invasion_rewards[2] = fread_number(fp);
               fMatch = TRUE;
               break;
            }
         }
         SKEY("Immskll", ch->pcdata->staffskll, fread_string(fp));
         KEY("Incog", ch->incog, fread_number(fp));
         KEY("Invis", ch->invis, fread_number(fp));
         if (!IS_NPC(ch))
         {
            if (!str_cmp(word, "Index"))
            {
               int i;
               for (i = 0; i < MAX_CLASS; i++)
                  fread_number(fp); /* index[] removed; discard legacy data */
               fMatch = TRUE;
               break;
            }
         }
         break;

      case 'K':
         KEY("Keep", ch->pcdata->keep_vnum, fread_number(fp));
         KEY("KeepHealerBought", ch->pcdata->keep_healer_bought, fread_number(fp));
         KEY("KeepHealerVnum", ch->pcdata->keep_healer_vnum, fread_number(fp));
      case 'L':

         KEY("Level", ch->level, fread_number(fp));
         SKEY("LongDescr", ch->long_descr, fread_string(fp));
         KEY("LoginSex", ch->login_sex, fread_number(fp));

         if (!IS_NPC(ch))
         {
            SKEY("LastLogin", ch->pcdata->lastlogin, fread_string(fp));
         }
         break;

      case 'M':
         if (!IS_NPC(ch))
         {
            KEY("Mkills", ch->pcdata->mkills, fread_number(fp));
            KEY("Mkilled", ch->pcdata->mkilled, fread_number(fp));
            KEY("Monitor", ch->pcdata->monitor, fread_number(fp));
         }
         if (!str_cmp(word, "MortalClass"))
         {
            for (cnt = 0; cnt < 4; cnt++)
               ch->mortal_class[cnt] = fread_number(fp);
            fMatch = TRUE;
            break;
         }
         if (!str_cmp(word, "MortalLevel"))
         {
            for (cnt = 0; cnt < 4; cnt++)
               ch->mortal_level[cnt] = fread_number(fp);
            fMatch = TRUE;
            break;
         }
         if (!str_cmp(word, "m/c"))
         {
            /* Legacy rev <= 15: 6 standard mortal class levels. */
            for (cnt = 0; cnt < MAX_CLASS; cnt++)
               legacy_class_level[cnt] = fread_number(fp);
            legacy_loaded = TRUE;
            fMatch = TRUE;
         }
         break;

      case 'N':
         if (!str_cmp(word, "Name"))
         {
            /*
             * Name already set externally.
             */
            fread_to_eol(fp);
            fMatch = TRUE;
            break;
         }
         KEY("Note", ch->last_note, fread_number(fp));
         break;

      case 'O':
         if (!str_cmp(word, "Order") && !IS_NPC(ch))
         {
            int i;
            for (i = 0; i < MAX_CLASS; i++)
               fread_number(fp); /* order[] removed; discard legacy data */
            fMatch = TRUE;
            break;
         }
         KEY("Overgrowth", ch->overgrowth, fread_number(fp));
         break;

      case 'P':
         if (!IS_NPC(ch))
         {
            KEY("Pagelen", ch->pcdata->pagelen, fread_number(fp));
            SKEY("Password", ch->pcdata->pwd, fread_string(fp));
            KEY("Pkills", ch->pcdata->pkills, fread_number(fp));
            KEY("Pkilled", ch->pcdata->pkilled, fread_number(fp));
            KEY("Pflags", ch->pcdata->pflags, fread_number(fp));
            KEY("PubSociety", ch->pcdata->pub_society, fread_number(fp));
            KEY("PubSocRank", ch->pcdata->pub_society_rank, fread_number(fp));
            KEY("PubSocTasks", ch->pcdata->pub_society_tasks_done, fread_number(fp));
            KEY("PubSocJoined", ch->pcdata->pub_society_joined, fread_number(fp));
            KEY("PubSocLeft", ch->pcdata->pub_society_left, fread_number(fp));
            KEY("QuestPoints", ch->pcdata->post_quest_points, fread_number(fp));
            KEY("PropDynCooldown", ch->pcdata->quest_dynamic_cooldown_until, fread_number(fp));
         }
         KEY("Played", ch->played, fread_number(fp));
         KEY("Position", ch->position, fread_number(fp));
         KEY("Practice", ch->practice, fread_number(fp));
         SKEY("Prompt", ch->prompt, fread_string(fp));
         if (!IS_NPC(ch))
         {
            int quest_i;
            if (sscanf(word, "PropType%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_type = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropDone%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_completed = (fread_number(fp) != 0);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropTargets%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_num_targets = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropT%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               int _k, _slot = -1;
               for (_k = 0; _k < QUEST_MAX_TARGETS; _k++)
                  if (ch->pcdata->quests[quest_i].quest_target_vnum[_k] == 0)
                  {
                     _slot = _k;
                     break;
                  }
               if (_slot >= 0)
               {
                  ch->pcdata->quests[quest_i].quest_target_vnum[_slot] = fread_number(fp);
                  ch->pcdata->quests[quest_i].quest_target_done[_slot] = (fread_number(fp) != 0);
               }
               else
               {
                  fread_number(fp);
                  fread_number(fp);
               }
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropKillNeed%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_kill_needed = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropKillGot%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_kill_count = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropStaticId%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_template_id = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropRewardGold%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_reward_gold = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropRewardQp%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_reward_qp = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropRewardItem%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_reward_item_vnum = fread_number(fp);
               ch->pcdata->quests[quest_i].quest_reward_item_count = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropStaticOfferer%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_offerer_vnum = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropCartArea%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_cartography_area_num = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropCartRooms%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_cartography_room_count = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropCartSeen%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               ch->pcdata->quests[quest_i].quest_cartography_explored_count = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (sscanf(word, "PropCartBits%d", &quest_i) == 1 && quest_i >= 0 &&
                quest_i < QUEST_MAX_QUESTS)
            {
               int b;
               for (b = 0; b < QUEST_CARTOGRAPHY_BYTES; b++)
                  ch->pcdata->quests[quest_i].quest_cartography_bits[b] =
                      (unsigned char)fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "PropStaticDoneCap"))
            {
               apply_prop_static_done_cap(ch->pcdata, fread_number(fp));
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "PropStaticDone"))
            {
               int done_id = fread_number(fp);
               if (done_id >= 0 && done_id < QUEST_MAX_TEMPLATES)
                  ch->pcdata->completed_quests[done_id] = TRUE;
               fMatch = TRUE;
               break;
            }

            /* Backward-compatible load from older single-quest keys. */
            if (!str_cmp(word, "PropType"))
            {
               ch->pcdata->quests[0].quest_type = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "PropDone"))
            {
               ch->pcdata->quests[0].quest_completed = (fread_number(fp) != 0);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "PropTargets"))
            {
               ch->pcdata->quests[0].quest_num_targets = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "PropT"))
            {
               int _k, _slot = -1;
               for (_k = 0; _k < QUEST_MAX_TARGETS; _k++)
                  if (ch->pcdata->quests[0].quest_target_vnum[_k] == 0)
                  {
                     _slot = _k;
                     break;
                  }
               if (_slot >= 0)
               {
                  ch->pcdata->quests[0].quest_target_vnum[_slot] = fread_number(fp);
                  ch->pcdata->quests[0].quest_target_done[_slot] = (fread_number(fp) != 0);
               }
               else
               {
                  fread_number(fp);
                  fread_number(fp);
               }
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "PropKillNeed"))
            {
               ch->pcdata->quests[0].quest_kill_needed = fread_number(fp);
               fMatch = TRUE;
               break;
            }
            if (!str_cmp(word, "PropKillGot"))
            {
               ch->pcdata->quests[0].quest_kill_count = fread_number(fp);
               fMatch = TRUE;
               break;
            }
         }

         break;

      case 'Q':
         KEY("Questpoints", ch->quest_points, fread_number(fp));
         break;

      case 'R':
         KEY("Race", ch->race, fread_number(fp));
         KEY("Revision", cur_revision, fread_number(fp));
         SKEY("Roomenter", ch->pcdata->room_enter, fread_string(fp));
         SKEY("Roomexit", ch->pcdata->room_exit, fread_string(fp));
         KEY("RulerRank", ch->pcdata->ruler_rank, fread_number(fp));

         if (!str_cmp(word, "RemortClass"))
         {
            for (cnt = 0; cnt < 2; cnt++)
               ch->remort_class[cnt] = fread_number(fp);
            fMatch = TRUE;
            break;
         }
         if (!str_cmp(word, "RemortLevel"))
         {
            for (cnt = 0; cnt < 2; cnt++)
               ch->remort_level[cnt] = fread_number(fp);
            fMatch = TRUE;
            break;
         }
         if (!str_cmp(word, "Remort"))
         {
            /* Legacy rev <= 15: 12 standard remort class levels. */
            for (cnt = CLASS_SOR; cnt < CLASS_SOR + MAX_REMORT; cnt++)
               legacy_class_level[cnt] = fread_number(fp);
            legacy_loaded = TRUE;
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "Reinc_data"))
         {
            for (cnt = 0; cnt < MAX_REINCARNATE; cnt++)
               ch->pcdata->reincarnation_data[cnt] = fread_number(fp);
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "Reincarnations"))
         {
            for (cnt = 0; cnt < MAX_CLASS; cnt++)
               ch->pcdata->reincarnations[cnt] = fread_number(fp);
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "Remortreincarnations"))
         {
            for (cnt = 0; cnt < MAX_REMORT; cnt++)
               ch->pcdata->reincarnations[MAX_CLASS + cnt] = fread_number(fp);
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "Room"))
         {
            ch->in_room = get_room_index(fread_number(fp));
            if (ch->in_room == NULL)
               ch->in_room = get_room_index(ROOM_VNUM_LIMBO);
            fMatch = TRUE;
            break;
         }
         if (!str_cmp(word, "RecallVnum"))
         {
            ch->pcdata->recall_vnum = fread_number(fp);
            if (ch->pcdata->recall_vnum == 3001 || ch->pcdata->recall_vnum == 3110)
               ch->pcdata->recall_vnum = ROOM_VNUM_TEMPLE;
            fMatch = TRUE;
            break;
         }

         break;

      case 'S':
         KEY("SavingThrow", ch->saving_throw, fread_number(fp));
         KEY("Sentence", ch->sentence, fread_number(fp));
         KEY("Sex", ch->sex, fread_number(fp));
         SKEY("ShortDescr", ch->short_descr, fread_string(fp));

         if (!str_cmp(word, "Skill") && !IS_NPC(ch))
         {
            int sn;
            int value;
            char *skill_word;
            value = fread_number(fp);
            skill_word = fread_word(fp);
            sn = skill_lookup(skill_word);
            if (sn < 0)
            {
               const char *new_name = skill_name_legacy(skill_word);
               if (new_name != NULL)
                  sn = skill_lookup(new_name);
            }
            if (sn < 0)
            {
               sprintf(log_buf, "Loading pfile %s, unknown skill %s.", ch->name, skill_word);
               monitor_chan(log_buf, MONITOR_BAD);
            }
            else
               ch->pcdata->learned[sn] = value;
            fMatch = TRUE;
         }

         if (!str_cmp(word, "SuperbossKills") && !IS_NPC(ch))
         {
            int sb;
            for (sb = 0; sb < MAX_SUPERBOSS; sb++)
               ch->pcdata->superboss_kills[sb] = fread_number(fp) ? TRUE : FALSE;
            fMatch = TRUE;
            break;
         }

         break;

      case 'T':
         KEY("Trust", ch->trust, fread_number(fp));
         KEY("TermRows", ch->pcdata->term_rows, fread_number(fp));
         KEY("TermColumns", ch->pcdata->term_columns, fread_number(fp));
         if (!str_cmp(word, "Title") && !IS_NPC(ch))
         {
            if (ch->pcdata->title != NULL)
               free_string(ch->pcdata->title);
            ch->pcdata->title = fread_string(fp);
            if (isalpha(ch->pcdata->title[0]) || isdigit(ch->pcdata->title[0]))
            {
               sprintf(buf, " %s", ch->pcdata->title);
               free_string(ch->pcdata->title);
               ch->pcdata->title = str_dup(buf);
            }
            fMatch = TRUE;
            break;
         }

         break;

      case 'V':
         if (!str_cmp(word, "Vnum") && IS_NPC(ch))
         {
            ch->pIndexData = get_mob_index(fread_number(fp));
            fMatch = TRUE;
            break;
         }

         break;

      case 'W':
         KEY("Wimpy", ch->wimpy, fread_number(fp));
         KEY("Wizbit", ch->wizbit, fread_number(fp));
         if (!str_cmp(word, "Whoname"))
         {
            if (ch->pcdata->who_name != NULL)
               free_string(ch->pcdata->who_name);
            ch->pcdata->who_name = fread_string(fp);
            sprintf(buf, "%s", ch->pcdata->who_name + 1);

            free_string(ch->pcdata->who_name);
            ch->pcdata->who_name = str_dup(buf);
            fMatch = TRUE;
            break;
         }

         break;
      }

      /*
       * Make sure old chars have this field - Kahn
       */
      if (!IS_NPC(ch))
      {
         if (!ch->pcdata->pagelen)
            ch->pcdata->pagelen = 20;
         if (!ch->prompt || *ch->prompt == '\0')
            ch->prompt = str_dup("<%h %m %mv> ");
      }

      /*
       * Why this: ??
       */
      /*
       * if ( ch->exp > 2000 )
       * ch->exp = 2000;
       */
      ch->long_descr_orig = str_dup(ch->long_descr);
      if (!fMatch)
      {
         sprintf(log_buf, "Loading in pfile :%s, no match for ( %s ).", ch->name, word);
         monitor_chan(log_buf, MONITOR_BAD);
         fread_to_eol(fp);
      }
   }
}
