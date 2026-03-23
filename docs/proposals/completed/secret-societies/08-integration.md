# Secret Societies: Integration with Existing Systems

## Clan System Integration

### Independence

Societies and clans are independent membership systems. A player can be in
clan 3 and society 2 simultaneously. The two systems do not share rank,
reputation, or communication channels.

### Points of Contact

| Interaction | Description |
|---|---|
| **Mission targeting** | Society missions may reference clan-related locations (clan rooms, clan donation rooms) as targets. A Reckoning mission might involve retrieving a document from a clan hall. |
| **Diplomacy intelligence** | Rank 3+ society members gain access to `society intel`, which includes the current `politics_data.diplomacy[][]` values between all clans — information normally only visible to clan diplomats. |
| **Clan equipment** | Society missions never target clan equipment (`ITEM_CLAN_EQ`). Clan equipment is protected from society interaction to avoid cross-system griefing. |
| **Exposure and clans** | A player's society exposure does not affect their clan standing. Guard NPCs hostile to the player's society do not extend hostility to the player's clan. |

### Why Separate

Clans are public political entities with declared wars and visible membership.
Societies are covert organizations with hidden membership and deniable
operations. Combining them would undermine both systems — clans would lose
their open political character, and societies would lose their secrecy.

The separation also prevents power stacking. A clan leader who is also a
society inner-circle member has access to both clan commands and society
skills, but the two do not multiply each other's effects.

---

## AI Dialogue Integration

### Society-Aware NPCs

Society NPCs (recruiter, handler, merchant, guard, contact) use the
`ACT_AI_DIALOGUE` system from the AI proposal when it is available. Their
dialogue behavior changes based on the player's society membership:

**For members:**
- The NPC's `AiPrompt` includes society-specific persona text
- The NPC addresses the player by rank title
- Dialogue options include mission briefings, intelligence reports, and
  advancement discussion
- The NPC's `AiKnowledge` includes society-relevant topic tags

**For non-members:**
- The NPC appears as an ordinary civilian
- Dialogue is generic and unremarkable
- No society information is revealed

### Implementation

The society check happens in `npc_dialogue_dispatch()`. When an
`ACT_AI_DIALOGUE` NPC has a non-zero `society` field on its `MOB_INDEX_DATA`,
the dispatch function checks the player's society against the NPC's:

```c
if (npc->pIndexData->society != SOCIETY_NONE)
{
   if (ch->pcdata != NULL &&
       ch->pcdata->society == npc->pIndexData->society)
   {
      /* Append society-specific context to system prompt */
      append_society_prompt(sys_prompt, npc, ch);
   }
   else
   {
      /* Use mundane persona — no society information */
      append_mundane_prompt(sys_prompt, npc);
   }
}
```

The `append_society_prompt()` function injects:
1. The player's rank title and reputation tier
2. Society-specific lore relevant to this NPC's role
3. Current mission status if the player is on a mission from this handler
4. Behavioral instruction: *"Address $PLAYER as $RANK_TITLE. You know they
   are a member of $SOCIETY. Discuss society matters only when asked."*

This gives society NPCs a dual personality — mundane to outsiders, informed
to members — without requiring two separate mob definitions.

### Lore System

Society lore entries are added to the `lore/` directory and loaded into the
`first_lore` linked list at boot. These entries use new `LORE_FLAG_*`
constants for each society:

```c
#define LORE_FLAG_RECKONING   (1 << 16)
#define LORE_FLAG_JADE_EELS   (1 << 17)
#define LORE_FLAG_ASH_CULT    (1 << 18)
#define LORE_FLAG_QUIET_HAND  (1 << 19)
#define LORE_FLAG_JACKAL      (1 << 20)
#define LORE_FLAG_PALE_LEDGER (1 << 21)
```

Society lore entries are only injected into the system prompt of NPCs whose
`lore_flags` include the matching society flag. This means a Reckoning
handler NPC knows Reckoning lore but not Jade Eel operational details.

The player-facing `lore` command can also surface society lore — but only to
players whose `lore_flags` (derived from their society membership) match the
entry. Non-members see generic lore; members see society-specific entries.

---

## Save System Integration

### Save Revision

Adding society data to the player file requires incrementing `SAVE_REVISION`
from 15 to 16. The save format changes are detailed in
`02-data-structures.md`.

### Backward Compatibility

When loading a player file with `SAVE_REVISION < 16`:
- All society fields default to 0 / `FALSE` / `SOCIETY_NONE`
- No data migration is needed
- The player is simply not in any society

### Forward Compatibility

Unknown `Soc*` keys in the player file are skipped by the reader. If a future
revision adds more society fields, older readers will ignore them without
error. This follows the existing save format's forward-compatibility pattern.

### What Is Saved

| Field | Saved | Notes |
|---|---|---|
| `society` | Yes | Society ID |
| `society_rank` | Yes | Current rank |
| `society_rep` | Yes | Current reputation |
| `society_flags` | Yes | Bitmask including DISAVOWED state |
| `society_missions` | Yes | Lifetime mission count |
| `society_joined` | Yes | Timestamp |
| `society_exposed` | Yes | Boolean |
| `society_exposure_timer` | Yes | Remaining ticks |
| `society_pvp_target` | No | Transient; cleared on logout |
| `society_pvp_timer` | No | Transient; cleared on logout |
| `society_kills` | Yes | Lifetime PvP kill count |

---

## Quest System Integration

### Quest Type

Society missions use `QUEST_TYPE_SOCIETY` (value 3) in the existing
`QUEST_DATA` struct. The quest system's completion and failure checks are
extended with society-specific logic:

```c
/* In quest completion check */
if (quest->quest_type == QUEST_TYPE_SOCIETY)
{
   society_mission_complete(ch, quest_slot);
   return;
}
```

### Quest Slots

Society missions occupy one of the 5 available `QUEST_DATA` slots. A player
can have one society mission and up to 4 normal quests simultaneously. The
`SFLAG_ON_MISSION` flag prevents accepting a second society mission while one
is active.

### Quest Items

Some society missions spawn quest items (documents, relics, packages). These
items are:
- Flagged `ITEM_SOCIETY` and `ITEM_NO_DROP`
- Despawned if the mission fails, is abandoned, or the player logs out
- Not saveable to player files (transient objects)

---

## Skill System Integration

### Skill Slots

Society skills are registered in `skill_table[]` at reserved indices. The
existing `MAX_SKILL = 999` provides ample room. Society skills occupy a
contiguous block:

```c
/* Example allocation (exact indices TBD at implementation) */
#define SKILL_SOCIETY_START  900
#define SKILL_SOCIETY_END    949  /* 50 slots: 6 per society + shared + spare */
```

### Practice

Society skills are practiced at society handler NPCs. The handler NPC is
registered as a trainer for society skills only — it does not teach class
skills. The `do_practice()` function is extended to check `can_use_society_skill()`
before allowing practice of society-flagged skills.

### Learned Array

Society skill proficiency is stored in the existing `PC_DATA->learned[]`
array at the reserved indices. This means society skill proficiency is
automatically saved and loaded with the player file — no additional save
format changes are needed for skills.

---

## Command Table Integration

New commands registered in `interp.c`:

| Command | Function | Min Level | Position |
|---|---|---|---|
| `society` | `do_society` | 1 | Standing |
| `deaddrop` | `do_dead_drop` | 1 | Standing |
| `societysense` | `do_society_sense` | 1 | Standing |

Society-specific skill commands (e.g., `bribe`, `smuggle`, `sabotage`) are
registered as standard commands but gate on `can_use_society_skill()` within
their implementation. Non-members who type the command see *"You don't know
how to do that."*

### Command Conflicts

Several society skill names could conflict with potential future commands:
- `sabotage` — unlikely to conflict (no existing command)
- `bribe` — unlikely to conflict
- `negotiate` — potential conflict with future diplomacy commands;
  prefixed as `soc_negotiate` internally but aliased to `negotiate` for
  society members

---

## Update Loop Integration

### Tick Processing

The following society-related functions are called from `update_handler()`
in `update.c`:

```c
void update_handler(void)
{
   /* ... existing updates ... */

   society_mission_update();    /* Decrement mission timers; auto-fail expired */
   society_exposure_update();   /* Decrement exposure timers; clear expired    */
   society_pvp_update();        /* Decrement PvP window timers; clear expired  */
   society_rep_decay();         /* Decay reputation above 300 toward 300       */
}
```

Each function iterates `first_player` and processes only players with
relevant active flags, so the overhead for non-society players is a single
`if (society == SOCIETY_NONE) continue` per tick.

### Frequency

All society updates run every tick (same frequency as existing quest and
combat updates). The per-tick cost is negligible — at most a handful of
comparisons per online player.

---

## Area File Integration

### Mob Fields

Society NPCs are defined in area files with two new optional fields:

```
#MOBILES
#<vnum>
Name       a weathered clerk~
...
AiPrompt   ...~
AiKnowledge history politics
Society    1
SocNpcRole 2
End
```

- `Society <id>` — parsed in `db.c` mob loader; sets `pMobIndex->society`
- `SocNpcRole <id>` — parsed in `db.c` mob loader; sets `pMobIndex->society_npc_role`

These fields are ignored for mobs with `Society 0` (the default).

### Room Flags

No new room flags are needed. Society HQ rooms are identified by vnum
(stored in `society_table[].hq_room`). Recruitment rooms are identified by
vnum (stored in `society_table[].recruit_room`). Room-based behavior (guard
attacks on non-members, sigil requirement for entry) is handled by mob
`spec_fun` functions, not room flags.

---

## Guard NPC Integration

### Society Guard Behavior

Mobs with `society_npc_role == SOCIETY_NPC_GUARD` have a `spec_fun` that:

1. Checks each player entering the room
2. If the player is not a member of the guard's society and is not carrying
   a valid disguise (cover identity from `SFLAG_COVER_ACTIVE`):
   - If the room is the society HQ: attack the player and trigger exposure
   - If the room is a public area: report the player's name to the society
     intelligence log
3. If the player is a member of a hostile society and is carrying their
   sigil: trigger exposure (sigil detected)

### City Guard Extension

Existing city guard NPCs (non-society) gain awareness of exposed players.
When `ch->pcdata->society_exposed == TRUE`, city guard NPCs in cities hostile
to the player's society may:
- Refuse to provide services (banking, training)
- Attack on sight (in extreme cases — only in the rival society's base city)

This is implemented as a check in existing guard `spec_fun` and shop
functions, not as a new system.
