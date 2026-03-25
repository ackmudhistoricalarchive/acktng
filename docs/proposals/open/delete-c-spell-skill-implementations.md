# Proposal: Delete C Spell/Skill Implementations After Lua Migration

## Problem

Phase 3 of the Lua migration has translated all 338 C spell/skill functions to Lua
scripts stored in `skills.script_source`. The dispatch layer in `magic.c` and
`interp.c` already prefers Lua over C when a script exists. This means the C
implementations are now dead code — they compile, link, and occupy binary space,
but never execute for any skill that has a Lua script.

Leaving them in place creates two risks:
1. Confusion about where the authoritative logic lives (C or Lua?)
2. Temptation to fix a bug in the C file that will never run

## Approach

The deletion has three parts:

### Part 1 — Replace spell_fun pointers with spell_null

`skill_table_data.c` contains a function pointer for every spell. Those pointers
still reference the C implementations (e.g. `spell_fireball`, `spell_charm_person`).
Deleting the `.c` files without touching the table would break the build.

For every spell entry whose Lua script is confirmed working, replace the function
pointer with `spell_null`. `spell_null` is the existing no-op stub already used for
passive/non-castable skills.

The migration script (`tools/migrate_phase3.py`) already has the mapping from
function name → sn, so a short companion script can generate the needed sed/patch
commands automatically.

### Part 2 — Remove the C do_* command handlers

Skills dispatched via `interp.c` are handled differently. The command table maps
typed commands (e.g. "bash") to a C `do_fun` pointer. `interp.c` checks
`skill_scripts[sn]` first and calls `lua_skill_execute` if a script exists, so the
C handler is already bypassed. Once we delete the `.c` file we need a placeholder
in the command table — use the existing `do_nothing` stub (or add one if absent).

For the handful of skills whose command handler is registered under a different name
than their Lua script (e.g. `do_pick` / "pick lock"), the same replacement applies.

### Part 3 — Delete the source files and update the Makefile

Delete the 338 `.c` files from `src/spells/` and `src/skills/` that have confirmed
working Lua scripts. The 14 files with no DB entry (orphaned implementations) can
be deleted at the same time since they are also unreachable.

The `Makefile` lists object files in `O_FILES`. Remove all the deleted files'
corresponding `.o` entries. This will require touching the Makefile carefully — the
`O_FILES` variable is defined twice (the second definition wins) so only the second
definition needs editing.

## Confirmation Criteria ("working implementation")

A Lua script is considered confirmed working when:
- The server boots with the script loaded (no Lua compile error in the log)
- The spell/skill executes at least once in-game without a Lua runtime error
- The observable effect matches the old C behaviour (damage, affects, messages)

For the initial deletion we do not need 100% in-game coverage of every spell.
The practical approach:
1. Merge PR #984 and boot the server
2. Monitor `log/server.log` for Lua errors over a normal play session
3. Fix any broken scripts (update `script_source` in DB, no recompile needed)
4. Once the error rate is zero or negligible, proceed with deletion

## What Stays

- `spell_table_data.c` and `skill_table_data.c` — these are the skill/spell
  metadata tables (levels, targets, mana costs, etc.) and are not being deleted
- `spell_null.c` — the no-op spell stub; still needed as the replacement pointer
- The fallback logic in `magic.c` and `interp.c` (`skill_scripts[sn]` check) —
  leave this in place so future skills can be added in either C or Lua
- The 14 orphaned C files that have no DB entry — delete alongside the rest

## Trade-offs

| | Keep C | Delete C |
|---|---|---|
| Authoritative source | Ambiguous | Clear (DB) |
| Bug fixes | Require recompile | DB update only |
| Rollback path | Always available | Must restore from git |
| Binary size | Larger | Smaller |
| Build time | Slower | Faster |

The only meaningful downside is loss of the in-binary rollback path. Since the C
code is in git, this is acceptable — reverting a broken Lua script means updating
the DB, not doing a git revert.

## Files Affected

- `src/spells/spell_*.c` — 246 files (all except `spell_null.c`, `spell_table_data.c`)
- `src/skills/do_*.c` — 110 files (all)
- `src/spells/spell_table_data.c` — edited in place (spell_fun pointers → spell_null)
- `src/skills/skill_table_data.c` — no change (skills use gsn pointers, not do_fun)
- `src/Makefile` — remove deleted files from O_FILES

## Implementation Order

1. Boot test (prerequisite — not a code change)
2. Write `tools/patch_spell_table.py` to replace live spell_fun pointers with spell_null
3. Apply the spell table patch, build, lint, test
4. Delete C source files, update Makefile, build, lint, test
5. Commit and open PR
