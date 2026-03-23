# Public Society: The Road Wardens

**Constant:** `PUB_SOCIETY_ROAD_WARDENS`
**Base City:** Intercity (headquartered at Midgaard crossroads)
**Abbreviation:** ROADW
**Level Requirement:** 20

---

## Lore Origin

The Road Wardens are documented across multiple city lore files as the
chartered companies that maintain and protect the trade routes connecting the
five cities. Unlike the other public societies, the Road Wardens are not bound
to a single city — they operate across the entire continental road network,
from the Roc Road in the west to the Saltglass Reach in the east, from
Kowloon's northern approaches to the Eccentric Woodland corridor into Rakuen.

The Road Wardens predate the current five-city network. Their earliest charters
trace to the post-Ashfall period when overland trade resumed and someone had to
keep the roads passable — clearing debris, maintaining bridges, marking safe
routes, and fighting off the bandits, beasts, and worse things that thrived in
the depopulated corridors between settlements. The Wardens were originally
independent operators paid per-route; over time, they consolidated into
chartered companies recognized by all five cities.

The Road Wardens' authority comes from the Five-City Corridor Minutes — the
quarterly anti-fraud clerk exchange that is the most consistently maintained
shared institution in the five-city network. Road Warden charters are renewed
at each Corridor Minutes session, giving the Wardens a unique cross-city
legitimacy that no other institution enjoys.

Their headquarters is at the Midgaard crossroads because Midgaard sits at the
continental center where all major routes converge, but Road Warden posts exist
at waypoints along every major route.

---

## Philosophy

*"The road is the realm."*

The Road Wardens believe that the roads connecting the five cities are more
important than any single city. Cities can fall and be rebuilt; if the roads
fail, every city starves. The Wardens' loyalty is to the network, not to any
one node in it. This gives them a broader perspective than city-bound
institutions but also makes them politically homeless — trusted by all five
cities but answerable to none.

Road Wardens are practical people who judge situations by whether caravans can
pass safely and supplies can reach their destination. They have no patience for
political disputes, factional maneuvering, or institutional rivalries that
endanger the road network. A Road Warden will escort a Reckoning sympathizer's
cargo with the same professionalism as a Continuity faction shipment — the
cargo matters, the politics do not.

---

## Gameplay Loop

**Caravan escort, route maintenance, and wilderness exploration.** The Road
Wardens offer a travel-and-exploration gameplay loop. Tasks involve moving
between cities, protecting shipments, maintaining infrastructure, and
mapping the wilderness.

Road Warden tasks involve:

- Escorting NPC caravans between cities along designated routes
- Clearing route hazards (hostile mobs, blocked paths, destroyed bridges)
- Scouting new routes or alternate paths through wilderness areas
- Delivering urgent dispatches between Road Warden posts in different cities
- Investigating reports of bandit activity on trade corridors
- Maintaining waypoints (restocking supply caches, repairing trail markers)

The Road Wardens are the best public society for players who enjoy traveling
the world, exploring wilderness areas, and the freedom of not being tied to
a single city.

---

## Hall Location

The Road Warden Lodge is a traveler's compound at the Midgaard crossroads,
where the major routes to Kowloon, Kiess, Rakuen, and Mafdet diverge. It
contains:

- A charter desk (enrollment NPC)
- A route board (task assignment NPC)
- A supply post (society merchant NPC with travel gear, rations, and maps)
- A training paddock (practice NPC for society skills)
- A warden-master's office (commanding officer NPC for promotions and briefings)
- A wayfarers' rest (rest area with road-travel bonuses for members)

Road Warden posts also exist as smaller waypoint buildings along each major
route, staffed by NPC wardens who can assign tasks and accept reports.

The Lodge is public. Anyone can enter. Only members can use the supply post,
training paddock, and warden-master's office.

---

## Rank Titles

| Rank Score | Title |
|---|---|
| -99 to 0 | Runner |
| 1 to 99 | Outrider |
| 100 to 199 | Trailkeeper |
| 200 to 299 | Road Sergeant |
| 300 to 399 | Road Captain |
| 400 to 500 | Warden-Master |

---

## Skills

Road Warden skills focus on travel, navigation, and wilderness logistics. Each
skill unlocks when the player's rank score reaches the listed threshold.

| Skill | Rank Required | Description |
|---|---|---|
| `wayfind` | -99 (Runner) | In wilderness rooms, reveal the shortest route to the nearest city or Road Warden post. Displays a series of exit directions. |
| `road_camp` | 1 (Outrider) | Establish a temporary camp in an outdoor room. The camp provides a safe resting point where aggressive mobs will not initiate combat. Lasts 30 minutes. |
| `forced_march` | 100 (Trailkeeper) | For the next 5 minutes, the player's movement between rooms has no movement-point cost. 30-minute cooldown. |
| `road_grit` | 200 (Road Sergeant) | **Combat (passive).** Constant exposure to ambushes has sharpened reflexes. Dodge chance is increased by 5% (added to `get_dodge()` return value). Always active — no cooldown, no activation command. |
| `dispatch_rider` | 300 (Road Captain) | Send a message to any online Road Warden member regardless of distance. Functions as a one-way long-range tell. 10-minute cooldown. |
| `waylay` | 400 (Warden-Master) | **Combat (active, positional).** Strike from a cleared angle, exploiting the target's blind spot. **Positional gate:** only usable when the target is already fighting someone else (`victim->fighting != NULL && victim->fighting != ch`) — the same check as `circle` in the backstab system (`skills_poison.c`). The Road Warden strikes when the target's attention is elsewhere, punishing mobs engaged with allies. Deals physical damage (`do_damage()` with `gsn_waylay`, `ELE_PHYSICAL`) with bonus damage scaling and applies a 2-round debuff: `APPLY_DODGE` (-5) and `APPLY_SPEED` (-2) via `affect_to_char()`, making the target easier to hit and slower to react. Proficiency-gated. `WAIT_STATE` of 24 beats (6 seconds) plus a 20-round `cooldown[gsn_waylay]` recharge timer. The road teaches you where the ambushes hide. |

Note: `route_marker` (previously in the Road Sergeant slot) is moved to rank
4 as an extension of `dispatch_rider`. `road_grit` replaces it as the rank 3
passive combat skill.

---

## Task Examples

| Difficulty | Type | Example |
|---|---|---|
| 1 | Delivery | Carry a dispatch pouch from the Midgaard Lodge to the Roc Road waystation |
| 2 | Escort | Escort the grain caravan from the Southern Wild Gate to the Eccentric Woodland crossroads |
| 3 | Clearing | Clear the bandit camp reported on the Kowloon northern approach road |
| 4 | Scouting | Scout and mark an alternate route through the Saltglass Reach to Mafdet |
| 5 | Expedition | Establish a new waypoint in the deep wilderness between Kiess and the Cinderteeth approaches |

---

## Affiliated NPCs

| Role | Description |
|---|---|
| **Charter Clerk** | Processes new members; explains Road Warden traditions and requirements |
| **Route Officer** | Assigns tasks from the route board; reports completion |
| **Supply Warden** | Sells travel gear, rations, maps, and navigational tools |
| **Trail Instructor** | Teaches society skills; available for practice sessions |
| **Warden-Master** | Handles promotions; provides route intelligence and briefings |
| **Waypoint Wardens** | Staff at route posts across the continent; recognize members by rank |
