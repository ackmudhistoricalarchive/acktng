# Server-Side Quest Template Loading from Database

## Problem

Quest templates are currently loaded from `.prop` flat files at boot (`template.c:quest_load_templates()`), even when the database is available. All other content types (areas, rooms, mobs, objects, help, shelp, lore, socials, bans, etc.) already load from the database when `HAVE_LIBPQ` is defined and `db_connected` is true. Quests are the last holdout.

## Approach

Follow the same pattern used by every other DB loader in the codebase.

### 1. Add `db_load_quest_templates()` to `db/db_load.c`

New function that:
- Queries `SELECT * FROM quest_templates ORDER BY id`
- Iterates result rows, populating `QUEST_TEMPLATE` structs
- Parses `target_vnums` from the PostgreSQL integer array format (`{1234,5678,…}`)
- Grows the `quest_template_table` array and increments `quest_template_count`
- Uses `str_dup()` for all string fields (matching existing convention)
- Maps NULL `prerequisite_template_id` to -1 (the C convention for "none")
- Maps NULL `offerer_vnum` to 0

### 2. Add declaration to `db/db_load.h`

```c
void db_load_quest_templates(void);
```

### 3. Wire into `db.c` boot sequence

Replace the unconditional `quest_load_templates()` call with:

```c
#ifdef HAVE_LIBPQ
   if (db_connected)
   {
      log_f("DB: loading quest templates from database.");
      db_load_quest_templates();
   }
   else
#endif
   {
      log_f("Loading quest templates.");
      quest_load_templates();
   }
```

The flat-file loader remains as the fallback when the database is unavailable.

### 4. Expose `quest_load_templates` declaration

`quest_load_templates()` is already declared in `quest_internal.h`, which `db.c` includes — no change needed.

## Affected Files

- `src/db/db_load.c` — add `db_load_quest_templates()`
- `src/db/db_load.h` — add declaration
- `src/db.c` — wire DB path with flat-file fallback

## Trade-offs

- No new tables or schema changes required — uses the existing `quest_templates` table
- Flat-file fallback preserved — servers without PostgreSQL continue to work unchanged
- The DB loader is simpler than the flat-file parser since PostgreSQL handles field separation
