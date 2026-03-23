# Proposal: Mortal Class Skill Lore Audit

**Status:** Draft — for discussion  
**Scope:** `src/` changes required (spell/skill table modifications, new skill implementations)  
**Depends on:** Completed work in `lore-skill-stance-renames.md` (gap transit, reach silence, applied understanding, seven shade hold, Dunmar's Watch, Gap Intuition, Deepest Form already implemented)

---

## Section I: Overview and Audit Methodology

### Background

The previous lore-skill work (`lore-skill-stance-renames.md`, now completed) renamed specific skills and stances whose in-game names were generic where the lore had established better ones. That work was targeted: four skill renames and three stance renames, all cases where a clear lore-grounded name existed and implementation was straightforward.

This audit goes deeper. It examines every skill and spell assigned to each mortal class and asks three questions:

1. **Is this skill lore-hostile?** Does it actively contradict the class's documented tradition — using the wrong magical lineage, employing concepts that belong to a different institution, or doing things the lore says this tradition specifically does not do?
2. **Is this skill lore-neutral?** Is it generic fantasy vocabulary with no connection to the class's documented world — present because every MUD needs a healing spell, not because it means something?
3. **Is this skill lore-grounded?** Does the name, mechanism, or flavor connect to the specific historical institutions, doctrines, practitioners, or events that define the class?

The goal is not to replace every generic skill — some generic names are fine. The goal is to identify the gaps and contradictions that make a class feel like a list of D&D spell names rather than a tradition with a specific history.

### Audit Categories

**Tier A — Lore-hostile (highest priority):** The skill actively contradicts the class's documented tradition. Remove or replace.

**Tier B — Lore-neutral (medium priority):** The skill is generic but not wrong. Could be renamed or replaced with something meaningful, but the current name causes no confusion.

**Tier C — Missing lore (medium priority):** An important element of the class's documented tradition has no mechanical representation. The class lacks something it should have.

**Tier D — Rename opportunity (lower priority):** The skill is correctly placed but has a generic name where the lore offers a better one. The completed renames proposal handled the clearest cases; this tier captures remaining opportunities.

### What Was Already Done

The following changes are already implemented and not revisited here:

| Old name | New name | Class |
|---|---|---|
| `shadow step` | `gap transit` | Cipher |
| `garrote` | `reach silence` | Cipher |
| `pressure point` | `applied understanding` | Pugilist / Monk |
| `fortify` | `seven shade hold` | Warden |

Stances already renamed: `Warden` → `Dunmar's Watch`, `Cipher` → `Gap Intuition`, `Monk` → `Deepest Form`.

The four remaining mortal stances — **Magi**, **Healer** (Cleric), **Psionicist**, and **Pugilist** — were left in the previous proposal because no lore-grounded alternative had been developed. This audit proposes names for all four.

### Audit Scope

Six mortal classes audited:

- **Magi** — 35 spells, 1 skill (`mystical potency`), 1 stance (`Magi`)
- **Cleric** — 30 spells, 1 skill (`spell critical damage`), 1 stance (`Healer`)
- **Psionicist** — 26 spells, 2 skills (`spell critical`, `shadowform`), 1 stance (`Psionicist`)
- **Warden** — 22 skills, 1 stance (`Dunmar's Watch` — already done)
- **Cipher** — 24 skills, 1 stance (`Gap Intuition` — already done)
- **Pugilist** — 16 skills, 1 stance (`Pugilist`)

---
## Section II: Magi

### Tradition Summary

The Magi tradition is the Wizard's Keep — an institution founded as the explicit corrective to the Spirebound Conclave's catastrophe. The Keep operates under Violet Compact registry law, enforces the Triple-Ledger Rule, and exists in a frontier military context in the Cinderteeth Mountains. Its practitioners carry the Conclave's three-faction inheritance: **Ember Canon** (fire, will-through-suffering, lifesteal), **Prism Thesis** (crystal harmonics, light refraction, structural resonance), and **Pale Ledger** (spirit imprinting, memory extraction — now primarily the Psionicist lineage). The Keep is bureaucratic, documented, pragmatic, and guilt-aware. It is not a court wizard academy; it is a regulated frontier institution with archives of atrocity it is custodian of.

### Spell Audit

**Lore-grounded (no change needed):**

| Spell | Lore connection |
|---|---|
| `fireball`, `burning hands` | Ember Canon fire research |
| `lightning bolt`, `chain lightning`, `shocking grasp`, `chill touch` | Frontier combat — Keep's militarized practical doctrine |
| `magic missile` | Foundational, generic but fine |
| `fly`, `pass door`, `portal`, `beacon`, `teleport`, `window` | Frontier traversal — the Keep's field-research necessity |
| `seal room`, `arcane lock` | Violet Compact registry law — securing documented spaces |
| `identify`, `detect magic`, `detect invis`, `detect evil`, `detect hidden`, `detect undead` | Keep's audit and assessment mandate |
| `locate object`, `sight` | Field reconnaissance |
| `know alignment` | Assessment, fine |
| `invis`, `mass invis` | Operational utility |
| `mirror image`, `elemental ward`, `mana barrier`, `mystic armour`, `armor`, `shield`, `stone skin` | Defensive doctrine |
| `dispel magic`, `dispel object` | Keep's mandate to counter dangerous magic |
| `lifesteal` | Ember Canon inheritance — ethically contested but documented |
| `enchant weapon`, `blindness`, `curse`, `summon`, `regen` | Fine |
| `fireball`, `wall of fire` | Fine |

**Tier A — Lore-hostile:**

None. The Magi spell list has no active contradictions with its tradition — unlike Cleric and Psionicist, the Keep's pragmatic frontier identity is broad enough to accommodate most combat and utility magic.

**Tier B — Lore-neutral:**

| Spell | Level | Problem |
|---|---|---|
| `faerie fire` | 3 | Fae magic. The Cinderteeth frontier has no documented fae tradition; the Keep works in elemental and structural magic. This is a borrowed D&D spell with no world-connection. |
| `color spray` | 7 | Cosmetically generic. No connection to any Conclave faction's documented research domain. |
| `ventriloquate` | 3 | A party trick. The Keep's low-level curriculum should be establishing registry practice and elemental fundamentals, not voice-throwing. |
| `acid blast` | 50 | Effective spell but named for no tradition. The Ember Canon does fire, Prism Thesis does crystal, Pale Ledger does spirit. "Acid" maps to none of the Keep's documented research lineages. |

**Tier C — Missing lore:**

| Missing element | Lore basis |
|---|---|
| Prism Thesis representation | The crystal harmonics faction has zero named spell representation. Magi currently have fire spells (Ember Canon), spirit-adjacent utility, and generic combat magic. No spell is distinctly *crystalline*. |
| Institutional identity | The Keep's defining characteristic — documented, witnessed, triple-ledger — has no low-level mechanical expression. `seal room` at level 70 is the only institutional spell, and it's very late. |

**Tier D — Rename opportunities:**

| Current name | Proposed name | Lore basis |
|---|---|---|
| `mystical potency` (skill) | `compact amplification` | The Violet Compact's sanctioning of registered practitioners — Keep-certified magi are specifically authorized to draw on amplified resonance as a registered benefit. "Mystical potency" is generic; "compact amplification" names the institutional mechanism. |
| `acid blast` | `dissolution` | Reframes the spell as the Keep's capacity to unmake improper magical constructions — resonant with the institution's founding mandate to correct Conclave excess. Functionally identical. |

**Stance — Tier D:**

The `Magi` stance is simply the class name. The lore offers richer vocabulary. The Keep's founding documents use the phrase **"Ember Protocol"** — referring to the Ember Canon's practical combat application doctrine that the Keep institutionalized when it inherited the faction's functional knowledge. A magi in full combat stance is executing the Keep's sanctioned combat framework, not simply "being a magi."

Proposed: `Magi` → **`Ember Protocol`**

### Proposed Changes Summary

| Change | Tier | Type | Notes |
|---|---|---|---|
| Replace `faerie fire` | B | Spell swap | Replace with `prism mark` — a Prism Thesis light-marking technique that disorients; same mechanical role (debuff/reveal), different lineage |
| Replace `color spray` | B | Spell swap | Replace with `crystal burst` — Prism Thesis crystal resonance weaponized; same mechanical role (multi-target damage), anchors Prism Thesis in the spell list |
| Replace `ventriloquate` | B | Spell removal | Remove outright or replace with `ledger inscription` — a low-level utility spell representing the Keep's documentation practice (functionally similar to identify but flavored as registry work) |
| Rename `acid blast` | D | Rename | → `dissolution` |
| Rename `mystical potency` | D | Rename | → `compact amplification` |
| Rename stance `Magi` | D | Stance rename | → `Ember Protocol` |

The Prism Thesis swap (replacing `faerie fire` and `color spray` with `prism mark` and `crystal burst`) is the highest-value change: it gives the Conclave's second faction a named presence in the Magi spell list, and removes two spells with zero world-connection.

---
## Section III: Cleric

### Tradition Summary

Clerics are the inheritors of the **Solar Court of the Undivided Disk** — the desert civilization's religious-administrative apparatus that built the Great Pyramid to contain the Black Sun Shard. The Solar Court organized its priesthood into five offices: **Dawn Liturgists** (cosmological calendars, sunrise rites), **Ledger Priests** (administrative magic, tribute, resource distribution), **Embalmer Houses** (containment of the dead, boundary between living and unliving), **Jackal Tribunal** (posthumous justice, testimony cross-reference, record reconciliation), and **Seal College** (reverse-solar geometry, the mathematical framework keeping the Shard dormant). The modern cleric tradition draws most heavily from the Dawn Liturgists and Seal College.

Faith, in this tradition, is *infrastructure* — not sentiment. Rites are maintenance procedures. The Cleric exists to keep cosmological machinery running. This is a desert civilization built on precision measurement, geometric containment, and bureaucratic rigor — not on nature, storms, or fae mystery.

### Spell Audit

**Lore-grounded (no change needed):**

| Spell | Lore connection |
|---|---|
| `solar flare` | Directly named after the Solar Court's fundamental symbol |
| `seal room` | Seal College's geometric containment practice |
| `turn undead` | Embalmer Houses — containing the unliving is their primary function |
| `exorcism` | Embalmer Houses / Jackal Tribunal — removing improper spiritual presences |
| `flamestrike` | Solar Court — the Undivided Disk's fire is a legitimate offensive expression |
| `divine strike` | Solar Court offensive doctrine, fine |
| `consecrate` | Sanctification of space, fits |
| `heal`, `restoration`, `influx`, `regen`, `refresh` | Healing tradition, fine |
| `cure blindness`, `cure poison`, `remove curse` | Remediation, fits the maintenance-of-order doctrine |
| `bless`, `sanctuary`, `protection` | Divine protection, fine |
| `detect evil`, `detect magic`, `detect hidden`, `detect poison` | Assessment, fine |
| `know alignment` | Jackal Tribunal — posthumous judgment requires alignment assessment |
| `locate object` | Ledger Priest administrative tracking |
| `continual light` | Dawn Liturgist — maintaining light against the Shard's anti-light properties |
| `prayer` | Fine, though see rename opportunity below |
| `word of recall` | Practical traversal, fine |
| `holy word` | See rename opportunity below |

**Tier A — Lore-hostile:**

These are the most urgent changes in the entire audit. The Solar Court is a desert civilization of geometric precision built to contain a specific cosmological threat. It has no storm theology, no nature doctrine, and its containment mandate makes destructive earth-shaking magic an institutional contradiction.

| Spell | Level | Problem |
|---|---|---|
| `call lightning` | 36 | Storm magic. The Solar Court worships the Undivided Disk — a solar, not meteorological, cosmology. There is no documented storm tradition in the desert civilization. This spell belongs to druidic or nature-deity traditions entirely absent from Solar Court lore. |
| `control weather` | 79 | Same problem, amplified. The Seal College does precision mathematics; the Dawn Liturgists do calendrical astronomy. Neither tradition has any basis for weather manipulation. This is imported D&D cleric vocabulary with no world-connection. |
| `earthquake` | 78 | Actively contradicts the containment doctrine. Clerics exist to maintain structural integrity — the Great Pyramid, the obelisk-line network, the cosmological machinery. Shaking the earth apart is the antithesis of what they do. The Khephari give seismic *warnings* precisely because the Solar Court cannot afford geological disruption. |
| `faerie fog` | 14 | Fae magic in a desert institutional priesthood. There is no documented fae tradition anywhere in Solar Court lore. This is a generic early utility spell that could be replaced with something from the actual tradition. |

**Tier B — Lore-neutral:**

| Spell | Problem |
|---|---|
| `cause serious` (40) | Generic D&D offensive spell. The Solar Court had specific frameworks for harm — Jackal Tribunal judgment, Embalmer House containment — but "cause serious" carries no connection to any of them. |
| `cause critical` (58) | Same. The highest-tier Solar Court offensive spell should name something. |

**Tier C — Missing lore:**

| Missing element | Lore basis |
|---|---|
| Shard containment | The entire reason the Solar Court exists — keeping the Black Sun Shard dormant — has zero spell representation. The Cleric's deepest tradition is containment doctrine, and there is no containment spell. |
| Seal College mathematics | The Seal College's reverse-solar geometry is the most technically sophisticated office's contribution. `seal room` is present (correctly), but nothing represents the geometric precision work that the Seal College is defined by. |
| Jackal Tribunal judgment | The posthumous justice office has `know alignment` and `exorcism` adjacent to it, but no spell that names or evokes the Tribunal's specific function — weighing, judging, condemning. |
| Dawn Liturgist precision | The office most committed to maintaining cosmic order has no named spell. `continual light` is adjacent, but the Dawn Liturgists' specific function — tracking light conditions the Shard could exploit, maintaining sunrise rites — has no mechanical expression. |

**Tier D — Rename opportunities:**

| Current name | Proposed name | Lore basis |
|---|---|---|
| `prayer` | `solar invocation` | The Solar Court's prayers are invocations of the Undivided Disk — not generic supplication. "Prayer" is fine but says nothing; "solar invocation" locates the practice in the tradition. |
| `holy word` | `disk's word` | Same reasoning. "Holy word" is generic divine vocabulary; "disk's word" names the specific theological source. |
| `cause serious` | `solar censure` | The Solar Court's intermediate offensive doctrine — not raw damage but institutional condemnation with force behind it. Jackal Tribunal ancestry. |
| `cause critical` | `tribunal's hand` | The Jackal Tribunal's decisive judgment made physical. High-level Solar Court offensive. |
| `spell critical damage` (skill) | `solar precision` | The Dawn Liturgists' defining quality — tracking conditions with exactitude enough that the margin for critical failure or success is well-understood. A Cleric who lands a critical has applied Solar precision to an opening the target created. |

**Stance — Tier D:**

The `Healer` stance names a function rather than a tradition. A Cleric at level 60 is not simply "a healer" — they are a practitioner of the Solar Court's five-office legacy. The stance's mechanical effect (Heal+1) does reflect healing, but the *name* should reflect the tradition the healing comes from.

The Solar Court organized everything through its five offices. A senior Cleric operating at full capacity is functioning as a **Solar Office** practitioner — deploying the consolidated institutional knowledge of all five offices in their fully integrated form.

Proposed: `Healer` → **`Solar Office`**

### Proposed Changes Summary

| Change | Tier | Type | Notes |
|---|---|---|---|
| Replace `call lightning` | A | Spell swap | → `shard ward`: a containment spell (defensive ward vs. undead/anti-light); fills the Tier C Shard gap simultaneously |
| Replace `control weather` | A | Spell swap | → `seal calculus`: a Seal College geometric binding; fills the Tier C Seal College gap simultaneously |
| Replace `earthquake` | A | Spell swap | → `jackal's verdict`: the Jackal Tribunal's judgment made manifest; fills the Tier C Tribunal gap simultaneously |
| Replace `faerie fog` | A | Spell swap | → `ledger mark`: Ledger Priest administrative marking (functions like faerie fire — reveals and tracks); fills the Tier C Ledger gap simultaneously |
| Rename `cause serious` | D | Rename | → `solar censure` |
| Rename `cause critical` | D | Rename | → `tribunal's hand` |
| Rename `prayer` | D | Rename | → `solar invocation` |
| Rename `holy word` | D | Rename | → `disk's word` |
| Rename `spell critical damage` | D | Rename | → `solar precision` |
| Rename stance `Healer` | D | Stance rename | → `Solar Office` |

The four Tier A removals are the most important changes in the audit. Replacing them with four spells drawn directly from the Solar Court's five offices simultaneously resolves the lore-hostile problem *and* the four missing-lore gaps. The new spells don't require inventing new mechanical functions — they take mechanical roles that the removed spells occupied (area debuff, binding, judgment/curse, reveal) and re-skin them with tradition-appropriate names and flavor.

---
## Section IV: Psionicist

### Tradition Summary

The Psionicist tradition has three partial ancestries: the **Spirebound Conclave's Pale Ledger faction** (spirit imprinting, iterative memory extraction — understanding how consciousness is structured from the outside), the **Void Citadel's recurrence technology** (testimony replay until resistance collapses — cognitive flooding rather than extraction), and the **institutional cognitive discipline traditions** (Kel'Shadra witness doctrine, Seal College cognitive coherence, bureaucratic resistance to external pressure). The modern tradition drew its formal academic structure primarily from the Pale Ledger inheritance and the ambient cognitive discipline lineages; the Void Citadel ancestry is the part practitioners carefully avoid discussing.

The defining tension in the tradition is between the **academic mainstream** (defensive and analytical: understanding minds to protect one's own) and the **Assertion Wing** (offensive: imposing cognitive states on others rather than merely defending against them). The Assertion Wing's most extreme product — the **Long Hold**, developed by **Vel Ashe** — is the technique for continuous low-intensity cognitive pressure that gradually redirects a target's assessments without their awareness. The Long Hold caused the formal expulsion that split the Egomancer lineage from the mainstream.

This is not a fire tradition. It is not a divine tradition. It is a cognitive architecture tradition: practitioners who work with the structure of will, memory, and consciousness as their primary medium.

### Spell Audit

**Lore-grounded (no change needed):**

| Spell | Lore connection |
|---|---|
| `mind flail`, `mind bolt`, `psychic thrust`, `psychic crush`, `ego whip` | Assertion Wing offensive cognitive techniques |
| `thoughtsteal` | Pale Ledger memory extraction — the most directly lore-named spell on the list |
| `phobia` | Memory/fear manipulation — Pale Ledger's iterative memory work produces this naturally |
| `precognition` | Cognitive modeling of incoming data — fits the tradition's analytical emphasis |
| `barrier`, `mindwall` | Defensive cognitive architecture — the Pale Ledger's dual-use insight (defense as valuable as offense) |
| `hypnosis` | Cognitive control, fine |
| `detection`, `know weakness`, `know critical`, `know item` | Information/assessment — Pale Ledger assessment methodology |
| `pain` | Assertion Wing — imposing states, not defending against them |
| `calm` | Cognitive regulation, fine |
| `phase` | Perceptual manipulation of spatial relationship, fits |
| `planergy` | Psionic energy foundation, fine |
| `bloody tears` | Cognitive horror, fits |
| `psionic recovery` | Self-healing through cognitive discipline — the tradition's resistance-building methodology |
| `suffocate` | Telekinetic — adjacent enough to cognitive force application |
| `visit` | Remote cognitive presence, fine |
| `astral projection` | Cognitive projection rather than literal astral travel — fits if framed as extended visit |
| `fighting trance` | Functional, see rename opportunity |
| `nerve fire` | Neural system manipulation, name works |

**Tier A — Lore-hostile:**

| Spell | Level | Problem |
|---|---|---|
| `purify` | 19 | Purification is explicitly a Solar Court / Cleric concept. The five-office priesthood manages contamination through ritual precision; the Embalmer Houses handle boundary violations; the Seal College manages propagation of the Shard's anti-light. A cognitive architecture practitioner has no documented basis for purification. The name belongs to a different tradition entirely. |
| `sense evil` | 10 | Alignment detection through divine attunement is a Paladin and Cleric mechanism — a function of moral cosmology, not cognitive science. The Psionicist tradition works through materialist cognitive analysis. A Psionicist who wants to detect threat does so by reading cognitive state, not by sensing moral alignment. This spell name belongs to the divine tradition, not here. |
| `mindflame` | 35 | There is no lore basis for combining "mind" with "flame." Fire is the Ember Canon's domain (Magi). The Psionicist tradition draws from Pale Ledger, Void Citadel recurrence tech, and cognitive discipline institutions — none of which involve fire in any documented way. "Mindflame" is a name that sounds psionic-adjacent but is actually an Ember Canon import. |

**Tier C — Missing lore:**

| Missing element | Lore basis |
|---|---|
| Long Hold | The most significant technique in the Psionicist tradition's documented history is absent from the spell list. Vel Ashe's continuous low-intensity cognitive pressure — below the threshold of obvious disruption, progressively redirecting a target's assessments over extended periods — is what caused the Egomancer schism and is the tradition's most feared capability. It should exist. |
| Pale Ledger imprinting | The Pale Ledger's spirit imprinting work — leaving a cognitive mark on a target, tracking their mental state over time — has no direct mechanical equivalent. `thoughtsteal` covers extraction; nothing covers the imprinting side. |
| Recurrence loop | The Void Citadel's recurrence technology — replaying testimony/memory iteratively until resistance collapses — is documented as a mechanically distinct technique from standard Assertion Wing methods. It operates through repetition and cognitive flooding rather than direct force. |

**Tier D — Rename opportunities:**

| Current name | Proposed name | Lore basis |
|---|---|---|
| `see magic` | `architecture scan` | Psionicists perceive magic not as a glow or shimmer but as structural patterning — cognitive architecture applied to magical systems. "See magic" is generic; "architecture scan" names the cognitive methodology. |
| `fighting trance` | `assertion form` | The Assertion Wing's combat doctrine — the practitioner in full offensive cognitive mode. "Fighting trance" is generic; "assertion form" names the Assertion Wing tradition directly. |
| `purge` | `cognitive flush` | Mechanically similar function but the name "purge" evokes cleansing-tradition vocabulary. "Cognitive flush" reframes the same action as resetting a corrupted cognitive state rather than a divine purging. |
| `spell critical` (skill) | `cognitive fault` | The Psionicist identifies and exploits architectural weaknesses in an opponent's mental defenses — the same analytical method the Pale Ledger developed for memory extraction, applied to spell delivery. |
| `night vision` | `peripheral extension` | Sensory augmentation through cognitive enhancement of peripheral processing — reframes a generic utility spell as a deliberate cognitive technique. |

**Stance — Tier D:**

The `Psionicist` stance is simply the class name — the weakest possible naming choice. A level-60 Psionicist who has reached their mortal class peak is not "a Psionicist" — they are a practitioner who has fully internalized the Assertion Wing's combat doctrine, the Pale Ledger's analytical framework, and the defensive architecture of the Kel'Shadra resistance tradition. The defining stance of this integration is the one that makes them most dangerous in extended engagement: the capacity to hold continuous cognitive pressure.

Proposed: `Psionicist` → **`Long Hold`**

This also serves as an in-game hint toward the technique's existence and toward the Egomancer path, where the Long Hold's full capabilities are developed.

### Proposed Changes Summary

| Change | Tier | Type | Notes |
|---|---|---|---|
| Replace `purify` | A | Spell swap | → `long hold`: the Assertion Wing's sustained cognitive pressure technique; low DoT or progressive debuff that builds over rounds; fills Tier C gap |
| Replace `sense evil` | A | Spell swap | → `pale imprint`: a Pale Ledger cognitive mark — tracks target, provides ongoing information; fills Tier C gap |
| Replace `mindflame` | A | Spell swap | → `recurrence loop`: Void Citadel recurrence technology — confusion/disorientation attack through memory flooding; fills Tier C gap |
| Rename `see magic` | D | Rename | → `architecture scan` |
| Rename `fighting trance` | D | Rename | → `assertion form` |
| Rename `purge` | D | Rename | → `cognitive flush` |
| Rename `spell critical` | D | Rename | → `cognitive fault` |
| Rename `night vision` | D | Rename | → `peripheral extension` |
| Rename stance `Psionicist` | D | Stance rename | → `Long Hold` |

As with Cleric, the three Tier A removals simultaneously resolve three Tier C gaps. The replacement spells don't need new mechanical functions — they occupy the same mechanical roles as the removed spells (sustained DoT, tracking mark, confusion attack) with names and flavor that connect to the actual tradition.

---
## Section V: Warden

### Tradition Summary

Wardens descend from the **road warden companies** of the Keeper Era — professional escorts chartered to protect desert caravans. Their founding doctrine comes from **Warden-Sergeant Caleph Dunmar**, who held the **Seven Shade Arches** waystation for 43 days against Ashbound siege, protecting a water convoy. Dunmar's doctrine: fight near shade infrastructure, target logistics first, hold through attrition. The **Last Watch Release** — ordered all-in assault when position is lost but objectives can still be preserved through aggressive cover — is the doctrine that `berserk` represents.

The modern tradition is defined by the tension between the **Dunmar School** (doctrine and discipline: Dunmar's example as the model) and the **Registry School** (institutional obedience: the requisition office system and Midgaard's Guard Command as the legitimate authority). Wardens hold the **Pale Line** around the Thornwood perimeter unofficially — a self-assigned obligation to contain an expanding necrotic entity that formal governance won't acknowledge.

The previous lore-skill proposal already renamed the two most significant Warden skills (`seven shade hold`, `Dunmar's Watch`) and left `berserk` intentionally unchanged as "Last Watch Release" is doctrine, not a rename. This section covers what remains.

### Skill Audit

**Lore-grounded (no change needed):**

| Skill | Lore connection |
|---|---|
| `seven shade hold` | Named after Dunmar's Forty-Three Days — already implemented |
| `berserk` | Last Watch Release doctrine — left intentionally as `berserk` per prior proposal |
| `enhanced damage` | Decisive positional striking — Dunmar doctrine |
| `field patch` | Road warden practical medicine — essential for caravan escort, excellently named |
| `parry`, `bash`, `cleave`, `rend` | Physical combat doctrine, fine |
| `rescue` | Protective doctrine — road wardens guarding charges |
| `shield block` | Holding ground, fine |
| `disarm`, `nodisarm` | Positional control |
| `read intent` | Situational awareness — fits Dunmar's comprehensive attention doctrine |
| `cleave`, `rend` | Decisive strikes, fine |
| `punch` | Foundational, fine |

**Tier A — Lore-hostile:**

None. The Warden skill list has no active contradictions — unlike Cleric and Psionicist, the physical combat tradition is broad enough that generic combat names don't contradict anything documented.

**Tier B — Lore-neutral:**

| Skill | Level | Problem |
|---|---|---|
| `warcry` | 10 | The road warden tradition is characterized by positional discipline and civic order, not emotional battle shouts. Wardens don't exhort — they communicate doctrine. The Dunmar School transmits tactical signals; the Registry School transmits chain-of-command orders. A "warcry" belongs to berserker or barbarian traditions entirely absent from warden lore. |
| `headbutt` | 6 | Generic brawling. The warden tradition's early techniques should reflect escort and caravan-defense doctrine, not street fighting. "Headbutt" carries no connection to Dunmar's discipline-centered approach. |
| `knee` | 8 | Same as headbutt. A knee strike is a practical move but the name places it firmly in a brawling tradition rather than a military doctrine framework. |
| `taunt` | 12 | Generic provocation. Wardens do engage in deliberate confrontation — but through institutional authority, not personal taunting. The warden tradition's version of drawing enemy attention is a formal challenge of position and authority, not a personal insult. |
| `dirt` | 35 | A blinding attack with no named tradition behind it. Wardens fought in the desert; the Ashfall Monsoon events of the Ash-and-Iron Decades filled the Cinderteeth corridor with volcanic particulate that warden companies learned to use tactically. The action is right; the name is generic. |

**Tier C — Missing lore:**

| Missing element | Lore basis |
|---|---|
| Pale Line doctrine | The Warden tradition's most distinctive contemporary expression — holding the Thornwood perimeter without official sanction, against a threat formal governance won't acknowledge — has no mechanical representation. A high-level Warden skill reflecting this self-assigned obligation would give the tradition's modern evolution a place in the skill list. |
| Dunmar/Registry tension | The tradition's internal political split (Dunmar School doctrine vs. Registry School institutional obedience) has no mechanical expression. Other classes have skills reflecting their internal tensions; Wardens don't. This is lower priority but notable. |

**Tier D — Rename opportunities:**

| Current name | Proposed name | Lore basis |
|---|---|---|
| `warcry` | `dunmar's call` | The tactical signal Dunmar used to coordinate his hold at the Seven Shade Arches — a specific doctrinal communication, not an emotional shout. Wardens trained in the Dunmar School learn this as the signal for coordinated positional assault. |
| `headbutt` | `checkpoint break` | Road warden doctrine for forced entry — breaking through a checkpoint or perimeter that's been illegitimately blocked. Reflects the warden's authority-enforcement function rather than street brawling. |
| `knee` | `road sweep` | A takedown technique developed for controlling individuals who resist on narrow caravan roads where full-swing combat isn't possible. Names the tactical context. |
| `taunt` | `charter challenge` | The formal warden institutional challenge — invoking the company's charter authority to demand an opponent's attention and response. Dunmar School vs. Registry School practitioners mean different things by it, but both use it. |
| `dirt` | `ashfall strike` | Named for the volcanic ash particulate from the Cinderteeth Mountains' Ashfall Monsoon events that warden companies operating the Saltglass Reach corridor learned to weaponize as a tactical blinding technique. |

### Proposed Changes Summary

| Change | Tier | Type | Notes |
|---|---|---|---|
| Rename `warcry` | D | Rename | → `dunmar's call` |
| Rename `headbutt` | D | Rename | → `checkpoint break` |
| Rename `knee` | D | Rename | → `road sweep` |
| Rename `taunt` | D | Rename | → `charter challenge` |
| Rename `dirt` | D | Rename | → `ashfall strike` |

The Warden section is all Tier D — no Tier A emergencies, no immediate Tier C gaps that require new skills (the Pale Line is a compelling addition but lower priority). The five renames turn a skill list that currently reads like generic fighter vocabulary into one where almost every skill name traces to a specific event, doctrine, or historical context in the warden tradition.

The Warden is already the best-served mortal class from the previous rename work. This section completes the job.

---
## Section VI: Cipher

### Tradition Summary

The Cipher tradition crystallized from three post-collapse lineages operating in the breakdown of the Keeper Era's charter road network after the Ninth Meridian Catastrophe: **route guides** (who knew pre-collapse paths and sold that knowledge to whoever could pay), **fence operators** (who moved contraband through gaps in registry systems), and **private intelligence dealers** (who forged documents, manipulated official records, and sold information about institutional vulnerabilities). The unifying insight: every system of control creates its own perimeter, and every perimeter has a gap.

The tradition's contemporary function includes maintaining the **Void Citadel monitoring network** — quarterly assessments of the abandoned institution's still-operational recurrence apparatus that no legitimate authority has claimed responsibility for. Ciphers are the only tradition actively watching that machinery.

The previous rename work implemented `gap transit` (from `shadow step`) and `reach silence` (from `garrote`), the two most significant lore-grounded names. This section covers what remains.

### Skill Audit

**Lore-grounded (no change needed):**

| Skill | Lore connection |
|---|---|
| `gap transit` | The defining Cipher metaphor — moving through the seam between control systems — already renamed |
| `reach silence` | Saltglass Reach corridor technique — already renamed |
| `backstab` | Assassination, foundational to the intelligence dealer lineage |
| `circle` | Combat maneuvering, fine |
| `sneak`, `hide`, `conceal` | Route guide stealth in territory with contested authority — core skills |
| `dodge` | Evasion, fine |
| `steal` | Fence operator heritage — the most direct expression of the contraband lineage |
| `peek` | Intelligence dealer surveillance, fine |
| `find doors` | Route guide knowledge — knowing where passages are that official maps don't show |
| `climb` | Route guide terrain navigation |
| `pick lock` | Fence operator access — getting through official barriers without official credentials |
| `appraise` | Fence operator valuation — knowing what something is worth in the shadow economy |
| `read intent` | Intelligence dealer threat assessment |
| `disarm`, `nodisarm`, `parry` | Combat skills, fine |
| `hunt` | Route guide tracking |
| `enhanced critical` | System-modeling methodology — identifying fault lines through analysis |

**Tier A — Lore-hostile:**

None. The Cipher skill list has no active contradictions. The tradition is broad enough (survival, stealth, information, combat) that generic names don't contradict documented lore.

**Tier B — Lore-neutral:**

| Skill | Level | Problem |
|---|---|---|
| `detox` | 27 | The name is clinical and modern. Route guides operating the Saltglass Reach had documented practical knowledge of desert remedies — the harsh terrain with its glass particulate, heat, and scarce water produced specific toxic exposure conditions that experienced guides knew how to address. The *function* is correct (route guides would carry antidotes); the *name* carries no connection to that context. |
| `mark target` | 15 | Generic targeting. Intelligence dealers didn't just "mark" targets — they conducted assessments, flagged assets, and built dossiers. The term should reflect the intelligence-dealer methodology rather than generic tactical vocabulary. |
| `feign death` | 36 | A useful deception technique with no named tradition behind it. The Cipher tradition has documented dissolution protocols — ways of erasing one's apparent presence from a situation entirely. The lore's Night Market Dissolution (the collapse of the post-collapse informal economy's largest trading hub) produced Cipher practitioners who became expert at disappearing from official awareness before enforcement arrived. |
| `set trap` | 35 | Functional but generic. Ciphers set traps at system boundary points — the seams between control systems — not just anywhere. The name should reflect the tradition's understanding of where traps work best. |

**Tier C — Missing lore:**

| Missing element | Lore basis |
|---|---|
| Document manipulation | The fence operator and intelligence dealer lineages are defined by their ability to forge documents, manipulate official records, and create false credentials. This is one of the Cipher tradition's most documented capabilities and has no mechanical representation. |
| Void Citadel monitoring | The Cipher tradition's contemporary institutional function — maintaining quarterly monitoring assessments of the abandoned Void Citadel's still-operational recurrence technology — has no mechanical expression anywhere in the skill list. |

**Tier D — Rename opportunities:**

| Current name | Proposed name | Lore basis |
|---|---|---|
| `detox` | `reach remedy` | Named for the Saltglass Reach — the three-route corridor where the tradition developed. Route guides carried remedies for the Reach's specific hazards (glass particulate inhalation, heat toxicity, contaminated water sources). "Reach remedy" names the tradition and the context. |
| `mark target` | `asset flag` | Intelligence dealer terminology — flagging a target as an asset to be tracked, not just a combat mark. Implies the dossier-building methodology that defines the lineage. |
| `feign death` | `dissolution protocol` | Named for the Night Market Dissolution and the broader Cipher tradition of erasing one's presence from official awareness before enforcement arrives. The "protocol" framing emphasizes that this is a practiced, documented procedure rather than improvisation. |
| `set trap` | `seam snare` | A trap set at the boundary between control systems — the Cipher's doctrine is that system junctions are both the best escape routes and the best ambush points. "Seam snare" names the tactical theory. |
| `conceal` | `gap hold` | Holding position in the gap between two control system perimeters — the specific concealment technique route guides developed for staying undetected in contested jurisdiction territory. |

### Proposed Changes Summary

| Change | Tier | Type | Notes |
|---|---|---|---|
| Rename `detox` | D | Rename | → `reach remedy` |
| Rename `mark target` | D | Rename | → `asset flag` |
| Rename `feign death` | D | Rename | → `dissolution protocol` |
| Rename `set trap` | D | Rename | → `seam snare` |
| Rename `conceal` | D | Rename | → `gap hold` |

The Cipher section is all Tier D, like Warden. The two Tier C gaps (document manipulation, Void Citadel monitoring) are noted but not proposed as new skills in this audit — they would require new mechanics and are better handled as a separate targeted proposal when the right mechanical form is determined.

The five renames complete the work the previous proposal started. After implementation, the Cipher skill list will read almost entirely in the tradition's own vocabulary: gap transit, reach silence, reach remedy, asset flag, dissolution protocol, seam snare, gap hold — names that describe what Ciphers are and where they came from.

---
## Section VII: Pugilist

### Tradition Summary

The Pugilist tradition developed among cistern laborers, caravan porters, and pyramid construction workers during the Solar Court era. It has no founding text, no academy, and no origin date — body knowledge transmitted through direct demonstration, surviving every institutional collapse that destroyed more formally documented traditions. The filtering process that shaped the tradition is explicit in the lore: only techniques that worked when exhausted, in poor light, in cramped quarters, against serious opponents survived into consistent use.

The tradition has no equipment doctrine by design. The lore is unambiguous: techniques developed "with limited equipment (body only)." The cramped cistern environments where the tradition emerged made weapon use impractical; the construction labor context made improvised-barrier redirection (using forearms, shoulders) a survival skill. The tradition's relationship with shields is documented specifically in pugilist_lore.md as using "any available barrier — a shield when one is held, a forearm or shoulder when it isn't" — meaning the tradition uses shields opportunistically, not as a dedicated discipline.

The **Interior Forms** are the Monk tradition's parallel physical discipline — a separate lineage the Pugilist encounters only through the Monk remort path. The Pugilist proper operates below that encounter: they have body knowledge, not the Interior Forms' breath-and-posture methodology.

### Skill Audit

**Lore-grounded (no change needed):**

| Skill | Lore connection |
|---|---|
| `martial arts` | The tradition's primary combat framework |
| `counter` | The specific timing that emerges from absorbing punishment — lore describes this precisely as a developed reflex, not a taught technique |
| `iron skin` | Physical resilience from chronic labor stress and surface-impact absorption across extended experience |
| `roll with blow` | Moving with incoming force — lore explicitly documents this as older than any formal description of it |
| `applied understanding` | Already renamed from `pressure point` — the empirical knowledge of strike nodes |
| `grapple`, `choke hold`, `leg sweep` | Close-quarters cistern techniques — exactly the cramped-environment combat the tradition developed |
| `flurry` | The tradition's high-output striking capacity |
| `pummel` | Sustained striking, fine |
| `dodge` | Economy of movement through accumulated experience |
| `feint` | Practical deception, fine |
| `conditioning` | Physical substrate maintenance — the lore is explicit that conditioning is what makes other skills available rather than merely known |

**Tier A — Lore-hostile:**

| Skill | Level | Problem |
|---|---|---|
| `weapon mastery` | 15 | The Pugilist tradition is defined by "body only, limited equipment." The lore documents this as a *feature*, not a gap — techniques that survived the filtering process did so precisely because they worked without equipment. Weapon mastery as a Pugilist skill isn't just lore-neutral; it actively contradicts the tradition's documented identity. A Pugilist who trains in weapon mastery is departing from the tradition, not advancing in it. |

**Tier B — Lore-neutral:**

| Skill | Level | Problem |
|---|---|---|
| `shield block` | 18 | The pugilist_lore.md explicitly addresses shields: the technique uses "any available barrier — a shield when one is held, a forearm or shoulder when it isn't." This is documented as opportunistic barrier use, not a trained shield discipline. The skill being named `shield block` implies a dedicated shield practice that the lore says doesn't exist. The underlying technique — redirecting force through angles — is lore-grounded; the name isn't. |
| `mind over body` | 21 | Generic mind-body vocabulary. The Pugilist tradition has a specific framework for this relationship: the interior discipline that the lore describes as what allows technique to function under exhaustion and adverse conditions. The generic name misses the tradition's specific vocabulary. |
| `conditioning` | 5 | The lore's most explicit statement about conditioning is that it comes from the cistern labor context — the chronic physical demands of Solar Court era construction work. The name is functional but carries no connection to the tradition's documented origin. |
| `applied understanding` | Already renamed | Confirmed lore-grounded. Noted for completeness. |
| `dirt` | 47 | A blinding attack at level 47 for a tradition of disciplined precision. The Pugilist tradition developed in cramped cisterns and harbor districts — environments where throwing debris could be a survival technique. But `dirt` is very late in the progression and the generic name suggests no connection to the tradition's specific environmental context. Lower priority given its high level. |

**Tier C — Missing lore:**

| Missing element | Lore basis |
|---|---|
| Deltari hydraulic heritage | The Pugilist tradition developed in cisterns built by **Deltari hydraulic engineers** — the non-human people who constructed the infrastructure where the tradition emerged. The lore documents an unresolved question about what obligation this derivation creates, and the Deltari Heritage Council's active petition to Midgaard's Guard Command. No skill acknowledges this connection. |
| Labor transmission | The tradition has no institutional transmission mechanism — it spreads through demonstration, worker to apprentice, in labor contexts. A skill reflecting this origin (perhaps a buff or training mechanic) would give the tradition's distinctive non-institutional character mechanical expression. |

**Tier D — Rename opportunities:**

| Current name | Proposed name | Lore basis |
|---|---|---|
| `weapon mastery` *(Tier A)* | `open hand mastery` | Replaces the lore-hostile name while preserving the mechanical role. The Pugilist's "mastery" is of their body as a weapon — the same mechanical benefit applies, but the name now reflects what the tradition actually masters. Alternatively the slot could be repurposed for something more distinctly Pugilist. |
| `shield block` | `force redirect` | Names the actual technique the lore documents — redirecting incoming force through the body rather than blocking it directly. Works mechanically the same; no longer implies a dedicated shield discipline. |
| `mind over body` | `interior discipline` | The Pugilist's internal framework for sustaining technique under conditions of exhaustion and adverse environment. "Interior discipline" is the appropriate vocabulary for this tradition — not the Interior Forms (that's the Monk), but the same conceptual territory approached from the labor tradition's perspective. |
| `conditioning` | `cistern discipline` | Names the environmental context where the tradition developed. Cistern laborers' conditioning came from the specific physical demands of underground hydraulic work — weight-bearing, confined movement, sustained exertion without relief. "Cistern discipline" carries the origin. |
| `dirt` | `harbor dust` | Harbor district fighting pits — the Kowloon harbor context where the tradition developed its competitive form. Throwing dust/debris in pit context was a documented practical technique, not a Warden ashfall maneuver. Different tradition, different name. |

**Stance — Tier D:**

The `Pugilist` stance is simply the class name. The previous rename proposal deferred this because no lore-grounded alternative had been developed. The pugilist_lore.md is explicit about what the stance represents: "every movement does exactly what it needs to do and nothing more" — the tradition calls this **economy of motion**, and the lore specifically names the Pugilist stance as the moment this economy becomes the practitioner's natural expression.

The lore already provides the name.

Proposed: `Pugilist` → **`Economy of Motion`**

This also creates a clean hierarchy with the Monk remort:
- **Pugilist** stance: `Economy of Motion` — the tradition's body knowledge fully expressing itself
- **Monk** stance: `Deepest Form` — the Interior Forms' integration producing something beyond economy

### Proposed Changes Summary

| Change | Tier | Type | Notes |
|---|---|---|---|
| Replace `weapon mastery` | A | Rename / repurpose | → `open hand mastery`: same mechanical role, lore-consistent name; or repurpose slot for a distinctly Pugilist technique |
| Rename `shield block` | B | Rename | → `force redirect` |
| Rename `mind over body` | B | Rename | → `interior discipline` |
| Rename `conditioning` | B | Rename | → `cistern discipline` |
| Rename `dirt` | B | Rename | → `harbor dust` |
| Rename stance `Pugilist` | D | Stance rename | → `Economy of Motion` |

The `weapon mastery` removal is the only Tier A finding in the Pugilist section, and it's a simple rename rather than a spell replacement — the mechanical function is fine, only the name contradicts the tradition. Renaming to `open hand mastery` resolves the contradiction with minimal implementation cost.

---
## Section VIII: Consolidated Summary and Implementation Plan

### All Proposed Changes by Class

#### Magi

| Change | Tier | Current | Proposed |
|---|---|---|---|
| Spell swap | B | `faerie fire` (lvl 3) | `prism mark` |
| Spell swap | B | `color spray` (lvl 7) | `crystal burst` |
| Spell removal / swap | B | `ventriloquate` (lvl 3) | remove or → `ledger inscription` |
| Rename | D | `acid blast` | `dissolution` |
| Rename | D | `mystical potency` (skill) | `compact amplification` |
| Stance rename | D | `Magi` | `Ember Protocol` |

#### Cleric

| Change | Tier | Current | Proposed |
|---|---|---|---|
| Spell swap | **A** | `call lightning` (lvl 36) | `shard ward` |
| Spell swap | **A** | `control weather` (lvl 79) | `seal calculus` |
| Spell swap | **A** | `earthquake` (lvl 78) | `jackal's verdict` |
| Spell swap | **A** | `faerie fog` (lvl 14) | `ledger mark` |
| Rename | D | `cause serious` | `solar censure` |
| Rename | D | `cause critical` | `tribunal's hand` |
| Rename | D | `prayer` | `solar invocation` |
| Rename | D | `holy word` | `disk's word` |
| Rename | D | `spell critical damage` (skill) | `solar precision` |
| Stance rename | D | `Healer` | `Solar Office` |

#### Psionicist

| Change | Tier | Current | Proposed |
|---|---|---|---|
| Spell swap | **A** | `purify` (lvl 19) | `long hold` |
| Spell swap | **A** | `sense evil` (lvl 10) | `pale imprint` |
| Spell swap | **A** | `mindflame` (lvl 35) | `recurrence loop` |
| Rename | D | `see magic` | `architecture scan` |
| Rename | D | `fighting trance` | `assertion form` |
| Rename | D | `purge` | `cognitive flush` |
| Rename | D | `spell critical` (skill) | `cognitive fault` |
| Rename | D | `night vision` | `peripheral extension` |
| Stance rename | D | `Psionicist` | `Long Hold` |

#### Warden

| Change | Tier | Current | Proposed |
|---|---|---|---|
| Rename | D | `warcry` | `dunmar's call` |
| Rename | D | `headbutt` | `checkpoint break` |
| Rename | D | `knee` | `road sweep` |
| Rename | D | `taunt` | `charter challenge` |
| Rename | D | `dirt` | `ashfall strike` |

#### Cipher

| Change | Tier | Current | Proposed |
|---|---|---|---|
| Rename | D | `detox` | `reach remedy` |
| Rename | D | `mark target` | `asset flag` |
| Rename | D | `feign death` | `dissolution protocol` |
| Rename | D | `set trap` | `seam snare` |
| Rename | D | `conceal` | `gap hold` |

#### Pugilist

| Change | Tier | Current | Proposed |
|---|---|---|---|
| Rename | **A** | `weapon mastery` | `open hand mastery` |
| Rename | B | `shield block` | `force redirect` |
| Rename | B | `mind over body` | `interior discipline` |
| Rename | B | `conditioning` | `cistern discipline` |
| Rename | B | `dirt` | `harbor dust` |
| Stance rename | D | `Pugilist` | `Economy of Motion` |

---

### Priority Order

**Immediate (Tier A — lore-hostile):** Seven changes across three classes.

1. Cleric: remove `call lightning`, `control weather`, `earthquake`, `faerie fog` — replace with `shard ward`, `seal calculus`, `jackal's verdict`, `ledger mark`
2. Psionicist: remove `purify`, `sense evil`, `mindflame` — replace with `long hold`, `pale imprint`, `recurrence loop`
3. Pugilist: rename `weapon mastery` → `open hand mastery`

**Medium (Tier B/C — lore-neutral and missing lore):** Renames across all six classes, plus the three new spells for Magi (Prism Thesis representation).

**Lower (Tier D — rename opportunities):** The stance renames and cosmetic Tier D renames. Lowest implementation risk; all are name-only changes in a single table entry.

---

### Implementation Notes

#### Spell/Skill Renames (Tier D)

All pure renames follow the pattern established in `lore-skill-stance-renames.md`:

1. Update the name string in `src/spells/spell_table_data.c` or `src/skills/skill_table_data.c`
2. Update the command string in `src/interp.c` if applicable
3. Add a migration alias in `src/save/save_players.c` (and `save_objects.c`, `save_mobs.c`) so old player files load correctly
4. Update help files if they exist

#### Stance Renames (Tier D)

Update the display name string in `src/stance.c`. Stances are not saved by name in player files — no migration needed.

Six stance renames proposed:

| Class | Current | Proposed |
|---|---|---|
| Magi | `Magi` | `Ember Protocol` |
| Cleric | `Healer` | `Solar Office` |
| Psionicist | `Psionicist` | `Long Hold` |
| Pugilist | `Pugilist` | `Economy of Motion` |
| Warden | `Dunmar's Watch` | *(already done)* |
| Cipher | `Gap Intuition` | *(already done)* |

#### Spell Swaps (Tier A and B)

For spells being replaced (Cleric's four, Psionicist's three, Magi's two-to-three):

The simplest implementation for each swap:
- **Same mechanical function, new name:** Update the name string only. The spell function, level, and mechanical effect stay identical. This is sufficient for cases where the replacement spell is mechanically equivalent to what was removed (e.g., `ledger mark` as a renamed `faerie fog`, `shard ward` as a renamed version of `call lightning`'s mechanical role).
- **New mechanical function:** Requires a new spell implementation file in `src/spells/`. This applies to `long hold` (which needs sustained DoT behavior distinct from existing spells) and potentially `recurrence loop`. These are larger implementations and should be scoped separately.

For spells that can be simple renames of existing mechanics:

| Removed | Replacement | Mechanical equivalent |
|---|---|---|
| `faerie fog` | `ledger mark` | Same as `faerie fire` — reveal + debuff, Cleric context |
| `call lightning` | `shard ward` | Ward/defensive area spell |
| `control weather` | `seal calculus` | Geometric binding — could function as existing ward |
| `earthquake` | `jackal's verdict` | Powerful curse/judgment |
| `sense evil` | `pale imprint` | Tracking mark — detect-equivalent |
| `mindflame` | `recurrence loop` | Disorientation/confusion attack |
| `purify` | `long hold` | *Requires new implementation* — sustained cognitive pressure |
| `faerie fire` | `prism mark` | Direct rename with Prism Thesis flavor |
| `color spray` | `crystal burst` | Multi-target damage, Prism Thesis |

#### New Implementations Required

Two spells require genuinely new mechanics rather than simple renames:

**`long hold`** (Psionicist, replacing `purify`): A sustained cognitive pressure effect — mechanically distinct from existing DoT spells in that it should build over rounds rather than dealing consistent damage, reflecting the Long Hold's documented property of operating below the threshold of obvious disruption. Level 19 placement appropriate.

**`shard ward`** (Cleric, replacing `call lightning`): The containment doctrine's signature spell — should have specific bonus effectiveness against undead and anti-light entities, reflecting the Solar Court's purpose. Could be implemented as sanctuary-adjacent with offensive properties vs. specific enemy types. Level 36 placement appropriate.

These two are the only changes requiring more than table-entry and interp-entry updates.

---

### Files Affected

| File | Changes |
|---|---|
| `src/spells/spell_table_data.c` | Name strings for all spell renames and swaps |
| `src/skills/skill_table_data.c` | Name strings for all skill renames |
| `src/interp.c` | Command strings for renamed skills |
| `src/stance.c` | Display name strings for four stance renames |
| `src/save/save_players.c` | Migration alias table additions |
| `src/save/save_objects.c` | Migration alias table (precautionary) |
| `src/save/save_mobs.c` | Migration alias table (precautionary) |
| `src/spells/spell_long_hold.c` | *New file* — Long Hold implementation |
| `src/spells/spell_shard_ward.c` | *New file* — Shard Ward implementation |
| `help/` | Help file updates for renamed skills/spells |

---

### What This Audit Does Not Propose

- Changes to remort or adept classes (out of scope for this audit)
- New skills for Tier C gaps (Cipher document manipulation, Void Citadel monitoring, Pugilist Deltari heritage, Warden Pale Line) — these are documented as missing but require separate targeted proposals with specific mechanical designs
- Changes to skill levels (only names are proposed, not rebalancing)
- Changes to mechanical effects of Tier D renames (all Tier D changes are name-only)

