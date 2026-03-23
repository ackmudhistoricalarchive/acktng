# Public Society: The Shrine of the First Claw

**Constant:** `PUB_SOCIETY_FIRST_CLAW`
**Base City:** Mafdet
**Abbreviation:** CLAW
**Level Requirement:** 20

---

## Lore Origin

The Shrine of the First Claw is documented in Mafdet's lore as the city's
oath-enforcement and swift-court justice institution. Named for a guardian
feline executioner who punished oath-breakers in the pre-city era, the Shrine
administers Mafdet's distinctive legal tradition: oaths are binding contracts,
and those who break them face judgment — not through slow tribunal proceedings,
but through swift, public adjudication at the Shrine's court.

The Shrine's authority extends beyond Mafdet's walls. Its oath-contracts are
recognized across the five-city network, its Claw Agents pursue oath-breakers
along the trade routes, and its arbitration decisions carry weight in the
Ledger Houses' commercial law. The Shrine of the First Claw is the closest
thing the continent has to a cross-city law enforcement institution — though
its jurisdiction is strictly limited to oath violations, not general crime.

The Shrine recruits openly, seeking individuals who can serve as investigators,
arbiters, and enforcers. The level requirement is higher than other public
societies because the Shrine demands both combat capability and judgment.

---

## Philosophy

*"The claw falls where the oath breaks."*

The Shrine believes that civilization rests on the sanctity of sworn
agreements. Commerce, governance, alliances, and personal bonds all depend on
the assumption that oaths will be honored. When that assumption fails — when a
merchant defaults on a bonded contract, when a debtor flees a sworn obligation,
when an official violates their oath of office — the Shrine acts.

The Shrine's agents do not concern themselves with morality in the abstract.
They concern themselves with whether an oath was made, whether it was broken,
and what the sworn penalty is. This makes the Shrine a tool of precise justice
and, in the wrong hands, a weapon of legalistic persecution. The institution's
internal debates center on where enforcement ends and abuse begins.

---

## Gameplay Loop

**Bounty hunting, dispute arbitration, and oath enforcement.** The Shrine of
the First Claw offers a bounty-hunter and investigator gameplay loop. Tasks
involve tracking down oath-breakers, resolving disputes, and enforcing
contracts.

Shrine tasks involve:

- Tracking and confronting NPC oath-breakers (hunt and confront/kill targets)
- Investigating reported oath violations (visit locations, gather evidence)
- Arbitrating disputes between NPC parties (dialogue-based resolution)
- Escorting individuals under oath-protection to safe locations
- Serving writs of enforcement on NPCs in distant cities
- Recovering assets seized as oath penalties

The Shrine of the First Claw is the best public society for players who enjoy
bounty-hunting, investigative gameplay, and the satisfaction of tracking down
a target.

---

## Hall Location

The Shrine of the First Claw is a stone-and-bronze temple complex near
Mafdet's central market. It contains:

- A petitioner's hall (enrollment NPC; also where NPCs file oath complaints)
- A writ board (task assignment NPC)
- An armory of judgment (society merchant NPC with tracking tools and restraints)
- A training court (practice NPC for society skills)
- A high arbiter's chamber (commanding officer NPC for promotions)
- An oath vault (secure room containing active oath contracts; rank 3+ access)

The Shrine is public. Anyone can enter the petitioner's hall. The inner
chambers are restricted to members.

---

## Rank Titles

| Rank Score | Title |
|---|---|
| -99 to 0 | Petitioner |
| 1 to 99 | Sworn |
| 100 to 199 | Claw Agent |
| 200 to 299 | Arbiter |
| 300 to 399 | Senior Arbiter |
| 400 to 500 | High Claw |

---

## Skills

Shrine skills focus on tracking, investigation, and enforcement authority. Each
skill unlocks when the player's rank score reaches the listed threshold.

| Skill | Rank Required | Description |
|---|---|---|
| `oath_mark` | -99 (Petitioner) | Mark an NPC or player as an oath-subject. The mark is visible only to Shrine members and persists for 1 hour. Used to identify task targets. |
| `track_quarry` | 1 (Sworn) | When used in a room the target has recently visited, reveals the direction they departed. Works on NPCs and players. 5-minute cooldown. |
| `writ_of_hold` | 100 (Claw Agent) | Present a writ to an NPC, preventing them from moving for 5 minutes. The NPC must be a valid task target. Does not work on players or bosses. |
| `marked_strike` | 200 (Arbiter) | **Combat (passive).** When fighting a target the player has `oath_mark`ed, all autoattacks gain +10% evasion piercing (added to `get_evasion_piercing()`) and a 10% chance to bypass parry/dodge/block entirely. Checked automatically in `check_avoidance()` against the `oath_mark` affect on the victim. Always active against marked targets — no cooldown, no activation command. |
| `claws_pursuit` | 300 (Senior Arbiter) | Initiate a formal pursuit against an `oath_mark`ed target that has left the room. Grants a 3-round buff that continuously reveals the target's location (room name and direction, like an automatic `track_quarry`) and halves the player's movement cost while pursuing. 10-minute cooldown. The Senior Arbiter has the authority to invoke the Shrine's pursuit rite — the oath-breaker cannot simply run. Bridges `writ_of_hold` (freeze them in place) and `claws_judgment` (finish them) with a pursuit tool for when the target flees. |
| `claws_judgment` | 400 (High Claw) | **Combat (active, combo finisher).** Execute the Shrine's sentence. Only usable against a target the player has `oath_mark`ed (refuses to fire otherwise) **and** requires `is_valid_finisher()` — the combo array must be at `max_combo - 1` or higher, just like `fleche` and `holystrike`. This makes it an alternative combo finisher available only to High Claw members: build a combo with standard war-attack skills, then close it out with the Shrine's judgment instead of a class finisher. Deals massive physical damage (`do_damage()` with `gsn_claws_judgment`, `ELE_PHYSICAL`) with a 2x damage multiplier against the marked target. On hit, triggers the finisher chain sequence via `combo()` — the accumulated combo skills fire as automatic chain attacks (3-6 hits with the standard damage multiplier). Proficiency-gated. `WAIT_STATE` of 24 beats (6 seconds) plus a 20-round `cooldown[gsn_claws_judgment]` recharge timer. The claw falls where the oath breaks. |

Note: `interrogate` (previously in the Arbiter slot) is moved to rank 3 as
part of the `writ_of_hold` skill's extended functionality. `marked_strike`
replaces it as the rank 3 passive combat skill.

---

## Task Examples

| Difficulty | Type | Example |
|---|---|---|
| 1 | Investigation | Visit the dockside warehouse and report whether the merchant's goods match the oath-manifest |
| 2 | Tracking | Track the fleeing debtor NPC from Mafdet to the Saltglass Reach |
| 3 | Enforcement | Serve a writ of seizure on the oath-breaking trader in the Northern Oasis |
| 4 | Bounty | Hunt and confront the rogue caravan master last seen in the Scorching Sands |
| 5 | Arbitration | Resolve the three-party trade dispute in Mafdet's Ledger Houses (multi-step investigation and judgment) |

---

## Affiliated NPCs

| Role | Description |
|---|---|
| **Petitions Clerk** | Processes new members; explains the Shrine's purpose and requirements |
| **Writ Officer** | Assigns tasks from the writ board; reports completion |
| **Armorer of Judgment** | Sells tracking tools, restraints, and enforcement equipment |
| **Enforcement Trainer** | Teaches society skills; available for practice sessions |
| **High Arbiter** | Handles promotions; provides intelligence on active oath violations |
| **Claw Sentinels** | Recognize members by rank; guard the Shrine's inner chambers |
