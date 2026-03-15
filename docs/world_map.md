# World Map — ACK!TNG

Geographic overview of all areas: surface zones, wilderness regions, dungeon
complexes, and their interconnections. Use `world_links.md` for the exact
room-level cross-area exit table. Use `area_index.md` for vnum envelopes and
level ranges.

---

## ASCII Overview Map

The map reads north-to-south (top-to-bottom) and west-to-east (left-to-right).
Solid lines (`──`, `│`) are walkable routes. Dagger (†) marks areas reachable
only through a portal, special door, or planar transition from the named anchor.

```
                              NORTH ↑

                    ┌─────────────────────────┐
                    │       K O W L O O N     │  delta port, Neon Covenant
                    │       [14000–14099]      │  northern sea trade hub
                    └────────────┬────────────┘
                                 │  Lantern Road
                    ┌────────────┴────────────┐
                    │   Great Northern Forest  │  old-growth wilderness,
                    │      [15500–15999]       │  Rootbound cult threat,
                    └────────────┬────────────┘  chartered trade artery
                                 │
    ┌──────────┐   Roc Road  ┌───┴────────────┐              ┌──────────────┐
    │  KIESS   ◄─────────────►  M I D G A A R D├─────────────► Akh'enet      │
    │ [13000–  │  [3201–3299]│   [3001–3200]  │  east road  │ [2000–2149]  │
    │  13099]  │             └───────┬────────┘             └──────┬───────┘
    └──────────┘                     │ south                        │
    (Roc Road west                   │                       ┌──────┴───────┐
     terminus, Evermeet              │                       │ Eastern      │
     ruins, anti-Conclave)           │                       │ Desert       │
                                     │                       │ [8500–8599]  │
    ┌─────────────────────┐          │                       └──┬────┬──┬───┘
    │  Roc Road Byways    │  ←side   │                 Arroyo ──┘    │  └── Scorched Wastes
    │  [3450–3649]        │  branches│              [19900–020]      │      [22500–22574]
    └─────────────────────┘          │                              │
                                     │                    Sultan's Palace†
    ┌─────────────────────┐          │                    [30325–30399]
    │  Violet Eclipse     │          │                              │
    │  Cathedral†         │          │                    Lost City†
    │  [30250–30324]      │          │                    [30400–30479]
    │  (off Roc Road)     │          │                              │
    └─────────────────────┘          │                    ┌─────────┴───────┐
                                     │                    │  G R E A T      │
    ┌─────────────────────┐          │                    │  O A S I S      │
    │ Farmlands           │          │                    │  [8600–8699]    │
    │ [7200–7299]         │          │                    │  (Deepwell hub) │
    │ (off Roc Road NW)   │          │                    └───┬─────────┬───┘
    └─────────────────────┘          │                   N Oasis     S Oasis
                                     │                [8700–8899] [9800–9999]
    ═══════════════════  WESTERN FOREST  REGION  ══════════════════════════
    │                                 │                        │           │
    │  Forest of Confusion            │                   N Pyramid   S Pyramid
    │  [9600–9799]                    │                [10125–10324][11450–11649]
    │  ├─ Withered Depths [1000–1149] │                        │           │
    │  └─ Verdant Depths [10000–124]  │                        └─────┬─────┘
    │                                 │                              │
    │  GRAVEYARD CLUSTER:             │                    ┌─────────┴───────┐
    │  Graveyard [11000–11099]        │                    │ G R E A T       │
    │  ├─ Thornwood [1150–1299]       │                    │ P Y R A M I D   │
    │  ├─ Shadowmere [1400–1600]      │                    │ [30480–30699]   │
    │  └─ Crimson Mist [8350–8499]    │                    │ (Black Sun Shard│
    │     └─ Crimson Castle [11300+]  │                    │  containment)   │
    │                                 │                    └─────────┬───────┘
    ═══════════════════════════════════════════════════════════════════════════
                                     │                              │
                         ┌───────────┴──────────┐         ┌────────┴────────┐
                         │  Eccentric Woodland   │         │ Scorching Sands │
                         │  [7300–7699]          │         │ [29900–29974]   │
                         └───────────┬───────────┘         └────────┬────────┘
                                     │                              │
                         ┌───────────┴───────────┐        ┌────────┴────────┐
                         │   R A K U E N          │        │ Saltglass Reach │
                         │   [16000–16199]         │        │ (littoral belt) │
                         │   disaster-recovery    │        └────────┬────────┘
                         │   city, south terminus │                 │
                         └────────────────────────┘        ┌────────┴────────┐
                                                            │  M A F D E T   │
                                                            │  (sea port)     │
                                                            │  maritime law   │
                                                            │  terminus       │
                                                            └─────────────────┘

                              SOUTH ↓ / SEA →
```

---

## Dungeon and Special Areas

These areas are attached to the world via portals, special doors, or
dimensional transitions rather than standard overland exits. Anchor room listed
where known.

### Midgaard Anchor Points

| Area | Vnum Range | Levels | Access |
|------|-----------|--------|--------|
| Gloamvault of Kel'Shadra | 30100–30174 | 5–20 | Midgaard 3005 |
| Public Dungeons | 9550–9599 | 5–35 | Midgaard 3014 |
| Catacombs of Nightfall | 21000–21199 | 10–25 | Midgaard 3025 |
| Sepulcher Pasture | 30175–30249 | 15–30 | Midgaard 3030 |
| Academy of Adventure | 20200–20299 | 1–10 | Midgaard 3110 |
| Void Citadel of Kel'Shadra | 610–999 | 150–170 | Midgaard 3232 |
| Umbra Heartspire | 11200–11274 | 25–40 | Midgaard 3232 |
| Crypts of Kel'Shadra | 9400–9549 | 150–170 | (portal/special) |
| Sunken Sanctum | 9500–9599 | 150 | (portal/special) |

### Desert Anchor Points

| Area | Vnum Range | Levels | Access |
|------|-----------|--------|--------|
| Sultan's Palace | 30325–30399 | 30–50 | Eastern Desert 8504 |
| Lost City (Khepra-Lesh) | 30400–30479 | 35–55 | Eastern Desert 8523 |
| Arroyo | 19900–20024 | 55–75 | Eastern Desert 8503 |
| Scorched Wastes | 22500–22574 | 65–85 | Eastern Desert 8507 |
| Khar'Daan (Sunken Necropolis) | 31000–31099 | 60–80 | (desert/oasis deep) |

### Off-Road and Planar

| Area | Vnum Range | Levels | Access |
|------|-----------|--------|--------|
| Violet Eclipse Cathedral | 30250–30324 | 20–35 | Roc Road 3202 |
| Forest Preserve | 20800–20849 | 1–10 | Academy 20200 |
| Hell | 1601–1753 | 100–130 | Vecna Tomb link |
| Tomb of Vecna | 20600–20799 | 95–110 | (portal; links to Hell) |
| Maze of Icarus | 4950–4999 | 150 | (special access) |
| Lost Isle | 5001–5099 | 50–70 | (special access) |
| Minotaur Keep | 1300–1399 | 110–130 | (special access) |
| The Hotel | 4400–4449 | 30–60 | (special access) |
| Miskatonic Asylum | 3900–3999 + 16000–16099 + 5100–5199 | 30–135 | (special access) |

### System Areas (not part of the game world)

| Area | Vnum Range | Notes |
|------|-----------|-------|
| The Wizard's Keep (Newadept) | 166–265 | new-player spawn, level 150 |
| The Arena | 300–309 | 1–10 |
| Player Housing | 310–609 | housing system |

---

## Major Travel Routes

### The Five-City Network
The five cities form a co-dependency chain: origin claim → overland custody →
dispute adjudication → hazard screening → ocean transfer.

```
  KIESS  ──── Roc Road ────  MIDGAARD  ───  [overland east]  ───  MAFDET
   (W)                         (C)                                  (E coast)
                                │ Great Northern Forest
                           KOWLOON (N)
                                │ Eccentric Woodland
                           RAKUEN (S)
```

### Roc Road (West Corridor)
`MIDGAARD 3101` → `Roc Road 3201–3299` → `KIESS 13059`

Branch points along Roc Road:
- Room 3202 → Violet Eclipse Cathedral (30250)
- Room 3252 ↔ Farmlands (7200)
- Roc Road Byways (3450–3649): side warrens and wilderness branches

### The Lantern Road (North Corridor)
`MIDGAARD 3010` → `Great Northern Forest 15512–15994` → `KOWLOON 14095`

The Lantern Road is the primary north-south artery, chartered jointly by
Midgaard and Kowloon. It winds through 500 rooms of old-growth forest with
escalating wilderness threats further from the road.

### The Eastern Road and Desert Corridor
`MIDGAARD 3120` → `Eastern Desert 8500–8599`

From the Eastern Desert:
- `8503` → Arroyo (canyon)
- `8504` → Sultan's Palace
- `8507` → Scorched Wastes
- `8508` ↔ Akh'enet
- `8519` → **Great Oasis** (corridor hub)
- `8523` → Lost City (Khepra-Lesh)

From the **Great Oasis** [8600–8699]:
- `8601–8609` ↔ Northern Oasis [8700–8899] (multiple crossing points)
- `8699` → Southern Oasis [9800–9999]

From the **Northern Oasis** [8700–8899]:
- `8879` ↔ Northern Pyramid [10125–10324]

Continuing south past the pyramids to the sea:
```
  Great Pyramid [30480–30699]
       │
  Scorching Sands [29900–29974]
       │
  Saltglass Reach  (inland → littoral transition)
       │
  MAFDET  (maritime port, Saltglass Reach terminus)
```

### The Southern Route (Eccentric Woodland to Rakuen)
`MIDGAARD 3190` → `Eccentric Woodland 7302–7699` → `Rakuen 16000` *(planned)*

Rakuen is a disaster-recovery city south of the woodland, accessible via the
woodland's southern path (room 7698). The reverse link at Rakuen 16000 is a
planned stub awaiting the Rakuen area build.

### The Western Forest (Forest of Confusion Network)
The Forest of Confusion and its connected depths form an interconnected
wilderness west of the Roc Road corridor:
- `Forest of Confusion [9600–9799]` ↔ `Withered Depths [1000–1149]`
- `Forest of Confusion [9600–9799]` ↔ `Verdant Depths [10000–10124]`

The **Graveyard Cluster** (southwest wilderness):
- `Graveyard [11000–11099]` ↔ `Thornwood [1150–1299]`
- `Graveyard [11000–11099]` ↔ `Shadowmere [1400–1600]`
- `Graveyard 11010` → `Crimson Mist [8350–8499]`
- `Crimson Mist 8488` → `Crimson Castle [11300–11449]`

---

## The Oasis-Pyramid Corridor: Civilizational Spine

The corridor is the civilizational core of the world — a logistics arc built
from desert roads, cistern law, and oasis administration that predates every
current dynasty. It runs roughly north-to-south through the desert east of
Midgaard.

```
  Eastern Desert  ─── [entrance from Midgaard east road] ───►
       │
  ┌────┴──────────────────────────────────────────────────────┐
  │  Akh'enet             Sultan's Palace    Lost City        │
  │  (legal-mortuary      (desert stronghold) (Khepra-Lesh,  │
  │   relay city)                             failed hinge)  │
  └────┬──────────────────────────────────────────────────────┘
       │ south
  ┌────┴───────────────────────────────────────────────────┐
  │          G R E A T   O A S I S                         │
  │          primary Deepwell Confluence vent              │
  │          administrative heart of the corridor          │
  └──────┬──────────────────────┬──────────────────────────┘
   NW    │                      │ SE
  ┌──────┴────────┐      ┌──────┴──────────┐
  │ Northern      │      │ Southern Oasis  │
  │ Oasis         │      │ casualty/convoy │
  │ verification  │      │ processing hub  │
  │ [8700–8899]   │      │ [9800–9999]     │
  └──────┬────────┘      └──────┬──────────┘
         │                      │
  ┌──────┴────────┐      ┌──────┴──────────┐
  │ Northern      │      │ Southern        │
  │ Pyramid       │      │ Pyramid         │
  │ (Sand         │      │ (Moon Sovereign,│
  │  Sovereign,   │      │ descent tiers,  │
  │  ascending    │      │ underworld law) │
  │  guardian     │      │ [11450–11649]   │
  │  floors)      │      └──────┬──────────┘
  │ [10125–10324] │             │
  └──────┬────────┘             │
         └──────────┬───────────┘
                    │
         ┌──────────┴──────────┐
         │   G R E A T         │
         │   P Y R A M I D     │
         │   Solar Court       │
         │   Black Sun Shard   │
         │   containment       │
         │   [30480–30699]     │
         └──────────┬──────────┘
                    │ southeast
         ┌──────────┴──────────┐
         │   Scorching Sands   │
         │   heat-cert transit │
         │   [29900–29974]     │
         └──────────┬──────────┘
                    │
         ┌──────────┴──────────┐
         │   Saltglass Reach   │
         │   inland→maritime   │
         │   legal handoff     │
         └──────────┬──────────┘
                    │
         ┌──────────┴──────────┐
         │    M A F D E T      │
         │    maritime port    │
         │    sea terminus     │
         │    dual-attestation │
         └─────────────────────┘
```

**North-south gradient** (per oasis-pyramid corridor lore):
secular-administrative in the north → death-saturated in the south.
The funerary weight increases with each step south: Northern Oasis
handles death administratively; Southern Oasis processes casualties as
civic function; the pyramids are death-architecture in monumental form.

---

## Khar'Daan (Sunken Necropolis)

Khar'Daan [31000–31099] is a sunken necropolis in the desert with hydraulic
funerary bureaucracy. Its relic-quarantine doctrine migrated through the
Saltglass Reach's Sealed Route to Mafdet, informing that city's hazard
classification system. Geographic access: deep desert, likely via the oasis
system or specialized caravan routes.

---

## Key Institutional Geography Notes

- **Deepwell Confluence**: a single limestone aquifer feeding all three oases.
  The Northern Oasis draws from the weakest vent; the Great Oasis from the
  primary vent; the Southern Oasis from a secondary vent. Shared water = shared
  institutional ancestry despite divergent cultures.

- **The Iseth Reach**: vanished river that once connected the Eastern Desert
  to all three oasis aquifer systems and all three pyramid cisterns. Its drying
  (accelerated by the Ninth Meridian Rite failure) severed every hydraulic link
  in the corridor simultaneously, producing the fragmented state the world is
  in now.

- **The Black Sun Shard** (Great Pyramid apex): the whole civilizational arc —
  Keepers of Measure documentation obsession, oasis water-law, pyramid
  containment architecture — was built as an anti-amnesia system against this
  cosmic fragment of anti-light. All three pyramids represent three responses:
  contain it (Great Pyramid), guard the container (Northern Pyramid), descend
  toward it (Southern Pyramid).

- **The Five-City Chain**: Midgaard (registry) → Kowloon (throughput) →
  Kiess (adjudication) → Rakuen (hazard/disaster) → Mafdet (maritime export).
  No city fully chose this arrangement; none can easily exit it.
