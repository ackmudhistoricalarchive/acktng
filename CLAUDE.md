# CLAUDE.md — ACK!MUD TNG 4.3.1

## Design Proposal Requirement

For any task that is not a bugfix, you MUST first deliver a design proposal describing the proposed changes — including the problem, approach, affected files, and any trade-offs — and discuss it with the user. Do NOT begin implementation until the user has explicitly signed off on the proposal. No code changes, no file creation, no prototyping, and no database writes until approval is given.

Querying databases for information to research or write a proposal is permitted. Writing to any database (INSERT, UPDATE, DELETE, or schema changes) is implementation and requires an approved proposal first.

Bugfixes do NOT require a proposal and may be implemented directly.

Proposals live in `docs/proposals/` within this repository:
- `docs/proposals/open/` — active proposals pending discussion or implementation
- `docs/proposals/completed/` — proposals that have been fully implemented
- `docs/proposals/rejected/` — proposals that were rejected

When writing any document (design proposals, lore files, area plans, etc.), write and deliver it in sections rather than producing the entire document at once. Continue through all sections without waiting for confirmation between them, unless the user asks for a different approach.

## Project Overview

ACK!TNG is a MUD (Multi-User Dungeon) game server written in C, descended from Diku → Merc → ACK! lineage. The server binary is called `ack` and runs from the `area/` directory, accepting telnet connections on a configurable port.

## Game World Lore

When you need information about the game world (history, factions, geography, lore, etc.), search `docs/lore/` — it is the canonical source for all world-building documentation. The directories `lore/` and `data/knowledge/` are generated from `docs/lore/` and contain no additional information.

## Specifications and Documentation

The `docs/` directory is the canonical source for all game specifications (area files, objects, mobs, rooms, quests, help files, data structures, etc.). You should never need to search `src/` to understand a specification — everything should be documented in `docs/`. If you find that a specification detail is missing from `docs/` and you had to look in `src/` to determine the answer, that information must be added to the appropriate `docs/` specification file before the task is complete.

## Build System

All build commands run from the `src/` directory:

```sh
cd src
make ack          # Build the server binary
make clean        # Remove object files and binary
make unit-tests   # Build and run all unit tests, then integration tests
make integration-test  # Build server + run integration test (boot, login, crash check)
make all          # Incremental: build changed files, run affected tests, integration test if anything changed (no-op if nothing changed)
make lint         # Check all source files match the .clang-format style (fails if any differ)
make format       # Auto-apply .clang-format style to all source files
```

- Compiler: GCC with `-O -g2 -Wall -DACK_43`
- Always links: `-lcrypt -lpthread -lz -lpq`
- Required: libpq (`libpq-dev`) — PostgreSQL client library for all database operations. The build will fail if libpq is not found.
- Optional (auto-detected via pkg-config):
  - OpenSSL (`-DHAVE_OPENSSL`) — enables TLS telnet and WSS support
- Every `.c` file depends on `ack.h` (the central header)
- The binary is built as `src/ack`; it runs from `area/` (e.g., `cd area && ../src/ack 4000`)

### Build Dependencies (Debian/Ubuntu)

```sh
apt-get install -y build-essential libcrypt-dev zlib1g-dev libssl-dev \
    pkg-config libpq-dev postgresql postgresql-client clang-format python3
```

All dependencies are installed automatically by the parent `aicli` project's `setup.sh`.

## Help, Shelp, and Lore System

Help entries, staff help (shelp), and lore are stored in and served from the PostgreSQL database. The server queries the database on demand via `db_help_lookup()` (in `src/db/db_help.c`). The schema is in `area/schema.sql`.

There are no flat `help/` or `shelp/` directories — all content lives in the database. The login greeting is also served from the database (`greeting1` through `greeting6` entries in `help_entries`).

## Testing

### Unit Tests

Each test is a standalone binary built from a `test_*.c` file paired with the module under test. The pattern:

1. **Test file**: `test_foo.c` — contains `main()`, test functions, and test doubles (stubs/mocks)
2. **Module under test**: `foo.unit-test.o` — compiled from `foo.c` with a `UNIT_TEST_FOO` define (when needed) to stub out dependencies
3. **Shared helper**: `test_is_fighting.o` — provides `is_fighting()` stub, linked into every test binary
4. **Linker trick**: `-ffunction-sections -fdata-sections` + `-Wl,--gc-sections` to dead-strip unused symbols that would cause link errors

To add a new unit test:
1. Create `src/tests/test_foo.c` with `main()` and test functions using `assert()`
2. Add Makefile rules for `foo.unit-test.o` and `unit-test-foo` target (binary goes to `tests/unit-test-foo`)
3. Add the target to the `unit-tests` dependency list and add `./tests/unit-test-foo` to the run list

Test conventions:
- Tests use `assert()` from `<assert.h>` — no test framework
- Test doubles stub external functions directly in the test file
- Use `#define DEC_GLOBALS_H 1` before `#include "ack.h"` to skip globals when they cause link issues
- Helper functions like `clear_character()` zero-initialize structs with `memset()`

### Integration Tests

`integration-tests.sh` (runs from the repo root) requires a running PostgreSQL server:
- Creates an ephemeral test database from `fixtures/test_data.sql`
- Builds the server and boots it on random ephemeral ports
- Runs four tests in parallel: WebSocket, telnet, TLS telnet, and WSS login flows
- Each test walks the full new-player login flow and validates existing-player login
- Tears down the test database on exit

`seed-test-player.py` creates pre-existing player files for the integration tests. It uses `ctypes` to call `libcrypt.so` directly (the Python `crypt` module was removed in Python 3.13).

### Running Tests

Always validate changes with:
```sh
cd src && make unit-tests
```

This runs all unit tests and all integration tests. The CI workflow runs this same command on every open PR.

All tests must be run locally. Never run tests on remote systems or trigger remote CI — always validate locally before pushing.

**Unit tests should always be written for changes where possible.** When modifying or adding functionality, add a corresponding unit test in `src/tests/` to cover the new or changed behavior.

## Branch and PR Policy

**NEVER push directly to main. All changes must go through a branch and pull request — no exceptions.** Always create a feature/fix branch, push it, and open a PR. This applies to all changes including documentation, area files, and bugfixes.

**A PR may NEVER be merged unless all GitHub CI checks pass.** If CI fails, fix the failure before merging. No overrides, no exceptions.

## Pre-Push Requirements

**Before committing or pushing any change, ALL of the following must pass — no exceptions:**

```sh
cd src
make lint         # Code must be correctly formatted (fails if any file differs from .clang-format)
make ack          # Build must succeed with zero errors
make unit-tests   # All unit tests and all integration tests must pass
```

Run these in order. Do not push if any step fails. Fix the failure first.

- A build that does not compile is never acceptable.
- Code that does not pass `make lint` must be reformatted with `make format` before pushing.
- Changes that break any unit test or integration test must not be pushed until fixed.

The CI workflow enforces these same checks on every PR. A PR will not be merged if any check fails.

**All lint issues must be fixed before pushing** — not just those in files you changed. If `make lint` fails on any file, run `make format` to fix the entire codebase, then verify `make lint` passes cleanly. Do not push while any lint error exists regardless of which file contains it.

`make lint` and `make format` use `clang-format-18` explicitly, matching the CI environment (Ubuntu 24.04). Do not use the bare `clang-format` command for formatting, as the default version on Debian/other systems may produce different output.

## Repository Structure

```
acktng/
├── src/              # C source code and Makefile
│   ├── ack.h         # Central header (includes typedefs.h, config.h, globals.h, etc.)
│   ├── typedefs.h    # Type definitions, structs, constants, macros
│   ├── config.h      # MUD configuration (name, limits, thresholds)
│   ├── globals.h     # Global variable declarations
│   ├── comm.c        # Network/connection handling, main game loop
│   ├── db.c          # Database loading (areas, objects, mobs, rooms)
│   ├── db/           # PostgreSQL database layer (connection, help queries, worker)
│   ├── handler.c     # Object/character handler utilities
│   ├── fight.c       # Combat system
│   ├── magic.c       # Spell system
│   ├── skills.c      # Skills system (skills_chi.c, skills_combo.c, etc.)
│   ├── skills/       # Individual skill implementations (do_*.c)
│   ├── spells/       # Individual spell implementations (spell_*.c)
│   ├── act_*.c       # Player command handlers (info, comm, move, obj, mob, wiz, clan)
│   ├── save/         # Player/area/object save/load
│   ├── update.c      # Periodic game tick updates
│   ├── socket.c      # Socket handling, TLS, WebSocket, telnet negotiation
│   ├── login.c       # Login flow and character creation
│   ├── invasion.c    # Mob invasion event system
│   ├── keep.c        # Player keep/chest system
│   ├── quests/       # Quest system (crusade, cartography, templates)
│   ├── ai/           # NPC AI special procedures
│   ├── tests/        # Unit test files
│   │   ├── test_*.c  # Test source files (one per module under test)
│   │   └── test_is_fighting.c  # Shared stub linked into most test binaries
│   ├── tools/        # DB migration/import utilities
│   └── Makefile      # Build rules
├── area/             # Area data files (~52 .are files) + runtime data
│   ├── area.lst      # Master list of areas to load at boot
│   ├── schema.sql    # PostgreSQL database schema
│   └── *.are         # Individual area files (rooms, mobs, objects, resets)
├── fixtures/         # Test fixture data (test_data.sql for integration tests)
├── data/             # Runtime data files (bans, clans, socials, rulers, etc.)
├── docs/             # Documentation (area file spec, data structures, lore, proposals)
├── player/           # Legacy player save directory (DB-backed since schema v8)
├── log/              # Server log directory
├── reports/          # Report files
├── web/              # Game-generated web data output (see web/README.md)
├── integration-tests.sh   # Integration test runner (PostgreSQL, 4 parallel tests)
├── integration-test.sh    # WebSocket login test
├── integration-test-telnet.sh     # Telnet login test
├── integration-test-telnet-tls.sh # TLS telnet login test
├── integration-test-wss.sh        # WSS login test
├── seed-test-player.py    # Creates test player files for integration tests
└── .github/workflows/     # CI: validate-open-prs.yml
```

## Key Data Structures

Defined in `typedefs.h` and `ack.h`:

- **CHAR_DATA** — Character (player or NPC). Key fields: `name`, `level`, `hit`/`max_hit`, `mana`/`max_mana`, `gold`, `pcdata` (NULL for NPCs), `in_room`, `fighting`, `position`
- **PC_DATA** — Player-specific data (within CHAR_DATA). Fields: `pwd` (password hash), `learned[]` (skills), `quest_points`
- **OBJ_DATA** — In-game object instance. Fields: `name`, `item_type`, `value[]`, `wear_loc`
- **ROOM_INDEX_DATA** — Room definition. Fields: `vnum`, `name`, `description`, `exit[]`, `people` (linked list)
- **MOB_INDEX_DATA** / **OBJ_INDEX_DATA** — Templates for mobs/objects loaded from area files

## Currency

The game uses a single currency: `int gold` on CHAR_DATA. There is no multi-currency system.

## Code Conventions

- C89/C99 style with some GCC extensions
- `sh_int` = `short int`, `bool` = custom boolean type (TRUE/FALSE macros)
- String functions: `str_cmp()`, `str_prefix()`, `str_infix()` (case-insensitive, defined in `strfuns.c`)
- Memory: custom allocator via `ssm.c` (shared string manager). Use `str_dup()` / `free_string()` for strings
- Linked lists: manual next/prev pointers with LINK/UNLINK macros (defined in `lists.h`)
- Player output: `send_to_char()`, `act()` with format codes (`$n`, `$N`, `$p`, etc.)
- Color codes: `@@r` (red), `@@g` (green), `@@l` (blue), `@@N` (reset), etc.
- Wear locations: `WEAR_HOLD_HAND_L`, `WEAR_HOLD_HAND_R`, `WEAR_TWO_HANDED`, `WEAR_BUCKLER`, etc.
- Guard macros for headers: `#define DEC_HEADERNAME_H 1` / `#ifndef DEC_HEADERNAME_H`

## CI/CD

The GitHub Actions workflow `.github/workflows/validate-open-prs.yml`:
- Runs hourly and on manual dispatch
- For each open PR: checks out the PR head, runs `make unit-tests` from `src/`
- Auto-approves PRs where tests pass; requests changes where tests fail
- Branch protection should require this check to pass

## Spells, Skills, and Commands

Whenever a new spell or skill is added, a corresponding detailed help entry MUST be added to the database. The help file should cover:
- What the spell/skill does
- How to use it (syntax, targets, etc.)
- Any relevant mechanics (damage, duration, cooldown, mana cost, etc.)
- Class/level availability if applicable

Whenever a new player command is added, a corresponding detailed help entry MUST be added to the database. The help file should cover:
- What the command does
- Full syntax and all options/arguments
- Examples of usage
- Any restrictions (level, class, position, etc.)

Likewise, whenever a spell, skill, or command is removed, its help entry MUST also be removed from the database.

These are hard requirements — no spell, skill, or command addition is complete without its help entry, and no removal is complete without also removing the corresponding help entry.

## Common Pitfalls

- The server binary must run from the `area/` directory (relative paths to area files, data, log dirs)
- `ack.h` is the single include for most `.c` files — it pulls in typedefs, config, globals, lists, strfuns
- When writing unit tests, use `#define DEC_GLOBALS_H 1` before including `ack.h` to avoid link errors from global arrays
- The Makefile defines `O_FILES` twice (second definition wins) — be aware when adding new source files
- Area files use a custom text format with tilde (`~`) delimiters — see `docs/area_file_spec.md`
- libpq is required — the build fails without it. OpenSSL is auto-detected; if TLS features aren't working, check that `libssl-dev` is installed
- The server refuses to boot if the PostgreSQL database connection fails (check `data/db.conf`)
- Integration tests require a running PostgreSQL server with peer auth for the postgres user
