# Design Proposal: Dynamic DB Lookup for Help, Shelp, and Lore

## Problem

Help, shelp, and lore entries are currently loaded from the database into three
in-memory linked lists (`first_help/last_help`, `first_shelp/last_shelp`,
`first_lore/last_lore`) at boot time. A flat-file fallback (`help/`, `shelp/`,
`lore/` directories) exists for when the DB is unavailable.

This means:
- All help content is resident in memory for the server's lifetime.
- Content changes require a server reboot to take effect.
- The flat files are a maintenance burden and are out of sync with the DB
  (they are the import source, not the canonical store).
- The fallback code path is dead weight now that the DB is required.

## Goals

1. Remove the in-memory help/shelp/lore lists entirely.
2. Have `do_help`, `do_shelp`, and `do_lore` query the database on demand.
3. Remove all flat-file fallback code for help/shelp/lore.
4. Delete the flat files (`help/`, `shelp/`, `lore/`).
5. Remove the `test_help_format.c` unit test that validated flat file format.

## Approach

### New module: `src/db/db_help.c` + `src/db/db_help.h`

A new module owns a persistent read-only PGconn* (the "help connection") that
is opened after boot completes and closed on shutdown. It exposes three
synchronous lookup functions called directly from the command handlers.

Since PostgreSQL indexed lookups are sub-millisecond, synchronous calls are
acceptable in the game loop. This avoids the complexity of an async flow with
player state suspension.

```c
/* Open the help read connection.  Call from comm.c after boot_db(). */
int db_help_open(const char *area_dir);

/* Close on shutdown. */
void db_help_close(void);

/* Look up a help entry by keyword; level is the caller's trust level.
 * Tries exact match then prefix match.  Writes keyword and text into the
 * provided buffers.  Returns 1 if found, 0 if not found. */
int db_help_lookup(const char *keyword, int level,
                   char *kw_out, size_t kw_sz,
                   char *text_out, size_t text_sz);

/* Same as db_help_lookup but searches shelp_entries. */
int db_shelp_lookup(const char *keyword, int level,
                    char *kw_out, size_t kw_sz,
                    char *text_out, size_t text_sz);

/* Look up a lore entry.  npc_flags drives the flag-scoring logic.
 * Returns 1 if found, 0 if not found. */
int db_lore_lookup(const char *keyword, long npc_flags,
                   char *text_out, size_t text_sz);
```

The connection reads `data/db.conf` (or `$ACK_DB_CONF`) using the same logic
as `db_conn.c`. It does not check the schema version — boot already did that.

### Query strategy

**help / shelp** (two-pass: exact then prefix):

Pass 1:
```sql
SELECT level, keywords, body FROM help_entries
WHERE level <= $1 AND LOWER(keywords) = LOWER($2)
LIMIT 1
```

Pass 2 (only if pass 1 returns no rows):
```sql
SELECT level, keywords, body FROM help_entries
WHERE level <= $1 AND LOWER(keywords) LIKE LOWER($2) || '%'
ORDER BY LENGTH(keywords)
LIMIT 1
```

Same queries against `shelp_entries` for shelp.

**lore** (two-pass with flag scoring):

Pass 1 (exact match):
```sql
SELECT e.flags, e.body
FROM lore_topics t
JOIN lore_entries e ON e.topic_id = t.id
WHERE LOWER(t.keywords) = LOWER($1)
  AND (e.flags = 0 OR (e.flags & $2::bigint) = e.flags)
ORDER BY (e.flags & $2::bigint) = 0 ASC,
         bit_count(e.flags & $2::bigint) DESC
LIMIT 1
```

Pass 2 (prefix match, only if pass 1 has no result):
```sql
SELECT e.flags, e.body
FROM lore_topics t
JOIN lore_entries e ON e.topic_id = t.id
WHERE LOWER(t.keywords) LIKE LOWER($1) || '%'
  AND (e.flags = 0 OR (e.flags & $2::bigint) = e.flags)
ORDER BY LENGTH(t.keywords) ASC,
         (e.flags & $2::bigint) = 0 ASC,
         bit_count(e.flags & $2::bigint) DESC
LIMIT 1
```

`bit_count()` is a PostgreSQL 14+ built-in. If the target PostgreSQL is older,
replace with `(SELECT count(*) FROM generate_series(0,62) g WHERE (v >> g) & 1 = 1)`
or a PL/pgSQL helper. We will verify the Postgres version at implementation time.

### Changes to existing files

| File | Change |
|------|--------|
| `src/db/db_help.c` | **New file** — runtime lookup functions |
| `src/db/db_help.h` | **New file** — declarations |
| `src/comm.c` | Call `db_help_open()` after `boot_db()`, `db_help_close()` before exit |
| `src/act_info.c` | `do_help`, `do_shelp`, `do_lore`, `find_best_lore`, `show_lore_entry` — replace list walks with `db_help_*` calls; remove `HELP_DATA` locals |
| `src/db/db_load.c` | Remove `load_helpdir_from_db`, `db_load_helps_from_db`, `db_load_shelps_from_db`, `db_load_lore_from_db` |
| `src/db/db_load.h` | Remove `db_load_helps_from_db`, `db_load_shelps_from_db`, `db_load_lore_from_db` declarations |
| `src/db.c` | Remove `load_help_file`, `load_lore_file`, `load_help_directory`, `load_lore_directory`, `load_help_files`; remove the `#ifdef HAVE_LIBPQ … else load_help_files()` block for help; remove `link_lore_entry`; remove `top_help` increments from help loaders |
| `src/lists.c` | Remove `first_help`, `last_help`, `first_shelp`, `last_shelp`, `first_lore`, `last_lore`, `help_free`, `help_free_destructor` |
| `src/headers/globals.h` | Remove extern declarations for the above |
| `src/headers/config.h` | Remove `HELP_DIR`, `SHELP_DIR`, `LORE_DIR` defines |
| `src/build.c` | `build_findhelp`, `build_helpedit`, `build_addhelp`: replace in-memory list access with direct DB queries/writes using the help connection |
| `src/Makefile` | Add `db_help.o` build rule; remove `test_help_format` target and its rules |
| `src/tests/test_help_format.c` | **Delete** |
| `help/` | **Delete all files** |
| `shelp/` | **Delete all files** |
| `lore/` | **Delete all files** |

### HELP_DATA struct

After this change, `HELP_DATA` (defined in `typedefs.h` / `ack.h`) is only
used by `build.c` during OLC help editing. If the build functions are updated
to use DB directly (writing into local stack buffers), `HELP_DATA` can be
removed entirely from the struct definitions.

### OLC help editing (build.c)

The three OLC help functions will be updated:

- **`build_findhelp`**: query `help_entries` for matching keyword, display
  results to staff.
- **`build_helpedit`**: query `help_entries` for the Nth match, then use
  `build_editstr` on a local buffer; on completion write the updated body back
  via `UPDATE help_entries SET body=$1 WHERE id=$2` through the help connection.
- **`build_addhelp`**: `INSERT INTO help_entries (level, keywords, body, filename) VALUES (...)`.

These are low-frequency staff operations; using the synchronous help connection
is appropriate.

### `top_help` and MSSP

`top_help` is currently used for the MSSP help count. After removing the lists,
it will be replaced by a `SELECT COUNT(*) FROM help_entries` query issued once
at startup (cached in `top_help` from the boot DB connection before it closes).

### `#ifdef HAVE_LIBPQ` guards

The remaining help-related `#ifdef HAVE_LIBPQ` guards in `db.c` (the
`else load_help_files()` branch) will be removed. Building without
`HAVE_LIBPQ` will no longer produce a functional MUD (the DB is required), so
this guard is already conceptually dead.

## Affected files summary

**Modified**: `comm.c`, `act_info.c`, `db.c`, `db/db_load.c`, `db/db_load.h`,
`lists.c`, `headers/globals.h`, `headers/config.h`, `build.c`, `Makefile`

**Created**: `db/db_help.c`, `db/db_help.h`

**Deleted**:
- `src/tests/test_help_format.c`
- All files under `help/` (422 files)
- All files under `shelp/` (409 files)
- All files under `lore/` (~130 files)

## Trade-offs

| Pro | Con |
|-----|-----|
| Help/shelp/lore updates take effect immediately without reboot | One extra DB connection open at runtime |
| ~831 flat files removed from the repo | Flat-file inspection/grep for debugging is gone |
| Removes ~500 lines of load/parse code | Slightly more complex query for lore flag scoring |
| No memory cost for holding all help text at runtime | Help commands require DB round-trip (~<1ms each) |

## Out of scope

- Migrating the import tools (`tools/import_to_db.c`, `tools/db_to_files.c`)
  — these are standalone scripts and can be updated or removed separately.
- Adding a help/shelp search command (fuzzy search across all entries).
