# Proposal: YAML-Based Game Content Management

**Status:** Draft

---

## 1. Problem

The database is now the authoritative store for all game content (areas, rooms, mobs, objects,
quests, shops, resets, and associated sub-records). Flat `.are` and `.prop` files have been
removed. This leaves a gap in the content workflow:

- **No human-readable audit format.** Reviewing what a mob or object currently looks like
  requires a SQL query. There is no way to diff content changes between edits.
- **No structured way to author new content outside OLC.** Adding new mobs, objects, rooms,
  or quests must be done either through in-game OLC or hand-written SQL. Neither is convenient
  for batch authoring or offline work.
- **No lightweight patch workflow.** Updating a field on 50 mobs requires writing explicit SQL
  UPDATE statements. There is no structured format for expressing "change these fields on these
  records."
- **No version-controllable content source.** With flat files gone, content changes are only
  tracked as SQL transactions in the DB. YAML files in the repo restore git-diffable history for
  content.

This proposal defines a YAML-based content management system covering all DB-backed game data,
with three distinct categories of files and the Python tooling to apply them.

---

## 2. Scope

**Content types covered:**
- Areas (header metadata)
- Rooms (with exits and extra descriptions)
- Mobiles (with loot tables and AI prompts)
- Objects (with affects)
- Resets
- Shops
- Quests (`quest_templates`)
- Help entries (`help_entries`)
- Skill/spell help entries (`shelp_entries`)
- Lore entries (`lore_entries`)

**Three file categories:**
- **Exports** — DB → YAML snapshots (read-only output; no DB writes; ephemeral)
- **Imports** — YAML → DB for new records (INSERT; fails if record already exists; deleted after apply)
- **Updates** — YAML patches → DB for modifying existing records (UPDATE; fails if record missing; deleted after apply)

**Out of scope for this proposal:**
- Player files (runtime state; not content)
- Runtime data (bans, brands, rulers, socials, sysdata — operational, not authored)

---

## 3. Directory Structure

All YAML files live under `acktng/yaml/`. All three subdirectories are **gitignored** — these
files are ephemeral working surfaces, not version-controlled artifacts.

The top-level structure is **content-type first, then operation**. Mobs, objects, rooms,
resets, and shops all belong to areas and live exclusively within the `areas/` tree. Only
content types with no area association (quests, help, shelp, lore) get their own top-level
directory.

```
acktng/yaml/
  areas/
    export/
      midgaard/        # One folder per area; each section is its own file.
        area.yaml
        rooms.yaml
        mobs.yaml
        objects.yaml
        resets.yaml
        shops.yaml
    import/
      whispering_forest_preserve/   # Same per-section layout for new areas.
        area.yaml
        rooms.yaml
        mobs.yaml
        objects.yaml
        resets.yaml
        shops.yaml
    update/
      midgaard/        # Only section files being patched need to be present.
        area.yaml      # Patches to area header fields
        mobs.yaml      # List of { vnum, ...changed fields }
        objects.yaml
        rooms.yaml
  quests/
    export/
    import/
    update/
  help/
    export/
    import/
    update/
  shelp/
    export/
    import/
    update/
  lore/
    export/
    import/
    update/
```

Files in `exports/` are **never edited by hand** and are never read by `yaml_apply.py`.

Files in `imports/` and `updates/` are hand-authored or tool-generated. After `yaml_apply.py`
successfully processes a file, it **deletes the file** automatically. This keeps the working
directories clean and prevents double-application.

---

## 4. YAML Schema

### 4.1 Area export (folder per area, one file per section)

Each area export is a folder named after the area keyword. Each section within the area is its
own YAML file. All files in the folder are always written by an area export; only the files
relevant to a given import or update need to be present.

**`yaml/areas/export/midgaard/area.yaml`**
```yaml
name: "Midgaard City"
min_vnum: 3000
max_vnum: 3999
keyword: midgaard
level_label: "10-20"
level_min: 10
level_max: 20
reset_rate: 15
music: midgaard.mp3
flags: 0
```

**`yaml/areas/export/midgaard/rooms.yaml`**
```yaml
- vnum: 3000
  name: "The Temple of Midgaard"
  description: |
    You are in the southern end of the temple hall in the city of Midgaard.
  room_flags: 0
  sector_type: 1
  exits:
    north:
      dest_vnum: 3001
      exit_flags: 0
      key_vnum: null
      keyword: null
      description: null
    south:
      dest_vnum: 3048
      exit_flags: 0
  extra_descs:
    - keyword: altar
      description: |
        The altar is made of white marble.
```

**`yaml/areas/export/midgaard/mobs.yaml`**
```yaml
- vnum: 3005
  name: "the cityguard"
  short_descr: "A cityguard"
  long_descr: "A cityguard is here, doing his duty.\n"
  description: "A burly guard stands at attention."
  act_flags: 1
  affected_by: 0
  alignment: 0
  level: 15
  sex: 1
  hp_mod: 0
  ac_mod: 0
  hr_mod: 5
  dr_mod: 5
  class: 0
  race: 0
  loot:
    - vnum: 3010
      chance: 50
    - vnum: 3011
      chance: 25
```

**`yaml/areas/export/midgaard/objects.yaml`**
```yaml
- vnum: 3010
  name: "a long sword"
  short_descr: "a long sword"
  description: "A long sword lies here."
  item_type: 5
  extra_flags: 0
  wear_flags: 16384
  item_apply: 0
  value_0: 1
  value_1: 10
  value_2: 4
  value_3: 0
  weight: 10
  cost: 100
  affects:
    - type: 1
      modifier: 3
```

**`yaml/areas/export/midgaard/resets.yaml`**
```yaml
- type: M
  mob_vnum: 3005
  room_vnum: 3001
  mob_max: 5
- type: O
  obj_vnum: 3010
  room_vnum: 3001
  obj_max: 2
- type: E
  obj_vnum: 3010
  wear_loc: 14
```

**`yaml/areas/export/midgaard/shops.yaml`**
```yaml
- keeper_vnum: 3005
  buy_type_0: 5
  buy_type_1: 9
  buy_type_2: 0
  buy_type_3: 0
  buy_type_4: 0
  buy_profit: 110
  sell_profit: 90
  open_hour: 6
  close_hour: 20
```

Area imports use the same per-file schema under `yaml/imports/areas/<keyword>/`. All six files
should be present for a new area import (omitting a section file means that section is empty).

Area updates follow the same folder structure under `yaml/updates/areas/<keyword>/`. Only the
section files containing records to patch need to be present. Each file is a list of records
where only the key field (vnum or keeper_vnum) and the fields to change are specified:

**`yaml/areas/update/midgaard/mobs.yaml`** (patch subset of mobs)
```yaml
- vnum: 3005
  level: 20
  hp_mod: 10
- vnum: 3006
  dr_mod: 8
```

**`yaml/areas/update/midgaard/area.yaml`** (patch the area header)
```yaml
reset_rate: 20
music: midgaard_v2.mp3
```

### 4.2 Quest import

```yaml
# yaml/quests/import/200.yaml
id: 200
title: "Slay the Orc Chief"
type: 0
num_targets: 1
target_vnums: [3050]
kill_needed: 1
min_level: 10
max_level: 50
offerer_vnum: 3005
reward_gold: 500
reward_qp: 10
reward_exp: 1000
accept_message: "Slay the orc chief to the north and return to me!"
completion_message: "Well done, adventurer. The city is safer for it."
reward_obj_short: ""
reward_obj_name: ""
reward_obj_long: ""
reward_obj_wear_flags: 0
reward_obj_extra_flags: 0
reward_obj_weight: 0
reward_obj_item_apply: 0
```

### 4.3 Help entry import

```yaml
# yaml/help/import/berserk.yaml
keyword: "BERSERK"
level: 0          # 0 = player-accessible; higher = restricted to that level+
body: |
  Syntax: berserk

  BERSERK is a warrior skill that sends you into a blind rage, increasing
  your damage output at the cost of defense. While berserk, you cannot flee.

  Duration: 3-6 ticks depending on level.
  Cooldown: 30 seconds after the effect ends.
```

### 4.4 Skill/spell help (shelp) import

`shelp` entries document skills and spells. They are separate from `help` entries, which cover
general player commands and game concepts.

```yaml
# yaml/shelp/import/berserk.yaml
keyword: "BERSERK"
level: 0
body: |
  Syntax: berserk

  BERSERK is a warrior skill. See also: HELP BERSERK for general usage.
  This shelp entry covers mechanical details used in skill lookups.
```

### 4.5 Lore entry import

```yaml
# yaml/lore/import/midgaard-history.yaml
keyword: "MIDGAARD-HISTORY"
body: |
  Midgaard is the oldest city in the known world, founded in the Second Age
  by the paladin-king Aldric the Just. Its great temple has stood since the
  city's founding and remains a center of worship for the gods of light.
```

### 4.6 Update patches

Update files specify only the fields to change. The vnum (or id for quests) identifies the
record; all other present keys are applied as SET clauses. Missing keys are not touched.

```yaml
# yaml/mobs/update/3005.yaml
vnum: 3005
level: 20
hp_mod: 10
dr_mod: 8
```

```yaml
# yaml/objects/update/3010.yaml
vnum: 3010
cost: 150
```

```yaml
# yaml/quests/update/42.yaml
id: 42
reward_gold: 750
max_level: 60
```

Area header updates (not rooms/mobs/objects):
```yaml
# yaml/updates/areas/midgaard.yaml
keyword: midgaard
reset_rate: 20
music: midgaard_v2.mp3
```

Help/shelp/lore updates are keyed by keyword:

```yaml
# yaml/help/update/berserk.yaml
keyword: "BERSERK"
body: |
  Updated description with new cooldown values.
```

```yaml
# yaml/lore/update/midgaard-history.yaml
keyword: "MIDGAARD-HISTORY"
body: |
  Revised lore text.
```

---

## 5. Tooling

Two Python scripts in `acktng/tools/`:

### 5.1 `tools/yaml_export.py`

Reads from the DB and writes YAML to `yaml/exports/`. Never modifies the DB.

```
# Export a full area (all rooms, mobs, objects, resets, shops)
python3 tools/yaml_export.py --area midgaard
python3 tools/yaml_export.py --area-vnum 3000
python3 tools/yaml_export.py --all-areas          # exports every area

# Export individual records
python3 tools/yaml_export.py --mob 3005
python3 tools/yaml_export.py --object 3010
python3 tools/yaml_export.py --quest 42
python3 tools/yaml_export.py --all-quests

# Export help/shelp/lore
python3 tools/yaml_export.py --help BERSERK
python3 tools/yaml_export.py --all-help
python3 tools/yaml_export.py --shelp BERSERK    # skill/spell help entries
python3 tools/yaml_export.py --all-shelp
python3 tools/yaml_export.py --lore MIDGAARD-HISTORY
python3 tools/yaml_export.py --all-lore
```

Output paths follow the directory structure in §3. An `--area midgaard` export creates
`yaml/areas/export/midgaard/` with `area.yaml`, `rooms.yaml`, `mobs.yaml`, `objects.yaml`,
`resets.yaml`, and `shops.yaml`. A `--mob 3005` export writes `yaml/mobs/export/3005.yaml`.
A `--help BERSERK` export writes `yaml/help/export/berserk.yaml`.

### 5.2 `tools/yaml_apply.py`

Reads a YAML file and applies it to the DB. Infers operation from the subdirectory:
- Files under `imports/` → INSERT (error if record already exists)
- Files under `updates/` → UPDATE (error if record not found)

After successful application, the input file is **deleted**.

```
# Area operations: pass the folder; tool reads all section files present
python3 tools/yaml_apply.py yaml/areas/import/whispering_forest_preserve/
python3 tools/yaml_apply.py yaml/areas/update/midgaard/

# Standalone record operations (quests, help, shelp, lore)
python3 tools/yaml_apply.py yaml/quests/import/200.yaml
python3 tools/yaml_apply.py yaml/help/import/berserk.yaml
python3 tools/yaml_apply.py yaml/shelp/import/ban.yaml
python3 tools/yaml_apply.py yaml/lore/import/midgaard-history.yaml
python3 tools/yaml_apply.py yaml/quests/update/42.yaml
python3 tools/yaml_apply.py yaml/help/update/berserk.yaml
```

A glob form is also supported for batch application:

```
python3 tools/yaml_apply.py yaml/quests/import/     # apply all files in a dir, delete each on success
python3 tools/yaml_apply.py yaml/areas/update/      # apply all area update folders
```

For area imports, `yaml_apply.py` reads the folder, inserts the area record first (obtaining
`area_id`), then inserts rooms, mobs, objects, resets, and shops in dependency order. After
successful application of an area folder, all files within it and the folder itself are deleted.

Both tools read the DB connection string from `data/db.conf` (same file the C server uses).

### 5.3 Dependencies

A new `tools/requirements.txt` in `acktng/`:

```
psycopg2-binary
pyyaml
```

These are dev/tooling dependencies only — not linked into the server binary.

---

## 6. Affected Files

- `acktng/tools/yaml_export.py` — new
- `acktng/tools/yaml_apply.py` — new
- `acktng/tools/requirements.txt` — new (`psycopg2-binary`, `pyyaml`)
- `acktng/yaml/` — new directory tree; gitignored via `acktng/.gitignore`
- `acktng/.gitignore` — add `yaml/` entry
- `aicli/setup.sh` — add `pip install -r acktng/tools/requirements.txt` step

No C code changes. No schema changes.

---

## 7. Trade-offs

**All YAML is ephemeral and gitignored:**
The `yaml/` tree is never committed. Exports are transient working copies for inspection or
use as templates. Imports and updates are consumed and deleted on successful apply. There is no
attempt to maintain a version-controlled YAML representation of the DB — the DB itself is the
source of truth.

**Imports and updates are deleted on success, not on failure:**
If `yaml_apply.py` encounters a DB error, the file is left in place so the author can inspect
and retry. Only a clean, committed transaction triggers deletion.

**Mobs and objects live inside areas, not at the top level:**
Mobs, objects, rooms, resets, and shops always belong to an area and are always accessed
through the `yaml/areas/` tree. There are no standalone `mobs/` or `objects/` directories.
Adding or patching content within an existing area uses an area update folder with only the
relevant section file present.

**Full export vs. partial:**
Exports always dump the full record so the YAML is self-contained. Updates are intentionally
partial (only changed fields) to avoid accidentally overwriting untouched fields.

**`area_id` vs. `area_keyword` in standalone imports:**
Both are accepted. `area_keyword` is more human-friendly; `area_id` is unambiguous if keywords
ever clash. The tool resolves `area_keyword` to `area_id` via a lookup before inserting.

**Python vs. C tooling:**
Python with `psycopg2` is significantly faster to write and iterate on for a content management
tool. The C tools (`import_to_db.c`, `db_to_files.c`) were migration utilities now scheduled
for removal. The new tools are content-authoring utilities, not build-time dependencies.

**No server-side hot-apply:**
This proposal covers offline authoring only — edit YAML, run `yaml_apply.py`, then reboot or
use the in-game OLC reload path to pick up changes. A live-reload path is out of scope.
