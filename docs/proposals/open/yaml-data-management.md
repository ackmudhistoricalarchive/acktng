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
- Staff help entries (`shelp_entries`)
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

```
acktng/yaml/
  exports/
    areas/           # One file per area; contains header, rooms, mobs, objects, resets, shops.
    mobs/            # Per-type subdirs for standalone single-record exports.
    objects/
    quests/
    help/            # Help entries
    shelp/           # Staff help entries
    lore/            # Lore entries
  imports/
    areas/           # New areas with all their contents
    mobs/            # Standalone new mobs (adding to an existing area)
    objects/         # Standalone new objects
    quests/          # New quest templates
    help/            # New help entries
    shelp/           # New staff help entries
    lore/            # New lore entries
  updates/
    areas/           # Patches to existing area header fields
    mobs/            # Patches to existing mob fields
    objects/         # Patches to existing object fields
    quests/          # Patches to existing quest fields
    help/            # Patches to existing help entries
    shelp/           # Patches to existing shelp entries
    lore/            # Patches to existing lore entries
```

Files in `exports/` are **never edited by hand** and are never read by `yaml_apply.py`.

Files in `imports/` and `updates/` are hand-authored or tool-generated. After `yaml_apply.py`
successfully processes a file, it **deletes the file** automatically. This keeps the working
directories clean and prevents double-application.

---

## 4. YAML Schema

### 4.1 Area-centric export (one file per area)

Exports for areas use a compound format that bundles all related records:

```yaml
# yaml/exports/areas/midgaard.yaml
area:
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

rooms:
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

mobiles:
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

objects:
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

resets:
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

shops:
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

Area-centric imports use the same schema. Area-centric updates are not supported — use the
per-type update files instead.

### 4.2 Standalone mob import

```yaml
# yaml/imports/mobs/3099.yaml
vnum: 3099
area_id: 1            # FK to areas.id; or use area_keyword: midgaard
name: "the veteran guard"
short_descr: "A veteran guard"
long_descr: "A veteran guard stands here, eyeing you carefully.\n"
description: "Scars mark this guard as a seasoned fighter."
act_flags: 1
alignment: 0
level: 20
sex: 1
hp_mod: 10
hr_mod: 6
dr_mod: 6
class: 2
race: 0
loot:
  - vnum: 3010
    chance: 40
```

### 4.3 Standalone object import

```yaml
# yaml/imports/objects/3099.yaml
vnum: 3099
area_id: 1            # or area_keyword: midgaard
name: "a steel shield"
short_descr: "a steel shield"
description: "A battered steel shield lies here."
item_type: 9
extra_flags: 0
wear_flags: 4096
item_apply: 0
value_0: 0
value_1: 0
value_2: 0
value_3: 0
weight: 15
cost: 200
affects:
  - type: 3
    modifier: 2
```

### 4.4 Quest import

```yaml
# yaml/imports/quests/200.yaml
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

### 4.5 Help entry import

```yaml
# yaml/imports/help/berserk.yaml
keyword: "BERSERK"
level: 0          # 0 = player-accessible; higher = restricted to that level+
body: |
  Syntax: berserk

  BERSERK is a warrior skill that sends you into a blind rage, increasing
  your damage output at the cost of defense. While berserk, you cannot flee.

  Duration: 3-6 ticks depending on level.
  Cooldown: 30 seconds after the effect ends.
```

### 4.6 Staff help (shelp) import

```yaml
# yaml/imports/shelp/ban.yaml
keyword: "BAN"
level: 56         # Minimum staff level to see this entry
body: |
  Syntax: ban <name|site> <type>

  BAN adds a ban entry. Types: name, site, newsite, permit.
  See also: UNBAN, BANS
```

### 4.7 Lore entry import

```yaml
# yaml/imports/lore/midgaard-history.yaml
keyword: "MIDGAARD-HISTORY"
body: |
  Midgaard is the oldest city in the known world, founded in the Second Age
  by the paladin-king Aldric the Just. Its great temple has stood since the
  city's founding and remains a center of worship for the gods of light.
```

### 4.8 Update patches

Update files specify only the fields to change. The vnum (or id for quests) identifies the
record; all other present keys are applied as SET clauses. Missing keys are not touched.

```yaml
# yaml/updates/mobs/3005.yaml
vnum: 3005
level: 20
hp_mod: 10
dr_mod: 8
```

```yaml
# yaml/updates/objects/3010.yaml
vnum: 3010
cost: 150
```

```yaml
# yaml/updates/quests/42.yaml
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
# yaml/updates/help/berserk.yaml
keyword: "BERSERK"
body: |
  Updated description with new cooldown values.
```

```yaml
# yaml/updates/lore/midgaard-history.yaml
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
python3 tools/yaml_export.py --shelp BAN
python3 tools/yaml_export.py --all-shelp
python3 tools/yaml_export.py --lore MIDGAARD-HISTORY
python3 tools/yaml_export.py --all-lore
```

Output paths follow the directory structure in §3. An `--area midgaard` export writes
`yaml/exports/areas/midgaard.yaml`. A `--mob 3005` export writes `yaml/exports/mobs/3005.yaml`.
A `--help BERSERK` export writes `yaml/exports/help/berserk.yaml`.

### 5.2 `tools/yaml_apply.py`

Reads a YAML file and applies it to the DB. Infers operation from the subdirectory:
- Files under `imports/` → INSERT (error if record already exists)
- Files under `updates/` → UPDATE (error if record not found)

After successful application, the input file is **deleted**.

```
python3 tools/yaml_apply.py yaml/imports/areas/new_area.yaml
python3 tools/yaml_apply.py yaml/imports/mobs/3099.yaml
python3 tools/yaml_apply.py yaml/imports/quests/200.yaml
python3 tools/yaml_apply.py yaml/imports/help/berserk.yaml
python3 tools/yaml_apply.py yaml/imports/shelp/ban.yaml
python3 tools/yaml_apply.py yaml/imports/lore/midgaard-history.yaml
python3 tools/yaml_apply.py yaml/updates/mobs/3005.yaml
python3 tools/yaml_apply.py yaml/updates/quests/42.yaml
python3 tools/yaml_apply.py yaml/updates/help/berserk.yaml
```

A glob form is also supported for batch application:

```
python3 tools/yaml_apply.py yaml/imports/mobs/        # apply all files in a dir, delete each on success
python3 tools/yaml_apply.py yaml/updates/objects/
```

For area imports, `yaml_apply.py` inserts the area record first (obtaining `area_id`), then
inserts rooms, mobs, objects, resets, and shops in dependency order. For standalone mob/object
imports, `area_id` must be provided directly or resolved via `area_keyword`.

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

**Area-centric vs. flat per-type exports:**
Area exports bundle everything about an area into one file, matching how designers think about
content ("the midgaard area"). Flat per-type exports (`yaml/exports/mobs/3005.yaml`) are also
supported for single-record inspection. Both modes are available from `yaml_export.py`.

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
