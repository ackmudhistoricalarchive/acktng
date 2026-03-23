# Low-Level Quests: Midgaard Only

## Problem

44 quest templates with `max_level <= 50` are currently spread across all five hub
cities (Midgaard, Kiess, Kowloon, Mafdet). 19 of them are offered by NPCs outside
Midgaard (Kiess postmaster/cartographer/registrar, Kowloon postmaster/gate captain,
Mafdet reach provisioner). Within Midgaard the postmaster alone holds 17 of the 25
existing Midgaard assignments.

The goal: all quests where `max_level <= 50` should be offered exclusively in Midgaard,
distributed across thematically appropriate AI-dialogue NPCs rather than concentrated
on the postmaster.

## Approach

Update `offerer_vnum` for all 44 affected quests to Midgaard AI-dialogue NPCs
(vnums 1100–1124). Assignments are made by thematic fit. No new AI flags required.

## Quest Assignments

| ID | Title | Level Range | Proposed Offerer | Rationale |
|---|---|---|---|---|
| 151 | Whispering Forest Preserve cartography | 1–10 | 1101 — the gate warden | Woodland area just outside city gates |
| 55 | Gloamvault threshold audit: novice interdiction | 5–12 | 1112 — the temple priest | Undead/crypt theme |
| 73 | Park vermin suppression | 5–18 | 1103 — a street merchant | City quality-of-life pest control |
| 67 | Gloamvault cartography commission | 5–20 | 1109 — a noble citizen | Exploration patron |
| 139 | Public Dungeons cartography | 5–35 | 1114 — Aldric the Magister | Scholarly dungeon survey |
| 56 | Compact jurisdiction survey: vault functionary assessment | 8–14 | 1114 — Aldric the Magister | Scholarly/juridical |
| 50 | Violet Compact enforcement: stalker suppression | 8–15 | 1122 — the postmaster | Law enforcement/compact |
| 72 | Catacomb entry clearance | 8–22 | 1112 — the temple priest | Catacombs/undead |
| 15 | Gateworks vermin purge | 10–19 | 1100 — a city guard | City gate maintenance |
| 51 | Covenant precedent recovery: militant purge | 10–17 | 1115 — Seraphel the Archwarden | Covenant authority |
| 68 | Nightfall catacombs cartography | 10–25 | 1112 — the temple priest | Catacombs |
| 75 | Chapel approach clearance | 10–25 | 1102 — a temple guardian | Chapel/sacred approach |
| 80 | Inlet cave pest clearance | 10–35 | 1101 — the gate warden | Clearing threats outside the gate |
| 53 | Jade Magistracy deep audit: warlock suppression | 14–20 | 1114 — Aldric the Magister | Magistracy/academic enforcement |
| 57 | Processional corridor threat assessment | 14–25 | 1100 — a city guard | Security corridor patrol |
| 52 | Registry priority warrant: Ossuary Champion | 15–20 | 1122 — the postmaster | Registry/warrant service |
| 6 | Road predator cull: Banner Hills to Dustward | 15–30 | 1104 — the caravan master | Road safety for trade routes |
| 69 | Sepulcher Pasture cartography | 15–30 | 1109 — a noble citizen | Exploration patron |
| 54 | Final covenant judgment: Matriarch Velastra | 18–20 | 1115 — Seraphel the Archwarden | Final judgment/authority |
| 49 | Reclaim labor disruption order | 18–25 | 1121 — Oswin the Storekeeper | Labor/trade disruption |
| 58 | Toll-Warden removal order | 18–25 | 1123 — the quartermaster | Toll route logistics |
| 60 | Covenant fracture investigation | 18–25 | 1112 — the temple priest | Covenant/religious inquiry |
| 26 | Bell-Post Line Reopening | 18–38 | 1123 — the quartermaster | Postal/logistics route |
| 1 | Route reconnaissance: Forest of Confusion | 20–39 | 1101 — the gate warden | Route recon near city |
| 16 | Lantern Road wolf cull | 20–39 | 1104 — the caravan master | Road predator clearance |
| 70 | Umbra Heartspire cartography | 25–40 | 1114 — Aldric the Magister | Arcane site survey |
| 59 | Sealed Names injunction: Matriarch strike | 22–25 | 1115 — Seraphel the Archwarden | Injunction/authority |
| 61 | Final audit termination: Sepulcher Lich | 22–25 | 1118 — Draga Ironclad | Dangerous boss, needs military authority |
| 9 | Convoy route security: Dustward predator sweep | 20–34 | 1104 — the caravan master | Convoy route protection |
| 27 | Mirrorbark Predator Census | 28–49 | 1104 — the caravan master | Census along trade routes |
| 62 | Compact intake disruption sweep | 22–30 | 1121 — Oswin the Storekeeper | Trade intake disruption |
| 41 | Violet archive stabilization sweep | 26–34 | 1119 — Thalindra the Enchanter | Magical archive |
| 63 | Reliquary regent injunction | 27–34 | 1115 — Seraphel the Archwarden | Injunction/authority |
| 42 | Evermeet reliquary quieting | 28–36 | 1119 — Thalindra the Enchanter | Magical reliquary |
| 64 | Null-halo archive seizure | 29–35 | 1119 — Thalindra the Enchanter | Magical archive seizure |
| 71 | Violet Eclipse Cathedral cartography | 20–35 | 1114 — Aldric the Magister | Cathedral scholarship |
| 65 | Thorn cardinal writ execution | 31–35 | 1115 — Seraphel the Archwarden | Writ of authority |
| 66 | Violet throne final deposition | 33–35 | 1118 — Draga Ironclad | Deposition by force |
| 43 | Lantern syndic penumbra audit | 30–38 | 1121 — Oswin the Storekeeper | Trade syndicate audit |
| 44 | Mirror-Queen injunction service | 34–39 | 1120 — Gorven Bladewright | Combat-capable injunction |
| 45 | Noctivar deposition writ | 36–40 | 1118 — Draga Ironclad | Military writ |
| 7 | Tollbreak crew interdiction | 30–44 | 1123 — the quartermaster | Toll route interdiction |
| 10 | Greenveil Spur reconnaissance | 35–49 | 1101 — the gate warden | Route reconnaissance |
| 154 | Sultan's Palace cartography | 30–50 | 1114 — Aldric the Magister | Palace/historical scholarship |

## NPC Distribution

| NPC | Quests | Count |
|---|---|---|
| 1100 — a city guard | 15, 57 | 2 |
| 1101 — the gate warden | 151, 1, 80, 10 | 4 |
| 1102 — a temple guardian | 75 | 1 |
| 1103 — a street merchant | 73 | 1 |
| 1104 — the caravan master | 6, 9, 16, 27 | 4 |
| 1109 — a noble citizen | 67, 69 | 2 |
| 1112 — the temple priest | 55, 60, 68, 72 | 4 |
| 1114 — Aldric the Magister | 56, 53, 71, 70, 139, 154 | 6 |
| 1115 — Seraphel the Archwarden | 51, 54, 59, 63, 65 | 5 |
| 1118 — Draga Ironclad | 61, 45, 66 | 3 |
| 1119 — Thalindra the Enchanter | 41, 42, 64 | 3 |
| 1120 — Gorven Bladewright | 44 | 1 |
| 1121 — Oswin the Storekeeper | 49, 43, 62 | 3 |
| 1122 — the postmaster | 50, 52 | 2 |
| 1123 — the quartermaster | 26, 58, 7 | 3 |

15 distinct NPCs. Max load: 6 (Aldric the Magister). Postmaster reduced from 17 → 2.

## Affected Files

| File | Change |
|---|---|
| Database `quest_templates` | `offerer_vnum` updated for 44 rows |
| `docs/quests_spec.md` | Offerer column updated for affected IDs |
