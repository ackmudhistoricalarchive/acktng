# Remove Quest Flatfiles and acktng/web Directory

## Problem

Now that both quest DB migration proposals have been implemented (`quest-database-migration.md`,
`quest-db-loading.md`), there is dead flatfile infrastructure that should be removed:

1. **`acktng/quests/`** — 158 `.prop` files. Production code (`db.c`) no longer reads them; it
   calls `db_load_quest_templates()` directly. However, the flatfile loader
   (`quest_load_templates()` in `template.c`) still exists and is still called by four unit tests
   in `test_quest.c`. The directory cannot be deleted until those tests are updated.

2. **`acktng/web/`** — Contains only `README.md`. No C code reads from this directory; `socket.c`
   references `/web/mp3/` URL paths but those files live elsewhere. Safe to delete immediately.

3. **`save_quests()` call** in `act_obj.c:1065` — Called but never defined anywhere in the
   codebase. Almost certainly inside a dead `#ifdef` block. Should be confirmed and removed.

## Approach

### Step 1 — Delete `acktng/web/`

No code changes required. Simply remove the directory. It is documentation-only.

### Step 2 — Port unit tests off flatfile loading

`test_quest.c` calls `quest_load_templates()` in four test setup sites (lines 396, 418, 435, 465)
to populate `quest_template_table` with real data for assertions.

Replace these calls with a small inline helper that inserts a minimal set of hardcoded
`QUEST_TEMPLATE` structs directly into `quest_template_table`, bypassing file I/O entirely.
This makes the tests self-contained, faster, and independent of the `.prop` files.

### Step 3 — Remove the flatfile loader

Once no callers remain:

- Delete `quest_load_templates()` and the `read_prop_line()` / `load_quest_template_file()`
  helpers from `template.c`
- Remove the `QUEST_TEMPLATE_DIR` define
- Remove the `quest_load_templates` declaration from `quest.h`

### Step 4 — Audit and remove `save_quests()`

Identify the `#ifdef` block containing the `save_quests()` call in `act_obj.c` and confirm it is
dead. Remove the call (and the dead block if appropriate). Since `save_quests` has no definition
anywhere, the server could not have linked with it enabled — the block is definitively dead.

### Step 5 — Delete `acktng/quests/`

With no remaining code references, remove the directory.

### Step 6 — Move open proposals to completed

Move `quest-database-migration.md` and `quest-db-loading.md` to `docs/proposals/completed/`.
Move this proposal there once done.

## Affected Files

- `acktng/web/` — deleted
- `acktng/quests/` — deleted (158 `.prop` files)
- `acktng/src/quests/template.c` — remove `quest_load_templates()`, `load_quest_template_file()`,
  `read_prop_line()`, and `QUEST_TEMPLATE_DIR`
- `acktng/src/headers/quest.h` — remove `quest_load_templates` declaration
- `acktng/src/tests/test_quest.c` — replace flatfile calls with inline test fixtures
- `acktng/src/act_obj.c` — remove dead `save_quests()` call / dead `#ifdef` block
- `acktng/docs/proposals/open/quest-database-migration.md` — move to `completed/`
- `acktng/docs/proposals/open/quest-db-loading.md` — move to `completed/`

## Trade-offs

- **Test fixtures vs DB**: Using hardcoded structs in tests is simpler than spinning up a DB
  connection in unit tests. The existing test infrastructure is file-based; keeping it that way
  (just without the file) is the right approach.
- **Flat-file fallback**: The `quest-db-loading.md` proposal preserved the flatfile loader as a
  DB-unavailable fallback. Since this proposal removes that fallback, servers must have a working
  PostgreSQL connection with `quest_templates` populated to boot. This is acceptable — all other
  content types already have the same requirement.
