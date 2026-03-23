# Proposal: Equipment and Character Sheet Panels for Web Client

**Date:** 2026-03-23
**Status:** Open — approved, pending implementation
**Repos affected:** `acktng` (server-side WS v2 messages), `web` (client-side panel UI)

---

## 1. Problem

The web client currently has two live data panels — Map and Room — that display structured game state pushed from the server. Players have no equivalent for their own character's equipment or stats. To see either, they must type `equipment` or `score` and read the output in the main scrollback. This is inconvenient and inconsistent with the panel-based design.

The requested addition:

- **Equip tab** — a floating panel showing the player's worn items by slot. Hidden by default; toggled by a toolbar button. Mouseover an item shows a popup with that item's full `appraise` stats. No backscroll — each update replaces the entire panel content.
- **Char tab** — a floating panel showing the player's `score` data. Hidden by default; toggled by a toolbar button. No backscroll — each update replaces the entire panel content.

Both panels follow the same constraints as the Map and Room panels: live server-pushed data, no scroll history, appear only after game entry (when the first `Map` message arrives).

---

## 2. Approach

### 2.1 New WS v2 message tags

Three new tags are added to the WebSocket v2 protocol:

| Tag | Sent when |
|-----|-----------|
| `Equipment` | Player types `equipment` or `wear`/`remove` with no argument; on game login |
| `Score` | Player types `score` |
| `Appraise` | Player's web client mouses over a worn item; client sends `appraise <item>` silently |

For each tag, the server intercepts the corresponding command for WebSocket clients, sends the structured JSON message, and skips the normal text output (same pattern as `do_look` for the Room/Map panels).

### 2.2 Equipment JSON (`Equipment` tag)

```json
{
  "slots": [
    { "slot_id": 0, "slot_name": "used as light", "item": { "name": "a brass lantern", "short_descr": "a brass lantern" } },
    { "slot_id": 1, "slot_name": "worn on head",  "item": null },
    ...
  ]
}
```

- One entry per wear location that the character's race can use (matching the `do_wear` display logic).
- `item` is `null` when the slot is empty.
- `short_descr` is used for the mouseover appraise call (it matches what `appraise` accepts).

### 2.3 Score JSON (`Score` tag)

```json
{
  "name": "Kaladina",
  "title": " the Battler",
  "race": "Human",
  "clan": "The Wanderers",
  "hit": 450, "hit_max": 500,
  "mana": 300, "mana_max": 350,
  "move": 200, "move_max": 220,
  "practices": 5,
  "str": 18, "str_max": 18,
  "int": 12, "int_max": 13,
  "wis": 11, "wis_max": 12,
  "dex": 16, "dex_max": 16,
  "con": 15, "con_max": 15,
  "classes": [{ "name": "Warrior", "level": 20 }],
  "exp": 123456,
  "quest_points": 50,
  "invasion_points": 30,
  "post_quest_points": 10,
  "gold": 5000,
  "carry_items": 10, "carry_items_max": 20,
  "carry_weight": 12.5, "carry_weight_max": 100,
  "position": "standing",
  "stance": "Viper",
  "wimpy": 50,
  "kills_npc": 120, "kills_player": 3,
  "killed_by_npc": 5, "killed_by_player": 1
}
```

### 2.4 Appraise JSON (`Appraise` tag)

```json
{
  "name": "a plate chestpiece",
  "type": "armor",
  "extra_flags": "glow hum",
  "worn": "worn on body",
  "item_class": "plate",
  "weight": 25,
  "cost": 5000,
  "level": 30,
  "armor_class": 12,
  "affects": [
    { "stat": "Strength", "modifier": 2 },
    { "stat": "Constitution", "modifier": 1 }
  ]
}
```

For weapons: `"damage_min"`, `"damage_max"`, `"damage_avg"` instead of `"armor_class"`.
For scrolls/potions/staves: `"spells": ["cure light", "bless"]`.
Unrecognized item types: only the header fields, no type-specific block.

---

## 3. Server-side changes (`acktng`)

### 3.1 New functions in `socket.c`

```c
void ws_send_equipment(DESCRIPTOR_DATA *d, CHAR_DATA *ch);
void ws_send_score(DESCRIPTOR_DATA *d, CHAR_DATA *ch);
void ws_send_appraise(DESCRIPTOR_DATA *d, CHAR_DATA *ch, OBJ_DATA *obj);
```

Declared in `headers/socket.h`.

### 3.2 `act_obj.c` — `do_wear()` intercept

At the top of the `arg[0] == '\0'` branch (equipment display), before the existing text loop:

```c
if (!IS_NPC(ch) && ch->desc && ch->desc->websocket_active)
{
    ws_send_equipment(ch->desc, ch);
    return;
}
```

The same intercept is added in `wear_obj()` and `remove_obj()` (the functions called after a successful wear or remove action), to push an updated `Equipment` message automatically when the player's worn items change.

### 3.3 `act_info.c` — `do_score()` intercept

At the top of `do_score()`:

```c
if (!IS_NPC(ch) && ch->desc && ch->desc->websocket_active)
{
    ws_send_score(ch->desc, ch);
    return;
}
```

### 3.4 `act_obj.c` — `do_appraise()` intercept

In `do_appraise()`, after the object is found, before calling `spell_identify`:

```c
if (!IS_NPC(ch) && ch->desc && ch->desc->websocket_active)
{
    ws_send_appraise(ch->desc, ch, obj);
    return;
}
```

### 3.5 Pulse push for score

In `socket.c`, add `ws_score_update()` called from `msdp_update()` / `gmcp_update()` (or equivalently, from the same game-loop site that calls those). It iterates all descriptors; for each WebSocket-active descriptor in `CON_PLAYING`, sends a `Score` message. This runs on every pulse so the char panel is always current.

### 3.6 Equipment push on all gear changes

`ws_send_equipment` is called from `wear_obj()` and `remove_obj()` for any PC descriptor — not filtered to player-initiated actions — so NPC disarms and script-driven removals also update the panel.

### 3.7 Login push

In the login flow (after `do_look` fires on game entry), call `ws_send_equipment` to populate the equip panel with the character's starting gear.

---

## 4. Client-side changes (`web`)

### 4.1 Toolbar buttons

Two toggle buttons added to the existing toolbar (alongside Map/Room toggles):
- **Equip** — toggles the equipment panel
- **Char** — toggles the character sheet panel

### 4.2 Floating panels

Two new `<div class="float-window">` elements added to the HTML, styled identically to the existing map and room windows:
- Hidden on page load via `hideV2Windows()` (extended to include the new panels)
- Shown when the first `Map` message arrives via `showV2Windows()` (extended likewise)
- Draggable, resizable, closeable — same behavior as map/room
- Position/size persisted in `localStorage` (new keys `equip_pos`, `equip_size`, `char_pos`, `char_size`)

### 4.3 `routeMap` handler — new cases

```js
case 'Equipment':
  renderEquipmentPanel(msg.data);
  break;
case 'Score':
  renderScorePanel(msg.data);
  break;
case 'Appraise':
  renderAppraisePopup(msg.data);
  break;
```

### 4.4 Equipment panel rendering

`renderEquipmentPanel(data)`:
- Clears the panel content (no backscroll)
- Renders a table: one row per slot
- Empty slots: slot name + dimmed "Nothing" text
- Filled slots: slot name + item name (styled)
- Each item name has a `mouseenter` listener that sends `appraise <short_descr>` to the server silently (no echo in main window)
- `mouseleave` hides the appraise popup

### 4.5 Character sheet panel rendering

`renderScorePanel(data)`:
- Clears the panel content (no backscroll)
- Renders structured fields: HP/Mana/Move bars, stat table (str/int/wis/dex/con with current/max), classes, exp, gold, position, stance

### 4.6 Appraise popup

`renderAppraisePopup(data)`:
- A small floating `<div>` positioned near the cursor
- Shows item name, type, relevant stats (AC or damage), affects list
- Dismissed on `mouseleave` from the item row, or on any click

### 4.7 Silent command send

The mouseover appraise send must not appear in the main output. The server-side intercept for WebSocket clients already suppresses text output for `do_appraise`; no client-side echo suppression is needed beyond what is already in place (command echo was already removed).

---

## 5. Affected files

**`acktng`:**
- `src/socket.c` — `ws_send_equipment`, `ws_send_score`, `ws_send_appraise` implementations
- `src/headers/socket.h` — declarations for new functions
- `src/act_obj.c` — `do_wear` intercept, `wear_obj`/`remove_obj` push, `do_appraise` intercept
- `src/act_info.c` — `do_score` intercept (still sends `Score` structured message on explicit command in addition to the pulse push)
- `src/login.c` — initial `ws_send_equipment` push on game entry

**`web`:**
- `templates/mud_client.html` — toolbar buttons, new floating panel HTML, JS for render functions, routeMap cases, popup logic

---

## 6. Trade-offs

**In favor:**
- Consistent with the existing Map/Room panel design — structured server push, no backscroll
- Equipment panel auto-updates on wear/remove without player action
- Appraise popup is faster than typing the command and finding the output in scrollback

**Against / risks:**
- `wear_obj` and `remove_obj` are called in many code paths — must guard so the push only fires when `ch->desc && ch->desc->websocket_active` (i.e., the character has an active WS connection, which NPCs never do)
- The `Appraise` message fires on every mouseover; a player hovering over many items rapidly could send many commands. Mitigation: add a small debounce (e.g., 300 ms) on the client before sending.
- `Score` is pushed on every game pulse (same cadence as `msdp_update`), not only on explicit command. This means the char panel reflects the current state at all times. The push is unconditional per pulse — no dirty-flag tracking — which is acceptable given the small JSON size and low pulse frequency.
- `Equipment` is pushed on any gear change, including NPC disarm, mob looting, and script-driven removal, since the panel claims to show live state.
