# Design Proposal: Remortal Class Skills — Lore Accuracy Audit & Proposed Changes

**Status:** For discussion
**Branch:** `claude/audit-immortal-skills-bAyNd`
**Scope:** `src/` changes required — design sign-off needed before implementation
**Related lore:** `docs/lore/grand_magi_lore.md`, `templar_lore.md`, `nightblade_lore.md`, `crusader_lore.md`, `kinetimancer_lore.md`, `martial_artist_lore.md` and component tradition lore files

---

## Section I: Overview & Scope

### What Was Audited

The six adept (remortal) classes and their exclusive skill sets, cross-referenced against their lore files and the lore for each component tradition. The current skill table entries are in `src/skills/skill_table_data.c`; implementations live in `src/skills/do_*.c`.

Current adept-exclusive skills, by class:

| Class | Skills | Levels |
|-------|--------|--------|
| Grand Magi (CLASS_GMA) | Elemental Inferno, Elemental Attunement, Elemental Shock, Elemental Deluge, Elemental Rupture | 1, 3, 5, 10, 15 |
| Templar (CLASS_TEM) | Enhanced Heal, Oathshield, Sanctified Strike | 1, 10, 15 |
| Nightblade (CLASS_NIG) | Poison:Nightshade, Shadow Reading, Hex Ward, Reflex Disruption | 2, 5, 10, 15 |
| Crusader (CLASS_CRU) | Anti Magic Shell, Momentum Chain, Iron Resolve, Overwhelming Assault | 2, 5, 10, 15 |
| Kinetimancer (CLASS_KIN) | Kinetic Reversion | 2 |
| Martial Artist (CLASS_MAR) | Chi Surge, Breath of Endurance, Fist of the Interior Form | 5, 10, 15 |

### Audit Methodology

Each class was assessed against three questions:

1. **Contradiction:** Do any current skills imply mechanics, origins, or flavors that contradict the tradition's established lore?
2. **Named gaps:** Does the lore name specific techniques that have no corresponding skill?
3. **Theme gaps:** Are there lore themes central to the tradition's identity that have no representation in the skill set?

The audit found issues in four of the six classes: Grand Magi (theme gap), Templar (theme gap), Kinetimancer (critical — named gaps plus near-total underdevelopment), and Martial Artist (contradiction). Nightblade and Crusader are well-aligned with their lore.

---

## Section II: Grand Magi

### Lore Summary

The Grand Magi tradition synthesizes two parent remort paths: **Sorcerer** (Chromatic Circle — elemental breadth, synthesis methodology) and **Wizard** (Axiomatic Schools — formal proof, reproducibility constraints). The tradition's foundational documents, the **Convergence Codices**, describe what happens when both methodologies are applied to the same elemental domain simultaneously: the results are not additive but **convergent** — phenomena that neither methodology alone could produce or predict.

The tradition was reclaimed during the **Ashfall Monsoon** when practitioners using multi-domain elemental synthesis redirected volcanic storm fronts. The honorific "Grand Magi" was revived as the civic term for this specific achievement. The doctrine that emerged is organized around **four primary domains** (fire, air/storm, water/tide, earth/stone) and, critically, **layered elemental pressure**: maintaining multiple elemental engagements simultaneously, each reinforcing the effects of the others.

The distinguishing characteristic is not simply knowing all four elements — a well-rounded Sorcerer or Wizard might claim that. It is the specific convergence in which the Axiomatic practitioner's formal constraints make elemental synthesis analyzable and reproducible, while the Circle practitioner's multi-school awareness broadens the formal methodology beyond what single-school analysis addresses. Grand Magi practice produces results neither parent tradition can replicate independently.

### Current Skills

| Skill | Level | Type | Assessment |
|-------|-------|------|------------|
| Elemental Inferno | 1 | Offensive spell (fire) | Thematic |
| Elemental Attunement | 3 | Passive | Partially thematic — see below |
| Elemental Shock | 5 | Offensive spell (air/lightning) | Thematic |
| Elemental Deluge | 10 | Offensive spell (water) | Thematic |
| Elemental Rupture | 15 | Offensive spell (earth) | Thematic |

The four elemental attacks represent the tradition's four primary domains. This is correct and well-grounded. The issue is the **Elemental Attunement** passive and what it implies about the Grand Magi's defining mechanic.

### Issue: Sequential Rotation vs. Simultaneous Convergence

**Elemental Attunement** provides a bonus for rotating through the elemental domains sequentially — a reward for cycling fire → lightning → water → earth. Sequential elemental cycling is how a **Sorcerer** operates: breadth-and-synthesis, one domain to the next.

The Convergence Codices and the Ashfall Monsoon account are explicit: Grand Magi does not simply cycle through elements. They hold multiple elements simultaneously, each reinforcing the others. The Ashfall intervention wasn't practitioners casting fire then water then air sequentially; it was sustained multi-domain elemental coordination *at the same time*, producing thermal-electrical-atmospheric combination effects. From `grand_magi_lore.md`, Section V:

> *"A Grand Magi who is applying Elemental Inferno is not merely deploying fire; they are deploying fire in coordination with Elemental Shock to create a thermal-electrical combination that amplifies both effects beyond what either achieves alone."*

The current Attunement passive is not *wrong* — it reflects that all four domains must be mastered — but it implies the defining technique is sequential cycling, when the lore is clear that the apex technique is simultaneous deployment with cross-domain amplification. The highest expression of Grand Magi practice is not "cycling through elements faster"; it is "holding two or three elements at once and producing effects neither achieves alone."

### Proposed Changes

**1. Add: Convergent Cascade (Level 8)**

A new active spell placed between Elemental Shock (L5) and Elemental Deluge (L10). It deals damage of two elemental types simultaneously — its damage bonus proportional to how recently the caster used a spell from each of those two domains. This represents the lore-accurate "amplified combined effect" the Codices describe: the moment where sequential mastery (which Attunement rewards) tips over into genuine simultaneous layering.

Mechanically: the spell selects two of the four elemental damage types based on the caster's recent casting history, applies both simultaneously, and gets a bonus if both domains were recently used (the "convergence condition"). This keeps the Attunement → Convergent Cascade interaction intact: Attunement rewards learning to cycle all four; Cascade rewards deploying two at once.

**2. Modify: Elemental Attunement (behavioral clarification)**

The existing Attunement passive should be adjusted so the rotation bonus accumulates toward a "convergence threshold" — once all four domains have been used in sequence, a persistent synergy bonus activates for the remainder of combat rather than simply resetting the counter. This distinguishes the Grand Magi's use of sequential casting from a Sorcerer's: the sequence is preparation, not the technique itself.

**Implementation scope:**
- New entry in `skill_table_data.c`: `Convergent Cascade`, `L(CLASS_GMA, 8)`, offensive, mana cost ~140 (reflecting two-domain simultaneous cost)
- New `src/skills/do_convergent_cascade.c`
- Modification to Attunement's threshold logic (in `do_elemental_attunement.c` or wherever passive triggers are evaluated)

**Trade-off:** This gives Grand Magi six skills, making it the most skill-rich adept class. That is proportionate: Grand Magi is also the most mana-intensive adept class (20 mana/level, highest of any adept), and simultaneous convergence is explicitly identified in the lore as the tradition's apex technique. It should have a corresponding apex skill.

---

## Section III: Templar

### Lore Summary

The Templar tradition descends from the Solar Court's **temple-guard auxiliaries** — wardens who performed both combat and consecration functions simultaneously because the job structurally required it. After the Sundering Wars, Paladin and Priest traditions specialized apart. The synthesis was reconstructed during the **Violet Compact's garrison crisis**, when combined forces were too few to maintain separate advance-post and interior functions and began integrating them out of operational necessity.

The resulting tradition is defined by **dual function without compromise**: a Templar who has genuine healing competence can maintain offensive pressure while sustaining allies; a Templar who has genuine combat competence can conduct healing operations in active threat environments. The signature technique — **Enhanced Heal** — is the most technically demanding expression of this: integrating status-cure functions (cure blindness, cure poison, remove curse) into the healing channel as subsidiary processes rather than sequential applications, eliminating the treatment gap.

The Templar also inherits the auxiliary tradition's **oath-above-institution** logic: practitioners are oath-bound to sacred spaces and covenant principles, not to commanders or organizational archives. The Rakuen garrison's "bare oath" doctrine is the most developed form of this — the position that an oath binds because the practitioner made it, regardless of whether an institution has recorded it.

### Current Skills

| Skill | Level | Type | Assessment |
|-------|-------|------|------------|
| Enhanced Heal | 1 | Passive (heal integration) | Directly from lore — excellent |
| Oathshield | 10 | Active (ally defense) | Thematic, but partially |
| Sanctified Strike | 15 | Active (holy damage + heal) | Thematic |

### Issue: The Threshold Warden Role Is Absent

The current skill set covers two of the three lore themes well: Enhanced Heal (the dual-function integration) and Sanctified Strike (the advance-and-sanctify Paladin component). What is missing is the **threshold** identity — the Templar's specific role as the practitioner who stands between two functions at once, holding ground that sacred practice requires.

The Oathshield is named for the oath but functions primarily as a defensive buff for allies. This is useful and somewhat consistent with the "protecting the threshold" image, but it doesn't capture what the lore means by oath-binding: the idea that the Templar's commitment is to *a principle* — the maintenance of conditions under which sacred practice can occur — not simply to an ally's hit points. The Templar isn't just a defensive buffer; they're the person who decides what ground is worth holding.

There is also a level gap: Enhanced Heal is L1, then nothing until Oathshield at L10. The Templar has no skill between levels 1 and 10, which leaves early adept progression feeling sparse. The lore supports a mid-tier addition: the garrison crisis period produced multiple integrated techniques at the Paladin-Priest boundary, not only Enhanced Heal.

### Proposed Change

**Add: Consecrated Ground (Level 5)**

A ground-target effect — or, in MUD terms, a room-affecting duration skill — that represents the threshold warden's specific function: **sanctifying a position while holding it**. The Templar performs this as an integrated act: the consecration doesn't happen after the fight; it happens during it, in the sustained way that the Solar Court auxiliaries' dual function required.

Mechanically, Consecrated Ground applies a sanctification effect to the room (or immediate area) for a duration. While active, it provides a modest healing-over-time benefit to allies in the room and a minor penalty to undead or shadow-type enemies — representing both the "restorative ward" Priest aspect and the "sanctified threat" Paladin aspect operating simultaneously. It is not a powerful combat cooldown; it is a sustained environmental effect that expresses the threshold role.

This is explicitly lore-grounded: `templar_lore.md` Section I describes the auxiliary role as "defend against physical threats" *and* "consecrate a space after desecration" — the Templar's innovation is doing both at once. Consecrated Ground is that innovation made mechanical. It is the skill that makes the Templar's ground mean something different from any other class's ground.

**Implementation scope:**
- New entry in `skill_table_data.c`: `consecrated ground`, `L(CLASS_TEM, 5)`, `TAR_IGNORE`, `POS_FIGHTING` (castable in combat), mana cost ~60
- New `src/skills/do_consecrated_ground.c` applying a room flag or affect that ticks healing/penalty

**Trade-off:** This places four skills on the Templar, matching Nightblade and Crusader in depth. It fills the L1-to-L10 gap with something that expresses the tradition's foundational role rather than a generic ability, and it distinguishes Templar more clearly from Paladin (which can do healing-adjacent things but cannot consecrate ground as a sustained combat act).

---

## Section IV: Nightblade

### Lore Summary

The Nightblade tradition has no guild and no formal founding — an inheritance from the Assassin lineage that forms half its ancestry. It emerged in the **post-empire margins** from the practical combination of two traditions operating outside institutional frameworks: Assassin practitioners (patience, positioning, studied execution) and Warlock practitioners (shadow-compact hexes, ward dissolution, entropy techniques).

The tradition's central doctrine is **pre-contact resistance stripping**: rather than waiting for an opening in the target's defenses to appear (Assassin's reactive approach), the Nightblade manufactures the opening on a schedule they control, by applying hex effects before striking. The three named technique categories are:

- **Ward dissolution**: Proactively degrading spiritual protective wards before the strike is delivered, timing the dissolution to complete at the moment of contact
- **Reflex disruption**: Targeted interference in specific defensive reflex chains — not broad cognitive disruption, but disruption of the precise reflexes the attack vector needs to overcome
- **Shadow reading**: Warlock compact-based perception of the target's defensive energy distribution in real time, revealing which angles are underdefended

The tradition's name practitioners, **Osh** and **the Pale Referral**, developed the key insight: shadow-compact techniques can be *timed and calibrated* to specific positional strike windows rather than simply deployed as stand-alone effects. This is what the Dual Backstab flag represents: a practitioner who has pre-positioned hexes to create not one but two simultaneous vulnerability windows.

### Current Skills

| Skill | Level | Type | Assessment |
|-------|-------|------|------------|
| Poison:Nightshade | 2 | Poison (setup mechanism) | Directly named in lore — excellent |
| Shadow Reading | 5 | Passive (defensive weakness) | Directly named in lore — excellent |
| Hex Ward | 10 | Debuff (buff degradation) | Maps to ward dissolution — good |
| Reflex Disruption | 15 | Debuff (dodge/parry/shield reduction) | Directly named in lore — excellent |

### Assessment: No Changes Proposed

The Nightblade is the **best-aligned** adept class in the current implementation. Three of its four skills are named directly in the lore (`nightblade_lore.md`, Section III and Section V). The fourth — Hex Ward — maps precisely to "ward dissolution," which the lore describes as progressive degradation of spiritual protective wards. The skill naming is accurate: "Hex Ward" (degrading wards via hexes) is true to the Warlock-derived side of the tradition.

The class also has clean thematic coverage across its two component traditions:
- **Assassin side:** Represented in the stance's Dual Backstab flag (the timing-calibrated double-strike) and in Shadow Reading (the positional awareness required for precision strikes)
- **Warlock side:** Represented by Nightshade (entropic hex applied to organic systems), Hex Ward (shadow-compact ward dissolution), and Reflex Disruption (targeted interference in specific reflex chains)

The `nightblade_lore.md` Section III's three technique categories (ward dissolution, reflex disruption, shadow reading) each have a corresponding skill. Poison:Nightshade fills the fourth slot as the tradition's biological-mechanism version of pre-contact resistance stripping, which is explicitly documented in Section V.

**No skill additions or modifications are proposed for Nightblade.** The class stands as the template for what good lore-to-skill alignment looks like: every skill has a named lore origin, every major lore technique has a skill representation, and the skill set as a whole expresses both component traditions.

---

## Section V: Crusader

### Lore Summary

The Crusader title originates as a Solar Court **campaign-honor designation** for wardens who served successive engagements without rotation, accumulating continuous field experience rather than episodic deployment. The modern tradition emerged from the **Ash-and-Iron Decades**, when Knight academy graduates and Swordsman dueling-court graduates were placed in the same reserve depots under Midgaard's militarization and forced into direct contact.

The resulting synthesis combines two historically competitive doctrines:
- **Knight:** Formation discipline, controlled body-strike chains (punch/kick/knee/headbutt sequences executed as coordinated pressure), collective effectiveness
- **Swordsman:** Individual excellence, escalating momentum (progressive pressure increase that the Cutting Books' timing theory describes), building force through sustained engagement

The Crusader doctrine: **chain the Knight's body-strikes into the Swordsman's building momentum**, and the accumulated force exceeds what either path sustains alone. This is not simply mixing two fight styles — it is using the Knight's structured impact chain as the *mechanism* by which the Swordsman's escalating momentum is generated, so that each body-strike adds momentum rather than spending it.

The **Anti Magic Shell** technique is a specific extension of Knight doctrine (physical discipline as foundation of defense) into a domain Knight doctrine hadn't previously addressed: the Ash-and-Iron campaigns included magical disruption from the destabilized Cinderteeth Mountains, and the Corps developed a physical-discipline-based counter.

### Current Skills

| Skill | Level | Type | Assessment |
|-------|-------|------|------------|
| Anti Magic Shell | 2 | Defensive (block magic) | Directly from lore — excellent |
| Momentum Chain | 5 | Combo (stacking damage bonus) | Maps to Swordsman escalating momentum — good |
| Iron Resolve | 10 | Passive (resist position change) | Maps to Knight stability — acceptable |
| Overwhelming Assault | 15 | Finisher (proportional to combo count) | Maps to accumulated force — good |

### Issue: The "Chaining Knight Strikes Into Swordsman Momentum" Mechanic Is Implied but Absent

The three combat skills (Momentum Chain, Iron Resolve, Overwhelming Assault) are all thematically consistent but they represent the **outcome** of the Crusader synthesis rather than the **mechanism**. The lore is specific: Knight body-strikes are the *input* to the Swordsman momentum system. The combo that Momentum Chain tracks and Overwhelming Assault expends should be built primarily through body-strikes — the Knight-tradition techniques (punch, kick, knee, headbutt, bash) — not through general combat actions.

Iron Resolve (resisting position-changing effects like bash and trip) is the weakest link thematically. It is defensive and passive, and while it can be read as "the Knight's stability doctrine makes me harder to knock down," it doesn't express anything distinctive about the *Crusader synthesis*. A Knight has reason to resist knockdown; so does a Swordsman who needs uninterrupted momentum buildup. Iron Resolve doesn't feel like something that required both traditions to develop — it feels like something either tradition would have developed independently.

The lore's other distinctive element — **continuous field service as the origin of doctrinal synthesis** — has no mechanical representation. The Solar Court campaign-honor origin means Crusaders are defined by fighting better through extended engagements, not just building up one combo burst. The lore describes practitioners who discovered the cross-doctrinal synthesis through repeated extended engagement, not through a single moment.

### Proposed Changes

**1. Retheme Iron Resolve → Veteran's Cadence (Level 10)**

Replace Iron Resolve's "resist position change" focus with a skill that expresses **the Crusader's extended-engagement advantage**. Veteran's Cadence is a passive that activates after a certain number of combat rounds, representing the campaign-service origin: practitioners who had been in the field long enough developed a sustainable rhythm of body-strikes that maintained momentum buildup without degrading over time. Mechanically, this could be a damage or hit-roll bonus that grows over the duration of a combat encounter, up to a cap — the longer the Crusader stays in the fight, the more the synthesis produces.

This is directly grounded in the lore's central contrast: Knight doctrine is about sustained formation pressure; Swordsman doctrine is about escalating momentum. The combination should produce a practitioner who gets *more effective* through a fight rather than one who simply starts strong.

**2. Clarify Momentum Chain's combo-building inputs (skill description, not mechanics)**

The description/help text for Momentum Chain should note that body-strikes (the Knight-tradition techniques) generate combo stack more efficiently than other attack types. This is a behavioral/design clarification that doesn't require a new skill — it reinforces the "chain Knight strikes into Swordsman momentum" doctrine without adding mechanical complexity.

**Implementation scope:**
- New entry in `skill_table_data.c`: `veteran's cadence`, `L(CLASS_CRU, 10)`, passive, `TAR_IGNORE`
- Remove `iron resolve` entry from `skill_table_data.c`
- New `src/skills/do_veterans_cadence.c` (passive that tracks combat duration and scales a bonus)
- Remove or repurpose `src/skills/do_iron_resolve.c`
- Update Momentum Chain's help text in the relevant help file

**Trade-off:** Iron Resolve is not a *bad* skill — it has mechanical utility and isn't lore-contradictory. Replacing it loses that utility. However, the proposal preserves the "resist being knocked out of momentum" aspect implicitly: a practitioner whose Veteran's Cadence is building up has strong incentive to stay in the fight, which is the same functional role Iron Resolve plays (don't get tripped and lose your chain). The difference is that Veteran's Cadence expresses *why* the Crusader doesn't go down — because they have been forged through sustained campaign experience — rather than simply that they resist knockdown mechanically.

---

## Section VI: Kinetimancer

### Lore Summary

The Kinetimancer tradition was named by the Violet Compact's registry scholars as a bureaucratic classification for a "dangerous theoretical synthesis" they couldn't assess: the convergence of **Necromancer** entropy-redirection and **Egomancer** will-imposition, as documented by the independent **Kinetic Formulists** before the Compact's founding.

The Kinetic Formulists' core observation was mathematical: the equations describing how will-imposition propagates through cognitive architecture are structurally analogous to the equations describing how entropic force propagates through biological systems undergoing dissolution. Both traditions were working with **momentum** — one redirecting the momentum of decay, the other redirecting the momentum of imposed intent.

The **momentum doctrine** that defines Kinetimancer practice holds that all effective force operates through redirection of existing momentum vectors rather than generation of force from scratch. This applies in two domains:

- **Physical combat:** The Kinetimancer does not primarily strike — they redirect. An opponent's attack contains momentum; that momentum can be redirected to amplify the Kinetimancer's own effects rather than simply absorbed. Environmental processes (gravity, thermal dynamics) contain momentum; those can be redirected to create effects without equivalent force generation.
- **Cognitive combat:** The Kinetimancer does not primarily impose — they redirect existing cognitive processes. An opponent concentrating intensely on a threat is directing cognitive momentum toward that focus; redirecting that cognitive momentum toward an alternative focus (or disrupting its directional coherence) is more efficient than direct override.

The lore explicitly names multiple technique categories and their applications, culminating in **predictive collapse**: the integration of real-time cognitive modeling (Egomancer) with calibrated entropy application (Necromancer), deploying entropy effects precisely calibrated to the opponent's anticipated responses rather than to a generic target.

### Current Skills

| Skill | Level | Type | Assessment |
|-------|-------|------|------------|
| Kinetic Reversion | 2 | Offensive spell (force reflection) | Directly from lore — excellent |

### Issue: Critical Underdevelopment

The Kinetimancer has **one skill**. Every other adept class has at minimum three. More importantly, the lore is not vague about what other techniques should exist — it describes multiple named capabilities across its five sections. The single implemented skill (Kinetic Reversion) represents only one facet of the momentum doctrine.

What the lore describes in detail and what has no skill representation:

1. **Entropy-based momentum drain** — drawing on the Necromancer tradition's capacity to redirect decay processes: not reversing incoming force but *absorbing* it to fuel the Kinetimancer's own output. This is distinct from Kinetic Reversion (which sends force back to its origin) — this keeps the redirected energy.

2. **Cognitive disruption** — the Egomancer side: fragmenting the directional coherence of an opponent's cognitive momentum so they cannot focus effectively. From `kinetimancer_lore.md` Section III: *"redirecting that cognitive momentum toward an alternative focus, or disrupting the directional coherence of the momentum so it cannot focus effectively."*

3. **Predictive collapse** — explicitly named in the lore as the **high-level** Kinetimancer technique, described at length in Section III and cross-referenced in both `necromancer_lore.md` Section VI and `egomancer_lore.md` Section IV. This is the full synthesis: using cognitive modeling to identify optimal timing and vectors, then deploying calibrated entropy at those precise moments.

The current implementation leaves the Egomancer component of the synthesis with zero skill representation. Kinetic Reversion (force reflection) is a Necromancer-lineage technique applied to kinetic force. There is nothing from the Egomancer's will-imposition and cognitive modeling toolkit.

### Proposed Changes

**1. Add: Force Siphon (Level 5)**

The Necromancer tradition's entropy-redirection applied to kinetic momentum — not reversing it but *absorbing* it. Where Kinetic Reversion sends the force back, Force Siphon pulls the momentum out of an incoming attack or from the environment (gravity, movement) and converts it into the Kinetimancer's own damage output on the next strike. This is the "practitioners treat incoming attacks as potential resources" logic described in `kinetimancer_lore.md` Section IV: *"the more powerful the attack, the more momentum is available for redirection."*

Mechanically: activating Force Siphon charges the next offensive action with bonus damage proportional to the power of the last incoming hit received. It rewards being in a dangerous situation (taking powerful hits) by making the response proportionally more dangerous — a direct expression of the momentum doctrine.

**2. Add: Cognitive Fracture (Level 10)**

The Egomancer component of the synthesis. This is targeted cognitive disruption — an assault on an opponent's ability to direct their mental momentum toward any specific task. In combat terms, it fragments the opponent's situational coherence: they cannot focus on attacking, cannot focus on defense, cannot land the specific technique they were winding up for. Unlike Hex Ward (Nightblade, which degrades buffs) or Reflex Disruption (Nightblade, which targets specific defensive reflexes), Cognitive Fracture attacks the *direction* of the opponent's cognitive momentum rather than its specific outputs.

Mechanically: a targeted debuff that reduces the target's accuracy, causes occasional misfires on active skills they attempt, and briefly suppresses any "building charge" effects they have active (combo stacks, chi accumulation, active buff chains). It is the Kinetimancer's specific counter to momentum-building opponents — turning their buildup against them by disrupting the cognitive coherence that buildup requires.

**3. Add: [Name TBD] (Level 15)**

The apex synthesis technique of the Kinetimancer tradition — described in `kinetimancer_lore.md` Section III and cross-referenced in the Egomancer and Necromancer lore files. The lore describes it as deploying entropy effects calibrated precisely to anticipated opponent responses, using real-time cognitive modeling to identify exactly when and where the entropy application will land with maximum effect.

*Note: the name "Predictive Collapse" was originally proposed for this skill but has since been given to the existing L2 skill (formerly "Kinetic Reversion"). A new name is needed for this L15 slot.*

Mechanically: a high-mana spell that deals substantial entropy-type damage, with a large bonus against opponents who are in a "building" state (mid-combo, mid-cast, charging an ability). It is the synthesis skill in the same way that Convergent Cascade is for Grand Magi: the moment where both component traditions operate simultaneously to produce an effect neither achieves alone. A Necromancer deploys entropy broadly; an Egomancer reads cognitive states; the Kinetimancer deploys entropy at the precise cognitive moment when it collapses the opponent's developing action.

**Implementation scope:**
- Three new entries in `skill_table_data.c`: `force siphon` (L5, offensive, ~100 mana), `cognitive fracture` (L10, offensive debuff, ~120 mana), `predictive collapse` (L15, offensive, ~150 mana)
- Three new implementation files: `do_force_siphon.c`, `do_cognitive_fracture.c`, `do_predictive_collapse.c`

**Trade-off:** This is the largest addition in the proposal — three new skills, bringing Kinetimancer from one skill to four. This is justified on two grounds: (1) the lore explicitly names all three techniques, making them specifications to be implemented rather than creative additions, and (2) the current single-skill state means the class is mechanically incoherent — the Egomancer component of the synthesis is entirely absent at the gameplay level. A player who chose Kinetimancer for its cognitive-manipulation lore would have nothing that expresses that character.

---

## Section VII: Martial Artist

### Lore Summary

The Martial Artist tradition synthesizes **Monk** (Interior Forms — breath discipline, postural precision, internal chi energy cultivation) and **Brawler** (pit-tested technique — physical impact methodology, grinding pressure, attrition-based endurance). The synthesis emerged from the **Copper Bracket Fights** in Kowloon's Seventh Harbor Circuit, where Meret Sohn (monastery-trained, sought pit technique) and Tessala Wink (pit veteran, sought Interior Forms) demonstrated in public that neither tradition was complete without the other.

The foundational discovery was the **chi integration**: applying the Monk's internal chi circulation patterns to the Brawler's physical impact methodology produces impacts with qualities neither approach alone achieves. The Brawler's mechanical efficiency combined with the Monk's internal energy contribution creates a synergistic effect that neither tradition's framework fully explains — which is why the tradition describes it as synthesis rather than derivation.

The lore is explicit that this is an **entirely secular, internally-derived tradition**. The Interior Forms are about breath, structural precision, and internal energy circulation. The Brawler tradition is about physical impact mechanics and body knowledge from pit testing. Neither tradition has any divine connection. The tradition's power comes from the combination of internal discipline and physical testing — it is earned through practice, not granted through covenant or faith.

### Current Skills

| Skill | Level | Type | Assessment |
|-------|-------|------|------------|
| Chi Surge | 5 | Active (all-chi offensive) | Thematic |
| Breath of Endurance | 10 | Active (chi-cost HoT + damage reduction) | Directly thematic — excellent |
| Fist of the Interior Form | 15 | Active (chi-gated, PHYSICAL\|HOLY damage) | **Lore contradiction — see below** |

### Issue: HOLY Damage on Fist of the Interior Form

**Fist of the Interior Form** deals `PHYSICAL|HOLY` damage. This is a lore contradiction.

The Interior Forms have no divine connection in the lore. They are a secular internal discipline — *interior* because the power comes from within the practitioner (breath, chi circulation, structural alignment), not from an external divine source. The word "interior" in the name is the lore document's own emphasis on this:

> *"The Monk tradition's Inner Forms include extensive work with what practitioners call chi — internal energy circulation patterns that, when correctly developed, contribute to sustained physical output and specific impact qualities in striking."* — `martial_artist_lore.md`, Section III

There is no mention of divine power, sacred covenant, divine favor, or anything that would produce HOLY-type damage in any of the relevant lore files: `martial_artist_lore.md`, `monk_lore.md`, or `brawler_lore.md`. HOLY damage is used elsewhere in the game for effects tied to explicit sacred traditions: Paladin (oath to the light, divine covenant), Priest (sacred ministry), and Templar (Solar Court temple-guardian inheritance). All three of these traditions have explicit theological frameworks connecting them to divine power.

The Martial Artist tradition is not theologically connected to anything. It is a practice tradition — two groups of fighters who discovered through empirical testing that their disciplines were complementary. Chi is not divine energy in this world's framework; it is internal energy cultivated through practice.

The HOLY damage element likely originated as a balance choice (giving the class a second damage type for variety or resist-bypass) rather than as a lore decision. But the specific type chosen — HOLY — is the one most inconsistent with the class's entire identity.

### Proposed Change

**Modify: Fist of the Interior Form — change damage type from `PHYSICAL|HOLY` to `PHYSICAL|CHI`**

If `CHI` is not a defined damage type in the game's damage type system, the alternatives in lore-accuracy order are:

1. **`PHYSICAL|CHI`** — ideal; chi as a distinct internal-energy damage type reflects exactly what the lore describes
2. **`PHYSICAL` (amplified)** — acceptable; a massive physical-only hit with a large multiplier, representing chi-augmented impact mechanics without needing a second damage type
3. **`PHYSICAL|FORCE`** — acceptable; force as the expression of chi-converted physical energy, without divine connotation
4. **`PHYSICAL|MAGIC`** — marginal; implies arcane origin rather than internal cultivation, but at least avoids the divine frame

Any of these is preferable to HOLY. The key requirement is that the damage type should convey *internal energy from disciplined practice*, not *divine favor from sacred covenant*.

**No other changes proposed for Martial Artist.** Chi Surge (spend all chi for massive strike) correctly expresses the Brawler's ferocity channeled through chi. Breath of Endurance (Monk's breath control producing sustained defensive and restorative output) is directly grounded in the lore's description of breath discipline. The only issue is the damage type on Fist of the Interior Form.

**Implementation scope:**
- Modify `do_fist_of_the_interior_form.c` to change the damage type from `PHYSICAL|HOLY` to `PHYSICAL|CHI` (or the appropriate alternative from the list above, depending on what damage types are defined in `typedefs.h`)
- No changes to `skill_table_data.c` required

**Trade-off:** Changing HOLY to CHI (or another internal-energy type) may affect the skill's resist-bypass properties, since HOLY has specific resist interactions in the game's combat system. If CHI is a new damage type with no defined resist, it may be slightly less effective against targets with HOLY resistance and slightly more effective against targets without it. This is acceptable: the skill should bypass physical damage reduction (the lore explicitly says the chi-augmented impact exceeds what mechanical efficiency alone explains) but there is no lore basis for it bypassing specifically *holy* defenses.

---

## Section VIII: Summary, Affected Files, and Sign-off Requirements

### Change Summary by Class

| Class | Finding | Proposed Action | Priority |
|-------|---------|-----------------|----------|
| Grand Magi | Theme gap: sequential vs. simultaneous convergence | Add Convergent Cascade (L8); clarify Attunement threshold logic | Medium |
| Templar | Theme gap: threshold warden role absent; L1–L10 skill gap | Add Consecrated Ground (L5) | Medium |
| Nightblade | No issues | No changes | — |
| Crusader | Theme gap: extended-engagement origin absent; Iron Resolve too generic | Replace Iron Resolve with Veteran's Cadence (L10) | Medium |
| Kinetimancer | Critical: Egomancer component absent; lore names 3 unimplemented techniques | Add Force Siphon (L5), Cognitive Fracture (L10), Predictive Collapse (L15) | High |
| Martial Artist | Contradiction: HOLY damage on Fist of the Interior Form | Change damage type from PHYSICAL\|HOLY to PHYSICAL\|CHI (or equivalent) | High |

### What Changes vs. What Doesn't

**Changes:** Five of the six classes require some modification. The most significant is Kinetimancer (+3 skills). The most surgical is Martial Artist (one line in one implementation file). Crusader requires a skill swap. Grand Magi and Templar each require one addition.

**Doesn't change:** The four-domain elemental structure of Grand Magi. All three existing Templar skills. The entire Nightblade skill set. Anti Magic Shell, Momentum Chain, and Overwhelming Assault for Crusader. Kinetic Reversion for Kinetimancer. Chi Surge and Breath of Endurance for Martial Artist.

### Affected Source Files

**New files required:**
- `src/skills/do_convergent_cascade.c` (Grand Magi, L8)
- `src/skills/do_consecrated_ground.c` (Templar, L5)
- `src/skills/do_veterans_cadence.c` (Crusader, L10, replaces iron_resolve)
- `src/skills/do_force_siphon.c` (Kinetimancer, L5)
- `src/skills/do_cognitive_fracture.c` (Kinetimancer, L10)
- `src/skills/do_predictive_collapse.c` (Kinetimancer, L15)

**Modified files:**
- `src/skills/skill_table_data.c` — new entries for Convergent Cascade, Consecrated Ground, Veteran's Cadence, Force Siphon, Cognitive Fracture, Predictive Collapse; remove Iron Resolve entry
- `src/skills/do_iron_resolve.c` — remove or repurpose
- `src/skills/do_fist_of_the_interior_form.c` — change damage type constant
- `src/Makefile` — new object targets for each new `.c` file
- `src/skills/do_elemental_attunement.c` — threshold logic modification (if this file exists; otherwise the logic lives in the spell handler)

**Lore files:** No lore file changes are required. All proposals are grounded in existing lore documents. However, if Kinetimancer's three new skills are implemented, the `kinetimancer_lore.md` file should have a brief mention of Force Siphon and Cognitive Fracture as named technique expressions (currently the lore describes them conceptually but doesn't name them as discrete techniques). This would be a pure `docs/lore/` edit, which does not require a design sign-off per CLAUDE.md.

### Priority Order for Implementation

If implementing in stages rather than all at once, the recommended order is:

1. **Martial Artist — damage type fix** (single-file change, corrects a factual lore contradiction)
2. **Kinetimancer — Force Siphon, Cognitive Fracture, Predictive Collapse** (fills the largest gap; highest gameplay impact)
3. **Crusader — Iron Resolve → Veteran's Cadence** (skill swap; requires care about removing existing skill without breaking save files or skill table indices)
4. **Templar — Consecrated Ground** (adds new skill; no removal required)
5. **Grand Magi — Convergent Cascade + Attunement threshold** (adds new skill; requires more nuanced attunement logic)

### Discussion Questions

Before implementation, the following require explicit decisions:

1. **Is `CHI` a defined damage type?** If not, which of the four alternatives listed in Section VII best captures the Martial Artist's lore? This determines the Fist of the Interior Form change.

2. **For Veteran's Cadence:** Should the extended-engagement bonus be time-based (combat duration in rounds) or action-based (number of hits landed)? The lore supports either: "continuous service" implies time, while "chain strikes into momentum" implies action count.

3. **For Consecrated Ground:** Should the room effect be visible to all players (room message on cast) or silent? Templar lore doesn't indicate a showy consecration rite — the auxiliaries consecrated space as part of their function, not as a performance. A quiet ongoing effect would be more consistent with the tradition's character.

4. **Iron Resolve removal:** Are there any players currently playing Crusaders who have Iron Resolve in their skill set? Removing a skill from the table may require a migration step in save file loading to avoid stale skill references.
