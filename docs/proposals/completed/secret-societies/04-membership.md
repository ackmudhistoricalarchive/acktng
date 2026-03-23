# Secret Societies: Membership System

## Joining a Society

### Discovery

Society recruitment contacts are ordinary-looking NPCs in specific rooms. They
give no indication of their society role until the player speaks the correct
passphrase using the `say` command. Passphrases are learned through:

- Lore entries accessible via the `lore` command
- Hints from AI dialogue NPCs who have appropriate `lore_flags`
- Other players (word of mouth — the oldest MUD tradition)
- Exploration of society-related areas (room descriptions containing clues)

A player who says the passphrase in the wrong room, or to the wrong NPC, gets
no response — the phrase is just words. Only the designated recruiter NPC in
the designated room triggers the recruitment path.

### Prerequisites

Each society has minimum requirements checked at recruitment time:

```c
bool can_join_society(CHAR_DATA *ch, int society_id)
{
   /* Must be a player */
   if (IS_NPC(ch) || ch->pcdata == NULL)
      return FALSE;

   /* Must not already be in a society */
   if (ch->pcdata->society != SOCIETY_NONE)
      return FALSE;

   /* Must not have been expelled in the last 30 real days */
   if (ch->pcdata->society_flags & SFLAG_DISAVOWED)
   {
      time_t cooldown = 30 * 24 * 60 * 60;
      if (current_time - ch->pcdata->society_joined < cooldown)
         return FALSE;
   }

   /* Society-specific level requirements */
   switch (society_id)
   {
      case SOCIETY_ASH_CULT:    return ch->level >= 20;
      case SOCIETY_JADE_EELS:   return ch->level >= 25;
      case SOCIETY_QUIET_HAND:  return ch->level >= 25;
      case SOCIETY_RECKONING:   return ch->level >= 30;
      case SOCIETY_JACKAL:      return ch->level >= 30;
      case SOCIETY_PALE_LEDGER: return ch->level >= 35;
      default:                  return FALSE;
   }
}
```

### Initiation

Joining is not instant. After the recruitment dialogue, the player receives an
initiation mission — a simple task that tests commitment and provides the first
society experience. Examples:

| Society | Initiation Task |
|---|---|
| Reckoning | Retrieve a sealed document from a restricted room and deliver it to the shadow archive |
| Jade Eels | Transport a contraband package from Kowloon harbor to a dead drop without being caught |
| Ash Cult | Perform a ritual at a specific Cinderteeth-connected site |
| Quiet Hand | Escort a supply shipment through the Eccentric Woodland to Rakuen |
| Jackal Synod | Recover a specific relic from a tomb and deliver it to the hidden shrine |
| Pale Ledger | Retrieve a Conclave research fragment from a sealed site in the Withered Depths |

On completion:
- `ch->pcdata->society` is set to the society ID
- `ch->pcdata->society_rank` is set to 0 (initiate)
- `ch->pcdata->society_rep` is set to 0
- `ch->pcdata->society_flags` is set to `SFLAG_INITIATED`
- `ch->pcdata->society_joined` is set to `current_time`
- The player receives a society sigil item (see below)

### The Society Sigil

Each society has a unique sigil item — a small object that identifies the
bearer to other society members and to society NPCs. The sigil is:

- Flagged `ITEM_SOCIETY` (new item extra flag)
- Not tradeable (`ITEM_NO_DROP | ITEM_NO_REMOVE` when worn, but can be
  hidden in inventory)
- Required for entry to society headquarters
- Detectable by guard NPCs — carrying it in a hostile city risks exposure

The sigil is not worn in a visible equipment slot. It occupies `WEAR_HIDDEN`
(new wear location, not displayed in `equipment` output). This means the
player must actively choose to carry it. Leaving it in a keep or locker
prevents exposure risk but also prevents HQ access and NPC recognition.

```c
#define WEAR_HIDDEN  MAX_WEAR  /* hidden slot; not shown in equipment list */
#define ITEM_SOCIETY BIT_30    /* item identifies bearer as society member */
```

---

## Rank Advancement

### Requirements

Advancement from rank N to rank N+1 requires:

| Rank | Missions Required | Reputation Required | Additional |
|---|---|---|---|
| 0 → 1 | 3 completed | 50+ | — |
| 1 → 2 | 8 completed | 100+ | Must not be exposed |
| 2 → 3 | 15 completed | 175+ | Must complete a specific rank mission |
| 3 → 4 | 25 completed | 275+ | Must complete a specific rank mission |
| 4 → 5 | 40 completed | 400+ | Must be nominated by a rank-5 member (or NPC handler) |

### Rank Missions

Ranks 3 and 4 require completion of a specific story mission unique to each
society. These are hand-crafted multi-step quests that test the player's
mastery of their society's gameplay loop:

- **Rank 3 mission:** A significant operation within the society's base city
  (e.g., the Reckoning's Breach Tower infiltration, the Jade Eels' harbor
  blockade manipulation, the Ash Cult's floodwork sabotage)
- **Rank 4 mission:** A cross-city operation requiring travel and coordination
  (e.g., the Quiet Hand's first independent trade route establishment, the
  Jackal Synod's inter-city relic pipeline, the Pale Ledger's recovery of a
  major Conclave document)

### Promotion Process

When a player meets all requirements, they can request promotion from their
society's handler NPC:

```
society promote
```

The handler checks requirements and either promotes or explains what is still
needed. Promotion is immediate — no waiting period.

On promotion:
- `society_rank` increments
- A society-wide announcement is sent on the society channel (first name
  only, using the player's rank title)
- New skills, rooms, and mission types become available per the rank tables
  in each society's definition

---

## Leaving a Society

### Voluntary Departure

A player can leave their society at any time:

```
society leave
```

This triggers a confirmation prompt. On confirmation:
- `society` is set to `SOCIETY_NONE`
- `society_rank` is set to 0
- `society_rep` is set to 0
- `society_flags` is cleared
- `society_joined` is set to `current_time` (starts the 30-day rejoin cooldown)
- Society skills become unusable (proficiency remains but skill checks
  auto-fail while `society == SOCIETY_NONE`)
- The society sigil crumbles to dust (object destroyed)

Voluntary departure does not trigger exposure. The society considers the
player a neutral former member — not a threat, not an asset.

### Expulsion (Disavowal)

If a player's reputation drops below -300 (Disgraced), the society expels
them automatically:

- `SFLAG_DISAVOWED` is set
- All effects of voluntary departure apply
- Additionally: the player is flagged as hostile to their former society.
  Former society members can target them for exposure missions, and society
  guard NPCs will attack on sight.

Disavowal lasts 30 real days. After the cooldown, the player can join a
different society (or the same one, if they can find a recruiter willing to
accept them — which requires the recruiter NPC to check for the disavow
history and impose a reputation penalty on re-entry).

### Switching Societies

A player can only belong to one society at a time. To switch:

1. Leave current society (voluntary or expulsion)
2. Wait 30 real days
3. Meet the new society's prerequisites
4. Complete the new society's recruitment and initiation

There is no fast path. Switching societies is a significant decision with a
real time cost. This prevents society-hopping for optimal skill access and
creates meaningful commitment to the chosen organization.

---

## Exposure and Recovery

### Exposure Sources

| Source | Reputation Penalty | Timer Set |
|---|---|---|
| Failed mission (critical failure) | -100 | 720 ticks (~6 hours) |
| Caught in rival HQ | -75 | 480 ticks (~4 hours) |
| Investigated by rival player | -50 | 360 ticks (~3 hours) |
| Society sigil detected by guard | -25 | 240 ticks (~2 hours) |

### During Exposure

While `society_exposed == TRUE`:

- The player's society abbreviation is visible to all players via a new
  field in `who` output (marked with a `[!]` prefix to indicate blown cover)
- City guard NPCs in cities hostile to the player's society may refuse
  services or attack
- The player cannot accept new society missions
- Society skills that require secrecy (disguise, dead drops) auto-fail
- The player's society handler NPC refuses to speak with them

### Recovery

When `society_exposure_timer` reaches 0 (decremented each game tick in
`update_handler()`):

- `society_exposed` is reset to `FALSE`
- The player's society affiliation is hidden again
- All services resume
- Reputation penalty is permanent — must be earned back through missions

### Exposure and Clans

Exposure does **not** affect clan membership or clan diplomacy. A player
exposed as a Jade Eel member does not drag their clan into the conflict. This
separation is by design — societies and clans operate on different planes of
the game's social structure.

---

## The `society` Command

All society interaction goes through a single command with subcommands:

```
society                  — Show your society status (rank, rep, flags)
society info             — Show your society's description and structure
society who              — List online members (rank and title only, no names
                           unless the viewer is rank 3+)
society say <message>    — Send message on the society channel
society missions         — List available missions from your handler
society promote          — Request rank advancement
society leave            — Leave the society
society history          — Show your mission completion history
```

Non-members who type `society` see: *"You are not a member of any secret
society."* No information about which societies exist is revealed.

The command is implemented in `src/act_society.c` (new file) and registered
in the command table in `interp.c`.
