# Quests Specification

This document specifies the static quest file format currently used by ACK and catalogs the static quest templates that are **actually loadable** at runtime.

## Scope and loading behavior

- Static quest templates are loaded from `../quests/<n>.prop` at startup, where `<n>` runs from `1` through `QUEST_MAX_STATIC_QUESTS` (`105`). Missing files are ignored.
- Each loaded file is assigned a zero-based static quest ID (`file number - 1`).
- The parser ignores blank lines, comment lines beginning with `#`, and leading/trailing whitespace around each significant line.
- Repository snapshot: `105` quest files exist in `quests/`, all in the loadable range `1-105`.
- Additional quest templates (IDs 106–158) are stored directly in the `quest_templates` database table and have no corresponding `.prop` files. They follow the same field semantics documented below.
- The current maximum quest template ID in the database is **158**. New quests added after the `.prop` file era go directly into `quest_templates`; do not create new `.prop` files.

## File format (`.prop`)

Each quest file has this structure:
1. **Title line**
   Freeform text used as the quest title.
2. **Numeric definition line** (11 integers):

```text
prerequisite_static_id  type  num_targets  kill_needed  min_level  max_level  offerer_vnum  reward_gold  reward_qp  reward_exp  <ignored>
```

- The 11th integer is read but discarded (`%*d` in sscanf). It exists as a legacy trailing field.
3. **Target vnum line**
   Space-delimited integer vnums. The parser reads up to `QUEST_MAX_TARGETS` (`5`) values.
4. **Accept message line** *(optional)*
   Message shown on acceptance.
5. **Completion message line** *(optional)*
   Message shown on completion hand-in.
6. **Custom reward-object block** *(all fields optional; block is triggered by its presence in the file, not by any flag value)*:
   - reward object short description
   - reward object keywords
   - reward object long description
   - reward object wear flags (int)
   - reward object extra flags (int)
   - reward object weight (int)
   - reward object item-apply selector (int)

## Field definitions

- `prerequisite_static_id`: static quest ID that must already be completed; `-1` means no prerequisite.
- `type`: quest objective type.
  - `1` = kill-variety objective (kill one of each listed target).
  - `2` = collect-items objective.
  - `3` = kill-count objective against a single target (`kill_needed` required).
  - `4` = cartography objective (explore every room in the area identified by the target vnum's area). Cartography quests may not be part of a chain.
- `num_targets`: number of valid entries expected from the target vnum line. For type `4`, this is `1` (the first room vnum of the target area).
- `kill_needed`: required kill count (used by type `3`; `0` for non-count objectives).
- `min_level`, `max_level`: pseudo-level gating for acceptance. A `max_level` of `5` means no effective upper limit (since level 5 is effectively unattainable as an upper bound in context).
- `offerer_vnum`: NPC vnum of the mob that offers the static quest and receives turn-in. See postmaster vnums below.
- `reward_gold`, `reward_qp`: fixed completion rewards.
- `reward_exp`: experience reward granted on completion.
- Custom reward-object block fields override the generated reward object identity and flags when present. The base object is always created from `OBJ_VNUM_MUSHROOM` and fully overridden by the reward block.

## Known postmaster vnums

| City | Vnum | Notes |
|---|---|---|
| Midgaard | 3015 | Main postmaster |
| Kiess | 3340 | Canonical; 3360 is a legacy alias normalized at load |
| Kowloon | 3440 | Canonical; 3460 is a legacy alias normalized at load |
| Mafdet | 3539 | Harbor authority postmaster |
| Rakuen | 4339 | Dispatch registrar; Zone 1 of Rakuen (vnums 4339–4346) |

## ITEM_WEAR flag values (for reward object wear_flags field)

These are bit flags built using the `BIT_N` macro where `BIT_0 = 0`, `BIT_1 = 1`, `BIT_2 = 2`, `BIT_3 = 4`, `BIT_N = 1 << (N-1)` for N ≥ 1.

| Flag | Value | Notes |
|---|---|---|
| `ITEM_TAKE` | 8388608 | BIT_24 |
| `ITEM_WEAR_FACE` | 16 | BIT_5 |
| `ITEM_WEAR_NECK` | 128 | BIT_8 |
| `ITEM_WEAR_SHOULDERS` | 512 | BIT_10 |
| `ITEM_WEAR_ARMS` | 1024 | BIT_11 |
| `ITEM_WEAR_WRIST` | 2048 | BIT_12 |
| `ITEM_WEAR_HANDS` | 4096 | BIT_13 |
| `ITEM_WEAR_FINGER` | 8192 | BIT_14 |
| `ITEM_WEAR_HOLD_HAND` | 32768 | BIT_16 |
| `ITEM_WEAR_ABOUT` | 65536 | BIT_17 |
| `ITEM_WEAR_BODY` | 262144 | BIT_19 |
| `ITEM_WEAR_LEGS` | 1048576 | BIT_21 |

## ITEM extra flag values (for reward object extra_flags field)

| Flag | Value | Notes |
|---|---|---|
| `ITEM_MAGIC` | 64 | BIT_7 |
| `ITEM_NODROP` | 128 | BIT_8 |
| `vamp` (`ITEM_QUEST_REWARD`) | 2097152 | BIT_22 — must be set on all quest reward objects; named `vamp` in `buildtab.c` |
| `ITEM_LOOT` | 67108864 | BIT_27 |
| `ITEM_BOSS` | 134217728 | BIT_28 |

Common combinations used in quest reward items:
- `2097344` = ITEM_MAGIC + ITEM_NODROP + vamp (non-boss quest rewards)
- `136315072` = ITEM_MAGIC + ITEM_NODROP + vamp + ITEM_BOSS (boss-kill quest rewards)

## Currently loadable static quests (`1.prop`-`105.prop`)

| File | Static ID | Title | Prereq ID | Type | Targets | Kill Needed | Level Range | Offerer | Rewards |
|---|---:|---|---:|---:|---|---:|---|---:|---|
| `1.prop` | 0 | Route reconnaissance: Forest of Confusion | -1 | 1 | `2340 2341 2342` | 0 | 20-39 | 1101 | 491 gold, 2 qp |
| `2.prop` | 1 | Conclave residue containment | 0 | 3 | `302` | 8 | 40-54 | 3340 | 2200 gold, 3 qp |
| `3.prop` | 2 | Eastern trade road interdiction | 1 | 1 | `4960 4965 4988` | 0 | 55-69 | 931 | 1366 gold, 4 qp |
| `4.prop` | 3 | Sealed-warrant courier disruption | 2 | 1 | `1832 2898 3174` | 0 | 70-84 | 931 | 5000 gold, 5 qp |
| `5.prop` | 4 | Blightfront quarantine sweep | 3 | 1 | `2751 321 341` | 0 | 85+ | 931 | 7000 gold, 6 qp |
| `6.prop` | 5 | Road predator cull: Banner Hills to Dustward | -1 | 1 | `1313 1314 1316` | 0 | 15-30 | 1104 | 800 gold, 2 qp |
| `7.prop` | 6 | Tollbreak crew interdiction: River Crossing | -1 | 3 | `1324` | 6 | 30-44 | 1123 | 2000 gold, 3 qp |
| `8.prop` | 7 | Shadow trade disruption: Crossroads to Greenveil | 7 | 1 | `1325 1326 1327` | 0 | 40-54 | 1123 | 3500 gold, 4 qp |
| `9.prop` | 8 | Convoy route security: Dustward predator sweep | -1 | 3 | `1316` | 8 | 20-34 | 1104 | 1000 gold, 2 qp |
| `10.prop` | 9 | Greenveil Spur reconnaissance: spirit clearance | -1 | 1 | `1323 1325` | 0 | 35-49 | 1101 | 1800 gold, 3 qp |
| `11.prop` | 10 | Inlet smuggler crackdown: cave network purge | 10 | 3 | `1327` | 10 | 50-64 | 3677 | 4000 gold, 5 qp |
| `12.prop` | 11 | Ruin custodian suppression: Weathered Causeway | -1 | 1 | `1319 1320 1321 1322` | 0 | 50-64 | 1123 | 4500 gold, 5 qp |
| `13.prop` | 12 | Coastal hazard survey: Western Shore predators | -1 | 1 | `1317 1318 1315` | 0 | 40-54 | 3751 | 2500 gold, 3 qp |
| `14.prop` | 13 | Transcontinental corridor assessment: full-road interdiction | 13 | 1 | `1313 1324 1319 1327 1318` | 0 | 60-74 | 3751 | 6000 gold, 6 qp |
| `15.prop` | 14 | Gateworks vermin purge | -1 | 1 | `21000 21001 21003` | 0 | 10-19 | 1100 | 800 gold, 2 qp |
| `16.prop` | 15 | Lantern Road wolf cull | -1 | 3 | `3744` | 10 | 20-39 | 931 | 591 gold, 2 qp |
| `17.prop` | 16 | Mosswater smuggler interdiction | -1 | 1 | `3769 3770 3776` | 0 | 30-54 | 3340 | 792 gold, 3 qp |
| `18.prop` | 17 | Northern Crown predator survey | -1 | 1 | `3819 3822 3827` | 0 | 40-69 | 3460 | 2800 gold, 3 qp |
| `19.prop` | 18 | Ironpine Rise Ashfang suppression | -1 | 1 | `3784 3785 3791` | 0 | 45-69 | 931 | 1116 gold, 4 qp |
| `20.prop` | 19 | Rootbound perimeter probe | -1 | 1 | `3801 3802` | 0 | 55-79 | 3340 | 3400 gold, 4 qp |
| `21.prop` | 20 | Rootbound lieutenant strike | 19 | 1 | `3807 3835` | 0 | 70-94 | 3340 | 5200 gold, 5 qp |
| `22.prop` | 21 | Thornmother sanctum assault | 20 | 3 | `3824` | 1 | 85+ | 3340 | 8000 gold, 7 qp |
| `23.prop` | 22 | Cairn-Keeper exorcism | -1 | 3 | `3800` | 6 | 60-84 | 3460 | 4000 gold, 4 qp |
| `24.prop` | 23 | Ashfang war-chief elimination | 22 | 1 | `3806 3812` | 0 | 75-99 | 3460 | 6000 gold, 5 qp |
| `25.prop` | 24 | Oathbreaker wraith banishment | 23 | 1 | `3823 3834` | 0 | 80+ | 3460 | 1632 gold, 6 qp |
| `26.prop` | 25 | Bell-Post Line Reopening | -1 | 3 | `2343` | 8 | 18-38 | 1123 | 1700 gold, 2 qp |
| `27.prop` | 26 | Mirrorbark Predator Census | -1 | 1 | `2346 2347 2349` | 0 | 28-49 | 1104 | 2400 gold, 3 qp |
| `28.prop` | 27 | Conclave Survey Slate Recovery | -1 | 2 | `2372` | 0 | 34-58 | 3460 | 2800 gold, 3 qp |
| `29.prop` | 28 | Neogi Warrant Service | -1 | 3 | `2367` | 1 | 35-60 | 3340 | 1116 gold, 4 qp |
| `30.prop` | 29 | Ashen Lattice Containment Sweep | -1 | 1 | `2359 2360 2362` | 0 | 42-68 | 931 | 3900 gold, 4 qp |
| `31.prop` | 30 | Wardline Reconsolidation | -1 | 1 | `2361 2358` | 0 | 48-74 | 3340 | 4300 gold, 4 qp |
| `32.prop` | 31 | Yugoloth Contract Severance | 30 | 3 | `2380` | 1 | 56-82 | 3340 | 5200 gold, 5 qp |
| `33.prop` | 32 | Ymmas Lair Strike | 31 | 3 | `2381` | 1 | 60-90 | 3340 | 7000 gold, 6 qp |
| `34.prop` | 33 | Cave Route Pacification | -1 | 1 | `2375 2379` | 0 | 44-70 | 3460 | 4200 gold, 4 qp |
| `35.prop` | 34 | Chitin Crown Decapitation | 33 | 3 | `2377` | 1 | 52-80 | 3460 | 6100 gold, 5 qp |
| `36.prop` | 35 | Fen kingpin decapitation | -1 | 3 | `3776` | 1 | 46-72 | 931 | 1366 gold, 4 qp |
| `37.prop` | 36 | Crown alpha hunt | -1 | 1 | `3826 3828` | 0 | 82-120 | 3340 | 6800 gold, 6 qp |
| `38.prop` | 37 | Briar throne breach | -1 | 3 | `3824` | 1 | 90+ | 3460 | 9000 gold, 7 qp |
| `39.prop` | 38 | Covenant ash purge | 37 | 1 | `3806 3828` | 0 | 92+ | 3460 | 2539 gold, 8 qp |
| `40.prop` | 39 | Oathnight final rite | 38 | 1 | `3823 3834` | 0 | 95+ | 3460 | 11000 gold, 9 qp |
| `41.prop` | 40 | Violet archive stabilization sweep | -1 | 1 | `3070 3073 3087` | 0 | 26-34 | 1119 | 2800 gold, 3 qp |
| `42.prop` | 41 | Evermeet reliquary quieting | -1 | 3 | `3095` | 6 | 28-36 | 1119 | 1116 gold, 3 qp |
| `43.prop` | 42 | Lantern syndic penumbra audit | -1 | 1 | `3105 3108 3112` | 0 | 30-38 | 1121 | 3800 gold, 4 qp |
| `44.prop` | 43 | Mirror-Queen injunction service | 42 | 1 | `3115 3116` | 0 | 34-39 | 1120 | 5200 gold, 5 qp |
| `45.prop` | 44 | Noctivar deposition writ | 43 | 3 | `3118` | 1 | 36-40 | 1118 | 7000 gold, 6 qp |
| `46.prop` | 45 | Preserve command decapitation order | -1 | 1 | `4645 4646 4650` | 0 | 90-124 | 931 | 1932 gold, 7 qp |
| `47.prop` | 46 | Preserve border attrition campaign | 45 | 3 | `4650` | 6 | 105-140 | 931 | 2539 gold, 8 qp |
| `48.prop` | 47 | Warden removal final writ | -1 | 1 | `4646 4645 4650` | 0 | 120+ | 931 | 12000 gold, 10 qp |
| `49.prop` | 48 | Reclaim labor disruption order | -1 | 3 | `21014` | 5 | 18-25 | 1121 | 1600 gold, 3 qp |
| `50.prop` | 49 | Violet Compact enforcement: stalker suppression | 54 | 3 | `30115` | 5 | 8-15 | 1122 | 1200 gold, 3 qp |
| `51.prop` | 50 | Covenant precedent recovery: militant purge | -1 | 1 | `30105 30108 30116` | 0 | 10-17 | 1115 | 1400 gold, 3 qp |
| `52.prop` | 51 | Registry priority warrant: Ossuary Champion | 48 | 1 | `30113` | 0 | 15-20 | 1122 | 2000 gold, 4 qp |
| `53.prop` | 52 | Jade Magistracy deep audit: warlock suppression | 49 | 3 | `30110` | 4 | 14-20 | 1114 | 1800 gold, 4 qp |
| `54.prop` | 53 | Final covenant judgment: Matriarch Velastra | 51 | 1 | `30114` | 0 | 18-20 | 1115 | 3000 gold, 5 qp |
| `55.prop` | 54 | Gloamvault threshold audit: novice interdiction | -1 | 1 | `30100 30102 30117` | 0 | 5-12 | 1112 | 800 gold, 2 qp |
| `56.prop` | 55 | Compact jurisdiction survey: vault functionary assessment | -1 | 1 | `30101 30103 30118` | 0 | 8-14 | 1114 | 1000 gold, 2 qp |
| `57.prop` | 56 | Processional corridor threat assessment | -1 | 1 | `21005 21006 21010` | 0 | 14-25 | 1100 | 1200 gold, 2 qp |
| `58.prop` | 57 | Toll-Warden removal order | 55 | 3 | `21026` | 1 | 18-25 | 1123 | 1800 gold, 3 qp |
| `59.prop` | 58 | Sealed Names injunction: Matriarch strike | 56 | 3 | `21029` | 1 | 22-25 | 1115 | 3000 gold, 5 qp |
| `60.prop` | 59 | Covenant fracture investigation | -1 | 1 | `21011 21015 21017` | 0 | 18-25 | 1112 | 1400 gold, 3 qp |
| `61.prop` | 60 | Final audit termination: Sepulcher Lich | 58 | 3 | `21030` | 1 | 22-25 | 1118 | 3200 gold, 5 qp |
| `62.prop` | 61 | Compact intake disruption sweep | -1 | 1 | `30252 30255 30261` | 0 | 22-30 | 1121 | 2400 gold, 3 qp |
| `63.prop` | 62 | Reliquary regent injunction | -1 | 3 | `30301` | 1 | 27-34 | 1115 | 3800 gold, 4 qp |
| `64.prop` | 63 | Null-halo archive seizure | 61 | 1 | `30296 30302` | 0 | 29-35 | 1119 | 4600 gold, 5 qp |
| `65.prop` | 64 | Thorn cardinal writ execution | 62 | 3 | `30303` | 1 | 31-35 | 1115 | 5600 gold, 6 qp |
| `66.prop` | 65 | Violet throne final deposition | 63 | 3 | `30304` | 1 | 33-35 | 1118 | 7600 gold, 8 qp |
| `67.prop` | 66 | Gloamvault cartography commission | -1 | 4 | `30100` | — | 5-20 | 1109 | 12000 gold, 20 qp |
| `68.prop` | 67 | Nightfall catacombs cartography commission | -1 | 4 | `21000` | — | 10-25 | 1112 | 14000 gold, 20 qp |
| `69.prop` | 68 | Sepulcher Pasture cartography commission | -1 | 4 | `30175` | — | 15-30 | 1109 | 24000 gold, 30 qp |
| `70.prop` | 69 | Umbra Heartspire cartography commission | -1 | 4 | `11200` | — | 25-40 | 1114 | 45000 gold, 50 qp |
| `71.prop` | 70 | Violet Eclipse Cathedral cartography commission | -1 | 4 | `30250` | — | 20-35 | 1114 | 35000 gold, 40 qp |
| `72.prop` | 71 | Catacomb entry clearance | -1 | 1 | `21000 21001 21002` | 0 | 8-22 | 1112 | 1200 gold, 3 qp |
| `73.prop` | 72 | Park vermin suppression | -1 | 3 | `21000` | 8 | 5-18 | 1103 | 800 gold, 2 qp |
| `74.prop` | 73 | Midgaard city district survey | -1 | 4 | `3001` | — | 1-60 | 3099 | 5000 gold, 8 qp |
| `75.prop` | 74 | Chapel approach clearance | -1 | 1 | `21003 21004 21005` | 0 | 10-25 | 1102 | 1400 gold, 3 qp |
| `76.prop` | 75 | Roc Road Byways cartographic survey | -1 | 4 | `3450` | — | 10-80 | 3015 | 3000 gold, 8 qp |
| `77.prop` | 76 | Banner Hills wolf suppression | -1 | 1 | `3455 3456 3488` | 0 | 20-55 | 13001 | 2200 gold, 5 qp |
| `78.prop` | 77 | Evermeet underfoundation shade clearance | -1 | 3 | `3475` | 3 | 45-80 | 13011 | 3500 gold, 6 qp |
| `79.prop` | 78 | Evermeet remnant guardian removal | 76 | 3 | `3476` | 1 | 60-100 | 13001 | 6500 gold, 9 qp |
| `80.prop` | 79 | Inlet cave pest clearance | -1 | 1 | `3472 3473` | 0 | 10-35 | 1101 | 1500 gold, 4 qp |
| `81.prop` | 80 | Cinderteeth foothills raid suppression | -1 | 1 | `6125 6126 6143` | 0 | 120-155 | 3340 | 7000 gold, 6 qp |
| `82.prop` | 81 | Warlord's blood oath, cancelled | 80 | 1 | `6130` | 0 | 120-155 | 3340 | 9500 gold, 8 qp + bracer |
| `83.prop` | 82 | Chalkwind archive undead threat survey | -1 | 1 | `6147 6153 6162` | 0 | 125-160 | 3340 | 7500 gold, 6 qp |
| `84.prop` | 83 | Ironpost archive corridor: revenant suppression | 82 | 1 | `6150 6156 6154` | 0 | 125-160 | 3340 | 8000 gold, 7 qp |
| `85.prop` | 84 | Warden-commander interdiction: Ironpost Archive | 83 | 1 | `6151` | 0 | 125-160 | 3340 | 11000 gold, 9 qp + neck pendant |
| `86.prop` | 85 | Glasswash hostile element assessment | -1 | 1 | `6164 6166 6171` | 0 | 130-160 | 3440 | 7500 gold, 6 qp |
| `87.prop` | 86 | Glasswash colossus elimination | 85 | 1 | `6172` | 0 | 130-160 | 3440 | 11500 gold, 9 qp + ring |
| `88.prop` | 87 | Bellspine resonance entity clearance | -1 | 1 | `6184 6187 6188` | 0 | 135-165 | 3440 | 8000 gold, 6 qp |
| `89.prop` | 88 | Anchor construct decommission | 87 | 1 | `6194` | 0 | 135-165 | 3440 | 12000 gold, 10 qp + bracer |
| `90.prop` | 89 | Western Fold doctrine apparatus: institutional assessment | -1 | 1 | `6214 6215 6216` | 0 | 140-170 | 3015 | 8500 gold, 7 qp |
| `91.prop` | 90 | Vent-Oracle silencing | 89 | 1 | `6221` | 0 | 140-170 | 3015 | 13000 gold, 10 qp + mask |
| `92.prop` | 91 | Ember Crown approach: transit threat survey | -1 | 1 | `6224 6227 6228` | 0 | 145-170 | 3539 | 8000 gold, 6 qp |
| `93.prop` | 92 | Ember Crown lava serpent cull | 91 | 3 | `6229` | 3 | 145-170 | 3539 | 8500 gold, 7 qp |
| `94.prop` | 93 | Secondtooth caldera watcher: territorial claim broken | 92 | 1 | `6238` | 0 | 148-170 | 3539 | 14000 gold, 11 qp + mantle |
| `95.prop` | 94 | Western Fold approach: bound spirit clearance | -1 | 1 | `6207 6208 6211` | 0 | 135-165 | 4339 | 7500 gold, 6 qp |
| `96.prop` | 95 | Oracle ground reckoning: Fold approach enforcement | 94 | 1 | `6209 6210 6213` | 0 | 135-165 | 4339 | 9000 gold, 8 qp + ring |
| `97.prop` | 96 | Cinderteeth Mountains cartography commission | -1 | 4 | `6124` | — | 125-170 | 4339 | 22000 gold, 30 qp |
| `98.prop` | 97 | Ash Cult forward position: zealot reduction | -1 | 3 | `6204` | 6 | 140-170 | 3015 | 8000 gold, 6 qp |
| `99.prop` | 98 | Ember Throne Patriarch: geological verdict appealed | -1 | 1 | `6242` | 0 | 150-170 | 3015 | 16000 gold, 12 qp + ring |
| `100.prop` | 99 | Foothills wildlife verification: Cinderteeth entry zone | -1 | 1 | `6131 6132 6140` | 0 | 120-155 | 3340 | 6500 gold, 5 qp |
| `101.prop` | 100 | Glasswash extraction site: labor ghost suppression | -1 | 3 | `6174` | 5 | 130-160 | 3440 | 7000 gold, 6 qp |
| `102.prop` | 101 | Mafdet mineral route: sulfur element threat reduction | -1 | 1 | `6169 6170 6173` | 0 | 135-165 | 3539 | 7500 gold, 6 qp |
| `103.prop` | 102 | Ghost of the last Ventspeaker: oracle ground cleared | -1 | 1 | `6212` | 0 | 140-170 | 3539 | 13000 gold, 10 qp + arms binding |
| `104.prop` | 103 | Bellspine resonance locust suppression | -1 | 3 | `6198` | 6 | 135-165 | 4339 | 7000 gold, 6 qp |
| `105.prop` | 104 | Cinderteeth entry zone: volcanic creature survey | -1 | 1 | `6134 6135 6136` | 0 | 120-155 | 4339 | 6500 gold, 5 qp |

## Database-only quest templates (IDs 106–158)

These templates have no `.prop` file. All fields use the same semantics as above.
`—` in the Kill Needed column indicates a cartography quest (type 4).

| Template ID | Title | Prereq ID | Type | Targets | Kill Needed | Level Range | Offerer | Rewards |
|---:|---|---:|---:|---|---:|---|---:|---|
| 106 | Saltglass Reach cartography survey: Mirror Flats | -1 | 1 | `6313 6315 6318` | 0 | 55-70 | 3779 | 4000 gold, 4 qp |
| 107 | Saltglass Reach cartography survey: Glasswind to Tidemouth | 105 | 1 | `6332 6334 6373` | 0 | 60-75 | 3776 | 6000 gold, 5 qp + item |
| 108 | Reach Warden toll restoration | -1 | 3 | `6318` | 8 | 55-70 | 3773 | 1266 gold, 4 qp + item |
| 109 | Red Sand Outrider interdiction | -1 | 1 | `6350 6351 6352` | 0 | 60-75 | 3780 | 4500 gold, 5 qp |
| 110 | Synod whisper cell disruption | -1 | 1 | `6337 6339 6338` | 0 | 58-72 | 3771 | 4000 gold, 4 qp |
| 111 | Cairn scavenger expulsion | -1 | 3 | `6346` | 10 | 60-72 | 3875 | 3800 gold, 4 qp + item |
| 112 | Sealed Route quarantine enforcement | 109 | 1 | `6364 6365 6361` | 0 | 65-78 | 3768 | 5500 gold, 5 qp + item |
| 113 | Glassworm burrower queen extermination | -1 | 3 | `6345` | 1 | 62-75 | 3877 | 5000 gold, 5 qp + item |
| 114 | Whisper Cell commandant elimination | 109 | 3 | `6364` | 1 | 68-78 | 3770 | 6000 gold, 6 qp + item |
| 115 | Shoreward Revenant banishment | -1 | 3 | `6395` | 1 | 70-80 | 3888 | 7000 gold, 6 qp + item |
| 116 | Toll-Marshal of the Three Routes: writ of dissolution | 114 | 3 | `6396` | 1 | 75-80 | 3887 | 1832 gold, 7 qp + item |
| 117 | Arbiter of the Conversion: final jurisdictional reckoning | 115 | 3 | `6397` | 1 | 78-80 | 3903 | 11000 gold, 9 qp + item |
| 118 | Cairn tablet recovery: forged precedents | -1 | 2 | `6405` | 0 | 60-75 | 3776 | 4500 gold, 5 qp |
| 119 | Glass-field thermal anomaly investigation | -1 | 1 | `6340 6341 6342` | 0 | 65-78 | 3778 | 5500 gold, 5 qp |
| 120 | Tidemouth jurisdiction enforcement sweep | -1 | 1 | `6377 6378 6379` | 0 | 60-72 | 3881 | 5000 gold, 5 qp |
| 121 | Scorching Sands cartography survey: Three Spines to Cinder Gate | -1 | 1 | `5365 5357 5361` | 0 | 65-80 | 3875 | 7000 gold, 6 qp |
| 122 | Burn Ledger recovery writ | -1 | 2 | `5381` | 0 | 66-82 | 3887 | 6200 gold, 5 qp |
| 123 | Seal fraud interdiction | -1 | 3 | `5356` | 10 | 67-83 | 3874 | 6400 gold, 5 qp + item |
| 124 | Cauterist re-firing disruption | 121 | 1 | `5368 5369 5371` | 0 | 70-84 | 3873 | 7200 gold, 6 qp + item |
| 125 | The Crucible Regent dismantled | 123 | 3 | `5374` | 1 | 75-85 | 3884 | 2539 gold, 8 qp + item |
| 126 | Channel casualty triage drill | -1 | 2 | `5382` | 0 | 66-81 | 3871 | 6100 gold, 5 qp |
| 127 | Witness-stick cohort verification | -1 | 1 | `5359 5360 5361` | 0 | 67-82 | 3874 | 6500 gold, 5 qp |
| 128 | Void Citadel cartography commission | -1 | 4 | `152` | — | 150-170 | 4690 | 500000 gold, 250 qp + item |
| 129 | Withered Depths cartography commission | -1 | 4 | `400` | — | 80-100 | 3754 | 160000 gold, 110 qp + item |
| 130 | Thornwood cartography commission | -1 | 4 | `550` | — | 50-80 | 3779 | 100000 gold, 75 qp + item |
| 131 | Shadowmere cartography commission | -1 | 4 | `700` | — | 30-60 | 3671 | 55000 gold, 55 qp + item |
| 132 | Sands of Akh'enet cartography commission | -1 | 4 | `950` | — | 70-90 | 3888 | 130000 gold, 95 qp + item |
| 133 | Eccentric Woodland cartography commission | -1 | 4 | `1650` | — | 18-92 | 3679 | 140000 gold, 100 qp + item |
| 134 | Eastern Desert cartography commission | -1 | 4 | `2050` | — | 30-100 | 3765 | 170000 gold, 115 qp + item |
| 135 | Great Oasis cartography commission | -1 | 4 | `2150` | — | 40-60 | 3760 | 65000 gold, 60 qp + item |
| 136 | Northern Oasis cartography commission | -1 | 4 | `2250` | — | 45-65 | 3756 | 80000 gold, 68 qp + item |
| 137 | Kel'Shadra crypts cartography commission | -1 | 4 | `2450` | — | 150-170 | 4684 | 480000 gold, 240 qp + item |
| 138 | Sunken Sanctum cartography commission | -1 | 4 | `2500` | — | 150 | 4689 | 450000 gold, 230 qp + item |
| 139 | Public Dungeons cartography commission | -1 | 4 | `2551` | — | 5-35 | 1114 | 18000 gold, 22 qp + item |
| 140 | Forest of Confusion cartography commission | -1 | 4 | `2600` | — | 10-60 | 3671 | 58000 gold, 56 qp + item |
| 141 | Southern Oasis cartography commission | -1 | 4 | `2800` | — | 50-70 | 3764 | 90000 gold, 73 qp + item |
| 142 | Verdant Depths cartography commission | -1 | 4 | `3000` | — | 75-95 | 4685 | 150000 gold, 105 qp + item |
| 143 | Northern Pyramid cartography commission | -1 | 4 | `3150` | — | 100-120 | 3857 | 200000 gold, 140 qp + item |
| 144 | Southern Pyramid cartography commission | -1 | 4 | `3450` | — | 120-140 | 3881 | 280000 gold, 175 qp + item |
| 145 | Kiess city district survey | -1 | 4 | `3650` | — | 1-170 | 3660 | 8000 gold, 10 qp + item |
| 146 | Kowloon city district survey | -1 | 4 | `3750` | — | 1-170 | 3753 | 8000 gold, 10 qp + item |
| 147 | Mafdet city district survey | -1 | 4 | `3850` | — | 1-170 | 3856 | 8000 gold, 10 qp + item |
| 148 | Great Northern Forest cartography commission | -1 | 4 | `4050` | — | 1-170 | 3683 | 180000 gold, 120 qp + item |
| 149 | Rakuen city district survey | -1 | 4 | `4861` | — | 1-170 | 4682 | 8000 gold, 10 qp + item |
| 150 | The Arroyo cartography commission | -1 | 4 | `4750` | — | 55-75 | 3878 | 95000 gold, 78 qp + item |
| 151 | Whispering Forest Preserve cartography commission | -1 | 4 | `5000` | — | 1-10 | 1101 | 4000 gold, 6 qp + item |
| 152 | Scorched Wastes cartography commission | -1 | 4 | `5250` | — | 65-85 | 3899 | 110000 gold, 88 qp + item |
| 153 | Scorching Sands cartography commission | -1 | 4 | `5350` | — | 65-85 | 3903 | 110000 gold, 88 qp + item |
| 154 | Sultan's Palace cartography commission | -1 | 4 | `5750` | — | 30-50 | 1114 | 42000 gold, 42 qp + item |
| 155 | Lost City cartography commission | -1 | 4 | `5850` | — | 35-55 | 3888 | 52000 gold, 52 qp + item |
| 156 | Ancient Pyramid cartography commission | -1 | 4 | `6050` | — | 90-100 | 3857 | 175000 gold, 125 qp + item |
| 157 | Khardaan necropolis cartography commission | -1 | 4 | `6200` | — | 60-80 | 3888 | 105000 gold, 85 qp + item |
| 158 | Saltglass Reach cartography commission | -1 | 4 | `6300` | — | 55-80 | 3887 | 105000 gold, 85 qp + item |
