#!/usr/bin/env python3
"""migrate_skills_to_db.py — Populate the skills table from compiled C skill/spell tables.

Reads src/spells/spell_table_data.c and src/skills/skill_table_data.c in the
order they are #included into const.c (spells first, then skills).  Assigns
sequential sn values starting at 0, extracts each entry's name, and INSERTs
(sn, name, script_source=NULL) rows into the skills table.

Existing rows are left intact (INSERT ... ON CONFLICT DO NOTHING), so the
tool is safe to re-run and will not overwrite script_source values already
set by hand.

Usage:
    python3 tools/migrate_skills_to_db.py [--dsn DSN] [--dry-run]

Options:
    --dsn DSN      PostgreSQL connection string.
                   Defaults to reading credentials/db.conf from the repo root.
    --dry-run      Print SQL instead of executing it.
"""

import argparse
import os
import re
import sys


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def parse_args():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dsn", default=None, help="PostgreSQL DSN (overrides db.conf)")
    ap.add_argument("--dry-run", action="store_true", help="Print SQL, do not execute")
    return ap.parse_args()


# ---------------------------------------------------------------------------
# DB.conf reader
# ---------------------------------------------------------------------------

def read_db_conf(repo_root):
    """Return a psycopg2-compatible DSN string from credentials/db.conf."""
    conf_path = os.path.join(repo_root, "credentials", "db.conf")
    if not os.path.exists(conf_path):
        sys.exit(f"error: {conf_path} not found — pass --dsn explicitly")
    with open(conf_path) as f:
        return f.read().strip()


# ---------------------------------------------------------------------------
# C source parser
# ---------------------------------------------------------------------------

# Matches the start of a skill_table entry: optional whitespace, then {NORM,
_ENTRY_START = re.compile(r"^\s*\{NORM\s*,", re.MULTILINE)

# Matches the first double-quoted string in a chunk of text.
_FIRST_STRING = re.compile(r'"((?:[^"\\]|\\.)*)"')


def extract_names_from_file(path):
    """Return list of skill/spell names in declaration order from a table data file."""
    with open(path) as f:
        src = f.read()

    # Split on each entry opener; first element is file header (discarded).
    parts = _ENTRY_START.split(src)
    if len(parts) < 2:
        sys.exit(f"error: no skill entries found in {path}")

    names = []
    for chunk in parts[1:]:  # skip header before first entry
        m = _FIRST_STRING.search(chunk)
        if not m:
            sys.exit(f"error: could not find name string in chunk:\n{chunk[:120]!r}")
        names.append(m.group(1))

    return names


def extract_all_skills(repo_root):
    """Return list of (sn, name) in the same order as const.c includes them."""
    spell_file = os.path.join(repo_root, "src", "spells", "spell_table_data.c")
    skill_file = os.path.join(repo_root, "src", "skills", "skill_table_data.c")

    spells = extract_names_from_file(spell_file)
    skills = extract_names_from_file(skill_file)

    entries = []
    for sn, name in enumerate(spells + skills):
        entries.append((sn, name))
    return entries


# ---------------------------------------------------------------------------
# SQL generation / execution
# ---------------------------------------------------------------------------

def build_sql(entries):
    rows = []
    for sn, name in entries:
        escaped = name.replace("'", "''")
        rows.append(f"({sn}, '{escaped}', NULL)")
    values = ",\n    ".join(rows)
    return (
        "INSERT INTO skills (sn, name, script_source)\nVALUES\n    "
        + values
        + "\nON CONFLICT (sn) DO NOTHING;"
    )


def run(args, repo_root):
    entries = extract_all_skills(repo_root)
    print(f"Parsed {len(entries)} skill/spell entries.", file=sys.stderr)

    sql = build_sql(entries)

    if args.dry_run:
        print(sql)
        return

    try:
        import psycopg2  # type: ignore
    except ImportError:
        sys.exit("error: psycopg2 not installed — run: pip install psycopg2-binary")

    dsn = args.dsn or read_db_conf(repo_root)
    conn = psycopg2.connect(dsn)
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute(sql)
                count = cur.rowcount
        print(f"Inserted {count} new rows into skills table.")
    finally:
        conn.close()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    args = parse_args()
    # Locate the repo root relative to this script (tools/ -> repo root).
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    run(args, repo_root)
