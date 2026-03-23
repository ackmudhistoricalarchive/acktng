# Proposal: Move Skill/Spell Definitions and Logic to Database via Effect Composition

**Status:** Open
**Date:** 2026-03-23
**Repos affected:** acktng, tngdb

---

## Problem

All skill and spell metadata and logic is compiled directly into the game binary:

- `src/spells/spell_table_data.c` (~3087 lines) — metadata for ~200+ spells
- `src/skills/skill_table_data.c` (~3358 lines) — metadata for ~300+ skills
- `src/spells/spell_*.c` — individual C function per spell
- `src/skills/do_*.c` — 109 individual C files, one per skill

Adding, removing, or tuning any skill or spell requires editing C source and recompiling the game. The web frontend and tngdb have no access to skill/spell data at all.

---

## Goal

Move both skill/spell **metadata** (name, per-class levels, mana cost, etc.) and **logic** (effects) from C into PostgreSQL. Skills and spells are expressed as **compositions of data-driven effect primitives**. The C engine becomes a generic interpreter that executes these effect descriptions at runtime.

Spell and skill logic that cannot be expressed as compositions (a small set, ~8 spells) retains a registered C function as a fallback, invoked by name.

---

## Effect Taxonomy

A survey of all spell/skill implementations reveals the following composable effect categories:

| Effect type | What it does | Examples |
|---|---|---|
| `DAMAGE` | Instant damage: element, formula, save | fireball, magic missile, holy wrath |
| `DAMAGE_AOE` | Room/area damage: element, formula, flags | chain lightning, wall of fire, earthquake |
| `HEAL` | Instant heal: resource (hp/mana/move), formula, target | cure light, refresh, group heal |
| `HOT` | Heal-over-time affect | regen |
| `DOT` | Damage-over-time affect | poison, black hand |
| `APPLY_AFFECT` | Apply APPLY_* and/or AFF_* affect to character | armor, bless, sanctuary, haste, berserk |
| `REMOVE_AFFECT` | Strip a specific affect type from target | cure blindness, cure poison, remove curse |
| `DRAIN` | Reduce a resource (mana/move/xp), optionally heal caster | energy drain |
| `ROOM_AFFECT` | Apply ROOM_BV_* affect to current room | rune fire, seal room, mana drain, cage |
| `CREATE_OBJECT` | Spawn an object by vnum | create food, beacon, spring |
| `SUMMON_CREATURE` | Summon a pet from a template or vnum | skeleton, diamond golem, gate |
| `ENCHANT_OBJECT` | Add permanent affects to a held/worn object | enchant weapon, poison weapon |
| `TRANSPORT` | Move a character to a destination | teleport, summon, word of recall |
| `WAR_ATTACK` | Standard melee skill attack via `war_attack()` | kick, bash, punch, holystrike |
| `CUSTOM` | Named C function for complex/unique logic | dispel magic, portal, animate, charm |

The vast majority of spells/skills (estimated 90%+) decompose cleanly into one or a small sequence of these types. The `CUSTOM` type handles the remainder without special-casing.

### Spells requiring `CUSTOM`

These ~8 spells have procedural logic that cannot be expressed as effect composition without losing fidelity:

- `spell_dispel_magic` — iterative per-affect stripping with probability decay, cloak interaction, selective preserve
- `spell_portal` — bidirectional portal creation consuming a beacon object via world search
- `spell_animate` — animates a NPC corpse: transfers items, equips them, sets AI flags, adds follower
- `spell_stalker` — spawns a mob scaled to target's stats with hunt AI; backfire path
- `spell_energy_drain` — multi-resource drain (mana+move+xp) + self-heal + alignment shift + damage
- `spell_charm_person` — follower management, extract timer, stop/add follower
- `spell_identify` — pure information display with item-type switching
- `spell_cage` — room affect with combo system interaction

---

## Database Schema

### `skills` table

Holds metadata (unchanged from original proposal):

```sql
CREATE TABLE skills (
    sn            SMALLINT    PRIMARY KEY,        -- stable index, matches current array position
    name          TEXT        NOT NULL UNIQUE,
    flag2         SMALLINT    NOT NULL DEFAULT 1, -- 1=NORM
    target        SMALLINT    NOT NULL DEFAULT 0,
    min_position  SMALLINT    NOT NULL DEFAULT 0,
    gsn_name      TEXT        NOT NULL DEFAULT '', -- C gsn variable name; '' if none
    slot          SMALLINT    NOT NULL DEFAULT 0,
    min_mana      SMALLINT    NOT NULL DEFAULT 0,
    beats         SMALLINT    NOT NULL DEFAULT 0,
    can_learn     BOOLEAN     NOT NULL DEFAULT TRUE,
    noun_damage   TEXT        NOT NULL DEFAULT '',
    msg_off       TEXT        NOT NULL DEFAULT '',
    room_off      TEXT        NOT NULL DEFAULT '',
    growth        SMALLINT    NOT NULL DEFAULT 0,
    class_levels  JSONB       NOT NULL DEFAULT '{}' -- {"MAG": 5, "CLE": 10, ...}; absent = NO_USE
);
```

### `skill_effects` table

Each row is one effect in a skill/spell's composition. Multiple rows per skill, ordered by `seq`.

```sql
CREATE TABLE skill_effects (
    id            SERIAL      PRIMARY KEY,
    sn            SMALLINT    NOT NULL REFERENCES skills(sn) ON DELETE CASCADE,
    seq           SMALLINT    NOT NULL DEFAULT 0,  -- execution order within the skill
    effect_type   TEXT        NOT NULL,            -- see effect types below
    params        JSONB       NOT NULL DEFAULT '{}' -- effect-type-specific parameters
);

CREATE INDEX idx_skill_effects_sn ON skill_effects(sn, seq);
```

### Effect type parameter schemas

Each `effect_type` defines which keys are expected in `params`:

**`DAMAGE`**
```json
{
  "element":  "FIRE",           // ELE_* constant name
  "formula":  "level_table",    // "level_table" | "NdM" | "NdM+level*X"
  "save":     "halve",          // "halve" | "none" | "negate"
  "flags":    ["NO_REFLECT"]    // optional element modifier flags
}
```

**`DAMAGE_AOE`**
```json
{
  "element":  "LIGHTNING",
  "formula":  "level_table",
  "flags":    ["AOE_SAVES", "AOE_SKIP_GROUP"],
  "decay":    0.20              // optional damage decay per target (chain lightning)
}
```

**`HEAL`**
```json
{
  "resource": "hp",             // "hp" | "mana" | "move"
  "formula":  "5d8+level",
  "cap":      50,               // optional cap
  "target":   "victim",         // "victim" | "self" | "group"
  "side_effects": ["cure_blind", "cure_poison"]  // optional
}
```

**`HOT`**
```json
{
  "formula":  "class_heal",     // "class_heal" | "Nd M"
  "duration": "level/4",
  "duration_type": "ROUND"      // "HOUR" | "ROUND"
}
```

**`DOT`**
```json
{
  "element":  "POISON",
  "modifier": "level/5",
  "duration": "level/3",
  "duration_type": "ROUND",
  "aff_flag": "AFF_POISON",     // optional AFF_* to apply alongside DOT
  "stack":    true              // use affect_join (stacking) vs affect_to_char
}
```

**`APPLY_AFFECT`**
```json
{
  "affects": [
    {"location": "AC",       "modifier": -20},
    {"location": "HITROLL",  "modifier": 2},
    {"aff_flag": "AFF_SANCTUARY"}
  ],
  "duration":      "level/4",
  "duration_type": "HOUR",     // "HOUR" | "ROUND"
  "save":          "negate"    // optional — return on save
}
```

**`REMOVE_AFFECT`**
```json
{
  "aff_flag": "AFF_BLIND"       // or "location": "APPLY_CURSE" — what to strip
}
```

**`DRAIN`**
```json
{
  "resources": ["mana", "move", "xp"],
  "amount":    "fraction_quarter",  // "fraction_quarter" | formula
  "heal_caster": true,
  "alignment_shift": -200
}
```

**`ROOM_AFFECT`**
```json
{
  "room_bv":  "ROOM_BV_FIRE_RUNE",
  "modifier": "level/5",
  "duration": "level/4"
}
```

**`CREATE_OBJECT`**
```json
{
  "vnum":   12345,
  "values": [{"index": 0, "formula": "level/2"}],
  "location": "room"             // "room" | "inventory"
}
```

**`SUMMON_CREATURE`**
```json
{
  "template": "SKELETON",        // player_summon() template enum name
  "level_formula": "class_level" // how to scale the summon
}
```

**`ENCHANT_OBJECT`**
```json
{
  "affects": [
    {"location": "HITROLL", "modifier": "level/30+1"},
    {"location": "DAMROLL", "modifier": "level/30+1"}
  ],
  "alignment_flags": true,       // apply anti-good/anti-evil based on caster alignment
  "max_modifier_cap": true       // cap by object level
}
```

**`TRANSPORT`**
```json
{
  "dest_type": "random_teleport" // "random_teleport" | "to_caster" | "race_recall" | "direction"
}
```

**`WAR_ATTACK`**
```json
{
  "element_override": "HOLY",    // optional element added to base PHYSICAL
  "damage_multiplier": 1.5       // optional (fleche)
}
```

**`CUSTOM`**
```json
{
  "function": "spell_dispel_magic"  // C function name in the registry
}
```

---

## C Runtime Engine

### Effect dispatch (new file: `src/db/db_skills.c`)

The C engine replaces each `spell_*` and `do_*` function with a single generic dispatcher:

```c
void effect_execute(int sn, int level, CHAR_DATA *ch, void *vo, int target);
```

This is the new `SPELL_FUN` for all DB-driven spells. At cast time:

1. Look up the preloaded `skill_effects` list for `sn`.
2. Iterate effects in `seq` order.
3. Dispatch each to the appropriate handler: `effect_damage()`, `effect_apply_affect()`, etc.
4. For `CUSTOM`, call the function named in `params.function` via the C registry.

Effect handlers are thin focused functions, replacing the entire current `spell_*.c` file per spell.

### Registries (compile-time, in `db_skills.c`)

Two registries remain in C:

```c
// GSN pointer registry (unchanged from original proposal)
static const GsnEntry gsn_registry[] = {
    { "gsn_backstab", &gsn_backstab },
    ...
};

// Custom function registry (only for CUSTOM effect type)
static const CustomFunEntry custom_registry[] = {
    { "spell_dispel_magic",  spell_dispel_magic  },
    { "spell_portal",        spell_portal        },
    { "spell_animate",       spell_animate       },
    { "spell_stalker",       spell_stalker       },
    { "spell_energy_drain",  spell_energy_drain  },
    { "spell_charm_person",  spell_charm_person  },
    { "spell_identify",      spell_identify      },
    { "spell_cage",          spell_cage          },
    { NULL, NULL }
};
```

The `custom_registry` is small (~8 entries) and changes rarely.

### Boot loading

`db_load_skill_table()` in `boot_db()`:
1. `SELECT * FROM skills ORDER BY sn` — populates `skill_table[]` metadata.
2. `SELECT * FROM skill_effects ORDER BY sn, seq` — populates a parallel `skill_effects_table[MAX_SKILL]` linked list.
3. Resolves `gsn_name` → `sh_int *` via `gsn_registry`.
4. Any skill without effects that has no `CUSTOM` entry uses a no-op handler.

---

## Phasing

| Phase | What changes | Game binary change? |
|---|---|---|
| 1 | `skills` + `skill_effects` tables in DB; migration script populates them; tngdb `/skills` endpoint | No |
| 2 | Boot loads `skill_table[]` metadata from DB; deletes `spell_table_data.c` + `skill_table_data.c` | Yes |
| 3 | Generic `effect_execute()` dispatcher; effect handler functions in C; deletes `spell_*.c` and most `do_*.c` | Yes |
| 4 | The ~8 `CUSTOM` C functions are the only remaining per-spell C code | Yes |

Each phase ships independently. Phases 2 and 3 require no tngdb changes.

---

## Affected Files

### acktng

| File | Change |
|---|---|
| `area/schema.sql` | Add `skills` and `skill_effects` tables |
| `src/headers/ack.h` | Add `skill_effects` list struct; `skill_table[]` non-const for dynamic load |
| `src/const.c` | Remove `skill_table[]` initializer (Phase 2) |
| `src/db.c` | Call `db_load_skill_table()` in `boot_db()` |
| `src/db/db_skills.c` | New: boot loader, `effect_execute()`, all effect handlers, gsn + custom registries |
| `src/db/db_skills.h` | New: declarations |
| `src/spells/spell_table_data.c` | Deleted (Phase 2) |
| `src/skills/skill_table_data.c` | Deleted (Phase 2) |
| `src/spells/spell_*.c` | ~190 files deleted (Phase 3), except ~8 `CUSTOM` functions |
| `src/skills/do_*.c` | ~101 files deleted (Phase 3), except edge cases |
| `tools/migrate_skills_to_db.py` | New: parses C data + source files, populates DB |

### tngdb

| File | Change |
|---|---|
| `api/main.py` | Add `/skills`, `/skills/{sn}`, `/skills/lookup/{name}` endpoints (Phase 1) |

---

## Trade-offs and Risks

| Risk | Mitigation |
|---|---|
| SN stability: renumbering rows corrupts player saves | `sn` is PK, never reassigned. Migration preserves existing positions. |
| `custom_registry` out of sync with DB | Boot logs a fatal error if a `CUSTOM` function name has no registry entry. |
| Formula evaluation security (e.g. `"level*2d6"`) | Formulas are a small closed DSL evaluated by a simple C parser — no `eval`, no arbitrary code. |
| Migration script misclassifies a spell | Phase 1 is read-only; game still boots from C. Migration validated against original before Phase 2. |
| `effect_execute()` performance vs direct function call | Dispatching ~15 effect types is an array lookup + function call — negligible vs combat logic. |
| DB unavailable at boot | `db_load_skill_table()` calls `exit()` on failure — same as all other DB boot connections. |

### Intentional Non-Changes

- `MAX_SKILL` (999) ceiling unchanged.
- `learned[]`, `cooldown[]`, `can_use_skill()`, `mana_cost()`, `raise_skill()` — all unchanged.
- `skill_lookup()` still searches `skill_table[]` (now DB-loaded) — no change to call sites.
- NPC `skills`/`power_skills` bitfields in `mobs` table — out of scope.

---

## Open Questions for Discussion

1. **Formula DSL scope**: Should the formula mini-language support only arithmetic + dice (`NdM`, `level*N`, `level/N+M`) or also `class_level[X]` references? The latter appears in several holy/druid spells.
2. **`APPLY_AFFECT` multi-affect**: Should multiple affects in one `APPLY_AFFECT` entry all share the same duration, or should each sub-affect have its own duration fields?
3. **HOT/DOT modifier formulas**: Some HOTs use `class_heal_character()` which depends on class-specific scaling. Should this be a named formula (`"class_heal"`) or should the formula DSL be able to express it?
4. **Phase 3 order**: Should the effect dispatcher be built and validated *before* deleting the old `spell_*.c` files (running both in parallel via a compile flag), or delete immediately once handlers are proven equivalent?
5. **tngdb auth**: The `/skills` endpoint will be public read-only, consistent with existing endpoints — confirm?
