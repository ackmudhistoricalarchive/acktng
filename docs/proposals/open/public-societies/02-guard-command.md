# Public Society: The Guard Command

**Constant:** `PUB_SOCIETY_GUARD_COMMAND`
**Base City:** Midgaard
**Abbreviation:** GUARD
**Level Requirement:** 15

---

## Lore Origin

The Guard Command is Midgaard's primary military institution, documented
extensively in the city lore. It employs Knight-tradition officers, maintains
the city walls, patrols the districts, and enforces civic law within Midgaard's
jurisdiction. The Guard Command shares provisioning doctrine with Kowloon's
Wardens of Iron and coordinates with Kiess's Wall Command on frontier
operations, but its authority is rooted in Midgaard's founding oaths — the Oath
of Stone, which binds the city to defend its people.

The Guard Command is not a political faction. It serves the Magistrate
Ledgerhouse's warrants, enforces the Lantern Registry's civic codes, and
provides escort for diplomatic missions between cities. Its officers are drawn
from the Knight academies formalized under the Compact of the Seven Chains, but
the rank and file include soldiers from every martial tradition — swordsmen,
crusaders, and conscripted civilians trained in basic arms.

The Guard Command is the most visible institution in Midgaard. Its patrols walk
every district, its banners hang from every gate, and its recruiting station is
the first building new arrivals see inside the Southern Wild Gate. Joining the
Guard Command is the most straightforward civic commitment a player can make.

---

## Philosophy

*"One wall, one watch, one answer."*

The Guard Command exists to protect Midgaard and its people. Its doctrine is
simple: maintain the walls, patrol the streets, respond to threats. The Command
does not concern itself with political factions (Continuity vs. Reckoning),
sealed archives, or institutional guilt. It concerns itself with whether the
gates are manned, whether the patrols are armed, and whether citizens are safe.

This deliberate apoliticism is both the Guard Command's strength and its
vulnerability. It makes the Command reliable and trusted — the one institution
everyone agrees should exist. It also makes the Command a tool for whoever
controls the Magistrate Ledgerhouse, since the Command enforces warrants
without questioning their origin.

---

## Gameplay Loop

**Combat patrols, city defense, and beast hunting.** The Guard Command is the
most combat-oriented public society. Its tasks send players into hostile areas
to clear threats, patrol trade routes, defend positions, and hunt dangerous
creatures reported by civilians.

Guard Command tasks involve:

- Patrolling designated routes (visit a series of rooms in sequence)
- Clearing reported threats (kill specific mobs in specific areas)
- Defending positions during timed events (remain in a room, survive waves)
- Escorting NPC civilians or diplomats between locations
- Investigating disturbances (visit a location, report what you find)
- Hunting specific beasts reported as a danger to travelers

The Guard Command is the best public society for players who enjoy combat and
want straightforward "go here, fight that, come back" gameplay.

---

## Hall Location

The Guard Command Hall is a large, well-marked building near Midgaard's central
square. It contains:

- An enrollment office (recruitment NPC)
- A duty board (task assignment NPC)
- An armory (society merchant NPC selling Guard-issue equipment)
- A training yard (practice NPC for society skills)
- A briefing room (commanding officer NPC for promotions and intelligence)
- Barracks (rest area with accelerated healing for members)

The Hall is a public building. Anyone can enter. Only members can use the
armory, training yard, and briefing room.

---

## Rank Titles

| Rank Score | Title |
|---|---|
| -99 to 0 | Watchman |
| 1 to 99 | Guard |
| 100 to 199 | Sergeant |
| 200 to 299 | Lieutenant |
| 300 to 399 | Captain |
| 400 to 500 | Marshal |

---

## Skills

Guard Command skills focus on martial readiness, tactical awareness, and field
survival. Each skill unlocks when the player's rank score reaches the listed
threshold. If rank drops below the threshold, the skill becomes unavailable
until rank recovers.

| Skill | Rank Required | Description |
|---|---|---|
| `patrol_sense` | -99 (Watchman) | Detect aggressive mobs within 2 rooms in any direction. Shows direction and approximate strength (weak/moderate/dangerous). |
| `field_mend` | 1 (Guard) | Perform basic field medicine on self or an ally. Restores a small amount of HP outside of combat. 5-minute cooldown. Does not scale with level — pure utility. |
| `rally_cry` | 100 (Sergeant) | Shout a rallying cry that briefly suppresses fear effects on all group members in the room. 10-minute cooldown. Does not grant combat bonuses — only removes existing fear/panic debuffs. |
| `shield_wall` | 200 (Lieutenant) | **Combat (passive).** While a shield is equipped, successful block checks also reduce the damage of the next attack that lands by 20%. Checked in `calculate_damage()` after a block in `check_avoidance()` sets a per-round flag. Always active — no cooldown, no activation command. |
| `tactical_assess` | 300 (Captain) | Examine a mob to reveal its special attacks, resistances, and approximate HP percentage. More detailed than standard `consider`. |
| `command_presence` | 400 (Marshal) | **Combat (active, combo builder).** Deliver a commanding strike and rally your group. Implemented via `war_attack(ch, argument, gsn_command_presence)` — the same pattern as bash, kick, and other combo-feeding skills. On hit, calls `combo(ch, victim, gsn_command_presence)` to add `gsn_command_presence` to the `ch->combo[]` array, contributing toward a finisher sequence. Deals physical damage (`do_damage()` with `gsn_command_presence`, `ELE_PHYSICAL`) and applies a 3-round `APPLY_HITROLL` buff (+5) to all group members in the room via `is_same_group()` iteration. Dual-purpose: personal combo contribution plus group support. Proficiency-gated. `WAIT_STATE` of 24 beats (6 seconds) plus a 20-round `cooldown[gsn_command_presence]` recharge timer. The Marshal leads from the front. |

Note: `fortify` (previously in the Lieutenant slot) is moved to a task
mechanic rather than a persistent skill. `shield_wall` replaces it as the
rank 3 passive combat skill.

---

## Task Examples

| Difficulty | Type | Example |
|---|---|---|
| 1 | Patrol | Walk the Lantern District patrol route (visit 5 rooms in order) |
| 2 | Hunt | Kill the wolf pack reported near the Southern Wild Gate |
| 3 | Defense | Hold the East Bridge checkpoint for 10 minutes against hostile mobs |
| 4 | Escort | Escort the diplomatic courier from Midgaard to the Roc Road waystation |
| 5 | Expedition | Clear the reported creature nest in the Great Northern Forest approaches |

---

## Affiliated NPCs

| Role | Description |
|---|---|
| **Enrollment Officer** | Processes new recruits; explains requirements and benefits |
| **Duty Sergeant** | Assigns tasks from the duty board; reports completion |
| **Quartermaster** | Sells Guard-issue equipment (functional, not best-in-slot) |
| **Drill Instructor** | Teaches society skills; available for practice sessions |
| **Commanding Officer** | Handles promotions; provides intelligence briefings |
| **Gate Guards** | Recognize members by rank; provide passage and information |
