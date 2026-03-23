# Proposal: Resolving the Three Major Unresolved Threads in Class Lore

**Status:** For discussion
**Scope:** `docs/lore/` only — no `src/` changes required
**Related work:** Implements the remaining suggestions from the class lore enhancement session (March 2026). Suggestions 4–9 were implemented directly; this proposal covers suggestions 1–3, which require more careful handling because they resolve questions that existing lore files have deliberately left open.

---

## Background

During a review of class lore files, three threads were identified as explicitly flagged "open in the scholarly sense, closed in the practical sense" — meaning the existing documents acknowledge that the question exists but decline to answer it. Unlike suggestions 4–9 (which added new content to areas that were simply underdeveloped), these three suggestions require taking a position on something the current lore treats as intentionally unresolved. That is a higher-stakes authorial choice, which is why they are gathered here for explicit sign-off.

---

## Thread 1: The Khephari/Monk Comparison That Was Never Made

### Current State

`monk_lore.md`, Section VI ("The Khephari Thread") explicitly states:

> "A Khephari walking a caravan route will stop to press their feet against the ground and listen... The Khephari maintained their own documentation of their earth-sense tradition; that documentation exists in Kiess's archives and has never been systematically compared with the earliest Interior Forms documents."

The section explains why the comparison hasn't been made — the institutional impetus to credit a non-human people's contribution isn't felt as compelling — and then leaves it there. `khephari_lore.md` doesn't reference the Interior Forms at all.

### Proposed Change

Add a section to `monk_lore.md` and a reciprocal note to `khephari_lore.md` describing a specific practitioner — a Monk scholar based in Kiess, with archive access under the anti-sealed-warrant framework — who actually made the comparison and published a working paper. The key findings:

- Specific postures in the Outer Forms' footwork sequences match documented Khephari vibration-sensing stances with a precision that cannot be attributed to convergent development in different environments.
- The Khephari earth-sense tradition's documentation describes a progression from "listening through contact" to "listening through anticipation" that maps directly onto the Outer/Inner Forms progression, using entirely different vocabulary to describe identical phenomenology.
- Several Outer Forms techniques — particularly the footwork patterns for uneven terrain that appear early in monastery curricula — were almost certainly derived from Khephari guides who worked with the mountain communities before Keeper Era documentation began.

The institutional response is where the interesting lore lives: the Monk tradition's main academies neither formally accept nor formally reject the paper. They file it in the category of "historically notable but methodologically disputed" — which is not a finding of error (the methodology is impeccable by Kiess archive standards) but an institutional deferral. Khephari community representatives who were informed of the paper's existence and conclusions issued a brief written acknowledgment that read more or less as: "Yes. We know."

### Why This Is a Harder Call Than Suggestions 4–9

The existing lore in `monk_lore.md` carefully preserves the question as open because it trusts readers to draw their own conclusions. Explicitly answering it changes the relationship between the player/reader and the world — from "this is a world with genuine historical uncertainty" to "this is a world where specific things are true and the institutions are slow to acknowledge them." That's a valid creative choice but it changes the texture of the lore, and should be made deliberately rather than by default.

**Suggested approach:** Answer it, but through the specific practitioner's paper rather than as omniscient narrative voice. This preserves some ambiguity (the paper is disputed, the academies haven't accepted it, a full comparative study hasn't been completed) while making clear that the evidence points in a specific direction. The world has genuine historical truth; the institutions are simply slow to acknowledge it.

---

## Thread 2: The Ashbound as Apostate Wardens

### Current State

`warden_lore.md`, Section X ("The Ashbound Question") describes the Kiess-based historical argument that the Ashbound besiegers at the Seven Shade Arches were former road wardens from the same charter system — which would make the Forty-Three Days a civil war within the tradition. It ends:

> "The question remains open in the historiographical sense and closed in the practical sense: no warden training hall teaches the Ashbound as a mirror lineage, and no warden tradition formally claims descent from the siege-side of the Seven Shade Arches engagement."

### Proposed Change

Give the question a concrete current dimension rather than letting it sit as pure historical debate. Specifically:

A **Reckoning-aligned archivist** at Midgaard's Granite Arcade (name: Eryn Vael) submits a formal findings report claiming that Keeper-Era convoy ledgers show Dunmar's company and the primary Ashbound faction's leadership both listed under the **Westridge Road Warden Association** — the same regional charter body, operating within three years of each other. The report does not claim to be definitive; it claims that the Ashbound's organizational coherence and specific interest in the convoy (which is unusual for post-collapse bandits) is most easily explained by them being practitioners of the same doctrine who had lost their charters and decided to take the convoy for themselves rather than starve.

Registry School response: files the report under "contested historical claims requiring further verification" — a classification that in practice means it will not be acted on.
Dunmar School response: brief public statement that the Forty-Three Days' meaning does not depend on the attackers' institutional background, and that Dunmar's conduct speaks for itself.
Reckoning faction's actual use of the report: less about the specific historical question and more about establishing a precedent that Midgaard's registry office cannot classify historical inconveniences as "requiring further verification" indefinitely. The report is a lever.

The report should not be "officially true" in the lore — it should remain contested, with good evidence on Vael's side and motivated reasoning on the Registry School's side. What changes is that the question is no longer only historical: it has a current institutional actor pressing it, which makes it relevant to any warden NPC who has opinions about the Continuity/Reckoning split.

### Why This Is a Harder Call

The Ashbound question is interesting precisely because both schools can plausibly claim Dunmar's example for their own position. Introducing an archivist who has evidence on the Kiess side risks tipping the scales in a way that makes one school's position objectively more defensible. The proposal above tries to avoid this by keeping the report formally contested — but any author developing this thread will need to resist the temptation to make the evidence too clear. The lore is more interesting if a careful reader genuinely can't tell whether Vael is right.

**Suggested approach:** Write the report as a section in `warden_lore.md`, not as a standalone document (which would imply more weight than appropriate), and make clear that Vael has a political agenda even if her methodology is sound. The most honest version of this lore is: the evidence suggests it, the institutions deny it, the reader has to decide how much that pattern of denial means.

---

## Thread 3: The Warlock's Implication That Is "Both Plausible and Incomplete"

### Current State

`warlock_lore.md`, Section I ("The Shadow Compacts") identifies the possibility that compact counterparties are fragments of the Black Sun Shard's anti-light essence operating outside the Great Pyramid's containment. It notes:

> "The tradition itself most consistently avoids [this implication], because it implies implications that practitioners prefer not to examine closely."

The avoidance is framed as habitual and institutional — but there's no specific institution keeping the question suppressed, and no current event that would force it back into the open.

### Proposed Change

Introduce a specific suppression mechanism: a **sealed investigation** by a Cleric sub-tradition (the former Inner Office's containment doctrine lineage, now operating within Midgaard's temple structures) that formally investigated the compact-counterparty question and reached a conclusion that was then classified. The investigation's existence is documented in the Kiess archives; its conclusions are not.

What Kiess-archive practitioners can determine from the access record: the investigation ran for approximately two years and involved senior practitioners from both the cleric tradition and the Wizard's Keep (the latter consulted on the question of whether the Black Sun Shard's anti-light essence could produce what the Shard's documentation calls "secondary resonance phenomena" outside the Pyramid's containment zone). The investigation's final report was classified at a level that the Kiess anti-sealed-warrant charter cannot override — meaning it was classified under a civil rather than a religious authority, which is itself unusual.

The implication this creates: someone with civil authority decided this investigation's conclusions were politically dangerous, which is different from the cleric tradition deciding they were theologically inconvenient. The question of who classified it, and why the civil authority cared, is left open for player investigation.

This connects Warlocks to the active Void Citadel monitoring operation described in the Cipher lore enhancement (suggestion 6), since both involve the same question of what is operating in the space that the Pyramid's containment was designed to prevent.

### Why This Is the Hardest Call

The Warlock's foundational uncertainty — about what compact counterparties actually are — is one of that class's most interesting lore features. Resolving it, even partially, risks making the tradition feel smaller. The proposal above tries to avoid this by making the suppression *more* interesting than the question it suppresses: the sealed investigation means someone knows the answer, but practitioners don't have access to that knowledge. This should make Warlocks feel more dangerous and more watched rather than more understood.

The risk is that by introducing a specific investigation with a specific suppressed conclusion, the lore implicitly answers the question in one direction (the Shard connection is real enough that civil authority classified the investigation's findings). Whether that's the right call depends on whether the world is more interesting with or without that answer potentially being true.

**Suggested approach:** Write this as an addition to `warlock_lore.md` and a cross-reference in `cipher_lore.md`. Keep the investigation's conclusions completely opaque — the Kiess access record shows the investigation happened, not what it found. Make the civil-authority classification the mystery rather than the answer.

---

## Implementation Notes

All three of these threads require:
1. A decision about how much to answer versus how much to preserve as open question
2. New sections in one or more class lore files
3. Corresponding cross-references in related files (Khephari for Thread 1; Cipher/Continuity-Reckoning for Threads 2 and 3)

None require `src/` changes. The changes are pure `docs/lore/` additions. Per CLAUDE.md, they can be implemented directly once the user has explicitly signed off on the approach.

The most important authorial decision in each thread is: how much certainty to give the reader? The existing lore earns its texture partly by preserving genuine uncertainty. These proposals should add current-stakes drama without flattening the uncertainty that makes the world feel real.
