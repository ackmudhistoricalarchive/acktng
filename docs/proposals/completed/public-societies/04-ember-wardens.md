# Public Society: The Ember Wardens

**Constant:** `PUB_SOCIETY_EMBER_WARDENS`
**Base City:** Rakuen
**Abbreviation:** EMBER
**Level Requirement:** 15

---

## Lore Origin

The Ember Wardens are documented in Rakuen's lore as the city's disaster
response and civil defense corps. Rakuen exists in a precarious ecological
position — built at the southern edge of the Eccentric Woodland, dependent on
botanical cultivation for survival, and exposed to threats that the other four
cities do not face: woodland incursions, Ash Cult sabotage, supply line
disruption, and the chronic emergency conditions that result from Midgaard's
"resettled south" mechanism.

The Ember Wardens were established after Rakuen's founding under the Southward
Compact as a practical necessity. Someone had to maintain the walls, organize
disaster response, coordinate supply distribution during shortages, and defend
the Canopy Workshops that produce the botanical materials Rakuen trades for
survival. The Wardens are that someone.

Unlike Midgaard's Guard Command, the Ember Wardens are not primarily a military
force. They are emergency responders first — firefighters, medics, engineers,
and supply coordinators who also happen to carry weapons because the things
that threaten Rakuen often have teeth.

---

## Philosophy

*"When the ember falls, we answer."*

The Ember Wardens take their name from the ash-ember warning system used to
signal emergencies in Rakuen's districts. When an ember lamp is lit atop a
watchtower, the Wardens respond. The philosophy is service without politics —
the Wardens do not care whether you are Amendment Track, Quiet Separatist, or
Preservation Coalition. When the wall cracks, when the supply line breaks, when
the blight spreads, the Wardens answer.

This institutional apoliticism makes the Ember Wardens Rakuen's most trusted
institution and the one most targeted by the Ash Cult, which views civic
resilience as an obstacle to the "honest destruction" they believe the cities
deserve.

---

## Gameplay Loop

**Emergency response, botanical gathering, and infrastructure repair.** The
Ember Wardens offer a support-focused gameplay loop. Tasks involve responding
to crises, gathering rare materials, repairing damaged structures, and
maintaining Rakuen's vulnerable supply infrastructure.

Ember Warden tasks involve:

- Responding to emergency events (reach a location within a time limit)
- Gathering botanical materials from the Eccentric Woodland and surroundings
- Repairing damaged objects and infrastructure (skill-based interactions)
- Delivering emergency supplies to isolated NPC settlements
- Extinguishing fires and containing blight outbreaks (room-based objectives)
- Defending supply caravans from hostile creatures

The Ember Wardens are the best public society for players who enjoy support
roles, resource gathering, and time-pressure missions.

---

## Hall Location

The Ember Warden Station is a fortified compound near Rakuen's main gate. It
contains:

- A muster desk (enrollment NPC)
- A dispatch board (task assignment NPC)
- A supply depot (society merchant NPC with botanical tools and emergency kit)
- A training ground (practice NPC for society skills)
- A command post (commanding officer NPC for promotions and briefings)
- An infirmary (rest area with accelerated healing for members and their group)

The Station is public. Anyone can enter. Only members can use the supply depot,
training ground, and command post.

---

## Rank Titles

| Rank Score | Title |
|---|---|
| -99 to 0 | Responder |
| 1 to 99 | Warden |
| 100 to 199 | Senior Warden |
| 200 to 299 | Warden-Lieutenant |
| 300 to 399 | Warden-Captain |
| 400 to 500 | Warden-Commander |

---

## Skills

Ember Warden skills focus on survival, resource management, and environmental
interaction. Each skill unlocks when the player's rank score reaches the
listed threshold.

| Skill | Rank Required | Description |
|---|---|---|
| `forage` | -99 (Responder) | Gather a random botanical material or food item from the current outdoor room. One item per use. 3-minute cooldown. Items are consumed for tasks. |
| `field_repair` | 1 (Warden) | Repair a damaged piece of equipment, restoring a portion of its condition. Cannot improve beyond original state. Works on own or ally's gear. 10-minute cooldown. |
| `ember_signal` | 100 (Senior Warden) | Light an emergency signal in the current room. All Ember Wardens in the same area receive a notification with the room name. One signal active at a time. |
| `ember_vitality` | 200 (Warden-Lieutenant) | **Combat (passive).** Emergency conditioning grants passive HP regeneration during combat. Each combat round, the player recovers HP equal to 1% of their `max_hit`. Checked in `violence_update()` before `multi_hit()`. Always active — no cooldown, no activation command. |
| `containment_ward` | 300 (Warden-Captain) | Place a temporary barrier on a room exit that prevents mobs below a level threshold from passing through. Lasts 10 minutes. One ward active at a time. |
| `controlled_burn` | 400 (Warden-Commander) | **Combat (active, chi-enhanced).** Unleash a searing strike. Deals fire elemental damage (`do_damage()` with `gsn_controlled_burn`, `ELE_FIRE`) and applies a 3-round fire `APPLY_DOT` via `affect_to_char()` (damage equal to 10% of the initial hit per tick, processed in `round_update_dot()`). **Chi enhancement (optional):** if the player has chi access (`get_chi(ch) >= 5`), they may spend 5 chi to double the DOT duration from 3 rounds to 6. This rewards martial artist / Ember Warden cross-builds without penalizing non-chi users — the base skill works identically without chi. Proficiency-gated. `WAIT_STATE` of 24 beats (6 seconds) plus a 20-round `cooldown[gsn_controlled_burn]` recharge timer. When the ember falls, the Wardens answer — with fire. |

Note: `stabilize` (previously in the Warden-Lieutenant slot) is moved to rank
5 as a passive death-prevention trigger on the `emergency_cache` skill.
`ember_vitality` replaces it as the rank 3 passive combat skill.

---

## Task Examples

| Difficulty | Type | Example |
|---|---|---|
| 1 | Gathering | Collect 3 thornwood bark samples from the Eccentric Woodland edge |
| 2 | Delivery | Rush an emergency supply kit to the Bloom Pavilion outpost within 5 minutes |
| 3 | Repair | Repair the southern wall breach (use field_repair at a specific room) |
| 4 | Containment | Extinguish ashmark fires in 4 Rakuen district rooms before they spread |
| 5 | Expedition | Lead a supply caravan through the Eccentric Woodland to a remote settlement |

---

## Affiliated NPCs

| Role | Description |
|---|---|
| **Muster Clerk** | Processes new members; explains requirements and benefits |
| **Dispatch Officer** | Assigns tasks from the dispatch board; reports completion |
| **Quartermaster** | Sells botanical tools, repair kits, and emergency supplies |
| **Training Warden** | Teaches society skills; available for practice sessions |
| **Warden-Commander** | Handles promotions; provides emergency briefings |
| **District Watchers** | Recognize members by rank; report emergency conditions |
