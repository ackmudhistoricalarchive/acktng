-- Sentinel Remort/Adept Classes: Database changes
-- Help entries, shelp entries for new classes and skills

-- =====================================================================
-- CLASS HELP ENTRIES (player-facing)
-- =====================================================================

INSERT INTO help_entries (filename, level, keywords, body) VALUES
('class_sentinel', 0, 'sentinel',
'@@lSentinel@@N\n\nThe Sentinel tradition descends from the Jackal Tribunal''s sentence enforcers, practitioners who carried out verdicts through disciplined physical intervention rather than priestly magic.\n\n@@ePrime Stat@@N: WIS\n@@eHP/Level@@N:  6\n@@eMana/Level@@N: 1\n@@eTier@@N: Mortal\n\n@@lCombat Identity@@N\n\nThe Sentinel is a WIS-based melee class whose power increases the longer they fight the same target. Their class resource, @@eTestimony@@N, accumulates marks on a single opponent through successful defensive reading. When enough marks are built, the Sentinel spends them to deliver a @@eVerdict@@N.\n\n@@lKey Skills@@N\n  @@eread opponent@@N  - Actively study the target, gaining instant testimony\n  @@everdict@@N        - Consume testimony marks for a powerful scaled strike\n  @@eninth descent@@N  - Double testimony accumulation for a burst window\n  @@econdemn@@N        - WIS-scaled debuff requiring 3+ testimony\n  @@eseal testimony@@N - Protect accumulated marks from disruption\n\n@@lProgression@@N\n  Remort: @@eJusticar@@N (offense) or @@eArbiter@@N (defense/utility)\n  Adept:  @@eInquisitor@@N (Justicar + Arbiter synthesis)\n\nSee also: JUSTICAR, ARBITER, INQUISITOR, TESTIMONY, VERDICT');

INSERT INTO help_entries (filename, level, keywords, body) VALUES
('class_justicar', 0, 'justicar',
'@@lJusticar@@N\n\nThe Justicars descend from the Jackal Tribunal''s sentence execution corps, the practitioners who carried out the three sentence classes (Interment, Service, Correction) through physical enforcement.\n\n@@ePrime Stat@@N: WIS\n@@eHP/Level@@N:  12\n@@eMana/Level@@N: 2\n@@eTier@@N: Remort (requires Sentinel)\n\n@@lCombat Identity@@N\n\nOffense-focused WIS melee. Cycles verdicts faster through @@eSeverity Escalation@@N (reduced mark thresholds) and @@eSecond Hearing@@N (retained marks after spending). @@eExecutioner''s Strike@@N provides sustained WIS-scaled damage between verdict cycles.\n\n@@lKey Skills@@N\n  @@eformal sentencing@@N    - Mark target for +25% verdict damage\n  @@eexecutioner''s strike@@N - WIS-scaled attack, bonus vs sentenced targets\n  @@esecond hearing@@N       - Retain 2 testimony after verdict (passive)\n  @@eseverity escalation@@N  - Reduce all verdict thresholds by 1 (passive)\n  @@ewrit of execution@@N   - Ultimate single-target burst\n  @@eread weakness@@N        - Self-buff for sustained hit/damage bonus\n  @@einevitable verdict@@N   - Faster testimony accumulation (passive)\n\n@@lStance@@N: Sentencing Form\n\nSee also: SENTINEL, ARBITER, INQUISITOR');

INSERT INTO help_entries (filename, level, keywords, body) VALUES
('class_arbiter', 0, 'arbiter',
'@@lArbiter@@N\n\nThe Arbiters descend from the Jackal Tribunal''s record-keeping and process guardian corps. The Tribunal''s integrity depended on counteracting the Black Sun Shard''s memory-corruption through meticulous cross-referencing of testimonial records.\n\n@@ePrime Stat@@N: WIS\n@@eHP/Level@@N:  10\n@@eMana/Level@@N: 4\n@@eTier@@N: Remort (requires Sentinel)\n\n@@lCombat Identity@@N\n\nDefense and utility focused WIS melee. Excels in group play through party-wide defensive buffs, enemy suppression, and the @@eContempt of Court@@N passive that builds testimony when the target attacks allies.\n\n@@lKey Skills@@N\n  @@eprocedural authority@@N  - Party buff: saves and dodge bonus\n  @@esustained objection@@N   - Suppress target''s skills for 2 rounds\n  @@ecross-examination@@N     - Chance to delay attacker''s skills (passive)\n  @@ejudicial immunity@@N     - Self-buff: immune to fear/charm/sleep\n  @@eappellate review@@N      - Remove one negative affect from an ally\n  @@esuppression order@@N     - Reduce target''s damage output\n  @@econtempt of court@@N     - Gain testimony when target attacks allies (passive)\n\n@@lStance@@N: Record-Keeper''s Guard\n\nSee also: SENTINEL, JUSTICAR, INQUISITOR');

INSERT INTO help_entries (filename, level, keywords, body) VALUES
('class_inquisitor', 0, 'inquisitor',
'@@lInquisitor@@N\n\nThe Inquisitor represents the synthesis of both Tribunal offices in a single practitioner: sentence execution (Justicar) and process enforcement (Arbiter).\n\n@@ePrime Stat@@N: WIS\n@@eHP/Level@@N:  20\n@@eMana/Level@@N: 6\n@@eTier@@N: Adept (requires Justicar + Arbiter)\n\n@@lCombat Identity@@N\n\nComplete WIS melee synthesis. Formal sentencing and suppression orders stack on the same target (@@eFull Tribunal@@N), @@eInquisition@@N amplifies all damage and doubles testimony gain, and @@eAbsolute Verdict@@N delivers the irrevocable sentence.\n\n@@lKey Skills@@N\n  @@efull tribunal@@N        - Stack sentencing + suppression; retain 3 marks\n  @@einquisition@@N          - +15% damage to target, doubled testimony gain\n  @@eabsolute verdict@@N     - 12x damage, 2-round stun, strip all buffs\n  @@eprecedent@@N            - +3 testimony on next target after verdict kill\n  @@esovereign authority@@N  - Party-wide hit/damage/saves buff\n  @@eseal of the tribunal@@N - Testimony cannot be lost, verdict cooldown = 1\n\n@@lStance@@N: Full Tribunal\n\nSee also: SENTINEL, JUSTICAR, ARBITER');

INSERT INTO help_entries (filename, level, keywords, body) VALUES
('class_druid', 0, 'druid',
'@@lDruid@@N\n\nThe druidic tradition predates the Wizard''s Keep, the Solar Court, and the Violet Compact. Druids draw power from the living substrate: forests, stone, water, root networks. The price is paid in blood. Druids cast spells using their own @@eHP@@N, not mana.\n\n@@ePrime Stat@@N: CON\n@@eHP/Level@@N:  5\n@@eMana/Level@@N: 0\n@@eTier@@N: Mortal\n\n@@lCombat Identity@@N\n\nHP caster with a unique risk curve. Every spell increases @@eOvergrowth@@N, which amplifies spell power (+3% per point) but also increases HP cost (+2% per point). The Druid grows more dangerous the longer a fight continues, but risks killing themselves if they push too far.\n\n@@lStance@@N: Root Posture\n\n@@lProgression@@N\n  Remort: @@eThornwarden@@N (martial enforcement) or @@eWildspeaker@@N (healing/support)\n  Adept:  @@eHierophant@@N (Thornwarden + Wildspeaker synthesis)\n\nSee also: THORNWARDEN, WILDSPEAKER, HIEROPHANT, OVERGROWTH');

INSERT INTO help_entries (filename, level, keywords, body) VALUES
('class_thornwarden', 0, 'thornwarden',
'@@lThornwarden@@N\n\nThe Thornwardens descend from the Everkeeper tradition: ironpine grove maintainers who declared territorial boundaries and enforced them through direct physical intervention.\n\n@@ePrime Stat@@N: CON\n@@eHP/Level@@N:  10\n@@eMana/Level@@N: 0\n@@eTier@@N: Remort (requires Druid)\n\n@@lCombat Identity@@N\n\nMartial enforcement. Specializes in damage-over-time effects, self-buffs that punish attackers, and area denial. Holds a position and makes enemies pay for every attack.\n\n@@lKey Abilities@@N\n  @@ecreeping rot@@N, @@ebark of thorns@@N, @@eironwood skin@@N, @@ewall of thorns@@N\n  @@eroot retaliation@@N, @@eblighted ground@@N, @@ethorn strike@@N\n\n@@lStance@@N: Thorn Boundary\n\nSee also: DRUID, WILDSPEAKER, HIEROPHANT, OVERGROWTH');

INSERT INTO help_entries (filename, level, keywords, body) VALUES
('class_wildspeaker', 0, 'wildspeaker',
'@@lWildspeaker@@N\n\nThe Wildspeakers descend from the Withered Depths lineage: practitioners who achieved deep communion with the ecological network, reading biological signals across species boundaries.\n\n@@ePrime Stat@@N: CON\n@@eHP/Level@@N:  8\n@@eMana/Level@@N: 0\n@@eTier@@N: Remort (requires Druid)\n\n@@lCombat Identity@@N\n\nHealing and support. Strong heals, group buffs, and battlefield control. Where the Thornwarden holds a position, the Wildspeaker sustains the party.\n\n@@lKey Abilities@@N\n  @@ecanopy shelter@@N, @@everdant surge@@N, @@eprimal restoration@@N\n  @@eregenerative spores@@N, @@enature''s bounty@@N, @@esap transfusion@@N\n\n@@lStance@@N: Wild Resonance\n\nSee also: DRUID, THORNWARDEN, HIEROPHANT, OVERGROWTH');

INSERT INTO help_entries (filename, level, keywords, body) VALUES
('class_hierophant', 0, 'hierophant',
'@@lHierophant@@N\n\nThe Hierophant has mastered both the Thornwarden''s boundary enforcement and the Wildspeaker''s ecological communion. Both operate simultaneously: the substrate flows through them as a continuous cycle of growth, decay, and renewal.\n\n@@ePrime Stat@@N: CON\n@@eHP/Level@@N:  16\n@@eMana/Level@@N: 0\n@@eTier@@N: Adept (requires Thornwarden + Wildspeaker)\n\n@@lCombat Identity@@N\n\nComplete substrate synthesis. Signature mechanic: @@eControlled Burn@@N via @@eRootflare@@N (burst damage spending Overgrowth) and @@eCycle of Renewal@@N (burst group heal spending Overgrowth).\n\n@@lKey Abilities@@N\n  @@erootflare@@N, @@ecycle of renewal@@N, @@esubstrate communion@@N\n  @@edeathbloom@@N, @@eworld''s embrace@@N, @@esubstrate form@@N, @@eworldseed@@N\n\n@@lStance@@N: Substrate Ascendance\n\nSee also: DRUID, THORNWARDEN, WILDSPEAKER, OVERGROWTH');

-- =====================================================================
-- SKILL SHELP ENTRIES (staff-facing)
-- =====================================================================

-- Sentinel mortal skills
INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_verdict', -1, 'verdict',
'@@lVerdict@@N  @@eSentinel Level 20@@N\n\nSyntax: verdict\nType: Active (offensive)\nCost: 0 mana\nBeats: 24\nReqs: 1+ testimony\n\nConsume all testimony marks for a scaled melee strike.\n\n  1-2 marks: Rebuke (1.5x)\n  3-4 marks: Censure (2.5x, -2 HR 3 rds)\n  5-6 marks: Binding Verdict (4x, -15% speed 4 rds)\n  7-8 marks: Sealing Verdict (6x, skill silence 2 rds)\n  9 marks: Final Verdict (9x, 1 rd stun)\n\nFormula: base_damage * tier_multiplier + WIS * level / 80\n\nModified by: Severity Escalation, Second Hearing, Full Tribunal, Formal Sentencing (+25%)');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_read_opponent', -1, 'read opponent',
'@@lRead Opponent@@N  @@eSentinel Level 10@@N\n\nSyntax: read <target>\nCost: 8 mana\nBeats: 24\nCooldown: 8 rounds (7 at WIS 22+)\n\nInstantly grants +2 testimony marks. Modified by Inevitable Verdict (+3 instead of +2). Doubled by Ninth Descent.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_testimonial_guard', -1, 'testimonial guard',
'@@lTestimonial Guard@@N  @@eSentinel Level 12@@N\n\nType: Passive\n\nWIS-scaled resistance to fear, charm, and mental effects.\nResistance = WIS * 3 percent (66% at WIS 22).\nAt 5+ testimony: full fear immunity.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_binding_strike', -1, 'binding strike',
'@@lBinding Strike@@N  @@eSentinel Level 22@@N\n\nSyntax: bind <target>\nCost: 5 mana\nBeats: 18\nDuration: 3 rounds\n\n1.5x weapon damage, target dodge/parry -10% for 3 rounds. Does not generate testimony. Debuff does not stack with itself.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_measured_response', -1, 'measured response',
'@@lMeasured Response@@N  @@eSentinel Level 30@@N\n\nType: Passive\n\nAfter parry or dodge, (WIS * 2)% chance to counter at 75% weapon damage. Each proc generates +1 testimony (+2 during Ninth Descent). Does not fire on block.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_ninth_descent', -1, 'ninth descent',
'@@lNinth Descent@@N  @@eSentinel Level 35@@N\n\nSyntax: descent\nCost: 25 mana\nBeats: 12\nCooldown: 20 rounds\nDuration: 4 rounds\n\nDouble all testimony gain for 4 rounds. Stacks multiplicatively with Inevitable Verdict and Inquisition.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_condemn', -1, 'condemn',
'@@lCondemn@@N  @@eSentinel Level 45@@N\n\nSyntax: condemn <target>\nCost: 15 mana\nBeats: 24\nReqs: 3+ testimony\nDuration: 5 rounds\n\nWIS-scaled debuff: target saves -WIS/3, AC -WIS/2. Does not consume testimony.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_seal_testimony', -1, 'seal testimony',
'@@lSeal Testimony@@N  @@eSentinel Level 50@@N\n\nSyntax: seal\nCost: 10 mana\nBeats: 12\nDuration: 3 rounds\n\nStun reduces testimony by 1 (instead of half). Target-switch loses 2 marks (instead of all). Does not prevent loss from flee/death.');

-- Justicar skills
INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_formal_sentencing', -1, 'formal sentencing',
'@@lFormal Sentencing@@N  @@eJusticar Level 3@@N\n\nSyntax: sentence <target>\nCost: 15 mana\nBeats: 24\nReqs: 3+ testimony\nDuration: 6 rounds\n\nMark target as formally sentenced. +25% verdict damage against them. Does not consume testimony. One target at a time.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_executioners_strike', -1, 'executioner''s strike',
'@@lExecutioner''s Strike@@N  @@eJusticar Level 8@@N\n\nSyntax: execute <target>\nCost: 10 mana\nBeats: 18\n\nWIS-scaled melee attack: (level * 3 + WIS * 2) damage. +50% vs formally sentenced targets.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_second_hearing', -1, 'second hearing',
'@@lSecond Hearing@@N  @@eJusticar Level 15@@N\n\nType: Passive\n\nAfter delivering a verdict, retain 2 testimony marks instead of resetting to 0. Full Tribunal (Inquisitor) increases to 3.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_severity_escalation', -1, 'severity escalation',
'@@lSeverity Escalation@@N  @@eJusticar Level 20@@N\n\nType: Passive\n\nVerdict tier thresholds reduced by 1:\n  Rebuke: 1, Censure: 2, Binding: 4, Sealing: 6, Final: 8');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_writ_of_execution', -1, 'writ of execution',
'@@lWrit of Execution@@N  @@eJusticar Level 28@@N\n\nSyntax: writ\nCost: 30 mana\nBeats: 24\nReqs: 5+ testimony\nCooldown: 30 rounds\n\nConsume all testimony for verdict damage * 1.5, bypassing armor for 1 round.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_read_weakness', -1, 'read weakness',
'@@lRead Weakness@@N  @@eJusticar Level 35@@N\n\nSyntax: weakness\nCost: 12 mana\nBeats: 12\nDuration: 5 rounds\n\nAll melee attacks vs current target gain +WIS/4 hit, +WIS/3 damage. Stacks with formal sentencing.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_inevitable_verdict', -1, 'inevitable verdict',
'@@lInevitable Verdict@@N  @@eJusticar Level 42@@N\n\nType: Passive\n\nRead Opponent grants +3 (from +2). Passive tick fires every 2 rounds (from 3).');

-- Arbiter skills
INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_procedural_authority', -1, 'procedural authority',
'@@lProcedural Authority@@N  @@eArbiter Level 3@@N\n\nSyntax: authority\nCost: 20 mana\nBeats: 24\nDuration: 6 rounds\n\nGroup buff: +WIS/4 saves vs spells, +WIS/5 dodge chance.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_sustained_objection', -1, 'sustained objection',
'@@lSustained Objection@@N  @@eArbiter Level 8@@N\n\nSyntax: objection <target>\nCost: 12 mana\nBeats: 18\nReqs: 2+ testimony\nDuration: 2 rounds\n\nSuppress target''s skills for 2 rounds. Does not consume testimony.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_cross_examination', -1, 'cross-examination',
'@@lCross-Examination@@N  @@eArbiter Level 15@@N\n\nType: Passive\n\nOn parry/dodge, (WIS * 2)% chance to delay attacker''s next skill by 1 round. 44% at WIS 22.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_judicial_immunity', -1, 'judicial immunity',
'@@lJudicial Immunity@@N  @@eArbiter Level 20@@N\n\nSyntax: immunity\nCost: 20 mana\nBeats: 12\nDuration: 4 rounds\n\nImmune to fear, charm, sleep. Testimony cannot be reduced by stun or target switch.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_appellate_review', -1, 'appellate review',
'@@lAppellate Review@@N  @@eArbiter Level 28@@N\n\nSyntax: appeal <ally>\nCost: 25 mana\nBeats: 24\n\nRemove one negative affect from an ally (charm > curse > poison > blindness). Cannot self-target.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_suppression_order', -1, 'suppression order',
'@@lSuppression Order@@N  @@eArbiter Level 35@@N\n\nSyntax: suppress <target>\nCost: 20 mana\nBeats: 24\nReqs: 4+ testimony\nDuration: 5 rounds\n\nReduce target damage output by ~20%. Does not consume testimony.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_contempt_of_court', -1, 'contempt of court',
'@@lContempt of Court@@N  @@eArbiter Level 42@@N\n\nType: Passive\n\nWhen testimony target attacks a group member (not the Arbiter), gain +2 testimony. Doubled by Ninth Descent.');

-- Inquisitor skills
INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_full_tribunal', -1, 'full tribunal',
'@@lFull Tribunal@@N  @@eInquisitor Level 5@@N\n\nType: Passive\n\nFormal sentencing + suppression order stack on same target. Second Hearing retains 3 marks instead of 2.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_inquisition', -1, 'inquisition',
'@@lInquisition@@N  @@eInquisitor Level 12@@N\n\nSyntax: inquisition <target>\nCost: 35 mana\nBeats: 24\nReqs: 5+ testimony (consumes 3)\nCooldown: 40 rounds\nDuration: 8 rounds\n\nTarget takes +15% damage from all sources, saves -WIS/3. Inquisitor''s testimony gain doubled (stacks with Ninth Descent).');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_absolute_verdict', -1, 'absolute verdict',
'@@lAbsolute Verdict@@N  @@eInquisitor Level 20@@N\n\nSyntax: absolute\nCost: 40 mana\nBeats: 24\nReqs: 9 testimony\nCooldown: 50 rounds\n\n12x base damage + WIS bonus, 2-round stun, strips all beneficial affects.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_precedent', -1, 'precedent',
'@@lPrecedent@@N  @@eInquisitor Level 30@@N\n\nType: Passive\n\nAfter killing with a verdict, gain +3 testimony marks applied to next combat target. Stacks with Second Hearing retained marks.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_sovereign_authority', -1, 'sovereign authority',
'@@lSovereign Authority@@N  @@eInquisitor Level 40@@N\n\nSyntax: sovereign\nCost: 50 mana\nBeats: 24\nCooldown: 30 rounds\nDuration: 6 rounds\n\nGroup buff: +WIS/3 hit, +WIS/4 damage, +WIS/5 saves.');

INSERT INTO shelp_entries (filename, level, keywords, body) VALUES
('shelp_seal_of_the_tribunal', -1, 'seal of the tribunal',
'@@lSeal of the Tribunal@@N  @@eInquisitor Level 50@@N\n\nSyntax: seal\nCost: 30 mana\nBeats: 12\nCooldown: 60 rounds\nDuration: 6 rounds\n\nTestimony cannot be lost by any means. Verdict cooldown reduced to 1 round. Avoidance grants +2 base testimony (before Ninth Descent doubling).');
