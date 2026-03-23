# Proposal: Secret Societies System

## Overview

Secret societies are hidden player-joinable organizations that exist beneath the
public clan system. Where clans are open political entities with public
membership rosters, war declarations, and visible diplomacy, secret societies
operate in shadow — their members are unknown to outsiders, their goals are
ambiguous, and their activities are deniable.

A player may belong to one clan, one public society, **and** one secret society
simultaneously. The three systems are independent but interact at specific
points: society missions may target rival clans, society intelligence networks
may reveal clan diplomacy states, and society rank may unlock dialogue options
with AI-enabled NPCs who recognize the player's hidden allegiance.

See `docs/proposals/public-societies/` for the public societies system —
open, well-known civic institutions (Guard Command, Harbor Syndics, Ember
Wardens, Shrine of the First Claw, Wall Command, Road Wardens) that operate
as the visible counterpart to the covert organizations described here.

---

## Design Goals

### 1. Hidden Identity

A player's society membership is never revealed through normal game commands.
`who`, `whois`, `score`, `finger`, and `clan` show no society information.
Society members recognize each other only through in-game signals — a specific
emote, a keyword spoken in the right room, or an item carried. The `society`
command is only available to members and shows only their own society's internal
state.

### 2. Lore Integration

Every society is rooted in the existing world lore. The six societies defined in
this proposal grow directly from factions already documented in the city lore
files: the Reckoning movement in Midgaard, the Jade Eels in Kowloon, the Ash
Cult across multiple cities, the Quiet Separatists in Rakuen, the Jackal Synod
in Mafdet, and the remnants of the Spirebound Conclave beneath the Cinderteeth
Mountains. These are not new inventions — they are the playable versions of
organizations the world already contains.

### 3. Asymmetric Gameplay

Each society offers a distinct gameplay loop. The Reckoning faction rewards
intelligence gathering and archive access. The Jade Eels reward smuggling and
trade manipulation. The Ash Cult rewards sabotage and ritual activities. The
Quiet Separatists reward diplomacy and supply-chain building. The Jackal Synod
rewards artifact recovery and relic trafficking. The Conclave Remnant rewards
arcane experimentation and forbidden knowledge. No two societies play the same
way.

### 4. Risk and Consequence

Society activities carry real risk. Failed missions can expose the player's
affiliation to NPCs or other players. Exposure triggers a reputation penalty
that makes the player a target for city guards, rival society members, and
bounty contracts. The society may disavow exposed members to protect itself.
Players who want safe progression stay in clans; players who want high-risk
high-reward covert gameplay join societies.

### 5. Horizontal Progression

Society rank does not grant raw combat power. It grants access: access to
hidden rooms, restricted NPC dialogue, covert communication channels, unique
non-combat skills (disguise, dead drops, coded messages, forged documents), and
society-specific mission chains. A rank-5 society member is not stronger in a
fight than a rank-1 member — they have more tools and more dangerous missions
available.

### 6. Player-Driven Conflict

Societies have built-in rivalries based on lore. The Reckoning and the Conclave
Remnant are natural enemies (exposure vs. suppression). The Jade Eels and the
Jackal Synod compete for smuggling routes. The Ash Cult and the Quiet
Separatists have opposing visions for Rakuen's future. These rivalries create
PvP and espionage opportunities without requiring explicit war declarations —
the conflict is structural and ongoing.

### 7. Minimal Combat Stat Impact

Society membership does not modify `hitroll`, `damroll`, `armor`, `max_hit`,
`max_mana`, or saving throws. Society skills are utility and roleplay tools,
not combat multipliers. This prevents societies from becoming mandatory for
competitive play and keeps the combat balance rooted in class, level, and
equipment.

---

## The Six Societies

| Society | Base City | Lore Origin | Gameplay Loop |
|---|---|---|---|
| **The Reckoning** | Midgaard | Reckoning faction (exposure of sealed records) | Intelligence, archives, political exposure |
| **The Jade Eels** | Kowloon | Jade Eels smuggling syndicate | Smuggling, trade manipulation, port control |
| **The Ash Cult** | Kowloon / Rakuen | Ash Cult (Cinderteeth worship) | Sabotage, rituals, infrastructure disruption |
| **The Quiet Hand** | Rakuen | Quiet Separatist movement | Diplomacy, supply chains, independence building |
| **The Jackal Synod** | Mafdet | Jackal Synod (relic trafficking) | Artifact recovery, tomb raiding, black market |
| **The Pale Ledger** | Kiess / Cinderteeth | Spirebound Conclave remnants | Forbidden arcana, experimentation, sealed knowledge |

Each society is detailed in `03-society-definitions.md`.

---

## Relationship to Existing Systems

| System | Interaction |
|---|---|
| **Public Societies** | Independent membership; a player can be in both. Public societies are visible civic institutions; secret societies are covert. See `docs/proposals/public-societies/`. |
| **Clans** | Independent membership; society missions may reference clan targets |
| **AI Dialogue** | Society NPCs use `ACT_AI_DIALOGUE`; members get additional dialogue paths |
| **Lore System** | Society lore entries injected for society-aware NPCs via `lore_flags` |
| **Quest System** | Society missions use the existing `QUEST_DATA` infrastructure |
| **Skills** | Society skills are non-combat utility abilities stored in `learned[]` |
| **Save System** | Society data persisted in player file; new save revision required |
| **Politics** | Society actions can shift clan diplomacy values indirectly |

---

## Scope

This proposal covers:

- Data structures for society membership, rank, and reputation
- Definitions of six societies with lore grounding
- Membership system (recruitment, initiation, advancement, expulsion)
- Society-specific non-combat skills
- Mission/contract system for society activities
- Inter-society rivalry and espionage mechanics
- Integration points with clans, AI NPCs, lore, saves, and quests
- Implementation checklist with file-level changes

This proposal does **not** cover:

- Area file definitions for society rooms (separate area-building task)
- AI dialogue persona text for society NPCs (separate AI content task)
- LoRA training data for society NPC speech patterns (separate ML task)
- Specific mission content (separate content-authoring task)
