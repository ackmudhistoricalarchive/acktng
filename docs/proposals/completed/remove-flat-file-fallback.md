# Proposal: Remove Flat-File Fallback Paths and Stale Data Files

## Status: Open

## Problem

The server has a dual-load architecture leftover from before the DB migration. Every
boot-time loader in `boot_db()` follows the pattern:

```c
#ifdef HAVE_LIBPQ
if (db_connected) { db_load_X(); }
else
#endif
{ /* read from flat file */ }
```

Similarly, the runtime save functions (`save_marks`, `save_bans`, `save_rulers`,
`save_sysdata`, `save_brands`, social save) dual-write: they write to a flat file first,
then also write to the DB via `#ifdef HAVE_LIBPQ`. And `save_area_files.c` contains a
large `build_save*` state machine (~600 lines) that is completely unreachable when
compiled with `HAVE_LIBPQ`.

The flat-file fallback is dead weight. Production always runs with libpq and a configured
DB. The result:
- Stale flat files (`.are`, `.prop`, `bans.lst`, `rulers.lst`, etc.) that are out-of-date
  copies of DB content
- Large dead-code branches complicating every boot-time loader
- A false "safe fallback" that would actually boot with stale or empty data

## Approach

### Phase 1 — Code: remove fallback branches and simplify

1. **`src/db.c` `boot_db()`** — Remove all `else` branches from the `if (db_connected)`
   blocks. Make a missing or failed DB connection an unconditional abort (it already
   aborts on failure if `db.conf` exists; now absence of `db.conf` also aborts). Remove
   the `db_connected` static variable.

2. **`src/save/save_areas.c`** — `save_marks()` and `save_bans()`: remove flat-file
   write blocks, keep only `db_worker_save_*()` calls (ungated). Remove `load_marks()`
   and `load_bans()` (only called from removed fallback path).

3. **`src/save/save_rulers.c`** — Same treatment for `save_rulers()`. Remove
   `load_rulers()`.

4. **`src/save/save_sysdata.c`** — Same treatment for `save_sysdata()`. Remove
   `load_sysdata()`.

5. **`src/save/save_socials.c`** — Remove flat-file write from social save. Remove
   `load_social_table()`.

6. **`src/spendqp.c`** — `save_brands()`: remove flat-file write, keep DB call (ungated).

7. **`src/save/save_area_files.c`** — Remove the entire `build_save*` flat-file OLC save
   state machine and associated globals. Simplify `do_savearea()` to just the DB path.

8. **`src/quests/template.c`** — Remove `quest_load_templates()` and
   `load_quest_template_file()` (only called from removed fallback path). The DB path
   `db_load_quest_templates()` in `db_load.c` remains.

9. **`src/headers/ack.h`** — Remove prototypes for deleted functions.

10. **`src/Makefile`** — Remove `tools/import_to_db` and `tools/db_to_files` targets.

### Phase 2 — Delete stale flat files

| File/Dir | Reason |
|---|---|
| `area/*.are` (52 files) | Area content authoritative in DB |
| `area/area.lst` | Only used by flat-file area loop |
| `area/boards/board.*` | Boards authoritative in DB |
| `quests/*.prop` (~158 files) | Quest templates authoritative in DB |
| `data/bans.lst` | Bans authoritative in DB |
| `data/brands.lst` | Brands authoritative in DB |
| `data/rulers.lst` | Rulers authoritative in DB |
| `data/clandata.dat` | Clan diplomacy authoritative in DB |
| `data/socials.txt` | Socials authoritative in DB |
| `data/system.dat` | Sysdata authoritative in DB |
| `data/sysdat.bln` | Sysdata blank — no longer needed |
| `data/system.blank` | Same |
| `data/roommarks.lst` | Room marks authoritative in DB |
| `src/tools/import_to_db.c` | Migration tool, no longer needed |
| `src/tools/db_to_files.c` | Migration tool, no longer needed |

**What stays:** `area/schema.sql`, `area/objs.vnums`, `area/migrate_v2_cast_rename.sql`,
`data/knowledge/`, `data/tls/`, `data/training/`, `data/chest/`.

## Trade-offs

**Pros:**
- Removes ~800+ lines of dead code across multiple files
- Eliminates stale flat files that could mislead future contributors
- Single source of truth: the DB
- `boot_db()` becomes dramatically simpler

**Cons:**
- The server will abort if `data/db.conf` is absent or DB is unreachable — no
  flat-file fallback. Correct for production; bare checkout won't boot without DB setup.
- `.are` and `.prop` files have been the canonical content-editing format for decades;
  removing them means content edits go through OLC in-game or the tngdb API. Content
  is preserved in the DB.

## Testing

```sh
cd src && make lint && make ack && make unit-tests
```

Integration tests boot the server from DB, confirming the DB-only path is fully
functional after fallback removal.
