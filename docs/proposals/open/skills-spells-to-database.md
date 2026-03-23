# Proposal: Move Skill/Spell Definitions to Database

**Status:** Open
**Date:** 2026-03-23
**Repos affected:** acktng, tngdb

---

## Problem

All skill and spell metadata is compiled directly into the game binary via two large C data files:

- `src/spells/spell_table_data.c` (~3087 lines, ~200+ spells)
- `src/skills/skill_table_data.c` (~3358 lines, ~300+ skills)

These files are `#include`d into `const.c` to initialize `skill_table[MAX_SKILL]` — a static 999-slot array. Adding, removing, or tuning any skill or spell requires editing C source and recompiling the game. There is no runtime visibility into skill data; the web frontend and tngdb have no access to it at all.

---

## Goal

Move skill/spell **metadata** (name, per-class level requirements, mana cost, beats, targeting, messages, etc.) from compiled C into PostgreSQL, loaded at boot. Spell and skill **logic** (the `do_*` and `spell_*` C functions) stays in C — this proposal does not touch those.

Benefits:
- Skills/spells can be tuned without a recompile
- tngdb can expose a `/skills` and `/spells` endpoint for the web frontend
- Class-level assignments become queryable (e.g. "what can a level-30 Mage cast?")
- Reduces size of compiled binary and two ~3000-line data-only C files

---

## Background: Current Architecture

### `struct skill_type` (ack.h:1268)

```c
struct skill_type {
    sh_int   flag2;                        // NORM(1) = normal skill/spell
    char    *name;                         // "fireball", "backstab"
    sh_int   skill_level[MAX_TOTAL_CLASS]; // required level per class; NO_USE(-999) = unavailable
    SPELL_FUN *spell_fun;                  // function pointer (NULL/spell_null for pure skills)
    sh_int   target;                       // TAR_* constant
    sh_int   minimum_position;            // POS_* constant
    sh_int  *pgsn;                         // pointer to gsn_* global variable
    sh_int   slot;                         // #OBJECT slot number
    sh_int   min_mana;                     // base mana cost (or energy base for skills)
    sh_int   beats;                        // lag after use
    bool     can_learn;
    char    *noun_damage;
    char    *msg_off;
    char    *room_off;
    sh_int   growth;                       // druid overgrowth per cast
};
```

### GSN Variables

Each skill/spell that needs fast lookup has a `gsn_*` global `sh_int` (e.g. `gsn_backstab`). At boot, `db.c:554` iterates the table and writes each entry's array index into the pointed-to `gsn_*` variable. Code then uses `skill_table[gsn_backstab]` directly.

### Two Non-Negotiable Constraints

1. **Stable SNs.** The skill number (`sn`) — the index into `skill_table[]` — is embedded in player save data. Rows cannot be renumbered arbitrarily.
2. **Function pointers cannot be stored in a DB.** `SPELL_FUN *spell_fun` and `sh_int *pgsn` must be resolved at boot by name lookup against a C-side registry.

---

## Proposed Approach

### Phase 1 — Schema and Data Migration (DB as source of truth, C still boots fine)

Add a `skills` table to the acktng PostgreSQL schema. Populate it from the existing C data files via a one-time migration script. The game still boots from the C table — Phase 1 is additive only. This lets tngdb serve the data immediately and gives us a chance to validate the migration before cutting over.

#### Database Schema

```sql
CREATE TABLE skills (
    sn            SMALLINT    PRIMARY KEY,        -- stable index, matches current array position
    name          TEXT        NOT NULL UNIQUE,
    flag2         SMALLINT    NOT NULL DEFAULT 1, -- 1=NORM
    spell_fun     TEXT        NOT NULL DEFAULT '', -- C function name, e.g. "spell_fireball"; '' for skills
    target        SMALLINT    NOT NULL DEFAULT 0,
    min_position  SMALLINT    NOT NULL DEFAULT 0,
    gsn_name      TEXT        NOT NULL DEFAULT '', -- C gsn variable name, e.g. "gsn_backstab"; '' if none
    slot          SMALLINT    NOT NULL DEFAULT 0,
    min_mana      SMALLINT    NOT NULL DEFAULT 0,
    beats         SMALLINT    NOT NULL DEFAULT 0,
    can_learn     BOOLEAN     NOT NULL DEFAULT TRUE,
    noun_damage   TEXT        NOT NULL DEFAULT '',
    msg_off       TEXT        NOT NULL DEFAULT '',
    room_off      TEXT        NOT NULL DEFAULT '',
    growth        SMALLINT    NOT NULL DEFAULT 0,
    class_levels  JSONB       NOT NULL DEFAULT '{}'  -- {"MAG": 5, "CLE": 10, ...}; absent = NO_USE
);
```

`class_levels` is a JSONB object mapping class abbreviation to minimum level. Classes absent from the object are treated as `NO_USE`. This mirrors the sparse `{LEVELS_INIT, L(CLASS_MAG, 5)}` initializer pattern.

#### Migration Script

A Python script (`acktng/tools/migrate_skills_to_db.py`) parses `spell_table_data.c` and `skill_table_data.c` and inserts rows into the `skills` table. It does not modify C source.

### Phase 2 — Load `skill_table[]` from DB at Boot

Replace the static `skill_table[MAX_SKILL]` with a dynamically allocated version loaded from the database during `boot_db()`.

#### C-Side Registries (new file: `src/db/db_skills.c`)

Two static arrays map string names to C symbols — populated at compile time, queried at boot:

```c
// spell function registry
typedef struct { const char *name; SPELL_FUN *fun; } SpellFunEntry;
static const SpellFunEntry spell_fun_registry[] = {
    { "spell_fireball",  spell_fireball },
    { "spell_cure_light", spell_cure_light },
    // ...
    { NULL, NULL }
};

// gsn pointer registry
typedef struct { const char *name; sh_int *ptr; } GsnEntry;
static const GsnEntry gsn_registry[] = {
    { "gsn_backstab",  &gsn_backstab },
    { "gsn_kick",      &gsn_kick },
    // ...
    { NULL, NULL }
};
```

#### Boot Loading (`boot_db()`)

`db_load_skill_table()` (called from `boot_db()`) executes `SELECT * FROM skills ORDER BY sn` and:

1. Allocates `skill_table[MAX_SKILL]` on the heap (or a file-scope array — same declaration, different initializer).
2. For each row, populates a `struct skill_type`:
   - Resolves `spell_fun` string → function pointer via registry (falls back to `spell_null`).
   - Resolves `gsn_name` string → `sh_int *` via registry; writes `sn` into the pointed-to variable.
   - Converts `class_levels` JSONB → `sh_int skill_level[MAX_TOTAL_CLASS]` (all slots start at `NO_USE`).
3. Entries not present in the DB remain zeroed/null (inert).

After Phase 2 lands, `spell_table_data.c` and `skill_table_data.c` are deleted, and `const.c` removes the `#include` lines.

### Phase 3 — tngdb `/skills` Endpoint (tngdb)

Add read-only endpoints to `tngdb/api/main.py`:

- `GET /skills` — paginated list, supports `?type=spell` / `?type=skill` filter
- `GET /skills/{sn}` — single entry by sn
- `GET /skills/lookup/{name}` — lookup by name (exact then prefix, matching game logic)

Response shape mirrors `struct skill_type` fields plus class_levels as a map.

---

## Affected Files

### acktng

| File | Change |
|---|---|
| `area/schema.sql` | Add `skills` table |
| `src/headers/ack.h` | `skill_table[]` declaration: change from `const` to non-const for dynamic load |
| `src/headers/globals.h` | `extern struct skill_type skill_table[]` unchanged; possibly `extern int skill_table_size` |
| `src/const.c` | Remove `skill_table[]` initializer (Phase 2) |
| `src/db.c` | Call `db_load_skill_table()` in `boot_db()`; remove GSN assignment loop |
| `src/db/db_skills.c` | New: DB load function, spell_fun registry, gsn registry |
| `src/db/db_skills.h` | New: `db_load_skill_table()` declaration |
| `src/spells/spell_table_data.c` | Deleted (Phase 2) |
| `src/skills/skill_table_data.c` | Deleted (Phase 2) |
| `tools/migrate_skills_to_db.py` | New: one-time migration script |

### tngdb

| File | Change |
|---|---|
| `api/main.py` | Add `/skills`, `/skills/{sn}`, `/skills/lookup/{name}` endpoints (Phase 3) |

---

## Trade-offs and Risks

### Risks

| Risk | Mitigation |
|---|---|
| SN stability: renumbering rows would corrupt player save data | `sn` is the PRIMARY KEY; it must never be reassigned. Migration preserves existing positions. Rows can be added at new SNs but never moved. |
| Function pointer registry must stay in sync with actual C functions | Build-time: if a `spell_fun` or `gsn_name` in the DB has no registry entry, log a fatal error at boot. A CI check script can cross-validate registry vs DB. |
| DB unavailable at boot | `db_load_skill_table()` calls `exit()` on failure — same as current behavior for other DB connections at boot. No silent degradation. |
| `class_levels` JSONB requires class name → index mapping | A static `const char *class_names[MAX_TOTAL_CLASS]` array in `db_skills.c` provides the mapping. Same names used everywhere. |
| Migration script misparses a data file entry | Phase 1 is read-only; the game still boots from C. Migration output is validated against the original C data before Phase 2 cutover. |

### Intentional Non-Changes

- Spell/skill **logic** (the `do_*`, `spell_*` functions) stays in C. This is purely a data migration.
- `MAX_SKILL` (999) stays as the hard ceiling. The DB just needs to not exceed it.
- Player proficiency (`learned[]`) and cooldown (`cooldown[]`) arrays are indexed by `sn` — no change.
- `skill_lookup()` logic stays the same; it searches `skill_table[]` which is now DB-loaded.

---

## Phasing Summary

| Phase | What changes | Safe to ship independently? |
|---|---|---|
| 1 | `skills` DB table + migration script + tngdb endpoint stub | Yes — game binary unchanged |
| 2 | acktng boots from DB; deletes C data files | Yes, after Phase 1 validated |
| 3 | tngdb `/skills` endpoints live | Yes, after Phase 1 |

---

## Open Questions for Discussion

1. **Class level encoding:** JSONB keyed by abbreviation (e.g. `"MAG"`) vs. a 29-element integer array vs. a separate `skill_class_levels` join table? JSONB is proposed for compactness and queryability.
2. **Tuning workflow:** After Phase 2, editing a skill means a DB UPDATE + game reload (or a future hot-reload tick). Is a game reload acceptable, or should we design for hot-reload from the start?
3. **Scope creep guard:** NPC `skills`/`power_skills` bitfields in the `mobs` table are separate from this effort — leave them alone.
4. **tngdb auth:** The `/skills` endpoint will be public read-only, consistent with existing tngdb endpoints.
