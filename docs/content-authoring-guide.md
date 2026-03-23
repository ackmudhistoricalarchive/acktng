# Game Content Authoring Guide

**Scope:** Areas, Quests, Lore, Help, Shelp — creation, editing, deletion, and cross-referencing

---

## 1. Overview

This document describes the authoritative workflow for adding, editing, and removing game content in ACK!TNG. It covers all five content types, explains how they cross-reference one another, and gives concrete examples including how to fix a quest with wrong target vnums.

The five content types and their storage:

| Type | Storage | Authoritative Source |
|------|---------|----------------------|
| Areas | PostgreSQL DB (rooms, mobs, objects, etc.) | Ingested from `area/incoming/<keyword>/` YAML dirs |
| Quests | Flat `.prop` files in `quests/` | `quests/<n>.prop` (loaded at server startup) |
| Lore | PostgreSQL DB (`lore_topics`, `lore_entries`) | SQL inserts; flat files in `lore/` are legacy |
| Help | PostgreSQL DB (`help_entries`) | SQL inserts; flat files in `help/` are legacy |
| Shelp | PostgreSQL DB (`shelp_entries`) | SQL inserts; flat files in `shelp/` are legacy |

---

## 2. Areas

### 2.1 What areas contain

An area is the primary container for game world content: rooms, exits, mobs, objects, shops, resets, and special functions. All content that players walk through, fight, loot, or buy lives in areas.

### 2.2 Directory structure

Submit a new area as a directory placed in `area/incoming/`:

```
area/incoming/
  <keyword>/
    area.yaml       ← required
    rooms.yaml      ← required
    mobs.yaml       ← optional (needed if you want mobs)
    objects.yaml    ← optional
    shops.yaml      ← optional
    resets.yaml     ← optional (needed for spawning mobs/objects)
    specials.yaml   ← optional
    objfuns.yaml    ← optional
```

The directory name must be lowercase `a–z`, `0–9`, `_` only and must exactly match the `keyword` field in `area.yaml`.

### 2.3 Vnum allocation

Every mob, object, and room in an area needs a globally unique virtual number (vnum). Check `docs/area_index.md` for the current vnum allocations. New areas must claim a contiguous range that does not overlap any existing area.

### 2.4 Creating a new area

1. Pick a keyword (e.g., `stonekeep`) and a vnum range (e.g., `32000–32199`).
2. Create `area/incoming/stonekeep/area.yaml`:

   ```yaml
   keyword: stonekeep
   name: The Ruins of Stonekeep
   credits: AuthorName
   level_min: 40
   level_max: 70
   vnum_low: 32000
   vnum_high: 32199
   update_existing: false
   ```

3. Create `area/incoming/stonekeep/rooms.yaml` with at least one room:

   ```yaml
   rooms:
     - vnum: 32000
       name: "Crumbling Gatehouse"
       description: "Ancient stonework crumbles around you..."
       sector: inside
       flags: []
       exits:
         north:
           to_room: 32001
   ```

4. (Optional) Add `mobs.yaml`, `objects.yaml`, `resets.yaml` for creatures and items.

5. Ingest the area — either wait for the next `PULSE_AREA` tick, or run immediately:

   ```
   areaingest run stonekeep
   ```

   On success the directory is deleted and the area goes live. On failure it moves to `area/incoming/failed/stonekeep/` with an error file.

### 2.5 Updating an existing area

Set `update_existing: true` in `area.yaml`, then drop the directory into `area/incoming/` again. The server deletes all existing DB rows for that area and replaces them. The area is hot-reloaded.

### 2.6 Key constraints (rejection causes)

- Keyword mismatch between directory name and `area.yaml` → reject.
- Missing `rooms.yaml` → reject immediately.
- `resets.yaml` references a vnum not in this submission and not in the DB → reject.
- Unrecognized files in the directory → reject.
- `update_existing: false` but keyword already in DB → reject.
- `update_existing: true` but keyword not in DB → reject.

See `docs/area_file_spec.md` for the full specification.

---

## 3. Quests

### 3.1 What quests are

Static quest templates are `.prop` files that define postmaster quests: kill-variety, collect-item, kill-count, and cartography objectives. They are chained by prerequisite and gated by level range.

### 3.2 File location and numbering

Quest files live in `quests/` (relative to the repo root). They are loaded at server startup from `quests/1.prop` through `quests/105.prop` (the current `QUEST_MAX_STATIC_QUESTS`). The static quest ID is `file_number - 1` (so `1.prop` = ID 0).

### 3.3 File format

```
Title text here
prerequisite_static_id  type  num_targets  kill_needed  min_level  max_level  offerer_vnum  reward_gold  reward_qp  reward_exp  0
<target vnum 1> <target vnum 2> ... (up to 5)
Accept message shown to player.
Completion message shown on hand-in.
```

Optionally followed by a custom reward-object block (short desc, keywords, long desc, wear_flags, extra_flags, weight, apply_selector).

Field notes:
- `prerequisite_static_id`: the static quest ID that must be completed first; `-1` = no prerequisite.
- `type`: `1` = kill one of each target, `2` = collect items, `3` = kill N of one target (uses `kill_needed`), `4` = cartography (map every room in the area containing that vnum).
- `offerer_vnum`: the mob that players talk to for this quest and hand in to.
- The 11th integer on the definition line is ignored (legacy field) — always write `0`.

### 3.4 Creating a new quest

1. Determine the next available quest number. Currently `1.prop`–`105.prop` are taken; a new quest would be `106.prop` (requiring a `QUEST_MAX_STATIC_QUESTS` bump in `src/config.h`).
2. Decide: standalone (`prereq = -1`) or chained (prereq = ID of the prerequisite quest).
3. Choose target mob vnums from an existing area (verify they exist in the DB or area files).
4. Choose the `offerer_vnum` — a postmaster NPC. Known postmaster vnums:

   | City | Vnum |
   |------|------|
   | Midgaard | 3015 |
   | Kiess | 3340 |
   | Kowloon | 3440 |
   | Mafdet | 3539 |
   | Rakuen | 4339 |

5. Write `quests/<n>.prop` following the format above.
6. If `n > QUEST_MAX_STATIC_QUESTS`, increment `QUEST_MAX_STATIC_QUESTS` in `src/config.h` and rebuild.

### 3.5 Editing an existing quest (e.g., wrong target vnums)

This is the most common quest fix. Example scenario: quest 81 (`82.prop`) references mob vnum `6130` but the correct vnum is `6133`.

**Step 1 — Identify the quest file.**

The static quest ID is `file_number - 1`. From `docs/quests_spec.md`'s table, find the static ID (81) → `82.prop`.

**Step 2 — Read the file and locate the target vnum line.**

The target vnum line is line 3 (after the title and the 11-integer definition line).

**Step 3 — Verify the correct vnum exists.**

```sql
-- Confirm mob 6133 exists
SELECT vnum, short_desc FROM mob_templates WHERE vnum = 6133;
```

Or check `area/objs.vnums` / search `docs/area_index.md` for the vnum range that contains 6133.

**Step 4 — Edit `quests/82.prop`.** Change `6130` to `6133` on the target vnum line.

**Step 5 — Restart the server** (quests are loaded at startup only; there is no hot-reload for quest templates). The fix takes effect on the next boot.

### 3.6 Removing a quest

Delete the `.prop` file. If it was in the middle of the chain (not the last file), the chain breaks because the quest with `prereq = <deleted_id>` will require a now-absent completion. Either update the chain or remove trailing quests too.

If the file is not the highest-numbered one, do not leave gaps — the loader scans 1 through `QUEST_MAX_STATIC_QUESTS` and skips missing files, so gaps in the middle produce IDs that shift relative to file numbers. Renumber downstream files if the gap would cause confusion.

---

## 4. Lore

### 4.1 What lore is

Lore entries are dense, AI-optimized reference text served to NPCs (and players via the `lore` command) about world topics: places, events, factions, figures. Each topic has a default entry plus city-specific and city+race-specific entries.

### 4.2 Adding a new lore topic

All lore lives in the database. The flat files in `lore/` are legacy reference material.

**Step 1 — Insert the topic:**

```sql
INSERT INTO lore_topics (filename, keywords)
VALUES ('stonekeep', 'stonekeep ruins keep fortress ancient');
```

**Step 2 — Insert the default entry (required, seq=1, flags=0):**

```sql
INSERT INTO lore_entries (topic_id, seq, flags, body)
VALUES (
    (SELECT id FROM lore_topics WHERE filename = 'stonekeep'),
    1, 0,
    'Stonekeep was a fortress-city abandoned after the Warden Collapse...'
    -- 10-15 lines of dense factual content
);
```

**Step 3 — Insert city entries (required — one per city, flags 1/2/4/8/16):**

```sql
-- MIDGAARD=1, KIESS=2, KOWLOON=4, RAKUEN=8, MAFDET=16
INSERT INTO lore_entries (topic_id, seq, flags, body)
VALUES
    ((SELECT id FROM lore_topics WHERE filename = 'stonekeep'), 2, 1,  'Midgaard perspective...'),
    ((SELECT id FROM lore_topics WHERE filename = 'stonekeep'), 3, 2,  'Kiess perspective...'),
    ((SELECT id FROM lore_topics WHERE filename = 'stonekeep'), 4, 4,  'Kowloon perspective...'),
    ((SELECT id FROM lore_topics WHERE filename = 'stonekeep'), 5, 8,  'Rakuen perspective...'),
    ((SELECT id FROM lore_topics WHERE filename = 'stonekeep'), 6, 16, 'Mafdet perspective...');
```

**Step 4 — Insert city+race entries (only where the race perspective adds something distinct):**

Race flags: HUMAN=32, KHENARI=64, KHEPHARI=128, ASHBORN=256, UMBRAL=512, RIVENNID=1024, DELTARI=2048, USHABTI=4096, SERATHI=8192, KETHARI=16384.

Combine city + race with bitwise OR. MIDGAARD + KHEPHARI = 1 | 128 = 129.

```sql
INSERT INTO lore_entries (topic_id, seq, flags, body)
VALUES ((SELECT id FROM lore_topics WHERE filename = 'stonekeep'), 7, 129,
        'What a Khephari in Midgaard knows: the geology of the collapse...');
```

**Step 5 — Validate:**

```sql
SELECT seq, flags, LEFT(body, 60)
FROM lore_entries
WHERE topic_id = (SELECT id FROM lore_topics WHERE filename = 'stonekeep')
ORDER BY seq;
-- Must have at least 6 rows: 1 default + 5 city entries
```

### 4.3 Rules for lore content

- No `@@` color codes. Tests enforce this.
- 10–15 lines per entry. Dense factual content; no prose essays.
- Maximum 56 entries per topic (1 default + 5 city + up to 50 city+race).
- City+race entries only when the race's cognitive posture produces a distinct perspective. See `docs/perspective.md`.
- Changes take effect on next server boot (lore is loaded at startup).

### 4.4 Setting NPC lore flags (so NPCs use context-specific lore)

```
set mob <vnum> lore MIDGAARD     -- toggle MIDGAARD flag on/off
set mob <vnum> lore KHEPHARI     -- toggle race flag
set mob <vnum> lore clear        -- remove all flags
```

These flags are saved in the area file with the `^` marker.

---

## 5. Help

### 5.1 What help entries are

Help entries are player-accessible in-game documentation reached with the `help <keyword>` command. They cover commands, game mechanics, world information, and the login greeting.

### 5.2 Adding a new help entry

```sql
INSERT INTO help_entries (filename, level, keywords, body)
VALUES (
    'stonekeep',           -- unique key; lowercase, no extension
    0,                     -- 0 = all players; -1 = immortal/staff only
    'stonekeep ruins keep',-- space-separated keyword list; first is primary
    'Stonekeep is an abandoned fortress north of Midgaard...\n\nSee also: HISTORY, RUINS'
);
```

### 5.3 Editing an existing help entry

```sql
UPDATE help_entries SET body = 'Revised text...' WHERE filename = 'stonekeep';
UPDATE help_entries SET keywords = 'stonekeep ruins keep fortress' WHERE filename = 'stonekeep';
```

### 5.4 Removing a help entry

```sql
DELETE FROM help_entries WHERE filename = 'stonekeep';
```

### 5.5 Key rules

- Changes take effect on **next server boot** (help is loaded at startup, no hot-reload).
- Every new player command and every new spell or skill MUST have a corresponding help entry. Removing a spell/skill/command means removing its help entry too.
- Trust level: `0` for all players; `-1` for immortal-only.
- The login greeting lives in `greeting1`–`greeting6` entries in `help_entries`.

---

## 6. Shelp

### 6.1 What shelp entries are

Shelp (spell/skill help) entries are accessed with the `shelp <keyword>` command. They cover spell and skill details in staff-formatted reference sheets. By convention these are restricted to staff (`level = -1`).

### 6.2 Adding a new shelp entry

```sql
INSERT INTO shelp_entries (filename, level, keywords, body)
VALUES (
    'shelp_stoneshield',         -- filename must start with shelp_
    -1,                          -- staff-only by convention
    'stoneshield stone shield',
    '*****...\n   Name: Stone Shield\n   Type: Defensive\n...'
);
```

### 6.3 Editing and removing shelp entries

```sql
UPDATE shelp_entries SET body = '...' WHERE filename = 'shelp_stoneshield';
DELETE FROM shelp_entries WHERE filename = 'shelp_stoneshield';
```

### 6.4 Key rules

- Filename must start with `shelp_`.
- Changes take effect on **next server boot**.

---

## 7. Cross-References Between Content Types

Content types link to each other through vnums, keywords, and flags. This section maps those connections.

### 7.1 Quests → Areas (mob vnums)

Quest target vnums and offerer vnums must reference mobs that exist in the game world.

- Target vnums in the `.prop` file must match `vnum` values in `mobs.yaml` (for area submissions) or existing `mob_templates` rows in the DB.
- Offerer vnums must be postmaster NPCs present in the DB.

**To verify a quest's target vnum is valid:**

```sql
SELECT vnum, short_desc FROM mob_templates WHERE vnum IN (6130, 6133);
```

If a target vnum is wrong (mob doesn't exist or is the wrong mob), edit the target vnum line in the `.prop` file and reboot.

### 7.2 Areas → Quests (offerer NPC must exist)

When adding a new area that introduces a postmaster NPC, any quests that use that NPC's vnum as `offerer_vnum` will not work until the area is ingested and that mob is in the DB.

Ensure the area is live before enabling quest chains that reference its NPCs.

### 7.3 Areas → Lore (NPC lore flags)

Mobs in area files carry `lore_flags` that determine which lore perspective they deliver. Set flags in `mobs.yaml` (via the builder `set mob <vnum> lore <FLAG>`) and include them in the area export. When you add lore for a topic associated with a city-specific NPC, make sure that NPC's `lore_flags` include the appropriate city/race flags so the right entry is served.

### 7.4 Help/Shelp → Areas and Quests (keyword references)

Help entries frequently reference game locations and quest series by keyword. When adding a new area, create or update help entries for:
- The area itself (general overview, how to get there, level range).
- Any new commands or mechanics introduced by the area's specials.

When adding quests, consider whether an existing help entry for the relevant quest line (if one exists) needs updating.

### 7.5 Shelp → Spells/Skills (code-driven)

Shelp entries document spells and skills defined in C code. They do not reference areas directly. When a new spell or skill is added to the codebase:
1. A shelp entry MUST be added to `shelp_entries` with `filename = 'shelp_<name>'`.
2. A player-facing help entry MUST also be added to `help_entries` for the spell/skill.

### 7.6 Lore → Lore source docs

Lore DB entries are written from source documents in `docs/lore/`. When adding lore for a new area or topic:
1. Create `docs/lore/<topic>_lore.md` with full worldbuilding detail (not loaded by server — reference only).
2. From that source document, write the lore entries into the DB.

---

## 8. Example Workflow: Fixing Wrong Quest Target Vnums

**Scenario:** Quest 81 (`82.prop`, "Warlord's blood oath, cancelled") has target vnum `6130` but the correct mob is `6133`.

**Step 1 — Identify and read the quest file:**

The static quest ID is 81 → file is `quests/82.prop`. Read it.

**Step 2 — Verify the correct mob:**

```sql
SELECT vnum, short_desc FROM mob_templates WHERE vnum IN (6130, 6133);
```

Confirm 6133 exists and is the intended target.

**Step 3 — Edit `quests/82.prop`.** The third line (after title and definition line) is the space-delimited vnum list. Change `6130` to `6133`.

**Step 4 — This is a bugfix — no proposal required.** Commit directly on a feature branch, push, and open a PR.

**Step 5 — The fix takes effect on next server boot.** Quest templates are loaded at startup only.

---

## 9. After Any Content Change

| Content type | Hot-reload? | What to do |
|---|---|---|
| Area | Yes — via `areaingest run <keyword>` or next PULSE_AREA tick | Drop YAML directory in `area/incoming/` |
| Quest | No | Edit `.prop` file; reboot server |
| Lore | No | SQL insert/update; reboot server |
| Help | No | SQL insert/update; reboot server |
| Shelp | No | SQL insert/update; reboot server |

Always run `cd src && make unit-tests` before pushing any change. For area changes, also run the integration test suite to confirm the ingestion path works.

---

## 10. Affected Files Summary

| Action | Files/Tables Touched |
|---|---|
| New area | `area/incoming/<keyword>/` YAML dir → DB tables (rooms, exits, mob_templates, obj_templates, shops, resets, specials, objfuns) |
| New quest | `quests/<n>.prop`; possibly `src/config.h` if `QUEST_MAX_STATIC_QUESTS` needs bumping |
| New lore | `lore_topics`, `lore_entries` tables; optionally `docs/lore/<topic>_lore.md` |
| New help | `help_entries` table |
| New shelp | `shelp_entries` table |
| Cross-link: area ↔ quest | `mobs.yaml` vnum must exist before quest offerer/target works |
| Cross-link: area ↔ lore | `mobs.yaml` `lore_flags` field; corresponding `lore_entries` rows |
| Cross-link: spell/skill ↔ help/shelp | `help_entries` + `shelp_entries` rows required for every spell/skill add/remove |
