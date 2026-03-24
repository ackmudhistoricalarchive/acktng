/* db_load.c — ACK!TNG PostgreSQL boot-time loader.
 *
 * Implements the db_load_* family that replaces the flat-file loaders called
 * from boot_db() when compiled with -DHAVE_LIBPQ.
 *
 * All functions use the synchronous boot connection returned by db_conn_get().
 * They must be called after db_conn_open() and before db_conn_close() / the
 * async worker is started.
 *
 * Per-area content is loaded inside db_load_areas_from_db() via static
 * helpers so that area_load always points to the current area — matching the
 * invariant expected by fix_exits(), check_resets(), and the OLC editor.
 */

#include "globals.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

#include "db_conn.h"
#include "db_load.h"
#include "../quests/quest_internal.h"

/* -----------------------------------------------------------------------
 * Internal SQL helpers
 * ----------------------------------------------------------------------- */

/* Execute a query with no parameters; caller must PQclear the result.
 * Returns NULL and logs on error. */
static PGresult *xq(const char *sql)
{
   PGconn *db = db_conn_get();
   PGresult *res;

   if (!db)
   {
      fprintf(stderr, "db_load: xq called with no connection\n");
      return NULL;
   }
   res = PQexec(db, sql);
   if (PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      fprintf(stderr, "db_load SQL error [%.80s]: %s\n", sql, PQresultErrorMessage(res));
      PQclear(res);
      return NULL;
   }
   return res;
}

/* Execute a parameterised query; caller must PQclear the result. */
static PGresult *xqp(const char *sql, int n, const char *const *vals)
{
   PGconn *db = db_conn_get();
   PGresult *res;

   if (!db)
   {
      fprintf(stderr, "db_load: xqp called with no connection\n");
      return NULL;
   }
   res = PQexecParams(db, sql, n, NULL, vals, NULL, NULL, 0);
   if (PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      fprintf(stderr, "db_load SQL error [%.80s]: %s\n", sql, PQresultErrorMessage(res));
      PQclear(res);
      return NULL;
   }
   return res;
}

/* -----------------------------------------------------------------------
 * Per-area sub-loader: rooms
 * ----------------------------------------------------------------------- */

static void load_exits_for_room(const char *vnum_str, ROOM_INDEX_DATA *pRoomIndex)
{
   const char *v[1];
   PGresult *res;
   int i, n;

   v[0] = vnum_str;
   res = xqp("SELECT direction, dest_vnum, exit_flags, key_vnum, keyword, description "
             "FROM room_exits WHERE room_vnum=$1 ORDER BY direction",
             1, v);
   if (!res)
      return;

   n = PQntuples(res);
   for (i = 0; i < n; i++)
   {
      int dir = atoi(PQgetvalue(res, i, 0));
      int dest = atoi(PQgetvalue(res, i, 1));
      int exit_flags = atoi(PQgetvalue(res, i, 2));
      int key_vnum = atoi(PQgetvalue(res, i, 3));
      const char *ekey = PQgetvalue(res, i, 4);
      const char *edesc = PQgetvalue(res, i, 5);
      EXIT_DATA *pexit;

      if (dir < 0 || dir > 5)
      {
         fprintf(stderr, "db_load: invalid exit direction %d for room %s\n", dir, vnum_str);
         continue;
      }

      GET_FREE(pexit, exit_free);
      pexit->to_room = NULL; /* resolved by fix_exits() */
      pexit->vnum = (unsigned short)dest;
      pexit->exit_info = (sh_int)exit_flags;
      pexit->key = (sh_int)key_vnum;
      pexit->keyword = str_dup(ekey);
      pexit->description = str_dup(edesc);
      pRoomIndex->exit[dir] = pexit;
   }
   PQclear(res);
}

static void load_extra_descs_for_room(const char *vnum_str, ROOM_INDEX_DATA *pRoomIndex)
{
   const char *v[1];
   PGresult *res;
   int i, n;

   v[0] = vnum_str;
   res = xqp("SELECT keyword, description FROM room_extra_descs WHERE room_vnum=$1", 1, v);
   if (!res)
      return;

   n = PQntuples(res);
   for (i = 0; i < n; i++)
   {
      EXTRA_DESCR_DATA *ed;
      GET_FREE(ed, exdesc_free);
      ed->keyword = str_dup(PQgetvalue(res, i, 0));
      ed->description = str_dup(PQgetvalue(res, i, 1));
      LINK(ed, pRoomIndex->first_exdesc, pRoomIndex->last_exdesc, next, prev);
   }
   PQclear(res);
}

static void load_rooms_for_area(const char *area_id_str, AREA_DATA *pArea)
{
   const char *v[1];
   PGresult *rooms;
   int r, nrooms;

   v[0] = area_id_str;
   rooms = xqp("SELECT vnum, name, description, room_flags, sector_type "
               "FROM rooms WHERE area_id=$1 ORDER BY vnum",
               1, v);
   if (!rooms)
      return;

   nrooms = PQntuples(rooms);
   for (r = 0; r < nrooms; r++)
   {
      int vnum = atoi(PQgetvalue(rooms, r, 0));
      const char *name = PQgetvalue(rooms, r, 1);
      const char *description = PQgetvalue(rooms, r, 2);
      int room_flags = atoi(PQgetvalue(rooms, r, 3));
      int sector_type = atoi(PQgetvalue(rooms, r, 4));
      int iHash;
      ROOM_INDEX_DATA *pRoomIndex;
      BUILD_DATA_LIST *bdl;
      char vnum_str[16];

      GET_FREE(pRoomIndex, rid_free);
      pRoomIndex->area = pArea;
      pRoomIndex->vnum = (unsigned short)vnum;
      pRoomIndex->name = str_dup(name);
      pRoomIndex->description = str_dup(description);
      pRoomIndex->room_flags = room_flags;
      pRoomIndex->sector_type = (sh_int)(sector_type == SECT_NULL ? SECT_INSIDE : sector_type);
      pRoomIndex->light = 0;
      pRoomIndex->affected_by = ROOM_BV_NONE;
      pRoomIndex->auto_message = NULL;
      pRoomIndex->block_timer = 0;
      pRoomIndex->gold = 0;

      snprintf(vnum_str, sizeof(vnum_str), "%d", vnum);
      load_exits_for_room(vnum_str, pRoomIndex);
      load_extra_descs_for_room(vnum_str, pRoomIndex);

      iHash = vnum % MAX_KEY_HASH;
      SING_TOPLINK(pRoomIndex, room_index_hash[iHash], next);

      GET_FREE(bdl, build_free);
      bdl->data = pRoomIndex;
      LINK(bdl, pArea->first_area_room, pArea->last_area_room, next, prev);

      top_room++;
   }
   PQclear(rooms);
}

/* -----------------------------------------------------------------------
 * Per-area sub-loader: mobiles
 * ----------------------------------------------------------------------- */

static void load_mobiles_for_area(const char *area_id_str, AREA_DATA *pArea)
{
   const char *v[1];
   PGresult *mobs;
   int r, nmobs;

   v[0] = area_id_str;
   mobs = xqp("SELECT vnum, player_name, short_descr, long_descr, description, "
              "act_flags, affected_by, alignment, level, sex, "
              "hp_mod, ac_mod, hr_mod, dr_mod, "
              "class, clan, race, position, skills, cast_flags, def, "
              "strong_magic, weak_magic, race_mods, power_skills, power_cast, resist, suscept, "
              "spellpower, crit, crit_mult, spell_crit, spell_mult, parry, dodge, block, pierce, "
              "ai_knowledge, accent, ai_prompt, "
              "loot_amount, loot_0, loot_1, loot_2, loot_3, loot_4, "
              "loot_5, loot_6, loot_7, loot_8, "
              "loot_chance_0, loot_chance_1, loot_chance_2, loot_chance_3, loot_chance_4, "
              "loot_chance_5, loot_chance_6, loot_chance_7, loot_chance_8 "
              "FROM mobiles WHERE area_id=$1 ORDER BY vnum",
              1, v);
   if (!mobs)
      return;

   nmobs = PQntuples(mobs);
   for (r = 0; r < nmobs; r++)
   {
      int col = 0;
      int vnum = atoi(PQgetvalue(mobs, r, col++));
      const char *player_name = PQgetvalue(mobs, r, col++);
      const char *short_descr = PQgetvalue(mobs, r, col++);
      const char *long_descr = PQgetvalue(mobs, r, col++);
      const char *description = PQgetvalue(mobs, r, col++);
      long long act_flags = atoll(PQgetvalue(mobs, r, col++));
      int affected_by = atoi(PQgetvalue(mobs, r, col++));
      int alignment = atoi(PQgetvalue(mobs, r, col++));
      int level = atoi(PQgetvalue(mobs, r, col++));
      int sex = atoi(PQgetvalue(mobs, r, col++));
      int hp_mod = atoi(PQgetvalue(mobs, r, col++));
      int ac_mod = atoi(PQgetvalue(mobs, r, col++));
      int hr_mod = atoi(PQgetvalue(mobs, r, col++));
      int dr_mod = atoi(PQgetvalue(mobs, r, col++));
      int mob_class = atoi(PQgetvalue(mobs, r, col++));
      int mob_clan = atoi(PQgetvalue(mobs, r, col++));
      int mob_race = atoi(PQgetvalue(mobs, r, col++));
      int position = atoi(PQgetvalue(mobs, r, col++));
      int skills = atoi(PQgetvalue(mobs, r, col++));
      int cast_flags = atoi(PQgetvalue(mobs, r, col++));
      int def = atoi(PQgetvalue(mobs, r, col++));
      int strong_magic = atoi(PQgetvalue(mobs, r, col++));
      int weak_magic = atoi(PQgetvalue(mobs, r, col++));
      int race_mods = atoi(PQgetvalue(mobs, r, col++));
      int power_skills = atoi(PQgetvalue(mobs, r, col++));
      int power_cast = atoi(PQgetvalue(mobs, r, col++));
      int resist = atoi(PQgetvalue(mobs, r, col++));
      int suscept = atoi(PQgetvalue(mobs, r, col++));
      int spellpower = atoi(PQgetvalue(mobs, r, col++));
      int crit = atoi(PQgetvalue(mobs, r, col++));
      int crit_mult = atoi(PQgetvalue(mobs, r, col++));
      int spell_crit = atoi(PQgetvalue(mobs, r, col++));
      int spell_mult = atoi(PQgetvalue(mobs, r, col++));
      int parry = atoi(PQgetvalue(mobs, r, col++));
      int dodge = atoi(PQgetvalue(mobs, r, col++));
      int block = atoi(PQgetvalue(mobs, r, col++));
      int pierce = atoi(PQgetvalue(mobs, r, col++));
      int ai_knowledge = atoi(PQgetvalue(mobs, r, col++));
      int accent = atoi(PQgetvalue(mobs, r, col++));
      const char *ai_prompt = PQgetvalue(mobs, r, col++);
      int loot_amount = atoi(PQgetvalue(mobs, r, col++));
      int loot_vnum[MAX_LOOT], loot_chance[MAX_LOOT];
      int i, iHash;
      MOB_INDEX_DATA *pMobIndex;
      BUILD_DATA_LIST *bdl;

      for (i = 0; i < MAX_LOOT; i++)
         loot_vnum[i] = atoi(PQgetvalue(mobs, r, col++));
      for (i = 0; i < MAX_LOOT; i++)
         loot_chance[i] = atoi(PQgetvalue(mobs, r, col++));

      GET_FREE(pMobIndex, mid_free);
      pMobIndex->area = pArea;
      pMobIndex->vnum = (unsigned short)vnum;
      pMobIndex->player_name = str_dup(player_name);
      pMobIndex->short_descr = str_dup(short_descr);
      pMobIndex->long_descr = str_dup(long_descr);
      pMobIndex->description = str_dup(description);
      pMobIndex->act = (unsigned long long)act_flags | ACT_IS_NPC;
      pMobIndex->affected_by = affected_by;
      pMobIndex->alignment = (sh_int)alignment;
      pMobIndex->level = (sh_int)level;
      pMobIndex->sex = (sh_int)sex;
      pMobIndex->hp_mod = hp_mod;
      pMobIndex->ac_mod = ac_mod;
      pMobIndex->hr_mod = hr_mod;
      pMobIndex->dr_mod = dr_mod;
      pMobIndex->class = (sh_int)mob_class;
      pMobIndex->clan = (sh_int)mob_clan;
      pMobIndex->race = (sh_int)mob_race;
      pMobIndex->position = (sh_int)position;
      pMobIndex->skills = skills;
      pMobIndex->cast = cast_flags;
      pMobIndex->def = def;
      pMobIndex->strong_magic = strong_magic;
      pMobIndex->weak_magic = weak_magic;
      pMobIndex->race_mods = race_mods;
      pMobIndex->power_skills = power_skills;
      pMobIndex->power_cast = power_cast;
      pMobIndex->resist = resist;
      pMobIndex->suscept = suscept;
      pMobIndex->spellpower_mod = spellpower;
      pMobIndex->crit_mod = crit;
      pMobIndex->crit_mult_mod = crit_mult;
      pMobIndex->spell_crit_mod = spell_crit;
      pMobIndex->spell_mult_mod = spell_mult;
      pMobIndex->parry_mod = parry;
      pMobIndex->dodge_mod = dodge;
      pMobIndex->block_mod = block;
      pMobIndex->pierce_mod = pierce;
      pMobIndex->ai_knowledge = ai_knowledge;
      pMobIndex->accent = (sh_int)accent;
      pMobIndex->ai_prompt = (ai_prompt && *ai_prompt) ? str_dup(ai_prompt) : NULL;
      pMobIndex->loot_amount = loot_amount;
      for (i = 0; i < MAX_LOOT; i++)
      {
         pMobIndex->loot[i] = loot_vnum[i];
         pMobIndex->loot_chance[i] = loot_chance[i];
      }
      pMobIndex->pShop = NULL;
      pMobIndex->spec_fun = NULL;
      pMobIndex->speech_fun = NULL;

      iHash = vnum % MAX_KEY_HASH;
      SING_TOPLINK(pMobIndex, mob_index_hash[iHash], next);

      GET_FREE(bdl, build_free);
      bdl->data = pMobIndex;
      LINK(bdl, pArea->first_area_mobile, pArea->last_area_mobile, next, prev);

      top_mob_index++;
      kill_table[level].number++;
   }
   PQclear(mobs);
}

/* -----------------------------------------------------------------------
 * Per-area sub-loaders: objects, resets, shops, specials, objfuns
 * ----------------------------------------------------------------------- */

static void load_objects_for_area(const char *area_id_str, AREA_DATA *pArea)
{
   const char *v[1];
   PGresult *objs;
   int r, nobjs;

   v[0] = area_id_str;
   objs = xqp(
       "SELECT vnum, name, short_descr, description, "
       "item_type, extra_flags, wear_flags, item_apply, "
       "value_0, value_1, value_2, value_3, value_4, value_5, value_6, value_7, value_8, value_9, "
       "weight, level "
       "FROM objects WHERE area_id=$1 ORDER BY vnum",
       1, v);
   if (!objs)
      return;

   nobjs = PQntuples(objs);
   for (r = 0; r < nobjs; r++)
   {
      int col = 0;
      int vnum = atoi(PQgetvalue(objs, r, col++));
      const char *name = PQgetvalue(objs, r, col++);
      const char *short_descr = PQgetvalue(objs, r, col++);
      const char *description = PQgetvalue(objs, r, col++);
      int item_type = atoi(PQgetvalue(objs, r, col++));
      long long extra_flags = atoll(PQgetvalue(objs, r, col++));
      int wear_flags = atoi(PQgetvalue(objs, r, col++));
      int item_apply = atoi(PQgetvalue(objs, r, col++));
      int val[10];
      int weight, obj_level;
      int i, iHash;
      OBJ_INDEX_DATA *pObjIndex;
      BUILD_DATA_LIST *bdl;
      PGresult *aff_res, *ed_res;
      char vnum_str[16];
      const char *vv[1];

      for (i = 0; i < 10; i++)
         val[i] = atoi(PQgetvalue(objs, r, col++));
      weight = atoi(PQgetvalue(objs, r, col++));
      obj_level = atoi(PQgetvalue(objs, r, col++));

      GET_FREE(pObjIndex, oid_free);
      pObjIndex->area = pArea;
      pObjIndex->vnum = (unsigned short)vnum;
      pObjIndex->name = str_dup(name);
      pObjIndex->short_descr = str_dup(short_descr);
      pObjIndex->description = str_dup(description);
      pObjIndex->owner = str_dup("");
      pObjIndex->item_type = item_type;
      pObjIndex->extra_flags = (unsigned long long)extra_flags;
      pObjIndex->wear_flags = wear_flags;
      pObjIndex->item_apply = item_apply;
      pObjIndex->weight = (sh_int)weight;
      pObjIndex->cost = 0;
      pObjIndex->level = (sh_int)obj_level;
      for (i = 0; i < 10; i++)
         pObjIndex->value[i] = val[i];

      snprintf(vnum_str, sizeof(vnum_str), "%d", vnum);
      vv[0] = vnum_str;

      /* Object affects (A lines) */
      aff_res =
          xqp("SELECT location, modifier FROM object_affects WHERE obj_vnum=$1 ORDER BY id", 1, vv);
      if (aff_res)
      {
         int na = PQntuples(aff_res);
         int x;
         for (x = 0; x < na; x++)
         {
            AFFECT_DATA *paf;
            GET_FREE(paf, affect_free);
            paf->type = -1;
            paf->duration = -1;
            paf->location = (sh_int)atoi(PQgetvalue(aff_res, x, 0));
            paf->modifier = (sh_int)atoi(PQgetvalue(aff_res, x, 1));
            paf->bitvector = 0;
            LINK(paf, pObjIndex->first_apply, pObjIndex->last_apply, next, prev);
         }
         PQclear(aff_res);
      }

      /* Extra descs (E sections) */
      ed_res = xqp("SELECT keyword, description FROM object_extra_descs WHERE obj_vnum=$1", 1, vv);
      if (ed_res)
      {
         int ned = PQntuples(ed_res);
         int x;
         for (x = 0; x < ned; x++)
         {
            EXTRA_DESCR_DATA *ed;
            GET_FREE(ed, exdesc_free);
            ed->keyword = str_dup(PQgetvalue(ed_res, x, 0));
            ed->description = str_dup(PQgetvalue(ed_res, x, 1));
            LINK(ed, pObjIndex->first_exdesc, pObjIndex->last_exdesc, next, prev);
         }
         PQclear(ed_res);
      }

      iHash = vnum % MAX_KEY_HASH;
      SING_TOPLINK(pObjIndex, obj_index_hash[iHash], next);

      GET_FREE(bdl, build_free);
      bdl->data = pObjIndex;
      LINK(bdl, pArea->first_area_object, pArea->last_area_object, next, prev);

      top_obj_index++;
   }
   PQclear(objs);
}

static void load_resets_for_area(const char *area_id_str, AREA_DATA *pArea)
{
   const char *v[1];
   PGresult *res;
   int r, nrows;

   v[0] = area_id_str;
   res = xqp("SELECT command, ifflag, arg1, arg2, arg3, notes "
             "FROM resets WHERE area_id=$1 ORDER BY seq",
             1, v);
   if (!res)
      return;

   nrows = PQntuples(res);
   for (r = 0; r < nrows; r++)
   {
      const char *cmd = PQgetvalue(res, r, 0);
      int ifflag = atoi(PQgetvalue(res, r, 1));
      int arg1 = atoi(PQgetvalue(res, r, 2));
      int arg2 = atoi(PQgetvalue(res, r, 3));
      int arg3 = atoi(PQgetvalue(res, r, 4));
      const char *notes = PQgetvalue(res, r, 5);
      RESET_DATA *pReset;

      GET_FREE(pReset, reset_free);
      pReset->command = cmd[0];
      pReset->ifflag = (sh_int)ifflag;
      pReset->arg1 = arg1;
      pReset->arg2 = arg2;
      pReset->arg3 = arg3;
      pReset->notes = (notes && *notes) ? str_dup(notes) : NULL;
      pReset->auto_message = NULL;

      LINK(pReset, pArea->first_reset, pArea->last_reset, next, prev);
   }
   PQclear(res);
}

static void load_shops_for_area(const char *area_id_str, AREA_DATA *pArea)
{
   const char *v[1];
   PGresult *res;
   int r, nrows;

   v[0] = area_id_str;
   res = xqp("SELECT s.keeper_vnum, s.buy_type_0, s.buy_type_1, s.buy_type_2, s.buy_type_3, "
             "s.buy_type_4, s.profit_buy, s.profit_sell, s.open_hour, s.close_hour "
             "FROM shops s JOIN mobiles m ON m.vnum = s.keeper_vnum "
             "WHERE m.area_id=$1 ORDER BY s.keeper_vnum",
             1, v);
   if (!res)
      return;

   nrows = PQntuples(res);
   for (r = 0; r < nrows; r++)
   {
      int keeper_vnum = atoi(PQgetvalue(res, r, 0));
      int i;
      MOB_INDEX_DATA *pMobIndex;
      SHOP_DATA *pShop;
      BUILD_DATA_LIST *bdl;

      pMobIndex = get_mob_index(keeper_vnum);
      if (!pMobIndex)
      {
         fprintf(stderr, "db_load: shop keeper vnum %d not found\n", keeper_vnum);
         continue;
      }

      GET_FREE(pShop, shop_free);
      pShop->keeper = (sh_int)keeper_vnum;
      for (i = 0; i < MAX_TRADE; i++)
         pShop->buy_type[i] = (sh_int)atoi(PQgetvalue(res, r, 1 + i));
      pShop->profit_buy = (sh_int)atoi(PQgetvalue(res, r, 6));
      pShop->profit_sell = (sh_int)atoi(PQgetvalue(res, r, 7));
      pShop->open_hour = (sh_int)atoi(PQgetvalue(res, r, 8));
      pShop->close_hour = (sh_int)atoi(PQgetvalue(res, r, 9));

      pMobIndex->pShop = pShop;
      LINK(pShop, first_shop, last_shop, next, prev);

      GET_FREE(bdl, build_free);
      bdl->data = pShop;
      LINK(bdl, pArea->first_area_shop, pArea->last_area_shop, next, prev);
   }
   PQclear(res);
}

static void load_specials_for_area(const char *area_id_str, AREA_DATA *pArea)
{
   const char *v[1];
   PGresult *res;
   int r, nrows;

   v[0] = area_id_str;
   res = xqp("SELECT ms.mob_vnum, ms.spec_name "
             "FROM mobile_specials ms JOIN mobiles m ON m.vnum = ms.mob_vnum "
             "WHERE m.area_id=$1 ORDER BY ms.mob_vnum",
             1, v);
   if (!res)
      return;

   nrows = PQntuples(res);
   for (r = 0; r < nrows; r++)
   {
      int mob_vnum = atoi(PQgetvalue(res, r, 0));
      const char *spec_name = PQgetvalue(res, r, 1);
      MOB_INDEX_DATA *pMobIndex;
      SPEC_FUN *sfun;

      pMobIndex = get_mob_index(mob_vnum);
      if (!pMobIndex)
      {
         fprintf(stderr, "db_load: special mob vnum %d not found\n", mob_vnum);
         continue;
      }
      sfun = spec_lookup(spec_name);
      if (!sfun)
         fprintf(stderr, "db_load: unknown spec function '%s' for vnum %d\n", spec_name, mob_vnum);
      else
         pMobIndex->spec_fun = sfun;
   }
   PQclear(res);
   (void)pArea; /* unused — kept for API consistency */
}

static void load_objfuns_for_area(const char *area_id_str, AREA_DATA *pArea)
{
   const char *v[1];
   PGresult *res;
   int r, nrows;

   v[0] = area_id_str;
   res = xqp("SELECT of.obj_vnum, of.fun_name "
             "FROM object_functions of JOIN objects o ON o.vnum = of.obj_vnum "
             "WHERE o.area_id=$1 ORDER BY of.obj_vnum",
             1, v);
   if (!res)
      return;

   nrows = PQntuples(res);
   for (r = 0; r < nrows; r++)
   {
      int obj_vnum = atoi(PQgetvalue(res, r, 0));
      const char *fun_name = PQgetvalue(res, r, 1);
      OBJ_INDEX_DATA *pObjIndex;
      OBJ_FUN *ofun;

      pObjIndex = get_obj_index(obj_vnum);
      if (!pObjIndex)
      {
         fprintf(stderr, "db_load: objfun obj vnum %d not found\n", obj_vnum);
         continue;
      }
      ofun = obj_fun_lookup(fun_name);
      if (!ofun)
         fprintf(stderr, "db_load: unknown obj function '%s' for vnum %d\n", fun_name, obj_vnum);
      else
         pObjIndex->obj_fun = ofun;
   }
   PQclear(res);
   (void)pArea;
}

/* -----------------------------------------------------------------------
 * Main area loader — replaces the area.lst / file-parse loop in boot_db()
 * ----------------------------------------------------------------------- */

void db_load_areas_from_db(void)
{
   PGresult *areas;
   int a, nareas;

   areas = xq("SELECT id, name, min_vnum, max_vnum, keyword, level_label, area_number, "
              "owner, level_min, level_max, map_offset, reset_rate, reset_msg, "
              "can_read, can_write, music, flags "
              "FROM areas ORDER BY min_vnum");
   if (!areas)
      return;

   nareas = PQntuples(areas);
   log_f("DB: loading %d areas from database.", nareas);

   for (a = 0; a < nareas; a++)
   {
      int col = 0;
      const char *area_id_str = PQgetvalue(areas, a, col++); /* id (DB bigint) */
      const char *name = PQgetvalue(areas, a, col++);
      int min_vnum = atoi(PQgetvalue(areas, a, col++));
      int max_vnum = atoi(PQgetvalue(areas, a, col++));
      const char *keyword = PQgetvalue(areas, a, col++);
      const char *level_label = PQgetvalue(areas, a, col++);
      int area_number = atoi(PQgetvalue(areas, a, col++));
      const char *owner = PQgetvalue(areas, a, col++);
      int level_min = atoi(PQgetvalue(areas, a, col++));
      int level_max = atoi(PQgetvalue(areas, a, col++));
      int map_offset = atoi(PQgetvalue(areas, a, col++));
      int reset_rate = atoi(PQgetvalue(areas, a, col++));
      const char *reset_msg = PQgetvalue(areas, a, col++);
      const char *can_read = PQgetvalue(areas, a, col++);
      const char *can_write = PQgetvalue(areas, a, col++);
      const char *music = PQgetvalue(areas, a, col++);
      int flags = atoi(PQgetvalue(areas, a, col++));
      AREA_DATA *pArea;

      GET_FREE(pArea, area_free);
      pArea->name = str_dup(name);
      pArea->age = 15;
      pArea->nplayer = 0;
      pArea->offset = map_offset;
      pArea->modified = FALSE;
      pArea->min_vnum = min_vnum;
      pArea->max_vnum = max_vnum;
      pArea->area_num = area_number;
      pArea->filename = str_dup("db");
      pArea->owner = str_dup(owner && *owner ? owner : "");
      pArea->can_read = str_dup(can_read && *can_read ? can_read : "all");
      pArea->can_write = str_dup(can_write && *can_write ? can_write : "all");
      pArea->level_label = str_dup(level_label && *level_label ? level_label : "");
      pArea->keyword = str_dup(keyword && *keyword ? keyword : "none");
      pArea->reset_msg = str_dup(reset_msg && *reset_msg ? reset_msg : "");
      pArea->min_level = (sh_int)level_min;
      pArea->max_level = (sh_int)level_max;
      pArea->reset_rate = (sh_int)(reset_rate > 0 ? reset_rate : 15);
      pArea->gold = 0;
      pArea->flags = flags;
      pArea->music = (music && *music) ? str_dup(music) : NULL;
      pArea->aggro_list = 0;
      pArea->control = NULL;

      area_load = pArea;
      if (area_number >= 0 && area_number < MAX_AREAS)
         area_used[area_number] = pArea;

      LINK(pArea, first_area, last_area, next, prev);
      top_area++;

      /* Load this area's content in dependency order. */
      load_rooms_for_area(area_id_str, pArea);
      load_mobiles_for_area(area_id_str, pArea);
      load_objects_for_area(area_id_str, pArea);
      load_resets_for_area(area_id_str, pArea);
      load_shops_for_area(area_id_str, pArea);
      load_specials_for_area(area_id_str, pArea);
      load_objfuns_for_area(area_id_str, pArea);

      log_f("  DB: area '%s' (vnums %d–%d) loaded.", name, min_vnum, max_vnum);
   }
   PQclear(areas);
}

/* -----------------------------------------------------------------------
 * Data file loaders
 * ----------------------------------------------------------------------- */

void db_load_bans(void)
{
   PGresult *res;
   int r, n;

   log_f("DB: loading bans.");
   res = xq("SELECT ban_type, address, banned_by FROM bans ORDER BY id");
   if (!res)
      return;

   n = PQntuples(res);
   for (r = 0; r < n; r++)
   {
      int ban_type = atoi(PQgetvalue(res, r, 0));
      const char *address = PQgetvalue(res, r, 1);
      const char *banned_by = PQgetvalue(res, r, 2);
      BAN_DATA *pban;

      GET_FREE(pban, ban_free);
      pban->name = str_dup(address);
      pban->banned_by = str_dup(banned_by);
      pban->newbie = (ban_type == 1) ? TRUE : FALSE;
      LINK(pban, first_ban, last_ban, next, prev);
   }
   PQclear(res);
}

void db_load_socials(void)
{
   PGresult *res;
   int r, n;
   extern struct social_type *social_table;
   extern int maxSocial;

   log_f("DB: loading socials.");
   res = xq("SELECT name, char_no_arg, others_no_arg, char_found, "
            "others_found, vict_found, char_auto, others_auto "
            "FROM socials ORDER BY name");
   if (!res)
      return;

   n = PQntuples(res);
   social_table = malloc(sizeof(struct social_type) * (n + 1));
   if (!social_table)
   {
      fprintf(stderr, "db_load: social_table malloc failed\n");
      PQclear(res);
      return;
   }

   for (r = 0; r < n; r++)
   {
      social_table[r].name = str_dup(PQgetvalue(res, r, 0));
      social_table[r].char_no_arg = str_dup(PQgetvalue(res, r, 1));
      social_table[r].others_no_arg = str_dup(PQgetvalue(res, r, 2));
      social_table[r].char_found = str_dup(PQgetvalue(res, r, 3));
      social_table[r].others_found = str_dup(PQgetvalue(res, r, 4));
      social_table[r].vict_found = str_dup(PQgetvalue(res, r, 5));
      social_table[r].char_auto = str_dup(PQgetvalue(res, r, 6));
      social_table[r].others_auto = str_dup(PQgetvalue(res, r, 7));
   }
   /* Sentinel entry with empty name (required by social lookup). */
   social_table[n].name = str_dup("");
   maxSocial = n;
   PQclear(res);
}

void db_load_clans(void)
{
   PGresult *res;
   int r, n;

   log_f("DB: loading clan diplomacy.");
   res = xq("SELECT id, gold, war_matrix FROM clans ORDER BY id");
   if (!res)
      return;

   n = PQntuples(res);
   for (r = 0; r < n; r++)
   {
      int clan_id = atoi(PQgetvalue(res, r, 0));
      int gold = atoi(PQgetvalue(res, r, 1));
      const char *arr = PQgetvalue(res, r, 2);
      int j;

      if (clan_id < 1 || clan_id >= MAX_CLAN)
         continue;

      politics_data.treasury[clan_id] = gold;

      /* Parse PostgreSQL array literal "{v0,v1,...}" */
      if (arr && *arr == '{')
      {
         char buf[512];
         char *tok;
         strncpy(buf, arr + 1, sizeof(buf) - 1);
         buf[sizeof(buf) - 1] = '\0';
         tok = strtok(buf, ",}");
         for (j = 0; j < MAX_CLAN && tok; j++, tok = strtok(NULL, ",}"))
            politics_data.diplomacy[clan_id][j] = atoi(tok);
      }
   }
   PQclear(res);
}

void db_load_rulers(void)
{
   PGresult *res;
   int r, n;

   log_f("DB: loading rulers.");
   res = xq("SELECT name FROM rulers ORDER BY id");
   if (!res)
      return;

   n = PQntuples(res);
   for (r = 0; r < n; r++)
   {
      const char *name = PQgetvalue(res, r, 0);
      RULER_DATA *ruler;
      RULER_LIST *ruler_member;

      GET_FREE(ruler, ruler_data_free);
      ruler->name = str_dup(name);
      ruler->affiliation_index = 0;
      ruler->affiliation_name = str_dup("No Affiliation");
      ruler->flags = 0;
      ruler->ruler_rank = 0;
      ruler->keywords = str_dup("");
      ruler->first_control = NULL;
      ruler->last_control = NULL;

      GET_FREE(ruler_member, ruler_list_free);
      ruler_member->this_one = ruler;
      LINK(ruler_member, first_ruler_list, last_ruler_list, next, prev);
   }
   PQclear(res);
}

void db_load_brands(void)
{
   PGresult *res;
   int r, n;

   log_f("DB: loading brands.");
   res = xq("SELECT branded_by, item_name, brand_date, description FROM brands ORDER BY id");
   if (!res)
      return;

   n = PQntuples(res);
   for (r = 0; r < n; r++)
   {
      const char *branded_by = PQgetvalue(res, r, 0);
      const char *item_name = PQgetvalue(res, r, 1);
      const char *brand_date = PQgetvalue(res, r, 2);
      const char *description = PQgetvalue(res, r, 3);
      BRAND_DATA *brand;
      DL_LIST *brand_member;

      GET_FREE(brand, brand_data_free);
      brand->branded = str_dup(item_name);
      brand->branded_by = str_dup(branded_by);
      brand->dt_stamp = str_dup(brand_date);
      brand->message = str_dup(description);
      brand->priority = str_dup("");

      GET_FREE(brand_member, dl_list_free);
      brand_member->this_one = brand;
      LINK(brand_member, first_brand, last_brand, next, prev);
   }
   PQclear(res);
}

void db_load_sysdata(void)
{
   /* bln_val_0 → sysdata.w_lock
    * bln_val_1 → sysdata.shownumbers
    * playtesters and staff[] are not stored in the DB schema; they retain
    * their default values (empty/NULL). */
   PGresult *res;
   extern bool wizlock;

   log_f("DB: loading sysdata.");
   res = xq("SELECT bln_val_0, bln_val_1 FROM sysdata WHERE id=1");
   if (!res)
      return;

   if (PQntuples(res) == 0)
   {
      PQclear(res);
      return;
   }

   sysdata.w_lock = (strcmp(PQgetvalue(res, 0, 0), "t") == 0) ? 1 : 0;
   sysdata.shownumbers = (strcmp(PQgetvalue(res, 0, 1), "t") == 0) ? TRUE : FALSE;
   if (sysdata.w_lock == 1)
      wizlock = TRUE;

   PQclear(res);
}

void db_load_boards(void)
{
   PGresult *bres;
   int r, nb;

   log_f("DB: loading boards.");
   bres = xq("SELECT id, vnum, expiry_days, min_read_lev, min_write_lev, clan "
             "FROM boards ORDER BY vnum");
   if (!bres)
      return;

   nb = PQntuples(bres);
   for (r = 0; r < nb; r++)
   {
      int board_db_id = atoi(PQgetvalue(bres, r, 0));
      int vnum = atoi(PQgetvalue(bres, r, 1));
      int expiry_days = atoi(PQgetvalue(bres, r, 2));
      int min_read_lev = atoi(PQgetvalue(bres, r, 3));
      int min_write_lev = atoi(PQgetvalue(bres, r, 4));
      int clan = atoi(PQgetvalue(bres, r, 5));
      BOARD_DATA *board;
      PGresult *mres;
      int m, nm;
      char id_str[32];

      /* Skip if already loaded (lazy load may have pre-populated). */
      {
         BOARD_DATA *b;
         int found = 0;
         for (b = first_board; b; b = b->next)
            if (b->vnum == vnum)
            {
               found = 1;
               break;
            }
         if (found)
            continue;
      }

      GET_FREE(board, board_free);
      board->vnum = vnum;
      board->expiry_time = expiry_days;
      board->min_read_lev = min_read_lev;
      board->min_write_lev = min_write_lev;
      board->clan = clan;
      board->first_message = NULL;
      board->last_message = NULL;
      LINK(board, first_board, last_board, next, prev);

      /* Load messages for this board. */
      snprintf(id_str, sizeof(id_str), "%d", board_db_id);
      {
         const char *v[1];
         v[0] = id_str;
         mres = xqp("SELECT posted_at, author, title, body "
                    "FROM board_messages WHERE board_id=$1 ORDER BY seq",
                    1, v);
      }
      if (!mres)
         continue;

      nm = PQntuples(mres);
      for (m = 0; m < nm; m++)
      {
         MESSAGE_DATA *msg;
         GET_FREE(msg, message_free);
         msg->datetime = (time_t)atol(PQgetvalue(mres, m, 0));
         msg->author = str_dup(PQgetvalue(mres, m, 1));
         msg->title = str_dup(PQgetvalue(mres, m, 2));
         msg->message = str_dup(PQgetvalue(mres, m, 3));
         msg->board = board;
         LINK(msg, board->first_message, board->last_message, next, prev);
      }
      PQclear(mres);
   }
   PQclear(bres);
}

void db_load_room_marks(void)
{
   PGresult *res;
   int r, n;
   extern bool booting_up;

   log_f("DB: loading room marks.");
   /* The schema stores room_vnum and mark_text only; author/duration/type
    * default to empty/0 for marks loaded from the database. */
   res = xq("SELECT room_vnum, mark_text FROM room_marks ORDER BY id");
   if (!res)
      return;

   n = PQntuples(res);
   booting_up = TRUE;
   for (r = 0; r < n; r++)
   {
      int room_vnum = atoi(PQgetvalue(res, r, 0));
      const char *mark_text = PQgetvalue(res, r, 1);
      MARK_DATA *mark;

      GET_FREE(mark, mark_free);
      mark->room_vnum = room_vnum;
      mark->message = str_dup(mark_text);
      mark->author = str_dup("");
      mark->duration = 0;
      mark->type = 0;
      mark_to_room(room_vnum, mark);
   }
   booting_up = FALSE;
   PQclear(res);
}

/* Load a single corpse row and its children (recursive via parent_id). */
static void load_one_corpse(int db_id, OBJ_DATA *parent_obj, int iNest)
{
   PGresult *res;
   int r, n;
   char id_str[32];
   const char *v[1];
   static OBJ_DATA obj_zero;

   snprintf(id_str, sizeof(id_str), "%d", db_id);
   v[0] = id_str;
   res = xqp("SELECT id, where_vnum, nest, name, short_descr, description, "
             "vnum, extra_flags, wear_flags, wear_loc, class_flags, item_type, "
             "weight, level, timer, cost, "
             "value_0, value_1, value_2, value_3, value_4, value_5, "
             "value_6, value_7, value_8, value_9 "
             "FROM corpses WHERE id=$1",
             1, v);
   if (!res)
      return;

   n = PQntuples(res);
   for (r = 0; r < n; r++)
   {
      int child_db_id = atoi(PQgetvalue(res, r, 0));
      int where_vnum = atoi(PQgetvalue(res, r, 1));
      /* nest col unused — we track depth via iNest parameter */
      const char *name = PQgetvalue(res, r, 3);
      const char *short_descr = PQgetvalue(res, r, 4);
      const char *description = PQgetvalue(res, r, 5);
      int obj_vnum = atoi(PQgetvalue(res, r, 6));
      long long extra_flags = atoll(PQgetvalue(res, r, 7));
      int wear_flags = atoi(PQgetvalue(res, r, 8));
      int wear_loc = atoi(PQgetvalue(res, r, 9));
      int class_flags = atoi(PQgetvalue(res, r, 10));
      int item_type = atoi(PQgetvalue(res, r, 11));
      int weight = atoi(PQgetvalue(res, r, 12));
      int level = atoi(PQgetvalue(res, r, 13));
      int timer = atoi(PQgetvalue(res, r, 14));
      int cost = atoi(PQgetvalue(res, r, 15));
      OBJ_DATA *obj;
      OBJ_INDEX_DATA *pIdx;

      pIdx = get_obj_index(obj_vnum);
      if (!pIdx)
      {
         pIdx = get_obj_index(1006); /* TEMP_VNUM fallback */
         if (!pIdx)
            continue;
      }

      GET_FREE(obj, obj_free);
      *obj = obj_zero;
      obj->pIndexData = pIdx;
      obj->name = str_dup(name);
      obj->short_descr = str_dup(short_descr);
      obj->description = str_dup(description);
      obj->extra_flags = (unsigned long long)extra_flags;
      obj->wear_flags = wear_flags;
      obj->wear_loc = wear_loc;
      obj->item_apply = class_flags;
      obj->item_type = item_type;
      obj->weight = weight;
      obj->level = level;
      obj->timer = timer;
      obj->cost = cost;
      obj->value[0] = atoi(PQgetvalue(res, r, 16));
      obj->value[1] = atoi(PQgetvalue(res, r, 17));
      obj->value[2] = atoi(PQgetvalue(res, r, 18));
      obj->value[3] = atoi(PQgetvalue(res, r, 19));
      obj->value[4] = atoi(PQgetvalue(res, r, 20));
      obj->value[5] = atoi(PQgetvalue(res, r, 21));
      obj->value[6] = atoi(PQgetvalue(res, r, 22));
      obj->value[7] = atoi(PQgetvalue(res, r, 23));
      obj->value[8] = atoi(PQgetvalue(res, r, 24));
      obj->value[9] = atoi(PQgetvalue(res, r, 25));

      pIdx->count++;
      LINK(obj, first_obj, last_obj, next, prev);

      if (iNest == 0 || parent_obj == NULL)
      {
         ROOM_INDEX_DATA *room = get_room_index(where_vnum);
         if (!room)
            room = get_room_index(65323); /* ROOM_VNUM_MORGUE */
         if (room)
            obj_to_room(obj, room);
      }
      else
      {
         obj_to_obj(obj, parent_obj);
      }

      /* Recursively load children. */
      {
         PGresult *cres;
         int c, nc;
         char parent_str[32];
         const char *cv[1];
         snprintf(parent_str, sizeof(parent_str), "%d", child_db_id);
         cv[0] = parent_str;
         cres = xqp("SELECT id FROM corpses WHERE parent_id=$1 ORDER BY id", 1, cv);
         if (cres)
         {
            nc = PQntuples(cres);
            for (c = 0; c < nc; c++)
               load_one_corpse(atoi(PQgetvalue(cres, c, 0)), obj, iNest + 1);
            PQclear(cres);
         }
      }
   }
   PQclear(res);
}

void db_load_corpses(void)
{
   PGresult *res;
   int r, n;

   log_f("DB: loading corpses.");
   /* Load top-level corpses only; children are loaded recursively. */
   res = xq("SELECT id FROM corpses WHERE parent_id IS NULL ORDER BY id");
   if (!res)
      return;

   n = PQntuples(res);
   for (r = 0; r < n; r++)
      load_one_corpse(atoi(PQgetvalue(res, r, 0)), NULL, 0);

   PQclear(res);
}

/* -----------------------------------------------------------------------
 * Keep chest loader
 *
 * Loads keep chest items from keep_chests / keep_chest_items.
 * For any chest vnum present in data/chest/ but absent from the DB,
 * falls back to the flat-file load_chest() and logs a warning.
 * ----------------------------------------------------------------------- */
void db_load_chests(void)
{
   PGresult *res;
   int r, n;

   /* --- Phase 1: load chests from the database -------------------------
    * Collect all known DB vnums into a bitfield so we can detect flat-file
    * stragglers below.  We use a simple sorted vnum list instead of a hash.
    */
   res = xq("SELECT kc.vnum, kci.id, kci.parent_id, kci.nest, kci.sort_order,"
            " kci.name, kci.short_descr, kci.description, kci.vnum AS obj_vnum,"
            " kci.extra_flags, kci.wear_flags, kci.wear_loc, kci.class_flags,"
            " kci.item_type, kci.weight, kci.level, kci.timer, kci.cost,"
            " kci.value_0, kci.value_1, kci.value_2, kci.value_3,"
            " kci.value_4, kci.value_5, kci.value_6, kci.value_7,"
            " kci.value_8, kci.value_9, kci.objfun, kc.id AS chest_db_id"
            " FROM keep_chests kc"
            " LEFT JOIN keep_chest_items kci ON kci.chest_id = kc.id"
            " ORDER BY kc.vnum, kci.sort_order");
   if (!res)
      return;

   n = PQntuples(res);

   /* Track which DB vnums we have seen */
   int *db_vnums = NULL;
   int db_vnum_count = 0;

   /* Collect distinct chest vnums first */
   for (r = 0; r < n; r++)
   {
      int vnum = atoi(PQgetvalue(res, r, 0));
      /* Only add once per chest */
      int found = 0;
      int k;
      for (k = 0; k < db_vnum_count; k++)
         if (db_vnums[k] == vnum)
         {
            found = 1;
            break;
         }
      if (!found)
      {
         db_vnums = realloc(db_vnums, (size_t)(db_vnum_count + 1) * sizeof(int));
         if (!db_vnums)
            break;
         db_vnums[db_vnum_count++] = vnum;
      }
   }

   /* Per-chest item loading using rgObjNest nesting array.
    * Items are stored with sort_order; the parent item with the matching
    * sort_order must be found in the nest array.  We track a local mapping
    * sort_order → OBJ_DATA* for items loaded so far in this chest.        */
   int cur_chest_vnum = -1;
   bool cur_chest_valid = FALSE; /* TRUE only when cur chest has a world object */
   /* max items per chest is bounded by game config; 256 is safe headroom */
#define CHEST_MAX_NEST_TRACK 512
   int nest_sort[CHEST_MAX_NEST_TRACK];
   OBJ_DATA *nest_obj[CHEST_MAX_NEST_TRACK];
   int nest_count = 0;

   for (r = 0; r < n; r++)
   {
      int chest_vnum = atoi(PQgetvalue(res, r, 0));
      int is_null_item = PQgetisnull(res, r, 1); /* kci.id is NULL when no items */

      if (chest_vnum != cur_chest_vnum)
      {
         /* Switch to a new chest */
         cur_chest_vnum = chest_vnum;
         cur_chest_valid = FALSE;
         nest_count = 0;

         /* Locate the already-created chest object in the world */
         OBJ_DATA *chest_obj = NULL;
         OBJ_DATA *o;
         for (o = first_obj; o != NULL; o = o->next)
            if (o->pIndexData != NULL && o->pIndexData->vnum == chest_vnum &&
                o->item_type == ITEM_CONTAINER && IS_SET(o->value[1], CONT_KEEP_CHEST))
            {
               chest_obj = o;
               break;
            }
         if (!chest_obj)
         {
            log_f("db_load_chests: no world object for chest vnum %d, skipping.", chest_vnum);
            continue;
         }
         /* Seed nest tracking: sort_order -1 means "child of chest root" */
         nest_sort[0] = -1;
         nest_obj[0] = chest_obj;
         nest_count = 1;
         cur_chest_valid = TRUE;
      }

      /* Skip all rows for chests that had no matching world object */
      if (!cur_chest_valid)
         continue;

      if (is_null_item)
         continue; /* chest exists in DB but has no items */

      /* Build the item OBJ_DATA */
      int parent_id_field = PQgetisnull(res, r, 2) ? -1 : atoi(PQgetvalue(res, r, 2));
      int sort_order = PQgetisnull(res, r, 4) ? 0 : atoi(PQgetvalue(res, r, 4));
      const char *name = PQgetvalue(res, r, 5);
      const char *short_descr = PQgetvalue(res, r, 6);
      const char *description = PQgetvalue(res, r, 7);
      int obj_vnum = atoi(PQgetvalue(res, r, 8));
      (void)parent_id_field;

      OBJ_INDEX_DATA *pIdx = get_obj_index(obj_vnum);
      /* If prototype not found, create a generic placeholder */
      if (!pIdx)
      {
         log_f("db_load_chests: obj vnum %d not found, skipping item.", obj_vnum);
         continue;
      }

      OBJ_DATA *obj;
      static OBJ_DATA obj_zero;
      GET_FREE(obj, obj_free);
      *obj = obj_zero;
      obj->pIndexData = pIdx;
      obj->name = str_dup(name[0] ? name : pIdx->name);
      obj->short_descr = str_dup(short_descr[0] ? short_descr : pIdx->short_descr);
      obj->description = str_dup(description[0] ? description : pIdx->description);
      obj->extra_flags = (long long)atoll(PQgetvalue(res, r, 9));
      obj->wear_flags = atoi(PQgetvalue(res, r, 10));
      obj->wear_loc = atoi(PQgetvalue(res, r, 11));
      obj->item_apply = atoi(PQgetvalue(res, r, 12));
      obj->item_type = atoi(PQgetvalue(res, r, 13));
      obj->weight = atoi(PQgetvalue(res, r, 14));
      obj->level = atoi(PQgetvalue(res, r, 15));
      obj->timer = atoi(PQgetvalue(res, r, 16));
      obj->cost = atoi(PQgetvalue(res, r, 17));
      obj->value[0] = atoi(PQgetvalue(res, r, 18));
      obj->value[1] = atoi(PQgetvalue(res, r, 19));
      obj->value[2] = atoi(PQgetvalue(res, r, 20));
      obj->value[3] = atoi(PQgetvalue(res, r, 21));
      obj->value[4] = atoi(PQgetvalue(res, r, 22));
      obj->value[5] = atoi(PQgetvalue(res, r, 23));
      obj->value[6] = atoi(PQgetvalue(res, r, 24));
      obj->value[7] = atoi(PQgetvalue(res, r, 25));
      obj->value[8] = atoi(PQgetvalue(res, r, 26));
      obj->value[9] = atoi(PQgetvalue(res, r, 27));

      if (!PQgetisnull(res, r, 28))
      {
         const char *objfun_name = PQgetvalue(res, r, 28);
         if (objfun_name[0])
            obj->obj_fun = obj_fun_lookup(objfun_name);
      }

      pIdx->count++;
      LINK(obj, first_obj, last_obj, next, prev);

      /* Find parent container using sort_order lookup */
      OBJ_DATA *parent_obj = nest_obj[0]; /* default: chest root */
      {
         int parent_db_id = PQgetisnull(res, r, 2) ? -1 : atoi(PQgetvalue(res, r, 2));
         if (parent_db_id != -1)
         {
            /* Find which previously-loaded item has that DB id */
            int ki;
            for (ki = 0; ki < n; ki++)
            {
               if (PQgetisnull(res, ki, 1))
                  continue;
               if (atoi(PQgetvalue(res, ki, 1)) == parent_db_id)
               {
                  /* Find the nest_obj entry for that sort_order */
                  int ps = PQgetisnull(res, ki, 4) ? 0 : atoi(PQgetvalue(res, ki, 4));
                  int nk;
                  for (nk = 0; nk < nest_count; nk++)
                     if (nest_sort[nk] == ps)
                     {
                        parent_obj = nest_obj[nk];
                        break;
                     }
                  break;
               }
            }
         }
      }

      obj_to_obj(obj, parent_obj);

      /* Track this item for child lookups */
      if (nest_count < CHEST_MAX_NEST_TRACK)
      {
         nest_sort[nest_count] = sort_order;
         nest_obj[nest_count] = obj;
         nest_count++;
      }
   }
#undef CHEST_MAX_NEST_TRACK

   PQclear(res);

   /* --- Phase 2: flat-file fallback for unmigrated chests ---------------
    * Scan data/chest/ for files whose vnum is not in the DB vnum list.
    */
   {
      DIR *dir;
      struct dirent *entry;
      char chest_dir[MAX_STRING_LENGTH];
      snprintf(chest_dir, sizeof(chest_dir), "%s", CHEST_DIR);

      dir = opendir(chest_dir);
      if (dir)
      {
         while ((entry = readdir(dir)) != NULL)
         {
            int file_vnum;
            int k, in_db;

            if (entry->d_name[0] == '.')
               continue;
            file_vnum = atoi(entry->d_name);
            if (file_vnum <= 0)
               continue;

            in_db = 0;
            for (k = 0; k < db_vnum_count; k++)
               if (db_vnums[k] == file_vnum)
               {
                  in_db = 1;
                  break;
               }

            if (!in_db)
            {
               log_f("DB: chest vnum %d not in database, loading from flat file (migrate it!).",
                     file_vnum);
               load_chest(file_vnum);
            }
         }
         closedir(dir);
      }
   }

   free(db_vnums);
}

/* -----------------------------------------------------------------------
 * Quest templates
 * ----------------------------------------------------------------------- */

/* Parse a PostgreSQL integer array literal like "{1234,5678,90}" into
 * an int array.  Returns the number of elements parsed. */
static int parse_pg_int_array(const char *pg, int *out, int max)
{
   const char *p;
   int count = 0;

   if (!pg || *pg != '{')
      return 0;

   p = pg + 1; /* skip '{' */
   while (*p && *p != '}' && count < max)
   {
      out[count++] = atoi(p);
      while (*p && *p != ',' && *p != '}')
         p++;
      if (*p == ',')
         p++;
   }
   return count;
}

void db_load_quest_templates(void)
{
   PGresult *res;
   int nrows, i;

   res = xq("SELECT id, title, prerequisite_template_id, type, num_targets, "
            "       target_vnums, kill_needed, min_level, max_level, offerer_vnum, "
            "       reward_gold, reward_qp, reward_exp, "
            "       accept_message, completion_message, "
            "       reward_obj_short, reward_obj_name, reward_obj_long, "
            "       reward_obj_wear_flags, reward_obj_extra_flags, "
            "       reward_obj_weight, reward_obj_item_apply "
            "FROM quest_templates ORDER BY id");
   if (!res)
      return;

   nrows = PQntuples(res);
   if (nrows == 0)
   {
      log_f("DB: quest_templates table is empty.");
      PQclear(res);
      return;
   }

   /* Free any previously loaded templates */
   if (quest_template_table != NULL)
   {
      for (i = 0; i < quest_template_count; i++)
      {
         free_string(quest_template_table[i].title);
         free_string(quest_template_table[i].reward_obj_short);
         free_string(quest_template_table[i].reward_obj_name);
         free_string(quest_template_table[i].reward_obj_long);
         free_string(quest_template_table[i].accept_message);
         free_string(quest_template_table[i].completion_message);
      }
      free(quest_template_table);
      quest_template_table = NULL;
      quest_template_count = 0;
   }

   quest_template_table = calloc(nrows, sizeof(QUEST_TEMPLATE));
   if (!quest_template_table)
   {
      log_f("DB: out of memory allocating %d quest templates.", nrows);
      PQclear(res);
      return;
   }

   for (i = 0; i < nrows; i++)
   {
      QUEST_TEMPLATE *tpl = &quest_template_table[i];

      tpl->id = atoi(PQgetvalue(res, i, 0));
      tpl->title = str_dup(PQgetvalue(res, i, 1));
      tpl->prerequisite_template_id = PQgetisnull(res, i, 2) ? -1 : atoi(PQgetvalue(res, i, 2));
      tpl->type = atoi(PQgetvalue(res, i, 3));
      tpl->num_targets = atoi(PQgetvalue(res, i, 4));
      parse_pg_int_array(PQgetvalue(res, i, 5), tpl->target_vnum, QUEST_MAX_TARGETS);
      tpl->kill_needed = atoi(PQgetvalue(res, i, 6));
      tpl->min_level = atoi(PQgetvalue(res, i, 7));
      tpl->max_level = atoi(PQgetvalue(res, i, 8));
      tpl->offerer_vnum = PQgetisnull(res, i, 9) ? 0 : atoi(PQgetvalue(res, i, 9));
      tpl->reward_gold = atoi(PQgetvalue(res, i, 10));
      tpl->reward_qp = atoi(PQgetvalue(res, i, 11));
      tpl->reward_exp = atoi(PQgetvalue(res, i, 12));
      tpl->accept_message = str_dup(PQgetvalue(res, i, 13));
      tpl->completion_message = str_dup(PQgetvalue(res, i, 14));
      tpl->reward_obj_short = str_dup(PQgetvalue(res, i, 15));
      tpl->reward_obj_name = str_dup(PQgetvalue(res, i, 16));
      tpl->reward_obj_long = str_dup(PQgetvalue(res, i, 17));
      tpl->reward_obj_wear_flags = atoi(PQgetvalue(res, i, 18));
      tpl->reward_obj_extra_flags = atoi(PQgetvalue(res, i, 19));
      tpl->reward_obj_weight = atoi(PQgetvalue(res, i, 20));
      tpl->reward_obj_item_apply = atoi(PQgetvalue(res, i, 21));
   }

   quest_template_count = nrows;
   log_f("DB: loaded %d quest template%s.", nrows, nrows == 1 ? "" : "s");
   PQclear(res);
}
