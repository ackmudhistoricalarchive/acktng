# Proposal: Move Skill/Spell Definitions to Database with Lua Scripting

**Status:** Open
**Date:** 2026-03-23 (revised 2026-03-24)
**Repos affected:** acktng, tngdb

---

## Problem

All skill and spell metadata and logic is compiled directly into the game binary:

- `src/spells/spell_table_data.c` (~3087 lines) — metadata for ~200+ spells
- `src/skills/skill_table_data.c` (~3358 lines) — metadata for ~300+ skills
- `src/spells/spell_*.c` — 246 individual C functions, one per spell
- `src/skills/do_*.c` — 109 individual C files, one per skill
- `src/ai/spec_*.c` — 120+ NPC special procedure functions, one per behavior

Adding, removing, or tuning any skill, spell, or NPC behavior requires editing C source and recompiling the game. The web frontend and tngdb have no access to skill/spell data at all.

The previous revision of this proposal addressed the metadata problem by moving definitions to PostgreSQL, and addressed the logic problem with a fixed taxonomy of ~15 composable effect types (`DAMAGE`, `HEAL`, `APPLY_AFFECT`, etc.) stored as JSONB parameter blobs. That approach had two weaknesses:

1. **Rigid effect taxonomy.** Any spell that didn't decompose into the predefined effect types required a `CUSTOM` fallback to a compiled C function. Eight spells were already identified as requiring this, and any future spell with novel mechanics would also need new C code — defeating the purpose of data-driving.

2. **Formula DSL limitations.** The proposed formula mini-language (`"5d8+level"`, `"level/4"`) was too simple for spells that reference class levels, combo counts, shield pools, or conditional branches. Extending the DSL toward those features would amount to inventing a bad programming language.

This revised proposal replaces the effect-composition/JSONB approach with **embedded Lua scripting**. Spell and skill logic is written in Lua scripts that call into a C API exposing game engine primitives. The same Lua engine also provides a future path for NPC AI scripting, replacing the 120+ compiled `spec_*.c` files.

---

## Goal

1. Move skill/spell **metadata** (name, per-class levels, mana cost, beats, etc.) from compiled C tables into PostgreSQL — unchanged from the original proposal.

2. Move skill/spell **logic** from compiled C functions into **Lua scripts** stored on disk (loaded at boot, reloadable at runtime). Each spell or skill that currently has a `spell_*.c` or `do_*.c` file gets a corresponding `.lua` script.

3. Embed a **Lua 5.4 interpreter** in the game server, with a sandboxed C API that exposes game primitives (damage, healing, affects, object manipulation, character queries, etc.) to Lua scripts.

4. Establish the Lua engine as the **shared scripting layer** for spells, skills, and (in a future phase) NPC AI — replacing `spec_*.c` special procedures.

5. Expose skill/spell metadata via **tngdb API endpoints** for the web frontend.

---

## Architecture Overview

```
                    +-----------------+
                    |   PostgreSQL    |
                    |  skills table   |  metadata (name, levels, mana, beats, etc.)
                    +--------+--------+
                             |
                      boot_db() loads
                             |
                             v
   +-------------------+    skill_table[]    +-------------------+
   |   magic.c         |  (runtime array)    |   skills.c        |
   |  cast/obj_cast    +---> lookup sn +---->+  can_use_skill    |
   |  mana_cost()      |                     |  energy_cost()    |
   +--------+----------+                     +--------+----------+
            |                                         |
            |  spell_fun(sn,level,ch,vo,obj)          |  do_fun(ch,argument)
            |                                         |
            v                                         v
   +----------------------------------------------------------+
   |                  Lua VM  (lua_State *)                    |
   |                                                           |
   |  scripts/spells/fireball.lua    scripts/skills/bash.lua   |
   |  scripts/spells/animate.lua     scripts/skills/chakra.lua |
   |  ...                            ...                       |
   +---------------------+------------------------------------+
                          |
                 calls C API functions
                          |
                          v
   +----------------------------------------------------------+
   |               C API  (src/lua/lua_api.c)                  |
   |                                                           |
   |  mud.damage()      mud.heal()       mud.send()            |
   |  mud.apply_affect() mud.remove_affect()  mud.act()        |
   |  mud.war_attack()  mud.saves_spell()  mud.dice()          |
   |  mud.create_object() mud.teleport() mud.get_room()        |
   |  char.hp  char.level  char.class_level[X]  char.mana     |
   |  ...                                                      |
   +----------------------------------------------------------+
```

**Data flow for casting a spell (e.g. `cast fireball goblin`):**

1. `magic.c:do_cast()` resolves the spell name to `sn`, checks mana, resolves target — all unchanged.
2. Instead of calling a compiled `spell_fireball()` C function, it calls `lua_spell_execute(sn, level, ch, vo, obj)`.
3. The Lua dispatcher loads the precompiled bytecode for `scripts/spells/fireball.lua` and calls its `execute()` function, passing a Lua table with the spell context (sn, level, caster, victim, obj).
4. The Lua script calls C API functions like `mud.damage()`, `mud.saves_spell()`, `mud.send()` which are thin wrappers around existing C engine functions.
5. Control returns to `magic.c`, which deducts mana on success — unchanged.

**Data flow for using a skill (e.g. `bash goblin`):**

1. `interp.c` dispatches to `lua_skill_execute(gsn_bash, ch, argument)`.
2. The Lua script for bash calls `mud.can_use_skill()`, `mud.war_attack()`, `mud.wait_state()`, etc.
3. Control returns to the interpreter.

### Why Lua

| Criterion | Lua | Custom DSL/JSONB | Python/JS |
|---|---|---|---|
| Embedding size | ~300 KB, zero dependencies | N/A | Huge runtime |
| Speed | LuaJIT-competitive; C API calls are direct | Interpreted JSON walks | Slower, GC pauses |
| MUD heritage | Industry standard (CoffeeMUD, MudOS forks, NeverWinter Nights, WoW) | Novel, untested | Rare in MUDs |
| Expressiveness | Full language: loops, conditionals, tables, closures | Limited to predefined effect types | Full but overkill |
| Sandboxing | Remove `io`, `os`, `loadfile`; whitelist API | Inherently sandboxed | Difficult to sandbox |
| Learning curve | Simple syntax, well-documented, familiar to game designers | Custom, must be learned | Familiar but heavy |
| Hot reload | `luaL_loadfile()` at runtime, no recompile | DB UPDATE + reload | Requires restart |

---

## Database Schema

### `skills` table

Holds metadata — unchanged from the original proposal. This is the single source of truth for skill/spell definitions, loaded into `skill_table[]` at boot.

```sql
CREATE TABLE skills (
    sn            SMALLINT    PRIMARY KEY,        -- stable index, matches current array position
    name          TEXT        NOT NULL UNIQUE,
    flag2         SMALLINT    NOT NULL DEFAULT 1,  -- 1=NORM
    target        SMALLINT    NOT NULL DEFAULT 0,
    min_position  SMALLINT    NOT NULL DEFAULT 0,
    gsn_name      TEXT        NOT NULL DEFAULT '',  -- C gsn variable name; '' if none
    slot          SMALLINT    NOT NULL DEFAULT 0,
    min_mana      SMALLINT    NOT NULL DEFAULT 0,
    beats         SMALLINT    NOT NULL DEFAULT 0,
    can_learn     BOOLEAN     NOT NULL DEFAULT TRUE,
    noun_damage   TEXT        NOT NULL DEFAULT '',
    msg_off       TEXT        NOT NULL DEFAULT '',
    room_off      TEXT        NOT NULL DEFAULT '',
    growth        SMALLINT    NOT NULL DEFAULT 0,
    class_levels  JSONB       NOT NULL DEFAULT '{}', -- {"MAG": 5, "CLE": 10, ...}; absent = NO_USE
    script_file   TEXT        NOT NULL DEFAULT ''     -- Lua script filename, e.g. "spells/fireball"
);
```

The `script_file` column names the Lua script that implements this skill/spell. The engine prepends `scripts/` and appends `.lua` to form the file path. An empty string means no script (skill is passive or uses a C fallback during migration).

The `skill_effects` table from the original proposal is **removed entirely** — Lua scripts replace effect composition.

### No schema changes for cooldowns or learned percentages

`cooldown[MAX_SKILL]` and `learned[MAX_SKILL]` remain as C arrays on `CHAR_DATA` / `PC_DATA` respectively, indexed by `sn`. These are per-character runtime state, not skill definitions. They are unchanged by this proposal:

- **`learned[sn]`** — Player's proficiency percentage (0-100) in skill `sn`. Set by `raise_skill()`, checked by `skill_success()`. Lua scripts read this via `char:learned(sn)` but never write it directly — they call `mud.raise_skill(ch, sn)` which delegates to the existing C function.
- **`cooldown[sn]`** — Ticks remaining before skill `sn` can be used again. Set by the Lua script via `mud.set_cooldown(ch, sn, ticks)`, checked by `can_use_skill()` in C before the Lua script is ever invoked.
- **`WAIT_STATE`** — Combat pulse delay after using a skill. Set by the Lua script via `mud.wait_state(ch, beats)`, using the value from `skill_table[sn].beats`.

---

## Lua C API

The C API exposed to Lua scripts is organized into modules. Each module is a set of C functions registered into the Lua VM at boot. Scripts access them as `mud.damage()`, `char:get_hp()`, etc.

### `mud` module — Game engine primitives

These are thin wrappers around existing C functions. They do not introduce new game mechanics — they expose what already exists.

#### Combat

| Lua function | C function it wraps | Description |
|---|---|---|
| `mud.damage(ch, victim, dam, sn, element, show)` | `sp_damage()` / `calculate_damage()` | Deal damage with element and skill attribution |
| `mud.damage_from_obj(obj, ch, victim, dam, element, sn, show)` | `sp_damage()` | Damage from an object-cast spell |
| `mud.war_attack(ch, victim, gsn)` | `war_attack()` | Standard melee skill attack (handles hit check, damage calc, combo) |
| `mud.saves_spell(level, victim)` | `saves_spell()` | Returns true if victim saves vs spell |
| `mud.is_safe(ch, victim)` | `is_safe()` | PK/safe-room check |
| `mud.can_hit_skill(ch, victim, gsn)` | `can_hit_skill()` | Skill hit check (incorporates learned%) |

#### Healing and Resources

| Lua function | C function | Description |
|---|---|---|
| `mud.heal(victim, amount)` | direct `victim->hit` manipulation + `update_pos()` | Heal HP |
| `mud.heal_mana(victim, amount)` | direct `victim->mana` manipulation | Restore mana |
| `mud.heal_move(victim, amount)` | direct `victim->move` manipulation | Restore movement |
| `mud.class_heal(ch, victim, base, sn, class)` | `class_heal_character()` | Class-scaled healing |

#### Affects

| Lua function | C function | Description |
|---|---|---|
| `mud.apply_affect(victim, af_table)` | `affect_to_char()` | Apply an affect (type, duration, location, modifier, bitvector, duration_type) |
| `mud.affect_join(victim, af_table)` | `affect_join()` | Apply or stack an affect |
| `mud.affect_strip(victim, sn)` | `affect_strip()` | Remove all affects of type `sn` |
| `mud.remove_affect_flag(victim, flag)` | `affect_strip()` by bitvector | Remove affects with specific AFF_* flag |
| `mud.is_affected(victim, sn)` | `is_affected()` | Check if victim has affect of type `sn` |
| `mud.has_affect_flag(victim, flag)` | `IS_AFFECTED()` macro | Check AFF_* bitvector |
| `mud.apply_room_affect(room, ra_table)` | `affect_to_room()` | Apply a room affect |

#### Objects and World

| Lua function | C function | Description |
|---|---|---|
| `mud.create_object(vnum, level)` | `create_object()` | Create an object instance from index |
| `mud.obj_to_room(obj, room)` | `obj_to_room()` | Place object in room |
| `mud.obj_to_char(obj, ch)` | `obj_to_char()` | Give object to character |
| `mud.get_obj_carry(ch, name)` | `get_obj_carry()` | Find object in character's inventory |
| `mud.extract_obj(obj)` | `extract_obj()` | Remove object from world |
| `mud.get_room(vnum)` | `get_room_index()` | Look up room by vnum |
| `mud.transfer(ch, room)` | `char_from_room()` + `char_to_room()` | Move character to room |

#### Characters and Followers

| Lua function | C function | Description |
|---|---|---|
| `mud.create_mobile(vnum)` | `create_mobile()` | Create NPC instance from index |
| `mud.char_to_room(mob, room)` | `char_to_room()` | Place character in room |
| `mud.add_follower(mob, master)` | `add_follower()` | Make mob follow master |
| `mud.stop_follower(mob)` | `stop_follower()` | Remove follower |
| `mud.extract_char(mob, pull)` | `extract_char()` | Remove character from world |

#### Output and Communication

| Lua function | C function | Description |
|---|---|---|
| `mud.send(ch, text)` | `send_to_char()` | Send colored text to character |
| `mud.act(format, ch, arg1, arg2, target)` | `act()` | Send formatted action message |
| `mud.echo_room(room, text)` | `send_to_room()` | Send text to all in room |

#### Skill System

| Lua function | C function | Description |
|---|---|---|
| `mud.raise_skill(ch, sn)` | `raise_skill()` | Increase learned% on successful use |
| `mud.wait_state(ch, beats)` | `WAIT_STATE()` macro | Set combat delay |
| `mud.set_cooldown(ch, sn, ticks)` | direct `ch->cooldown[sn] = ticks` | Set skill cooldown |
| `mud.can_use_skill(ch, sn)` | `can_use_skill()` | Check if skill is available (cooldown, level, etc.) |
| `mud.subtract_energy(ch, gsn)` | `subtract_energy_cost()` | Deduct move cost for physical skills |
| `mud.mana_cost(ch, sn)` | `mana_cost()` | Calculate mana cost for a spell |
| `mud.skill_success(ch, sn)` | `skill_success()` | Roll against learned% for skill check |

#### Randomness and Utility

| Lua function | C function | Description |
|---|---|---|
| `mud.dice(n, m)` | `dice()` | Roll NdM |
| `mud.number_range(lo, hi)` | `number_range()` | Random integer in range |
| `mud.number_percent()` | `number_percent()` | Random 1-100 |
| `mud.UMIN(a, b)` | `UMIN()` | Integer minimum |
| `mud.UMAX(a, b)` | `UMAX()` | Integer maximum |

#### Combo System

| Lua function | C function | Description |
|---|---|---|
| `mud.combo(ch, victim, gsn)` | `combo()` | Register hit in combo chain |
| `mud.get_combo_count(ch)` | `get_combo_count()` | Current combo length |
| `mud.reset_combo(ch)` | `reset_combo()` | Clear combo chain |

#### Shield System

| Lua function | C function | Description |
|---|---|---|
| `mud.is_shielded(ch, type)` | `is_shielded()` | Check for magic shield |
| `mud.add_shield(ch, shield_table)` | shield LINK code | Add magic shield to character |
| `mud.remove_shield(ch, type)` | shield UNLINK code | Remove magic shield |

### `char` userdata — Character properties

Character pointers (`CHAR_DATA *`) are exposed as Lua userdata with read-only property access via `__index` metamethods:

```lua
-- Read-only properties (examples)
ch:get_hp()              -- ch->hit
ch:get_max_hp()          -- get_max_hp(ch)
ch:get_mana()            -- ch->mana
ch:get_max_mana()        -- ch->max_mana
ch:get_move()            -- ch->move
ch:get_level()           -- ch->level
ch:get_class_level(cls)  -- ch->class_level[cls]
ch:get_alignment()       -- ch->alignment
ch:get_gold()            -- ch->gold
ch:get_name()            -- ch->name
ch:get_room()            -- ch->in_room (returns room userdata)
ch:get_fighting()        -- ch->fighting (returns char userdata or nil)
ch:get_position()        -- ch->position
ch:get_str()             -- get_curr_str(ch)
ch:get_dex()             -- get_curr_dex(ch)
ch:get_wis()             -- get_curr_wis(ch)
ch:get_int()             -- get_curr_int(ch)
ch:get_con()             -- get_curr_con(ch)
ch:get_chi()             -- get_chi(ch)
ch:is_npc()              -- IS_NPC(ch)
ch:is_affected(sn)       -- is_affected(ch, sn)
ch:learned(sn)           -- ch->pcdata->learned[sn] (0 for NPCs)
ch:cooldown(sn)          -- ch->cooldown[sn]

-- Mutable properties (via set functions, not direct assignment)
ch:set_hp(val)           -- ch->hit = val; update_pos(ch)
ch:set_mana(val)         -- ch->mana = val
ch:set_move(val)         -- ch->move = val
ch:set_alignment(val)    -- ch->alignment = val
ch:set_gold(val)         -- ch->gold = val
ch:set_position(val)     -- ch->position = val
```

Mutable setters are restricted to values that spell/skill scripts legitimately need to modify. Structural fields (name, level, class, room) are changed only through `mud.*` functions that enforce game rules.

### Constants

Game constants are pre-loaded into the Lua VM as read-only tables:

```lua
ELE.FIRE            -- ELE_FIRE
ELE.LIGHTNING       -- ELE_LIGHTNING
ELE.HOLY            -- ELE_HOLY
-- etc.

AFF.BLIND           -- AFF_BLIND
AFF.POISON          -- AFF_POISON
AFF.SANCTUARY       -- AFF_SANCTUARY
-- etc.

APPLY.AC            -- APPLY_AC
APPLY.HITROLL       -- APPLY_HITROLL
APPLY.DAMROLL       -- APPLY_DAMROLL
APPLY.DOT           -- APPLY_DOT
-- etc.

POS.STANDING        -- POS_STANDING
POS.FIGHTING        -- POS_FIGHTING
-- etc.

TAR.CHAR_OFFENSIVE  -- TAR_CHAR_OFFENSIVE
TAR.CHAR_DEFENSIVE  -- TAR_CHAR_DEFENSIVE
-- etc.

CLASS.MAG           -- CLASS_MAG
CLASS.CLE           -- CLASS_CLE
CLASS.WAR           -- CLASS_WAR
CLASS.MON           -- CLASS_MON
-- etc.

DURATION.HOUR       -- DURATION_HOUR
DURATION.ROUND      -- DURATION_ROUND
```

---

## Example Lua Scripts

These examples show how existing C spell/skill functions translate to Lua. Each script exports an `execute()` function that receives a context table.

### Simple damage spell: Fireball

Current C (`src/spells/spell_fireball.c`, ~50 lines):
```c
bool spell_fireball(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj) {
    static const sh_int dam_each[] = { 0, 0, ..., 130 };
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    int dam = number_range(dam_each[level]/2, dam_each[level]*2);
    if (saves_spell(level, victim)) dam /= 2;
    sp_damage(obj, ch, victim, dam, ELE_FIRE, sn, TRUE);
    return TRUE;
}
```

Lua equivalent (`scripts/spells/fireball.lua`):
```lua
local dam_each = {
    [0]=0, 0, 0, 0, 0, 0, 0, 0, 0, 0,             -- 0-9
    0, 0, 0, 0, 0, 30, 35, 40, 45, 50,             -- 10-19
    55, 60, 65, 70, 75, 80, 82, 84, 86, 88,        -- 20-29
    90, 92, 94, 96, 98, 100, 102, 104, 106, 108,   -- 30-39
    110, 112, 114, 116, 118, 120, 122, 124, 126, 128, -- 40-49
    130                                              -- 50
}

function execute(ctx)
    local level = mud.UMIN(ctx.level, #dam_each)
    local base = dam_each[level] or dam_each[#dam_each]
    local dam = mud.number_range(base / 2, base * 2)

    if mud.saves_spell(level, ctx.victim) then
        dam = dam / 2
    end

    mud.damage(ctx.ch, ctx.victim, dam, ctx.sn, ELE.FIRE, true)
    return true
end
```

### Healing spell: Cure Light

Current C (~15 lines). Lua equivalent:
```lua
function execute(ctx)
    local heal = mud.dice(5, 8) + ctx.level
    heal = mud.UMIN(heal, 50)

    local victim = ctx.victim
    victim:set_hp(mud.UMIN(victim:get_hp() + heal, victim:get_max_hp()))
    mud.send(victim, "@@aYou feel better!@@N\n")
    return true
end
```

### Buff spell: Armor

```lua
function execute(ctx)
    local victim = ctx.victim

    if victim:is_affected(ctx.sn) then
        mud.send(ctx.ch, "They are already armored.\n")
        return false
    end

    mud.apply_affect(victim, {
        type     = ctx.sn,
        duration = 24,
        location = APPLY.AC,
        modifier = -20,
        duration_type = DURATION.HOUR,
    })

    mud.act("$N is surrounded by a protective aura.", ctx.ch, nil, victim, "room")
    mud.send(victim, "@@aYou feel someone protecting you.@@N\n")
    return true
end
```

### DOT spell: Poison

```lua
function execute(ctx)
    local victim = ctx.victim

    if mud.saves_spell(ctx.level, victim) then
        mud.send(ctx.ch, "They resist your poison.\n")
        return false
    end

    mud.apply_affect(victim, {
        type     = ctx.sn,
        duration = ctx.level / 3,
        location = APPLY.DOT,
        modifier = ctx.level / 5,
        bitvector = AFF.POISON,
        duration_type = DURATION.ROUND,
    })

    mud.act("$N looks very ill.", ctx.ch, nil, victim, "room")
    mud.send(victim, "@@dYou feel very sick.@@N\n")
    return true
end
```

### Melee skill: Bash

Current C calls `war_attack()` directly. Lua equivalent:
```lua
function execute(ctx)
    local ch = ctx.ch
    local argument = ctx.argument

    if not ch:get_fighting() and argument == "" then
        mud.send(ch, "Bash whom?\n")
        return false
    end

    if not mud.can_use_skill(ch, ctx.sn) then
        return false
    end

    mud.subtract_energy(ch, ctx.sn)
    mud.wait_state(ch, skill_table[ctx.sn].beats)
    mud.raise_skill(ch, ctx.sn)
    mud.war_attack(ch, argument, ctx.sn)
    return true
end
```

### Complex spell: Animate (previously required CUSTOM C fallback)

This is one of the ~8 spells that could not be expressed as effect composition. With Lua, it needs no special handling:

```lua
function execute(ctx)
    local ch = ctx.ch
    local argument = ctx.argument

    -- Find the corpse in the room
    local corpse = mud.get_obj_room(ch:get_room(), argument)
    if not corpse or corpse:get_item_type() ~= ITEM.NPC_CORPSE then
        mud.send(ch, "You don't see that corpse here.\n")
        return false
    end

    -- Create the zombie mob
    local mob = mud.create_mobile(MOB_VNUM.ZOMBIE)
    if not mob then
        mud.send(ch, "The animation fails.\n")
        return false
    end

    -- Scale to caster
    mob:set_level(ch:get_level())
    mob:set_max_hp(ch:get_max_hp() / 2)
    mob:set_hp(mob:get_max_hp())

    -- Transfer items from corpse to mob and equip them
    local items = mud.get_obj_contents(corpse)
    for _, item in ipairs(items) do
        mud.obj_from_obj(item)
        mud.obj_to_char(item, mob)
    end
    mud.interpret(mob, "wear all")

    -- Place in room and make follower
    mud.char_to_room(mob, ch:get_room())
    mud.add_follower(mob, ch)
    mud.extract_obj(corpse)

    mud.act("$n gestures at $p... it rises!", ch, corpse, nil, "room")
    mud.send(ch, "@@eYou animate the corpse!@@N\n")
    return true
end
```

### Complex spell: Chain Lightning (AOE with decay)

```lua
local dam_each = { --[[ level table as above ]] }

function execute(ctx)
    local ch = ctx.ch
    local victim = ctx.victim
    local level = ctx.level
    local base = dam_each[mud.UMIN(level, #dam_each)]
    local dam = mud.number_range(base / 2, base * 2)

    -- Primary target
    if mud.saves_spell(level, victim) then
        dam = dam / 2
    end
    mud.damage(ch, victim, dam, ctx.sn, ELE.LIGHTNING, true)

    -- Chain to others in room
    for _, vch in ipairs(mud.chars_in_room(ch:get_room())) do
        if vch ~= victim and vch ~= ch and not mud.is_safe(ch, vch) then
            dam = (4 * dam) / 5  -- 20% decay per target
            if mud.saves_spell(level, vch) then
                dam = dam / 2
            end
            mud.damage(ch, vch, dam, ctx.sn, ELE.LIGHTNING, true)
        end
    end

    return true
end
```

---

## Lua Engine Implementation

### Embedding: `src/lua/lua_engine.c`

A single `lua_State *` is created at boot and shared across all spell/skill invocations. Lua is single-threaded, which is fine — the MUD game loop is also single-threaded.

```c
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static lua_State *L = NULL;

void lua_engine_init(void) {
    L = luaL_newstate();
    // Open only safe libraries — no io, os, debug, loadfile, dofile
    luaL_requiref(L, "base",   luaopen_base,   1);  // print, type, pairs, etc.
    luaL_requiref(L, "table",  luaopen_table,   1);
    luaL_requiref(L, "string", luaopen_string,  1);
    luaL_requiref(L, "math",   luaopen_math,    1);

    // Remove dangerous globals
    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "require");

    // Register C API modules
    lua_register_mud_api(L);       // mud.damage(), mud.heal(), etc.
    lua_register_constants(L);     // ELE, AFF, APPLY, POS, CLASS, etc.
    lua_register_char_metatable(L); // char userdata with __index
    lua_register_obj_metatable(L);  // obj userdata with __index
    lua_register_room_metatable(L); // room userdata with __index
}

void lua_engine_shutdown(void) {
    if (L) lua_close(L);
    L = NULL;
}
```

### Script Loading and Caching

Scripts are loaded from `area/scripts/` at boot and compiled to bytecode. Compiled bytecode is cached in a Lua registry table keyed by script name, avoiding re-parsing on every cast.

```c
// Load and compile a script, store bytecode in registry
bool lua_load_script(const char *script_name) {
    char path[MAX_STRING_LENGTH];
    snprintf(path, sizeof(path), "scripts/%s.lua", script_name);

    if (luaL_loadfile(L, path) != LUA_OK) {
        log_string("Lua: failed to load %s: %s", path, lua_tostring(L, -1));
        lua_pop(L, 1);
        return FALSE;
    }

    // Execute the chunk to define its functions in a new environment
    lua_newtable(L);                        // script environment
    lua_newtable(L);                        // metatable
    lua_getglobal(L, "_G");
    lua_setfield(L, -2, "__index");         // inherit from _G
    lua_setmetatable(L, -2);
    lua_setupvalue(L, -2, 1);              // set as chunk's _ENV

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        log_string("Lua: error in %s: %s", path, lua_tostring(L, -1));
        lua_pop(L, 1);
        return FALSE;
    }

    // Store environment in registry[script_name]
    // ... (registry storage code)
    return TRUE;
}
```

### Spell Dispatch

```c
bool lua_spell_execute(int sn, int level, CHAR_DATA *ch, void *vo, OBJ_DATA *obj) {
    const char *script = skill_table[sn].script_file;
    if (script[0] == '\0') return FALSE;  // no script — fall through to C

    // Push the script's execute() function from registry
    lua_get_script_function(L, script, "execute");

    // Build context table
    lua_newtable(L);
    lua_pushinteger(L, sn);     lua_setfield(L, -2, "sn");
    lua_pushinteger(L, level);  lua_setfield(L, -2, "level");
    lua_push_char(L, ch);       lua_setfield(L, -2, "ch");

    // Push victim/target based on skill_table[sn].target
    if (skill_table[sn].target == TAR_CHAR_OFFENSIVE ||
        skill_table[sn].target == TAR_CHAR_DEFENSIVE ||
        skill_table[sn].target == TAR_CHAR_SELF) {
        lua_push_char(L, (CHAR_DATA *)vo);
        lua_setfield(L, -2, "victim");
    } else if (skill_table[sn].target == TAR_OBJ_INV) {
        lua_push_obj(L, (OBJ_DATA *)vo);
        lua_setfield(L, -2, "obj_target");
    }

    if (obj) {
        lua_push_obj(L, obj);
        lua_setfield(L, -2, "cast_obj");
    }

    // Call with error handler
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        log_string("Lua spell error [%s]: %s",
                   skill_table[sn].name, lua_tostring(L, -1));
        lua_pop(L, 1);
        return FALSE;
    }

    bool result = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return result;
}
```

### Skill Dispatch

```c
void lua_skill_execute(int sn, CHAR_DATA *ch, char *argument) {
    const char *script = skill_table[sn].script_file;
    if (script[0] == '\0') return;

    lua_get_script_function(L, script, "execute");

    lua_newtable(L);
    lua_pushinteger(L, sn);      lua_setfield(L, -2, "sn");
    lua_push_char(L, ch);        lua_setfield(L, -2, "ch");
    lua_pushstring(L, argument); lua_setfield(L, -2, "argument");

    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        log_string("Lua skill error [%s]: %s",
                   skill_table[sn].name, lua_tostring(L, -1));
        lua_pop(L, 1);
    } else {
        lua_pop(L, 1);
    }
}
```

### Hot Reload

A staff command `luareload <script_name>` (or `luareload all`) reloads scripts at runtime without restarting the server:

```c
void do_luareload(CHAR_DATA *ch, char *argument) {
    if (argument[0] == '\0' || !str_cmp(argument, "all")) {
        lua_load_all_scripts();
        send_to_char("All Lua scripts reloaded.\n", ch);
    } else {
        if (lua_load_script(argument)) {
            send_to_char("Script reloaded.\n", ch);
        } else {
            send_to_char("Failed to reload script. Check logs.\n", ch);
        }
    }
}
```

This is one of the primary advantages over compiled C — tuning a spell's damage formula, adjusting a buff duration, or fixing a bug in a skill can be done by editing a `.lua` file and typing `luareload fireball` in-game. No compile, no reboot.

### Sandboxing and Safety

The Lua VM is sandboxed to prevent scripts from:

1. **File I/O** — `io`, `os`, `loadfile`, `dofile`, `require` are removed at init.
2. **Infinite loops** — A Lua debug hook counts instructions and aborts scripts exceeding a configurable limit (e.g. 100,000 instructions). This prevents a buggy script from hanging the game loop.
3. **Stack overflow** — `lua_checkstack()` before deep operations.
4. **Memory exhaustion** — A custom Lua allocator with a per-call byte ceiling.
5. **Direct memory access** — Character/object/room data is accessed only through the metatable API, never through raw pointers.

```c
// Instruction limit hook
static void lua_instruction_hook(lua_State *L, lua_Debug *ar) {
    luaL_error(L, "Script exceeded instruction limit");
}

// Set before each script call
lua_sethook(L, lua_instruction_hook, LUA_MASKCOUNT, MAX_LUA_INSTRUCTIONS);
```

### Error Handling

If a Lua script errors (syntax, runtime, instruction limit), the error is:
1. Logged to the server log with the script name and error message.
2. The spell/skill returns `FALSE` (spell fizzles, skill fails).
3. The game loop continues normally — a broken script cannot crash the server.
4. If the caster is staff (level >= LEVEL_IMMORTAL), the error message is also sent to them in-game for debugging.

---

## NPC AI Scripting (Future Phase)

The same Lua engine and C API supports replacing compiled `spec_*.c` NPC special procedures with Lua scripts. This is **out of scope for the initial implementation** but is a key motivation for choosing Lua over a closed DSL.

### Current NPC AI architecture

Each NPC mob index has a `SPEC_FUN *spec_fun` — a pointer to a C function with signature `bool spec_fn(CHAR_DATA *ch)`. The game loop calls this every combat round (for fighting NPCs) and on a random tick (for idle NPCs). There are 120+ spec functions in `src/ai/`, each compiled into the binary.

Additionally, `SPEECH_FUN *speech_fun` handles NPC responses to player speech, with signature `bool speech_fn(CHAR_DATA *mob, CHAR_DATA *player, const char *message)`.

### Future Lua NPC scripts

A future phase would add:

- `scripts/npc/<spec_name>.lua` — NPC behavior scripts with `on_tick(mob)`, `on_combat(mob)`, `on_speech(mob, player, message)` entry points.
- A `spec_lua` C function that dispatches to the appropriate Lua script based on the mob's `spec_script` field.
- The same `mud.*` and `char:*` APIs already built for spells/skills — no new API layer needed.
- `SPEECH_FUN` handlers could also dispatch to Lua, coexisting with the existing LLM speech dispatch.

This is mentioned here to confirm that the Lua API surface designed for spells/skills is broad enough to serve NPC AI without a second scripting layer.

---

## Phasing

| Phase | What changes | Binary change? | Description |
|---|---|---|---|
| **1: Lua engine** | New `src/lua/` directory; Lua 5.4 linked into build | Yes | Embed Lua, register C API, sandboxing, error handling. No spells/skills moved yet. |
| **2: Metadata to DB** | `skills` table in PostgreSQL; boot loads from DB | Yes | `skill_table[]` populated from DB instead of compiled arrays. Deletes `spell_table_data.c` + `skill_table_data.c`. |
| **3: Spell scripts** | `area/scripts/spells/*.lua` | Yes | Migrate all 246 `spell_*.c` files to Lua scripts. `spell_fun` in `skill_table[]` points to `lua_spell_execute` for migrated spells. |
| **4: Skill scripts** | `area/scripts/skills/*.lua` | Yes | Migrate all 109 `do_*.c` skill files to Lua scripts. Command table entries point to `lua_skill_execute` for migrated skills. |
| **5: tngdb API** | `/skills` endpoints in tngdb | No | Expose skill metadata via REST API for web frontend. |
| **6: NPC AI** (future) | `area/scripts/npc/*.lua` | Yes | Migrate `spec_*.c` to Lua. Out of scope for this proposal. |

### Phase details

**Phase 1** is the foundation — it can be built and tested independently with a few test scripts before any real spells are migrated. It introduces the build dependency on Lua 5.4 (`liblua5.4-dev`).

**Phase 2** is unchanged from the original proposal. The `skills` table schema gains only one new column (`script_file`) compared to the original.

**Phases 3 and 4** are the bulk of the migration work. Each spell/skill is migrated individually — the C function is kept alongside the Lua script during migration. A compile flag or runtime check determines which path is used, allowing side-by-side validation.

**Phase 5** is unchanged from the original proposal.

**Phase 6** is future work, listed for architectural awareness only.

---

## Affected Files

### New files

| File | Description |
|---|---|
| `src/lua/lua_engine.c` | Lua VM lifecycle, script loading, caching, hot reload |
| `src/lua/lua_engine.h` | Public declarations for lua_engine |
| `src/lua/lua_api.c` | `mud.*` C API functions registered into Lua |
| `src/lua/lua_api.h` | Public declarations for lua_api |
| `src/lua/lua_char.c` | `char` userdata metatables (character property access) |
| `src/lua/lua_obj.c` | `obj` userdata metatables (object property access) |
| `src/lua/lua_room.c` | `room` userdata metatables (room property access) |
| `src/lua/lua_constants.c` | Constant registration (ELE, AFF, APPLY, POS, CLASS, etc.) |
| `src/db/db_skills.c` | DB boot loader for `skill_table[]` from PostgreSQL |
| `src/db/db_skills.h` | Declarations for db_skills |
| `area/scripts/spells/*.lua` | ~246 spell scripts (Phase 3) |
| `area/scripts/skills/*.lua` | ~109 skill scripts (Phase 4) |
| `tools/migrate_skills_to_db.py` | Migration: parse C tables, populate DB |
| `tools/migrate_spells_to_lua.py` | Migration: translate `spell_*.c` to `.lua` (semi-automated) |
| `tools/migrate_skills_to_lua.py` | Migration: translate `do_*.c` to `.lua` (semi-automated) |

### Modified files

| File | Change |
|---|---|
| `area/schema.sql` | Add `skills` table |
| `src/headers/ack.h` | Add `script_file` field to `SKILL_TYPE`; add Lua engine declarations |
| `src/db.c` | Call `lua_engine_init()` and `db_load_skill_table()` in `boot_db()` |
| `src/magic.c` | In `do_cast()` / `obj_cast_spell()`: dispatch to `lua_spell_execute()` when `script_file` is set |
| `src/interp.c` | For Lua-scripted skills: dispatch to `lua_skill_execute()` |
| `src/comm.c` | Call `lua_engine_shutdown()` on server shutdown |
| `src/Makefile` | Add `src/lua/` objects, link `-llua5.4`, add pkg-config for Lua |

### Deleted files (after migration)

| Files | Count | When |
|---|---|---|
| `src/spells/spell_table_data.c` | 1 | Phase 2 |
| `src/skills/skill_table_data.c` | 1 | Phase 2 |
| `src/spells/spell_*.c` | ~246 | Phase 3 (after Lua equivalents validated) |
| `src/skills/do_*.c` | ~109 | Phase 4 (after Lua equivalents validated) |

---

## Trade-offs and Risks

| Risk | Mitigation |
|---|---|
| **New build dependency (Lua 5.4)** | Lua is a single `.a` / `.so` with zero transitive dependencies. `liblua5.4-dev` is available in all major distros. Added to `setup.sh` alongside existing deps. |
| **SN stability** | Unchanged from original proposal — `sn` is PK, never reassigned. |
| **Lua performance vs direct C** | Lua function call overhead is ~1 microsecond. Spell logic is dominated by the C API calls (damage calc, affect application), not by the Lua wrapper. Profiling needed but expected to be negligible. |
| **Lua VM memory** | A single `lua_State` with ~400 cached script environments uses ~2-5 MB. Trivial compared to area data. |
| **Script errors in production** | Sandboxed execution with `lua_pcall` — errors are caught, logged, and the spell fizzles. Server continues. Staff see errors in-game. |
| **Migration fidelity** | Semi-automated migration tools generate Lua scripts from C source. Each script is validated by running the existing unit tests against both C and Lua paths before the C version is deleted. |
| **Instruction limit too low/high** | Configurable at boot via `area/startup.lua` or a config constant. Start at 100,000 (far more than any spell needs), adjust based on monitoring. |
| **Stale char/obj userdata** | Userdata stores a pointer and a serial/generation number. API calls validate the pointer is still live before dereferencing. Returns nil/error if the character or object was extracted. |
| **DB unavailable at boot** | `db_load_skill_table()` calls `exit()` on failure — same as all other DB boot connections. |

### Intentional Non-Changes

- `MAX_SKILL` (999) ceiling unchanged.
- `learned[]`, `cooldown[]`, `can_use_skill()`, `mana_cost()`, `raise_skill()` — all unchanged in C. Lua scripts call into these existing C functions via the API.
- `skill_lookup()` still searches `skill_table[]` (now DB-loaded) — no change to call sites.
- `war_attack()`, `combo()`, `calculate_damage()`, `sp_damage()` — remain in C. Lua scripts call them, they don't reimplement them.
- The spell/skill dispatch in `magic.c` and `interp.c` is minimally changed — a single `if (script_file[0]) lua_*_execute(...)` check before the existing C path.
- NPC `skills`/`power_skills` bitfields — out of scope.
- Player save format — unchanged. `learned[]` and `cooldown[]` are indexed by `sn` which is stable.

---

## Comparison with Original Effect-Composition Approach

| Aspect | Original (JSONB effects) | Revised (Lua scripts) |
|---|---|---|
| **Spell logic storage** | JSONB blobs in `skill_effects` table rows | `.lua` files in `area/scripts/` |
| **Expressiveness** | Fixed taxonomy of ~15 effect types | Full programming language |
| **Complex spells** | Require `CUSTOM` C fallback (~8+ spells) | All spells expressible in Lua, no fallback needed |
| **Formula language** | Custom DSL (`"5d8+level"`) parsed in C | Native Lua arithmetic (`dice(5,8) + level`) |
| **Conditional logic** | Not supported (or JSONB `"conditions"` arrays) | Native `if/elseif/else` |
| **Iteration** | Not supported (AOE hardcoded as effect type) | Native `for` loops |
| **New DB tables** | `skills` + `skill_effects` | `skills` only (simpler schema) |
| **Hot reload** | DB UPDATE + server reload command | Edit `.lua` file + `luareload` command |
| **NPC AI path** | Would need a separate system | Same Lua engine, same API |
| **Testing** | Assert JSONB output matches expected | Run Lua scripts in test harness with mock API |
| **Tooling for authors** | Edit JSONB in DB or via tngdb admin UI | Edit `.lua` files in any text editor |

---

## Open Questions for Discussion

1. **Lua version**: Lua 5.4 (stable, integer support, widely packaged) vs LuaJIT (faster, stuck at 5.1 semantics, less portable). Recommendation: Lua 5.4 for stability and integer math support.

2. **Script file location**: `area/scripts/` (alongside area data, included in runtime directory) vs `scripts/` at repo root? The `area/` prefix keeps scripts next to the data they operate on and means the server finds them without path configuration.

3. **Migration tooling**: How automated should the C-to-Lua migration be? Options range from fully manual (human translates each spell) to semi-automated (tool generates Lua skeleton from C AST, human reviews) to AI-assisted (LLM translates each C function to Lua given the API spec).

4. **Phase 3 validation strategy**: Run both C and Lua paths in parallel during migration? Or migrate one spell at a time with unit test coverage gating the switchover?

5. **Shared Lua libraries**: Should common patterns (level-based damage tables, standard save-or-halve logic) be extracted into shared Lua modules that spell scripts can call? E.g. `local common = require("spells.common")`. This would need a controlled `require()` that only loads from `scripts/lib/`.

6. **tngdb API**: The `/skills` endpoint will be public read-only, consistent with existing endpoints — confirm?

7. **Hot reload scope**: Should `luareload` also support reloading skill metadata from the DB, or only Lua scripts? Metadata reload would require re-querying `skills` table and rebuilding `skill_table[]`.
