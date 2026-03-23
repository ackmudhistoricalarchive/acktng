# Secret Societies: Society Skills

## Design Principles

Society skills are **non-combat utility abilities**. They do not modify
`hitroll`, `damroll`, `armor`, `max_hit`, `max_mana`, or saving throws. They
provide tools for espionage, deception, information gathering, and covert
operations. A player with maxed society skills is no more powerful in a
straight fight than one without — but they have more options outside of combat.

Society skills are stored in the existing `PC_DATA->learned[]` array using
reserved skill slots. Each society has a block of 6 skill indices assigned via
`society_table[].skill_first` and `society_table[].skill_count`. The skills
are defined in `skill_table[]` with `skill_level[]` set to `NO_USE` for all
classes — they are unlocked by society rank, not by class level.

### Skill Access by Rank

Each skill within a society's block has a minimum rank requirement:

| Skill Slot | Min Rank | Description |
|---|---|---|
| 0 | 0 (Initiate) | Basic society skill, available on joining |
| 1 | 1 | Rank 1 skill |
| 2 | 2 | Rank 2 skill |
| 3 | 3 | Rank 3 skill |
| 4 | 4 | Rank 4 skill |
| 5 | 5 | Rank 5 (inner circle) skill |

Skills are learned at the society handler NPC using `practice`, just like
normal skills. The handler only teaches skills the player's rank permits.
Proficiency increases with use, following the standard `learned[]` percentage
system.

### Skill Check Function

```c
/* Returns TRUE if ch can use society skill sn */
bool can_use_society_skill(CHAR_DATA *ch, int sn)
{
   int soc, slot;

   if (IS_NPC(ch) || ch->pcdata == NULL)
      return FALSE;

   soc = ch->pcdata->society;
   if (soc == SOCIETY_NONE)
      return FALSE;

   /* Verify sn is within this society's skill block */
   if (sn < society_table[soc].skill_first ||
       sn >= society_table[soc].skill_first + society_table[soc].skill_count)
      return FALSE;

   /* Check rank requirement */
   slot = sn - society_table[soc].skill_first;
   if (ch->pcdata->society_rank < slot)
      return FALSE;

   /* Disavowed members cannot use skills */
   if (IS_SET(ch->pcdata->society_flags, SFLAG_DISAVOWED))
      return FALSE;

   return TRUE;
}
```

---

## Shared Skills (All Societies)

Two skills are available to all society members regardless of which society
they belong to. These are stored at fixed skill slots outside the per-society
blocks.

### Dead Drop (`dead_drop`)

**Rank Required:** 1
**Command:** `deaddrop <item> [room]`

Place an item in a hidden location in the current room that only society
members can retrieve. The item becomes invisible to non-members. Another
society member in the same room can `get` the item by name — it appears in
their `look` output with a `(hidden)` tag.

Dead drops expire after 2 real hours (removed by `update_handler()`). Only
one dead drop per player can be active at a time (`SFLAG_DEAD_DROP_SET`).

Implementation: creates a special container object in the room with the
`ITEM_SOCIETY` flag and the placing player's society ID stored in `value[0]`.
The `look` and `get` handlers check the viewer's society against `value[0]`.

### Society Sense (`society_sense`)

**Rank Required:** 3
**Command:** `societysense <target>`

Attempt to determine if another player in the room is a member of any secret
society. This is a skill check — success depends on `learned[sn]` proficiency,
modified by the target's level and whether they have an active cover (see
below).

On success: the player learns the target's society (but not rank).
On failure: no information, and the target receives a message (*"You feel
someone studying you carefully."*) alerting them to the attempt.
On critical failure: the target learns the attempter's society and name.

This skill cannot be used in combat. Cooldown: 10 minutes.

---

## The Reckoning Skills

| Slot | Skill | Rank | Description |
|---|---|---|---|
| 0 | `cipher_read` | 0 | Decode encrypted text on objects and room extras. Reveals hidden messages in areas with Reckoning-relevant content. |
| 1 | `archive_search` | 1 | In rooms flagged as archives or libraries, reveals hidden room extras containing lore fragments. Success chance based on proficiency. |
| 2 | `forge_document` | 2 | Create a forged document object (custom short desc) that can be planted in rooms or given to NPCs. Used in mission objectives. |
| 3 | `informant_network` | 3 | Once per hour, receive a tip about a random active mission target's location (room vnum area). Works cross-city. |
| 4 | `shadow_copy` | 4 | Copy the contents of a sealed object (book, scroll, document) without destroying the original. Creates a duplicate object. |
| 5 | `expose` | 5 | Target a player you suspect is in the rival society (Pale Ledger). If correct, forces their exposure. Reputation cost on failure. |

---

## The Jade Eels Skills

| Slot | Skill | Rank | Description |
|---|---|---|---|
| 0 | `assess_cargo` | 0 | Identify the true value and restricted status of an item. Reveals whether an item would trigger guard detection. Unlike the general `appraise` skill (which shows item stats), this focuses on smuggling risk and contraband classification. |
| 1 | `bribe` | 1 | Attempt to bribe an NPC shopkeeper or guard. On success, the NPC ignores one instance of contraband or trespass. Costs gold. |
| 2 | `smuggle` | 2 | Conceal one item in inventory from guard NPC detection and the `inventory` display of other players. Item is still usable. |
| 3 | `black_market` | 3 | Summon a temporary shadow stash in the current room (if private). Contains restricted items available to the summoner. Items cost gold to take. 1-hour cooldown. |
| 4 | `price_fix` | 4 | Temporarily alter the buy prices at an NPC shop in the current room. Lasts 30 minutes. Useful for mission objectives. |
| 5 | `trade_embargo` | 5 | Mark a specific NPC shop as "embargoed" — it refuses to deal with members of a specified rival society. Lasts 1 hour. |

---

## The Ash Cult Skills

| Slot | Skill | Rank | Description |
|---|---|---|---|
| 0 | `ashmark` | 0 | Leave an invisible cult symbol in a room. Other Ash Cult members see it. Used to mark safe routes and warn of danger. |
| 1 | `kindle` | 1 | Light a small fire in the current room. Cosmetic effect, but required for ritual missions and damages wooden objects left on the ground. |
| 2 | `sabotage` | 2 | Attempt to disable a door, lock, or mechanical device in the room. On success, the object becomes non-functional for 30 minutes. |
| 3 | `ash_ward` | 3 | Place a ward on a room exit. The next non-Ash-Cult player to pass through receives a brief disorientation (2 rounds of lag). One-use. |
| 4 | `invoke_cinder` | 4 | Perform a ritual that creates a temporary "ashfall" weather effect in the current area for 10 minutes. Reduces visibility, adding atmosphere. |
| 5 | `pyroclasm` | 5 | Destroy a single non-artifact object in the room. The only society skill with a destructive effect. Cannot target player equipment or carried items — only ground objects. Used for sabotage missions. |

---

## The Quiet Hand Skills

| Slot | Skill | Rank | Description |
|---|---|---|---|
| 0 | `trailmark` | 0 | Mark the current room as a waypoint. Quiet Hand members can see a faint trail leading toward the nearest trailmark. Navigation aid. |
| 1 | `diplomatic_pouch` | 1 | Seal an item in a diplomatic pouch that cannot be opened by anyone except the intended recipient (specified by player name). |
| 2 | `safe_passage` | 2 | Temporarily suppress aggressive behavior of guard NPCs in the current room toward the player. Lasts 5 minutes. |
| 3 | `establish_cache` | 3 | Create a persistent supply cache in the current room (if outdoors/wilderness). Functions as a small container accessible to all Quiet Hand members. Lasts 24 hours. |
| 4 | `negotiate` | 4 | Improve an NPC's disposition toward the player temporarily. Succeeds based on proficiency. Can unlock dialogue options that normally require higher reputation. |
| 5 | `trade_route` | 5 | Establish a virtual trade route between two rooms the player has visited. Once per day, generates a small gold income deposited in the player's bank balance. Maximum 3 active routes. |

---

## The Jackal Synod Skills

| Slot | Skill | Rank | Description |
|---|---|---|---|
| 0 | `relic_sense` | 0 | Detect the presence of artifact-flagged objects within 3 rooms in any direction. Gives direction only, not specifics. |
| 1 | `false_manifest` | 1 | Alter the apparent item type and name of an object in inventory. The disguise breaks on `identify` or `lore` but fools casual inspection. |
| 2 | `tomb_ward` | 2 | Bypass a single trap on a container or door in the current room. More reliable than standard `disarm trap` for tomb-specific traps. |
| 3 | `ancestor_query` | 3 | In rooms flagged as tombs or shrines, invoke a brief vision that reveals a clue about the area (exit directions, trap locations, hidden rooms). |
| 4 | `seal_break` | 4 | Open a magically sealed container or door that resists normal `pick` or `open`. One-use per seal. Required for high-level tomb missions. |
| 5 | `eclipsed_tongue` | 5 | Communicate with undead NPCs as if they were living. Opens dialogue options with spectral and undead mobs that normally cannot be spoken to. |

---

## The Pale Ledger Skills

| Slot | Skill | Rank | Description |
|---|---|---|---|
| 0 | `crystal_attune` | 0 | Attune to a Conclave crystal deposit in the room. Reveals its properties and whether it is safe to handle. Required for experimentation missions. |
| 1 | `spirit_trace` | 1 | Detect residual spirit energy in the current room. Reveals whether spirit-extraction was performed here and how long ago. |
| 2 | `prism_shield` | 2 | Activate a crystal to create a temporary ward against scrying. For 10 minutes, `society_sense` cannot detect the player. |
| 3 | `extract_essence` | 3 | Extract a magical essence from a corpse or magical object. Creates a consumable item that provides a temporary non-combat buff (e.g., enhanced vision, spirit sight). |
| 4 | `conclave_cipher` | 4 | Read Conclave-encoded inscriptions found in Cinderteeth and Withered Depths areas. Required for high-level research missions. |
| 5 | `pale_gate` | 5 | Open a temporary portal to the Pale Ledger headquarters from any room in the Cinderteeth or Withered Depths areas. One-use per day. Requires a crystal focus item. |

---

## Skill Registration

All society skills are registered in `skill_table[]` with:

- `skill_level[*]` = `NO_USE` for all 24 classes
- `can_learn` = `TRUE`
- `spell_fun` = `spell_null` (no spell function — these are command-driven)
- `target` = `TAR_IGNORE`
- `min_mana` = 0 (society skills do not cost mana)
- A new field or flag marks them as society skills so the `practice` command
  can filter appropriately

Each skill's implementation function follows the pattern:

```c
void do_cipher_read(CHAR_DATA *ch, char *argument)
{
   int sn = gsn_cipher_read;

   if (!can_use_society_skill(ch, sn))
   {
      send_to_char("You don't know how to do that.\n\r", ch);
      return;
   }

   /* skill implementation */
}
```

The `gsn_*` global variables are defined in `globals.h` and populated by
`skill_lookup()` at boot time, following the existing pattern for all other
skills in the codebase.

---

## Skill Proficiency and Practice

Society skills use the same `learned[]` proficiency system as all other
skills:

- Starting proficiency: 1%
- Improved by: using the skill successfully, or practicing at the handler NPC
- Maximum proficiency: 95% (same cap as other skills)
- Skill checks: `number_percent() < ch->pcdata->learned[sn]`

Practice sessions spent on society skills are deducted from the same
`ch->practice` pool used for class skills. This creates a meaningful trade-off
— practice sessions spent on society utilities are not available for combat
skill improvement.
