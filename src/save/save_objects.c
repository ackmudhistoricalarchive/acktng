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
 * save_objects.c — Object and keep-chest save/load.
 *
 * Handles serialisation of in-game objects carried by players and stored
 * in keep chests:
 *   fwrite_obj / fread_obj         — player inventory save/load
 *   fwrite_chest / fread_chest_item — keep chest contents save/load
 *   save_chest / load_chest        — per-chest file I/O
 *   delete_chest_file              — chest file removal on destruction
 */

#include "save.h"
#include "../db/db_worker.h"

extern int top_obj_index;

/*
 * Write an object and its contents.
 */
void fwrite_obj(CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest)
{
   EXTRA_DESCR_DATA *ed;
   AFFECT_DATA *paf;
   /*
    * Slick recursion to write lists backwards,
    *   so loading them will load in forwards order.
    */
   loop_counter++;
   if (loop_counter > 650)
   {
      abort_wrapper();
   }
   if (obj->next_in_carry_list != NULL)
      fwrite_obj(ch, obj->next_in_carry_list, fp, iNest);

   /*
    * Castrate storage characters.
    */

   /*
    * Also bypass no-save objects -S-
    */

   bool can_save = FALSE;
   if (get_psuedo_level(ch) + 5 >= obj->level)
      can_save = TRUE;
   if (get_total_reincarnations(ch) > 0)
      can_save = TRUE;
   if (obj->item_type == ITEM_KEY || obj->item_type == ITEM_BEACON ||
       IS_SET(obj->extra_flags, ITEM_NOSAVE))
      can_save = FALSE;

   if (!can_save)
      return;

   fprintf(fp, "#OBJECT\n");
   fprintf(fp, "Nest         %d\n", iNest);
   fprintf(fp, "Name         %s~\n", obj->name);
   fprintf(fp, "ShortDescr   %s~\n", obj->short_descr);
   fprintf(fp, "Description  %s~\n", obj->description);
   fprintf(fp, "Vnum         %d\n", obj->pIndexData->vnum);
   fprintf(fp, "ExtraFlags   %llu\n", obj->extra_flags);
   fprintf(fp, "WearFlags    %d\n", obj->wear_flags);
   fprintf(fp, "WearLoc      %d\n", obj->wear_loc);
   if (obj->obj_fun != NULL)
      fprintf(fp, "Objfun       %s~\n", rev_obj_fun_lookup(obj->obj_fun));

   fprintf(fp, "ClassFlags   %d\n", obj->item_apply);
   /*
    * ClassFlags still used to save fucking with pfiles
    */
   fprintf(fp, "ItemType     %d\n", obj->item_type);
   fprintf(fp, "Weight       %d\n", obj->weight);
   fprintf(fp, "Level        %d\n", obj->level);
   fprintf(fp, "Timer        %d\n", obj->timer);
   fprintf(fp, "Cost         %d\n", obj->cost);
   fprintf(fp, "Values       %d %d %d %d %d %d %d %d %d %d\n", obj->value[0], obj->value[1],
           obj->value[2], obj->value[3], obj->value[4], obj->value[5], obj->value[6], obj->value[7],
           obj->value[8], obj->value[9]);

   switch (obj->item_type)
   {
   case ITEM_POTION:
   case ITEM_SCROLL:
      if (obj->value[1] > 0)
      {
         fprintf(fp, "Spell 1      '%s'\n", skill_table[obj->value[1]].name);
      }

      if (obj->value[2] > 0)
      {
         fprintf(fp, "Spell 2      '%s'\n", skill_table[obj->value[2]].name);
      }

      if (obj->value[3] > 0)
      {
         fprintf(fp, "Spell 3      '%s'\n", skill_table[obj->value[3]].name);
      }

      break;

   case ITEM_PILL:
   case ITEM_STAFF:
      if (obj->value[3] > 0)
      {
         fprintf(fp, "Spell 3      '%s'\n", skill_table[obj->value[3]].name);
      }

      break;
   }

   for (paf = obj->first_apply; paf != NULL; paf = paf->next)
   {
      fprintf(fp, "Affect       %d %d %d %d %d\n", paf->type, paf->duration, paf->modifier,
              paf->location, paf->bitvector);
   }

   for (ed = obj->first_exdesc; ed != NULL; ed = ed->next)
   {
      fprintf(fp, "ExtraDescr   %s~ %s~\n", ed->keyword, ed->description);
   }

   fprintf(fp, "End\n\n");

   if (obj->first_in_carry_list != NULL)
      fwrite_obj(ch, obj->first_in_carry_list, fp, iNest + 1);

   return;
}

void fread_obj(CHAR_DATA *ch, FILE *fp)
{
   static OBJ_DATA obj_zero;
   OBJ_DATA *obj;
   char *word;
   int iNest;
   bool fMatch;
   bool fNest;
   bool fVnum;
   int Temp_Obj = 0, OldVnum = 0;

   GET_FREE(obj, obj_free);
   *obj = obj_zero;
   obj->name = str_dup("");
   obj->short_descr = str_dup("");
   obj->description = str_dup("");
   fNest = FALSE;
   fVnum = TRUE;
   iNest = 0;

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
         if (!str_cmp(word, "Affect"))
         {
            AFFECT_DATA *paf;

            GET_FREE(paf, affect_free);
            paf->type = fread_number(fp);
            paf->duration = fread_number(fp);
            paf->modifier = fread_number(fp);
            paf->location = fread_number(fp);
            paf->bitvector = fread_number(fp);
            LINK(paf, obj->first_apply, obj->last_apply, next, prev);
            fMatch = TRUE;
            break;
         }
         break;

      case 'C':
         KEY("Cost", obj->cost, fread_number(fp));
         KEY("ClassFlags", obj->item_apply, fread_number(fp));
         break;

      case 'D':
         SKEY("Description", obj->description, fread_string(fp));
         break;

      case 'E':
         KEY("ExtraFlags", obj->extra_flags, fread_number_ull(fp));

         if (!str_cmp(word, "ExtraDescr"))
         {
            EXTRA_DESCR_DATA *ed;

            GET_FREE(ed, exdesc_free);
            ed->keyword = fread_string(fp);
            ed->description = fread_string(fp);
            LINK(ed, obj->first_exdesc, obj->last_exdesc, next, prev);
            fMatch = TRUE;
         }

         if (!str_cmp(word, "End"))
         {
            if (!fNest || !fVnum)
            {
               AFFECT_DATA *paf;
               EXTRA_DESCR_DATA *ed;

               monitor_chan("Fread_obj: incomplete object.", MONITOR_BAD);
               while ((paf = obj->first_apply) != NULL)
               {
                  obj->first_apply = paf->next;
                  PUT_FREE(paf, affect_free);
               }
               while ((ed = obj->first_exdesc) != NULL)
               {
                  obj->first_exdesc = ed->next;
                  PUT_FREE(ed, exdesc_free);
               }
               PUT_FREE(obj, obj_free);
               return;
            }
            else
            {
               LINK(obj, first_obj, last_obj, next, prev);
               obj->pIndexData->count++;

               if (Temp_Obj)
               {
                  int newvnum;
                  OBJ_INDEX_DATA *pObjIndex;
                  int nMatch = 0;
                  int vnum;

                  /*
                   * One of three things:
                   * Obj Vnum was deleted
                   * Obj Vnum was moved
                   * Obj Vnum was previously deleted
                   */
                  newvnum = TEMP_VNUM;

                  if (OldVnum != TEMP_VNUM)
                  {
                     /*
                      * Check on move table
                      */
                     if ((newvnum =
                              vnum_from_hash_ref(get_hash_entry(hash_changed_vnums, OldVnum))) != 0)
                     {
                        obj->pIndexData = get_obj_index(newvnum);
                        if (obj->pIndexData == NULL)
                        {
                           obj->pIndexData = get_obj_index(TEMP_VNUM);
                           newvnum = TEMP_VNUM;
                        }
                     }
                  }

                  if (newvnum == TEMP_VNUM)
                  {
                     /*
                      * Scan through objects, trying to find a matching description
                      */
                     for (vnum = 0; nMatch < top_obj_index; vnum++)
                     {
                        if ((pObjIndex = get_obj_index(vnum)) != NULL)
                        {
                           nMatch++;
                           if (!str_cmp(obj->short_descr, pObjIndex->short_descr))
                           {
                              obj->pIndexData = pObjIndex;
                              break;
                           }
                        }
                     }
                  }
               }

               if (iNest == 0 || rgObjNest[iNest] == NULL)
                  obj_to_char(obj, ch);
               else /*
                     * if ( rgObjNest[iNest-1] == obj )
                     * obj_to_char( obj, ch );
                     * else */
                  obj_to_obj(obj, rgObjNest[iNest - 1]);
               return;
            }
         }
         break;

      case 'I':
         KEY("ItemType", obj->item_type, fread_number(fp));
         break;

      case 'L':
         KEY("Level", obj->level, fread_number(fp));
         break;
      case 'N':
         SKEY("Name", obj->name, fread_string(fp));

         if (!str_cmp(word, "Nest"))
         {
            iNest = fread_number(fp);
            if (iNest < 0 || iNest >= MAX_NEST)
            {
               monitor_chan("Fread_obj: bad nest.", MONITOR_BAD);
            }
            else
            {
               rgObjNest[iNest] = obj;
               fNest = TRUE;
            }
            fMatch = TRUE;
         }
         break;

      case 'O':
         /*	   KEY( "Objfun", obj->obj_fun,  obj_fun_lookup( fread_string( fp ) ) );  */
         if (!str_cmp(word, "Objfun"))
         {
            char *dumpme;
            dumpme = fread_string(fp);
            obj->obj_fun = obj_fun_lookup(dumpme);
            free_string(dumpme);
            fMatch = TRUE;
         }
         break;

      case 'S':
         SKEY("ShortDescr", obj->short_descr, fread_string(fp));

         if (!str_cmp(word, "Spell"))
         {
            int iValue;
            int sn;
            const char *spell_word;

            iValue = fread_number(fp);
            spell_word = fread_word(fp);
            sn = skill_lookup(spell_word);
            if (sn < 0)
            {
               const char *new_name = skill_name_legacy(spell_word);
               if (new_name != NULL)
                  sn = skill_lookup(new_name);
            }
            if (iValue < 0 || iValue > 3)
            {
               monitor_chan("Fread_obj: bad iValue ", MONITOR_BAD);
            }
            else if (sn < 0)
            {
               monitor_chan("Fread_obj: unknown skill.", MONITOR_BAD);
            }
            else
            {
               obj->value[iValue] = sn;
            }
            fMatch = TRUE;
            break;
         }

         break;

      case 'T':
         KEY("Timer", obj->timer, fread_number(fp));
         break;

      case 'V':
         if (!str_cmp(word, "Values"))
         {
            int looper;

            if (cur_revision < UPGRADE_REVISION)
            {
               obj->value[0] = fread_number(fp);
               obj->value[1] = fread_number(fp);
               obj->value[2] = fread_number(fp);
               obj->value[3] = fread_number(fp);
               for (looper = 4; looper < 10; obj->value[looper] = 0, looper++)
                  ;
            }
            else
            {
               for (looper = 0; looper < 10; obj->value[looper] = fread_number(fp), looper++)
                  ;
            }
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "Vnum"))
         {
            int vnum;

            vnum = fread_number(fp);

            if ((obj->pIndexData = get_obj_index(vnum)) == NULL || vnum == TEMP_VNUM)
            {
               /*
                * Set flag saying that object is temporary
                */
               Temp_Obj = 1;
               OldVnum = vnum;
               vnum = TEMP_VNUM;
               obj->pIndexData = get_obj_index(vnum);
            }
            /*
             * bug( "Fread_obj: bad vnum %d.", vnum ); This killed it.
             */
            else
               fVnum = TRUE;
            fMatch = TRUE;
            break;
         }
         break;

      case 'W':
         if (!str_cmp(word, "WearLoc"))
         {
            if (cur_revision < UPGRADE_REVISION)
            {
               fread_number(fp);
               obj->wear_loc = -1;
               fMatch = TRUE;
               break;
            }
            else
            {
               KEY("WearLoc", obj->wear_loc, fread_number(fp));
            }
         }
         if (!str_cmp(word, "WearFlags"))
         {
            if (cur_revision < UPGRADE_REVISION)
            {
               int temp_flags, index, new_flags = 0;
               extern const int convert_wearflags[];
               temp_flags = fread_number(fp);
               for (index = 0; index < 32; index++)
               {
                  if (IS_SET(temp_flags, (1 << index)))
                  {
                     SET_BIT(new_flags, convert_wearflags[index]);
                  }
               }
               obj->wear_flags = new_flags;
               fMatch = TRUE;
               break;
            }
            else
            {
               KEY("WearFlags", obj->wear_flags, fread_number(fp));
            }
         }
         KEY("Weight", obj->weight, fread_number(fp));
         break;
      }

      if (!fMatch)
      {
         monitor_chan("Fread_obj: no match.", MONITOR_BAD);
         fread_to_eol(fp);
      }
   }
}

/*
 * Build the file path for a keep chest save file.
 * Path is CHEST_DIR/<vnum>  (e.g. "../data/chest/311").
 * Returns dest on success, NULL if dest_size is too small.
 */
#ifdef UNIT_TEST_SAVE
char *chest_file_path(int vnum, char *dest, size_t dest_size)
#else
static char *chest_file_path(int vnum, char *dest, size_t dest_size)
#endif
{
   int n = snprintf(dest, dest_size, "%s%d", CHEST_DIR, vnum);
   if (n < 0 || (size_t)n >= dest_size)
      return NULL;
   return dest;
}

/*
 * Write one keep chest object (and its contents recursively) to fp.
 * Nest=0 is the chest itself; its contents are written at Nest=1+.
 * Unlike fwrite_corpse, no WhereVnum is written — chest contents are
 * always loaded back into their parent container, never into a room.
 */
static void fwrite_chest(OBJ_DATA *obj, FILE *fp, int iNest)
{
   EXTRA_DESCR_DATA *ed;
   AFFECT_DATA *paf;

   /* Slick recursion: write list backwards so loading restores forward order. */
   if (obj->next_in_carry_list != NULL)
      fwrite_chest(obj->next_in_carry_list, fp, iNest);

   fprintf(fp, "#OBJECT\n");
   fprintf(fp, "Nest         %d\n", iNest);
   fprintf(fp, "Name         %s~\n", obj->name);
   fprintf(fp, "ShortDescr   %s~\n", obj->short_descr);
   fprintf(fp, "Description  %s~\n", obj->description);
   fprintf(fp, "Vnum         %d\n", obj->pIndexData->vnum);
   fprintf(fp, "ExtraFlags   %llu\n", obj->extra_flags);
   fprintf(fp, "WearFlags    %d\n", obj->wear_flags);
   fprintf(fp, "WearLoc      %d\n", obj->wear_loc);
   if (obj->obj_fun != NULL)
      fprintf(fp, "Objfun       %s~\n", rev_obj_fun_lookup(obj->obj_fun));
   fprintf(fp, "ClassFlags   %d\n", obj->item_apply);
   fprintf(fp, "ItemType     %d\n", obj->item_type);
   fprintf(fp, "Weight       %d\n", obj->weight);
   fprintf(fp, "Level        %d\n", obj->level);
   fprintf(fp, "Timer        %d\n", obj->timer);
   fprintf(fp, "Cost         %d\n", obj->cost);
   fprintf(fp, "Values       %d %d %d %d\n", obj->value[0], obj->value[1], obj->value[2],
           obj->value[3]);

   switch (obj->item_type)
   {
   case ITEM_POTION:
   case ITEM_SCROLL:
      if (obj->value[1] > 0)
         fprintf(fp, "Spell 1      '%s'\n", skill_table[obj->value[1]].name);
      if (obj->value[2] > 0)
         fprintf(fp, "Spell 2      '%s'\n", skill_table[obj->value[2]].name);
      if (obj->value[3] > 0)
         fprintf(fp, "Spell 3      '%s'\n", skill_table[obj->value[3]].name);
      break;
   case ITEM_PILL:
   case ITEM_STAFF:
      if (obj->value[3] > 0)
         fprintf(fp, "Spell 3      '%s'\n", skill_table[obj->value[3]].name);
      break;
   }

   for (paf = obj->first_apply; paf != NULL; paf = paf->next)
      fprintf(fp, "Affect       %d %d %d %d %d\n", paf->type, paf->duration, paf->modifier,
              paf->location, paf->bitvector);

   for (ed = obj->first_exdesc; ed != NULL; ed = ed->next)
      fprintf(fp, "ExtraDescr   %s~ %s~\n", ed->keyword, ed->description);

   fprintf(fp, "End\n\n");

   if (obj->first_in_carry_list != NULL)
      fwrite_chest(obj->first_in_carry_list, fp, iNest + 1);
}

/*
 * Read one #OBJECT block from fp and place the item into its parent container
 * using rgObjNest[].  rgObjNest[0] must be set to the chest before calling.
 * Items at Nest=0 are silently skipped (the chest itself already exists).
 */
#ifdef UNIT_TEST_SAVE
void fread_chest_item(FILE *fp)
#else
static void fread_chest_item(FILE *fp)
#endif
{
   static OBJ_DATA obj_zero;
   OBJ_DATA *obj;
   char *word;
   int iNest;
   bool fMatch;
   bool fNest;
   bool fVnum;
   int Temp_Obj = 0, OldVnum = 0;

   GET_FREE(obj, obj_free);
   *obj = obj_zero;
   obj->name = str_dup("");
   obj->short_descr = str_dup("");
   obj->description = str_dup("");
   fNest = FALSE;
   fVnum = TRUE;
   iNest = 0;

   init_changed_vnum_hash();

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
         if (!str_cmp(word, "Affect"))
         {
            AFFECT_DATA *paf;

            GET_FREE(paf, affect_free);
            paf->type = fread_number(fp);
            paf->duration = fread_number(fp);
            paf->modifier = fread_number(fp);
            paf->location = fread_number(fp);
            paf->bitvector = fread_number(fp);
            LINK(paf, obj->first_apply, obj->last_apply, next, prev);
            fMatch = TRUE;
            break;
         }
         break;

      case 'C':
         KEY("Cost", obj->cost, fread_number(fp));
         KEY("ClassFlags", obj->item_apply, fread_number(fp));
         break;

      case 'D':
         SKEY("Description", obj->description, fread_string(fp));
         break;

      case 'E':
         KEY("ExtraFlags", obj->extra_flags, fread_number_ull(fp));

         if (!str_cmp(word, "ExtraDescr"))
         {
            EXTRA_DESCR_DATA *ed;

            GET_FREE(ed, exdesc_free);
            ed->keyword = fread_string(fp);
            ed->description = fread_string(fp);
            LINK(ed, obj->first_exdesc, obj->last_exdesc, next, prev);
            fMatch = TRUE;
         }

         if (!str_cmp(word, "End"))
         {
            if (!fNest || !fVnum)
            {
               AFFECT_DATA *paf;
               EXTRA_DESCR_DATA *ed;

               monitor_chan("fread_chest_item: incomplete object.", MONITOR_BAD);
               while ((paf = obj->first_apply) != NULL)
               {
                  obj->first_apply = paf->next;
                  PUT_FREE(paf, affect_free);
               }
               while ((ed = obj->first_exdesc) != NULL)
               {
                  obj->first_exdesc = ed->next;
                  PUT_FREE(ed, exdesc_free);
               }
               PUT_FREE(obj, obj_free);
               return;
            }
            else
            {
               /* iNest == 0 means this is the chest record itself — skip it. */
               if (iNest == 0)
               {
                  AFFECT_DATA *paf;
                  EXTRA_DESCR_DATA *ed;

                  while ((paf = obj->first_apply) != NULL)
                  {
                     obj->first_apply = paf->next;
                     PUT_FREE(paf, affect_free);
                  }
                  while ((ed = obj->first_exdesc) != NULL)
                  {
                     obj->first_exdesc = ed->next;
                     PUT_FREE(ed, exdesc_free);
                  }
                  PUT_FREE(obj,
                           obj_free); /* obj_free_destructor frees name/short_descr/description */
                  return;
               }

               LINK(obj, first_obj, last_obj, next, prev);
               obj->pIndexData->count++;

               if (Temp_Obj)
               {
                  int newvnum;
                  OBJ_INDEX_DATA *pObjIndex;
                  int nMatch = 0;
                  int vnum;

                  newvnum = TEMP_VNUM;

                  if (OldVnum != TEMP_VNUM)
                  {
                     if ((newvnum =
                              vnum_from_hash_ref(get_hash_entry(hash_changed_vnums, OldVnum))) != 0)
                     {
                        obj->pIndexData = get_obj_index(newvnum);
                        if (obj->pIndexData == NULL)
                        {
                           obj->pIndexData = get_obj_index(TEMP_VNUM);
                           newvnum = TEMP_VNUM;
                        }
                     }
                  }

                  if (newvnum == TEMP_VNUM)
                  {
                     for (vnum = 0; nMatch < top_obj_index; vnum++)
                     {
                        if ((pObjIndex = get_obj_index(vnum)) != NULL)
                        {
                           nMatch++;
                           if (!str_cmp(obj->short_descr, pObjIndex->short_descr))
                           {
                              obj->pIndexData = pObjIndex;
                              break;
                           }
                        }
                     }
                  }
               }

               /* Place item into parent container using nesting array. */
               if (rgObjNest[iNest - 1] != NULL)
                  obj_to_obj(obj, rgObjNest[iNest - 1]);
               else
               {
                  monitor_chan("fread_chest_item: missing parent container in nest.", MONITOR_BAD);
                  obj->pIndexData->count--;
                  UNLINK(obj, first_obj, last_obj, next, prev);
                  PUT_FREE(obj,
                           obj_free); /* obj_free_destructor frees name/short_descr/description */
               }
               return;
            }
         }
         break;

      case 'I':
         KEY("ItemType", obj->item_type, fread_number(fp));
         break;

      case 'L':
         KEY("Level", obj->level, fread_number(fp));
         break;

      case 'N':
         KEY("Name", obj->name, fread_string(fp));

         if (!str_cmp(word, "Nest"))
         {
            iNest = fread_number(fp);
            if (iNest < 0 || iNest >= MAX_NEST)
            {
               monitor_chan("fread_chest_item: bad nest.", MONITOR_BAD);
            }
            else
            {
               /*
                * Do NOT overwrite rgObjNest[0] — load_chest() seeds that
                * slot with the already-instantiated chest object.  The
                * Nest=0 record is the chest descriptor itself and is
                * discarded; clobbering slot 0 would leave it dangling after
                * PUT_FREE, causing obj_to_obj to write into freed memory
                * for every Nest=1 item that follows.
                */
               if (iNest > 0)
                  rgObjNest[iNest] = obj;
               fNest = TRUE;
            }
            fMatch = TRUE;
         }
         break;

      case 'O':
         if (!str_cmp(word, "Objfun"))
         {
            char *dumpme;
            dumpme = fread_string(fp);
            obj->obj_fun = obj_fun_lookup(dumpme);
            free_string(dumpme);
            fMatch = TRUE;
         }
         break;

      case 'S':
         SKEY("ShortDescr", obj->short_descr, fread_string(fp));

         if (!str_cmp(word, "Spell"))
         {
            int iValue;
            int sn;
            const char *spell_word;

            iValue = fread_number(fp);
            spell_word = fread_word(fp);
            sn = skill_lookup(spell_word);
            if (sn < 0)
            {
               const char *new_name = skill_name_legacy(spell_word);
               if (new_name != NULL)
                  sn = skill_lookup(new_name);
            }
            if (iValue < 0 || iValue > 3)
               monitor_chan("fread_chest_item: bad iValue", MONITOR_BAD);
            else if (sn < 0)
               monitor_chan("fread_chest_item: unknown skill.", MONITOR_BAD);
            else
               obj->value[iValue] = sn;
            fMatch = TRUE;
            break;
         }
         break;

      case 'T':
         KEY("Timer", obj->timer, fread_number(fp));
         break;

      case 'V':
         if (!str_cmp(word, "Values"))
         {
            obj->value[0] = fread_number(fp);
            obj->value[1] = fread_number(fp);
            obj->value[2] = fread_number(fp);
            obj->value[3] = fread_number(fp);
            fMatch = TRUE;
            break;
         }

         if (!str_cmp(word, "Vnum"))
         {
            int vnum;

            vnum = fread_number(fp);

            if ((obj->pIndexData = get_obj_index(vnum)) == NULL || vnum == TEMP_VNUM)
            {
               Temp_Obj = 1;
               OldVnum = vnum;
               vnum = TEMP_VNUM;
               obj->pIndexData = get_obj_index(vnum);
            }
            else
               fVnum = TRUE;
            fMatch = TRUE;
            break;
         }
         break;

      case 'W':
         KEY("WearFlags", obj->wear_flags, fread_number(fp));
         KEY("WearLoc", obj->wear_loc, fread_number(fp));
         KEY("Weight", obj->weight, fread_number(fp));
         break;
      }

      if (!fMatch)
      {
         monitor_chan("fread_chest_item: no match.", MONITOR_BAD);
         fread_to_eol(fp);
      }
   }
}

/*
 * Save a single keep chest (and its contents) to CHEST_DIR/<vnum>.
 * Uses write-to-temp-then-rename for atomicity.
 */
void save_chest(OBJ_DATA *chest)
{
   FILE *fp;
   char path[MAX_STRING_LENGTH];
   char temp_path[MAX_STRING_LENGTH];
   int n;

   if (chest == NULL)
      return;
   if (chest->item_type != ITEM_CONTAINER || !IS_SET(chest->value[1], CONT_KEEP_CHEST))
      return;

   if (chest_file_path(chest->pIndexData->vnum, path, sizeof(path)) == NULL)
   {
      bug("save_chest: path too long for vnum %d", chest->pIndexData->vnum);
      return;
   }

   n = snprintf(temp_path, sizeof(temp_path), "%s.temp", path);
   if (n < 0 || (size_t)n >= sizeof(temp_path))
   {
      bug("save_chest: temp path too long for vnum %d", chest->pIndexData->vnum);
      return;
   }

   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }

   if ((fp = fopen(temp_path, "w")) == NULL)
   {
      bug("save_chest: fopen failed for %s", 0);
      perror(temp_path);
      return;
   }

   fwrite_chest(chest, fp, 0);
   fprintf(fp, "#END\n\n");
   fflush(fp);
   fclose(fp);

   rename(temp_path, path);
   db_worker_save_chest(chest);
}

/*
 * Test helper: write a minimal keep chest #OBJECT block to fp using the
 * exact field names produced by fwrite_chest.  This lets test_save.c verify
 * the format contract (e.g. presence of Nest, Vnum, End; absence of
 * WhereVnum) without needing skill_table or rev_obj_fun_lookup in the link.
 */
#ifdef UNIT_TEST_SAVE
void fwrite_chest_minimal_for_test(FILE *fp, int vnum, const char *name, int nest)
{
   fprintf(fp, "#OBJECT\n");
   fprintf(fp, "Nest         %d\n", nest);
   fprintf(fp, "Name         %s~\n", name);
   fprintf(fp, "ShortDescr   %s~\n", name);
   fprintf(fp, "Description  %s~\n", name);
   fprintf(fp, "Vnum         %d\n", vnum);
   fprintf(fp, "ExtraFlags   0\n");
   fprintf(fp, "WearFlags    0\n");
   fprintf(fp, "WearLoc      0\n");
   fprintf(fp, "ClassFlags   0\n");
   fprintf(fp, "ItemType     0\n");
   fprintf(fp, "Weight       0\n");
   fprintf(fp, "Level        0\n");
   fprintf(fp, "Timer        0\n");
   fprintf(fp, "Cost         0\n");
   fprintf(fp, "Values       0 0 0 0\n");
   fprintf(fp, "End\n\n");
}
#endif

/*
 * Delete the save file for a keep chest (called when the chest is destroyed).
 */
void delete_chest_file(int vnum)
{
   char path[MAX_STRING_LENGTH];
   if (chest_file_path(vnum, path, sizeof(path)) != NULL)
      unlink(path);
}

/*
 * Load the contents of a keep chest from CHEST_DIR/<vnum>.
 * The chest object must already exist in the world (placed by create_object).
 * The first #OBJECT block describes the chest itself and is silently
 * discarded by fread_chest_item (Nest=0).  Subsequent blocks are the
 * chest's contents and are placed into the chest via obj_to_obj.
 *
 * Called from create_object() whenever a keep chest object is created
 * so the chest is populated with its saved contents immediately.
 */
void load_chest(int vnum)
{
   FILE *fp;
   char path[MAX_STRING_LENGTH];
   OBJ_DATA *obj;
   int i;

   if (chest_file_path(vnum, path, sizeof(path)) == NULL)
      return;
   if ((fp = fopen(path, "r")) == NULL)
      return;

   /* Locate the already-created chest object in the global object list. */
   for (obj = first_obj; obj != NULL; obj = obj->next)
   {
      if (obj->pIndexData != NULL && obj->pIndexData->vnum == vnum &&
          obj->item_type == ITEM_CONTAINER && IS_SET(obj->value[1], CONT_KEEP_CHEST))
         break;
   }

   /*
    * Pre-populate the nesting array so that Nest=1 items are placed inside
    * the existing chest rather than into a room.
    */
   for (i = 0; i < MAX_NEST; i++)
      rgObjNest[i] = NULL;
   if (obj != NULL)
      rgObjNest[0] = obj;

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
         break;
      word = fread_word(fp);
      if (!str_cmp(word, "END"))
         break;
      if (str_cmp(word, "OBJECT"))
         break;
      fread_chest_item(fp);
   }

   fclose(fp);
}
