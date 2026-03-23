# Quest Database Migration

## Problem

Quest templates (`.prop` files in `quests/`) are the only major content type not yet included in the PostgreSQL database migration. The `import_to_db` tool imports help, shelp, lore, areas (rooms/mobs/objects), bans, and socials — but skips quests entirely. This means the database does not have a complete picture of game content.

## Approach

### 1. Add `quest_templates` table to `area/schema.sql`

```sql
CREATE TABLE IF NOT EXISTS quest_templates (
    id                       INTEGER PRIMARY KEY,
    title                    TEXT    NOT NULL,
    prerequisite_template_id INTEGER NOT NULL DEFAULT -1,
    type                     INTEGER NOT NULL DEFAULT 0,
    num_targets              INTEGER NOT NULL DEFAULT 0,
    target_vnums             INTEGER[] NOT NULL DEFAULT '{}',
    kill_needed              INTEGER NOT NULL DEFAULT 0,
    min_level                INTEGER NOT NULL DEFAULT 0,
    max_level                INTEGER NOT NULL DEFAULT 170,
    offerer_vnum             INTEGER NOT NULL DEFAULT 0,
    reward_gold              INTEGER NOT NULL DEFAULT 0,
    reward_qp                INTEGER NOT NULL DEFAULT 0,
    reward_exp               INTEGER NOT NULL DEFAULT 0,
    accept_message           TEXT    NOT NULL DEFAULT '',
    completion_message       TEXT    NOT NULL DEFAULT '',
    reward_obj_short         TEXT    NOT NULL DEFAULT '',
    reward_obj_name          TEXT    NOT NULL DEFAULT '',
    reward_obj_long          TEXT    NOT NULL DEFAULT '',
    reward_obj_wear_flags    INTEGER NOT NULL DEFAULT 0,
    reward_obj_extra_flags   INTEGER NOT NULL DEFAULT 0,
    reward_obj_weight        INTEGER NOT NULL DEFAULT 0,
    reward_obj_item_apply    INTEGER NOT NULL DEFAULT 0
);
```

Uses `INTEGER[]` for `target_vnums` to store the variable-length target vnum list (up to `QUEST_MAX_TARGETS`), matching how PostgreSQL naturally handles arrays and avoiding a separate join table for a simple list.

### 2. Add `import_quests()` to `src/tools/import_to_db.c`

A new function that:
- Scans the `../quests` directory for `*.prop` files
- Parses each file using the same format as `template.c:load_quest_template_file()`:
  - Line 1: title
  - Line 2: numeric fields (prerequisite_template_id, type, num_targets, kill_needed, min_level, max_level, offerer_vnum, reward_gold, reward_qp, reward_exp)
  - Line 3: space-separated target vnums
  - Line 4: accept_message
  - Line 5: completion_message
  - Lines 6-9 (optional): reward_obj_short, reward_obj_name, reward_obj_long, reward_obj_wear_flags
  - Lines 10-11 (optional): reward_obj_extra_flags, reward_obj_weight
  - Line 12 (optional): reward_obj_item_apply
- Inserts into `quest_templates` with parameterized queries
- Called from `main()` alongside the existing import steps

### 3. Add `quest_templates` to tngdb schema

Add the same table definition to `tngdb/area/schema.sql` so the tngdb API database stays in sync.

### 4. Add migration script

Add `tngdb/area/migrations/003_add_quest_templates.sql` (or next available number) with the `CREATE TABLE IF NOT EXISTS` statement.

## Affected Files

- `acktng/area/schema.sql` — add `quest_templates` table
- `acktng/src/tools/import_to_db.c` — add `import_quests()` function and call from `main()`
- `acktng/fixtures/test_data.sql` — add sample quest template rows for integration tests
- `tngdb/area/schema.sql` — add matching `quest_templates` table
- `tngdb/area/migrations/` — add migration script

## Trade-offs

- **Array column vs join table**: Using `INTEGER[]` for target_vnums is simpler and matches the fixed-size array in the C struct. A join table would be more normalized but adds complexity for no practical benefit since targets are always read/written as a unit.
- **Parsing duplication**: The `.prop` parser in `import_to_db.c` will duplicate some logic from `template.c`. This is intentional — the import tool is a standalone binary that doesn't link against the server, so it can't reuse server code directly.
