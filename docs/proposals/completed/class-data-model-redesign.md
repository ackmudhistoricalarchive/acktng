# Class Data Model Redesign

**Status:** Pending approval
**Scope:** acktng only
**Type:** Refactor

---

## Problem

The current class system uses a single flat array, `class_level[MAX_TOTAL_CLASS]`, indexed by class constants (0-28). This design has several problems:

- **No explicit class selection record.** Character creation sets the prime class via `ch->class` and `ch->class_level[prime] = 1`, then sets all other mortal class slots to `0`. Nothing records which 4 mortal classes a player chose; any of the 8 mortal class slots can be gained into.
- **Overloaded sentinel value.** `-1` means "not chosen" for mortal slots but "not yet unlocked" for remort/adept slots. These are semantically different states using the same value.
- **No ordering beyond prime.** `ch->class` records the prime (first) class, but the ordering of classes 2-4 is not stored anywhere.
- **Non-contiguous class indices.** Mortal classes are at indices 0-5, 24, and 28. Remort classes are at 6-17, 25-26. Adept at 18-23, 27. Loops must either hard-code these ranges or call `IS_MORTAL_CLASS()` over the full 29-slot array.
- **Wasted slots.** `class_level[MAX_TOTAL_CLASS]` allocates 29 slots per character regardless of how many are used.

---

## Proposed Design

Replace `class_level[MAX_TOTAL_CLASS]` and `sh_int class` with explicit, typed fields:

```c
/* In CHAR_DATA */
int mortal_class[4];   /* class index (into gclass_table) for each chosen mortal class,
                          in order (mortal_class[0] is prime). -1 = unused slot.       */
int mortal_level[4];   /* level in mortal_class[i]. 0 = no levels yet.                */
int remort_class[2];   /* class index for each remort class. -1 = unused.             */
int remort_level[2];   /* level in remort_class[i].                                   */
int adept_class;       /* class index for the adept class. -1 = none.                 */
int adept_level;       /* level in the adept class. (field already exists; keep it)   */
```

`ch->class` is replaced by `ch->mortal_class[0]` (the prime class). The old `class_level[]` array and `sh_int class` field are removed.

### What this gives us

- **Explicit class identity.** Which 4 mortal classes a player chose is stored directly, not inferred from which slots are non-negative.
- **Explicit ordering.** Index 0 is prime, 1 is second, etc. No ambiguity.
- **No sentinel overloading.** A slot that says `-1` in `mortal_class[i]` means "this slot is empty," not "you haven't leveled yet." `mortal_level[i]` starts at 0 and grows from there.
- **Natural enforcement.** The 4-class limit is enforced by struct size. `do_gain` just checks whether the requested mortal class appears in `mortal_class[0..3]`.
- **Cleaner loops.** Code iterates `for (i = 0; i < 4; i++)` over mortal classes rather than over a 29-slot array with `IS_MORTAL_CLASS()` guards.

### Unchanged

- `adept_level` already exists as a standalone field and is kept.
- `reincarnations[]` is not changed by this proposal; it can be addressed separately.
- The `gclass_table` and class index constants (`CLASS_MAG`, etc.) are unchanged.

---

## Affected Files

### Core struct and headers

- `src/headers/ack.h` - CHAR_DATA struct fields
- `src/headers/config.h` - can remove some now-unneeded macros (e.g. `CLASS_SOR` offset arithmetic in loops)

### Character creation and login

- `src/login.c` - creation flow must collect all 4 mortal classes and store them in `mortal_class[0..3]`

### Save and load

- `src/save/save_players.c` - write and read the new fields; add migration path for old format (see Migration section)

### Game logic (large surface area)

The following files access `class_level[]` and will need updating. Most usages fall into a small number of patterns that can be addressed with helper functions:

- `src/act_info.c` - `do_gain`, `do_worth`, `do_score`, etc.
- `src/update.c` - `advance_level`, `advance_level_remort`, `advance_level_adept`
- `src/macros.c` - macro-driven level advancement
- `src/handler.c` - `get_pseudo_level`, class utilities
- `src/damage.c`, `src/fight.c`, `src/magic.c`, `src/spell_dam.c`
- `src/reincarnate.c`, `src/death.c`
- `src/act_wiz.c` - `setclass` command
- `src/login.c` - old-player load path
- `src/hotreboot.c`
- ~80 skill and spell files that read `ch->class_level[CLASS_XYZ]` to determine scaling

### Tests

- `src/tests/test_act_info.c` and other test files that stub or set `class_level[]`

---

## Helper Functions

To avoid touching 94 files individually, the implementation will introduce helper functions in `handler.c` that translate between the new fields and the common lookup patterns:

```c
/* Return the level a character has in a given class index, or -1 if not chosen. */
int char_class_level(const CHAR_DATA *ch, int class_idx);

/* Return true if the character has the given class as one of their mortal classes. */
bool char_has_mortal_class(const CHAR_DATA *ch, int class_idx);

/* Return true if the character has the given class as one of their remort classes. */
bool char_has_remort_class(const CHAR_DATA *ch, int class_idx);
```

Skill/spell files that currently do `ch->class_level[CLASS_KNI]` would call `char_class_level(ch, CLASS_KNI)`. This limits the blast radius of the struct change.

---

## Character Creation Changes

The `CON_GET_NEW_CLASS` state currently collects one class and sets `ch->class`. It would be extended to a multi-step flow:

1. Player enters 4 mortal class names in order (prime first), e.g. `psi mag cle cip`
2. The parser validates each against `IS_MORTAL_CLASS()` and checks for duplicates
3. `mortal_class[0..3]` is set to the chosen class indices
4. `mortal_level[0] = 1` (prime starts at level 1), `mortal_level[1..3] = 0`
5. `remort_class[0..1] = -1`, `remort_level[0..1] = 0`, `adept_class = -1`, `adept_level = 0`

---

## Migration

Existing players are saved in flat text files (and DB-backed since schema v8). The save format will be versioned via the existing `Revision` field. On load, if the save revision is older than the new format:

1. Read the old `m/c`, `Remort`, `Adept`, `Druidlevels` lines into a temporary `class_level[]` array.
2. Derive `mortal_class[]`: collect mortal class indices where `class_level[i] >= 0`, ordered by their level descending (highest level = earliest pick). Place `ch->class` (old prime) first regardless of level.
3. Derive `remort_class[]`: collect the two remort class indices with `class_level[i] > 0`.
4. Derive `adept_class`: the adept class index with `class_level[i] > 0`, if any.
5. Write the character in the new format immediately on next save.

This is a best-effort migration; ordering of classes 2-4 for existing characters cannot be perfectly reconstructed and will be approximated by level.

---

## Trade-offs

| | Current | Proposed |
|---|---|---|
| Mortal class ordering stored | No (only prime via `ch->class`) | Yes (all 4, in order) |
| "Not chosen" representation | -1 in class_level | Absent from mortal_class[] |
| Code to look up class level | `ch->class_level[CLASS_X]` | `char_class_level(ch, CLASS_X)` |
| Iteration over mortal classes | Loop 0..28 with IS_MORTAL_CLASS | Loop 0..3 |
| Files touched | N/A | ~94 (via helper functions, most are mechanical) |
| Migration complexity | N/A | Medium (ordering approximated for existing chars) |

---

## Open Questions

1. Should `reincarnations[]` be redesigned in parallel to match the new layout (4 mortal, 2 remort, 1 adept), or deferred?
2. For the Sentinel lineage: Sentinel (CLASS_SEN) has no remort variants. Is a Sentinel-prime character expected to still pick 4 total mortal classes, one of which is Sentinel?
3. The `setclass` wiz command currently sets `class_level[class]` directly. Should it be updated to operate on the new fields, or is a staff-only escape hatch acceptable?
