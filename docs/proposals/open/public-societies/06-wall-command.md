# Public Society: The Wall Command

**Constant:** `PUB_SOCIETY_WALL_COMMAND`
**Base City:** Kiess
**Abbreviation:** WALLC
**Level Requirement:** 15

---

## Lore Origin

The Wall Command is one of Kiess's three governing powers, documented in the
city lore alongside the Compact Temples and the Trade Syndics. Kiess was built
on the ruins of Evermeet at the western edge of the continent, facing the Roc
Road corridor that connects it to the eastern cities and the Cinderteeth
Mountains that loom to the north. The Wall Command exists because Kiess is a
frontier city — the western bulwark, the last settlement before the wilderness,
and the first line of defense against whatever comes down from the mountains.

The Wall Command maintains Kiess's fortifications, patrols the Roc Road
corridor's western reaches, monitors the Cinderteeth approaches for volcanic
and creature activity, and secures the frontier gate that controls access to
the wilderness beyond. Its soldiers are not the parade-ground officers of
Midgaard's Guard Command — they are frontier troops, accustomed to long patrols
in hostile terrain, improvised defenses, and operating far from resupply.

The Wall Command recruits anyone willing to serve on the frontier. Its founding
charter includes Kiess's anti-sealed-warrant doctrine, meaning the Wall
Command operates with unusual transparency — no secret orders, no sealed
warrants, no classified operations. Everything the Wall Command does is logged
and auditable. This makes it the most principled military institution on the
continent and the most frustrating for anyone who wants it to bend rules.

---

## Philosophy

*"The wall does not negotiate."*

The Wall Command's doctrine is physical security through visible strength.
Walls are built, manned, and maintained. Patrols run on schedule. Threats are
identified, reported, and neutralized. The Wall Command does not engage in
intelligence operations, covert actions, or political maneuvering — it builds
walls and defends them.

This rigidity is informed by Kiess's founding charter, which was written as a
direct repudiation of the Spirebound Conclave's institutional secrecy and the
Violet Compact's sealed-warrant system. The Wall Command's transparency
doctrine means that every patrol route, every garrison strength, and every
operational order is a matter of public record. The Command's enemies know
exactly what it will do, but its citizens know they can trust it.

---

## Gameplay Loop

**Frontier patrols, fortification, and wilderness security.** The Wall Command
offers an exploration-and-defense gameplay loop. Tasks send players into
frontier territory to patrol, build, defend, and report.

Wall Command tasks involve:

- Patrolling the Roc Road corridor and frontier approaches
- Scouting Cinderteeth approaches for creature activity or volcanic signs
- Reinforcing frontier positions (use skills at specific locations)
- Escorting trade caravans through the Roc Road corridor
- Clearing beast nests and hostile camps in frontier territory
- Reporting on terrain conditions and route safety

The Wall Command is the best public society for players who enjoy exploration,
frontier adventure, and the fantasy of being a frontier soldier.

---

## Hall Location

The Wall Command Garrison is a stone fortress near Kiess's frontier gate. It
contains:

- A recruitment post (enrollment NPC)
- A patrol board (task assignment NPC)
- A frontier armory (society merchant NPC with survival gear and fortification tools)
- A training yard (practice NPC for society skills)
- A commander's quarters (commanding officer NPC for promotions and briefings)
- A scout's lodge (rest area with outdoor survival bonuses for members)

The Garrison is public. Anyone can enter the outer yard. The armory, training
yard, and commander's quarters are restricted to members.

---

## Rank Titles

| Rank Score | Title |
|---|---|
| -99 to 0 | Sentry |
| 1 to 99 | Ranger |
| 100 to 199 | Pathfinder |
| 200 to 299 | Wall Sergeant |
| 300 to 399 | Wall Captain |
| 400 to 500 | Castellan |

---

## Skills

Wall Command skills focus on wilderness survival, fortification, and frontier
awareness. Each skill unlocks when the player's rank score reaches the listed
threshold.

| Skill | Rank Required | Description |
|---|---|---|
| `dig_in` | -99 (Sentry) | Fortify your position in the current room, granting a 10-minute buff that reduces movement cost to zero when moving back to this room (you can always retreat to your fortified position cheaply) and provides a +5 AC bonus while in the room. One fortified position active at a time; fortifying a new room replaces the previous one. The sentry digs in and holds ground — the most fundamental Wall Command discipline. |
| `shelter` | 1 (Ranger) | Construct a temporary shelter in the current outdoor room. The shelter provides weather protection and slightly accelerated HP/mana regen for the builder's group. Lasts 1 hour. |
| `trailblaze` | 100 (Pathfinder) | Mark a path through the current room. Wall Command members who enter the room see the mark and the direction it points, aiding navigation in confusing wilderness areas. Lasts 12 hours. |
| `bulwark` | 200 (Wall Sergeant) | **Combat (passive).** Frontier hardening reduces all incoming physical damage by 5%. Checked in `calculate_damage()` as a flat reduction after all other modifiers. Always active — no cooldown, no activation command. |
| `signal_tower` | 300 (Wall Captain) | Erect a temporary signal point in the current outdoor room. All Wall Command members in the same area receive a notification when an aggressive mob enters a room adjacent to the signal. Lasts 30 minutes. |
| `rampart_slam` | 400 (Castellan) | **Combat (active, combo builder).** Drive your shield into the enemy with fortress-wall force. Requires a shield equipped. Implemented via `war_attack(ch, argument, gsn_rampart_slam)` — the same pattern as bash, kick, and other combo-feeding skills. On hit, calls `combo(ch, victim, gsn_rampart_slam)` to add `gsn_rampart_slam` to the `ch->combo[]` array, contributing toward a finisher sequence. Deals physical damage (`do_damage()` with `gsn_rampart_slam`, `ELE_PHYSICAL`) and sets the victim's position to `POS_STUNNED` for 1 round (same stun mechanic as `bash` but with higher base damage). Proficiency-gated. `WAIT_STATE` of 24 beats (6 seconds) plus a 20-round `cooldown[gsn_rampart_slam]` recharge timer. The wall does not negotiate. |

Note: `barricade` (previously in the Wall Sergeant slot) is moved to a task
mechanic. `bulwark` replaces it as the rank 3 passive combat skill.

---

## Task Examples

| Difficulty | Type | Example |
|---|---|---|
| 1 | Patrol | Walk the Roc Road western checkpoint route (visit 4 rooms) |
| 2 | Scouting | Scout the northern Cinderteeth approach and report creature activity |
| 3 | Fortification | Reinforce the frontier outpost (use barricade at 3 designated rooms) |
| 4 | Escort | Escort a supply caravan from Kiess through the Roc Road to the halfway point |
| 5 | Expedition | Clear and secure a new frontier outpost location deep in the western wilderness |

---

## Affiliated NPCs

| Role | Description |
|---|---|
| **Recruitment Sergeant** | Processes new members; explains frontier service requirements |
| **Patrol Officer** | Assigns tasks from the patrol board; reports completion |
| **Frontier Quartermaster** | Sells survival gear, fortification tools, and rations |
| **Drill Ranger** | Teaches society skills; available for practice sessions |
| **Castellan** | Handles promotions; provides frontier intelligence briefings |
| **Wall Sentries** | Recognize members by rank; provide passage through the frontier gate |
