# Quest Giver Vnum Assignment

## Problem

53 quest templates (IDs 106–158) have `offerer_vnum` values that point to non-existent or
incorrect NPCs:

| Broken vnum | Status | Quests |
|---|---|---|
| `3015` | Exists but is a sporecrown drone (enemy mob, not AI) | 110, 112, 114, 121, 122, 124, 126 |
| `3340` | Does not exist (old Kiess postmaster) | 106, 107, 108, 113, 119, 123, 127 |
| `3440` | Does not exist (old Kowloon postmaster) | 109, 111, 118, 129, 130, 133, 148 |
| `3539` | Does not exist (old Mafdet postmaster) | 115, 116, 117, 120, 125, 132, 134, 135, 136, 141, 143, 144, 150, 152, 153, 156, 157, 158 |
| `939`  | Does not exist | 131, 139, 145, 146, 147, 149, 151, 154 |
| `3349` | Does not exist | 137, 140, 142, 155 |
| `4339` | Does not exist (old Rakuen registrar) | 128, 138 |

These are legacy vnums from a previous schema never updated when cities were rebuilt.

## Approach

Reassign all 53 quests to valid, AI-dialogue-flagged NPCs in the five hub cities
(Midgaard, Kiess, Kowloon, Mafdet, Rakuen). No AI flags need to be added to any NPC —
all targets are already flagged. Assignments are distributed across thematically
appropriate NPCs rather than defaulting to postmasters.

The fix is a single SQL `UPDATE` statement per affected quest, executed as a migration.

## Quest Assignments

Assignments follow two rules:
1. **City affiliation**: match the geographic/cultural proximity of the quest's target area
2. **Thematic fit**: choose an NPC whose role relates to the quest's subject matter

### Saltglass Reach — quests 106–120, 158 (target area: Saltglass Reach, vnums 6300–6699)

The Saltglass Reach is a coastal salt-flat region accessed from Kowloon and Mafdet.
Quests are split between the two cities by subject matter.

| ID | Title | Proposed Offerer | City |
|---|---|---|---|
| 106 | Saltglass Reach cartography survey: Mirror Flats | 3779 — a caravan caller | Kowloon |
| 107 | Saltglass Reach cartography survey: Glasswind to Tidemouth | 3776 — a civic scribe | Kowloon |
| 108 | Reach Warden toll restoration | 3773 — a plaza inspector | Kowloon |
| 109 | Red Sand Outrider interdiction | 3780 — a customs clerk | Kowloon |
| 110 | Synod whisper cell disruption | 3771 — a wall patrol sergeant | Kowloon |
| 111 | Cairn scavenger expulsion | 3875 — a Sand-Sea Carter journeyman | Mafdet |
| 112 | Sealed Route quarantine enforcement | 3768 — the Iron Gate captain | Kowloon |
| 113 | Glassworm burrower queen extermination | 3877 — a Quay Concord bosun | Mafdet |
| 114 | Whisper Cell commandant elimination | 3770 — the Tide Gate captain | Kowloon |
| 115 | Shoreward Revenant banishment | 3888 — the Strand Rememberer elder | Mafdet |
| 116 | Toll-Marshal of the Three Routes: writ of dissolution | 3887 — the Tide Ledger head archivist | Mafdet |
| 117 | Arbiter of the Conversion: final jurisdictional reckoning | 3903 — the Tide Ledger QLC specialist | Mafdet |
| 118 | Cairn tablet recovery: forged precedents | 3776 — a civic scribe | Kowloon |
| 119 | Glass-field thermal anomaly investigation | 3778 — a shrine keeper | Kowloon |
| 120 | Tidemouth jurisdiction enforcement sweep | 3881 — a Harbor Warden | Mafdet |
| 158 | Saltglass Reach cartography commission | 3887 — the Tide Ledger head archivist | Mafdet |

### Scorching Sands — quests 121–127 (target area: Scorching Sands, vnums 5350–5449)

The Scorching Sands is a desert trade-route zone with a corruption/forgery theme.
Mafdet's Ledger House and Sand-Sea Carter NPCs are the best thematic fit.

| ID | Title | Proposed Offerer | City |
|---|---|---|---|
| 121 | Scorching Sands cartography survey: Three Spines to Cinder Gate | 3875 — a Sand-Sea Carter journeyman | Mafdet |
| 122 | Burn Ledger recovery writ | 3887 — the Tide Ledger head archivist | Mafdet |
| 123 | Seal fraud interdiction | 3874 — a Ledger House clerk | Mafdet |
| 124 | Cauterist re-firing disruption | 3873 — a harbor sailor | Mafdet |
| 125 | The Crucible Regent dismantled | 3884 — a Harbor Warden captain | Mafdet |
| 126 | Channel casualty triage drill | 3871 — a caravan drover | Mafdet |
| 127 | Witness-stick cohort verification | 3874 — a Ledger House clerk | Mafdet |

### Cartography commissions — quests 128–157

These cover 30 diverse areas across the game world. Assigned by geographic and
cultural proximity to a hub city.

**Rakuen** (deep/adept-tier areas; Rakuen is the furthest hub, appropriate for
high-difficulty or exotic commissions):

| ID | Title | Proposed Offerer | Notes |
|---|---|---|---|
| 128 | Void Citadel cartography commission (150–170) | 4690 — Senior Steward Colwen Dast | Highest authority, adept-only |
| 137 | Kel'Shadra crypts cartography commission (150–170) | 4684 — the Archive Custodian | Already gives Cinderteeth Mtns quest |
| 138 | Sunken Sanctum cartography commission (150) | 4689 — the Upper Dispatch Registrar | Deep-tier assignment |
| 142 | Verdant Depths cartography commission (75–95) | 4685 — the Ember Warden Liaison Captain | Jungle/depths theme |
| 149 | Rakuen city district survey (1–170) | 4682 — the Bloom Council Steward | Civic survey |

**Kiess** (northern and forest-adjacent areas):

| ID | Title | Proposed Offerer | Notes |
|---|---|---|---|
| 131 | Shadowmere cartography commission (30–60) | 3671 — a Forest Confusion scout | Wilderness expertise |
| 133 | Eccentric Woodland cartography commission (18–92) | 3679 — the Kiess stablemaster | Knows forest routes |
| 139 | Public Dungeons cartography commission (5–35) | 3682 — an enchanter practitioner | Dungeon expertise |
| 140 | Forest of Confusion cartography commission (10–60) | 3671 — a Forest Confusion scout | Directly thematic |
| 145 | Kiess city district survey (1–170) | 3660 — a Cartographer of Kiess | City's own cartographer |
| 148 | Great Northern Forest cartography commission (1–170) | 3683 — the North Gate customs clerk | Knows the northern approaches |
| 151 | Whispering Forest Preserve cartography commission (1–10) | 3680 — a Temple Concord lay sister | Low-level, gentle area |

**Kowloon** (eastern, trade-route, and desert-adjacent areas):

| ID | Title | Proposed Offerer | Notes |
|---|---|---|---|
| 129 | Withered Depths cartography commission (80–100) | 3754 — Warden Instructor Ohn | High-difficulty zone |
| 130 | Thornwood cartography commission (50–80) | 3779 — a caravan caller | Overland route knowledge |
| 134 | Eastern Desert cartography commission (30–100) | 3765 — the Harbor chandler | Eastern trade connections |
| 135 | Great Oasis cartography commission (40–60) | 3760 — the Salt-Rice provisioner | Desert supply routes |
| 136 | Northern Oasis cartography commission (45–65) | 3756 — the Green Banner merchant | Desert trade |
| 141 | Southern Oasis cartography commission (50–70) | 3764 — the Iron Bit stablemaster | Caravan route expertise |
| 146 | Kowloon city district survey (1–170) | 3753 — Banker Jien | City records |

**Mafdet** (southern, coastal, desert, and pyramid areas):

| ID | Title | Proposed Offerer | Notes |
|---|---|---|---|
| 132 | Sands of Akh'enet cartography commission (70–90) | 3888 — the Strand Rememberer elder | Ancient desert knowledge |
| 143 | Northern Pyramid cartography commission (100–120) | 3857 — a Hazard Scribe reagent dealer | Archaeological/hazardous |
| 144 | Southern Pyramid cartography commission (120–140) | 3881 — a Harbor Warden | Southern approach security |
| 147 | Mafdet city district survey (1–170) | 3856 — the Strandline Compact banker | City records |
| 150 | The Arroyo cartography commission (55–75) | 3878 — a harbor merchant | Trade route knowledge |
| 152 | Scorched Wastes cartography commission (65–85) | 3899 — a Tidewall sentinel | Harsh terrain experience |
| 153 | Scorching Sands cartography commission (65–85) | 3903 — the Tide Ledger QLC specialist | Adjacent to Scorching Sands quests |
| 154 | Sultan's Palace cartography commission (30–50) | 3855 — a Reach Provisioner | Expedition provisioning |
| 155 | Lost City cartography commission (35–55) | 3888 — the Strand Rememberer elder | Knowledge of lost places |
| 156 | Ancient Pyramid cartography commission (90–100) | 3857 — a Hazard Scribe reagent dealer | Archaeological |
| 157 | Khardaan necropolis cartography commission (60–80) | 3888 — the Strand Rememberer elder | Death lore expertise |

## Affected Files

| File | Change |
|---|---|
| Database `quest_templates` | `offerer_vnum` updated for 53 rows |
| `docs/quests_spec.md` | Offerer column updated for IDs 106–158 |

## NPC Distribution Summary

No single NPC receives more than 4 quests. Distribution across hubs:

| City | NPCs used | Quest count |
|---|---|---|
| Kowloon | 3768, 3770, 3771, 3773, 3776, 3778, 3779, 3780 + 3 existing (3767) | 7 (new) + existing |
| Mafdet | 3871, 3873, 3874, 3875, 3877, 3878, 3881, 3884, 3887, 3888, 3899, 3903 + existing (3886, 3851) | 19 (new) + existing |
| Kiess | 3660, 3671, 3679, 3680, 3682, 3683 + existing (3651, 3652, 3661, 3677, 3678) | 7 (new) + existing |
| Rakuen | 4682, 4684, 4685, 4689, 4690 + existing (4650, 4682, 4684) | 5 (new) + existing |
| Midgaard | (none new — existing postmaster/quartermaster/invasion warden continue) | 0 new |

## Trade-offs

| Consideration | Notes |
|---|---|
| Mafdet receives the most new assignments (19) | The Scorching Sands and Saltglass Reach are its natural hinterland, and Mafdet has the richest pool of non-postmaster NPCs among the hubs |
| Some NPCs appear twice | The Strand Rememberer elder (3888) takes 3 cartography commissions and the civic scribe (3776) takes 2 Reach quests — all thematically consistent |
| City surveys use city-specific NPCs | Each city's survey goes to a non-postmaster official in that city |
| Midgaard is not used for new assignments | Midgaard already handles Roc Road, Midgaard district, and many lower-ID quests; adding more would concentrate it further |
