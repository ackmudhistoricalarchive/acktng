#!/usr/bin/env python3
"""Export game content from PostgreSQL to YAML files.

Run from acktng/ root or any directory — paths are resolved relative to this script.

Usage:
    python3 tools/yaml_export.py --area midgaard
    python3 tools/yaml_export.py --area-vnum 3000
    python3 tools/yaml_export.py --all-areas
    python3 tools/yaml_export.py --quest 42
    python3 tools/yaml_export.py --all-quests
    python3 tools/yaml_export.py --help-entry BERSERK
    python3 tools/yaml_export.py --all-help
    python3 tools/yaml_export.py --shelp BERSERK
    python3 tools/yaml_export.py --all-shelp
    python3 tools/yaml_export.py --lore MIDGAARD-HISTORY
    python3 tools/yaml_export.py --all-lore

Output lands in acktng/yaml/<type>/export/.
"""

import argparse
import os
import sys

import psycopg2
import psycopg2.extras
import yaml

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)
YAML_ROOT = os.path.join(REPO_ROOT, 'yaml')
DB_CONF = os.path.join(REPO_ROOT, 'data', 'db.conf')

DIR_NAMES = {0: 'north', 1: 'east', 2: 'south', 3: 'west', 4: 'up', 5: 'down'}

RESET_CMD_TO_TYPE = {
    'M': 'mob', 'O': 'object', 'G': 'give', 'E': 'equip',
    'P': 'put', 'D': 'door', 'R': 'randomize', 'A': 'auto',
}


def _str_representer(dumper, data):
    """Use block scalar style for multi-line strings."""
    if '\n' in data:
        return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
    return dumper.represent_scalar('tag:yaml.org,2002:str', data)


yaml.add_representer(str, _str_representer)


def connect():
    try:
        with open(DB_CONF) as f:
            dsn = f.read().strip()
    except FileNotFoundError:
        sys.exit(f"DB config not found: {DB_CONF}")
    return psycopg2.connect(dsn, cursor_factory=psycopg2.extras.RealDictCursor)


def write_yaml(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f:
        yaml.dump(data, f, default_flow_style=False, allow_unicode=True, sort_keys=False)
    print(f"  {os.path.relpath(path, REPO_ROOT)}")


def decode_reset(row):
    """Convert a DB resets row to a human-readable dict."""
    cmd = row['command'].strip()
    a1, a2, a3 = row['arg1'], row['arg2'], row['arg3']
    r = {'type': RESET_CMD_TO_TYPE.get(cmd, cmd)}
    if row['ifflag']:
        r['ifflag'] = row['ifflag']
    if cmd == 'M':
        r.update(mob_vnum=a1, mob_max=a2, room_vnum=a3)
    elif cmd == 'O':
        r.update(obj_vnum=a1, obj_max=a2, room_vnum=a3)
    elif cmd == 'G':
        r.update(obj_vnum=a1, obj_max=a2)
    elif cmd == 'E':
        r.update(obj_vnum=a1, obj_max=a2, wear_loc=a3)
    elif cmd == 'P':
        r.update(obj_vnum=a1, obj_max=a2, container_vnum=a3)
    elif cmd == 'D':
        r.update(room_vnum=a1, direction=a2, door_state=a3)
    elif cmd == 'R':
        r.update(room_vnum=a1, num_exits=a2)
    else:
        r.update(arg1=a1, arg2=a2, arg3=a3)
    if row.get('notes'):
        r['notes'] = row['notes']
    if row.get('auto_msg'):
        r['auto_msg'] = row['auto_msg']
    return r


def export_area(conn, area_row):
    area_id = area_row['id']
    kw = area_row['keyword'] or str(area_row['min_vnum'])
    out_dir = os.path.join(YAML_ROOT, 'areas', 'export', kw)
    print(f"Exporting area '{kw}'...")

    # area.yaml — drop the serial id; all other fields are content
    area_data = {k: v for k, v in area_row.items() if k != 'id'}
    write_yaml(os.path.join(out_dir, 'area.yaml'), area_data)

    cur = conn.cursor()

    # rooms.yaml
    cur.execute(
        "SELECT * FROM rooms WHERE area_id = %s ORDER BY vnum", (area_id,)
    )
    rooms = []
    for room in cur.fetchall():
        r = {k: v for k, v in room.items() if k != 'area_id'}
        vnum = r['vnum']

        cur.execute(
            "SELECT direction, dest_vnum, exit_flags, key_vnum, keyword, description"
            " FROM room_exits WHERE room_vnum = %s ORDER BY direction",
            (vnum,),
        )
        exits = {}
        for ex in cur.fetchall():
            d = DIR_NAMES.get(ex['direction'], str(ex['direction']))
            exits[d] = {k: v for k, v in ex.items() if k != 'direction'}
        if exits:
            r['exits'] = exits

        cur.execute(
            "SELECT keyword, description FROM room_extra_descs WHERE room_vnum = %s",
            (vnum,),
        )
        extra = [dict(row) for row in cur.fetchall()]
        if extra:
            r['extra_descs'] = extra

        rooms.append(r)
    write_yaml(os.path.join(out_dir, 'rooms.yaml'), rooms)

    # mobs.yaml
    cur.execute(
        "SELECT * FROM mobiles WHERE area_id = %s ORDER BY vnum", (area_id,)
    )
    mobs = []
    for mob in cur.fetchall():
        m = {k: v for k, v in mob.items() if k != 'area_id'}
        vnum = m['vnum']

        # Decode denormalized loot columns → list
        loot = []
        for i in range(m['loot_amount']):
            loot_vnum = m[f'loot_{i}']
            chance = m[f'loot_chance_{i}']
            if loot_vnum:
                loot.append({'vnum': loot_vnum, 'chance': chance})
        for i in range(9):
            del m[f'loot_{i}']
            del m[f'loot_chance_{i}']
        del m['loot_amount']
        if loot:
            m['loot'] = loot

        cur.execute(
            "SELECT spec_name FROM mobile_specials WHERE mob_vnum = %s", (vnum,)
        )
        spec = cur.fetchone()
        if spec:
            m['special'] = spec['spec_name']

        cur.execute(
            "SELECT seq, trigger, args, commands FROM mob_scripts"
            " WHERE mob_vnum = %s ORDER BY seq",
            (vnum,),
        )
        scripts = [dict(row) for row in cur.fetchall()]
        if scripts:
            m['scripts'] = scripts

        mobs.append(m)
    write_yaml(os.path.join(out_dir, 'mobs.yaml'), mobs)

    # objects.yaml
    cur.execute(
        "SELECT * FROM objects WHERE area_id = %s ORDER BY vnum", (area_id,)
    )
    objs = []
    for obj in cur.fetchall():
        o = {k: v for k, v in obj.items() if k != 'area_id'}
        vnum = o['vnum']

        cur.execute(
            "SELECT location, modifier FROM object_affects WHERE obj_vnum = %s",
            (vnum,),
        )
        affects = [dict(row) for row in cur.fetchall()]
        if affects:
            o['affects'] = affects

        cur.execute(
            "SELECT keyword, description FROM object_extra_descs WHERE obj_vnum = %s",
            (vnum,),
        )
        extra = [dict(row) for row in cur.fetchall()]
        if extra:
            o['extra_descs'] = extra

        cur.execute(
            "SELECT fun_name FROM object_functions WHERE obj_vnum = %s", (vnum,)
        )
        fn = cur.fetchone()
        if fn:
            o['function'] = fn['fun_name']

        objs.append(o)
    write_yaml(os.path.join(out_dir, 'objects.yaml'), objs)

    # resets.yaml
    cur.execute(
        "SELECT * FROM resets WHERE area_id = %s ORDER BY seq", (area_id,)
    )
    resets = [decode_reset(row) for row in cur.fetchall()]
    write_yaml(os.path.join(out_dir, 'resets.yaml'), resets)

    # shops.yaml — join through mobiles to filter by area
    cur.execute(
        """
        SELECT s.keeper_vnum, s.buy_type_0, s.buy_type_1, s.buy_type_2,
               s.buy_type_3, s.buy_type_4, s.profit_buy, s.profit_sell,
               s.open_hour, s.close_hour
        FROM shops s
        JOIN mobiles m ON m.vnum = s.keeper_vnum
        WHERE m.area_id = %s
        ORDER BY s.keeper_vnum
        """,
        (area_id,),
    )
    shops = [dict(row) for row in cur.fetchall()]
    write_yaml(os.path.join(out_dir, 'shops.yaml'), shops)


def export_quests(conn, quest_id=None):
    cur = conn.cursor()
    if quest_id is not None:
        cur.execute("SELECT * FROM quest_templates WHERE id = %s", (quest_id,))
        rows = cur.fetchall()
        if not rows:
            sys.exit(f"Quest {quest_id} not found")
    else:
        cur.execute("SELECT * FROM quest_templates ORDER BY id")
        rows = cur.fetchall()
    for q in rows:
        write_yaml(
            os.path.join(YAML_ROOT, 'quests', 'export', f"{q['id']}.yaml"),
            dict(q),
        )


def export_help_table(conn, table, subdir, keyword=None):
    cur = conn.cursor()
    if keyword:
        cur.execute(
            f"SELECT * FROM {table} WHERE keywords ILIKE %s", (f'%{keyword}%',)
        )
    else:
        cur.execute(f"SELECT * FROM {table} ORDER BY filename")
    for row in cur.fetchall():
        data = {k: v for k, v in row.items() if k != 'id'}
        write_yaml(
            os.path.join(YAML_ROOT, subdir, 'export', f"{row['filename']}.yaml"),
            data,
        )


def export_lore(conn, keyword=None):
    cur = conn.cursor()
    if keyword:
        cur.execute(
            "SELECT * FROM lore_topics WHERE keywords ILIKE %s", (f'%{keyword}%',)
        )
    else:
        cur.execute("SELECT * FROM lore_topics ORDER BY filename")
    topics = cur.fetchall()
    for topic in topics:
        cur.execute(
            "SELECT seq, flags, body FROM lore_entries"
            " WHERE topic_id = %s ORDER BY seq",
            (topic['id'],),
        )
        entries = [dict(e) for e in cur.fetchall()]
        data = {
            'filename': topic['filename'],
            'keywords': topic['keywords'],
            'entries': entries,
        }
        write_yaml(
            os.path.join(YAML_ROOT, 'lore', 'export', f"{topic['filename']}.yaml"),
            data,
        )


def main():
    parser = argparse.ArgumentParser(
        description="Export game content from the database to YAML"
    )
    g = parser.add_mutually_exclusive_group(required=True)
    g.add_argument('--area', metavar='KEYWORD', help="Export one area by keyword")
    g.add_argument(
        '--area-vnum', type=int, metavar='VNUM',
        help="Export the area containing VNUM",
    )
    g.add_argument('--all-areas', action='store_true', help="Export all areas")
    g.add_argument('--quest', type=int, metavar='ID', help="Export one quest by id")
    g.add_argument('--all-quests', action='store_true', help="Export all quests")
    g.add_argument(
        '--help-entry', metavar='KEYWORD',
        help="Export help entries whose keywords match KEYWORD",
    )
    g.add_argument('--all-help', action='store_true', help="Export all help entries")
    g.add_argument(
        '--shelp', metavar='KEYWORD',
        help="Export skill/spell help entries matching KEYWORD",
    )
    g.add_argument(
        '--all-shelp', action='store_true',
        help="Export all skill/spell help entries",
    )
    g.add_argument(
        '--lore', metavar='KEYWORD',
        help="Export lore topics matching KEYWORD",
    )
    g.add_argument('--all-lore', action='store_true', help="Export all lore topics")
    args = parser.parse_args()

    conn = connect()

    if args.area or args.area_vnum is not None or args.all_areas:
        cur = conn.cursor()
        if args.area:
            cur.execute("SELECT * FROM areas WHERE keyword = %s", (args.area,))
            rows = cur.fetchall()
            if not rows:
                sys.exit(f"Area '{args.area}' not found")
        elif args.area_vnum is not None:
            cur.execute(
                "SELECT * FROM areas WHERE min_vnum <= %s AND max_vnum >= %s",
                (args.area_vnum, args.area_vnum),
            )
            rows = cur.fetchall()
            if not rows:
                sys.exit(f"No area contains vnum {args.area_vnum}")
        else:
            cur.execute("SELECT * FROM areas ORDER BY min_vnum")
            rows = cur.fetchall()
        for row in rows:
            export_area(conn, row)

    elif args.quest is not None or args.all_quests:
        export_quests(conn, args.quest)

    elif args.help_entry or args.all_help:
        export_help_table(conn, 'help_entries', 'help', args.help_entry)

    elif args.shelp or args.all_shelp:
        export_help_table(conn, 'shelp_entries', 'shelp', args.shelp)

    elif args.lore or args.all_lore:
        export_lore(conn, args.lore)

    conn.close()


if __name__ == '__main__':
    main()
