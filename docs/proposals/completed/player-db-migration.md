# Proposal: Migrate Player Files to PostgreSQL

## Status: Approved

## Problem

Player character data is the last remaining flat-file data store. Every other game
data source — areas, rooms, mobs, objects, help, quests, bans, clans, boards, corpses,
etc. — has been migrated to PostgreSQL. Player files remain in `player/<letter>/<Name>`
text files, creating:

- No transactional saves — a crash mid-write can corrupt a player file (the temp+rename
  pattern mitigates but does not eliminate this)
- No access from `tngdb` API or web tools without the game server running
- Can't query player data (leaderboards, character info) without parsing flat files
- Backup requires filesystem snapshots of the `player/` directory tree
- Flat files are the last thing preventing a fully DB-backed server

The DB infrastructure is already partially built: a `players` table exists with
`raw_save TEXT` and `pwd_hash TEXT` columns, an async worker thread (`db_worker.c`)
has `DB_WRITE_PLAYER` and `DB_READ_PLAYER` operations with coalescing, and
`CON_LOADING_FROM_DB` is a defined connection state. The save/load paths just aren't
wired up yet.

## Approach

Three phases, each independently deployable and testable.

### Phase 1 — Dual-write: save to DB alongside flat files

**`src/save/save_players.c` — `save_char_obj`**

Add an `#ifdef HAVE_LIBPQ` block after the flat-file write. Use `open_memstream` to
serialize to an in-memory buffer (calling the same `fwrite_char` + `fwrite_obj` +
`#END` sequence), then call `db_worker_enqueue_write(DB_WRITE_PLAYER, raw, rawlen,
ch->name)`. The flat file write stays in place during this phase.

**`src/db/db_worker.c` — `handle_write_player`**

Complete the function: parse the `Password` line from the raw_save text to extract the
actual `pwd_hash`, then upsert `players (name, pwd_hash, raw_save)` with real values
instead of the current empty-string placeholder.

### Phase 2 — Load from DB (async path at login)

The async infrastructure is already in place but incomplete. `handle_read_player`
fetches the row but posts `NULL ch` back. `db_worker_poll_results` has the state-machine
skeleton but never hydrates a character.

**`src/save/save_players.c` — extract `alloc_char_for_login`**

Factor the CHAR_DATA allocation and initialisation block out of `load_char_obj` into
a new function `alloc_char_for_login(DESCRIPTOR_DATA *d, const char *name)`. This
sets `d->character` and initialises all PC_DATA fields. Callable from both the
synchronous path (`load_char_obj`) and the async path (`db_worker_poll_results`).

**`src/db/db_worker.h` — extend `DB_PLAYER_RESULT`**

Add `char *raw_save` (heap-allocated text, NULL for new player) and `int found` fields.

**`src/db/db_worker.c` — complete `handle_read_player`**

On row found: `strdup` the `raw_save` column text into `r->raw_save`, set `r->found = 1`.
On row not found: post result with `raw_save = NULL`, `found = 0`.

**`src/db/db_worker.c` — complete `db_worker_poll_results`**

When `r->found && r->raw_save != NULL`:
1. Call `alloc_char_for_login(r->d, r->d->character->name)` — allocates/inits the char
2. Open `fmemopen` on `r->raw_save` and run the existing `fread_char` / `fread_obj`
   parse loop (same code as in `load_char_obj`)
3. Run post-load checks: PLR_DENY, CONFIG_JUSTIFY, `check_reconnect`, wizlock,
   deathmatch, `check_playing` — extracted from `nanny()` into
   `finish_player_login(DESCRIPTOR_DATA *d, bool fOld)`
4. Advance to `CON_GET_OLD_PASSWORD` (existing) or close on deny/lock

When `!r->found`:
- New player: call `alloc_char_for_login`, run new-player ban check, advance to
  `CON_CONFIRM_NEW_NAME`

**`src/login.c` — `nanny()` at `CON_GET_NAME`**

Replace the synchronous `fOld = load_char_obj(d, argument, FALSE)` call + post-load
block with a call to `alloc_char_for_login(d, argument)` followed by
`db_worker_enqueue_load_player(d, argument)` and `return`. The post-load checks move
to `finish_player_login` called from `db_worker_poll_results`.

### Phase 3 — Remove flat files

Once Phase 2 is in production and verified:

1. **`save_char_obj`**: Remove the flat-file write (temp file, rename). DB write only.

2. **`db_worker.c`**: Add `char *db_worker_fetch_player_raw_save(const char *name)` — a
   synchronous helper that opens its own temporary PGconn (reading the same db.conf),
   fetches `raw_save`, and returns a malloced string. Used by non-login paths.

3. **`load_char_obj`**: Replace the `fopen(strsave, "r")` path with a call to
   `db_worker_fetch_player_raw_save(name)` + `fmemopen` + parse. This covers hotreboot
   (`copyover_recover`), offline email check (`email.c`), and the info-offline load
   (`act_info.c`). These paths can block briefly — no game loop is running (hotreboot)
   or the blocking is acceptable (offline staff ops).

4. **`seed-test-player.py`**: Convert to a SQL seed script
   `fixtures/seed_test_player.sql` that inserts directly into the `players` table.
   Update `integration-tests.sh` to load it into the ephemeral test DB.

5. **`area/schema.sql`**: Bump schema version to 8.

## Affected Files

| File | Phase | Change |
|---|---|---|
| `src/save/save_players.c` | 1, 2, 3 | Dual-write (1); extract alloc helper, fmemopen parse (2); remove flat write/read (3) |
| `src/db/db_worker.c` | 1, 2, 3 | Complete write handler (1); complete read handler + poll_results (2); add sync fetch (3) |
| `src/db/db_worker.h` | 2 | Extend DB_PLAYER_RESULT; add sync fetch declaration |
| `src/login.c` | 2 | Replace load_char_obj with async enqueue; extract post-load checks |
| `src/hotreboot.c` | 3 | load_char_obj now fetches from DB via sync helper |
| `src/save/save.h` | 2 | Export alloc_char_for_login declaration |
| `fixtures/seed_test_player.sql` | 3 | New file: SQL seed for integration test player |
| `seed-test-player.py` | 3 | Remove or repurpose |
| `integration-tests.sh` | 3 | Load SQL seed instead of calling Python seeder |
| `area/schema.sql` | 3 | Bump schema version to 8 |

## Trade-offs

**Pros:**
- Player data joins the rest of the game in PostgreSQL — single source of truth,
  consistent backup story
- Enables `tngdb` API access to player data (leaderboards, character pages, etc.)
- Transactional saves via the async worker; coalescing prevents redundant writes
- Removes the `player/` directory tree of flat files

**Cons:**
- Phase 2 introduces an async login path while hotreboot load remains synchronous —
  a temporary inconsistency resolved in Phase 3
- `fread_char` parsing happens on the game thread in `db_worker_poll_results` — correct
  (all game-state mutations stay on the single-threaded game loop) but slightly unusual
- `open_memstream` is glibc-specific (POSIX.1-2008). Already assumed; server is
  Linux-only
- Integration test seeder changes from Python flat-file writer to SQL insert

## Testing

```sh
cd src && make lint && make ack && make unit-tests
```

Integration tests boot the server from DB, validating the async login path.
The existing-player login flow exercises the full read-from-DB path.
