# Secret Societies: Inter-Society Dynamics

## Rivalry System

Each society has one primary rival (defined in `society_table[].rival_society`)
and varying relationships with others (hostile, wary, allied, neutral — see the
rivalry matrix in `03-society-definitions.md`). These relationships drive
inter-society gameplay: espionage, counter-operations, and targeted PvP.

---

## Espionage Mechanics

### Investigation

Players with the `society_sense` skill (rank 3+) can attempt to identify
another player's society affiliation. This is the primary intelligence-
gathering tool between rival societies.

**Process:**
1. Investigator uses `societysense <target>` in the same room
2. Skill check: `number_percent() < investigator.learned[gsn_society_sense]`
3. Modified by target's level difference and whether they have `prism_shield`
   active
4. On success: investigator learns target's society
5. On failure: target is alerted but investigator's identity is not revealed
6. On critical failure (roll > learned + 30): target learns investigator's
   society and name

### Counter-Intelligence

The Pale Ledger's `prism_shield` skill provides direct counter-intelligence —
while active, `society_sense` automatically fails against the shielded player.

All societies can practice passive counter-intelligence by:
- Not carrying their sigil in hostile cities
- Avoiding society HQ areas when rival members are online
- Using the `society who` command to check if rivals are active before
  undertaking sensitive missions

### Intelligence Reports

Rank 3+ members of any society can access intelligence reports via their
handler NPC:

```
society intel
```

This shows:
- Which rival society members have been detected online recently (by any
  member's `society_sense` successes — pooled within the society)
- Recent exposure events involving rival society members
- Areas where rival activity has been detected (based on mission activity)

Intelligence is pooled: when any member successfully uses `society_sense`,
the result is added to the society's intelligence log, accessible to all
rank 3+ members. The log retains entries for 48 real hours before expiring.

---

## Counter-Operations

### Interference Missions

Rank 3+ members can accept missions that directly interfere with rival
society activities:

| Society | Counter-Op Target | Example |
|---|---|---|
| Reckoning | Pale Ledger | Intercept a Conclave document before Pale agents retrieve it |
| Jade Eels | Jackal Synod | Sabotage a relic shipment en route to Mafdet |
| Ash Cult | Quiet Hand | Disrupt a supply caravan the Quiet Hand is escorting |
| Quiet Hand | Ash Cult | Repair sabotaged infrastructure before the damage takes effect |
| Jackal Synod | Jade Eels | Steal a contraband shipment the Eels are transporting |
| Pale Ledger | Reckoning | Suppress a document before Reckoning agents publish it |

These missions target NPC activities, not player-carried items or player
characters directly. The rival society's "operations" are represented by
NPC couriers, shipments, and infrastructure objects that the counter-operative
must intercept or protect.

### Mission Collision

When two players from rival societies accept missions that reference the same
target area or target vnum, a **collision event** may occur:

1. Both players are notified that "rival activity has been detected in the
   area" when they enter the target zone
2. The first player to complete their objective succeeds; the second player's
   mission fails automatically
3. If both players are in the same room, they become aware of each other's
   society affiliation (forced mutual `society_sense` success)
4. PvP becomes available between the two players for 10 minutes (see below)

Collision events are rare — they require two rival players to accept
conflicting missions and reach the target area within the same time window.
When they occur, they create memorable emergent PvP encounters.

---

## PvP Rules

### Society PvP Flag

Society PvP is **opt-in by action**, not by membership. Simply joining a
society does not enable PvP. PvP becomes available between two players when:

1. **Mission collision:** Both players are in the same room during a
   collision event (10-minute PvP window)
2. **Exposure attack:** A player uses `expose` on a rival (PvP window opens
   for 5 minutes if the target wants to retaliate)
3. **Mutual consent:** Both players use `society challenge <target>` to
   agree to a duel (standard duel rules apply)

Outside these conditions, society members cannot attack each other based on
society affiliation alone. This prevents griefing while preserving meaningful
PvP for players who engage with the espionage system.

### PvP Window Implementation

```c
/* On CHAR_DATA (temporary, not saved) */
CHAR_DATA *society_pvp_target;     /* who I can attack */
sh_int     society_pvp_timer;      /* ticks remaining */
```

When a PvP window opens:
- Both players' `society_pvp_target` is set to the other player
- `society_pvp_timer` is set to the duration
- Both players receive a message: *"You sense hostile intent from $N..."*

During the window, `is_safe()` checks in `fight.c` permit attacks between
the two players. When the timer expires, the fields are cleared and normal
safety rules resume.

### PvP Consequences

| Outcome | Winner | Loser |
|---|---|---|
| Kill during collision | +30 rep, +500 gold | -20 rep, standard death penalties |
| Kill during exposure | +20 rep | -30 rep (exposed + killed) |
| Kill during duel | +10 rep | -10 rep |
| Flee during any PvP | No penalty | -5 rep |

PvP kills during society encounters do not count toward the player's normal
`pkills`/`pkilled` stats. They are tracked separately in a new field:

```c
/* In PC_DATA */
int society_kills;   /* rival society members killed in PvP */
```

This stat is visible only in `society` output and contributes to society
reputation tracking.

---

## Cross-Society Alliances

### Allied Societies

Currently, only one alliance exists: **Reckoning + Quiet Hand**. Both seek to
undermine Midgaard's institutional control — the Reckoning through exposure,
the Quiet Hand through economic independence. Allied society members can:

- See each other's society abbreviation in `society who` (if rank 3+)
- Share intelligence reports
- Accept joint missions that benefit both societies
- Cannot attack each other during collision events (collision resolves
  peacefully — first to complete wins, but no PvP window opens)

### Shifting Alliances (Future)

The alliance matrix is stored in a static table for now. A future enhancement
could make alliances dynamic based on aggregate player actions:

- If enough Reckoning members complete missions that also benefit the Quiet
  Hand, the alliance strengthens (new joint mission types unlock)
- If Reckoning members repeatedly act against Quiet Hand interests (e.g.,
  exposing Quiet Hand operatives), the alliance degrades toward Neutral

This is explicitly out of scope for the initial implementation but the data
structures support it (the rivalry matrix could be moved to a runtime table
persisted alongside clan diplomacy data).

---

## Bounty System

### Society Bounties

Rank 4+ members can place bounties on exposed rival society members:

```
society bounty <target> <gold amount>
```

Requirements:
- Target must be currently exposed (`society_exposed == TRUE`)
- Target must be in a hostile society
- Gold is deducted from the bounty placer's bank
- Minimum bounty: 1,000 gold
- Maximum bounty: 50,000 gold

Bounties are visible to all members of the placer's society via:

```
society bounties
```

Any member of the placer's society can claim the bounty by killing the target
during a valid PvP window. The bounty gold is deposited into the killer's
bank. If the target's exposure timer expires before the bounty is claimed,
the bounty is refunded.

### Bounty Data

```c
struct society_bounty_type
{
   char  *target_name;          /* name of the exposed player */
   sh_int target_society;       /* society of the target */
   sh_int placer_society;       /* society that placed the bounty */
   int    gold;                 /* bounty amount */
   time_t placed_time;          /* when the bounty was placed */
   struct society_bounty_type *next;
};

extern SOCIETY_BOUNTY *first_bounty;  /* global linked list */
```

Bounties are not saved to disk. They exist only while the target is exposed
(timer-bounded) and are cleared when:
- The target's exposure timer expires
- The bounty is claimed (target killed)
- The bounty placer cancels it (`society bounty cancel <target>`)

---

## Information Asymmetry

The inter-society system is designed around information asymmetry — knowing
more than your rivals is the primary competitive advantage, not combat stats.

| Information | Who Knows It |
|---|---|
| Your society membership | Only you, your society members (rank 3+), and anyone who successfully investigates you |
| Your society rank | Only you and your handler NPC |
| Your active mission | Only you |
| Rival activity in an area | Your society's intelligence pool (rank 3+) |
| A rival's exposure status | Everyone (exposure is public) |
| Active bounties | Only the placing society's members |

This creates interesting gameplay dynamics:
- A rank-2 player cannot tell if the person next to them is a rival agent
- A rank-5 player can see online society members but not their missions
- An exposed player knows they are visible but not who placed the bounty
- Mission collision reveals rivals to each other but not to bystanders
