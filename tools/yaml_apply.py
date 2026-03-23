#!/usr/bin/env python3
"""Apply YAML imports/updates to the PostgreSQL database.

The operation (import vs update) is inferred from the path — it must contain
the word 'import' or 'update' as a path component.  After a successful apply
the input file (or folder, for areas) is deleted.

Usage:
    # Area operations — pass the folder
    python3 tools/yaml_apply.py yaml/areas/import/whispering_forest_preserve/
    python3 tools/yaml_apply.py yaml/areas/update/midgaard/

    # Standalone records
    python3 tools/yaml_apply.py yaml/quests/import/200.yaml
    python3 tools/yaml_apply.py yaml/quests/update/42.yaml
    python3 tools/yaml_apply.py yaml/help/import/berserk.yaml
    python3 tools/yaml_apply.py yaml/help/update/berserk.yaml
    python3 tools/yaml_apply.py yaml/shelp/import/berserk.yaml
    python3 tools/yaml_apply.py yaml/lore/import/midgaard-history.yaml
    python3 tools/yaml_apply.py yaml/lore/update/midgaard-history.yaml

    # Batch — apply every file in a directory
    python3 tools/yaml_apply.py yaml/quests/import/
    python3 tools/yaml_apply.py yaml/areas/update/
"""

import argparse
import os
import shutil
import sys

import psycopg2
import psycopg2.sql
import yaml

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
DB_CONF = os.path.join(REPO_ROOT, 'data', 'db.conf')

RESET_TYPE_TO_CMD = {
    'mob': 'M', 'object': 'O', 'give': 'G', 'equip': 'E',
    'put': 'P', 'door': 'D', 'randomize': 'R', 'auto': 'A',
}


# ---------------------------------------------------------------------------
# DB connection
# ---------------------------------------------------------------------------

def connect():
    try:
        with open(DB_CONF) as f:
            dsn = f.read().strip()
    except FileNotFoundError:
        sys.exit(f"DB config not found: {DB_CONF}")
    return psycopg2.connect(dsn)


# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------

def infer_op(path):
    """Return 'import' or 'update' based on path components."""
    parts = os.path.normpath(path).split(os.sep)
    for p in parts:
        if p == 'import':
            return 'import'
        if p == 'update':
            return 'update'
    sys.exit(
        f"Cannot infer operation — path must contain 'import' or 'update': {path}"
    )


def detect_type(path):
    """Return the content type (areas/quests/help/shelp/lore) from path."""
    parts = os.path.normpath(path).split(os.sep)
    for t in ('areas', 'quests', 'help', 'shelp', 'lore'):
        if t in parts:
            return t
    sys.exit(f"Cannot detect content type from path: {path}")


# ---------------------------------------------------------------------------
# YAML loading
# ---------------------------------------------------------------------------

def load_yaml(path):
    with open(path) as f:
        return yaml.safe_load(f)


def load_yaml_opt(path):
    if os.path.exists(path):
        return load_yaml(path)
    return None


# ---------------------------------------------------------------------------
# Generic SQL helpers
# ---------------------------------------------------------------------------

def _insert(cur, table, data):
    cols = list(data.keys())
    vals = [data[c] for c in cols]
    query = psycopg2.sql.SQL(
        "INSERT INTO {table} ({cols}) VALUES ({placeholders})"
    ).format(
        table=psycopg2.sql.Identifier(table),
        cols=psycopg2.sql.SQL(', ').join(map(psycopg2.sql.Identifier, cols)),
        placeholders=psycopg2.sql.SQL(', ').join(
            psycopg2.sql.Placeholder() for _ in cols
        ),
    )
    cur.execute(query, vals)


def _update(cur, table, data, key_col, key_val):
    """UPDATE table SET col=val,... WHERE key_col=key_val.  Skips key_col itself."""
    fields = {k: v for k, v in data.items() if k != key_col}
    if not fields:
        return
    query = psycopg2.sql.SQL(
        "UPDATE {table} SET {sets} WHERE {key} = %s"
    ).format(
        table=psycopg2.sql.Identifier(table),
        sets=psycopg2.sql.SQL(', ').join(
            psycopg2.sql.SQL("{} = %s").format(psycopg2.sql.Identifier(k))
            for k in fields
        ),
        key=psycopg2.sql.Identifier(key_col),
    )
    cur.execute(query, list(fields.values()) + [key_val])


# ---------------------------------------------------------------------------
# Loot encoding (list → denormalized columns)
# ---------------------------------------------------------------------------

def encode_loot(loot_list):
    r = {'loot_amount': len(loot_list)}
    for i in range(9):
        if i < len(loot_list):
            r[f'loot_{i}'] = loot_list[i]['vnum']
            r[f'loot_chance_{i}'] = loot_list[i].get('chance', 100)
        else:
            r[f'loot_{i}'] = 0
            r[f'loot_chance_{i}'] = 0
    return r


# ---------------------------------------------------------------------------
# Reset encoding (human-readable dict → DB columns)
# ---------------------------------------------------------------------------

def encode_reset(r, seq):
    cmd = RESET_TYPE_TO_CMD.get(r['type'], r['type'])
    ifflag = r.get('ifflag', 0)
    notes = r.get('notes')
    auto_msg = r.get('auto_msg')
    if cmd == 'M':
        a1, a2, a3 = r['mob_vnum'], r.get('mob_max', 1), r['room_vnum']
    elif cmd == 'O':
        a1, a2, a3 = r['obj_vnum'], r.get('obj_max', 1), r['room_vnum']
    elif cmd == 'G':
        a1, a2, a3 = r['obj_vnum'], r.get('obj_max', 1), 0
    elif cmd == 'E':
        a1, a2, a3 = r['obj_vnum'], r.get('obj_max', 1), r['wear_loc']
    elif cmd == 'P':
        a1, a2, a3 = r['obj_vnum'], r.get('obj_max', 1), r['container_vnum']
    elif cmd == 'D':
        a1, a2, a3 = r['room_vnum'], r['direction'], r['door_state']
    elif cmd == 'R':
        a1, a2, a3 = r['room_vnum'], r.get('num_exits', 0), 0
    else:
        a1, a2, a3 = r.get('arg1', 0), r.get('arg2', 0), r.get('arg3', 0)
    return cmd, ifflag, a1, a2, a3, seq, notes, auto_msg


# ---------------------------------------------------------------------------
# Area sub-record helpers (shared by import and update)
# ---------------------------------------------------------------------------

DIR_NUMS = {'north': 0, 'east': 1, 'south': 2, 'west': 3, 'up': 4, 'down': 5}


def _insert_room(cur, room, area_id):
    exits = room.pop('exits', {})
    extra_descs = room.pop('extra_descs', [])
    room['area_id'] = area_id
    _insert(cur, 'rooms', room)
    vnum = room['vnum']
    for dir_name, ex in exits.items():
        ex['room_vnum'] = vnum
        ex['direction'] = DIR_NUMS.get(dir_name, int(dir_name))
        _insert(cur, 'room_exits', ex)
    for ed in extra_descs:
        ed['room_vnum'] = vnum
        _insert(cur, 'room_extra_descs', ed)


def _insert_mob(cur, mob, area_id):
    special = mob.pop('special', None)
    scripts = mob.pop('scripts', [])
    loot = mob.pop('loot', [])
    mob['area_id'] = area_id
    mob.update(encode_loot(loot))
    _insert(cur, 'mobiles', mob)
    vnum = mob['vnum']
    if special:
        cur.execute(
            "INSERT INTO mobile_specials (mob_vnum, spec_name) VALUES (%s, %s)",
            (vnum, special),
        )
    for s in scripts:
        cur.execute(
            "INSERT INTO mob_scripts (mob_vnum, seq, trigger, args, commands)"
            " VALUES (%s, %s, %s, %s, %s)",
            (vnum, s['seq'], s['trigger'], s.get('args', ''), s['commands']),
        )


def _insert_object(cur, obj, area_id):
    affects = obj.pop('affects', [])
    extra_descs = obj.pop('extra_descs', [])
    function = obj.pop('function', None)
    obj['area_id'] = area_id
    _insert(cur, 'objects', obj)
    vnum = obj['vnum']
    for aff in affects:
        cur.execute(
            "INSERT INTO object_affects (obj_vnum, location, modifier)"
            " VALUES (%s, %s, %s)",
            (vnum, aff['location'], aff['modifier']),
        )
    for ed in extra_descs:
        ed['obj_vnum'] = vnum
        _insert(cur, 'object_extra_descs', ed)
    if function:
        cur.execute(
            "INSERT INTO object_functions (obj_vnum, fun_name) VALUES (%s, %s)",
            (vnum, function),
        )


def _insert_resets(cur, resets_data, area_id):
    for seq, r in enumerate(resets_data):
        cmd, ifflag, a1, a2, a3, seq_n, notes, auto_msg = encode_reset(r, seq)
        cur.execute(
            "INSERT INTO resets"
            " (area_id, seq, command, ifflag, arg1, arg2, arg3, notes, auto_msg)"
            " VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)",
            (area_id, seq_n, cmd, ifflag, a1, a2, a3, notes, auto_msg),
        )


# ---------------------------------------------------------------------------
# Area import
# ---------------------------------------------------------------------------

def import_area(conn, folder):
    area_data = load_yaml(os.path.join(folder, 'area.yaml'))
    if not area_data:
        sys.exit(f"area.yaml is empty in {folder}")

    rooms_data = load_yaml_opt(os.path.join(folder, 'rooms.yaml')) or []
    mobs_data = load_yaml_opt(os.path.join(folder, 'mobs.yaml')) or []
    objs_data = load_yaml_opt(os.path.join(folder, 'objects.yaml')) or []
    resets_data = load_yaml_opt(os.path.join(folder, 'resets.yaml')) or []
    shops_data = load_yaml_opt(os.path.join(folder, 'shops.yaml')) or []

    with conn:
        cur = conn.cursor()
        cols = list(area_data.keys())
        vals = [area_data[c] for c in cols]
        cur.execute(
            f"INSERT INTO areas ({', '.join(cols)})"
            f" VALUES ({', '.join(['%s'] * len(cols))}) RETURNING id",
            vals,
        )
        area_id = cur.fetchone()[0]

        for room in rooms_data:
            _insert_room(cur, dict(room), area_id)
        for mob in mobs_data:
            _insert_mob(cur, dict(mob), area_id)
        for obj in objs_data:
            _insert_object(cur, dict(obj), area_id)
        _insert_resets(cur, resets_data, area_id)
        for shop in shops_data:
            _insert(cur, 'shops', dict(shop))

    print(f"Imported area from {os.path.relpath(folder, REPO_ROOT)}/")
    shutil.rmtree(folder)


# ---------------------------------------------------------------------------
# Area update
# ---------------------------------------------------------------------------

def update_area(conn, folder):
    area_kw = os.path.basename(os.path.normpath(folder))

    with conn:
        cur = conn.cursor()
        cur.execute("SELECT id FROM areas WHERE keyword = %s", (area_kw,))
        row = cur.fetchone()
        if not row:
            sys.exit(f"Area '{area_kw}' not found in database")
        area_id = row[0]

        # area.yaml — patch area header fields
        area_file = os.path.join(folder, 'area.yaml')
        if os.path.exists(area_file):
            _update(cur, 'areas', load_yaml(area_file), 'keyword', area_kw)

        # rooms.yaml — patch rooms by vnum; replace exits/extra_descs if present
        rooms_file = os.path.join(folder, 'rooms.yaml')
        if os.path.exists(rooms_file):
            for room in (load_yaml(rooms_file) or []):
                vnum = room.pop('vnum')
                exits = room.pop('exits', None)
                extra_descs = room.pop('extra_descs', None)
                if room:
                    _update(cur, 'rooms', room, 'vnum', vnum)
                if exits is not None:
                    cur.execute(
                        "DELETE FROM room_exits WHERE room_vnum = %s", (vnum,)
                    )
                    for dir_name, ex in exits.items():
                        ex['room_vnum'] = vnum
                        ex['direction'] = DIR_NUMS.get(dir_name, int(dir_name))
                        _insert(cur, 'room_exits', ex)
                if extra_descs is not None:
                    cur.execute(
                        "DELETE FROM room_extra_descs WHERE room_vnum = %s", (vnum,)
                    )
                    for ed in extra_descs:
                        ed['room_vnum'] = vnum
                        _insert(cur, 'room_extra_descs', ed)

        # mobs.yaml — patch mobs by vnum; replace loot/specials/scripts if present
        mobs_file = os.path.join(folder, 'mobs.yaml')
        if os.path.exists(mobs_file):
            for mob in (load_yaml(mobs_file) or []):
                vnum = mob.pop('vnum')
                loot = mob.pop('loot', None)
                special = mob.pop('special', None)
                scripts = mob.pop('scripts', None)
                if loot is not None:
                    mob.update(encode_loot(loot))
                if mob:
                    _update(cur, 'mobiles', mob, 'vnum', vnum)
                if special is not None:
                    cur.execute(
                        "INSERT INTO mobile_specials (mob_vnum, spec_name)"
                        " VALUES (%s, %s)"
                        " ON CONFLICT (mob_vnum) DO UPDATE SET spec_name = EXCLUDED.spec_name",
                        (vnum, special),
                    )
                if scripts is not None:
                    cur.execute(
                        "DELETE FROM mob_scripts WHERE mob_vnum = %s", (vnum,)
                    )
                    for s in scripts:
                        cur.execute(
                            "INSERT INTO mob_scripts"
                            " (mob_vnum, seq, trigger, args, commands)"
                            " VALUES (%s, %s, %s, %s, %s)",
                            (vnum, s['seq'], s['trigger'],
                             s.get('args', ''), s['commands']),
                        )

        # objects.yaml — patch objects by vnum; replace affects/extra_descs/function if present
        objs_file = os.path.join(folder, 'objects.yaml')
        if os.path.exists(objs_file):
            for obj in (load_yaml(objs_file) or []):
                vnum = obj.pop('vnum')
                affects = obj.pop('affects', None)
                extra_descs = obj.pop('extra_descs', None)
                function = obj.pop('function', None)
                if obj:
                    _update(cur, 'objects', obj, 'vnum', vnum)
                if affects is not None:
                    cur.execute(
                        "DELETE FROM object_affects WHERE obj_vnum = %s", (vnum,)
                    )
                    for aff in affects:
                        cur.execute(
                            "INSERT INTO object_affects"
                            " (obj_vnum, location, modifier) VALUES (%s, %s, %s)",
                            (vnum, aff['location'], aff['modifier']),
                        )
                if extra_descs is not None:
                    cur.execute(
                        "DELETE FROM object_extra_descs WHERE obj_vnum = %s", (vnum,)
                    )
                    for ed in extra_descs:
                        ed['obj_vnum'] = vnum
                        _insert(cur, 'object_extra_descs', ed)
                if function is not None:
                    cur.execute(
                        "INSERT INTO object_functions (obj_vnum, fun_name)"
                        " VALUES (%s, %s)"
                        " ON CONFLICT (obj_vnum) DO UPDATE SET fun_name = EXCLUDED.fun_name",
                        (vnum, function),
                    )

        # resets.yaml — full replacement for this area
        resets_file = os.path.join(folder, 'resets.yaml')
        if os.path.exists(resets_file):
            cur.execute("DELETE FROM resets WHERE area_id = %s", (area_id,))
            _insert_resets(cur, load_yaml(resets_file) or [], area_id)

        # shops.yaml — patch shops by keeper_vnum
        shops_file = os.path.join(folder, 'shops.yaml')
        if os.path.exists(shops_file):
            for shop in (load_yaml(shops_file) or []):
                keeper_vnum = shop.pop('keeper_vnum')
                if shop:
                    _update(cur, 'shops', shop, 'keeper_vnum', keeper_vnum)

    print(f"Updated area '{area_kw}' from {os.path.relpath(folder, REPO_ROOT)}/")
    shutil.rmtree(folder)


# ---------------------------------------------------------------------------
# Quest
# ---------------------------------------------------------------------------

def import_quest(conn, path):
    data = load_yaml(path)
    with conn:
        cur = conn.cursor()
        _insert(cur, 'quest_templates', data)
    print(f"Imported quest {data['id']}")
    os.remove(path)


def update_quest(conn, path):
    data = load_yaml(path)
    quest_id = data.pop('id')
    with conn:
        cur = conn.cursor()
        _update(cur, 'quest_templates', data, 'id', quest_id)
    print(f"Updated quest {quest_id}")
    os.remove(path)


# ---------------------------------------------------------------------------
# Help / shelp
# ---------------------------------------------------------------------------

def import_help_entry(conn, table, path):
    data = load_yaml(path)
    with conn:
        cur = conn.cursor()
        _insert(cur, table, data)
    print(f"Imported {table} '{data.get('filename', path)}'")
    os.remove(path)


def update_help_entry(conn, table, path):
    data = load_yaml(path)
    filename = data.pop('filename')
    with conn:
        cur = conn.cursor()
        _update(cur, table, data, 'filename', filename)
    print(f"Updated {table} '{filename}'")
    os.remove(path)


# ---------------------------------------------------------------------------
# Lore
# ---------------------------------------------------------------------------

def import_lore(conn, path):
    data = load_yaml(path)
    entries = data.pop('entries', [])
    with conn:
        cur = conn.cursor()
        cur.execute(
            "INSERT INTO lore_topics (filename, keywords) VALUES (%s, %s) RETURNING id",
            (data['filename'], data['keywords']),
        )
        topic_id = cur.fetchone()[0]
        for e in entries:
            cur.execute(
                "INSERT INTO lore_entries (topic_id, seq, flags, body)"
                " VALUES (%s, %s, %s, %s)",
                (topic_id, e['seq'], e.get('flags', 0), e['body']),
            )
    print(f"Imported lore topic '{data['filename']}'")
    os.remove(path)


def update_lore(conn, path):
    data = load_yaml(path)
    filename = data['filename']
    entries = data.pop('entries', None)
    with conn:
        cur = conn.cursor()
        cur.execute(
            "SELECT id FROM lore_topics WHERE filename = %s", (filename,)
        )
        row = cur.fetchone()
        if not row:
            sys.exit(f"Lore topic '{filename}' not found")
        topic_id = row[0]
        if 'keywords' in data:
            cur.execute(
                "UPDATE lore_topics SET keywords = %s WHERE id = %s",
                (data['keywords'], topic_id),
            )
        if entries is not None:
            cur.execute(
                "DELETE FROM lore_entries WHERE topic_id = %s", (topic_id,)
            )
            for e in entries:
                cur.execute(
                    "INSERT INTO lore_entries (topic_id, seq, flags, body)"
                    " VALUES (%s, %s, %s, %s)",
                    (topic_id, e['seq'], e.get('flags', 0), e['body']),
                )
    print(f"Updated lore topic '{filename}'")
    os.remove(path)


# ---------------------------------------------------------------------------
# Dispatch
# ---------------------------------------------------------------------------

def apply_path(conn, path):
    op = infer_op(path)
    content_type = detect_type(path)

    if os.path.isdir(path):
        if content_type == 'areas':
            if op == 'import':
                import_area(conn, path)
            else:
                update_area(conn, path)
        else:
            # Batch: apply every .yaml file in the directory
            for fname in sorted(os.listdir(path)):
                if fname.endswith('.yaml'):
                    apply_path(conn, os.path.join(path, fname))
    else:
        if content_type == 'quests':
            (import_quest if op == 'import' else update_quest)(conn, path)
        elif content_type == 'help':
            table = 'help_entries'
            (import_help_entry if op == 'import' else update_help_entry)(
                conn, table, path
            )
        elif content_type == 'shelp':
            table = 'shelp_entries'
            (import_help_entry if op == 'import' else update_help_entry)(
                conn, table, path
            )
        elif content_type == 'lore':
            (import_lore if op == 'import' else update_lore)(conn, path)
        else:
            sys.exit(
                f"Unexpected file for content type '{content_type}': {path}"
            )


def main():
    parser = argparse.ArgumentParser(
        description="Apply a YAML import or update to the database"
    )
    parser.add_argument(
        'path', help="Path to a YAML file or directory to apply"
    )
    args = parser.parse_args()

    path = os.path.normpath(os.path.join(os.getcwd(), args.path))
    if not os.path.exists(path):
        sys.exit(f"Path does not exist: {path}")

    conn = connect()
    try:
        apply_path(conn, path)
    finally:
        conn.close()


if __name__ == '__main__':
    main()
