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
#include "globals.h"
#include "special.h"
#ifdef HAVE_LIBPQ
#include "../db/db_worker.h"
#endif

/* Way this works:
   Mud reads in area files, stores details in data lists.
   Edit rooms, objects, resets.
   type savearea.
   Sets bool saving_area to true.
   Incrementally saves an area, using data lists.
*/

#define SAVEQUEUESIZE 50
#define NOT_SAVING 0
#define START_SAVING 1
#define AM_SAVING 2
#define BUILD_OK -1
#define BUILD_CANTSAVE 1
#define BUILD_TOOMANY 2

#define BUILD_SEC_AREA 1
#define BUILD_SEC_ROOMS 2
#define BUILD_SEC_MOBILES 3
#define BUILD_SEC_OBJECTS 4
#define BUILD_SEC_SHOPS 5
#define BUILD_SEC_RESETS 6
#define BUILD_SEC_SPECIALS 7
#define BUILD_SEC_SPEECH 8
#define BUILD_SEC_OBJFUNS 9 /* -S- Mod */
#define BUILD_SEC_END 10
#define AREA_VERSION 16

struct save_queue_type
{
   AREA_DATA *area;
   CHAR_DATA *ch;
   int loops;
} SaveQ[SAVEQUEUESIZE];

/* Semi-local vars. */
int saving_area = 0;

/* local */
int offset;
int ToBeSaved = 0;
int CurrentSaving = -1;
AREA_DATA *CurSaveArea = NULL;
CHAR_DATA *CurSaveChar = NULL;
int CurLoops = 1;
int Section;
BUILD_DATA_LIST *Pointer;
RESET_DATA *ResetPointer;
FILE *SaveFile;
FILE *Envy;
int AreasModified = 0;

/* Local functions */
/* void build_save(); proto in merc.h */
void build_save_area(void);
void build_save_mobs(void);
void build_save_objects(void);
void build_save_rooms(void);
void build_save_shops(void);
void build_save_resets(void);
void build_save_specs(void);
void build_save_speech(void);
void build_save_objfuns(void);
void build_save_end(void);
void vuild_save_flush(void);
/*  int convert(int lev); */
/* Convert levels from ack -> envy! */

/* int  convert(int lev)	*/
/*  { 		*/
/*   return( lev - ( lev/5 ) ); 	*/
/* }   */

void do_savearea(CHAR_DATA *ch, char *argument)
{
   AREA_DATA *SaveArea;
   int loops;
   /*     char first_arg[MAX_INPUT_LENGTH]; unused? */

   if (ch == NULL)
   {
      SaveArea = (AREA_DATA *)argument;
      loops = 10;
   }
   else
   {
      if (ch->in_room == NULL)
      {
         send_to_char("Do not know what room you are in!!, cannot save.\n", ch);
         return;
      }

      SaveArea = (ch->in_room)->area;
      if (SaveArea == NULL)
      {
         send_to_char("Do not know what area you are in!!, cannot save.\n", ch);
         return;
      }

      if (*argument != '\0')
      {
         loops = atoi(argument);
         if (loops < 1)
            loops = 1;
      }
      else
         loops = 1;
   }

#ifdef HAVE_LIBPQ
   {
      BUILD_DATA_LIST *pList;

      db_worker_save_area_meta(SaveArea);

      for (pList = SaveArea->first_area_room; pList != NULL; pList = pList->next)
         db_worker_save_room((ROOM_INDEX_DATA *)pList->data);

      for (pList = SaveArea->first_area_mobile; pList != NULL; pList = pList->next)
         db_worker_save_mob((MOB_INDEX_DATA *)pList->data);

      for (pList = SaveArea->first_area_object; pList != NULL; pList = pList->next)
         db_worker_save_obj((OBJ_INDEX_DATA *)pList->data);

      for (pList = SaveArea->first_area_shop; pList != NULL; pList = pList->next)
         db_worker_save_shop((SHOP_DATA *)pList->data);

      db_worker_save_resets(SaveArea);

      if (ch != NULL)
         send_to_char("Area saved to database.\n", ch);
   }
   return;
#endif

   if (ToBeSaved == CurrentSaving)
   {
      send_to_char("Too many areas in queue, please try later.\n", ch);
      return;
   }

   SaveQ[ToBeSaved].area = SaveArea;
   SaveQ[ToBeSaved].ch = ch;
   SaveQ[ToBeSaved].loops = loops;
   ToBeSaved = (ToBeSaved + 1) % SAVEQUEUESIZE;

   if (saving_area == NOT_SAVING)
      saving_area = START_SAVING;
   else
      send_to_char("Save is queued, please wait. \n", ch);

   build_save();
   return;
}

void build_save()
{
#ifdef HAVE_LIBPQ
   return; /* DB saves are enqueued directly; no file-based save state machine needed. */
#endif
   int a;
   char filename[255];
   char buf[MAX_STRING_LENGTH];

   for (a = 0; a < CurLoops && saving_area > 0; a++)
   {

      if (saving_area == START_SAVING)
      {
         CurrentSaving = (CurrentSaving + 1) % SAVEQUEUESIZE;
         CurSaveArea = SaveQ[CurrentSaving].area;
         CurSaveChar = SaveQ[CurrentSaving].ch;
         CurLoops = SaveQ[CurrentSaving].loops;
         send_to_char("Starting Save.\n", CurSaveChar);

         sprintf(filename, "%s.new", CurSaveArea->filename);
         SaveFile = fopen(filename, "w");
         if (SaveFile == NULL)
         {
            if (CurrentSaving == ToBeSaved)
               saving_area = NOT_SAVING;
            send_to_char("Can not open file for saving.\n", CurSaveChar);
            return;
         }
         /*
          * Open second file for saving in envy format
          */

         sprintf(buf, "Starting to save %s", CurSaveArea->filename);
         monitor_chan(buf, MONITOR_AREA_SAVING);

         Section = 1;
         offset = CurSaveArea->offset;
         saving_area = AM_SAVING;
         Pointer = NULL;
         ResetPointer = NULL;
      }

      switch (Section)
      {
      case BUILD_SEC_AREA:
         build_save_area();
         break;
      case BUILD_SEC_ROOMS:
         build_save_rooms();
         break;
      case BUILD_SEC_MOBILES:
         build_save_mobs();
         break;
      case BUILD_SEC_OBJECTS:
         build_save_objects();
         break;
      case BUILD_SEC_SHOPS:
         build_save_shops();
         break;
      case BUILD_SEC_RESETS:
         build_save_resets();
         break;
      case BUILD_SEC_SPECIALS:
         build_save_specs();
         break;
      case BUILD_SEC_SPEECH:
         build_save_speech();
         break;
      case BUILD_SEC_OBJFUNS:
         build_save_objfuns();
         break;
      case BUILD_SEC_END:
         build_save_end();
         break;
      }
   }
   return;
}

void build_save_area()
{
   fprintf(SaveFile, "#AREA\n");
   fprintf(SaveFile, "%s~\n", CurSaveArea->name);
   fprintf(SaveFile, "Q %i\n", AREA_VERSION);
   fprintf(SaveFile, "K %s~\n", CurSaveArea->keyword);
   fprintf(SaveFile, "L %s~\n", CurSaveArea->level_label);
   fprintf(SaveFile, "N %i\n", CurSaveArea->area_num);
   fprintf(SaveFile, "I %i %i\n", CurSaveArea->min_level, CurSaveArea->max_level);
   fprintf(SaveFile, "V %i %i\n", CurSaveArea->min_vnum, CurSaveArea->max_vnum);
   fprintf(SaveFile, "X %i\n", CurSaveArea->offset);
   fprintf(SaveFile, "F %i\n", CurSaveArea->reset_rate);
   fprintf(SaveFile, "U %s~\n", CurSaveArea->reset_msg);
   if (CurSaveArea->owner != NULL)
      fprintf(SaveFile, "O %s~\n", CurSaveArea->owner);
   if (CurSaveArea->can_read != NULL)
      fprintf(SaveFile, "R %s~\n", CurSaveArea->can_read);
   if (CurSaveArea->can_write != NULL)
      fprintf(SaveFile, "W %s~\n", CurSaveArea->can_write);
   if (IS_SET(CurSaveArea->flags, AREA_PAYAREA))
      fprintf(SaveFile, "P This is a pay area.\n");
   if (IS_SET(CurSaveArea->flags, AREA_TELEPORT))
      fprintf(SaveFile, "T You can teleport into here\n");
   if (IS_SET(CurSaveArea->flags, AREA_BUILDING))
      fprintf(SaveFile, "B Currently building area.\n");
   if (IS_SET(CurSaveArea->flags, AREA_NOSHOW))
      fprintf(SaveFile, "S Title not shown on area list.\n");
   if (IS_SET(CurSaveArea->flags, AREA_NO_ROOM_AFF))
      fprintf(SaveFile, "M No bad room spells allowed.\n");
   if (CurSaveArea->music != NULL)
      fprintf(SaveFile, "C %s~\n", CurSaveArea->music);

   /*     fprintf( Envy, "#AREA\n" );                      remove save bug */
   /*     fprintf( Envy, "%s~\n", CurSaveArea->name );                     */

   Section++;
}

void build_save_mobs()
{
   MOB_INDEX_DATA *pMobIndex;

   if (Pointer == NULL) /* Start */
   {
      if (CurSaveArea->first_area_mobile == NULL)
      {
         Section++;
         return;
      }
      send_to_char("Saving mobs.\n", CurSaveChar);
      fprintf(SaveFile, "#MOBILES\n");
      Pointer = CurSaveArea->first_area_mobile;
   }

   pMobIndex = Pointer->data;
   fprintf(SaveFile, "#%i\n", pMobIndex->vnum);
   fprintf(SaveFile, "%s~\n", pMobIndex->player_name);
   fprintf(SaveFile, "%s~\n", pMobIndex->short_descr);
   fprintf(SaveFile, "%s~\n", pMobIndex->long_descr);
   fprintf(SaveFile, "%s~\n", pMobIndex->description);
   fprintf(SaveFile, "%llu %i %i S\n", pMobIndex->act, pMobIndex->affected_by,
           pMobIndex->alignment);
   fprintf(SaveFile, "%i %i\n", pMobIndex->level, pMobIndex->sex);
   fprintf(SaveFile, "%li %i %i %i\n", pMobIndex->hp_mod, pMobIndex->ac_mod, pMobIndex->hr_mod,
           pMobIndex->dr_mod);

   /*
    * Write out new details - clan, class, race and skills
    * * The '!' signifies new section to load_mobiles() in db.c
    */

   fprintf(SaveFile, "! %i %i %i %i %i %i %i\n", pMobIndex->class, pMobIndex->clan, pMobIndex->race,
           pMobIndex->position, pMobIndex->skills, pMobIndex->cast, pMobIndex->def);
   fprintf(SaveFile, "| %i %i %i %i %i %i %i\n", pMobIndex->strong_magic, pMobIndex->weak_magic,
           pMobIndex->race_mods, pMobIndex->power_skills, pMobIndex->power_cast, pMobIndex->resist,
           pMobIndex->suscept);
   fprintf(SaveFile, "+ %i %i %i %i %i %i %i %i %i\n", pMobIndex->spellpower_mod,
           pMobIndex->crit_mod, pMobIndex->crit_mult_mod, pMobIndex->spell_crit_mod,
           pMobIndex->spell_mult_mod, pMobIndex->parry_mod, pMobIndex->dodge_mod,
           pMobIndex->block_mod, pMobIndex->pierce_mod);
   fprintf(SaveFile, "l %i ", pMobIndex->loot_amount);
   for (int i = 0; i < MAX_LOOT; i++)
      fprintf(SaveFile, "%i ", pMobIndex->loot[i]);
   fprintf(SaveFile, "\nL ");
   for (int i = 0; i < MAX_LOOT; i++)
      fprintf(SaveFile, "%i ", pMobIndex->loot_chance[i]);
   fprintf(SaveFile, "\n");
   if (pMobIndex->lore_flags != 0)
      fprintf(SaveFile, "^ %ld\n", pMobIndex->lore_flags);
   if (IS_SET(pMobIndex->act, ACT_AI_DIALOGUE) && pMobIndex->ai_prompt != NULL &&
       pMobIndex->ai_prompt[0] != '\0')
      fprintf(SaveFile, "a %d %d %s~\n", pMobIndex->ai_knowledge, pMobIndex->accent,
              pMobIndex->ai_prompt);

   Pointer = Pointer->next;
   if (Pointer == NULL) /* End */
   {
      fprintf(SaveFile, "#0\n");
      Section++;
   }
   return;
}

void build_save_objects()
{
   OBJ_INDEX_DATA *pObject;
   AFFECT_DATA *pAf;
   EXTRA_DESCR_DATA *pEd;
   int val0, val1, val2, val3;

   if (Pointer == NULL) /* Start */
   {
      if (CurSaveArea->first_area_object == NULL)
      {
         Section++;
         return;
      }
      send_to_char("Saving objects.\n", CurSaveChar);
      fprintf(SaveFile, "#OBJECTS\n");
      Pointer = CurSaveArea->first_area_object;
   }

   pObject = Pointer->data;

   fprintf(SaveFile, "#%i\n", pObject->vnum);
   fprintf(SaveFile, "%s~\n", pObject->name);
   fprintf(SaveFile, "%s~\n", pObject->short_descr);
   fprintf(SaveFile, "%s~\n", pObject->description);
   fprintf(SaveFile, "%i %llu %i %i\n", pObject->item_type, pObject->extra_flags,
           pObject->wear_flags, pObject->item_apply);

   /*
    * Check for pills, potions, scrolls, staffs and wands.
    */
   val0 = pObject->value[0];
   val1 = pObject->value[1];
   val2 = pObject->value[2];
   val3 = pObject->value[3];
   switch (pObject->item_type)
   {
   case ITEM_PILL:
   case ITEM_POTION:
   case ITEM_SCROLL:
      val1 = val1 < 0 ? -1 : skill_table[val1].slot;
      val2 = val2 < 0 ? -1 : skill_table[val2].slot;
      val3 = val3 < 0 ? -1 : skill_table[val3].slot;
      break;

   case ITEM_STAFF:
      val3 = val3 < 0 ? -1 : skill_table[val3].slot;
      break;
   }
   fprintf(SaveFile, "%i %i %i %i %i %i %i %i %i %i\n", val0, val1, val2, val3, pObject->value[4],
           pObject->value[5], pObject->value[6], pObject->value[7], pObject->value[8],
           pObject->value[9]);
   fprintf(SaveFile, "%i\n", pObject->weight);

   pAf = pObject->first_apply;
   while (pAf)
   {
      fprintf(SaveFile, "A\n");
      fprintf(SaveFile, "%i %i\n", pAf->location, pAf->modifier);
      pAf = pAf->next;
   }

   pEd = pObject->first_exdesc;
   while (pEd)
   {
      fprintf(SaveFile, "E\n");
      fprintf(SaveFile, "%s~\n", pEd->keyword);
      fprintf(SaveFile, "%s~\n", pEd->description);
      pEd = pEd->next;
   }

   if ((pObject->level > 1) && (pObject->level <= 150))
   {
      fprintf(SaveFile, "L\n");
      fprintf(SaveFile, "%d\n", pObject->level);
   }
   else
   {
      fprintf(SaveFile, "L\n");
      fprintf(SaveFile, "%d\n", 1);
   }

   /*
    * Now for Envy... taken from my OLC :P
    */

   Pointer = Pointer->next;
   if (Pointer == NULL) /* End */
   {
      fprintf(SaveFile, "#0\n");
      Section++;
   }

   return;
}

void build_save_rooms()
{
   ROOM_INDEX_DATA *pRoomIndex;
   EXTRA_DESCR_DATA *pEd;
   int d;
   EXIT_DATA *pexit;
   int locks = 0;

   if (Pointer == NULL) /* Start */
   {
      if (CurSaveArea->first_area_room == NULL)
      {
         Section++;
         return;
      }
      send_to_char("Saving rooms.\n", CurSaveChar);
      fprintf(SaveFile, "#ROOMS\n");
      Pointer = CurSaveArea->first_area_room;
   }

   pRoomIndex = Pointer->data;

   fprintf(SaveFile, "#%i\n", pRoomIndex->vnum);
   fprintf(SaveFile, "%s~\n", pRoomIndex->name);
   fprintf(SaveFile, "%s~\n", pRoomIndex->description);
   fprintf(SaveFile, "%i %i\n", pRoomIndex->room_flags, pRoomIndex->sector_type);

   /*
    * Now do doors.
    */
   for (d = 0; d < 6; d++)
   {
      if (pRoomIndex->exit[d])
      {

         fprintf(SaveFile, "D%i\n", d);
         pexit = pRoomIndex->exit[d];
         fprintf(SaveFile, "%s~\n", pexit->description);
         fprintf(SaveFile, "%s~\n", pexit->keyword);
         /*
          * Deal with locks
          */
         /*
          * -S- Mod: Filter out EX_LOCKED and EX_CLOSED and save exit_info
          */
         locks = pexit->exit_info;
         if (IS_SET(locks, EX_CLOSED))
            REMOVE_BIT(locks, EX_CLOSED);
         if (IS_SET(locks, EX_LOCKED))
            REMOVE_BIT(locks, EX_LOCKED);

#if 0
         /*
          * BUG: before, an isdoor OR pickproof flag alone would give locks=1 
          */
         if( pexit->exit_info & EX_ISDOOR )
            locks++;
         if( pexit->exit_info & EX_PICKPROOF )
            locks++;
#endif

         fprintf(SaveFile, "%i %i %i\n", locks, pexit->key, pexit->vnum);
      }
   }

   /*
    * Now do extra descripts..
    */

   pEd = pRoomIndex->first_exdesc;
   while (pEd)
   {
      fprintf(SaveFile, "E\n");
      fprintf(SaveFile, "%s~\n", pEd->keyword);
      fprintf(SaveFile, "%s~\n", pEd->description);
      pEd = pEd->next;
   }

   /*
    * End of one room
    */
   fprintf(SaveFile, "S\n");

   Pointer = Pointer->next;
   if (Pointer == NULL) /* End */
   {
      fprintf(SaveFile, "#0\n");
      Section++;
   }
   return;
}

void build_save_shops()
{
   SHOP_DATA *pShop;
   int iTrade;

   if (Pointer == NULL) /* Start */
   {
      if (CurSaveArea->first_area_shop == NULL)
      {
         Section++;
         return;
      }
      send_to_char("Saving shops.\n", CurSaveChar);
      fprintf(SaveFile, "#SHOPS\n");
      Pointer = CurSaveArea->first_area_shop;
   }

   pShop = Pointer->data;
   fprintf(SaveFile, "%i ", pShop->keeper);
   for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
      fprintf(SaveFile, "%i ", pShop->buy_type[iTrade]);
   fprintf(SaveFile, "%i %i %i %i\n", pShop->profit_buy, pShop->profit_sell, pShop->open_hour,
           pShop->close_hour);

   Pointer = Pointer->next;
   if (Pointer == NULL) /* End */
   {
      fprintf(SaveFile, "0\n");
      Section++;
   }
   return;
}

void build_save_specs()
{
   MOB_INDEX_DATA *pMob;

   if (Pointer == NULL) /* Start */
   {
      if (CurSaveArea->first_area_specfunc == NULL)
      {
         Section++;
         return;
      }
      send_to_char("Saving specials.\n", CurSaveChar);
      fprintf(SaveFile, "#SPECIALS\n");
      Pointer = CurSaveArea->first_area_specfunc;
   }

   pMob = Pointer->data;

   fprintf(SaveFile, "M %i ", pMob->vnum);
   fprintf(SaveFile, "%s\n", rev_spec_lookup(pMob->spec_fun));

   Pointer = Pointer->next;
   if (Pointer == NULL) /* End */
   {
      fprintf(SaveFile, "S\n");
      Section++;
   }
   return;
}

void build_save_objfuns()
{
   OBJ_INDEX_DATA *pObj;

   if (Pointer == NULL) /* Start */
   {
      if (CurSaveArea->first_area_objfunc == NULL)
      {
         Section++;
         return;
      }
      send_to_char("Saving objfuns.\n", CurSaveChar);
      fprintf(SaveFile, "#OBJFUNS\n");
      Pointer = CurSaveArea->first_area_objfunc;
   }

   pObj = Pointer->data;

   fprintf(SaveFile, "O %i ", pObj->vnum);
   fprintf(SaveFile, "%s\n", rev_obj_fun_lookup(pObj->obj_fun));

   Pointer = Pointer->next;
   if (Pointer == NULL) /* End */
   {
      fprintf(SaveFile, "S\n");
      Section++;
   }
   return;
}

void build_save_resets()
{
   if (ResetPointer == NULL) /* Start */
   {
      if (CurSaveArea->first_reset == NULL)
      {
         Section++;
         return;
      }
      send_to_char("Saving resets.\n", CurSaveChar);
      fprintf(SaveFile, "#RESETS\n");
      ResetPointer = CurSaveArea->first_reset;
   }

   fprintf(SaveFile, "%c %i %i %i ", ResetPointer->command, ResetPointer->ifflag,
           ResetPointer->arg1, ResetPointer->arg2);
   if (ResetPointer->command == 'G' || ResetPointer->command == 'R')
      fprintf(SaveFile, "%s\n", ResetPointer->notes);
   else
      fprintf(SaveFile, "%i %s\n", ResetPointer->arg3, ResetPointer->notes);

   ResetPointer = ResetPointer->next;
   if (ResetPointer == NULL) /* End */
   {
      fprintf(SaveFile, "S\n");
      Section++;
   }
   return;
}

void build_save_end()
{
   char filename[255];
   char buf[MAX_STRING_LENGTH];

   sprintf(buf, "Finished saving %s", CurSaveArea->filename);
   monitor_chan(buf, MONITOR_AREA_SAVING);

   fprintf(SaveFile, "#$\n");
   send_to_char("Finished saving.\n", CurSaveChar);
   if (SaveFile != NULL)
   {
      fclose(SaveFile);
      SaveFile = NULL;
   }
   /*
    * Save backup
    */
   sprintf(filename, "%s.old", CurSaveArea->filename);
   rename(CurSaveArea->filename, filename);
   /*
    * And rename .new to area filename
    */
   sprintf(filename, "%s.new", CurSaveArea->filename);
   rename(filename, CurSaveArea->filename);

   Section = 0;
   if (ToBeSaved == (CurrentSaving + 1) % SAVEQUEUESIZE)
      saving_area = NOT_SAVING;
   else
      saving_area = START_SAVING;
}

void build_save_flush()
{
   AREA_DATA *pArea;

   if (AreasModified == 0)
      return;

   for (pArea = first_area; pArea != NULL; pArea = pArea->next)
   {
      if (pArea->modified)
      {
         pArea->modified = 0;
         do_savearea(NULL, (char *)pArea);
      }
   }

   AreasModified = 0;
}

/* (unused variable suppression when compiled without file-based save) */

void area_modified(AREA_DATA *pArea)
{
   pArea->modified = 1;
   AreasModified = 1;
}

const int convert_wearflags[] = {BIT_24, BIT_14, BIT_8,  BIT_19, BIT_4,  BIT_21, BIT_22, BIT_13,
                                 BIT_11, BIT_16, BIT_17, BIT_18, BIT_12, BIT_16, BIT_16, BIT_5,
                                 BIT_7,  BIT_16, BIT_24, BIT_24, BIT_24, BIT_24, BIT_24, BIT_24,
                                 BIT_24, BIT_24, BIT_24, BIT_24, BIT_24, BIT_24, BIT_24, BIT_24};

/*
 * Snarf an 'area' header line.
 */
void load_area(FILE *fp)
{
   AREA_DATA *pArea;
   char letter;
   int a;

   GET_FREE(pArea, area_free);
   pArea->first_reset = NULL;
   pArea->last_reset = NULL;
   pArea->name = fread_string(fp);
   pArea->age = 15;
   pArea->reset_rate = 15;
   pArea->nplayer = 0;
   pArea->offset = 0;
   /* MAG Mod */
   pArea->modified = 0;
   pArea->min_vnum = 0;
   pArea->max_vnum = MAX_VNUM;
   pArea->area_num = 0;
   pArea->filename = str_dup(strArea);
   pArea->owner = str_empty;
   pArea->can_read = str_dup("all");
   pArea->can_write = str_dup("all");
   pArea->level_label = str_dup("{?? ??}");
   pArea->keyword = str_dup("none");
   pArea->reset_msg = str_dup("You hear the screams of the Dead within your head.");
   pArea->min_level = 0;
   pArea->max_level = 0;
   pArea->gold = 0;
   pArea->flags = 0;
   pArea->first_area_room = NULL;
   pArea->last_area_room = NULL;
   pArea->first_area_object = NULL;
   pArea->last_area_object = NULL;
   pArea->first_area_mobile = NULL;
   pArea->last_area_mobile = NULL;
   pArea->first_area_shop = NULL;
   pArea->last_area_shop = NULL;
   pArea->first_area_specfunc = NULL;
   pArea->last_area_specfunc = NULL;
   pArea->first_area_speechfun = NULL;
   pArea->last_area_speechfun = NULL;
   area_revision = -1;

   /* MAG Mod for optionals additions to area headers. */
   for (;;)
   {
      letter = fread_letter(fp);
      if (letter == '#')
      {
         ungetc(letter, fp);
         break;
      }

      switch (letter)
      {
      case 'F':
         pArea->reset_rate = fread_number(fp);
         break;
      case 'O':
         pArea->owner = fread_string(fp);
         break;
      case 'Q':
         area_revision = fread_number(fp);
         break;
      case 'U':
         free_string(pArea->reset_msg);
         pArea->reset_msg = fread_string(fp);
         break;
      case 'R':
         free_string(pArea->can_read);
         pArea->can_read = fread_string(fp);
         break;
      case 'W':
         free_string(pArea->can_write);
         pArea->can_write = fread_string(fp);
         break;
      case 'P':
         SET_BIT(pArea->flags, AREA_PAYAREA);
         fread_to_eol(fp);
         break;
      case 'M':
         SET_BIT(pArea->flags, AREA_NO_ROOM_AFF);
         fread_to_eol(fp);
         break;

      case 'X':
         pArea->offset = fread_number(fp);
         break;
      case 'V':
         pArea->min_vnum = fread_number(fp);
         pArea->max_vnum = fread_number(fp);
         break;
      case 'N':
         pArea->area_num = fread_number(fp);
         break;
      case 'T':
         SET_BIT(pArea->flags, AREA_TELEPORT);
         fread_to_eol(fp);
         break;
      case 'B':
         SET_BIT(pArea->flags, AREA_BUILDING);
         fread_to_eol(fp);
         break;
      case 'S':
         SET_BIT(pArea->flags, AREA_NOSHOW);
         fread_to_eol(fp);
         break;
      case 'K':
         free_string(pArea->keyword);
         pArea->keyword = fread_string(fp);
         break;
      case 'L':
         free_string(pArea->level_label);
         pArea->level_label = fread_string(fp);
         break;
      case 'I':
         pArea->min_level = fread_number(fp);
         pArea->max_level = fread_number(fp);
         break;
      case 'C':
         pArea->music = fread_string(fp);
         break;
      }
   }

   if (pArea->area_num == 0)
   {
      /*
       * Find a unique area number
       */
      for (a = 0; a < MAX_AREAS; a++)
         if (area_used[a] == NULL)
            break;
      pArea->area_num = a;
   }

   area_used[pArea->area_num] = pArea;
   area_load = pArea;

   LINK(pArea, first_area, last_area, next, prev);

   top_area++;
   return;
}

void load_mobiles(FILE *fp)
{
   char buf[MSL];
   MOB_INDEX_DATA *pMobIndex;
   BUILD_DATA_LIST *pList;

   for (;;)
   {
      int vnum;
      char letter;
      int iHash;
      bool is_dup;

      letter = fread_letter(fp);
      if (letter != '#')
      {
         sprintf(buf, "Load_mobiles: # not found, %c found instead.", letter);
         bug(buf, 0);
         hang("Loading Mobiles in db.c");
      }

      vnum = fread_number(fp);
      if (vnum == 0)
         break;

      fBootDb = FALSE;
      is_dup = (get_mob_index(vnum) != NULL);
      fBootDb = TRUE;
      if (is_dup)
         bug("Load_mobiles: vnum %d duplicated -- skipping.", vnum);

      GET_FREE(pMobIndex, mid_free);
      pMobIndex->vnum = vnum;
      pMobIndex->area = area_load;
      pMobIndex->player_name = fread_string(fp);
      pMobIndex->short_descr = fread_string(fp);
      pMobIndex->long_descr = fread_string(fp);
      pMobIndex->description = fread_string(fp);

      pMobIndex->long_descr[0] = UPPER(pMobIndex->long_descr[0]);
      pMobIndex->description[0] = UPPER(pMobIndex->description[0]);

      pMobIndex->act = fread_number_ull(fp) | ACT_IS_NPC;
      pMobIndex->affected_by = fread_number(fp);
      pMobIndex->pShop = NULL;
      pMobIndex->alignment = fread_number(fp);
      letter = fread_letter(fp);
      pMobIndex->level = fread_number(fp);

      pMobIndex->sex = fread_number(fp);

      pMobIndex->hp_mod = fread_number(fp);
      pMobIndex->ac_mod = fread_number(fp); /* read      */
      pMobIndex->hr_mod = fread_number(fp); /* in the    */
      pMobIndex->dr_mod = fread_number(fp); /* modifiers */

      if (letter != 'S')
      {
         bug("Load_mobiles: vnum %d non-S.", vnum);
         hang("Loading Mobiles in db.c");
      }

      /*
       * The "new" additions to mobs start with a !
       */
      letter = fread_letter(fp);
      if (letter == '!')
      {
         pMobIndex->class = fread_number(fp);
         pMobIndex->clan = fread_number(fp);
         pMobIndex->race = fread_number(fp);
         pMobIndex->position = POS_STANDING;
         fread_number(fp); /* position */
         pMobIndex->skills = fread_number(fp);
         pMobIndex->cast = fread_number(fp);
         pMobIndex->def = fread_number(fp);
         if ((area_revision < 16) || (pMobIndex->race < 0))
         {
            pMobIndex->race = 0;
         }
      }
      else
         ungetc(letter, fp);

      letter = fread_letter(fp);
      if (letter == '|')
      {
         pMobIndex->strong_magic = fread_number(fp);
         pMobIndex->weak_magic = fread_number(fp);
         pMobIndex->race_mods = fread_number(fp);
         fread_number(fp); /* power_skills */
         fread_number(fp); /* power_cast */
         pMobIndex->resist = fread_number(fp);
         pMobIndex->suscept = fread_number(fp);
      }
      else
         ungetc(letter, fp);

      letter = fread_letter(fp);
      if (letter == '+')
      {
         pMobIndex->spellpower_mod = fread_number(fp);
         pMobIndex->crit_mod = fread_number(fp);
         pMobIndex->crit_mult_mod = fread_number(fp);
         pMobIndex->spell_crit_mod = fread_number(fp);
         pMobIndex->spell_mult_mod = fread_number(fp);
         pMobIndex->parry_mod = fread_number(fp);
         pMobIndex->dodge_mod = fread_number(fp);
         pMobIndex->block_mod = fread_number(fp);
         pMobIndex->pierce_mod = fread_number(fp);
      }
      else
         ungetc(letter, fp);

      letter = fread_letter(fp);
      if (letter == 'l')
      {
         pMobIndex->loot_amount = fread_number(fp);
         for (int i = 0; i < MAX_LOOT; i++)
         {
            pMobIndex->loot[i] = fread_number(fp);
         }
      }
      else
         ungetc(letter, fp);

      letter = fread_letter(fp);
      if (letter == 'L')
      {
         for (int i = 0; i < MAX_LOOT; i++)
         {
            pMobIndex->loot_chance[i] = fread_number(fp);
         }
      }
      else
         ungetc(letter, fp);

      letter = fread_letter(fp);
      if (letter == '^')
      {
         pMobIndex->lore_flags = fread_number(fp);
      }
      else
         ungetc(letter, fp);

      pMobIndex->ai_prompt = NULL;
      pMobIndex->ai_knowledge = 0;
      pMobIndex->accent = ACCENT_NONE;

      letter = fread_letter(fp);
      if (letter == 'a')
      {
         pMobIndex->ai_knowledge = fread_number(fp);
         pMobIndex->accent = (sh_int)fread_number(fp);
         pMobIndex->ai_prompt = fread_string(fp);
      }
      else
         ungetc(letter, fp);

      if (is_dup)
      {
         free_string(pMobIndex->player_name);
         free_string(pMobIndex->short_descr);
         free_string(pMobIndex->long_descr);
         free_string(pMobIndex->description);
         if (pMobIndex->ai_prompt)
            free_string(pMobIndex->ai_prompt);
         PUT_FREE(pMobIndex, mid_free);
      }
      else
      {
         iHash = vnum % MAX_KEY_HASH;
         SING_TOPLINK(pMobIndex, mob_index_hash[iHash], next);
         /* MAG Mod */
         GET_FREE(pList, build_free);
         pList->data = pMobIndex;
         LINK(pList, area_load->first_area_mobile, area_load->last_area_mobile, next, prev);

         top_mob_index++;
         kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL - 1)].number++;
      }
   }

   return;
}

/*
 * Snarf an obj section.
 */

void load_objects(FILE *fp)
{
   OBJ_INDEX_DATA *pObjIndex;
   BUILD_DATA_LIST *pList;
   sh_int looper;

   for (;;)
   {
      int vnum;
      char letter;
      int iHash;
      bool is_dup;

      letter = fread_letter(fp);
      if (letter != '#')
      {
         bug("Load_objects: # not found.", 0);
         hang("Loading Objects in db.c");
      }

      vnum = fread_number(fp);
      if (vnum == 0)
         break;

      fBootDb = FALSE;
      is_dup = (get_obj_index(vnum) != NULL);
      fBootDb = TRUE;
      if (is_dup)
         bug("Load_objects: vnum %d duplicated -- skipping.", vnum);

      GET_FREE(pObjIndex, oid_free);
      pObjIndex->vnum = vnum;
      pObjIndex->area = area_load;
      pObjIndex->name = fread_string(fp);
      pObjIndex->short_descr = fread_string(fp);
      pObjIndex->description = fread_string(fp);

      pObjIndex->short_descr[0] = LOWER(pObjIndex->short_descr[0]);
      pObjIndex->description[0] = UPPER(pObjIndex->description[0]);

      pObjIndex->item_type = fread_number(fp);
      pObjIndex->extra_flags = fread_number_ull(fp);
      if (area_revision < 15)
      {
         int temp_flags, index, new_flags = 0;
         temp_flags = fread_number(fp);
         for (index = 0; index < 32; index++)
         {
            if (IS_SET(temp_flags, (1 << index)))
            {
               SET_BIT(new_flags, convert_wearflags[index]);
            }
            pObjIndex->wear_flags = new_flags;
         }
      }
      else
      {
         pObjIndex->wear_flags = fread_number(fp);
      }
      pObjIndex->item_apply = fread_number(fp);
      pObjIndex->value[0] = fread_number(fp);
      pObjIndex->value[1] = fread_number(fp);
      pObjIndex->value[2] = fread_number(fp);
      pObjIndex->value[3] = fread_number(fp);
      if (area_revision < 15)
      {
         for (looper = 4; looper < 10; pObjIndex->value[looper] = 0, looper++)
            ;
      }
      else
      {
         for (looper = 4; looper < 10; pObjIndex->value[looper] = fread_number(fp), looper++)
            ;
      }
      pObjIndex->weight = fread_number(fp);
      pObjIndex->cost = 0;

      if (pObjIndex->item_type == ITEM_POTION)
         SET_BIT(pObjIndex->extra_flags, ITEM_NODROP);

      for (;;)
      {
         char letter;

         letter = fread_letter(fp);

         if (letter == 'A')
         {
            AFFECT_DATA *paf;

            GET_FREE(paf, affect_free);
            paf->type = -1;
            paf->duration = -1;
            paf->location = fread_number(fp);
            paf->modifier = fread_number(fp);
            paf->bitvector = 0;
            paf->caster = NULL;
            LINK(paf, pObjIndex->first_apply, pObjIndex->last_apply, next, prev);
            top_affect++;
         }

         else if (letter == 'E')
         {
            EXTRA_DESCR_DATA *ed;

            GET_FREE(ed, exdesc_free);
            ed->keyword = fread_string(fp);
            ed->description = fread_string(fp);
            LINK(ed, pObjIndex->first_exdesc, pObjIndex->last_exdesc, next, prev);
            top_ed++;
         }

         else if (letter == 'L')
         {
            pObjIndex->level = fread_number(fp);
         }

         else
         {
            ungetc(letter, fp);
            break;
         }
      }

      /*
       * Translate spell "slot numbers" to internal "skill numbers."
       */
      switch (pObjIndex->item_type)
      {
      case ITEM_PILL:
      case ITEM_POTION:
      case ITEM_SCROLL:
         pObjIndex->value[1] = slot_lookup(pObjIndex->value[1]);
         pObjIndex->value[2] = slot_lookup(pObjIndex->value[2]);
         pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
         break;

      case ITEM_STAFF:
         pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
         break;
      }

      if (is_dup)
      {
         AFFECT_DATA *paf;
         EXTRA_DESCR_DATA *ed;

         free_string(pObjIndex->name);
         free_string(pObjIndex->short_descr);
         free_string(pObjIndex->description);
         while ((paf = pObjIndex->first_apply) != NULL)
         {
            pObjIndex->first_apply = paf->next;
            PUT_FREE(paf, affect_free);
         }
         while ((ed = pObjIndex->first_exdesc) != NULL)
         {
            pObjIndex->first_exdesc = ed->next;
            PUT_FREE(ed, exdesc_free);
         }
         PUT_FREE(pObjIndex, oid_free);
      }
      else
      {
         iHash = vnum % MAX_KEY_HASH;
         SING_TOPLINK(pObjIndex, obj_index_hash[iHash], next);
         /* MAG Mod */
         GET_FREE(pList, build_free);
         pList->data = pObjIndex;
         LINK(pList, area_load->first_area_object, area_load->last_area_object, next, prev);

         top_obj_index++;
      }
   }

   return;
}

/*

 * Snarf a reset section.
 */

/* Spec: it isn't obvious how this works, so I'll add a comment here.
 *
 * load_resets only allows resets that are placed in current rooms - this is
 * the reset_ok code here. It does _not_ check the validity of room and object
 * vnums. It handles linking the resets into the appropriate room-reset-list
 * for OLC.
 *
 * check_resets is called after all areas are loaded. It checks that all
 * resets point to valid mobs and object and spits errors as necessary. It
 * does _not_ touch the room lists any more!
 */
void load_resets(FILE *fp)
{
   RESET_DATA *pReset = NULL;
   BUILD_DATA_LIST *pList = NULL;
   ROOM_INDEX_DATA *last_mob_room;
   ROOM_INDEX_DATA *last_obj_room;

   last_mob_room = NULL;
   last_obj_room = NULL;
   (void)last_obj_room;

   if (area_load == NULL)
   {
      bug("Load_resets: no #AREA seen yet.", 0);
      hang("Loading Resets in db.c");
   }

   for (;;)
   {
      ROOM_INDEX_DATA *pRoomIndex;
      /*
       * EXIT_DATA *pexit; unused?
       */
      char letter;
      /*
       * Temp values to hold reset, while we see if valid
       */
      char Tcommand;
      int Tifflag;
      int Targ1;
      int Targ2;
      int Targ3;
      bool reset_ok;

      reset_ok = FALSE;

      if ((letter = fread_letter(fp)) == 'S')
         break;

      if (letter == '*')
      {
         fread_to_eol(fp);
         continue;
      }

      Tcommand = letter;
      /*
       * MAG Mod
       */

      Tifflag = fread_number(fp); /* Not used */
      (void)Tifflag;
      Targ1 = fread_number(fp);
      Targ2 = fread_number(fp);
      Targ3 = (letter == 'G' || letter == 'R') ? 0 : fread_number(fp);

      pRoomIndex = NULL; /* Set it to check for room numbers */
      /*
       * Validate parameters.
       * We're calling the index functions for the side effect.
       */
      switch (letter)
      {
      default:
         bug("Load_resets: bad command '%c'.", letter);
         reset_ok = FALSE;
         break;

      case 'A':
         reset_ok = FALSE;
         break; /* obsolete reset, discard */
      case 'M':
         if ((pRoomIndex = get_room_index(Targ3)) != NULL)
         {
            reset_ok = TRUE;
            last_mob_room = pRoomIndex;
         }
         break;
      case 'O':
         if ((pRoomIndex = get_room_index(Targ3)) != NULL)
         {
            reset_ok = TRUE;
            last_obj_room = pRoomIndex;
         }
         break;
      case 'P':
         if (last_obj_room && get_obj_index(Targ1) && get_obj_index(Targ3))
         {
            pRoomIndex = last_obj_room;
            reset_ok = TRUE;
         }
         break;
      case 'G':
      case 'E':
         if (last_mob_room)
         {
            pRoomIndex = last_mob_room;
            reset_ok = TRUE;
         }
         break;

      case 'D':
         if ((pRoomIndex = get_room_index(Targ1)) != NULL)
         {
            reset_ok = TRUE;
         }
         break;
      case 'R':
         if ((pRoomIndex = get_room_index(Targ1)) != NULL)
         {
            reset_ok = TRUE;
         }
         break;
      }

      if (reset_ok) /* i.e. only add if reset is valid. */
      {

         GET_FREE(pReset, reset_free);
         pReset->command = Tcommand;
         pReset->arg1 = Targ1;
         pReset->arg2 = Targ2;
         pReset->arg3 = Targ3;
         pReset->notes = fsave_to_eol(fp);

         LINK(pReset, area_load->first_reset, area_load->last_reset, next, prev);
         top_reset++;
         GET_FREE(pList, build_free);
         pList->data = pReset;
         LINK(pList, pRoomIndex->first_room_reset, pRoomIndex->last_room_reset, next, prev);
      }
   }

   return;
}

/*
 * Snarf a room section.
 */

void load_rooms(FILE *fp)
{
   ROOM_INDEX_DATA *pRoomIndex;
   BUILD_DATA_LIST *pList;
   sh_int cnt;

   (void)cnt;

   if (area_load == NULL)
   {
      bug("Load_rooms: no #AREA seen yet.", 0);
      hang("Loading Rooms in db.c");
   }

   for (;;)
   {
      int vnum;
      char letter;
      int door;
      int iHash;
      bool is_dup;

      letter = fread_letter(fp);
      if (letter != '#')
      {
         bug("Load_rooms: # not found.", 0);
         hang("Loading Rooms in db.c");
      }

      vnum = fread_number(fp);
      if (vnum == 0)
         break;

      fBootDb = FALSE;
      is_dup = (get_room_index(vnum) != NULL);
      fBootDb = TRUE;
      if (is_dup)
         bug("Load_rooms: vnum %d duplicated -- skipping.", vnum);

      GET_FREE(pRoomIndex, rid_free);
      pRoomIndex->first_person = NULL;
      pRoomIndex->last_person = NULL;
      pRoomIndex->first_content = NULL;
      pRoomIndex->last_content = NULL;
      pRoomIndex->first_exdesc = NULL;
      pRoomIndex->last_exdesc = NULL;
      pRoomIndex->area = area_load;
      pRoomIndex->vnum = vnum;
      pRoomIndex->name = fread_string(fp);
      pRoomIndex->description = fread_string(fp);
      pRoomIndex->room_flags = fread_number(fp);
      pRoomIndex->sector_type = fread_number(fp);
      if (pRoomIndex->sector_type == SECT_NULL)
         pRoomIndex->sector_type = SECT_INSIDE;
      pRoomIndex->light = 0;
      pRoomIndex->affected_by = ROOM_BV_NONE;
      pRoomIndex->first_room_affect = NULL;
      pRoomIndex->last_room_affect = NULL;
      for (door = 0; door <= 5; door++)
         pRoomIndex->exit[door] = NULL;
      pRoomIndex->first_room_reset = NULL; /* MAG Mod */
      pRoomIndex->last_room_reset = NULL;

      for (;;)
      {
         letter = fread_letter(fp);

         if (letter == 'S')
            break;

         if (letter == 'D')
         {
            EXIT_DATA *pexit;
            int locks;

            door = fread_number(fp);
            if (door < 0 || door > 5)
            {
               bug("Fread_rooms: vnum %d has bad door number.", vnum);
               hang("Loading Rooms in db.c");
            }

            GET_FREE(pexit, exit_free);
            pexit->description = fread_string(fp);
            pexit->keyword = fread_string(fp);
            pexit->exit_info = 0;
            locks = fread_number(fp);
            pexit->key = fread_number(fp);
            pexit->vnum = fread_number(fp);

            /*
             * -S- Mod:  exit_flags now saved as-is, with EX_CLOSED and
             * EX_LOCKED filtered out.  If locks is 2, it's from an
             * 'old' area, so handle it, otherwise just set exit_info.
             * This allows new exit flags to be easily added.  :)
             * (Impossible for new area to have locks  = 2(closed)
             */

            if (locks == 2)
               pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
            else
               pexit->exit_info = locks;

            pRoomIndex->exit[door] = pexit;
            top_exit++;
         }
         else if (letter == 'E')
         {
            EXTRA_DESCR_DATA *ed;

            GET_FREE(ed, exdesc_free);
            ed->keyword = fread_string(fp);
            ed->description = fread_string(fp);
            LINK(ed, pRoomIndex->first_exdesc, pRoomIndex->last_exdesc, next, prev);
            top_ed++;
         }
         else
         {
            bug("Load_rooms: vnum %d has flag not 'DES'.", vnum);
            hang("Loading Rooms in db.c");
         }
      }

      if (is_dup)
      {
         EXTRA_DESCR_DATA *ed;

         free_string(pRoomIndex->name);
         free_string(pRoomIndex->description);
         for (door = 0; door <= 5; door++)
         {
            if (pRoomIndex->exit[door] != NULL)
               PUT_FREE(pRoomIndex->exit[door], exit_free);
         }
         while ((ed = pRoomIndex->first_exdesc) != NULL)
         {
            pRoomIndex->first_exdesc = ed->next;
            PUT_FREE(ed, exdesc_free);
         }
         PUT_FREE(pRoomIndex, rid_free);
      }
      else
      {
         iHash = vnum % MAX_KEY_HASH;
         SING_TOPLINK(pRoomIndex, room_index_hash[iHash], next);
         /* MAG Mod */
         GET_FREE(pList, build_free);
         pList->data = pRoomIndex;
         LINK(pList, area_load->first_area_room, area_load->last_area_room, next, prev);

         top_room++;
      }
   }

   return;
}

/*

 * Snarf a shop section.
 */
void load_shops(FILE *fp)
{
   SHOP_DATA *pShop;
   BUILD_DATA_LIST *pList;

   for (;;)
   {
      MOB_INDEX_DATA *pMobIndex;
      int iTrade;
      int keeper;

      keeper = fread_number(fp);
      if (keeper == 0)
         break;
      GET_FREE(pShop, shop_free);
      pShop->keeper = keeper;
      for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
         pShop->buy_type[iTrade] = fread_number(fp);
      pShop->profit_buy = fread_number(fp);
      pShop->profit_sell = fread_number(fp);
      pShop->open_hour = fread_number(fp);
      pShop->close_hour = fread_number(fp);
      fread_to_eol(fp);
      pMobIndex = get_mob_index(pShop->keeper);
      pMobIndex->pShop = pShop;

      /* MAG Mod */
      GET_FREE(pList, build_free);
      pList->data = pShop;
      LINK(pList, area_load->first_area_shop, area_load->last_area_shop, next, prev);

      LINK(pShop, first_shop, last_shop, next, prev);
      top_shop++;
   }

   return;
}

/*

 * Snarf spec proc declarations.
 */
void load_specials(FILE *fp)
{
   for (;;)
   {
      MOB_INDEX_DATA *pMobIndex;
      char letter;

      switch (letter = fread_letter(fp))
      {
      default:
         bug("Load_specials: letter '%c' not *, M, E, or S.", letter);
         hang("Loading Specials in db.c");

      case 'S':
         return;

      case '*':
         break;

      case 'M':
         pMobIndex = get_mob_index(fread_number(fp));
         pMobIndex->spec_fun = spec_lookup(fread_word(fp));
         if (pMobIndex->spec_fun == 0)
         {
            bug("Load_specials(mob): 'M': vnum %d.", pMobIndex->vnum);
         }
         else
         {
            BUILD_DATA_LIST *pList;

            /*
             * MAG Mod
             */
            GET_FREE(pList, build_free);
            pList->data = pMobIndex;
            LINK(pList, area_load->first_area_specfunc, area_load->last_area_specfunc, next, prev);
         }
         break;
      }
      /*
       * NB. Comments will not be saved when using areasave - MAG.
       */
      fread_to_eol(fp);
   }
}

void build_save_speech()
{
   MOB_INDEX_DATA *pMob;

   if (Pointer == NULL) /* Start */
   {
      if (CurSaveArea->first_area_speechfun == NULL)
      {
         Section++;
         return;
      }
      send_to_char("Saving speech handlers.\n", CurSaveChar);
      fprintf(SaveFile, "#SPEECH\n");
      Pointer = CurSaveArea->first_area_speechfun;
   }

   pMob = Pointer->data;

   fprintf(SaveFile, "M %i ", pMob->vnum);
   fprintf(SaveFile, "%s\n", speech_name(pMob->speech_fun));

   Pointer = Pointer->next;
   if (Pointer == NULL) /* End */
   {
      fprintf(SaveFile, "S\n");
      Section++;
   }
   return;
}

/* Snarf speech handler declarations. */

void load_speech(FILE *fp)
{
   for (;;)
   {
      MOB_INDEX_DATA *pMobIndex;
      char letter;

      switch (letter = fread_letter(fp))
      {
      default:
         bug("Load_speech: letter '%c' not *, M, or S.", letter);
         hang("Loading Speech in db.c");

      case 'S':
         return;

      case '*':
         break;

      case 'M':
         pMobIndex = get_mob_index(fread_number(fp));
         pMobIndex->speech_fun = speech_lookup(fread_word(fp));
         if (pMobIndex->speech_fun == 0)
         {
            bug("Load_speech(mob): 'M': vnum %d.", pMobIndex->vnum);
         }
         else
         {
            BUILD_DATA_LIST *pList;

            GET_FREE(pList, build_free);
            pList->data = pMobIndex;
            LINK(pList, area_load->first_area_speechfun, area_load->last_area_speechfun, next,
                 prev);
         }
         break;
      }
      fread_to_eol(fp);
   }
}

/* Snarf objfun declarations.  -S- */

void load_objfuns(FILE *fp)
{
   for (;;)
   {
      OBJ_INDEX_DATA *pObj;
      char letter;

      switch (letter = fread_letter(fp))
      {
      default:
         bug("Load_objfuns: letter '%c' not *, O or S.", letter);
         hang("Loading Objfuns in db.c");

      case 'S':
         return;

      case '*':
         break;

      case 'O': /* -S- mod => obj_funs */
         if ((pObj = get_obj_index(fread_number(fp))) == NULL)
         {
            char *temp;
            char buf[MSL];
            temp = fread_word(fp);
            snprintf(buf, sizeof(buf),
                     "Error in Load Objfuns:  area %s has Objfun without corresponding object.  "
                     "Save this area after booting complete to remove.",
                     strArea);
            log_f("%s", buf);
            free_string(temp);
         }
         else
            pObj->obj_fun = obj_fun_lookup(fread_word(fp));
         if (pObj != NULL)
         {
            if (pObj->obj_fun == 0)
            {
               bug("Load_specials(obj): 'X': vnum %d.", pObj->vnum);
            }
            else
            {
               BUILD_DATA_LIST *pList;

               GET_FREE(pList, build_free);
               pList->data = pObj;
               LINK(pList, area_load->first_area_objfunc, area_load->last_area_objfunc, next, prev);
            }
         }
         break;
      }
      /*
       * NB. Comments will not be saved when using areasave - MAG.
       */
      fread_to_eol(fp);
   }
}
