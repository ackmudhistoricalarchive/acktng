# Design Proposal: Message Tagging System & Multi-Panel UI

**Status:** Pending approval
**Spec reference:** Web Client: Message Tagging System & Multi-Panel UI Specification
**Affected layers:** `src/` (server), `web/` (client)

---

## 1. Problem Statement

The current WebSocket output path sends raw, ANSI-coded plain text to the web client. The client has no way to distinguish combat output from room descriptions, tells from system messages, or map data from regular speech. This makes it impossible to:

- Route output to dedicated UI panels (room, map, chat)
- Apply per-category styling (tell highlights, system muting)
- Build a live room-state panel that updates without re-parsing free-form look text
- Build a graphical minimap

The spec defines a v2 JSON wire protocol with semantic tags and a three-panel client layout to solve all of this.

---

## 2. Approach Overview

The work splits into two mostly independent tracks:

| Track | Scope | Files |
|-------|-------|-------|
| **A — Server tagging** | Wrap WebSocket output in JSON envelopes with semantic tags; emit structured `Room` and `Map` payloads | `src/socket.c`, `src/comm.c`, `src/act_move.c`, `src/act_info.c`, plus tag-site edits throughout |
| **B — Client UI** | Replace the single-pane client with the three-panel layout, routing logic, canvas map, and room panel | `web/templates/mud_client.html` |

Legacy telnet and v1 WebSocket behaviour is untouched.

---

## 3. Track A — Server-Side Tagging

### 3.1 New helper: `send_tagged_to_char()`

A new function in `socket.c` (declared in `socket.h`) wraps a string in the v2 JSON envelope and writes it to a descriptor only if `d->websocket_active`. Non-WebSocket descriptors receive the raw string via the existing path.

```c
void send_tagged_to_char(CHAR_DATA *ch, const char *tag, const char *text);
```

Internally:

```c
// JSON-escape `text`, then:
// {"v":2,"tag":"<tag>","data":"<escaped_text>"}
```

Uses the existing `json_escape()` from `npc_dialogue.c` (moved or exposed via a shared header). For non-WebSocket descriptors the function calls `send_to_char()` directly.

### 3.2 Structured payload helpers

Two additional helpers produce structured `data` objects (not plain strings):

```c
void send_room_to_char(CHAR_DATA *ch);        // emits tag "Room" with full room JSON
void send_map_to_char(CHAR_DATA *ch);         // emits tag "Map" with surrounding rooms
void send_map_scout_to_char(CHAR_DATA *ch);   // emits tag "Map:Scout" for extra rooms
```

These are called only when `ch->desc && ch->desc->websocket_active`.

### 3.3 Tag assignment — call-site strategy

Rather than auditing every `send_to_char` call, we adopt a **layered tagging** strategy:

1. **High-value structured paths first** (Room, Map) — these are the ones that enable the new panels and must be exact.
2. **Communication paths** — `do_tell`, `do_say`, `do_yell`, `do_shout`, `do_gossip`, channel handlers — each gets its specific `Communication:*` tag.
3. **Combat** — the combat round formatter in `fight.c` gets tag `Combat`; death messages get `Combat:Death`.
4. **Prompt** — the prompt string assembled in `process_output()` gets tag `Prompt`.
5. **Everything else** — a catch-all `send_tagged_to_char` wrapper defaults to an appropriate broad tag (e.g. `Skill`, `Inventory`, `Score`, `Help`, `System`).

The tagging table (mapping call site → tag) will be maintained in `docs/tagging-table.md` as a living document updated alongside the code.

### 3.4 Room message emission

`send_room_to_char()` is called from:
- `do_look()` (no argument, or on room entry) in `act_info.c`
- `char_to_room()` / teleport / portal in `act_move.c` and `handler.c`

It serialises the room's `ROOM_INDEX_DATA` into the structured `Room` payload defined in spec §1.3, including `name`, `description`, `exits[]`, `mobs[]`, `players[]`, `objects[]`, and `extras[]`. The `actions` array for each entity is computed server-side based on entity type and (for mobs) whether they are aggressive/attackable.

Delta events (`Room:Enter`, `Room:Leave`, `Room:ObjectAppear`, `Room:ObjectVanish`) are emitted from the arrival/departure hooks in `handler.c` (`char_to_room`, `char_from_room`, `obj_to_room`, `obj_from_room`).

### 3.5 Map message emission

`send_map_to_char()` is called after every successful room transition. It walks the exit graph up to 2 steps in each cardinal direction, collects room metadata (vnum, terrain, exits, mob counts via scan logic), and emits the `Map` payload. If the player has the `scout` skill, `send_map_scout_to_char()` extends coverage to 3 steps and emits `Map:Scout`.

Terrain strings are mapped from the existing `SECT_*` constants in `typedefs.h` to the lowercase terrain names in spec §2.1.4.

### 3.6 Music backward compatibility

The existing `send_music_play()` / `send_music_stop()` JSON format (`{"type":"music",...}`) is preserved unchanged. The spec explicitly calls this out as `Music` tag with v1 compat — we wrap it in the v2 envelope:

```json
{"v":2,"tag":"Music","data":{"type":"music","action":"play","url":"..."}}
```

### 3.7 Affected source files (Track A)

| File | Change |
|------|--------|
| `src/socket.c` | Add `send_tagged_to_char()`, `send_room_to_char()`, `send_map_to_char()`, `send_map_scout_to_char()`; update music helpers to v2 envelope |
| `src/socket.h` | Declare new helpers |
| `src/comm.c` | Tag `send_to_char` call sites for System, Prompt |
| `src/act_info.c` | Emit `Room` from `do_look()`; tag Score, Inventory, Help |
| `src/act_move.c` | Emit `Room` + `Map` after movement; tag move-related messages |
| `src/act_comm.c` | Tag Communication:Tell/Say/Yell/Shout/Gossip/Channel/Emote |
| `src/fight.c` | Tag Combat, Combat:Death |
| `src/handler.c` | Emit Room:Enter/Leave/ObjectAppear/ObjectVanish deltas |
| `src/skills*.c` | Tag Skill output |
| `src/npc_dialogue.c` | Extract `json_escape()` to `src/json_utils.c` + `src/json_utils.h` for shared use |

---

## 4. Track B — Client-Side UI

### 4.1 Three-panel layout

The existing single `<div id="output">` is replaced with three panels side-by-side (flex row on wide viewports, stacked column on narrow):

```
┌─────────────┬─────────────┬─────────────────────┐
│  #map-panel │ #room-panel │     #io-panel        │
│  (canvas)   │ (live-data) │  (scrollback)        │
│             │             ├─────────────────────┤
│             │             │  #command-input      │
└─────────────┴─────────────┴─────────────────────┘
```

Panel widths are persisted to `localStorage` keys `ack.panel.map.width`, `ack.panel.room.width`, `ack.panel.io.width`. Drag dividers are implemented with `mousedown`/`mousemove`/`mouseup` on thin `<div class="divider">` elements between panels.

On viewport width < 768 px the layout switches to vertical stacking (map → room → io) via a CSS media query; the `localStorage` key `ack.panel.layout` tracks user override.

### 4.2 Message routing

The `onmessage` handler is refactored into the routing logic from spec §3:

```js
socket.addEventListener('message', (event) => {
  let msg;
  try { msg = JSON.parse(event.data); } catch { appendIO(event.data, null); return; }
  if (!msg.v || msg.v !== 2) { handleV1(msg); return; }

  const category = msg.tag.split(':')[0];
  switch (category) {
    case 'Room':  routeRoom(msg); break;
    case 'Map':   routeMap(msg);  break;
    case 'Music': handleMusic(msg.data); break;  // existing logic
    default:      appendIO(msg);
  }
});
```

Unknown tags fall through to `appendIO` and a `console.warn` is emitted.

### 4.3 Room panel

State object:

```js
const roomState = {
  name: '', description: '', exits: [],
  mobs: {}, players: {}, objects: {}, extras: {}
};
```

`routeRoom(msg)` updates `roomState` per the delta/full-replace rules from spec §3, then calls `renderRoom()`.

`renderRoom()` rebuilds `#room-content` innerHTML from scratch each call. It renders:
- `<h2>` room name
- `<p>` description
- Exit chips (clickable → send direction abbrev)
- Collapsible sections for mobs, players, objects, extras — each row has a `[▼]` dropdown built from the entity's `actions` array using the command mapping table in spec §2.2.3

### 4.4 Map panel (canvas)

State object:

```js
const mapState = {
  currentId: null,
  hasScouted: false,
  rooms: {}  // keyed by room id
};
```

`routeMap(msg)` updates `mapState` per spec §3 rules, then calls `scheduleMapRedraw()`.

`scheduleMapRedraw()` uses a `requestAnimationFrame` coalescing flag so multiple map messages in one tick produce one redraw.

`drawMap()` is the canvas render function:
1. Clear canvas
2. Draw exit lines between tile centres (semi-transparent, rendered first)
3. Draw room tiles: load `img/terrain/<terrain>.png` → fallback to solid fill colour from spec §2.1.4
4. Highlight current room with `#5b9cf6` border + player dot
5. Draw mob count badges on adjacent rooms (skip badge if count = 0)
6. Scouted rooms (hasScouted + radius > 2) rendered at 50% opacity

The canvas is sized via `ResizeObserver` on the panel element.

### 4.5 I/O panel styling

`appendIO(msg)` creates a `<div>` with a CSS class derived from the tag:

| Tag | Class | Style |
|-----|-------|-------|
| `System` | `io-system` | `color: #9ca3af; font-style: italic` |
| `Communication:Tell` | `io-tell` | `color: #a5f3fc` |
| all others | — | default |

ANSI escape code rendering reuses the existing conversion logic.

### 4.6 Legacy / v1 fallback

If the first message received is not valid v2 JSON, the client stays in v1 mode: all output goes to the I/O panel, and the Room and Map panels display a `<p class="placeholder">No room data — legacy connection</p>` message.

### 4.7 Affected client files (Track B)

| File | Change |
|------|--------|
| `web/templates/mud_client.html` | Full rewrite of layout, routing, room panel, map canvas, I/O panel styling |

No new files are required on the client side — everything stays within the single template.

---

## 5. New/Extracted Files

| File | Purpose |
|------|---------|
| `src/json_utils.c` | `json_escape()` extracted from `npc_dialogue.c`; shared by socket and dialogue code |
| `src/json_utils.h` | Header for json_utils |
| `docs/tagging-table.md` | Living table: call site → tag mapping |

---

## 6. Unit Tests

| Test file | What it covers |
|-----------|---------------|
| `src/tests/test_json_utils.c` | `json_escape()` edge cases (control chars, unicode escaping, empty string) |
| `src/tests/test_send_tagged.c` | `send_tagged_to_char()` produces valid JSON envelope; non-WebSocket descriptor gets plain text |
| `src/tests/test_room_payload.c` | `send_room_to_char()` serialises room correctly; delta helpers update payload correctly |
| `src/tests/test_map_payload.c` | `send_map_to_char()` walks exit graph correctly; scout extension adds correct rooms |

The existing integration test already exercises the login flow; no changes needed there.

---

## 7. Trade-offs & Risks

| Concern | Mitigation |
|---------|-----------|
| **Performance** — JSON wrapping adds per-message overhead | Output is already buffered through `write_to_buffer()`; JSON envelope for typical message adds < 40 bytes. Map messages (largest) are only sent on room change, not every tick. |
| **Tagging completeness** — many `send_to_char` call sites exist; missing tags degrade gracefully (fallback to Main I/O) | Adopt layered strategy: structured Room/Map/Communication first; catch-all for remainder. Tagging table tracks gaps. |
| **Canvas tile images absent** — no `img/terrain/` assets yet | Spec explicitly defines fallback solid colours; the client renders correctly without images. Assets can be added later without code changes. |
| **Backwards compatibility** — existing telnet and v1 WebSocket clients | All changes are conditional on `d->websocket_active`; telnet path is untouched. v1 JSON (music) wrapped in v2 envelope but `data` field preserves v1 shape. |
| **`json_escape()` extraction** — currently only in `npc_dialogue.c` | Move to `json_utils.c`; update `npc_dialogue.c` to call the shared version. No behavioural change. |

---

## 8. Out of Scope

- Terrain tile PNG assets (client works with fallback colours)
- `Map:Scan` auto-emit on tick (scan logic can be wired later; `Map` already carries `mob_count` on adjacent rooms)
- Mobile/touch drag for panel resizers
- Any changes to telnet clients or non-WebSocket output paths

---

## 9. Implementation Order

1. Extract `json_escape()` → `json_utils.c` + tests
2. Add `send_tagged_to_char()` → `socket.c` + tests
3. Add `send_room_to_char()` + delta helpers → `socket.c`/`handler.c` + tests
4. Add `send_map_to_char()` + scout variant → `socket.c` + tests
5. Tag Communication call sites (`act_comm.c`)
6. Tag Combat call sites (`fight.c`)
7. Tag remaining call sites (info, skills, system, prompt)
8. Client rewrite: layout + routing + room panel
9. Client: canvas map panel
10. Client: I/O panel styling
11. Full `make unit-tests` + manual WebSocket smoke test
