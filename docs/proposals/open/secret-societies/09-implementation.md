# Secret Societies: Implementation Checklist

## Phase 1: Core Data Structures and Save System

### Constants and Types (`src/headers/`)

- [ ] Add society constants to `typedefs.h`: `MAX_SOCIETY`, `MAX_SOCIETY_RANK`,
  `MAX_SOCIETY_REP`, `SOCIETY_NONE` through `SOCIETY_PALE_LEDGER`
- [ ] Add society flag constants to `config.h`: `SFLAG_INITIATED` through
  `SFLAG_COVER_ACTIVE`
- [ ] Add `ITEM_SOCIETY` item extra flag to `config.h`
- [ ] Add `WEAR_HIDDEN` wear location to `config.h`
- [ ] Add `CHANNEL_SOCIETY` channel flag to `config.h`
- [ ] Add `QUEST_TYPE_SOCIETY` quest type constant to `quest.h`
- [ ] Add mission objective type constants (`MISSION_OBJ_DELIVER` through
  `MISSION_OBJ_RITUAL`) to a new `society.h` header
- [ ] Add society NPC role constants (`SOCIETY_NPC_NONE` through
  `SOCIETY_NPC_CONTACT`) to `society.h`

### Struct Modifications (`src/headers/ack.h`)

- [ ] Add to `PC_DATA`: `society`, `society_rank`, `society_rep`,
  `society_flags`, `society_missions`, `society_joined`, `society_exposed`,
  `society_exposure_timer`, `society_kills`
- [ ] Add to `MOB_INDEX_DATA`: `society`, `society_npc_role`
- [ ] Add to `CHAR_DATA` (transient): `society_pvp_target`, `society_pvp_timer`

### Society Table (`src/const_society.c` — new file)

- [ ] Define `struct society_type` with fields: `name`, `abbr`, `base_city`,
  `hq_room`, `recruit_room`, `rival_society`, `passphrase`, `skill_first`,
  `skill_count`
- [ ] Populate `society_table[MAX_SOCIETY]` with data for all 6 societies
- [ ] Define rivalry matrix as a static 2D array

### Save System (`src/save/`)

- [ ] Increment `SAVE_REVISION` from 15 to 16
- [ ] Add `fwrite` block for society fields in `fwrite_char()` (only written
  when `society > 0`)
- [ ] Add `fread` cases for `Soc*` keys in `fread_char()` with defaults for
  `SAVE_REVISION < 16`
- [ ] Write unit test: save and load a player with society data, verify all
  fields round-trip correctly
- [ ] Write unit test: load a pre-revision-16 player file, verify society
  fields default to zero

### Global Declarations (`src/headers/globals.h`)

- [ ] Declare `extern const struct society_type society_table[]`
- [ ] Declare `extern SOCIETY_BOUNTY *first_bounty`
- [ ] Declare `gsn_*` variables for all society skills

---

## Phase 2: Core Commands and Membership

### Society Command (`src/act_society.c` — new file)

- [ ] Implement `do_society()` with subcommand dispatch: `info`, `who`, `say`,
  `missions`, `promote`, `leave`, `history`, `intel`, `bounty`, `bounties`,
  `accept`, `abandon`, `challenge`
- [ ] Register `society` command in `interp.c` command table
- [ ] Implement society channel via `CHANNEL_SOCIETY` in `do_society_say()`
- [ ] Implement `can_join_society()` prerequisite check
- [ ] Write unit test: `can_join_society()` returns FALSE for NPCs, already-
  members, disavowed within cooldown, below level requirements

### Recruitment System

- [ ] Implement passphrase detection in `do_say()` (`act_comm.c`): when a
  player says a society passphrase in the designated room to a recruiter NPC,
  trigger recruitment dialogue
- [ ] Implement initiation quest creation (uses existing quest infrastructure)
- [ ] Implement society join on initiation completion: set all PC_DATA fields,
  create sigil item
- [ ] Write unit test: passphrase matching is case-insensitive and room-specific

### Rank Advancement

- [ ] Implement `do_society_promote()`: check mission count, reputation, rank-
  specific requirements
- [ ] Write unit test: promotion requirements are correctly enforced at each
  rank boundary

### Departure and Expulsion

- [ ] Implement `do_society_leave()`: confirmation prompt, field clearing,
  sigil destruction, cooldown timer
- [ ] Implement automatic disavowal when reputation drops below -300
- [ ] Write unit test: leaving resets all fields; disavowal sets correct flags

---

## Phase 3: Society Skills

### Skill Registration

- [ ] Reserve skill indices 900-949 in `skill_table[]` for society skills
- [ ] Define all 36 society-specific skills (6 per society) plus 2 shared
  skills in `src/skills/skill_table_data.c`
- [ ] Add `gsn_*` declarations for all society skills in `globals.h`
- [ ] Implement `can_use_society_skill()` gating function in `act_society.c`

### Shared Skills

- [ ] Implement `do_dead_drop()`: create hidden container in room, flag with
  society ID, expiry timer
- [ ] Implement `do_society_sense()`: skill check, success/failure/critical-
  failure outcomes, cooldown
- [ ] Register `deaddrop` and `societysense` commands in `interp.c`
- [ ] Write unit test: dead drops are invisible to non-members, expire correctly
- [ ] Write unit test: society_sense skill check produces correct outcomes

### Per-Society Skills (implement in order of priority)

- [ ] Reckoning: `cipher_read`, `archive_search`, `forge_document`,
  `informant_network`, `shadow_copy`, `expose`
- [ ] Jade Eels: `assess_cargo`, `bribe`, `smuggle`, `black_market`,
  `price_fix`, `trade_embargo`
- [ ] Ash Cult: `ashmark`, `kindle`, `sabotage`, `ash_ward`, `invoke_cinder`,
  `pyroclasm`
- [ ] Quiet Hand: `trailmark`, `diplomatic_pouch`, `safe_passage`,
  `establish_cache`, `negotiate`, `trade_route`
- [ ] Jackal Synod: `relic_sense`, `false_manifest`, `tomb_ward`,
  `ancestor_query`, `seal_break`, `eclipsed_tongue`
- [ ] Pale Ledger: `crystal_attune`, `spirit_trace`, `prism_shield`,
  `extract_essence`, `conclave_cipher`, `pale_gate`
- [ ] Register all skill commands in `interp.c`
- [ ] Write unit tests for each skill's gating logic and basic behavior

### Practice Integration

- [ ] Extend `do_practice()` to recognize society handler NPCs as trainers
  for society skills
- [ ] Society handler checks `can_use_society_skill()` before teaching

---

## Phase 4: Mission System

### Mission Templates

- [ ] Define `struct society_mission_type` in `society.h`
- [ ] Create `src/const_society_missions.c` (new file) with the mission
  template table
- [ ] Write 5 mission templates per society (30 total) covering difficulties
  1-5

### Mission Flow

- [ ] Implement `do_society_missions()`: filter and display available missions
- [ ] Implement `do_society_accept()`: create QUEST_DATA entry, set flags
- [ ] Implement mission completion detection for each objective type
  (DELIVER, RETRIEVE, KILL, VISIT, SABOTAGE, SMUGGLE, TALK, RITUAL)
- [ ] Implement `society_mission_complete()`: reputation, gold, mission count
- [ ] Implement `society_mission_fail()`: reputation penalty, exposure trigger
- [ ] Implement `do_society_abandon()`: half penalty, cooldown
- [ ] Write unit test: mission acceptance validates rank and reputation
- [ ] Write unit test: mission completion awards correct reputation and gold
- [ ] Write unit test: mission failure applies correct penalties

### Mission Timer

- [ ] Implement `society_mission_update()` in `update.c` tick loop
- [ ] Write unit test: mission auto-fails when timer reaches 0

---

## Phase 5: Exposure System

### Exposure Mechanics

- [ ] Implement exposure triggers in mission failure, guard detection, and
  `society_sense` paths
- [ ] Implement `society_exposure_update()` in `update.c`: decrement timer,
  clear exposure on expiry
- [ ] Modify `who` output to show `[!]` + society abbreviation for exposed
  players
- [ ] Write unit test: exposure timer decrements and clears correctly
- [ ] Write unit test: exposure makes society abbreviation visible

### Guard Integration

- [ ] Implement `spec_society_guard()` spec_fun: check entrants, attack
  non-members in HQ, detect sigils
- [ ] Extend existing city guard shop/service functions to check
  `society_exposed` status
- [ ] Register `spec_society_guard` in `special.c`

---

## Phase 6: Inter-Society PvP and Bounties

### PvP Windows

- [ ] Add `society_pvp_target` and `society_pvp_timer` transient fields to
  `CHAR_DATA`
- [ ] Implement PvP window creation for mission collision, exposure attack,
  and mutual challenge
- [ ] Extend `is_safe()` in `fight.c` to allow attacks during active PvP
  windows
- [ ] Implement `society_pvp_update()` in `update.c`: decrement timer, clear
  fields on expiry
- [ ] Implement `do_society_challenge()`: mutual-consent duel setup
- [ ] Write unit test: PvP window correctly enables/disables combat

### Mission Collision

- [ ] Implement collision detection: when two rival players enter the same
  mission target area, notify both and open PvP window
- [ ] First to complete objective wins; second auto-fails
- [ ] Write unit test: collision detection triggers correctly

### Bounty System

- [ ] Implement `struct society_bounty_type` and global linked list
- [ ] Implement `do_society_bounty()`: place, cancel, list
- [ ] Implement bounty claim on PvP kill
- [ ] Implement bounty expiry on exposure timer clear
- [ ] Write unit test: bounty placement validates requirements
- [ ] Write unit test: bounty claim transfers gold correctly

---

## Phase 7: AI and Lore Integration

### AI Dialogue

- [ ] Extend `npc_dialogue_dispatch()` to check NPC `society` field against
  player's society
- [ ] Implement `append_society_prompt()` and `append_mundane_prompt()`
- [ ] Add society lore flags (`LORE_FLAG_RECKONING` through
  `LORE_FLAG_PALE_LEDGER`) to lore flag definitions

### Lore Content

- [ ] Write lore entries for each society (6 entries minimum)
- [ ] Add lore entries to `lore/` directory with appropriate `lore_flags`
- [ ] Write society-specific `AiPrompt` text for handler, recruiter, guard,
  merchant, and contact NPC archetypes

### Area File Parsing

- [ ] Extend mob loader in `db.c` to parse `Society` and `SocNpcRole` fields
- [ ] Write unit test: mob loading correctly sets society fields

---

## Phase 8: Area Content (Separate from Code)

### Society HQ Areas

- [ ] Design and build Reckoning HQ area (beneath Lantern Registry)
- [ ] Design and build Jade Eels HQ area (Kowloon waterfront warehouse)
- [ ] Design and build Ash Cult HQ areas (Kowloon undercity + Eccentric
  Woodland)
- [ ] Design and build Quiet Hand HQ area (beneath Bloom Pavilion)
- [ ] Design and build Jackal Synod HQ area (beneath Mafdet funerary shop)
- [ ] Design and build Pale Ledger HQ area (Cinderteeth foothills)

### Society NPCs

- [ ] Create mob definitions for each society's recruiter, handler, merchant,
  guard, and contact NPCs
- [ ] Place recruiter NPCs in designated rooms
- [ ] Configure `spec_fun` assignments for guard NPCs

### Mission Content

- [ ] Write detailed mission descriptions for all 30+ mission templates
- [ ] Create mission-specific objects (documents, relics, packages)
- [ ] Place mission target objects/NPCs in appropriate areas

---

## Build System Changes (`src/Makefile`)

- [ ] Add `act_society.o` to `O_FILES`
- [ ] Add `const_society.o` to `O_FILES`
- [ ] Add `const_society_missions.o` to `O_FILES`
- [ ] Add unit test targets for society-related tests
- [ ] Add unit test binaries to `unit-tests` dependency list

---

## File Summary

### New Files

| File | Purpose |
|---|---|
| `src/headers/society.h` | Society constants, struct definitions, function prototypes |
| `src/act_society.c` | Society command handler and membership functions |
| `src/const_society.c` | Society table data |
| `src/const_society_missions.c` | Mission template table |
| `src/tests/test_society.c` | Unit tests for core society functions |
| `src/tests/test_society_save.c` | Unit tests for save/load round-trip |
| `src/tests/test_society_skills.c` | Unit tests for skill gating and behavior |
| `src/tests/test_society_missions.c` | Unit tests for mission flow |

### Modified Files

| File | Changes |
|---|---|
| `src/headers/typedefs.h` | Society constants |
| `src/headers/config.h` | Society flags, item flags, channel flag |
| `src/headers/ack.h` | PC_DATA fields, MOB_INDEX_DATA fields, CHAR_DATA transient fields |
| `src/headers/globals.h` | Society table extern, gsn declarations, bounty list |
| `src/headers/quest.h` | `QUEST_TYPE_SOCIETY` constant |
| `src/save/save.h` | `SAVE_REVISION` increment to 16 |
| `src/save/save_players.c` | Society save/load blocks |
| `src/interp.c` | Command registrations |
| `src/db.c` | Mob loader extension for Society/SocNpcRole fields |
| `src/act_comm.c` | Passphrase detection in `do_say()` |
| `src/update.c` | Society tick functions in `update_handler()` |
| `src/fight.c` | PvP window check in `is_safe()` |
| `src/special.c` | `spec_society_guard` registration |
| `src/Makefile` | New object files and test targets |

---

## Testing Strategy

### Unit Tests

Each test file follows the project's existing pattern: standalone binary with
`main()`, `assert()` checks, test doubles for external dependencies.

| Test File | Tests |
|---|---|
| `test_society.c` | `can_join_society()`, rank requirements, reputation bounds, exposure timer logic |
| `test_society_save.c` | Save/load round-trip for all society fields; backward compatibility with revision 15 |
| `test_society_skills.c` | `can_use_society_skill()` gating for each rank; disavowed member lockout |
| `test_society_missions.c` | Mission acceptance validation; reward calculation; timer auto-failure |

### Integration Test

The existing `integration-test.sh` validates server boot and basic login. No
society-specific integration test is needed for Phase 1 — the server should
boot cleanly with the new data structures even when no societies are
configured. Later phases should verify that a player can join a society, accept
a mission, and complete it through the full login → play → save → reload cycle.

### Manual Testing

Before merging each phase:
1. Boot server with `make unit-tests` passing
2. Create a test character
3. Verify society commands respond appropriately for non-members
4. (Phase 2+) Complete recruitment flow end-to-end
5. (Phase 3+) Practice and use society skills
6. (Phase 4+) Accept, complete, and fail missions
7. (Phase 5+) Trigger and recover from exposure
8. (Phase 6+) Test PvP window mechanics between two test characters

---

## Implementation Order

The phases are ordered by dependency:

1. **Phase 1** (data structures) is prerequisite for everything
2. **Phase 2** (commands, membership) requires Phase 1
3. **Phase 3** (skills) requires Phase 2
4. **Phase 4** (missions) requires Phase 2 and Phase 3
5. **Phase 5** (exposure) requires Phase 2
6. **Phase 6** (PvP, bounties) requires Phase 4 and Phase 5
7. **Phase 7** (AI, lore) can begin as early as Phase 2
8. **Phase 8** (area content) can begin as early as Phase 1

Phases 7 and 8 are content-heavy and can be developed in parallel with the
code phases by different contributors.
