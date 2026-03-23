# Proposal: Public Societies System

## Overview

Public societies are open, well-known civic institutions that players can join.
Where secret societies operate in shadow with hidden membership and deniable
operations, public societies are the visible pillars of city life — military
commands, trade guilds, religious orders, and civic services that everyone knows
about and that openly recruit members.

A player may belong to one clan, one public society, **and** one secret society
simultaneously. The three systems are independent layers of affiliation:

| System | Visibility | Discovery | Risk | Progression |
|---|---|---|---|---|
| **Clan** | Public | Player-created, invitation | War system | Political rank |
| **Public Society** | Public | Visit a hall, meet requirements | None | Institutional rank |
| **Secret Society** | Hidden | Passphrase, lore clues | Exposure, disavowal | Covert rank |

Public societies occupy the space between clans (player-run political entities)
and secret societies (covert NPC-run organizations). They are NPC-run
institutions with structured advancement, civic skills, and task-board missions
— the professional guilds and service orders that keep the five cities
functioning.

---

## Design Goals

### 1. Open Identity

A player's public society membership is visible in `who`, `score`, `finger`,
and `whois`. There is no secrecy — these are legitimate institutions. The
player's rank title appears alongside their name, communicating their standing
to everyone.

### 2. Lore Integration

Every public society is an institution already documented in the city lore
files. The Guard Command, the Harbor Syndics, the Ember Wardens, the Shrine of
the First Claw, the Wall Command, and the Road Wardens all exist in the world
already. This proposal makes them player-joinable.

### 3. Civic Gameplay

Each society offers a gameplay loop centered on its institutional function:
military patrols, trade contracts, disaster response, justice enforcement,
frontier defense, or inter-city logistics. These are straightforward,
legitimate activities — no smuggling, no sabotage, no exposure risk.

### 4. Rank-Gated Skills

Each public society provides 6 unique skills. Skills unlock automatically as
the player's rank score crosses defined thresholds — no promotion request
needed. If rank drops below a threshold, the corresponding skill becomes
unavailable until rank recovers. **Leaving the society causes all society
skills to become immediately unusable.** This makes society membership a
meaningful, ongoing commitment: the skills are the reason to stay, and losing
them is the cost of leaving.

Public society skills are primarily practical utility: field medicine, equipment
repair, navigation, appraisal, and logistics. Each society also provides one
**passive** combat skill (unlocked at rank 3 / Officer tier) that is always
active during combat with no activation command or cooldown — like how extra
attack passively grants bonus autoattacks. These passive combat skills hook
into existing combat functions (`check_avoidance()`, `calculate_damage()`,
`multi_hit()`, `get_dodge()`) and provide small, meaningful bonuses: a chance
for a bonus autoattack, flat damage reduction, improved dodge, post-block
damage mitigation, in-combat HP regeneration, or evasion piercing against
marked targets.

At the highest rank (Commander tier, rank 400+), each society unlocks one
**active** combat skill — a player-typed command usable during combat. These
follow the standard skill pattern: `can_use_skill()` gate, proficiency roll,
`do_damage()` call with the skill's `gsn_*` as damage type, optional
`affect_to_char()` for buffs/debuffs, a `WAIT_STATE` action delay, and a
20-round `cooldown[]` recharge timer. Each is
thematically tied to its society's identity: a rallying group buff (Guard
Command), a heavy debuffing strike (Harbor Syndics), a fire DOT (Ember
Wardens), a mark-dependent finisher (Shrine of the First Claw), a
shield-based stun (Wall Command), or an accuracy-reducing ambush strike (Road
Wardens). These are powerful but not class-defining — they reward sustained
commitment to a single society at maximum rank.

Where thematically appropriate, Commander-tier skills integrate with existing
combat subsystems rather than standing alone:

- **Combo system:** `command_presence` (Guard Command), `anchor_slam` (Harbor
  Syndics), and `rampart_slam` (Wall Command) are combo builders — they feed
  into the `ch->combo[]` array via `combo(ch, victim, gsn)` just like bash,
  kick, and other war-attack skills. `claws_judgment` (Shrine of the First
  Claw) is a combo finisher — it requires `is_valid_finisher()` (like fleche
  and holystrike) and triggers the finisher chain sequence on hit, giving
  Shrine members a unique way to close out a combo that is gated behind both
  combo readiness and an active `oath_mark`.
- **Chi system:** `controlled_burn` (Ember Wardens) has an optional chi
  enhancement — if the player has chi access (`get_chi(ch) >= 5`), they may
  spend 5 chi to double the DOT duration. This rewards martial artist / Ember
  Warden cross-builds without penalizing non-chi users.
- **Positional system:** `waylay` (Road Wardens) uses the same positional
  gate as `circle` from the backstab system — it can only be used when the
  target is fighting someone else. This gives it the ambush identity of a
  flanking strike without duplicating backstab's mechanics.

### 5. One Society, One Commitment

A player can join **one and only one** public society at a time. This is the
core constraint that gives the choice weight. Different societies offer
different skill sets, different task types, and different institutional
benefits. Choosing the Guard Command means not having access to the Harbor
Syndics' trade skills or the Road Wardens' travel abilities. The choice
defines a playstyle.

Switching is possible but costly: leave the current society (losing all skills
and benefits immediately), wait 7 days, then join another and rebuild rank
from scratch.

### 6. Cooperative Structure

Public societies do not have built-in rivalries or PvP mechanics. They are
civic institutions that cooperate (or at least coexist) within the five-city
network. Inter-society interaction is diplomatic, not adversarial — a Guard
Command officer and an Ember Warden work together during a crisis, they do not
fight each other.

---

## The Six Public Societies

| Society | Base City | Lore Origin | Gameplay Loop |
|---|---|---|---|
| **The Guard Command** | Midgaard | Midgaard's primary military institution | Combat patrols, city defense, beast hunting |
| **The Harbor Syndics** | Kowloon | Kowloon's maritime commerce authority | Trade contracts, shipping, cargo management |
| **The Ember Wardens** | Rakuen | Rakuen's disaster response corps | Emergency response, botanical gathering, infrastructure repair |
| **The Shrine of the First Claw** | Mafdet | Mafdet's justice and oath-enforcement order | Bounty hunting, dispute arbitration, oath contracts |
| **The Wall Command** | Kiess | Kiess's frontier defense force | Frontier patrols, escort duty, wilderness security |
| **The Road Wardens** | Intercity | Chartered trade-route protection companies | Caravan escort, route maintenance, wilderness exploration |

Each society is detailed in its own file.

---

## Shared Mechanics

### Joining

Public society recruitment is open. A player visits the society's hall, speaks
to the enrollment officer NPC, and joins if they meet the prerequisites. No
passphrase, no initiation mission, no hidden discovery — the hall is a known
location, the officer is clearly identified, and the requirements are stated
plainly.

```
enlist <society>
```

The `enlist` command is available in any room containing a public society
enrollment officer NPC. The officer checks prerequisites and either enrolls the
player or explains what is missing.

### Prerequisites

All public societies share a base requirement:

- Must be a player (not NPC)
- Must not already be in a public society
- Must meet the society-specific level requirement
- Must not have left a public society within the last 7 real days (shorter
  cooldown than secret societies, reflecting the lower commitment)

### Rank (Core Progression)

Rank is the **sole progression axis** for public societies. A player's rank
is a numeric score that goes up when they complete tasks and goes down when
they fail or abandon them. Rank directly determines which skills the player
has access to, which tasks they can accept, and what institutional privileges
they hold.

There is no separate promotion step. As rank crosses a threshold, the
corresponding skill unlocks immediately. If rank drops below a threshold
(through task failure or other penalties), the skill becomes unavailable
again until rank recovers.

Public society rank uses a -500 to +500 range:

| Range | Title | Skills Unlocked | Effect |
|---|---|---|---|
| -500 to -300 | Dishonorably Discharged | None | Expelled; barred from rejoining for 30 days |
| -299 to -100 | Disciplined | None (suspended) | All skills suspended; restricted task access |
| -99 to 0 | Recruit | Skill 1 | Basic access only |
| 1 to 99 | Member | Skills 1-2 | Standard access |
| 100 to 199 | Veteran | Skills 1-3 | Priority task access |
| 200 to 299 | Officer | Skills 1-4 | Can mentor recruits |
| 300 to 399 | Senior Officer | Skills 1-5 | Can enroll new members; institutional privileges |
| 400 to 500 | Commander | Skills 1-6 (all) | Maximum standing; leadership tier |

Each society defines 6 skills. This makes rank a living, dynamic gate — a
Senior Officer who fails several tasks and drops to Veteran loses access to
their 5th skill until they rebuild.

Rank changes:
- Completing tasks: +10 to +50 depending on difficulty
- Failing tasks: -5 to -20 depending on severity
- Abandoning tasks: -3 to -10
- Mentoring a recruit (Officer+): +15
- Time-based decay: rank above 300 decays by 1 per game day toward 300

Each society overrides the general title labels (Recruit, Member, Veteran,
etc.) with its own thematic titles — see individual society files.

### Leaving and Loss of Benefits

A player can leave their public society at any time:

```
society resign
```

**Leaving a public society means losing everything it granted.** On departure:

- All public society skills become immediately unusable. Proficiency values
  are retained in `learned[]` (so rejoining the same society later does not
  require re-practicing from scratch), but all skill checks auto-fail while
  the player is not a member.
- Access to the society hall's restricted areas (armory, training yard,
  command post) is revoked.
- The society communication channel is removed.
- Society-affiliated NPC merchants and trainers will no longer serve the
  player.
- Any active society task is automatically abandoned.
- The player's reputation is preserved internally (for potential re-enrollment)
  but provides no benefits while unaffiliated.
- A 7-day cooldown is imposed before joining any public society.

Voluntary departure has no reputation penalty — it is an honorable discharge.
Expulsion (reputation below -300) imposes a 30-day cooldown and resets
reputation to 0.

### Visibility

Public society membership is displayed in:

- `who` output: society abbreviation and rank title after the player's name
- `score`: society name, rank, and reputation
- `finger`/`whois`: society membership line

This is the opposite of secret societies, where membership is hidden from all
public commands.

---

## Relationship to Other Systems

| System | Interaction |
|---|---|
| **Clans** | Independent membership; society tasks never target clan resources |
| **Secret Societies** | Independent membership; a player can be in both |
| **AI Dialogue** | Society NPCs recognize members and address them by rank |
| **Quest System** | Society tasks use `QUEST_DATA` with `QUEST_TYPE_PUBLIC_SOCIETY` |
| **Skills** | Public society skills stored in `learned[]` at reserved indices |
| **Save System** | Public society data persisted in player file |

---

## Scope

This proposal covers:

- Definitions of six public societies with lore grounding
- Shared membership, rank, and reputation mechanics
- Public society skills (non-combat utility)
- Task-board mission system
- Data structure additions
- Integration with existing systems

This proposal does **not** cover:

- Area file definitions for society halls (separate area-building task)
- AI dialogue persona text for society NPCs (separate AI content task)
- Specific task content beyond examples (separate content-authoring task)
