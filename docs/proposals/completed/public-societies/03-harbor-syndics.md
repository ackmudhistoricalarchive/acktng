# Public Society: The Harbor Syndics

**Constant:** `PUB_SOCIETY_HARBOR_SYNDICS`
**Base City:** Kowloon
**Abbreviation:** SYNDX
**Level Requirement:** 15

---

## Lore Origin

The Harbor Syndics are one of Kowloon's five governing institutions, documented
in the city lore as the authority over maritime commerce, port administration,
and trade regulation. Founded under the Neon Covenant's compact — "one
ledger, one ration code" — the Syndics manage everything that moves through
Kowloon's delta port: cargo manifests, berthing assignments, tariff collection,
trade dispute arbitration, and the licensing of merchant vessels.

The Syndics are the most institutionally stable of Kowloon's five pillars. The
Jade Magistracy handles law, the Temple Circle handles religion, the Wardens of
Iron handle defense, and the Courier Lantern Office handles communications —
but the Syndics handle money, and money is what keeps Kowloon's massive port
infrastructure functioning.

The Syndics recruit openly. Their offices line the harbor district, their
agents staff every customs checkpoint, and their trade schools train
apprentices in accounting, appraisal, and contract law. For players interested
in the economic side of the game, the Harbor Syndics are the natural home.

---

## Philosophy

*"One ledger, one standard, one port."*

The Syndics believe that fair, regulated trade is the foundation of
civilization. Unregulated commerce breeds fraud, exploitation, and the kind of
shadow economies that organizations like the Jade Eels thrive in. The Syndics'
mission is to ensure that every transaction passing through Kowloon's port is
documented, taxed, and arbitrable — not because they love bureaucracy, but
because the alternative is piracy with extra steps.

This puts the Syndics in direct institutional tension with the Jade Eels, who
view the Syndics as the legitimate face of a system that exists to enrich
itself. The Syndics are aware of smuggling operations in their port and
periodically launch investigations — but the Jade Eels' infiltration of the
Syndics' own ranks makes these investigations frustratingly inconclusive.

---

## Gameplay Loop

**Trade contracts, shipping management, and commercial arbitration.** The
Harbor Syndics offer an economy-focused gameplay loop. Tasks involve buying,
appraising, transporting, and protecting legitimate cargo.

Harbor Syndic tasks involve:

- Appraising cargo shipments for customs clearance (identify item values)
- Transporting trade goods between Kowloon's harbor and inland destinations
- Negotiating trade contracts with NPC merchants in other cities
- Investigating suspected fraudulent cargo manifests
- Protecting trade shipments from bandit and pirate NPCs
- Auditing NPC shops for price irregularities

The Harbor Syndics are the best public society for players who enjoy economic
gameplay, item trading, and inter-city commerce.

---

## Hall Location

The Harbor Syndics Hall is a large administrative complex on Kowloon's central
wharf. It contains:

- A registration desk (enrollment NPC)
- A contracts board (task assignment NPC)
- A bonded warehouse (society merchant NPC with trade goods and appraisal tools)
- A training office (practice NPC for society skills)
- A senior syndic's chamber (commanding officer NPC for promotions)
- A records room (accessible to rank 3+; contains trade intelligence)

The Hall is public. Anyone can enter. Only members can use the bonded
warehouse, training office, and records room.

---

## Rank Titles

| Rank Score | Title |
|---|---|
| -99 to 0 | Clerk |
| 1 to 99 | Factor |
| 100 to 199 | Assessor |
| 200 to 299 | Arbiter |
| 300 to 399 | Senior Syndic |
| 400 to 500 | Port Master |

---

## Skills

Harbor Syndic skills focus on commerce, appraisal, and trade logistics. Each
skill unlocks when the player's rank score reaches the listed threshold.

| Skill | Rank Required | Description |
|---|---|---|
| `appraise` | -99 (Clerk) | Determine the exact gold value of any item, including hidden modifiers. More accurate than standard `value` at shops. |
| `dockside_audit` | 1 (Factor) | Inspect a mob or NPC to reveal their carried items, equipment condition, and approximate total gold value — a customs inspection. More detailed than standard `look` at equipment: shows item types, wear condition, and hidden modifiers. For player targets, shows only visibly worn gear (no snooping). Pairs with `appraise` (value items you hold) to build the Syndics' "we know what everything is worth" identity. |
| `haggle` | 100 (Assessor) | When buying from an NPC shop, temporarily improve the price by up to 10% based on proficiency. 15-minute cooldown per shop. |
| `longshoreman` | 200 (Arbiter) | **Combat (passive).** Years of hauling cargo have built raw striking power. Grants one additional autoattack per round, checked in `multi_hit()` in the same block that grants bonus hits for Pugilist, Warden, and other mortal/remort classes (around lines 328-357). Uses the same probability roll as those class-based extra attacks. Always active — no cooldown, no activation command. |
| `bonded_cargo` | 300 (Senior Syndic) | Seal an item in a bonded cargo container that protects it from theft, drop-on-death, and decay for 1 hour. One container active at a time. |
| `anchor_slam` | 400 (Port Master) | **Combat (active, combo builder).** Swing with dockyard brutality. Implemented via `war_attack(ch, argument, gsn_anchor_slam)` — the same pattern as bash, kick, and other combo-feeding skills. On hit, calls `combo(ch, victim, gsn_anchor_slam)` to add `gsn_anchor_slam` to the `ch->combo[]` array, contributing toward a finisher sequence. Deals heavy physical damage (`do_damage()` with `gsn_anchor_slam`, `ELE_PHYSICAL`) and applies a 2-round `APPLY_SPEED` debuff (-3) to the target via `affect_to_char()`, reducing their extra attack chance and avoidance. Proficiency-gated. `WAIT_STATE` of 24 beats (6 seconds) plus a 20-round `cooldown[gsn_anchor_slam]` recharge timer. Maritime brute force. |

Note: `trade_ledger` (previously in the Arbiter slot) is moved to a task
mechanic. `longshoreman` replaces it as the rank 3 combat skill.

---

## Task Examples

| Difficulty | Type | Example |
|---|---|---|
| 1 | Courier | Deliver a sealed trade manifest from the harbor to the Jade Magistracy |
| 2 | Appraisal | Assess the contents of a cargo hold (identify 5 items and report values) |
| 3 | Investigation | Audit a harbor-district shop suspected of fencing stolen goods |
| 4 | Trade Route | Transport high-value cargo from Kowloon to Mafdet without loss |
| 5 | Arbitration | Resolve a trade dispute between two NPC merchant houses (multi-step negotiation) |

---

## Affiliated NPCs

| Role | Description |
|---|---|
| **Registrar** | Processes new members; explains requirements and benefits |
| **Contracts Clerk** | Assigns tasks from the contracts board; reports completion |
| **Bonded Warehouseman** | Sells trade tools and appraisal instruments |
| **Trade Instructor** | Teaches society skills; available for practice sessions |
| **Senior Syndic** | Handles promotions; provides trade intelligence |
| **Customs Inspectors** | Recognize members by rank; provide passage through checkpoints |
