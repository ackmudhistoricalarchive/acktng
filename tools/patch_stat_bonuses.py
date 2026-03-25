#!/usr/bin/env python3
"""
patch_stat_bonuses.py - Add explicit spellpower/damroll to Lua damage calls.

For mud.damage_from_obj and mud.damage (non-physical): adds mud.get_spellpower(ctx.ch)
For mud.damage (ELE.PHYSICAL): adds mud.get_damroll(ch) // 2

Background: mud.damage and mud.damage_from_obj now set NO_STAT_BONUS, so the C
pipeline no longer auto-adds spellpower or damroll. Scripts must add bonuses
explicitly before calling the damage function.
"""

import re
import sys
import psycopg2


DB_CONF = "/root/aicli/credentials/db.conf"


def parse_db_conf(path):
    kv = {}
    with open(path) as f:
        for token in f.read().split():
            if "=" in token:
                k, v = token.split("=", 1)
                kv[k] = v
    return kv


def extract_nth_arg(s, n, start=0):
    """
    From string s starting at index start (which should point just after the
    opening paren of a function call), find the nth argument (0-indexed) and
    return (arg_text, arg_start, arg_end) where arg_end points to the char
    after the last char of the argument (i.e., the comma or closing paren).
    """
    depth = 0
    current_arg = 0
    arg_start = start
    i = start
    while i < len(s):
        c = s[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            if depth == 0:
                # end of argument list
                if current_arg == n:
                    return s[arg_start:i].strip(), arg_start, i
                return None
            depth -= 1
        elif c == "," and depth == 0:
            if current_arg == n:
                # Advance arg_start past leading whitespace (including newlines) so that
                # the replacement preserves the original indentation prefix.
                actual_start = arg_start
                while actual_start < i and s[actual_start] in " \t\n":
                    actual_start += 1
                return s[arg_start:i].strip(), actual_start, i
            current_arg += 1
            arg_start = i + 1
        i += 1
    return None


def find_caster_var(script, call_line_start):
    """
    Figure out the caster variable name used in the script.
    Looks backwards from the damage call for 'local ch = ctx.ch' or similar.
    Returns 'ctx.ch' as default, or 'ch' if a local alias is found.
    """
    before = script[:call_line_start]
    if "local ch = ctx.ch" in before:
        return "ch"
    return "ctx.ch"


def find_indentation(line):
    """Return the leading whitespace of a line."""
    return line[: len(line) - len(line.lstrip())]


def collect_full_call(lines, start_idx):
    """
    Starting at lines[start_idx] which contains a function call opening paren,
    collect subsequent lines until parentheses are balanced.
    Returns (combined_text, num_lines_consumed).
    """
    combined = lines[start_idx]
    depth = combined.count("(") - combined.count(")")
    n = 1
    while depth > 0 and start_idx + n < len(lines):
        combined += "\n" + lines[start_idx + n]
        depth += lines[start_idx + n].count("(") - lines[start_idx + n].count(")")
        n += 1
    return combined, n


def patch_damage_from_obj(script):
    """
    For each mud.damage_from_obj(obj, ch, victim, DAM, ele, sn, show) call,
    add mud.get_spellpower(caster) to DAM.
    """
    lines = script.split("\n")
    result = []
    i = 0
    while i < len(lines):
        line = lines[i]

        if "mud.damage_from_obj(" in line:
            indent = find_indentation(line)
            script_so_far = "\n".join(result)
            if "local ch = ctx.ch" in script_so_far:
                caster = "ch"
            else:
                caster = "ctx.ch"

            # Collect the full (possibly multi-line) call
            full_call, n_lines = collect_full_call(lines, i)
            call_start = full_call.index("mud.damage_from_obj(") + len("mud.damage_from_obj(")
            arg_info = extract_nth_arg(full_call, 3, call_start)

            if arg_info is not None:
                dam_expr, arg_s, arg_e = arg_info
                dam_expr = dam_expr.strip()
                if dam_expr == "dam":
                    result.append(f"{indent}dam = dam + mud.get_spellpower({caster})")
                    result.extend(lines[i:i + n_lines])
                else:
                    result.append(f"{indent}local dam = {dam_expr}")
                    result.append(f"{indent}dam = dam + mud.get_spellpower({caster})")
                    new_call = full_call[:arg_s] + "dam" + full_call[arg_e:]
                    result.extend(new_call.split("\n"))
                i += n_lines
            else:
                result.append(line)
                i += 1
        else:
            result.append(line)
            i += 1
    return "\n".join(result)


def patch_damage_call(script, is_physical):
    """
    For each mud.damage(ch, victim, DAM, ...) call, add the appropriate bonus.
    Physical: adds mud.get_damroll(caster) // 2
    Non-physical: adds mud.get_spellpower(caster)
    Only processes lines matching the expected element type.
    """
    lines = script.split("\n")
    result = []
    i = 0
    while i < len(lines):
        line = lines[i]

        has_damage = "mud.damage(" in line
        has_physical = "ELE.PHYSICAL" in line
        matches = has_damage and (is_physical == has_physical)

        if matches:
            indent = find_indentation(line)
            script_so_far = "\n".join(result)
            if "local ch = ctx.ch" in script_so_far:
                caster = "ch"
            else:
                caster = "ctx.ch"

            full_call, n_lines = collect_full_call(lines, i)
            call_start = full_call.index("mud.damage(") + len("mud.damage(")
            arg_info = extract_nth_arg(full_call, 2, call_start)

            if arg_info is not None:
                dam_expr, arg_s, arg_e = arg_info
                dam_expr = dam_expr.strip()
                bonus = (
                    f"mud.get_damroll({caster}) // 2"
                    if is_physical
                    else f"mud.get_spellpower({caster})"
                )
                if dam_expr == "dam":
                    result.append(f"{indent}dam = dam + {bonus}")
                    result.extend(lines[i:i + n_lines])
                else:
                    result.append(f"{indent}local dam = {dam_expr}")
                    result.append(f"{indent}dam = dam + {bonus}")
                    new_call = full_call[:arg_s] + "dam" + full_call[arg_e:]
                    result.extend(new_call.split("\n"))
                i += n_lines
            else:
                result.append(line)
                i += 1
        else:
            result.append(line)
            i += 1
    return "\n".join(result)


def patch_damage_physical(script):
    return patch_damage_call(script, is_physical=True)


def patch_damage_non_physical(script):
    return patch_damage_call(script, is_physical=False)


def patch_script(name, script):
    """Determine which patches to apply based on script content."""
    has_damage_from_obj = "mud.damage_from_obj(" in script
    has_damage_physical = "mud.damage(" in script and "ELE.PHYSICAL" in script
    # Non-physical mud.damage: has mud.damage but NOT damage_from_obj and NOT physical
    # (dispel evil, high explosive, jackal's verdict)
    has_damage_non_physical = (
        "mud.damage(" in script
        and not has_damage_from_obj
        and "ELE.PHYSICAL" not in script
    )

    patched = script
    if has_damage_from_obj:
        patched = patch_damage_from_obj(patched)
    if has_damage_physical:
        patched = patch_damage_physical(patched)
    if has_damage_non_physical:
        patched = patch_damage_non_physical(patched)

    return patched


def main():
    dry_run = "--dry-run" in sys.argv

    conf = parse_db_conf(DB_CONF)
    conn = psycopg2.connect(
        host=conf["host"],
        port=int(conf["port"]),
        dbname=conf["dbname"],
        user=conf["user"],
        password=conf["password"],
        sslmode=conf.get("sslmode", "prefer"),
    )

    cur = conn.cursor()
    cur.execute(
        """
        SELECT name, script_source FROM skills
        WHERE script_source LIKE '%mud.damage_from_obj%'
           OR (script_source LIKE '%mud.damage(%'
               AND script_source NOT LIKE '%mud.damage_from_obj%')
        ORDER BY name
        """
    )
    rows = cur.fetchall()

    updated = 0
    for name, script in rows:
        patched = patch_script(name, script)
        if patched != script:
            if dry_run:
                print(f"\n=== {name} ===")
                print("--- BEFORE ---")
                print(script[:500])
                print("--- AFTER ---")
                print(patched[:500])
            else:
                cur.execute(
                    "UPDATE skills SET script_source = %s WHERE name = %s",
                    (patched, name),
                )
                print(f"  patched: {name}")
            updated += 1
        else:
            print(f"  unchanged: {name}")

    if dry_run:
        print(f"\nDry run: {updated} scripts would be updated")
        conn.rollback()
    else:
        conn.commit()
        print(f"\nUpdated {updated} scripts")

    cur.close()
    conn.close()


if __name__ == "__main__":
    main()
