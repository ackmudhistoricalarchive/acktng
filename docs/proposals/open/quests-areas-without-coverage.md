# Proposal: Quests for Areas Without Coverage

## Problem

Four playable areas have no kill, fetch, or boss quests — only a cartography commission
(or nothing at all). Players leveling through or exploring these areas have no quest
motivation to engage them.

The following areas were identified by cross-referencing `quest_templates.target_vnums`
against `mobiles.vnum` and `mobiles.area_id`:

| Area | Levels | Rooms | Mobs | Existing quests |
|------|--------|-------|------|-----------------|
| Academy of Adventure (4900–4999) | 1–10 | 29 | 11 | none |
| Scorched Wastes (5250–5349) | 65–85 | 30 | 20 | none |
| Rakuen, City of the Last Promise (4550–4749) | 45–95 | 100 | 50 | cartography only |
| The Great Pyramid (5950–6199) | 90–110 | 206 | 92 | cartography only |

Skipped as non-playable: Wizard's Keep (staff-only), Midgaard Class Shop Extensions
(1 room/0 mobs), Player Housing, Public Society Halls, Topmost Area.

---

## Approach

Add 19 `quest_templates` rows across four areas using existing mobs as targets and
appropriate NPCs as offerers. All quests are kill or boss-kill quests (types 1 and 3).
No new mobs, rooms, or objects are required.

Each multi-quest area has at least one chain (quests linked via `prerequisite_template_id`).
The final quest of every chain, and every boss quest, carries an equipment reward defined
inline on the template row (reward_obj_* fields). Reward equipment is described below
each section. The Academy, which has only one quest, is standalone.

Rewards are calibrated to match quests at comparable level ranges already in the database.

Quest IDs begin at 159 (current max is 158).

---

## Quests by Area

### Academy of Adventure — 1 quest (ID 159)

Offerer: the Academy Guide (vnum 4910).

One quest only: defeat the wise old sage (4909), the named final mob of the tutorial
zone. Boss quest; carries an equipment reward.

| ID | Title | Type | Prereq | Targets | Kill needed | Min lvl | Max lvl | Gold | QP | Exp |
|----|-------|------|--------|---------|-------------|---------|---------|------|----|-----|
| 159 | Academy final exercise: the Sage | 3 | — | {4909} | 1 | 1 | 10 | 800 | 2 | 14000 |

**Equipment reward (ID 159):**
- short: `@@Wan academy graduate's field badge@@N`
- name: `academy graduate field badge`
- long: `A small enameled badge marking completion of Academy field training lies here.`
- wear_flags: 128 (wrist), extra_flags: 2097344, weight: 1, item_apply: 512

Accept/completion message in the voice of the Academy Guide framing it as the
graduation exercise before heading into the wider world.

---

### Scorched Wastes — 4 quests (IDs 160–163)

Offerers: Port Mafdet NPCs (vnums 3875 and 3886), which already offer quests for the
adjacent Scorching Sands.

Boss mobs (Pharaoh's Wrath 5296, Khepri 5297, Scorpion Queen 5298, Dune Colossus 5299)
are level 90 despite the area's stated max of 85; quest level caps reflect actual mob
level rather than area label.

Chain: **160 → 161** (gnoll interdiction requires predator census). Quest 161 and boss
quest 163 both carry equipment rewards.

| ID | Title | Type | Prereq | Targets | Kill needed | Min lvl | Max lvl | Gold | QP | Exp |
|----|-------|------|--------|---------|-------------|---------|---------|------|----|-----|
| 160 | Scorched Wastes predator census | 1 | — | {5280, 5281, 5282} | 0 | 65 | 80 | 4200 | 4 | 900000 |
| 161 | Gnoll raider interdiction: Scorched Wastes | 1 | 160 | {5283, 5285, 5290} | 0 | 68 | 82 | 4800 | 5 | 1100000 |
| 162 | Undead containment: Scorched Wastes circuit | 1 | — | {5284, 5289, 5293, 5295} | 0 | 72 | 85 | 5500 | 5 | 1300000 |
| 163 | Scarab Lord extermination writ | 3 | — | {5297} | 1 | 78 | 95 | 7500 | 7 | 2500000 |

Target breakdown:
- 160: sand viper (5280), dust scorpion (5281), desert vulture (5282)
- 161: bone gnoll (5283), dune raider (5285), nomad marauder (5290)
- 162: sand wraith (5284), bleached skeleton (5289), scorched mummy (5293), sandstorm wraith (5295)
- 163: Khepri the Scarab Lord (5297)

**Equipment rewards:**

ID 161 (chain final):
- short: `@@ya gnoll raider's bone talisman@@N`
- name: `gnoll raider bone talisman`
- long: `A crude talisman of gnoll bone and dried sinew taken from a desert raider lies here.`
- wear_flags: 128 (wrist), extra_flags: 2097344, weight: 2, item_apply: 256

ID 163 (boss):
- short: `@@yKhepri's scarab signet@@N`
- name: `khepri scarab signet ring`
- long: `A heavy ring carved with the image of the Scarab Lord, taken from his remains, lies here.`
- wear_flags: 8192 (finger), extra_flags: 136315072, weight: 2, item_apply: 32

---

### Rakuen, City of the Last Promise — 6 quests (IDs 164–169)

Offerers: the Dispatch Registrar (4650), the Bloom Council Steward (4682), and
the Archive Custodian (4684), who already offers the Rakuen city district survey.

Civic NPCs (guards, stewards, wardens, clerks) are excluded as targets. The Causeway
Enforcement Marshal (4681) and Queue Lane Enforcer (4677) are borderline enforcement
figures used as targets, not offerers. The Drowned-Root Revenant (4662) is undead and
clearly hostile.

Chains: **165 → 166** (revenant banishment requires root-leech eradication); **167 → 169**
(Dast writ requires separatist courier interdiction). Quest 166, and boss quest 169,
carry equipment rewards.

| ID | Title | Type | Prereq | Targets | Kill needed | Min lvl | Max lvl | Gold | QP | Exp | Offerer |
|----|-------|------|--------|---------|-------------|---------|---------|------|----|----|---------|
| 164 | Undercroft looter sweep | 1 | — | {4698, 4661, 4665} | 0 | 38 | 55 | 3000 | 3 | 340000 | 4650 |
| 165 | Root-leech eradication: Rakuen canopy | 1 | — | {4667, 4699, 4668} | 0 | 44 | 60 | 3600 | 4 | 440000 | 4682 |
| 166 | Drowned Root Revenant banishment | 3 | 165 | {4662} | 5 | 48 | 65 | 4200 | 5 | 600000 | 4682 |
| 167 | Separatist courier interdiction | 1 | — | {4688, 4673, 4695} | 0 | 50 | 70 | 4800 | 5 | 680000 | 4684 |
| 168 | Agitator suppression: Petition Quarter | 3 | — | {4674} | 8 | 42 | 58 | 3800 | 4 | 380000 | 4650 |
| 169 | Senior Steward accountability: Dast removal writ | 3 | 167 | {4690} | 1 | 72 | 95 | 9000 | 8 | 2800000 | 4682 |

Target breakdown:
- 164: Undercroft Scavenger (4698), Root-Rot Scavenger (4661), Promenade Ruin Looter (4665)
- 165: Waterlogged Root-Leech (4667), Ash-Slick Root-Leech (4699), Scavenger Hound (4668)
- 166: Drowned-Root Revenant (4662)
- 167: Quiet Separatist Courier (4688), Black-Register Smuggler (4673), Quiet Separatist Organizer (4695)
- 168: Petition Agitator (4674)
- 169: Senior Steward Colwen Dast (4690)

**Equipment rewards:**

ID 166 (chain final):
- short: `@@ga drowned-root revenant binding cord@@N`
- name: `drowned root revenant binding cord`
- long: `A length of rootwood cord etched with banishment marks, taken from the revenant's binding, lies here.`
- wear_flags: 128 (wrist), extra_flags: 2097344, weight: 2, item_apply: 65536

ID 169 (chain final + boss):
- short: `@@pColwen Dast's official signet@@N`
- name: `colwen dast official signet ring`
- long: `The official signet ring of Senior Steward Colwen Dast, stripped of its authority upon his removal.`
- wear_flags: 8192 (finger), extra_flags: 136315072, weight: 1, item_apply: 32

---

### The Great Pyramid — 8 quests (IDs 170–177)

Offerers: Port Mafdet NPCs — the same vnum 3857 already used for the Ancient Pyramid
and Northern Pyramid cartography commissions, plus 3886 (used for Cinderteeth/Scorching
Sands), 3856 (Mafdet city survey), and 3875.

The pyramid has 92 mobs across levels 100–110 with 8 named level-110 bosses. Quests
progress from entry-zone sweeps through to the final two boss writs.

Chains: **170 → 171** (scarab clearance requires entry zone survey); **174 → 176 → 177**
(Amenhotep requires cult strike; Pharaoh requires Amenhotep). Quests 171, 176, and 177
carry equipment rewards.

| ID | Title | Type | Prereq | Targets | Kill needed | Min lvl | Max lvl | Gold | QP | Exp | Offerer |
|----|-------|------|--------|---------|-------------|---------|---------|------|----|----|---------|
| 170 | Great Pyramid entry zone: surface threat assessment | 1 | — | {5950, 5996, 5998} | 0 | 90 | 104 | 7500 | 6 | 2500000 | 3857 |
| 171 | Scarab infestation clearance: outer chambers | 1 | 170 | {6001, 5956, 5976} | 0 | 92 | 104 | 7800 | 6 | 2700000 | 3875 |
| 172 | Crypt undead suppression: pyramid corridors | 1 | — | {5951, 5955, 5960, 6017} | 0 | 92 | 105 | 8000 | 7 | 2800000 | 3886 |
| 173 | Serpent den reduction: pyramid chambers | 1 | — | {5952, 5966, 5977} | 0 | 92 | 106 | 8200 | 7 | 2900000 | 3857 |
| 174 | Cult operative strike: pyramid interior | 1 | — | {5963, 6007, 6011} | 0 | 95 | 106 | 8500 | 7 | 3000000 | 3886 |
| 175 | Golem construct decommission: inner sanctum | 1 | — | {5961, 6003, 6009, 6012} | 0 | 95 | 108 | 8800 | 8 | 3100000 | 3875 |
| 176 | High Priest Amenhotep: final judgment | 3 | 174 | {6031} | 1 | 100 | 110 | 10000 | 9 | 3500000 | 3856 |
| 177 | Pharaoh Ramesses: eternal reign disputed | 3 | 176 | {6036} | 1 | 105 | 110 | 12000 | 10 | 4000000 | 3856 |

Target breakdown:
- 170: Entombed Soldier (5950), Pyramid Guard (5996), Tomb Robber (5998)
- 171: Scarab Swarm (6001), Shadow Scarab (5956), Brood Scarab (5976)
- 172: Ossuary Crawler (5951), Festering Ghoul (5955), Catacomb Wight (5960), Desert Ghoul (6017)
- 173: Crypt Asp (5952), Deep Viper (5966), Pit Viper (5977)
- 174: Subterranean Cultist (5963), Pyramid Cult Fanatic (6007), Blood Cultist (6011)
- 175: Crypt Golem (5961), Guardian Golem (6003), Hieroglyph Construct (6009), Stone Sentinel (6012)
- 176: High Priest Amenhotep (6031)
- 177: Pharaoh Ramesses the Eternal (6036)

**Equipment rewards:**

ID 171 (chain final):
- short: `@@ya scarab-shell ward token@@N`
- name: `scarab shell ward token`
- long: `A flat disc of polished scarab shell engraved with a ward against swarm-creatures lies here.`
- wear_flags: 128 (wrist), extra_flags: 2097344, weight: 2, item_apply: 65536

ID 176 (boss):
- short: `@@yHigh Priest Amenhotep's ceremonial ankh@@N`
- name: `high priest amenhotep ceremonial ankh`
- long: `A golden ankh bearing the High Priest's personal seal, taken from his chamber, rests here.`
- wear_flags: 32768 (hold), extra_flags: 136315072, weight: 3, item_apply: 512

ID 177 (chain final + boss):
- short: `@@ythe Eternal Pharaoh's cartouche seal@@N`
- name: `eternal pharaoh cartouche seal`
- long: `A heavy golden cartouche bearing Pharaoh Ramesses's royal seal, torn from his burial wrappings, lies here.`
- wear_flags: 2097344, extra_flags: 136315072, weight: 5, item_apply: 1536

---

## Affected Files / Repos

- **Database**: 19 `INSERT` statements into `quest_templates`. No schema changes.
- **acktng**: No code changes. The quest system already supports all types used here (1 and 3).
- **tngdb**: No changes needed; it reads quest data from the same DB.

## Trade-offs

- All four offerer NPCs in Rakuen and the Academy are mobs that already exist in-area,
  so players don't need to travel to a hub city to pick up quests. This matches the
  pattern used for most mid-range areas.
- Scorched Wastes and Great Pyramid quests funnel through Port Mafdet NPCs, consistent
  with how the neighboring Scorching Sands and Pyramids are handled.
- Boss quests for Scorched Wastes (Khepri, ID 166) and Great Pyramid (IDs 179–180)
  have max_level set above the area's stated level ceiling to reflect the actual mob
  levels, following the same pattern used in Cinderteeth and Great Northern Forest.
- The Academy has one standalone quest with no in-area chain; a two-quest chain is not
  possible without adding a second quest, which conflicts with the one-quest design
  decision. The single boss quest carries an equipment reward as a graduation token.
- Accept and completion messages are left blank here; these need to be written in the
  same bureaucratic/institutional register used across the rest of the quest set.
  If preferred, messages can be included before implementation.
