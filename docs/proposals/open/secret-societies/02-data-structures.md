# Secret Societies: Data Structures

## Constants

```c
#define MAX_SOCIETY        7   /* 0 = none, 1-6 = the six societies          */
#define MAX_SOCIETY_RANK   6   /* ranks 0-5 (0 = initiate, 5 = inner circle) */
#define MAX_SOCIETY_REP  500   /* reputation cap (positive and negative)      */

#define SOCIETY_NONE       0
#define SOCIETY_RECKONING  1   /* Midgaard — archive exposure                */
#define SOCIETY_JADE_EELS  2   /* Kowloon — smuggling syndicate              */
#define SOCIETY_ASH_CULT   3   /* Kowloon/Rakuen — sabotage and ritual       */
#define SOCIETY_QUIET_HAND 4   /* Rakuen — separatist diplomacy              */
#define SOCIETY_JACKAL      5   /* Mafdet — relic trafficking                 */
#define SOCIETY_PALE_LEDGER 6   /* Kiess/Cinderteeth — forbidden arcana       */
```

These are defined in `typedefs.h` alongside existing constants.

---

## Society Table (Static Definition)

The society table follows the same pattern as `clan_table[]` in `const_clan.c`
— a static array of structs defining each society's properties.

```c
struct society_type
{
   char *name;              /* Full name: "The Reckoning"                  */
   char *abbr;              /* Five-char abbreviation: "RECKN"             */
   sh_int base_city;        /* Primary city constant (CITY_MIDGAARD, etc.) */
   sh_int hq_room;          /* Vnum of hidden headquarters room            */
   sh_int recruit_room;     /* Vnum of recruitment contact room            */
   sh_int rival_society;    /* Society ID of primary rival (0 = none)      */
   char *passphrase;        /* Spoken passphrase to reveal recruiter NPC   */
   int   skill_first;       /* First skill index in society skill block    */
   int   skill_count;       /* Number of society-specific skills           */
};

extern const struct society_type society_table[MAX_SOCIETY];
```

**File:** `src/const_society.c` (new file, following `const_clan.c` pattern)

The `passphrase` field is the string a player must `say` in the `recruit_room`
to trigger the recruitment NPC's dialogue. This prevents accidental discovery —
players must learn the phrase through lore, other players, or NPC hints.

---

## Player Society Data

Society membership data is stored on `PC_DATA`, parallel to how clan membership
is stored.

### New Fields on PC_DATA

```c
/* In PC_DATA (ack.h) */
sh_int society;              /* Society ID (0 = none, 1-6 = society)       */
sh_int society_rank;         /* Rank within society (0-5)                  */
int    society_rep;          /* Reputation (-500 to 500)                   */
int    society_flags;        /* Bitfield for society-specific state        */
int    society_missions;     /* Lifetime completed mission count           */
time_t society_joined;       /* Timestamp of joining (for cooldowns)       */
bool   society_exposed;      /* TRUE if identity has been blown            */
sh_int society_exposure_timer; /* Ticks remaining on exposure penalty      */
```

### Society Flags

```c
/* Society-specific state flags (PC_DATA->society_flags) */
#define SFLAG_INITIATED      (1 << 0)  /* Completed initiation ritual       */
#define SFLAG_HAS_SIGIL      (1 << 1)  /* Carries society sigil item        */
#define SFLAG_ON_MISSION     (1 << 2)  /* Currently on a society mission    */
#define SFLAG_DISAVOWED      (1 << 3)  /* Society has disavowed this member */
#define SFLAG_DEAD_DROP_SET  (1 << 4)  /* Has an active dead drop placed    */
#define SFLAG_COVER_ACTIVE   (1 << 5)  /* Disguise/cover identity active    */
```

---

## Society Reputation

Reputation is a signed integer ranging from -500 to +500. It tracks the
player's standing within their society.

| Range | Label | Effect |
|---|---|---|
| -500 to -300 | Disgraced | Marked for elimination; society skills disabled |
| -299 to -100 | Suspect | Reduced mission availability; watched |
| -99 to 99 | Neutral | Standard access |
| 100 to 299 | Trusted | Access to sensitive missions |
| 300 to 499 | Honored | Access to inner circle missions; can recruit |
| 500 | Exalted | Maximum standing; society leadership tier |

Reputation changes:
- Completing missions: +10 to +50 depending on difficulty
- Failing missions: -5 to -30 depending on severity
- Being exposed: -100 immediate penalty
- Betraying society secrets: -200
- Recruiting a new member: +25
- Time-based decay: reputation above 300 decays by 1 per game day toward 300
  (prevents indefinite accumulation without activity)

---

## Exposure System

The exposure mechanic is central to the risk/reward design.

### How Exposure Happens

1. **Failed mission** — certain mission failure conditions set
   `society_exposed = TRUE`
2. **Caught in restricted area** — entering a society HQ while non-member and
   being seen by a society guard NPC
3. **Player report** — a player with the `investigate` society skill can
   attempt to expose another player's society affiliation (skill check)
4. **Carrying society items in public** — society sigils and tools are flagged
   `ITEM_SOCIETY`; if detected by a guard NPC, exposure triggers

### Exposure Consequences

When `society_exposed` is set to `TRUE`:

- `society_exposure_timer` is set to 720 ticks (~6 real hours)
- The player's society abbreviation becomes visible to city guard NPCs in the
  society's rival cities
- Guard NPCs in hostile cities may attack on sight or refuse services
- The player cannot accept new society missions until the timer expires
- Society reputation takes a -100 penalty
- If reputation drops below -300, the society sets `SFLAG_DISAVOWED` and the
  player is expelled

### Recovery

When `society_exposure_timer` reaches 0:
- `society_exposed` is reset to `FALSE`
- Guard NPC hostility returns to normal
- Missions become available again
- Reputation penalty remains (must be earned back through missions)

---

## NPC Society Awareness

Society NPCs need to recognize members. This uses new fields on
`MOB_INDEX_DATA`:

```c
/* In MOB_INDEX_DATA (ack.h) */
sh_int society;           /* Society this NPC belongs to (0 = none)       */
sh_int society_npc_role;  /* Role: recruiter, handler, merchant, guard    */
```

### NPC Roles

```c
#define SOCIETY_NPC_NONE       0
#define SOCIETY_NPC_RECRUITER  1  /* Initiates recruitment dialogue           */
#define SOCIETY_NPC_HANDLER    2  /* Assigns missions, reports status         */
#define SOCIETY_NPC_MERCHANT   3  /* Sells society-specific items to members  */
#define SOCIETY_NPC_GUARD      4  /* Protects HQ; attacks non-members         */
#define SOCIETY_NPC_CONTACT    5  /* Field contact; provides hints/intel      */
```

When a player interacts with a society NPC (via `say` or entering the room),
the NPC checks `ch->pcdata->society` against its own `society` field. Members
get full interaction; non-members see a mundane NPC with no special behavior.

---

## Save Format

Society data is persisted in the player save file. This requires incrementing
`SAVE_REVISION` from 15 to 16.

### Save Block

```
Society      3
SocRank      2
SocRep       175
SocFlags     3
SocMissions  14
SocJoined    1742400000
SocExposed   0
SocExpTimer  0
```

### Save/Load Functions

In `save_players.c` (`fwrite_char`):

```c
if (ch->pcdata->society > 0)
{
   fprintf(fp, "Society      %d\n", ch->pcdata->society);
   fprintf(fp, "SocRank      %d\n", ch->pcdata->society_rank);
   fprintf(fp, "SocRep       %d\n", ch->pcdata->society_rep);
   fprintf(fp, "SocFlags     %d\n", ch->pcdata->society_flags);
   fprintf(fp, "SocMissions  %d\n", ch->pcdata->society_missions);
   fprintf(fp, "SocJoined    %ld\n", (long)ch->pcdata->society_joined);
   fprintf(fp, "SocExposed   %d\n", ch->pcdata->society_exposed);
   fprintf(fp, "SocExpTimer  %d\n", ch->pcdata->society_exposure_timer);
}
```

In `save_players.c` (`fread_char`):

The reader matches each `Soc*` key and populates the corresponding `PC_DATA`
field. Unknown keys are skipped (forward compatibility). On `SAVE_REVISION < 16`,
all society fields default to 0/FALSE.

---

## Runtime Data

### Society Mission State

Active missions use the existing `QUEST_DATA` infrastructure on `PC_DATA`. A
society mission is a quest with `quest_type == QUEST_TYPE_SOCIETY` (new constant).
The `SFLAG_ON_MISSION` flag is set when a mission is accepted and cleared on
completion or failure.

No new struct is needed for mission tracking — the quest system already supports
target vnums, completion flags, and expiry timers.

### Society Communication Channel

Members of the same society share a covert communication channel, implemented
as a new channel flag:

```c
#define CHANNEL_SOCIETY  (1 << 18)  /* society-only channel */
```

The `society` command includes a `say` subcommand:

```
society say <message>
```

This broadcasts to all online members of the player's society, similar to clan
chat but with no public visibility. The channel name is not shown in `channels`
output to non-members.

---

## Memory Layout Summary

| Location | New Fields | Size Impact |
|---|---|---|
| `PC_DATA` | 8 fields (society through exposure_timer) | ~28 bytes per player |
| `MOB_INDEX_DATA` | 2 fields (society, society_npc_role) | ~4 bytes per mob template |
| `society_table[]` | Static array of 7 entries | ~500 bytes total (constant) |
| Player save file | 8 new lines per society member | ~120 bytes per save |

The memory impact is negligible. Society data is only meaningful for players who
opt in; non-members carry zeroed fields that cost storage but no processing.
