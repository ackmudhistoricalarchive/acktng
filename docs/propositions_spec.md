# Propositions Specification

This document specifies the static proposition file format currently used by ACK and catalogs the propositions that are currently defined in `propositions/*.prop`.

## Scope and loading behavior

- Static proposition templates are loaded from `../propositions/<n>.prop` at startup, where `<n>` runs from `1` through `PROP_MAX_STATIC_PROPOSITIONS` (`16`). Missing files are ignored.  
- Each loaded file is assigned a zero-based static proposition ID (`file number - 1`).  
- The parser ignores blank lines, comment lines beginning with `#`, and leading/trailing whitespace around each significant line.

## File format (`.prop`)

Each proposition file has this structure:

1. **Title line**  
   Freeform text used as the proposition title.

2. **Numeric definition line** (10 or 11 integers):

   ```text
   prerequisite_static_id type num_targets kill_needed min_level [max_level] offerer_vnum reward_gold reward_qp reward_item_vnum reward_item_count
   ```

   - If `max_level` is omitted (10-integer legacy form), runtime default is `170`.

3. **Target vnum line**  
   Space-delimited integer vnums. The parser reads up to `PROP_MAX_TARGETS` (`5`) values.

4. **Accept message line** *(optional)*  
   Message shown on acceptance.

5. **Completion message line** *(optional)*  
   Message shown on completion hand-in.

## Field definitions

- `prerequisite_static_id`: static proposition ID that must already be completed; `-1` means no prerequisite.
- `type`: proposition objective type.
  - `1` = kill **any one** listed target vnum.
  - `2` = acquire a target object vnum.
  - `3` = kill-count objective against a single target vnum (`kill_needed` required).
  - `4` = cartography objective: explore every room in one target area (`target_vnum[0] = area_num`, static-only).
- `num_targets`: number of valid entries expected from the target vnum line.
- `kill_needed`: required kill count (used by type `3`; `0` for non-count objectives).
- `min_level`, `max_level`: pseudo-level gating for acceptance.
- `offerer_vnum`: NPC vnum of the postmaster/postman who offers the static proposition.
- `reward_gold`, `reward_qp`: fixed completion rewards.
- `reward_item_vnum`, `reward_item_count`: optional item reward (use `0 0` for none).

## Current static propositions

| File | Static ID | Title | Prereq ID | Type | Targets | Kill Needed | Level Range | Offerer | Rewards |
|---|---:|---|---:|---:|---|---:|---|---:|---|
| `1.prop` | 0 | Route reconnaissance: Forest of Confusion | -1 | 1 | `9601 9602 9603` | 0 | 20-39 | 13001 | 1200 gold, 2 qp |
| `2.prop` | 1 | Conclave residue containment | 0 | 3 | `1011` | 8 | 40-54 | 13001 | 2200 gold, 3 qp |
| `3.prop` | 2 | Eastern trade road interdiction | 1 | 1 | `29904 29909 29932` | 0 | 55-69 | 3015 | 3600 gold, 4 qp |
| `4.prop` | 3 | Sealed-warrant courier disruption | 2 | 1 | `8500 10159 11485` | 0 | 70-84 | 3015 | 5000 gold, 5 qp |
| `5.prop` | 4 | Blightfront quarantine sweep | 3 | 1 | `10012 1030 1050` | 0 | 85-170 | 3015 | 7000 gold, 6 qp |

## Progression notes from current data

- The current chain is strictly linear by prerequisite (`-1 -> 0 -> 1 -> 2 -> 3`).
- Propositions `1` and `2` are offered by vnum `13001`; propositions `3`-`5` are offered by vnum `3015`.
- All current propositions use fixed gold/qp rewards and no item rewards.
