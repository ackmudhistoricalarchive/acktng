# Secret Societies: Mission System

## Overview

Society missions are the primary advancement mechanism. They provide reputation,
unlock rank progression, and drive the gameplay loops defined for each society.
Missions use the existing `QUEST_DATA` infrastructure on `PC_DATA` with a new
quest type constant.

---

## Mission Architecture

### Quest Type

```c
#define QUEST_TYPE_SOCIETY  3  /* society mission (alongside existing types) */
```

A society mission occupies one of the 5 `QUEST_DATA` slots in
`PC_DATA->quests[]`. This means a player can have a society mission active
alongside normal quests. The `SFLAG_ON_MISSION` flag is set when a society
mission is accepted and cleared on completion or failure.

### Mission Templates

Missions are defined in a static table loaded at boot, similar to quest
templates. Each mission template specifies:

```c
struct society_mission_type
{
   int    id;                           /* unique mission ID                  */
   sh_int society;                      /* which society offers this          */
   sh_int min_rank;                     /* minimum rank to accept             */
   sh_int min_rep;                      /* minimum reputation to accept       */
   sh_int difficulty;                   /* 1-5 scale                          */
   int    rep_reward;                   /* reputation gained on success       */
   int    rep_penalty;                  /* reputation lost on failure          */
   int    gold_reward;                  /* gold paid on success               */
   bool   exposure_on_fail;            /* does failure expose the player?     */
   int    time_limit;                   /* ticks before auto-failure (0=none) */
   char  *name;                         /* mission name shown to player       */
   char  *desc;                         /* brief description                  */
   sh_int objective_type;              /* what the player must do             */
   int    target_vnum;                 /* mob, obj, or room vnum for target   */
   int    delivery_vnum;               /* room or mob vnum for delivery       */
};
```

### Objective Types

```c
#define MISSION_OBJ_DELIVER    0  /* Deliver an item to a location/NPC     */
#define MISSION_OBJ_RETRIEVE   1  /* Retrieve an item from a location      */
#define MISSION_OBJ_KILL       2  /* Kill a specific mob                   */
#define MISSION_OBJ_VISIT      3  /* Visit a specific room                 */
#define MISSION_OBJ_SABOTAGE   4  /* Use sabotage skill on a target        */
#define MISSION_OBJ_SMUGGLE    5  /* Transport item without guard detection*/
#define MISSION_OBJ_TALK       6  /* Speak to a specific NPC               */
#define MISSION_OBJ_RITUAL     7  /* Perform a ritual at a specific room   */
```

---

## Mission Flow

### Accepting a Mission

1. Player visits their society handler NPC
2. Player types `society missions` to see available missions
3. Available missions are filtered by rank, reputation, and whether the player
   is already on a society mission
4. Player types `society accept <number>` to accept
5. The handler NPC describes the mission objective through dialogue
6. A `QUEST_DATA` entry is created with `quest_type = QUEST_TYPE_SOCIETY`
7. `SFLAG_ON_MISSION` is set
8. If the mission has a time limit, the quest timer starts

### Completing a Mission

Completion is triggered when the objective condition is met:

- **DELIVER:** Player gives the mission item to the delivery NPC or drops it
  in the delivery room
- **RETRIEVE:** Player picks up the target item (auto-spawned when mission
  is accepted in the target room)
- **KILL:** Target mob dies while player is in the room
- **VISIT:** Player enters the target room
- **SABOTAGE:** Player uses `sabotage` skill on the target object/room
- **SMUGGLE:** Player carries the mission item into the delivery room without
  having been flagged by any guard NPC en route
- **TALK:** Player uses `say` in the presence of the target NPC
- **RITUAL:** Player uses a ritual skill in the target room

On completion:
- Reputation increases by `rep_reward`
- Gold is deposited in the player's bank (`ch->balance`)
- `society_missions` counter increments
- `SFLAG_ON_MISSION` is cleared
- The quest slot is freed
- A message is sent on the society channel acknowledging completion

### Failing a Mission

Failure occurs when:
- The time limit expires
- The player dies during the mission (optional per-mission flag)
- A guard NPC detects contraband during a smuggling mission
- The player is caught in a restricted area during an infiltration mission

On failure:
- Reputation decreases by `rep_penalty`
- `SFLAG_ON_MISSION` is cleared
- The quest slot is freed
- If `exposure_on_fail` is set, `society_exposed = TRUE` and the exposure
  timer starts
- A message is sent to the player (not on the society channel — failures
  are private)

### Abandoning a Mission

```
society abandon
```

The player can abandon an active mission at any time. This counts as a minor
failure:
- Half the `rep_penalty` is applied
- No exposure is triggered
- The quest slot is freed
- A 30-minute cooldown before accepting a new mission

---

## Mission Categories by Society

### The Reckoning

| Difficulty | Mission Type | Example |
|---|---|---|
| 1 | Courier | Deliver a sealed document between two archive contacts |
| 2 | Retrieval | Copy a document from a restricted archive room |
| 3 | Infiltration | Enter the Breach Tower and retrieve a specific ledger page |
| 4 | Informant | Speak to a specific NPC contact in Kowloon or Rakuen |
| 5 | Exposure | Plant evidence in a public location; evade guard detection |

### The Jade Eels

| Difficulty | Mission Type | Example |
|---|---|---|
| 1 | Smuggling | Transport a package from harbor to dead drop |
| 2 | Bribery | Pay off a specific NPC guard (requires gold + bribe skill) |
| 3 | Route disruption | Sabotage a rival trade shipment in transit |
| 4 | Market manipulation | Use price_fix on a specific shop for a set duration |
| 5 | Embargo | Establish a trade embargo affecting a target NPC shop |

### The Ash Cult

| Difficulty | Mission Type | Example |
|---|---|---|
| 1 | Marking | Place ashmarks in a series of rooms across a district |
| 2 | Ritual | Perform kindle at a specific site during a specific time |
| 3 | Sabotage | Disable a target infrastructure object (door, lock, bridge) |
| 4 | Disruption | Create an ashfall effect in a populated area |
| 5 | Destruction | Destroy a specific target object using pyroclasm |

### The Quiet Hand

| Difficulty | Mission Type | Example |
|---|---|---|
| 1 | Escort | Protect a supply NPC walking between two points |
| 2 | Diplomacy | Deliver a diplomatic pouch to a contact in another city |
| 3 | Route building | Establish a cache at a wilderness waypoint |
| 4 | Negotiation | Use negotiate on a specific NPC to unlock a dialogue path |
| 5 | Trade route | Establish a new trade route between two cities |

### The Jackal Synod

| Difficulty | Mission Type | Example |
|---|---|---|
| 1 | Scouting | Visit a tomb area and report what you find (visit rooms) |
| 2 | Recovery | Retrieve a specific relic from a tomb |
| 3 | Smuggling | Transport a relic through Quarantine Ledger (avoid detection) |
| 4 | Excavation | Use seal_break on a sealed tomb door, then retrieve contents |
| 5 | Deep recovery | Multi-step: enter deep tomb, solve obstacle, retrieve artifact |

### The Pale Ledger

| Difficulty | Mission Type | Example |
|---|---|---|
| 1 | Survey | Attune to crystal deposits in a Cinderteeth area |
| 2 | Research | Trace spirit energy in a series of Withered Depths rooms |
| 3 | Recovery | Retrieve a Conclave document using conclave_cipher to read it |
| 4 | Experimentation | Extract essences from specific sources for laboratory use |
| 5 | Suppression | Intercept a Reckoning agent (NPC) before they publish evidence |

---

## Mission Generation

Missions are not randomly generated. They are selected from the template table
based on the player's rank, reputation, and what missions they have already
completed. The selection algorithm:

```c
/* Pseudocode for mission list generation */
for each template in society_mission_templates:
   if template.society != player.society:      skip
   if template.min_rank > player.society_rank: skip
   if template.min_rep > player.society_rep:   skip
   if player has completed this template in the last 24 hours: skip
   add to available list

sort available by difficulty ascending
present up to 5 missions to the player
```

The 24-hour cooldown per template prevents grinding the same easy mission
repeatedly. Combined with the reputation requirements for advancement, this
ensures players engage with progressively harder content.

---

## Mission Rewards

### Reputation

| Difficulty | Success | Failure | Abandon |
|---|---|---|---|
| 1 | +10 | -5 | -3 |
| 2 | +20 | -10 | -5 |
| 3 | +30 | -15 | -8 |
| 4 | +40 | -20 | -10 |
| 5 | +50 | -30 | -15 |

### Gold

| Difficulty | Gold Reward |
|---|---|
| 1 | 500 |
| 2 | 1,500 |
| 3 | 3,000 |
| 4 | 5,000 |
| 5 | 10,000 |

Gold rewards are deposited directly into the player's bank balance to avoid
the risk of dropping gold on death. The amounts are calibrated to be
meaningful but not game-breaking — society membership supplements income
rather than replacing normal gold acquisition.

### Unique Items

Certain high-difficulty missions (rank 4-5) reward unique items that cannot
be obtained elsewhere:

- Society-themed equipment with cosmetic effects (glowing sigils, unique
  short descriptions)
- Consumable items produced by society skills (essence vials, ritual
  components)
- Keys that grant one-time access to restricted rooms

These items are flagged `ITEM_SOCIETY` and `ITEM_NO_DROP` to prevent them
from entering the general economy.

---

## Mission Timer and Updates

Active society missions are checked in `update_handler()` alongside existing
quest updates:

```c
void society_mission_update(void)
{
   CHAR_DATA *ch;

   for (ch = first_player; ch != NULL; ch = ch->next_player)
   {
      if (ch->pcdata == NULL)
         continue;
      if (!IS_SET(ch->pcdata->society_flags, SFLAG_ON_MISSION))
         continue;

      /* Find the active society quest slot */
      for (int i = 0; i < QUEST_MAX_QUESTS; i++)
      {
         if (ch->pcdata->quests[i].quest_type != QUEST_TYPE_SOCIETY)
            continue;
         if (ch->pcdata->quests[i].time_limit > 0)
         {
            ch->pcdata->quests[i].time_limit--;
            if (ch->pcdata->quests[i].time_limit <= 0)
            {
               society_mission_fail(ch, i, "Time has run out!");
            }
         }
         break;
      }
   }
}
```

This function is called once per game tick from `update_handler()`, keeping
the mission timer in sync with the existing tick-based update system.
