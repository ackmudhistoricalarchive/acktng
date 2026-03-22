/* db_to_files.c — ACK!TNG PostgreSQL → flat-file export tool.
 *
 * Standalone binary (not linked with the server).  Reads all six content
 * stores from PostgreSQL and regenerates the original flat files under
 * export/ subdirectories, suitable for diff against originals or rollback.
 *
 * Usage:
 *   ./tools/db_to_files [connstr] [--areas-only|--help-only|--lore-only|
 *                                   --data-only|--boards-only]
 *
 * connstr defaults to the contents of data/db.conf.  Option flags limit
 * which content stores are exported.
 *
 * Output directories (relative to CWD, which must be area/ at runtime):
 *   ../area/export/      — .are files (one per area, same format as originals)
 *   ../help/export/      — help entry files
 *   ../shelp/export/     — shelp entry files
 *   ../lore/export/      — lore entry files
 *   ../data/export/      — bans.lst, socials.txt, rulers.lst, brands.lst,
 *                          clandata.dat
 *   ../area/boards/export/ — board.<vnum> files
 *
 * Build: make tools/db_to_files  (from src/)
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <libpq-fe.h>

/* -----------------------------------------------------------------------
 * Connection state
 * ----------------------------------------------------------------------- */

static PGconn *conn;
static int errors;

/* -----------------------------------------------------------------------
 * SQL helpers
 * ----------------------------------------------------------------------- */

/* Execute a query; returns result (caller must PQclear) or NULL on error. */
static PGresult *xquery(const char *sql)
{
   PGresult *res = PQexec(conn, sql);
   if (PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      fprintf(stderr, "SQL error [%s]: %s\n", sql, PQresultErrorMessage(res));
      PQclear(res);
      errors++;
      return NULL;
   }
   return res;
}

static PGresult *xquery_params(const char *sql, int n, const char *const *vals)
{
   PGresult *res = PQexecParams(conn, sql, n, NULL, vals, NULL, NULL, 0);
   if (PQresultStatus(res) != PGRES_TUPLES_OK)
   {
      fprintf(stderr, "SQL error [params]: %s\n", PQresultErrorMessage(res));
      PQclear(res);
      errors++;
      return NULL;
   }
   return res;
}

/* -----------------------------------------------------------------------
 * File write utilities
 *
 * These mirror the format that the server's fread_* parsers expect.
 * ----------------------------------------------------------------------- */

/* Write a tilde-terminated string followed by a newline.
 * An empty or NULL string becomes just "~\n". */
static void fw_tilde(FILE *fp, const char *s)
{
   if (s && *s)
      fprintf(fp, "%s~\n", s);
   else
      fprintf(fp, "~\n");
}

/* Write a tilde-terminated string on its own line, preceded by a newline.
 * Used for long_descr / description fields that conventionally start
 * on their own line in the file. */
static void fw_tilde_block(FILE *fp, const char *s)
{
   if (s && *s)
      fprintf(fp, "%s~\n", s);
   else
      fprintf(fp, "~\n");
}

/* -----------------------------------------------------------------------
 * Directory helpers
 * ----------------------------------------------------------------------- */

/* Create a directory (and its parent) if it does not already exist.
 * Only one level of parent creation is attempted. */
static int ensure_dir(const char *path)
{
   struct stat st;
   if (stat(path, &st) == 0)
      return S_ISDIR(st.st_mode) ? 1 : 0;
   if (mkdir(path, 0755) == 0)
      return 1;
   /* Try creating the parent first. */
   {
      char parent[512];
      char *slash;
      strncpy(parent, path, sizeof(parent) - 1);
      parent[sizeof(parent) - 1] = '\0';
      slash = strrchr(parent, '/');
      if (slash && slash != parent)
      {
         *slash = '\0';
         if (mkdir(parent, 0755) != 0 && errno != EEXIST)
         {
            fprintf(stderr, "WARN: cannot create %s: %s\n", parent, strerror(errno));
            return 0;
         }
      }
   }
   if (mkdir(path, 0755) != 0 && errno != EEXIST)
   {
      fprintf(stderr, "WARN: cannot create %s: %s\n", path, strerror(errno));
      return 0;
   }
   return 1;
}

/* -----------------------------------------------------------------------
 * Connection string helper
 * ----------------------------------------------------------------------- */

static char *read_db_conf(const char *base_dir)
{
   char path[512];
   FILE *fp;
   char buf[1024];
   size_t n;

   snprintf(path, sizeof(path), "%s/../data/db.conf", base_dir);
   fp = fopen(path, "r");
   if (!fp)
      return NULL;
   n = fread(buf, 1, sizeof(buf) - 1, fp);
   fclose(fp);
   if (!n)
      return NULL;
   buf[n] = '\0';
   while (n > 0 &&
          (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ' || buf[n - 1] == '\t'))
      buf[--n] = '\0';
   return strdup(buf);
}

/* -----------------------------------------------------------------------
 * Help / shelp exporter
 *
 * Regenerates one file per row: level/keywords header then "---" then body.
 * ----------------------------------------------------------------------- */

static int export_helpdir(const char *out_dir, const char *table)
{
   PGresult *res;
   int i, nrows, count = 0;
   char sql[128];

   if (!ensure_dir(out_dir))
      return 0;

   snprintf(sql, sizeof(sql), "SELECT filename, level, keywords, body FROM %s ORDER BY filename",
            table);
   res = xquery(sql);
   if (!res)
      return 0;

   nrows = PQntuples(res);
   for (i = 0; i < nrows; i++)
   {
      const char *filename = PQgetvalue(res, i, 0);
      const char *level = PQgetvalue(res, i, 1);
      const char *keywords = PQgetvalue(res, i, 2);
      const char *body = PQgetvalue(res, i, 3);
      char path[512];
      FILE *fp;

      snprintf(path, sizeof(path), "%s/%s", out_dir, filename);
      fp = fopen(path, "w");
      if (!fp)
      {
         fprintf(stderr, "WARN: cannot write %s: %s\n", path, strerror(errno));
         errors++;
         continue;
      }
      fprintf(fp, "level %s\n", level);
      fprintf(fp, "keywords %s\n", keywords);
      fprintf(fp, "---\n");
      if (body && *body)
         fprintf(fp, "%s", body);
      fclose(fp);
      count++;
   }

   PQclear(res);
   return count;
}

/* -----------------------------------------------------------------------
 * Lore exporter
 *
 * Regenerates one file per topic: keywords header then "---" then body,
 * with additional "flags <N>\n---\n<body>" blocks for entries seq > 1.
 * ----------------------------------------------------------------------- */

static int export_lore(const char *out_dir)
{
   PGresult *topics;
   int t, ntopics, count = 0;

   if (!ensure_dir(out_dir))
      return 0;

   topics = xquery("SELECT id, filename, keywords FROM lore_topics ORDER BY filename");
   if (!topics)
      return 0;

   ntopics = PQntuples(topics);
   for (t = 0; t < ntopics; t++)
   {
      const char *topic_id = PQgetvalue(topics, t, 0);
      const char *filename = PQgetvalue(topics, t, 1);
      const char *keywords = PQgetvalue(topics, t, 2);
      char path[512];
      FILE *fp;
      PGresult *entries;
      int e, nentries;
      const char *v[1];

      snprintf(path, sizeof(path), "%s/%s", out_dir, filename);
      fp = fopen(path, "w");
      if (!fp)
      {
         fprintf(stderr, "WARN: cannot write %s: %s\n", path, strerror(errno));
         errors++;
         continue;
      }

      fprintf(fp, "keywords %s\n", keywords);
      fprintf(fp, "---\n");

      v[0] = topic_id;
      entries = xquery_params(
          "SELECT seq, flags, body FROM lore_entries WHERE topic_id=$1 ORDER BY seq", 1, v);
      if (entries)
      {
         nentries = PQntuples(entries);
         for (e = 0; e < nentries; e++)
         {
            const char *body = PQgetvalue(entries, e, 2);
            if (e > 0)
            {
               /* Subsequent entries have a flags header block. */
               const char *flags = PQgetvalue(entries, e, 1);
               fprintf(fp, "flags %s\n", flags);
               fprintf(fp, "---\n");
            }
            if (body && *body)
               fprintf(fp, "%s\n", body);
         }
         PQclear(entries);
      }

      fclose(fp);
      count++;
   }

   PQclear(topics);
   return count;
}

/* -----------------------------------------------------------------------
 * Area file exporters
 *
 * Each section writes to the FILE* already opened by export_one_area().
 * The format matches exactly what the server's load_* functions expect.
 * ----------------------------------------------------------------------- */

/* Area flags (must match src/headers/config.h) */
#define DF_AREA_PAYAREA 1
#define DF_AREA_TELEPORT 2
#define DF_AREA_BUILDING 4
#define DF_AREA_NOSHOW 8
#define DF_AREA_NO_ROOM_AFF 16

static void export_area_header(FILE *fp, PGresult *row, int r)
{
   const char *name = PQgetvalue(row, r, 1); /* col 1 */
   int min_vnum = atoi(PQgetvalue(row, r, 2));
   int max_vnum = atoi(PQgetvalue(row, r, 3));
   const char *keyword = PQgetvalue(row, r, 4);
   const char *level_label = PQgetvalue(row, r, 5);
   int area_number = atoi(PQgetvalue(row, r, 6));
   const char *owner = PQgetvalue(row, r, 7);
   int level_min = atoi(PQgetvalue(row, r, 8));
   int level_max = atoi(PQgetvalue(row, r, 9));
   int map_offset = atoi(PQgetvalue(row, r, 10));
   int reset_rate = atoi(PQgetvalue(row, r, 11));
   const char *reset_msg = PQgetvalue(row, r, 12);
   const char *can_read = PQgetvalue(row, r, 13);
   const char *can_write = PQgetvalue(row, r, 14);
   const char *music = PQgetvalue(row, r, 15);
   int flags = atoi(PQgetvalue(row, r, 16));
   /* col 0 = area_revision, set by caller from schema (not stored — we write
    * the current code-level revision so round-tripped files load cleanly). */
   int area_revision = atoi(PQgetvalue(row, r, 17));

   fprintf(fp, "#AREA\n");
   fw_tilde(fp, name);

   /* Directives — only write if non-default */
   if (area_revision >= 0)
      fprintf(fp, "Q %d\n", area_revision);
   fprintf(fp, "V %d %d\n", min_vnum, max_vnum);
   if (area_number > 0)
      fprintf(fp, "N %d\n", area_number);
   if (keyword && *keyword && strcmp(keyword, "none") != 0)
      fprintf(fp, "K %s~\n", keyword);
   if (level_label && *level_label)
      fprintf(fp, "L %s~\n", level_label);
   if (owner && *owner)
      fprintf(fp, "O %s~\n", owner);
   if (can_read && *can_read && strcmp(can_read, "all") != 0)
      fprintf(fp, "R %s~\n", can_read);
   if (can_write && *can_write && strcmp(can_write, "all") != 0)
      fprintf(fp, "W %s~\n", can_write);
   if (level_min > 0 || level_max > 0)
      fprintf(fp, "I %d %d\n", level_min, level_max);
   if (map_offset != 0)
      fprintf(fp, "X %d\n", map_offset);
   if (reset_rate != 15)
      fprintf(fp, "F %d\n", reset_rate);
   if (reset_msg && *reset_msg)
      fprintf(fp, "U %s~\n", reset_msg);
   if (music && *music)
      fprintf(fp, "C %s~\n", music);
   if (flags & DF_AREA_PAYAREA)
      fprintf(fp, "P\n");
   if (flags & DF_AREA_NO_ROOM_AFF)
      fprintf(fp, "M\n");
   if (flags & DF_AREA_TELEPORT)
      fprintf(fp, "T\n");
   if (flags & DF_AREA_BUILDING)
      fprintf(fp, "B\n");
   if (flags & DF_AREA_NOSHOW)
      fprintf(fp, "S\n");
}

static void export_rooms_section(FILE *fp, const char *area_id_str)
{
   PGresult *rooms;
   int r, nrooms;
   const char *v[1];

   v[0] = area_id_str;
   rooms = xquery_params("SELECT vnum, name, description, room_flags, sector_type "
                         "FROM rooms WHERE area_id=$1 ORDER BY vnum",
                         1, v);
   if (!rooms)
      return;

   nrooms = PQntuples(rooms);
   if (nrooms == 0)
   {
      PQclear(rooms);
      return;
   }

   fprintf(fp, "\n#ROOMS\n");

   for (r = 0; r < nrooms; r++)
   {
      int vnum = atoi(PQgetvalue(rooms, r, 0));
      const char *name = PQgetvalue(rooms, r, 1);
      const char *description = PQgetvalue(rooms, r, 2);
      int room_flags = atoi(PQgetvalue(rooms, r, 3));
      int sector_type = atoi(PQgetvalue(rooms, r, 4));

      char vnum_str[16];
      PGresult *exits, *edesc_res;
      int x, nexits, nedesc;
      const char *vv[1];

      fprintf(fp, "\n#%d\n", vnum);
      fw_tilde(fp, name);
      fw_tilde_block(fp, description);
      fprintf(fp, "%d %d\n", room_flags, sector_type);

      /* Exits */
      snprintf(vnum_str, sizeof(vnum_str), "%d", vnum);
      vv[0] = vnum_str;
      exits =
          xquery_params("SELECT direction, dest_vnum, exit_flags, key_vnum, keyword, description "
                        "FROM room_exits WHERE room_vnum=$1 ORDER BY direction",
                        1, vv);
      if (exits)
      {
         nexits = PQntuples(exits);
         for (x = 0; x < nexits; x++)
         {
            int dir = atoi(PQgetvalue(exits, x, 0));
            int dest = atoi(PQgetvalue(exits, x, 1));
            int exit_flags = atoi(PQgetvalue(exits, x, 2));
            int key_vnum = atoi(PQgetvalue(exits, x, 3));
            const char *ekey = PQgetvalue(exits, x, 4);
            const char *edesc = PQgetvalue(exits, x, 5);

            fprintf(fp, "D%d\n", dir);
            fw_tilde(fp, edesc);
            fw_tilde(fp, ekey);
            fprintf(fp, "%d %d %d\n", exit_flags, key_vnum, dest);
         }
         PQclear(exits);
      }

      /* Extra descs */
      edesc_res = xquery_params(
          "SELECT keyword, description FROM room_extra_descs WHERE room_vnum=$1", 1, vv);
      if (edesc_res)
      {
         nedesc = PQntuples(edesc_res);
         for (x = 0; x < nedesc; x++)
         {
            fprintf(fp, "E\n");
            fw_tilde(fp, PQgetvalue(edesc_res, x, 0));
            fw_tilde(fp, PQgetvalue(edesc_res, x, 1));
         }
         PQclear(edesc_res);
      }

      fprintf(fp, "S\n");
   }

   fprintf(fp, "\n#0\n");
   PQclear(rooms);
}

static void export_mobiles_section(FILE *fp, const char *area_id_str)
{
   PGresult *mobs;
   int r, nmobs;
   const char *v[1];

   v[0] = area_id_str;
   mobs = xquery_params(
       "SELECT vnum, player_name, short_descr, long_descr, description, "
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
   if (nmobs == 0)
   {
      PQclear(mobs);
      return;
   }

   fprintf(fp, "\n#MOBILES\n");

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
      int clan = atoi(PQgetvalue(mobs, r, col++));
      int race = atoi(PQgetvalue(mobs, r, col++));
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
      int loot[9], loot_chance[9], i;
      for (i = 0; i < 9; i++)
         loot[i] = atoi(PQgetvalue(mobs, r, col++));
      for (i = 0; i < 9; i++)
         loot_chance[i] = atoi(PQgetvalue(mobs, r, col++));

      fprintf(fp, "\n#%d\n", vnum);
      fw_tilde(fp, player_name);
      fw_tilde(fp, short_descr);
      fw_tilde(fp, long_descr);
      fw_tilde(fp, description);
      fprintf(fp, "%lld %d %d\n", act_flags, affected_by, alignment);
      fprintf(fp, "S %d\n", level);
      fprintf(fp, "%d\n", sex);
      fprintf(fp, "%d %d %d %d\n", hp_mod, ac_mod, hr_mod, dr_mod);

      /* '!' extension — always write; default 0s are harmless */
      fprintf(fp, "! %d %d %d %d %d %d %d\n", mob_class, clan, race, position, skills, cast_flags,
              def);

      /* '|' extension — only write if any field is non-zero */
      if (strong_magic || weak_magic || race_mods || power_skills || power_cast || resist ||
          suscept)
         fprintf(fp, "| %d %d %d %d %d %d %d\n", strong_magic, weak_magic, race_mods, power_skills,
                 power_cast, resist, suscept);

      /* '+' extension — only write if any field is non-zero */
      if (spellpower || crit || crit_mult || spell_crit || spell_mult || parry || dodge || block ||
          pierce)
         fprintf(fp, "+ %d %d %d %d %d %d %d %d %d\n", spellpower, crit, crit_mult, spell_crit,
                 spell_mult, parry, dodge, block, pierce);

      /* 'l'/'L' loot extensions — only write if loot_amount > 0 */
      if (loot_amount > 0)
      {
         fprintf(fp, "l %d", loot_amount);
         for (i = 0; i < 9; i++)
            fprintf(fp, " %d", loot[i]);
         fprintf(fp, "\n");
         fprintf(fp, "L");
         for (i = 0; i < 9; i++)
            fprintf(fp, " %d", loot_chance[i]);
         fprintf(fp, "\n");
      }

      /* 'a' AI extension */
      if (ai_prompt && *ai_prompt)
      {
         fprintf(fp, "a %d %d\n", ai_knowledge, accent);
         fw_tilde(fp, ai_prompt);
      }
   }

   fprintf(fp, "\n#0\n");
   PQclear(mobs);
}

static void export_objects_section(FILE *fp, const char *area_id_str)
{
   PGresult *objs;
   int r, nobjs;
   const char *v[1];

   v[0] = area_id_str;
   objs = xquery_params(
       "SELECT vnum, name, short_descr, description, "
       "item_type, extra_flags, wear_flags, item_apply, "
       "value_0, value_1, value_2, value_3, value_4, value_5, value_6, value_7, value_8, value_9, "
       "weight, level "
       "FROM objects WHERE area_id=$1 ORDER BY vnum",
       1, v);
   if (!objs)
      return;

   nobjs = PQntuples(objs);
   if (nobjs == 0)
   {
      PQclear(objs);
      return;
   }

   fprintf(fp, "\n#OBJECTS\n");

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
      int value[10];
      int i;
      for (i = 0; i < 10; i++)
         value[i] = atoi(PQgetvalue(objs, r, col++));
      int weight = atoi(PQgetvalue(objs, r, col++));
      int obj_level = atoi(PQgetvalue(objs, r, col++));

      char vnum_str[16];
      PGresult *aff_res, *ed_res;
      int x, naff, ned;
      const char *vv[1];

      fprintf(fp, "\n#%d\n", vnum);
      fw_tilde(fp, name);
      fw_tilde(fp, short_descr);
      fw_tilde(fp, description);
      fprintf(fp, "%d %lld %d %d\n", item_type, extra_flags, wear_flags, item_apply);
      fprintf(fp, "%d %d %d %d\n", value[0], value[1], value[2], value[3]);
      fprintf(fp, "%d %d %d %d %d %d\n", value[4], value[5], value[6], value[7], value[8],
              value[9]);
      fprintf(fp, "%d\n", weight);

      snprintf(vnum_str, sizeof(vnum_str), "%d", vnum);
      vv[0] = vnum_str;

      /* Affects */
      aff_res = xquery_params(
          "SELECT location, modifier FROM object_affects WHERE obj_vnum=$1 ORDER BY id", 1, vv);
      if (aff_res)
      {
         naff = PQntuples(aff_res);
         for (x = 0; x < naff; x++)
            fprintf(fp, "A %s %s\n", PQgetvalue(aff_res, x, 0), PQgetvalue(aff_res, x, 1));
         PQclear(aff_res);
      }

      /* Extra descs */
      ed_res = xquery_params(
          "SELECT keyword, description FROM object_extra_descs WHERE obj_vnum=$1", 1, vv);
      if (ed_res)
      {
         ned = PQntuples(ed_res);
         for (x = 0; x < ned; x++)
         {
            fprintf(fp, "E\n");
            fw_tilde(fp, PQgetvalue(ed_res, x, 0));
            fw_tilde(fp, PQgetvalue(ed_res, x, 1));
         }
         PQclear(ed_res);
      }

      /* Level */
      if (obj_level > 0)
         fprintf(fp, "L %d\n", obj_level);
   }

   fprintf(fp, "\n#0\n");
   PQclear(objs);
}

static void export_resets_section(FILE *fp, const char *area_id_str)
{
   PGresult *res;
   int r, nrows;
   const char *v[1];

   v[0] = area_id_str;
   res = xquery_params("SELECT command, ifflag, arg1, arg2, arg3, notes "
                       "FROM resets WHERE area_id=$1 ORDER BY seq",
                       1, v);
   if (!res)
      return;

   nrows = PQntuples(res);
   if (nrows == 0)
   {
      PQclear(res);
      return;
   }

   fprintf(fp, "\n#RESETS\n");

   for (r = 0; r < nrows; r++)
   {
      const char *cmd = PQgetvalue(res, r, 0);
      int ifflag = atoi(PQgetvalue(res, r, 1));
      int arg1 = atoi(PQgetvalue(res, r, 2));
      int arg2 = atoi(PQgetvalue(res, r, 3));
      int arg3 = atoi(PQgetvalue(res, r, 4));
      const char *notes = PQgetvalue(res, r, 5);

      /* G and R have no arg3 in the file format. */
      if (cmd[0] == 'G' || cmd[0] == 'R')
         fprintf(fp, "%c %d %d %d", cmd[0], ifflag, arg1, arg2);
      else
         fprintf(fp, "%c %d %d %d %d", cmd[0], ifflag, arg1, arg2, arg3);

      if (notes && *notes)
         fprintf(fp, " %s", notes);
      fprintf(fp, "\n");
   }

   fprintf(fp, "S\n");
   PQclear(res);
}

static void export_shops_section(FILE *fp, const char *area_id_str)
{
   PGresult *res;
   int r, nrows;
   /* Shops FK to mobiles(vnum); join to get only shops belonging to this area. */
   const char *sql =
       "SELECT s.keeper_vnum, s.buy_type_0, s.buy_type_1, s.buy_type_2, s.buy_type_3, "
       "s.buy_type_4, s.profit_buy, s.profit_sell, s.open_hour, s.close_hour "
       "FROM shops s JOIN mobiles m ON m.vnum = s.keeper_vnum "
       "WHERE m.area_id=$1 ORDER BY s.keeper_vnum";
   const char *v[1];

   v[0] = area_id_str;
   res = xquery_params(sql, 1, v);
   if (!res)
      return;

   nrows = PQntuples(res);
   if (nrows == 0)
   {
      PQclear(res);
      return;
   }

   fprintf(fp, "\n#SHOPS\n");

   for (r = 0; r < nrows; r++)
      fprintf(fp, "%s %s %s %s %s %s %s %s %s %s\n", PQgetvalue(res, r, 0), PQgetvalue(res, r, 1),
              PQgetvalue(res, r, 2), PQgetvalue(res, r, 3), PQgetvalue(res, r, 4),
              PQgetvalue(res, r, 5), PQgetvalue(res, r, 6), PQgetvalue(res, r, 7),
              PQgetvalue(res, r, 8), PQgetvalue(res, r, 9));

   fprintf(fp, "0\n");
   PQclear(res);
}

static void export_specials_section(FILE *fp, const char *area_id_str)
{
   PGresult *res;
   int r, nrows;
   const char *v[1];

   v[0] = area_id_str;
   res = xquery_params("SELECT ms.mob_vnum, ms.spec_name "
                       "FROM mobile_specials ms JOIN mobiles m ON m.vnum = ms.mob_vnum "
                       "WHERE m.area_id=$1 ORDER BY ms.mob_vnum",
                       1, v);
   if (!res)
      return;

   nrows = PQntuples(res);
   if (nrows == 0)
   {
      PQclear(res);
      return;
   }

   fprintf(fp, "\n#SPECIALS\n");
   for (r = 0; r < nrows; r++)
      fprintf(fp, "M %s %s\n", PQgetvalue(res, r, 0), PQgetvalue(res, r, 1));
   fprintf(fp, "S\n");
   PQclear(res);
}

static void export_objfuns_section(FILE *fp, const char *area_id_str)
{
   PGresult *res;
   int r, nrows;
   const char *v[1];

   v[0] = area_id_str;
   res = xquery_params("SELECT of.obj_vnum, of.fun_name "
                       "FROM object_functions of JOIN objects o ON o.vnum = of.obj_vnum "
                       "WHERE o.area_id=$1 ORDER BY of.obj_vnum",
                       1, v);
   if (!res)
      return;

   nrows = PQntuples(res);
   if (nrows == 0)
   {
      PQclear(res);
      return;
   }

   fprintf(fp, "\n#OBJFUNS\n");
   for (r = 0; r < nrows; r++)
      fprintf(fp, "O %s %s\n", PQgetvalue(res, r, 0), PQgetvalue(res, r, 1));
   fprintf(fp, "S\n");
   PQclear(res);
}

/* Export all areas to out_dir/<area_number>.are (named by area_number for
 * uniqueness; import round-trips don't need to match the original filename). */
static int export_areas(const char *out_dir)
{
   PGresult *areas;
   int a, nareas, count = 0;

   if (!ensure_dir(out_dir))
      return 0;

   /* col 0 = id (used as FK param), col 17 = area_revision proxy via flags
    * We store no area_revision in schema; export a fixed value of 16 so the
    * server's revision-dependent paths (wear_flags, race) take the modern
    * branch on re-import. */
   areas = xquery("SELECT id, name, min_vnum, max_vnum, keyword, level_label, "
                  "area_number, owner, level_min, level_max, map_offset, reset_rate, "
                  "reset_msg, can_read, can_write, music, flags, "
                  "16 AS area_revision " /* synthesised — always export as rev 16 */
                  "FROM areas ORDER BY min_vnum");
   if (!areas)
      return 0;

   nareas = PQntuples(areas);
   for (a = 0; a < nareas; a++)
   {
      const char *area_id_str = PQgetvalue(areas, a, 0);
      int min_vnum = atoi(PQgetvalue(areas, a, 2));
      char path[512];
      FILE *fp;

      /* Name each file after its min_vnum to avoid collisions. */
      snprintf(path, sizeof(path), "%s/area_%d.are", out_dir, min_vnum);
      fp = fopen(path, "w");
      if (!fp)
      {
         fprintf(stderr, "WARN: cannot write %s: %s\n", path, strerror(errno));
         errors++;
         continue;
      }

      export_area_header(fp, areas, a);
      export_rooms_section(fp, area_id_str);
      export_mobiles_section(fp, area_id_str);
      export_objects_section(fp, area_id_str);
      export_resets_section(fp, area_id_str);
      export_shops_section(fp, area_id_str);
      export_specials_section(fp, area_id_str);
      export_objfuns_section(fp, area_id_str);

      fprintf(fp, "\n#$\n");
      fclose(fp);
      count++;
   }

   PQclear(areas);
   return count;
}

/* -----------------------------------------------------------------------
 * data/ file exporters
 * ----------------------------------------------------------------------- */

static int export_bans(const char *path)
{
   PGresult *res;
   int r, nrows;
   FILE *fp;

   res = xquery("SELECT ban_type, address, banned_by FROM bans ORDER BY id");
   if (!res)
      return 0;

   fp = fopen(path, "w");
   if (!fp)
   {
      fprintf(stderr, "WARN: cannot write %s: %s\n", path, strerror(errno));
      PQclear(res);
      errors++;
      return 0;
   }

   nrows = PQntuples(res);
   for (r = 0; r < nrows; r++)
   {
      fprintf(fp, "#BAN~\n");
      fprintf(fp, "%s\n", PQgetvalue(res, r, 0)); /* ban_type */
      fw_tilde(fp, PQgetvalue(res, r, 1));        /* address  */
      fw_tilde(fp, PQgetvalue(res, r, 2));        /* banned_by */
   }
   fprintf(fp, "#END~\n");

   fclose(fp);
   PQclear(res);
   return nrows;
}

static int export_socials(const char *path)
{
   PGresult *res;
   int r, nrows;
   FILE *fp;

   res = xquery("SELECT name, char_no_arg, others_no_arg, char_found, "
                "others_found, vict_found, char_auto, others_auto "
                "FROM socials ORDER BY name");
   if (!res)
      return 0;

   fp = fopen(path, "w");
   if (!fp)
   {
      fprintf(stderr, "WARN: cannot write %s: %s\n", path, strerror(errno));
      PQclear(res);
      errors++;
      return 0;
   }

   nrows = PQntuples(res);
   fprintf(fp, "%d\n", nrows);
   for (r = 0; r < nrows; r++)
   {
      fw_tilde(fp, PQgetvalue(res, r, 0)); /* name           */
      fw_tilde(fp, PQgetvalue(res, r, 1)); /* char_no_arg    */
      fw_tilde(fp, PQgetvalue(res, r, 2)); /* others_no_arg  */
      fw_tilde(fp, PQgetvalue(res, r, 3)); /* char_found     */
      fw_tilde(fp, PQgetvalue(res, r, 4)); /* others_found   */
      fw_tilde(fp, PQgetvalue(res, r, 5)); /* vict_found     */
      fw_tilde(fp, PQgetvalue(res, r, 6)); /* char_auto      */
      fw_tilde(fp, PQgetvalue(res, r, 7)); /* others_auto    */
   }

   fclose(fp);
   PQclear(res);
   return nrows;
}

static int export_rulers(const char *path)
{
   PGresult *res;
   int r, nrows;
   FILE *fp;

   res = xquery("SELECT name FROM rulers ORDER BY id");
   if (!res)
      return 0;

   fp = fopen(path, "w");
   if (!fp)
   {
      fprintf(stderr, "WARN: cannot write %s: %s\n", path, strerror(errno));
      PQclear(res);
      errors++;
      return 0;
   }

   nrows = PQntuples(res);
   for (r = 0; r < nrows; r++)
   {
      fprintf(fp, "#RULER~\n");
      fw_tilde(fp, PQgetvalue(res, r, 0)); /* name */
      /* affiliation_index, flags, ruler_rank, keywords not stored — write defaults */
      fprintf(fp, "0\n"); /* affiliation_index */
      fprintf(fp, "0\n"); /* flags             */
      fprintf(fp, "0\n"); /* ruler_rank        */
      fw_tilde(fp, "");   /* keywords          */
   }
   fprintf(fp, "#END~\n");

   fclose(fp);
   PQclear(res);
   return nrows;
}

static int export_brands(const char *path)
{
   PGresult *res;
   int r, nrows;
   FILE *fp;

   res = xquery("SELECT branded_by, item_name, brand_date, description "
                "FROM brands ORDER BY id");
   if (!res)
      return 0;

   fp = fopen(path, "w");
   if (!fp)
   {
      fprintf(stderr, "WARN: cannot write %s: %s\n", path, strerror(errno));
      PQclear(res);
      errors++;
      return 0;
   }

   nrows = PQntuples(res);
   for (r = 0; r < nrows; r++)
   {
      fprintf(fp, "#BRAND~\n");
      fw_tilde(fp, PQgetvalue(res, r, 0)); /* branded_by  */
      fw_tilde(fp, PQgetvalue(res, r, 1)); /* item_name   */
      fw_tilde(fp, PQgetvalue(res, r, 2)); /* brand_date  */
      fw_tilde(fp, PQgetvalue(res, r, 3)); /* description */
      fw_tilde(fp, "");                    /* priority — not stored, write empty */
   }
   fprintf(fp, "#END~\n");

   fclose(fp);
   PQclear(res);
   return nrows;
}

/* Clan table size (must match src/headers/config.h MAX_CLAN). */
#define DF_MAX_CLAN 11

static int export_clans(const char *path)
{
   PGresult *res;
   FILE *fp;
   int i, j;

   res = xquery("SELECT id, gold, war_matrix FROM clans ORDER BY id");
   if (!res)
      return 0;

   if (PQntuples(res) < DF_MAX_CLAN)
   {
      fprintf(stderr, "WARN: clans table has %d rows, expected %d\n", PQntuples(res), DF_MAX_CLAN);
   }

   fp = fopen(path, "w");
   if (!fp)
   {
      fprintf(stderr, "WARN: cannot write %s: %s\n", path, strerror(errno));
      PQclear(res);
      errors++;
      return 0;
   }

   /* Reconstruct diplomacy and treasury arrays from stored data. */
   int treasury[DF_MAX_CLAN];
   int diplomacy[DF_MAX_CLAN][DF_MAX_CLAN];
   for (i = 0; i < DF_MAX_CLAN; i++)
   {
      treasury[i] = 0;
      for (j = 0; j < DF_MAX_CLAN; j++)
         diplomacy[i][j] = 0;
   }

   int nrows = PQntuples(res);
   for (i = 0; i < nrows && i < DF_MAX_CLAN; i++)
   {
      int clan_id = atoi(PQgetvalue(res, i, 0));
      treasury[clan_id] = atoi(PQgetvalue(res, i, 1));

      /* war_matrix is stored as a PostgreSQL array literal "{v0,v1,...}".
       * Parse it manually: strip braces, split on commas. */
      const char *arr = PQgetvalue(res, i, 2);
      if (arr && *arr == '{')
      {
         char buf[512];
         strncpy(buf, arr + 1, sizeof(buf) - 1);
         char *tok = strtok(buf, ",}");
         for (j = 0; j < DF_MAX_CLAN && tok; j++, tok = strtok(NULL, ",}"))
            diplomacy[clan_id][j] = atoi(tok);
      }
   }
   PQclear(res);

   /* Write in the exact format load_clan_table() reads. */
   fprintf(fp, "%d\n", DF_MAX_CLAN);
   for (i = 1; i < DF_MAX_CLAN; i++)
      for (j = 1; j < DF_MAX_CLAN; j++)
         fprintf(fp, "%d\n", diplomacy[i][j]);
   for (i = 1; i < DF_MAX_CLAN; i++)
      fprintf(fp, "%d\n", treasury[i]);
   /* end_current_state — not stored; write zeros */
   for (i = 1; i < DF_MAX_CLAN; i++)
      for (j = 1; j < DF_MAX_CLAN; j++)
         fprintf(fp, "0\n");

   fclose(fp);
   return DF_MAX_CLAN;
}

/* -----------------------------------------------------------------------
 * Board file exporter
 * ----------------------------------------------------------------------- */

static int export_boards(const char *out_dir)
{
   PGresult *boards;
   int b, nboards, count = 0;

   if (!ensure_dir(out_dir))
      return 0;

   boards = xquery("SELECT id, vnum, expiry_days, min_read_lev, min_write_lev, clan "
                   "FROM boards ORDER BY vnum");
   if (!boards)
      return 0;

   nboards = PQntuples(boards);
   for (b = 0; b < nboards; b++)
   {
      const char *board_id_str = PQgetvalue(boards, b, 0);
      int vnum = atoi(PQgetvalue(boards, b, 1));
      int expiry_days = atoi(PQgetvalue(boards, b, 2));
      int min_read_lev = atoi(PQgetvalue(boards, b, 3));
      int min_write_lev = atoi(PQgetvalue(boards, b, 4));
      int clan = atoi(PQgetvalue(boards, b, 5));

      char path[512];
      FILE *fp;
      PGresult *msgs;
      int m, nmsgs;
      const char *v[1];

      snprintf(path, sizeof(path), "%s/board.%d", out_dir, vnum);
      fp = fopen(path, "w");
      if (!fp)
      {
         fprintf(stderr, "WARN: cannot write %s: %s\n", path, strerror(errno));
         errors++;
         continue;
      }

      fprintf(fp, "ExpiryTime %d\n", expiry_days);
      fprintf(fp, "MinReadLev %d\n", min_read_lev);
      /* File uses "MaxWriteLev"; server load_board() reads both spellings. */
      fprintf(fp, "MaxWriteLev %d\n", min_write_lev);
      fprintf(fp, "Clan %d\n", clan);
      fprintf(fp, "Messages\n");

      v[0] = board_id_str;
      msgs = xquery_params("SELECT posted_at, author, title, body "
                           "FROM board_messages WHERE board_id=$1 ORDER BY seq",
                           1, v);
      if (msgs)
      {
         nmsgs = PQntuples(msgs);
         for (m = 0; m < nmsgs; m++)
         {
            fprintf(fp, "M%s\n", PQgetvalue(msgs, m, 0)); /* posted_at timestamp */
            fw_tilde(fp, PQgetvalue(msgs, m, 1));         /* author  */
            fw_tilde(fp, PQgetvalue(msgs, m, 2));         /* title   */
            fw_tilde(fp, PQgetvalue(msgs, m, 3));         /* body    */
         }
         PQclear(msgs);
      }

      fprintf(fp, "S\n");
      fclose(fp);
      count++;
   }

   PQclear(boards);
   return count;
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */

static void usage(const char *prog)
{
   fprintf(stderr,
           "Usage: %s [connstr] [--areas-only|--help-only|--lore-only|"
           "--data-only|--boards-only]\n"
           "\n"
           "connstr   PostgreSQL connection string (default: data/db.conf)\n"
           "\n"
           "Without a filter flag all content stores are exported.\n"
           "Run from the area/ directory (same as the server).\n",
           prog);
}

int main(int argc, char *argv[])
{
   const char *explicit_connstr = NULL;
   int do_areas = 1, do_help = 1, do_lore = 1, do_data = 1, do_boards = 1;
   int filter_set = 0;
   int i;
   char *connstr;
   int total_errors;

   /* Parse arguments: optional connstr (no leading --) then optional flags. */
   for (i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "--areas-only") == 0)
      {
         if (!filter_set)
         {
            do_areas = 0;
            do_help = 0;
            do_lore = 0;
            do_data = 0;
            do_boards = 0;
            filter_set = 1;
         }
         do_areas = 1;
      }
      else if (strcmp(argv[i], "--help-only") == 0)
      {
         if (!filter_set)
         {
            do_areas = 0;
            do_help = 0;
            do_lore = 0;
            do_data = 0;
            do_boards = 0;
            filter_set = 1;
         }
         do_help = 1;
      }
      else if (strcmp(argv[i], "--lore-only") == 0)
      {
         if (!filter_set)
         {
            do_areas = 0;
            do_help = 0;
            do_lore = 0;
            do_data = 0;
            do_boards = 0;
            filter_set = 1;
         }
         do_lore = 1;
      }
      else if (strcmp(argv[i], "--data-only") == 0)
      {
         if (!filter_set)
         {
            do_areas = 0;
            do_help = 0;
            do_lore = 0;
            do_data = 0;
            do_boards = 0;
            filter_set = 1;
         }
         do_data = 1;
      }
      else if (strcmp(argv[i], "--boards-only") == 0)
      {
         if (!filter_set)
         {
            do_areas = 0;
            do_help = 0;
            do_lore = 0;
            do_data = 0;
            do_boards = 0;
            filter_set = 1;
         }
         do_boards = 1;
      }
      else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
      {
         usage(argv[0]);
         return 0;
      }
      else if (argv[i][0] != '-')
      {
         /* Positional argument — treat as connection string. */
         if (explicit_connstr)
         {
            fprintf(stderr, "Error: unexpected argument '%s'\n", argv[i]);
            usage(argv[0]);
            return 1;
         }
         explicit_connstr = argv[i];
      }
      else
      {
         fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
         usage(argv[0]);
         return 1;
      }
   }

   /* Connect ---------------------------------------------------------------- */
   if (explicit_connstr)
   {
      connstr = NULL; /* use argv value directly */
      conn = PQconnectdb(explicit_connstr);
   }
   else
   {
      /* Try to read ../data/db.conf (CWD must be area/). */
      connstr = read_db_conf(".");
      conn = PQconnectdb(connstr ? connstr : "");
      free(connstr);
   }

   if (PQstatus(conn) != CONNECTION_OK)
   {
      fprintf(stderr, "db_to_files: connection failed: %s\n", PQerrorMessage(conn));
      PQfinish(conn);
      return 1;
   }
   fprintf(stderr, "db_to_files: connected\n");

   /* Export ----------------------------------------------------------------- */

   if (do_areas)
   {
      int n = export_areas("../area/export");
      fprintf(stderr, "  areas:   %d files written\n", n);
   }

   if (do_help)
   {
      int n = export_helpdir("../help/export", "help_entries");
      fprintf(stderr, "  help:    %d files written\n", n);
      n = export_helpdir("../shelp/export", "shelp_entries");
      fprintf(stderr, "  shelp:   %d files written\n", n);
   }

   if (do_lore)
   {
      int n = export_lore("../lore/export");
      fprintf(stderr, "  lore:    %d files written\n", n);
   }

   if (do_data)
   {
      int n;
      if (!ensure_dir("../data/export"))
      {
         fprintf(stderr, "WARN: could not create ../data/export\n");
         errors++;
      }
      else
      {
         n = export_bans("../data/export/bans.lst");
         fprintf(stderr, "  bans:    %d entries\n", n);

         n = export_socials("../data/export/socials.txt");
         fprintf(stderr, "  socials: %d entries\n", n);

         n = export_rulers("../data/export/rulers.lst");
         fprintf(stderr, "  rulers:  %d entries\n", n);

         n = export_brands("../data/export/brands.lst");
         fprintf(stderr, "  brands:  %d entries\n", n);

         n = export_clans("../data/export/clandata.dat");
         fprintf(stderr, "  clans:   %d slots written\n", n);
      }
   }

   if (do_boards)
   {
      int n = export_boards("../area/boards/export");
      fprintf(stderr, "  boards:  %d files written\n", n);
   }

   /* Teardown --------------------------------------------------------------- */
   PQfinish(conn);
   conn = NULL;

   total_errors = errors;
   if (total_errors > 0)
      fprintf(stderr, "db_to_files: finished with %d error(s)\n", total_errors);
   else
      fprintf(stderr, "db_to_files: done\n");

   return total_errors > 0 ? 1 : 0;
}
