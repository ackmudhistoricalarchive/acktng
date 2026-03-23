# GSGP Implementation Proposal

## Problem

ACK!MUD TNG has no presence on [GameScry](https://game-scry.online) or similar MUD listing/discovery
aggregators. The [Game Scry Game Protocol (GSGP)](https://game-scry.online/about?goto=gsgp) is a
lightweight HTTP-based JSON protocol that lets aggregators poll a game server for real-time data
(active player count, leaderboards) approximately every 30 minutes. Implementing it would expose
ACK!MUD TNG to new players browsing those sites.

## Architecture

The server already uses a file-based bridge between the C game server and the Python web server:
- C writes `../web/soewholist.html` and `../web/whocount.html` via `list_who_to_output()` in
  `src/comm.c` (called every ~5–8 seconds from `gain_update()`).
- The separate web server reads those files and serves them at `/players`.

GSGP follows the same pattern: **C writes `../web/gsgp.json` → the web server serves it at `/gsgp`**.

### Leaderboard scope

Leaderboards contain only *currently online* players. This keeps the implementation simple with no
player-file scanning and guarantees accuracy. The trade-off is empty leaderboards when the server
is idle; if an all-time top-N leaderboard is desired later, that can be a follow-up proposal.

### Atomic writes

`write_gsgp_data()` writes to `gsgp.json.tmp` then calls `rename()` to atomically replace the live
file, preventing the web server from ever reading a partial JSON response.

## Part 1: Game Server Changes (this repo — `src/`)

These changes are **implemented in this PR**.

### `src/headers/config.h`

```c
#define GSGP_JSON_FILE "../web/gsgp.json" /* Game Scry Game Protocol endpoint data */
```

### `src/comm.c`

Added alongside `list_who_to_output()`:
- `struct gsgp_entry` — holds per-player data (name, level, pkills, quest_points)
- `gsgp_by_level()`, `gsgp_by_pkills()`, `gsgp_by_qpts()` — qsort comparators
- `write_gsgp_board()` — writes one leaderboard section to a FILE*
- `write_gsgp_data()` — writes the full GSGP JSON to `gsgp.json.tmp`, then renames it
- `list_who_to_output()` — updated to collect player data and call `write_gsgp_data()`

## Part 2: Web Server Changes (separate web server repo)

These changes need to be applied in the web server repository.

### Add `GSGP_FILE` constant

```python
GSGP_FILE = Path(os.environ.get("ACK_GSGP_FILE", str(WEB_DIR / "gsgp.json")))
```

### Add `/gsgp` route in `do_GET()`

Add before the existing `/players` route:

```python
if route in ("/gsgp", "/gsgp/"):
    self._send_gsgp()
    return
```

### Add `_send_gsgp()` method to `WhoRequestHandler`

```python
def _send_gsgp(self) -> None:
    import json

    data = _read_file_if_present(GSGP_FILE)
    if data is None:
        data = json.dumps({"name": "ACK!MUD TNG", "active_players": 0, "leaderboards": []})
    body_bytes = data.encode("utf-8")
    self.send_response(200)
    self.send_header("Content-Type", "application/json")
    self.send_header("Content-Length", str(len(body_bytes)))
    self.send_header("Access-Control-Allow-Origin", "*")
    self.end_headers()
    self.wfile.write(body_bytes)
```

The `Access-Control-Allow-Origin: *` header is standard for public API endpoints. The fallback
`json.dumps(...)` fires only if the game server has never written the file (e.g., the server
hasn't booted yet), ensuring the endpoint always returns valid JSON.

## JSON Format

```json
{
  "name": "ACK!MUD TNG",
  "active_players": 5,
  "leaderboards": [
    {
      "name": "Top Players by Level",
      "entries": [
        { "name": "Aragorn", "value": 101 },
        { "name": "Gandalf", "value": 95 }
      ]
    },
    {
      "name": "Top PKillers",
      "entries": [
        { "name": "Sauron", "value": 42 }
      ]
    },
    {
      "name": "Top Quest Point Earners",
      "entries": [
        { "name": "Frodo", "value": 1200 }
      ]
    }
  ]
}
```

Each leaderboard is capped at 10 entries. Players with 0 pkills are omitted from the PKillers board.

## Affected Files (this repo)

| File | Change |
|------|--------|
| `src/headers/config.h` | Add `GSGP_JSON_FILE` define |
| `src/comm.c` | Add GSGP struct, comparators, `write_gsgp_board()`, `write_gsgp_data()`; update `list_who_to_output()` |
| `src/tests/test_comm.c` | Documentation comment update only |

## Trade-offs

- **Online players only** (not all-time): simpler, always fresh, may be empty at low-population times.
- **Atomic rename**: prevents partial JSON reads at the cost of two file ops per update cycle.
- **No new source file**: `write_gsgp_data()` lives in `comm.c` alongside `list_who_to_output()`,
  which has the same structure and constraints.

## Verification

1. `cd src && make unit-tests` — all tests pass.
2. Apply Part 2 changes to the web server repo.
3. Start web server and game server.
4. `curl http://localhost:PORT/gsgp` — verify JSON response with correct structure.
5. Log in a test character; re-curl — verify `active_players` increments and character appears in
   leaderboards.
6. Register the `/gsgp` endpoint URL on the GameScry listing for ACK!MUD TNG.
