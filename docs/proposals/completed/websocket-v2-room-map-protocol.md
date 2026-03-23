# Proposal: WebSocket v2 Room and Map Protocol

**Status:** Open
**Repos affected:** `acktng`, `web` (client already implemented)

---

## Problem

The web MUD client (`web/templates/mud_client.html`) contains two floating windows — a **Room panel** and a **Map canvas** — that display structured game state alongside the text I/O panel. These windows are fully implemented on the client side and are activated only when the server sends JSON messages of the form:

```json
{"v": 2, "tag": "Room", "data": { ... }}
```

The client's `handleMessage` function checks `msg.v === 2` to switch into v2 mode and call `showV2Windows()`. Until a v2-tagged message is received, the windows remain hidden and the toggle buttons are invisible.

**acktng never sends any such messages.** It sends raw game text in WebSocket frames and `{"type":"music",...}` commands — neither of which matches the `v === 2` check. The GMCP protocol it uses for structured data (room info, vitals) is a telnet sub-negotiation mechanism and cannot be parsed by the browser client. As a result, the Room and Map windows never appear when connected to acktng.

---

## Approach

Add a WebSocket v2 JSON protocol layer to acktng. When a player is connected via WebSocket (`d->websocket_active`), the server will emit v2-tagged JSON frames in parallel with the normal game text, providing the client with structured room and map data.

### Wire Format

Every v2 message is a WebSocket text frame containing:

```json
{"v":2,"tag":"<Tag>","data":<json-object>}
```

This is identical to the format the client already expects. No client changes are required.

### Tags to Implement (Phase 1)

**`Room`** — Full room snapshot. Sent on room entry (after a move) and when the player uses `look`. Replaces the entire room panel state.

```json
{
  "v": 2,
  "tag": "Room",
  "data": {
    "name": "The Town Square",
    "description": "A large cobblestone plaza...",
    "exits": ["north", "east", "west"],
    "mobs": [
      {"id": 12345, "name": "a town guard", "keywords": ["guard"], "actions": ["look", "attack", "consider"]}
    ],
    "players": [
      {"name": "Ariakas", "actions": ["look", "tell", "group"]}
    ],
    "objects": [
      {"id": 67890, "name": "a rusty sword", "keyword": "sword", "actions": ["look", "get", "examine"]}
    ],
    "extras": []
  }
}
```

**`Map`** — BFS map of nearby rooms. Sent on room entry.

```json
{
  "v": 2,
  "tag": "Map",
  "data": {
    "current_room_id": 3001,
    "rooms": [
      {"id": 3001, "rel_x": 0, "rel_y": 0, "terrain": "city", "exits": ["north", "east"], "mob_count": 2},
      {"id": 3002, "rel_x": 0, "rel_y": -1, "terrain": "road", "exits": ["south", "north"], "mob_count": 0}
    ]
  }
}
```

**`Map:Scan`** — Result of the `scan` command. Sent after `do_scan` runs for WebSocket clients.

```json
{
  "v": 2,
  "tag": "Map:Scan",
  "data": {
    "north": {"room_id": 3002, "count": 3},
    "south": null,
    "east":  {"room_id": 3005, "count": 1},
    "west":  null
  }
}
```

### Deferred (Phase 2)

Incremental updates (`Room:Enter`, `Room:Leave`, `Room:ObjectAppear`, `Room:ObjectVanish`) allow the client to update the room panel without re-sending the full snapshot. These are already supported by the client. Deferring them keeps Phase 1 scope small — the full `Room` re-send on each transition is correct, just slightly less efficient.

---

## Implementation Plan

### 1. Core helper: `ws_v2_send()` — `socket.c`

Add a new function (static within socket.c) that assembles the v2 envelope and writes it as a WebSocket text frame:

```c
static void ws_v2_send(DESCRIPTOR_DATA *d, const char *tag, const char *data_json);
```

Builds `{"v":2,"tag":"<tag>","data":<data_json>}` into a stack buffer (or dynamic buffer if needed) and calls `write_websocket_frame(d, 0x1, ...)`. This function is a no-op if `!d->websocket_active`.

### 2. Terrain mapping

Add a `static const char *ws_terrain_name(sh_int sector)` helper in `socket.c` that maps `SECT_*` constants to the string names the client's `TERRAIN_COLOR` map recognizes:

| SECT constant    | String       |
|-----------------|--------------|
| `SECT_CITY`     | `"city"`     |
| `SECT_FIELD`    | `"field"`    |
| `SECT_FOREST`   | `"forest"`   |
| `SECT_HILLS`    | `"hills"`    |
| `SECT_MOUNTAIN` | `"mountain"` |
| `SECT_WATER_SWIM`   | `"water_swim"`   |
| `SECT_WATER_NOSWIM` | `"water_noswim"` |
| `SECT_DESERT`   | `"desert"`   |
| `SECT_INSIDE` / `SECT_NULL` / `SECT_RECALL_OK` | `"inside"` |
| `SECT_AIR`      | `"inside"`   |

### 3. Entity ID scheme

`CHAR_DATA` and `OBJ_DATA` have no stable numeric id field. Use the pointer cast to `unsigned long` as the runtime id:

```c
unsigned long mob_id = (unsigned long)(uintptr_t)mob;
```

This is unique per session without requiring struct changes. The id only needs to be consistent within a single client session (the client discards all state on disconnect).

### 4. Room description color codes

acktng room descriptions contain `@@X` color codes. The client's `renderRoom()` calls `ansiToHtml(roomState.description)` to render ANSI escape sequences. Two options:

- **Option A (recommended):** Strip colors entirely via `color_strip()` (or equivalent) before sending. Clean, simple, no client changes. The room name and description are readable without color.
- **Option B:** Convert `@@X` codes to ANSI escape sequences before embedding in JSON. The client already handles ANSI, so this would render colors in the panel. More complex (need a converter function) but richer output.

Recommend Option A for Phase 1 and revisit in Phase 2.

### 5. Room snapshot builder — `socket.c`

Add:

```c
void ws_send_room(DESCRIPTOR_DATA *d, CHAR_DATA *ch);
```

Declared in `socket.h`. Builds and sends a `Room` message by iterating:
- `ch->in_room->name` and `ch->in_room->description` (stripped of color)
- `ch->in_room->exit[door]` for doors 0–5 (N/S/E/W/U/D), emitting direction strings for non-null, passable exits
- `ch->in_room->first_in_room` object list (name, keyword from `name`, id from pointer)
- The room's `people` linked list, split into mobs (`IS_NPC`) and players (not NPC); filtered to exclude `ch` herself, switched characters, and hidden mobs above player's detection level

Actions exposed per entity type:
- Mobs: `["look", "attack", "consider"]`
- Players: `["look", "tell", "group"]`
- Objects: `["look", "get", "examine"]` (conditionally add `"drink"` for drink containers, `"drop"` if player carries it — but skip conditional logic in Phase 1 and just send the full set)

### 6. Map BFS builder — `socket.c`

Add:

```c
void ws_send_map(DESCRIPTOR_DATA *d, CHAR_DATA *ch);
```

Declared in `socket.h`. Performs a BFS from `ch->in_room`, following exits (N/S/E/W/U/D), up to **depth 4**. Uses a visited set keyed by `ROOM_INDEX_DATA *` pointer to handle cycles.

Relative coordinates are computed by accumulating directional unit vectors:
- North: `(0, -1)`, South: `(0, +1)`, East: `(+1, 0)`, West: `(-1, 0)`, Up: `(0, -1)`, Down: `(0, +1)`

Up/Down share the same plane as North/South for simplicity (the map is 2D). The BFS uses the first-reached `(rel_x, rel_y)` for each room; if a room is reachable via two paths with conflicting coordinates (non-Euclidean layout), the BFS-first result stands. This is a best-effort approach and will be incorrect for non-Euclidean areas, but it's acceptable for the initial implementation.

For each room in the BFS result:
- `id`: room vnum
- `rel_x`, `rel_y`: accumulated relative coordinates
- `terrain`: from `ws_terrain_name(room->sector_type)`
- `exits`: list of passable exit direction strings
- `mob_count`: count of NPCs in `room->people` list

The BFS limit of depth 4 produces at most ~121 rooms in a perfect grid, which is well within a single WebSocket frame.

### 7. Hook: room entry — `comm.c`

In `move_char()`, after the character has been moved to the new room and the normal `look` text has been sent, add:

```c
if (ch->desc && ch->desc->websocket_active)
{
    ws_send_room(ch->desc, ch);
    ws_send_map(ch->desc, ch);
}
```

### 8. Hook: `look` command — `act_info.c`

At the end of `do_look()`, when the target is the player's own room (no argument or `here`), add:

```c
if (ch->desc && ch->desc->websocket_active)
    ws_send_room(ch->desc, ch);
```

### 9. Hook: `scan` command — `act_info.c`

At the end of `do_scan()`, add:

```c
if (ch->desc && ch->desc->websocket_active)
    ws_send_map_scan(ch->desc, ch);
```

Where `ws_send_map_scan()` checks each of the 4 cardinal directions, counts mobs in the adjacent room, and sends a `Map:Scan` message.

---

## Affected Files

| File | Change |
|------|--------|
| `src/socket.c` | Add `ws_v2_send()`, `ws_terrain_name()`, `ws_send_room()`, `ws_send_map()`, `ws_send_map_scan()` |
| `src/headers/socket.h` | Declare `ws_send_room()`, `ws_send_map()`, `ws_send_map_scan()` |
| `src/act_info.c` | Hook `do_look()` and `do_scan()` |
| `src/comm.c` | Hook `move_char()` |

No changes to `web/` are required — the client already handles all v2 tags used in Phase 1.

---

## Trade-offs and Risks

**Non-Euclidean maps:** The BFS coordinate system breaks for portal rooms, one-way exits, and loops. Rooms may be assigned wrong relative positions. This is a known limitation; the map will still be useful for standard areas.

**Up/Down on 2D map:** Up and Down are rendered on the same plane as North and South. Players in multi-level areas will see a compressed view. A future 3D map extension could address this.

**Re-send cost on every move:** Sending a full `Room` and `Map` snapshot on each movement is more data than incremental updates would be. Given the scale of a MUD (tens of players, rooms of ~20 entities, BFS of ~100 rooms), this is unlikely to be a bottleneck, but Phase 2 incremental updates would reduce it.

**No color in room descriptions (Phase 1):** Descriptions are sent color-stripped. This is consistent with how the room name is displayed in many MUD clients and is readable; it just loses the atmospheric color.

**Buffer sizing:** Room snapshots with many entities and map snapshots with 100+ rooms require careful buffer management. The existing `json_append` / dynamic-buffer pattern in `socket.c` should be extended rather than using fixed-size stack buffers for these payloads.

---

## Out of Scope

- Changes to `tngdb`, `tng-ai`, or `web/web_who_server.py`
- Phase 2 incremental updates (`Room:Enter`, `Room:Leave`, etc.)
- ANSI color in room descriptions
- NPC visibility filtering beyond basic `IS_NPC` check
- Map: 3D (up/down) rendering
