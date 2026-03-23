# Public Societies: Implementation Checklist

## Relationship to Secret Society Implementation

The public society system shares architectural patterns with the secret society
system. Many components are parallel: table definitions, PC_DATA fields, save
format, skill registration, quest integration, and update-loop functions. The
two systems can be implemented in the same phases or independently.

Where the secret society system has 8 implementation phases, the public society
system is simpler (no exposure, no PvP, no bounties, no espionage) and fits
into 5 phases.

---

## Phase 1: Core Data Structures and Save System

### Constants and Types (`src/headers/`)

- [ ] Add public society constants to `typedefs.h`: `MAX_PUB_SOCIETY`,
  `MAX_PUB_SOCIETY_RANK`, `MAX_PUB_SOCIETY_REP`, `PUB_SOCIETY_NONE` through
  `PUB_SOCIETY_ROAD_WARDENS`
- [ ] Add `QUEST_TYPE_PUB_SOCIETY` quest type constant to `quest.h`
- [ ] Add `CHANNEL_PUB_SOCIETY` channel flag to `config.h`
- [ ] Add public society NPC role constants (`PUB_SOC_NPC_NONE` through
  `PUB_SOC_NPC_COMMANDER`) to `society.h`

### Struct Modifications (`src/headers/ack.h`)

- [ ] Add to `PC_DATA`: `pub_society`, `pub_society_rank` (int, -500 to 500),
  `pub_society_tasks`, `pub_society_joined`
- [ ] Add to `MOB_INDEX_DATA`: `pub_society`, `pub_society_npc_role`

### Public Society Table (`src/const_pub_society.c` -- new file)

- [ ] Define `struct pub_society_type` with fields: `name`, `abbr`,
  `base_city`, `hall_room`, `min_level`, `skill_first`, `skill_count`
- [ ] Populate `pub_society_table[MAX_PUB_SOCIETY]` with data for all 6
  societies

### Save System (`src/save/`)

- [ ] Add `fwrite` block for public society fields in `fwrite_char()` (only
  written when `pub_society > 0`)
- [ ] Add `fread` cases for `PubSoc*` keys in `fread_char()` with defaults
  for earlier revisions
- [ ] Write unit test: save and load a player with public society data, verify
  all fields round-trip correctly

### Global Declarations (`src/headers/globals.h`)

- [ ] Declare `extern const struct pub_society_type pub_society_table[]`
- [ ] Declare `gsn_*` variables for all public society skills

---

## Phase 2: Core Commands and Membership

### Enlist Command (`src/act_pub_society.c` -- new file)

- [ ] Implement `do_enlist()`: check prerequisites, enroll player, set fields
- [ ] Register `enlist` command in `interp.c`
- [ ] Implement `can_join_pub_society()` prerequisite check
- [ ] Write unit test: `can_join_pub_society()` returns FALSE for NPCs,
  already-members, below level requirements, within cooldown

### Society Command Extensions

- [ ] Extend `do_society()` to detect public society membership and dispatch
  to public society subcommands when appropriate
- [ ] Implement public society subcommands: `info`, `who`, `say`, `tasks`,
  `promote`, `resign`, `history`
- [ ] Implement public society channel via `CHANNEL_PUB_SOCIETY`

### Rank and Skill Gating

- [ ] Implement `pub_society_skills_available()`: return number of skills
  unlocked at a given rank score
- [ ] Implement `can_use_pub_society_skill()`: check membership and rank
  threshold for the specific skill
- [ ] Implement rank title lookup from rank score
- [ ] Write unit test: skills unlock/lock correctly at each threshold
- [ ] Write unit test: rank title updates dynamically with score changes

### Departure and Skill Loss

- [ ] Implement `do_pub_society_resign()`: confirmation prompt, field clearing,
  7-day cooldown, all skills immediately unusable
- [ ] Implement automatic discharge when rank drops below -300
- [ ] Write unit test: leaving zeroes membership and disables all skills
- [ ] Write unit test: skill proficiency is retained in `learned[]` but
  `can_use_pub_society_skill()` returns FALSE when not a member

### Visibility

- [ ] Modify `who` output to show public society abbreviation and rank title
- [ ] Modify `score` output to show public society section
- [ ] Modify `finger`/`whois` output to show public society membership

---

## Phase 3: Public Society Skills

### Skill Registration

- [ ] Reserve skill indices 950-999 in `skill_table[]` for public society
  skills
- [ ] Define all 36 public society skills (6 per society) in
  `src/skills/skill_table_data.c`
- [ ] Add `gsn_*` declarations for all public society skills in `globals.h`
- [ ] Verify `can_use_pub_society_skill()` gates on both membership and rank
  score threshold (implemented in Phase 2)

### Per-Society Skills

- [ ] Guard Command: `patrol_sense`, `field_mend`, `rally_cry`, `shield_wall`
  (passive: post-block damage reduction), `tactical_assess`,
  `command_presence` (active combo builder: strike + group hitroll buff,
  feeds `ch->combo[]`)
- [ ] Harbor Syndics: `appraise`, `dockside_audit`, `haggle`, `longshoreman`
  (passive: bonus autoattack), `bonded_cargo`, `anchor_slam` (active
  combo builder: heavy hit + speed debuff, feeds `ch->combo[]`)
- [ ] Ember Wardens: `forage`, `field_repair`, `ember_signal`,
  `ember_vitality` (passive: in-combat HP regen), `containment_ward`,
  `controlled_burn` (active chi-enhanced: fire hit + fire DOT, optional
  chi spend doubles DOT duration)
- [ ] Shrine of the First Claw: `oath_mark`, `track_quarry`, `writ_of_hold`,
  `marked_strike` (passive: evasion piercing vs marked targets),
  `claws_pursuit` (pursuit buff vs marked target), `claws_judgment`
  (active combo finisher: 2x damage vs oath_marked, requires
  `is_valid_finisher()` + mark)
- [ ] Wall Command: `dig_in`, `shelter`, `trailblaze`, `bulwark`
  (passive: flat physical damage reduction), `signal_tower`,
  `rampart_slam` (active combo builder: shield slam + stun, feeds
  `ch->combo[]`)
- [ ] Road Wardens: `wayfind`, `road_camp`, `forced_march`, `road_grit`
  (passive: dodge bonus), `dispatch_rider`, `waylay` (active positional:
  hit + dodge/speed debuff, requires target fighting someone else)
- [ ] Register all skill commands in `interp.c`
- [ ] Write unit tests for each skill's gating logic

### Practice Integration

- [ ] Extend `do_practice()` to recognize public society trainer NPCs
- [ ] Trainer checks `can_use_pub_society_skill()` before teaching

---

## Phase 4: Task System

### Task Templates

- [ ] Define `struct pub_society_task_type` in `society.h`
- [ ] Create `src/const_pub_society_tasks.c` (new file) with the task
  template table
- [ ] Write 5 task templates per society (30 total) covering difficulties 1-5

### Task Flow

- [ ] Implement `do_pub_society_tasks()`: filter and display available tasks
- [ ] Implement `do_pub_society_accept()`: create QUEST_DATA entry
- [ ] Implement task completion detection (reuses MISSION_OBJ_* handlers)
- [ ] Implement `pub_society_task_complete()`: reputation, gold, task count
- [ ] Implement `pub_society_task_fail()`: reputation penalty (no exposure)
- [ ] Implement `do_pub_society_abandon()`: half penalty, cooldown
- [ ] Write unit tests for task acceptance, completion, and failure

### Task Timer

- [ ] Implement `pub_society_task_update()` in `update.c` tick loop

---

## Phase 5: AI, Lore, and Area Integration

### AI Dialogue

- [ ] Extend `npc_dialogue_dispatch()` to check NPC `pub_society` field
- [ ] Public society NPCs address members by rank title
- [ ] Add public society lore flags to lore flag definitions

### Area File Parsing

- [ ] Extend mob loader in `db.c` to parse `PubSociety` and `PubSocNpcRole`
  fields
- [ ] Write unit test: mob loading correctly sets public society fields

### Area Content (Separate from Code)

- [ ] Design and build Guard Command Hall in Midgaard
- [ ] Design and build Harbor Syndics Hall in Kowloon
- [ ] Design and build Ember Warden Station in Rakuen
- [ ] Design and build Shrine of the First Claw in Mafdet
- [ ] Design and build Wall Command Garrison in Kiess
- [ ] Design and build Road Warden Lodge at Midgaard crossroads
- [ ] Design and build Road Warden waypoint posts along major routes
- [ ] Create NPC definitions for each society's enrollment, task, merchant,
  trainer, and commander NPCs

---

## Build System Changes (`src/Makefile`)

- [ ] Add `act_pub_society.o` to `O_FILES`
- [ ] Add `const_pub_society.o` to `O_FILES`
- [ ] Add `const_pub_society_tasks.o` to `O_FILES`
- [ ] Add unit test targets for public society tests
- [ ] Add unit test binaries to `unit-tests` dependency list

---

## File Summary

### New Files

| File | Purpose |
|---|---|
| `src/const_pub_society.c` | Public society table data |
| `src/const_pub_society_tasks.c` | Task template table |
| `src/act_pub_society.c` | Public society commands and membership functions |
| `src/tests/test_pub_society.c` | Unit tests for core functions |
| `src/tests/test_pub_society_save.c` | Unit tests for save/load round-trip |

### Modified Files

| File | Changes |
|---|---|
| `src/headers/typedefs.h` | Public society constants |
| `src/headers/config.h` | Channel flag |
| `src/headers/ack.h` | PC_DATA fields, MOB_INDEX_DATA fields |
| `src/headers/globals.h` | Pub society table extern, gsn declarations |
| `src/headers/quest.h` | `QUEST_TYPE_PUB_SOCIETY` constant |
| `src/headers/society.h` | Public society NPC role constants, task struct |
| `src/save/save_players.c` | Public society save/load blocks |
| `src/interp.c` | Command registrations (`enlist`, skill commands) |
| `src/db.c` | Mob loader extension for PubSociety/PubSocNpcRole |
| `src/update.c` | Task timer and reputation decay in `update_handler()` |
| `src/act_info.c` | `who`, `score`, `finger` output modifications |
| `src/Makefile` | New object files and test targets |

---

## Testing Strategy

### Unit Tests

| Test File | Tests |
|---|---|
| `test_pub_society.c` | `can_join_pub_society()`, `pub_society_skills_available()`, rank threshold gating, skill loss on departure |
| `test_pub_society_save.c` | Save/load round-trip; backward compatibility |

### Integration Test

The existing `integration-test.sh` validates server boot. The server should
boot cleanly with public society data structures. No public-society-specific
integration test is needed for Phase 1.

### Manual Testing

Before merging each phase:
1. Boot server with `make unit-tests` passing
2. Create a test character
3. Verify `enlist` and `society` commands respond appropriately for non-members
4. (Phase 2+) Complete enrollment flow, verify visibility in `who`/`score`
5. (Phase 3+) Practice and use public society skills
6. (Phase 4+) Accept, complete, and fail tasks

---

## Implementation Order

1. **Phase 1** (data structures) is prerequisite for everything
2. **Phase 2** (commands, membership, visibility) requires Phase 1
3. **Phase 3** (skills) requires Phase 2
4. **Phase 4** (tasks) requires Phase 2 and Phase 3
5. **Phase 5** (AI, lore, areas) can begin as early as Phase 2

Phases can be developed in parallel with the secret society implementation
phases. The two systems share `society.h`, the save revision, and the
`update_handler()` integration but are otherwise independent codepaths.
