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
 * save_mobs.c — Mob corpse (persistent container) save and load.
 *
 * Handles serialisation and deserialisation of mob corpses that persist
 * across server reboots:
 *   fwrite_corpse / fread_corpse  — per-corpse serialisation
 *   save_corpses                  — write all corpses to CORPSE_FILE
 */

#include "save.h"

extern int top_obj_index;

static int resolve_persistent_container_room_vnum(int room_vnum)
{
   if (get_room_index(room_vnum) != NULL)
      return room_vnum;

   return ROOM_VNUM_LIMBO;
}

static int persistent_container_where_vnum_for_save(int in_room_vnum, bool in_obj)
{
   if (in_obj)
      return ROOM_VNUM_MORGUE;

   if (in_room_vnum > 0)
      return in_room_vnum;

   return ROOM_VNUM_MORGUE;
}

#ifdef UNIT_TEST_SAVE
int resolve_persistent_container_room_vnum_for_test(int room_vnum)
{
   return resolve_persistent_container_room_vnum(room_vnum);
}

int persistent_container_where_vnum_for_save_for_test(int in_room_vnum, int in_obj)
{
   return persistent_container_where_vnum_for_save(in_room_vnum, in_obj != 0);
}
#endif

void fread_corpse(FILE *fp)
{
   static OBJ_DATA obj_zero;
   OBJ_DATA *obj;
   char *word;
   int iNest;
   bool fMatch;
   bool fNest;
   bool fVnum;
   int Temp_Obj = 0, OldVnum = 0;
   int this_room_vnum;

   GET_FREE(obj, obj_free);
   *obj = obj_zero;
   obj->name = str_dup("");
   obj->short_descr = str_dup("");
   obj->description = str_dup("");
   fNest = FALSE;
   fVnum = TRUE;
   iNest = 0;
   this_room_vnum = 0;

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
               {
                  ROOM_INDEX_DATA *target_room;
                  int target_room_vnum = resolve_persistent_container_room_vnum(this_room_vnum);

                  target_room = get_room_index(target_room_vnum);
                  if (target_room == NULL)
                     target_room = get_room_index(ROOM_VNUM_LIMBO);

                  if (target_room == NULL)
                  {
                     monitor_chan("Fread_corpse: limbo room missing for persistent container.",
                                  MONITOR_BAD);
                     obj->pIndexData->count--;
                     UNLINK(obj, first_obj, last_obj, next, prev);
                     PUT_FREE(obj, obj_free);
                     return;
                  }

                  obj_to_room(obj, target_room);
               }
               else
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
         KEY("Name", obj->name, fread_string(fp));

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
         KEY("WearFlags", obj->wear_flags, fread_number(fp));
         KEY("WearLoc", obj->wear_loc, fread_number(fp));
         KEY("Weight", obj->weight, fread_number(fp));
         KEY("WhereVnum", this_room_vnum, fread_number(fp));
         break;
      }

      if (!fMatch)
      {
         monitor_chan("Fread_obj: no match.", MONITOR_BAD);
         fread_to_eol(fp);
      }
   }
}

void fwrite_corpse(OBJ_DATA *obj, FILE *fp, int iNest)
{
   EXTRA_DESCR_DATA *ed;
   AFFECT_DATA *paf;
   int where_vnum;
   /*
    * Slick recursion to write lists backwards,
    *   so loading them will load in forwards order.
    */
   if (obj->next_in_carry_list != NULL)
      fwrite_corpse(obj->next_in_carry_list, fp, iNest);

   where_vnum = persistent_container_where_vnum_for_save(
       obj->in_room != NULL ? obj->in_room->vnum : 0, obj->in_obj != NULL);

   if (obj->in_room == NULL && obj->in_obj == NULL)
      obj->in_room = get_room_index(ROOM_VNUM_LIMBO);

   if (obj->pIndexData == NULL)
   {
      bug("fwrite_corpse: obj '%s' has NULL pIndexData, skipping", (int)obj->name);
      return;
   }

   fprintf(fp, "#OBJECT\n");
   fprintf(fp, "WhereVnum    %d\n", where_vnum);

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
   fprintf(fp, "Values       %d %d %d %d\n", obj->value[0], obj->value[1], obj->value[2],
           obj->value[3]);

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
      fwrite_corpse(obj->first_in_carry_list, fp, iNest + 1);

   return;
}

void save_corpses()
{

   FILE *fp;
   char corpse_file_name[MAX_STRING_LENGTH];
   CORPSE_DATA *this_corpse;

   if (fpReserve != NULL)
   {
      fclose(fpReserve);
      fpReserve = NULL;
   }
   sprintf(corpse_file_name, "%s", CORPSE_FILE);

   if ((fp = fopen(corpse_file_name, "w")) == NULL)
   {
      bug("Save Corpses: fopen", 0);
      perror("failed open of corpse_file in save_corpses");
   }
   else
   {
      for (this_corpse = first_corpse; this_corpse != NULL; this_corpse = this_corpse->next)
      {
         /* Keep chests are saved to their own per-vnum files in CHEST_DIR. */
         if (this_corpse->this_corpse->item_type == ITEM_CONTAINER &&
             IS_SET(this_corpse->this_corpse->value[1], CONT_KEEP_CHEST))
            continue;
         fwrite_corpse(this_corpse->this_corpse, fp, 0);
      }
      fprintf(fp, "#END\n\n");

      fflush(fp);
      if (fp != NULL)
      {
         fclose(fp);
         fp = NULL;
      }
   }
   return;
}

void load_corpses(void)
{

   FILE *corpsefp;
   char corpse_file_name[MAX_STRING_LENGTH];
   char buf[MAX_STRING_LENGTH];

   snprintf(corpse_file_name, sizeof(corpse_file_name), "%s", CORPSE_FILE);

   db_format_status(buf, sizeof(buf), "Loading", corpse_file_name);
   log_f("%s", buf);

   if ((corpsefp = fopen(corpse_file_name, "r")) == NULL)
   {
      log_f("Load corpse Table: fopen");
      perror("failed open of corpse_table.dat in load_corpse_table");
   }

   else
   {
      fpArea = corpsefp;
      db_set_area_name(corpse_file_name);

      for (;;)

      {
         char letter;
         char *word;

         letter = fread_letter(corpsefp);
         if (letter == '*')
         {
            fread_to_eol(corpsefp);
            continue;
         }

         if (letter != '#')
         {
            log_f("Load_char_obj: # not found.");
            break;
         }

         word = fread_word(corpsefp);
         if (!str_cmp(word, "OBJECT"))
            fread_corpse(corpsefp);
         else if (!str_cmp(word, "END"))
            break;
         else
         {
            log_f("Load_char_obj: bad section.");
            break;
         }
      }
   }
   if (corpsefp != NULL)
   {
      fclose(corpsefp);
      corpsefp = NULL;
   }
   fpArea = NULL;
   db_format_status(buf, sizeof(buf), "Done Loading", corpse_file_name);
   monitor_chan(buf, MONITOR_CLAN);
}
