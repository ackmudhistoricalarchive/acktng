#!/usr/bin/env python3
"""migrate_phase3.py — Translate C spell/skill files to Lua and load into DB.

For each spell_*.c in src/spells/ and do_*.c in src/skills/:
  1. Extract the C function name and body.
  2. Map the function name to the DB skill name (strip prefix, replace _ → space).
  3. Call Claude to produce a Lua execute(ctx) script.
  4. Upsert the script into skills.script_source.

Usage:
    python3 tools/migrate_phase3.py [options]

Options:
    --dsn DSN       PostgreSQL connection string (overrides credentials/db.conf)
    --dry-run       Translate and print Lua scripts, do not write to DB
    --limit N       Process at most N files (for testing)
    --file FILE     Process a single C file by path
    --overwrite     Overwrite existing script_source values (default: skip)
    --model MODEL   Claude model to use (default: claude-sonnet-4-6)
    --workers N     Parallel translation workers (default: 2)
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dsn", default=None)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--file", default=None, help="Process a single C file")
    ap.add_argument("--overwrite", action="store_true",
                    help="Replace existing script_source values")
    ap.add_argument("--model", default="claude-sonnet-4-6")
    ap.add_argument("--workers", type=int, default=2)
    return ap.parse_args()


# ---------------------------------------------------------------------------
# DB credentials
# ---------------------------------------------------------------------------

def read_db_conf(repo_root):
    path = os.path.join(repo_root, "credentials", "db.conf")
    if not os.path.exists(path):
        sys.exit(f"error: {path} not found — pass --dsn explicitly")
    with open(path) as f:
        return f.read().strip()


# ---------------------------------------------------------------------------
# Build fn_name ↔ sn mapping from the C table data files
# ---------------------------------------------------------------------------

_ENTRY_START = re.compile(r"^\s*\{NORM\s*,", re.MULTILINE)
_FIRST_STRING = re.compile(r'"((?:[^"\\]|\\.)*)"')
# Spell table: function pointer follows {LEVELS_INIT...}.
# Handles optional C comments between the levels block and the function name.
_SPELL_FN_PTR = re.compile(
    r"\{LEVELS_INIT[^}]*\}"          # levels block
    r"(?:\s*,\s*(?:/\*.*?\*/\s*)?)?" # optional comma + comment
    r"\s*,?\s*([a-zA-Z_][a-zA-Z0-9_]*|NULL)\s*,",
    re.DOTALL,
)
# Skill table: gsn pointer in the entry
_GSN_PTR = re.compile(r"&(gsn_[a-zA-Z0-9_]+)\s*,")


def _parse_entries(path):
    """Return list of (first_quoted_name, raw_chunk) for each {NORM, ...} entry."""
    with open(path) as f:
        src = f.read()
    parts = _ENTRY_START.split(src)
    entries = []
    for chunk in parts[1:]:
        m = _FIRST_STRING.search(chunk)
        if m:
            entries.append((m.group(1), chunk))
    return entries


def build_fn_to_sn(repo_root):
    """Return dict mapping C function name → (sn, skill_name).

    For spells: key = spell function name (e.g. 'spell_acid_blast').
    For skills: key = do_* name derived from gsn (e.g. 'do_beserk').
    """
    spell_file = os.path.join(repo_root, "src", "spells", "spell_table_data.c")
    skill_file = os.path.join(repo_root, "src", "skills", "skill_table_data.c")

    spell_entries = _parse_entries(spell_file)
    skill_entries = _parse_entries(skill_file)

    fn_to_sn = {}  # fn_name → (sn, skill_name)

    # Spells: sn 0..N-1
    for sn, (name, chunk) in enumerate(spell_entries):
        m = _SPELL_FN_PTR.search(chunk)
        if m:
            fn_name = m.group(1)
            if fn_name != "NULL" and fn_name != "spell_null":
                fn_to_sn[fn_name] = (sn, name)

    # Skills: sn starts after spells
    sn_base = len(spell_entries)
    for i, (name, chunk) in enumerate(skill_entries):
        gsn_m = _GSN_PTR.search(chunk)
        if gsn_m:
            gsn_name = gsn_m.group(1)          # e.g. 'gsn_bash'
            do_name = "do_" + gsn_name[4:]     # e.g. 'do_bash'
            fn_to_sn[do_name] = (sn_base + i, name)
        else:
            # Some skill-table entries use a real spell_fun (not spell_null)
            # and have no gsn pointer (e.g. wizard-only spells listed here).
            fn_m = _SPELL_FN_PTR.search(chunk)
            if fn_m:
                fn_name = fn_m.group(1)
                if fn_name not in ("NULL", "spell_null"):
                    fn_to_sn[fn_name] = (sn_base + i, name)

    # Explicit aliases for C functions whose name doesn't match the gsn-derived name.
    # do_pick.c implements "pick lock" but gsn_pick_lock → derived "do_pick_lock"
    # do_shieldblock.c implements "shieldblock" but gsn_riposte is shared → derived "do_riposte"
    # do_fist_of_the_interior_form.c → gsn_fist_interior → derived "do_fist_interior"
    _aliases = {
        "do_pick":                      "do_pick_lock",
        "do_shieldblock":               "do_riposte",   # fn_to_sn["do_riposte"] == (356,"shieldblock")
        "do_fist_of_the_interior_form": "do_fist_interior",
    }
    for alias, canonical in _aliases.items():
        if canonical in fn_to_sn:
            fn_to_sn[alias] = fn_to_sn[canonical]

    return fn_to_sn


# ---------------------------------------------------------------------------
# C function extraction
# ---------------------------------------------------------------------------

# Match the top-level spell or skill function definition.
_SPELL_FN = re.compile(
    r"bool\s+(spell_\w+)\s*\(int\s+sn,\s*int\s+level,"
    r"\s*CHAR_DATA\s*\*ch,\s*void\s*\*vo,\s*OBJ_DATA\s*\*obj\)"
)
_SKILL_FN = re.compile(
    r"void\s+(do_\w+)\s*\(CHAR_DATA\s*\*ch,\s*char\s*\*argument\)"
)


def extract_function(src):
    """Return (kind, fn_name, full_function_source) or None if not found."""
    for pattern, kind in ((_SPELL_FN, "spell"), (_SKILL_FN, "skill")):
        m = pattern.search(src)
        if m:
            fn_name = m.group(1)
            start = m.start()
            # Find the matching closing brace.
            depth = 0
            i = src.index("{", start)
            while i < len(src):
                if src[i] == "{":
                    depth += 1
                elif src[i] == "}":
                    depth -= 1
                    if depth == 0:
                        return kind, fn_name, src[start : i + 1]
                i += 1
    return None


def fn_name_to_skill_name(fn_name):
    """Convert C function name to a best-guess DB skill name (fallback only).

    Use build_fn_to_sn() for accurate lookup; this is only for logging.
    """
    for prefix in ("spell_", "do_"):
        if fn_name.startswith(prefix):
            return fn_name[len(prefix):].replace("_", " ")
    return fn_name.replace("_", " ")


# ---------------------------------------------------------------------------
# Lua API reference (system prompt)
# ---------------------------------------------------------------------------

LUA_API_REFERENCE = """
You are translating ACK!MUD TNG C spell/skill functions to Lua 5.4.

=== CONTEXT VARIABLES ===

For SPELLS, execute(ctx) receives:
  ctx.sn        -- skill number (integer)
  ctx.level     -- spell level (integer)
  ctx.beats     -- cast time beats
  ctx.ch        -- caster (CHAR_DATA userdata)
  ctx.victim    -- target char (for TAR_CHAR_* targets)
  ctx.obj_target-- target object (for TAR_OBJ_INV)
  ctx.cast_obj  -- wand/scroll/staff used (may be nil)
  ctx.target_name-- target name string (for TAR_IGNORE spells)

For SKILLS, execute(ctx) receives:
  ctx.sn        -- skill number
  ctx.beats     -- beat time (for wait state)
  ctx.ch        -- character using skill
  ctx.argument  -- command argument string

Spell scripts return true (success) or false (failure).
Skill scripts have no meaningful return value.

=== CHAR_DATA METHODS ===

ch:get_hp()            ch:set_hp(n)
ch:get_max_hp()
ch:get_mana()          ch:set_mana(n)
ch:get_max_mana()
ch:get_move()          ch:set_move(n)
ch:get_max_move()
ch:get_level()
ch:get_class_level(cls)  -- cls is a CLASS.* constant
ch:get_alignment()     ch:set_alignment(n)
ch:get_gold()          ch:set_gold(n)
ch:get_name()
ch:get_room()
ch:get_fighting()      -- returns char or nil
ch:get_position()      ch:set_position(n)
ch:get_str()  ch:get_dex()  ch:get_wis()  ch:get_int()  ch:get_con()
ch:get_chi()
ch:get_sex()           ch:set_sex(n)
ch:get_master()        -- returns char or nil
ch:get_extract_timer() ch:set_extract_timer(n)
ch:is_npc()
ch:is_affected(sn)     -- checks affect list by spell sn
ch:has_aff(bit)        -- checks affected_by bitvector (IS_AFFECTED macro)
ch:learned(sn)         -- learned % for a skill (0 for NPCs)
ch:cooldown(sn)        -- cooldown ticks remaining

=== mud.* API ===

-- Damage / combat
mud.damage(ch, victim, dam, sn, element, show_msg)  -- returns bool (hit)
mud.damage_from_obj(obj_or_nil, ch, victim, dam, element, sn, show_msg)
mud.war_attack(ch, arg, sn)          -- standard warrior attack
mud.saves_spell(level, victim)       -- returns bool
mud.is_safe(ch, victim)              -- returns bool
mud.set_fighting(ch, victim)
mud.stop_fighting(ch, both)          -- both=true to stop both sides
mud.check_killer(ch, victim)
mud.can_hit_skill(ch, victim, sn)    -- returns bool
mud.calculate_damage(ch, victim, dam, dt, element, crit_possible)  -- returns actual dam
mud.basic_damage(ch, victim, dam, dt)  -- raw damage(), returns int
mud.one_hit(ch, victim, dt)
mud.aoe_damage(ch, sn, level, min_dam, max_dam, element, flags[, obj])
mud.breath_damage(ch, sn, element)
mud.cast_wizard_elemental_dot_spell(sn, level, ch, victim, obj_or_nil, cast_msg, dmg_msg, element)  -- returns bool
mud.trigger_elemental_spell_combo(ch, victim, obj_or_nil, sn, level)  -- returns bool
mud.apply_elemental_spell_debuff(ch, victim, sn, msg)
mud.multi_hit(ch, victim, dt)
mud.combo(ch, victim, sn)            -- returns bool
mud.backstab(ch, victim, is_backstab_bool)
mud.stun(ch, victim)
mud.disarm(ch, victim)
mud.trip(ch, victim)
mud.check_killer(ch, victim)

-- Healing
mud.heal(victim, amount)             -- heal HP, calls update_pos
mud.heal_mana(victim, amount)
mud.heal_move(victim, amount)
mud.heal_character(ch, victim, base, sn, hot)  -- class-scaled heal
mud.do_spell_heal(ch, victim, sn)    -- standard cleric heal

-- Affects
mud.apply_affect(victim, {type=sn, duration=N, duration_type=D,
                           location=APPLY.X, modifier=N, bitvector=AFF.X,
                           element=ELE.X, level=N, caster=ch})
mud.affect_join(victim, {same fields})   -- adds or extends existing affect
mud.affect_strip(victim, sn)
mud.is_affected(ch, sn)              -- returns bool (checks affect list)
mud.affect_to_room(room, {type=sn, duration=N, level=N, bitvector=0,
                           applies_spell=sn, modifier=N, location=APPLY.X,
                           caster=ch})

-- Output
mud.send(victim, "message\n\r")
mud.act(msg, ch, obj_or_nil, victim_or_nil, to_whom)
  -- to_whom: "TO_CHAR", "TO_VICT", "TO_ROOM", "TO_NOTVICT"
mud.echo_room(room, msg)

-- Skill system
mud.raise_skill(ch, sn)              -- returns bool
mud.wait_state(ch, beats)
mud.set_cooldown(ch, sn, ticks)
mud.can_use_skill(ch, sn)            -- returns bool, no message
mud.can_use_skill_message(ch, sn)    -- returns bool, sends failure msg
mud.can_use_pub_society_skill(ch, sn)-- returns bool
mud.subtract_energy(ch, sn)         -- returns bool
mud.mana_cost(ch, sn)               -- returns int
mud.skill_success(ch, sn)           -- returns bool (random success check)
mud.skill_lookup(name)               -- returns sn or -1
mud.is_valid_finisher(ch)
mud.reset_combo(ch)
mud.get_chi(ch)                     -- returns chi amount
mud.chi_skill_cost(base, cooldown)  -- returns bool (deducts chi)
mud.pug_attack(ch, arg, sn)
mud.do_poison(ch, arg, gsn)         -- returns bool
mud.can_see(ch, victim)             -- returns bool
mud.apply_necromancer_debuff(ch, victim, sn, dam[, obj])
mud.cast_spell(sn, level, ch, victim_or_nil[, obj_or_nil])  -- calls C spell_fun, returns bool

-- World / movement
mud.char_to_room(ch, room)
mud.get_room(vnum)                   -- returns room or nil
mud.get_char_room(ch, name)         -- returns char or nil
mud.get_char_world(ch, name)        -- returns char or nil
mud.chars_in_room(room)             -- returns TABLE (array) of chars, NOT an iterator
mud.transfer(ch, room)
mud.room_is_private(room)           -- returns bool
mud.set_hunt(ch, fch, victim, set_flags[, rem_flags]) -- returns bool
mud.gain_exp(ch, amount)

-- NPC / followers
mud.create_mobile(mob_index)
mud.add_follower(ch, master)
mud.stop_follower(ch)
mud.extract_char(ch, fished)
mud.set_mob_level(mob, level)
mud.set_mob_max_hp(mob, max_hp)

-- Objects
mud.create_object(obj_index, level)
mud.obj_to_room(obj, room)
mud.obj_to_char(obj, ch)
mud.obj_from_obj(obj)
mud.extract_obj(obj)
mud.get_obj_carry(ch, name)
mud.get_obj_room(room, name)
mud.get_obj_contents(obj)

-- Commands
mud.interpret(ch, cmd)
mud.do_say(ch, msg)
mud.do_look(ch[, arg])
mud.do_sleep(ch[, arg])

-- Randomness
mud.dice(num, size)                  -- returns int
mud.number_range(min, max)           -- returns int
mud.number_percent()                 -- returns 1..100
mud.UMIN(a, b)    mud.UMAX(a, b)

=== CONSTANTS ===

ELE.NONE  ELE.PHYSICAL  ELE.MENTAL  ELE.HOLY  ELE.AIR
ELE.EARTH ELE.WATER     ELE.FIRE    ELE.SHADOW ELE.POISON

AFF.BLIND  AFF.INVISIBLE  AFF.DETECT_EVIL  AFF.DETECT_INVIS
AFF.DETECT_MAGIC  AFF.DETECT_HIDDEN  AFF.SANCTUARY  AFF.FAERIE_FIRE
AFF.INFRARED  AFF.CURSE  AFF.POISON  AFF.PROTECT  AFF.SNEAK
AFF.HIDE  AFF.SLEEP  AFF.CHARM  AFF.FLYING  AFF.PASS_DOOR
AFF.ANTI_MAGIC  AFF.BLASTED  AFF.REMORT_CURSE  AFF.CONFUSED
AFF.HOLD  AFF.PARALYSIS

APPLY.NONE  APPLY.STR  APPLY.DEX  APPLY.INT  APPLY.WIS  APPLY.CON
APPLY.SEX  APPLY.CLASS  APPLY.LEVEL  APPLY.AGE  APPLY.HEIGHT
APPLY.WEIGHT  APPLY.MANA  APPLY.HIT  APPLY.MOVE  APPLY.GOLD
APPLY.EXP  APPLY.AC  APPLY.HITROLL  APPLY.DAMROLL  APPLY.DOT
APPLY.SPELLPOWER  APPLY.SAVING_PARA  APPLY.SAVING_ROD
APPLY.SAVING_SPELL  APPLY.SPEED

POS.DEAD  POS.MORTAL  POS.INCAP  POS.STUNNED  POS.SLEEPING
POS.RESTING  POS.FIGHTING  POS.STANDING

TAR.IGNORE  TAR.CHAR_OFFENSIVE  TAR.CHAR_DEFENSIVE  TAR.CHAR_SELF
TAR.OBJ_INV  TAR.CHAR_NOTSELF

CLASS.MAG  CLASS.CLE  CLASS.CIP  CLASS.WAR  CLASS.PSI  CLASS.PUG
CLASS.SOR  CLASS.PAL  CLASS.ASS  CLASS.KNI  CLASS.NEC  CLASS.MON
CLASS.WIZ  CLASS.PRI  CLASS.WLK  CLASS.SWO  CLASS.EGO  CLASS.BRA
CLASS.GMA  CLASS.TEM  CLASS.NIG  CLASS.CRU  CLASS.KIN  CLASS.MAR
CLASS.DRU  CLASS.THO  CLASS.WIL  CLASS.HIE  CLASS.SEN

DURATION.HOUR  DURATION.ROUND

FLAGS.NO_REFLECT  FLAGS.NO_ABSORB

AOE.SAVES  AOE.SKIP_GROUP

CLOAK.REFLECTION  CLOAK.FLAMING  CLOAK.ABSORPTION  CLOAK.ADEPT

ITEM_APPLY.NONE  ITEM_APPLY.INFRA  ITEM_APPLY.INV  ITEM_APPLY.DET_INV
ITEM_APPLY.SANC  ITEM_APPLY.SNEAK  ITEM_APPLY.HIDE  ITEM_APPLY.PROT
ITEM_APPLY.ENHANCED  ITEM_APPLY.DET_MAG  ITEM_APPLY.DET_HID
ITEM_APPLY.DET_EVIL  ITEM_APPLY.PASS_DOOR  ITEM_APPLY.DET_POISON
ITEM_APPLY.FLY  ITEM_APPLY.KNOW_ALIGN  ITEM_APPLY.DET_UNDEAD
ITEM_APPLY.HEATED

HUNT.MERC  HUNT.CR

=== C → LUA TRANSLATION RULES ===

1. ELEMENT MACROS: ELEMENT_FIRE → ELE.FIRE (all ELEMENT_* = ELE.*).

2. IS_AFFECTED(ch, AFF_X) → ch:has_aff(AFF.X)
   is_affected(ch, sn)    → mud.is_affected(ch, sn) or ch:is_affected(sn)

3. AFFECT_DATA blocks → mud.apply_affect(victim, {...}) table.
   af.duration_type field: DURATION_ROUND → DURATION.ROUND, DURATION_HOUR → DURATION.HOUR

4. HEAL patterns:
   victim->hit = UMIN(victim->hit + heal, get_max_hp(victim)); update_pos(victim);
   → mud.heal(victim, heal)

5. dam_each[] tables → Lua tables (1-indexed: dam_each[level+1] for 0-based C arrays).
   level = UMIN(level, N-1) guards → just index with mud.UMIN(level+1, #dam_each).

6. C: gsn_blindness → Lua: mud.skill_lookup("blindness")
   Use mud.skill_lookup() for all gsn_* references.

7. Calling another spell from a spell:
   spell_cure_blindness(sn, level, ch, vo, obj) →
   mud.cast_spell(mud.skill_lookup("cure blindness"), level, ch, ctx.victim)

8. WAIT_STATE(ch, beats) → mud.wait_state(ch, ctx.beats)
   (spell beats come from skill_table; just use ctx.beats)
   For skills, ch->cooldown[gsn_X] = N → mud.set_cooldown(ch, ctx.sn, N)
   For skills that set a DIFFERENT skill's cooldown:
   mud.set_cooldown(ch, mud.skill_lookup("other skill name"), N)

9. get_psuedo_level(ch) → mud.get_pseudo_level(ch)  (note corrected spelling in Lua)

10. one_argument(argument, arg) — in skills, use Lua's string library:
    local arg = ctx.argument:match("^(%S+)") or ""
    local rest = ctx.argument:match("^%S+%s*(.*)") or ""

11. number_range(a, b) → mud.number_range(a, b)
    dice(n, s) → mud.dice(n, s)
    number_percent() → mud.number_percent()

12. MOB vnums: use the integer literal, e.g. mud.create_mobile(mud.get_mob_index(30001))
    Actually: create_mobile takes a mob_index; use mud.create_mobile_vnum(vnum) if available,
    or just note the vnum as a comment and call mud.create_mobile with the integer.
    NOTE: mud.create_mobile(vnum) accepts an integer vnum directly.

13. IS_NPC(ch) → ch:is_npc()
    IS_GOOD(ch) → ch:get_alignment() >= 350
    IS_EVIL(ch) → ch:get_alignment() <= -350
    IS_NEUTRAL(ch) → (not good and not evil)
    IS_AWAKE(ch) → ch:get_position() > POS.SLEEPING

14. aoe_damage(ch, obj, sn, level, min, max, element, flags) →
    mud.aoe_damage(ch, sn, level, min, max, element, flags, obj_or_nil)
    (obj goes to the end as optional 8th argument)

15. affect_to_room(room, &raf) → mud.affect_to_room(room, {...})
    ROOM_AFFECT_DATA fields: duration, level, type, bitvector, applies_spell, modifier, location, caster

16. apply_necromancer_damage_debuff(ch, victim, sn, damage, obj) →
    mud.apply_necromancer_debuff(ch, victim, sn, damage[, obj])

17. For TAR_IGNORE spells using target_name:
    get_char_world(ch, target_name) → mud.get_char_world(ch, ctx.target_name)

18. Mob vitals after create: mob->level = N → mud.set_mob_level(mob, N)
    mob->max_hit = N → mud.set_mob_max_hp(mob, N) then mob:set_hp(mob:get_max_hp())

19. For loops over room characters (for vch in ch->in_room->people...):
    for _, vch in ipairs(mud.chars_in_room(ch:get_room())) do ... end
    (mud.chars_in_room returns a table, NOT an iterator — always use ipairs)

20. str_cmp(s1, s2) == 0 → s1 == s2  (case-insensitive equal)

=== OUTPUT FORMAT ===

Return ONLY the Lua code, no explanations, no markdown fences.
The script must define function execute(ctx) at module level.
Preserve all game logic faithfully. Use gsn lookups for all gsn_* references.
"""


TRANSLATE_PROMPT = """\
Translate this ACK!MUD TNG C {kind} to Lua.

Skill/spell name: "{skill_name}" (sn will be ctx.sn at runtime)

C source:
```c
{c_source}
```

Return ONLY the Lua execute(ctx) script, no markdown, no explanation.
"""


# ---------------------------------------------------------------------------
# Claude translation
#
# Two backends, selected automatically:
#   1. Anthropic SDK  — when ANTHROPIC_API_KEY is set in the environment.
#                       Requires: pip install anthropic
#   2. claude -p CLI  — fallback when no API key is present; requires the
#                       Claude Code CLI to be installed and authenticated.
# ---------------------------------------------------------------------------

def _strip_fences(text):
    text = re.sub(r"^```(?:lua)?\n?", "", text.strip())
    text = re.sub(r"\n?```$", "", text)
    return text.strip()


def _translate_via_sdk(client, model, full_prompt):
    """Translate using the Anthropic Python SDK (requires ANTHROPIC_API_KEY)."""
    response = client.messages.create(
        model=model,
        max_tokens=4096,
        messages=[{"role": "user", "content": full_prompt}],
    )
    return _strip_fences(response.content[0].text)


def _translate_via_cli(model, full_prompt):
    """Translate using the `claude -p` CLI subprocess."""
    result = subprocess.run(
        [
            "claude", "-p",
            "--output-format", "text",
            "--no-session-persistence",
            "--model", model,
        ],
        input=full_prompt,
        capture_output=True,
        text=True,
        timeout=300,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"claude exited {result.returncode}")
    return _strip_fences(result.stdout)


def translate_with_claude(client, model, kind, skill_name, c_source):
    """Translate a C spell/skill to Lua.

    Uses the Anthropic SDK when `client` is not None (ANTHROPIC_API_KEY set),
    otherwise falls back to the `claude -p` CLI.
    """
    user_msg = TRANSLATE_PROMPT.format(
        kind=kind,
        skill_name=skill_name,
        c_source=c_source,
    )
    full_prompt = LUA_API_REFERENCE + "\n\n" + user_msg

    if client is not None:
        return _translate_via_sdk(client, model, full_prompt)
    return _translate_via_cli(model, full_prompt)


# ---------------------------------------------------------------------------
# DB upsert
# ---------------------------------------------------------------------------

def upsert_script(conn, sn, script_source, overwrite):
    # Skills always already exist in the DB (we got their sn from the DB mapping).
    # Never INSERT — just UPDATE to avoid NOT NULL violations on other columns.
    with conn:
        with conn.cursor() as cur:
            if overwrite:
                cur.execute(
                    "UPDATE skills SET script_source = %s WHERE sn = %s",
                    (script_source, sn),
                )
            else:
                cur.execute(
                    """UPDATE skills SET script_source = %s
                       WHERE sn = %s
                         AND (script_source IS NULL OR script_source = '')""",
                    (script_source, sn),
                )


def get_existing_scripts(conn):
    """Return set of sn values that already have a non-empty script_source."""
    with conn.cursor() as cur:
        cur.execute(
            "SELECT sn FROM skills "
            "WHERE script_source IS NOT NULL AND script_source != ''"
        )
        return {row[0] for row in cur.fetchall()}


# ---------------------------------------------------------------------------
# File enumeration
# ---------------------------------------------------------------------------

def collect_c_files(repo_root):
    """Return sorted list of (path, kind) for all spell/skill C files."""
    files = []
    for p in sorted(Path(repo_root, "src", "spells").glob("spell_*.c")):
        if p.name != "spell_table_data.c":
            files.append((str(p), "spell"))
    for p in sorted(Path(repo_root, "src", "skills").glob("do_*.c")):
        if p.name not in ("skill_table_data.c",):
            files.append((str(p), "skill"))
    return files


# ---------------------------------------------------------------------------
# Main translation worker
# ---------------------------------------------------------------------------

def process_file(path, kind, fn_to_sn, client, model, dry_run, overwrite,
                 existing_scripts, conn):
    """Process one C file.  Returns (skill_name, sn, status_str)."""
    with open(path) as f:
        src = f.read()

    result = extract_function(src)
    if result is None:
        return None, None, "SKIP: no top-level function found"

    fn_kind, fn_name, fn_src = result
    entry = fn_to_sn.get(fn_name)

    if entry is None:
        guess = fn_name_to_skill_name(fn_name)
        return guess, None, f"SKIP: '{fn_name}' not in skill table"

    sn, skill_name = entry

    if not overwrite and sn in existing_scripts:
        return skill_name, sn, "SKIP: already has script"

    try:
        lua_script = translate_with_claude(client, model, fn_kind, skill_name, fn_src)
    except Exception as exc:
        return skill_name, sn, f"ERROR: {exc}"

    if dry_run:
        print(f"\n--- sn={sn}  name='{skill_name}'  ({path}) ---")
        print(lua_script)
        return skill_name, sn, "DRY-RUN"

    try:
        upsert_script(conn, sn, lua_script, overwrite)
        return skill_name, sn, "OK"
    except Exception as exc:
        return skill_name, sn, f"DB ERROR: {exc}"


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    args = parse_args()
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    # Build fn_name → (sn, skill_name) mapping.
    fn_to_sn = build_fn_to_sn(repo_root)
    print(f"Loaded {len(fn_to_sn)} spell/skill function mappings.", file=sys.stderr)

    # Set up translation backend.
    # Prefer Anthropic SDK (ANTHROPIC_API_KEY); fall back to `claude -p` CLI.
    client = None
    api_key = os.environ.get("ANTHROPIC_API_KEY", "")
    if api_key:
        try:
            import anthropic  # type: ignore
            client = anthropic.Anthropic(api_key=api_key)
            print("Using Anthropic SDK (ANTHROPIC_API_KEY).", file=sys.stderr)
        except ImportError:
            sys.exit("error: ANTHROPIC_API_KEY set but anthropic not installed — "
                     "pip install anthropic")
    else:
        if subprocess.run(["which", "claude"], capture_output=True).returncode != 0:
            sys.exit(
                "error: no ANTHROPIC_API_KEY set and 'claude' CLI not found in PATH.\n"
                "Either set ANTHROPIC_API_KEY and install the anthropic package, "
                "or install the Claude Code CLI."
            )
        print("Using claude -p CLI (no ANTHROPIC_API_KEY set).", file=sys.stderr)

    # Determine files to process.
    if args.file:
        # Detect kind from path.
        p = args.file
        kind = "skill" if "/do_" in p or "\\do_" in p else "spell"
        files = [(p, kind)]
    else:
        files = collect_c_files(repo_root)

    if args.limit:
        files = files[: args.limit]

    print(f"Processing {len(files)} files with model {args.model}.", file=sys.stderr)

    # Connect to DB unless dry-run.
    conn = None
    existing_scripts = set()
    if not args.dry_run:
        try:
            import psycopg2  # type: ignore
        except ImportError:
            sys.exit("error: psycopg2 not installed — run: pip install psycopg2-binary")
        dsn = args.dsn or read_db_conf(repo_root)
        conn = psycopg2.connect(dsn)
        conn.set_client_encoding("UTF8")
        existing_scripts = get_existing_scripts(conn)
        print(f"{len(existing_scripts)} skills already have scripts.", file=sys.stderr)

    # Process files (parallel for speed).
    results = []
    def worker(item):
        path, kind = item
        return process_file(
            path, kind, fn_to_sn, client, args.model,
            args.dry_run, args.overwrite, existing_scripts, conn,
        )

    ok = skip = err = 0
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        futures = {pool.submit(worker, f): f for f in files}
        for fut in as_completed(futures):
            path, kind = futures[fut]
            try:
                skill_name, sn, status = fut.result()
            except Exception as exc:
                skill_name, sn, status = "?", None, f"EXCEPTION: {exc}"
            fname = os.path.basename(path)
            print(f"  [{status:12}] sn={sn!s:4}  {fname}  ({skill_name})")
            if status.startswith("OK") or status == "DRY-RUN":
                ok += 1
            elif status.startswith("SKIP"):
                skip += 1
            else:
                err += 1

    if conn:
        conn.close()

    print(f"\nDone: {ok} translated, {skip} skipped, {err} errors.", file=sys.stderr)
    if err:
        sys.exit(1)


if __name__ == "__main__":
    main()
