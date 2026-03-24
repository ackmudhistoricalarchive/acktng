# Proposal: WS Inventory Panel, Extended Score, and Room Object Stats

## Problem

The web client (web PR #46) added three new UI features that require corresponding server-side changes in `socket.c`:

1. **Inventory window** — expects a structured `Inventory` tag payload with per-item stats. Currently `do_inventory` sends raw text; no `ws_send_inventory` function exists.
2. **Extended character sheet** — expects additional fields in the `Score` payload: alignment, hitroll, damroll, AC, saving throw, age, and bank balance. These are not currently emitted by `ws_send_score`.
3. **Room object appraise on hover** — expects `Room.objects` entries to include inline item stats (same shape as equipment items). Currently objects include only `id`, `name`, `keyword`, `actions`.

---

## Approach

All changes are in `src/socket.c` and `src/act_info.c`. No schema changes, no new commands, no area file changes.

### 1. `ws_send_inventory` — new function

Add a new function following the same pattern as `ws_send_equipment`. Called from `do_inventory` when `ch->desc->websocket_active`.

**Payload shape:**

```json
{
  "v": 2,
  "tag": "Inventory",
  "data": {
    "items": [
      {
        "id": "<pointer-as-decimal>",
        "keyword": "<first keyword>",
        "actions": ["look", "drop", "examine"],
        <...ws_append_item_stats fields...>
      }
    ]
  }
}
```

Each item entry starts with `id` (pointer cast, same as room objects), `keyword` (first keyword via `json_first_keyword`), and `actions` array, then the full `ws_append_item_stats` fields (`short_descr`, `type`, `item_class`, `weight`, `cost`, `level`, weapon/armor specifics, `affects`).

The `keyword` field is used by the client to build commands (`drop <keyword>`, `look <keyword>`, etc.).

`actions` is always `["look", "drop", "examine"]` for now. Future extensions (e.g., `"equip"`, `"compare"`) can be added later.

**Buffer size:** `32768` bytes — inventory can hold more items than equipment slots, each with full stat fields.

**`do_inventory` change:**

```c
void do_inventory(CHAR_DATA *ch, char *argument)
{
    if (!IS_NPC(ch) && ch->desc && ch->desc->websocket_active)
    {
        ws_send_inventory(ch->desc, ch);
        return;
    }
    send_to_char("You are carrying:\n\r", ch);
    show_list_to_char(ch->first_carry, ch, TRUE, TRUE);
}
```

---

### 2. `ws_send_score` — extend with new fields

Append the following fields to the existing JSON object, after the current content and before the closing `}`:

| JSON field | C source | Notes |
|-----------|----------|-------|
| `"alignment"` | `ch->alignment` | Raw value (-1000 to 1000) |
| `"alignment_label"` | computed | `"Good"` if `>= 350`, `"Neutral"` if `-349..349`, `"Evil"` if `<= -350` |
| `"hitroll"` | `get_hitroll(ch)` | Effective HR including all bonuses |
| `"damroll"` | `get_damroll(ch)` | Effective DR including all bonuses |
| `"ac"` | `ch->armor` | Armor class |
| `"saving_throw"` | `ch->saving_throw` | Single saving throw value |
| `"age"` | `get_age(ch)` | Character age in game-years |
| `"bank_gold"` | `ch->pcdata->balance` | Bank balance |

**Alignment label thresholds** (matching standard MUD convention):
- `alignment >= 350` → `"Good"`
- `alignment >= -349` → `"Neutral"`
- else → `"Evil"`

**`ws_compute_score_checksum` extension:** Add `hitroll`, `damroll`, `ac`, `alignment`, `saving_throw`, `age`, and `balance` to the checksum so the panel updates automatically when these change (e.g. after wearing/removing items, resting, levelling).

**Buffer size:** `ws_send_score` currently uses `8192`. The new fields add ~120 bytes worst-case; bump to `12288` to retain headroom.

---

### 3. `ws_send_room` — add inline item stats for floor objects

In the `/* objects on the floor */` loop in `ws_send_room`, after the existing `id`/`name`/`keyword`/`actions` fields, call `ws_append_item_stats` to include full item data:

```c
/* Before (current): */
json_append(buf, &pos, sizeof(buf), ",\"actions\":[\"look\",\"get\",\"examine\"]}");

/* After: */
json_append(buf, &pos, sizeof(buf), ",\"actions\":[\"look\",\"get\",\"examine\"],");
ws_append_item_stats(buf, &pos, sizeof(buf), obj);
json_append(buf, &pos, sizeof(buf), "}");
```

**Buffer size:** `ws_send_room` currently uses `16384`. A room with many objects all with full stats could overflow. Bump to `32768`.

---

## Affected Files

| File | Change |
|------|--------|
| `src/socket.c` | Add `ws_send_inventory`; extend `ws_send_score` + checksum; extend `ws_send_room` objects loop; bump buffer sizes |
| `src/act_info.c` | Add WS early-return path in `do_inventory` |
| `src/headers/socket.h` | Declare `ws_send_inventory` |

---

## Trade-offs

| Decision | Alternative | Reason chosen |
|----------|-------------|---------------|
| Inline item stats in `ws_send_room` objects | Separate `RoomAppraise` message on hover | Avoids server round-trips per mouseover; consistent with equipment |
| `32768` for room/inventory buffers | Dynamic allocation | Consistent with existing static buffer style in socket.c |
| Single `saving_throw` field | Separate spell/breath/rod fields | The codebase has only one `saving_throw` field on `CHAR_DATA`; no separate save types exist |
| Alignment label thresholds ±350 | Match `IS_GOOD`/`IS_EVIL` (0 boundary) | ±350 gives a meaningful neutral band; the binary IS_GOOD/IS_EVIL at 0 is too coarse for display |

---

## Out of Scope

- Mob-carried item stats in `Room.objects` (mobs don't expose carried items in the room message)
- `keywords` array on inventory items (using `keyword` singular with `json_first_keyword`, same as room objects)
- Unit tests: `ws_send_*` functions depend on live descriptor/character state and are not practical to unit test without substantial mocking infrastructure; covered by integration test
