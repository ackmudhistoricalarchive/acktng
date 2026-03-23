# Object Specification

**Version:** strict ingestion format. All rules here are hard requirements enforced by the ingestion validator (`src/areaingest.c`). Violations are rejections; there are no warnings.

---

## 1. Section Structure

```
#OBJECTS
#<vnum>
<name>~
<short_descr>~
<description>~
<item_type> <extra_flags> <wear_flags> <item_apply>
<value0> <value1> <value2> <value3> <value4> <value5> <value6> <value7> <value8> <value9>
<weight>
[zero or more A/E/L trailing entries]
...
#0
```

- Each object is introduced by `#<vnum>` on its own line.
- No blank lines within `#OBJECTS` — a blank line anywhere inside the section is a rejection.
- `<name>`, `<short_descr>`, and `<description>` are tilde-terminated strings.
- The four flag/type fields are on a single line of exactly four integers.
- The value line is exactly 10 integers.
- The weight line is a single integer.
- Trailing entries (`A`, `E`, `L`) follow weight in any order, zero or more per object.
- The section ends with `#0`.

---

## 2. Text Fields

### 2.1. `<name>~`

- Tilde-terminated. Non-empty → rejection.
- The keyword list used for `get`, `drop`, `look at` etc. (e.g., `blade black coral weapon`).
- All lowercase → rejection if any uppercase.
- Must be unique within the area file. Duplicate `<name>~` across two objects in the same file → rejection.
- Must not contain vnums → rejection.

### 2.2. `<short_descr>~`

- Tilde-terminated. Non-empty → rejection.
- Shown in room contents, inventory, etc. (e.g., `a blade of black coral`).
- Should begin with `a`, `an`, or `the` → rejection if it begins with uppercase.
- Must not contain vnums → rejection.

### 2.3. `<description>~`

- Tilde-terminated. Non-empty → rejection.
- Shown when the object is examined or on the ground.
- Must end with exactly one `\n` before the `~` → rejection.
- Must not contain vnums → rejection.

---

## 3. Header Flags Line

```
<item_type> <extra_flags> <wear_flags> <item_apply>
```

Exactly four integers on one line. Fewer or more → rejection.

---

## 4. `item_type` Enum

First integer on the header flags line. Defined in `src/buildtab.c` (`tab_item_types`):

| Value | Name | Notes |
|------:|------|-------|
| 1 | `light` | Light source |
| 2 | `scroll` | Readable spell scroll |
| 3 | `null` | Legacy placeholder; no effect |
| 4 | `staff` | Staff item |
| 5 | `weapon` | Weapon (requires `hold` and `take` wear flags — see §8) |
| 6 | `beacon` | Beacon item |
| 7 | `portal` | Portal item |
| 8 | `treasure` | Generic treasure |
| 9 | `armor` | Armor/shield item |
| 10 | `potion` | Consumable potion |
| 11 | `clutch` | Clutch item |
| 12 | `furniture` | Furniture (can sit/rest on) |
| 13 | `trash` | Trash item |
| 14 | `trigger` | Quest trigger item |
| 15 | `container` | Container (can hold items) |
| 16 | `quest` | Quest item |
| 17 | `drink_con` | Drink container |
| 18 | `key` | Key item |
| 19 | `food` | Food item |
| 20 | `money` | Currency item |
| 21 | `stake` | Stake (vampire hunting) |
| 22 | `boat` | Boat item |
| 23 | `corpse_npc` | NPC corpse — **must not be authored in area files → rejection** |
| 24 | `corpse_pc` | Player corpse — **must not be authored in area files → rejection** |
| 25 | `fountain` | Fountain (infinite drink) |
| 26 | `pill` | Consumable pill |
| 27 | `board` | Bulletin board — see §10 |
| 28 | `soul` | Soul item |
| 29 | `piece` | Piece item (combinable) — see §11 |
| 30 | `matrix` | Spell matrix |
| 31 | `enchantment` | Enchantment item |
| 32 | `present` | Wrapped present |
| 33 | `stash` | Stash container |

Value outside 1–33 → rejection.
`corpse_npc` (23) or `corpse_pc` (24) → rejection; runtime-generated only.

---

## 5. `extra_flags` Bitvector

Second integer on the header flags line. Defined in `src/config.h`:

| Bit | Name | Notes |
|----:|------|-------|
| 1 | `ITEM_GENERATED` | **Runtime-only — must not be authored → rejection** |
| 2 | `ITEM_BIND_EQUIP` | Binds on equip |
| 4 | `ITEM_NODISARM` | Cannot be disarmed |
| 8 | `ITEM_LOCK` | Locked item |
| 16 | `ITEM_EVIL` | Evil-aligned object |
| 32 | `ITEM_INVIS` | Invisible object |
| 64 | `ITEM_MAGIC` | Magical object |
| 128 | `ITEM_NODROP` | Cannot be dropped |
| 256 | `ITEM_BLESS` | Blessed object |
| 512 | `ITEM_ANTI_GOOD` | Good-aligned characters cannot use |
| 1024 | `ITEM_ANTI_EVIL` | Evil-aligned characters cannot use |
| 2048 | `ITEM_UNUSED_ANTI_NEUTRAL` | **Intentionally unused** — anti-neutral alignment was removed; do not set |
| 4096 | `ITEM_NOREMOVE` | Cannot be removed once equipped |
| 8192 | `ITEM_INVENTORY` | Part of mob inventory |
| 16384 | `ITEM_NOSAVE` | Not saved to player file |
| 32768 | `ITEM_CLAN_EQ` | Clan equipment |
| 65536 | `ITEM_TRIG_DESTROY` | Destroyed on use trigger |
| 131072 | `ITEM_NO_AUCTION` | Cannot be auctioned |
| 262144 | `ITEM_MYTHIC` | Mythic rarity |
| 524288 | `ITEM_LEGENDARY` | Legendary rarity |
| 1048576 | `ITEM_RARE` | Rare rarity |
| 2097152 | `ITEM_QUEST_REWARD` | Quest reward item — set by the quest system on inline reward objects; required on all quest reward items. Named **`vamp`** in `buildtab.c` (legacy string name used by the area file parser and admin tools) |
| 4194304 | `ITEM_NOLOOT` | Cannot be looted from corpse |
| 8388608 | `ITEM_NOSAC` | Cannot be sacrificed |
| 16777216 | `ITEM_UNIQUE` | Unique (only one per player) |
| 33554432 | `ITEM_LIFESTEALER` | Lifestealer weapon — requires `ITEM_ANTI_GOOD` |
| 67108864 | `ITEM_LOOT` | Loot-table item — must be set on mob loot objects |
| 134217728 | `ITEM_BOSS` | Boss drop — must be set on items that can drop from bosses |
| 268435456 | `ITEM_BUCKLER` | Buckler shield type |
| 536870912 | `ITEM_EXTRA_WAND` | Wand extra flag |
| 1073741824 | `ITEM_FIST` | Fist weapon — required for `value3 = 0` weapons |
| 2147483648 | `ITEM_TWO_HANDED` | Two-handed weapon |

**Rejection rules:**
- `ITEM_GENERATED` (bit 1) set → rejection.
- `ITEM_LIFESTEALER` set without `ITEM_ANTI_GOOD` also set → rejection.
- Any object that can drop from a boss mob (referenced in a boss mob's loot table) without `ITEM_BOSS` → rejection.
- Any object referenced in any mob loot table (`l`/`L` lines) without `ITEM_LOOT` → rejection.
- Any undefined bit → rejection.

---

## 6. `wear_flags` Bitvector

Third integer on the header flags line. Defined in `src/buildtab.c` (`tab_wear_flags`):

| Bit | Name | Slot |
|----:|------|------|
| 1 | `halo` | Halo slot |
| 2 | `aura` | Aura slot |
| 4 | `horns` | Horns slot |
| 8 | `head` | Head slot |
| 16 | `face` | Face slot |
| 32 | `beak` | Beak slot |
| 64 | `ear` | Ear slot |
| 128 | `neck` | Neck slot |
| 256 | `wings` | Wings slot |
| 512 | `shoulders` | Shoulders slot |
| 1024 | `arms` | Arms slot |
| 2048 | `wrist` | Wrist slot |
| 4096 | `hands` | Hands slot |
| 8192 | `finger` | Finger slot |
| 16384 | `claws` | Claws slot |
| 32768 | `hold` | Held in hand slot |
| 65536 | `about` | About body (cloak) slot |
| 131072 | `waist` | Waist slot |
| 262144 | `body` | Body slot |
| 524288 | `tail` | Tail slot |
| 1048576 | `legs` | Legs slot |
| 2097152 | `feet` | Feet slot |
| 4194304 | `hooves` | Hooves slot |
| 8388608 | `take` | Takeable (required on all objects) |
| 16777216 | `clan_colors` | **Runtime-only — must not be authored → rejection** |
| 33554432 | `invasion_emblem` | **Runtime-only — must not be authored → rejection** |

**Rejection rules:**
- `take` (8388608) not set → rejection. Every authored object must be takeable.
- `clan_colors` (16777216) set → rejection.
- `invasion_emblem` (33554432) set → rejection.
- `item_type = ITEM_WEAPON` (5) without both `hold` and `take` → rejection.
- Object `name`, `short_descr`, and `description` must be thematically consistent with the non-`take` wear flags (e.g., a `head` item must read as headgear) → rejection if flags and text are clearly inconsistent.
- Any undefined bit → rejection.

**Archetype conventions (enforced as hard requirements):**
- Shield: `item_type = 9` (armor), wear flags include `hold` → must not have `ITEM_TWO_HANDED`.
- Weapon: `item_type = 5`, wear flags include `hold` and `take` → rejection if either missing.
- Buckler: `item_type = 9`, wear flags include `hold`, `extra_flags` includes `ITEM_BUCKLER`.
- Two-handed weapon: `item_type = 5`, wear flags include `hold`, `extra_flags` includes `ITEM_TWO_HANDED`. If `name`, `short_descr`, or `description` clearly indicates a two-handed archetype (e.g., "great axe", "halberd", "greatsword"), `ITEM_TWO_HANDED` must be set → rejection otherwise.
- Fist weapon: `item_type = 5`, wear flags include `hold`, `extra_flags` includes `ITEM_FIST`. If `value3 = 0` (`hit`), `ITEM_FIST` must be set → rejection otherwise.

---

## 7. `item_apply` Bitvector

Fourth integer on the header flags line. Defined in `src/buildtab.c` (`tab_item_apply`):

| Bit | Name | Effect |
|----:|------|--------|
| 1 | `nada` | No apply effect (default) |
| 2 | `infra` | Infravision |
| 4 | `invis` | Invisibility |
| 8 | `det_invis` | Detect invisibility |
| 16 | `sanc` | Sanctuary |
| 32 | `sneak` | Sneak |
| 64 | `hide` | Hide |
| 128 | `prot` | Protection |
| 256 | `enhanced` | Enhanced combat abilities |
| 512 | `det_mag` | Detect magic |
| 1024 | `det_hid` | Detect hidden |
| 2048 | `det_evil` | Detect evil |
| 4096 | `pass_door` | Pass door |
| 8192 | `det_poison` | Detect poison |
| 16384 | `fly` | Flying |
| 32768 | `know_align` | Know alignment |
| 65536 | `detect_undead` | Detect undead |
| 131072 | `heated` | Heated (environmental protection) |

- `nada` (bit 1) is the baseline. It is set on newly created objects with no special apply effects.
- Multiple effects can be combined with bitwise OR.
- Any undefined bit → rejection.

---

## 8. Value Array

```
<value0> <value1> <value2> <value3> <value4> <value5> <value6> <value7> <value8> <value9>
```

Exactly 10 integers. Fewer or more → rejection.

Value semantics depend on `item_type`:

### Weapons (`item_type = 5`)

- `value3`: weapon attack type. See §9 for allowed values.
- Other values: available for runtime use; use `0` unless otherwise required.

### Boards (`item_type = 27`)

- `value[0]`: expiry time in days. Must be `>= 1` → rejection.
- `value[1]`: minimum read level. Must be `0–105` → rejection.
- `value[2]`: minimum write level. Must be `0–105` → rejection.
- `value[3]`: board vnum. Must be a unique non-zero integer. Must not conflict with any existing board vnum in the database → rejection.

### Pieces (`item_type = 29`)

- `value0`: previous-piece vnum. `0` if this is the first piece.
- `value1`: next-piece vnum. `0` if this is the last piece.
- `value2`: replacement object vnum created when two pieces connect. Must reference an object in this file or in the database → rejection if non-zero and not found.
- At runtime: two pieces connect when `pieceA.value0 == pieceB.vnum` or `pieceA.value1 == pieceB.vnum`.

### All other item types

- Use `0` for unused value slots.
- Area-authored items must not define fixed stat values in the value array. Stats are generated at runtime from item level and runtime parameters → rejection if non-zero stat values are detected in value slots reserved for runtime use.

---

## 9. Weight

Single integer after the value array.

- Must be `1–15` → rejection outside range.
- Weight encodes item archetype (required policy):
  - `1–5`: caster item.
  - `6–10`: melee item.
  - `11–15`: tank item.
- Weight must be consistent with the object's identity and wear slot. A head piece with weight `1` is a caster helmet. A weapon with weight `6` is a melee weapon. → rejection if weight archetype conflicts with the object's explicit `name`/`short_descr` identity (e.g., a "great sword" with weight `1` is a rejection).

---

## 10. Weapon Attack Type (`value3` for `item_type = 5`)

Defined in `src/buildtab.c` (`tab_weapon_types`):

| Value | Name |
|------:|------|
| 0 | `hit` — **only valid when `ITEM_FIST` is set → rejection otherwise** |
| 1 | `slice` |
| 2 | `stab` |
| 3 | `slash` |
| 4 | `whip` |
| 5 | `claw` |
| 6 | `blast` |
| 7 | `pound` |
| 8 | `crush` |
| 9 | `rend` |
| 10 | `bite` |
| 11 | `pierce` |
| 12 | `drain` |
| 13 | `sear` |

Value outside 0–13 → rejection.
`value3 = 0` without `ITEM_FIST` in `extra_flags` → rejection.
`value3` must be thematically consistent with the weapon's `name`, `short_descr`, and `description` — a sword should be `slice` or `slash`, not `bite` → rejection if clearly inconsistent.

---

## 11. Board Object (`item_type = 27`)

Board objects require a corresponding entry in the `boards` database table (vnum = `value[3]`). The ingestion validator:
- Verifies `value[3]` is non-zero and unique → rejection.
- Creates the `boards` table row as part of the DB transaction.

`value[3]` must not be `0` → rejection.

---

## 12. Piece Object (`item_type = 29`)

- `value0` and `value1` form a chain. At least one must be non-zero (a piece with no connections is meaningless) → rejection if both `value0` and `value1` are zero.
- `value2` (replacement vnum) must reference an object in this file or the database → rejection if non-zero and not found.
- All pieces in a chain must be defined in the same area file or already in the database. A partial chain (referencing a piece in a file not yet ingested) → rejection.

---

## 13. Trailing Entries

### 13.1. `A` — Object Affect

```
A <location> <modifier>
```

- `A` on a line, followed by two integers. Integers may be on the same line as `A` or on the following line. Both formats are accepted by the loader and validator.
- `location`: affect location integer. Must be a valid `APPLY_*` value (see §14) → rejection.
- `modifier`: signed integer. Must be `0` (use `A` entries without the runtime stat system adding the modify is a no-op) → rejection if non-zero. **Area files must not define fixed stat modifiers** — all stats are runtime-generated.

### 13.2. `E` — Extra Description

```
E
<keyword>~
<description>~
```

- `E` on its own line.
- `<keyword>`: tilde-terminated, non-empty → rejection.
- `<description>`: tilde-terminated. Must end with exactly one `\n` before `~` → rejection.
- Extra descriptions are shown when a player examines the keyword.

### 13.3. `L` — Level

```
L
<level>
```

- `L` on its own line.
- `<level>`: single integer on the next line.
- Must be `1–105` → rejection outside range.
- This sets the object's effective level for runtime stat generation.

---

## 14. `APPLY_*` Location Values (for `A` trailing entries)

| Value | Name |
|------:|------|
| 0 | `APPLY_NONE` |
| 1 | `APPLY_STR` |
| 2 | `APPLY_DEX` |
| 3 | `APPLY_INT` |
| 4 | `APPLY_WIS` |
| 5 | `APPLY_CON` |
| 6 | `APPLY_SEX` |
| 7 | `APPLY_CLASS` |
| 8 | `APPLY_LEVEL` |
| 9 | `APPLY_AGE` |
| 10 | `APPLY_HEIGHT` |
| 11 | `APPLY_WEIGHT_STAT` |
| 12 | `APPLY_MANA` |
| 13 | `APPLY_HIT` |
| 14 | `APPLY_MOVE` |
| 17 | `APPLY_AC` |
| 18 | `APPLY_HITROLL` |
| 19 | `APPLY_DAMROLL` |
| 21 | `APPLY_SAVES` |

Value not in this list → rejection.
