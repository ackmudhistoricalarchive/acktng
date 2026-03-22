/* db_load.c — ACK!TNG PostgreSQL boot-time loader.
 *
 * Implements the db_load_* family that replaces the flat-file loaders called
 * from boot_db() when compiled with -DHAVE_LIBPQ -DUSE_DB_LOAD.
 *
 * All functions use the synchronous boot connection returned by db_conn_get().
 * They must be called after db_conn_open() and before db_conn_close() / the
 * async worker is started.
 *
 * Per-area content is loaded inside db_load_areas_from_db() via static
 * helpers so that area_load always points to the current area — matching the
 * invariant expected by fix_exits(), check_resets(), and the OLC editor.
 */

#ifdef HAVE_LIBPQ

#include "globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

#include "db_conn.h"
#include "db_load.h"

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
 * Help / shelp / lore loaders
 * ----------------------------------------------------------------------- */

/* Shared implementation: reads from table_name into the given linked list. */
static void load_helpdir_from_db(const char *table, HELP_DATA **first, HELP_DATA **last)
{
   char sql[128];
   PGresult *res;
   int i, n;

   snprintf(sql, sizeof(sql), "SELECT level, keywords, body FROM %s ORDER BY filename", table);
   res = xq(sql);
   if (!res)
      return;

   n = PQntuples(res);
   for (i = 0; i < n; i++)
   {
      int lev = atoi(PQgetvalue(res, i, 0));
      const char *keywords = PQgetvalue(res, i, 1);
      const char *body = PQgetvalue(res, i, 2);
      HELP_DATA *pHelp;

      GET_FREE(pHelp, help_free);
      pHelp->level = (sh_int)lev;
      pHelp->flags = 0;
      pHelp->keyword = str_dup(keywords && *keywords ? keywords : "");
      pHelp->text = str_dup(body && *body ? body : "");
      LINK(pHelp, *first, *last, next, prev);
   }
   PQclear(res);
}

void db_load_helps_from_db(void)
{
   log_f("DB: loading help entries.");
   load_helpdir_from_db("help_entries", &first_help, &last_help);
}

void db_load_shelps_from_db(void)
{
   log_f("DB: loading shelp entries.");
   load_helpdir_from_db("shelp_entries", &first_shelp, &last_shelp);
}

void db_load_lore_from_db(void)
{
   PGresult *topics;
   int t, ntopics;

   log_f("DB: loading lore.");

   topics = xq("SELECT id, keywords FROM lore_topics ORDER BY id");
   if (!topics)
      return;

   ntopics = PQntuples(topics);
   for (t = 0; t < ntopics; t++)
   {
      const char *topic_id = PQgetvalue(topics, t, 0);
      const char *keywords = PQgetvalue(topics, t, 1);
      const char *v[1];
      PGresult *entries;
      int e, nentries;

      v[0] = topic_id;
      entries = xqp("SELECT flags, body FROM lore_entries WHERE topic_id=$1 ORDER BY seq", 1, v);
      if (!entries)
         continue;

      nentries = PQntuples(entries);
      for (e = 0; e < nentries; e++)
      {
         long flags = atol(PQgetvalue(entries, e, 0));
         const char *body = PQgetvalue(entries, e, 1);
         HELP_DATA *pHelp;

         GET_FREE(pHelp, help_free);
         pHelp->level = 0;
         pHelp->flags = flags;
         pHelp->keyword = str_dup(keywords && *keywords ? keywords : "");
         pHelp->text = str_dup(body && *body ? body : "");
         LINK(pHelp, first_lore, last_lore, next, prev);
      }
      PQclear(entries);
   }
   PQclear(topics);
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

#endif /* HAVE_LIBPQ */
