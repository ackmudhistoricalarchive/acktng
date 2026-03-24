#!/usr/bin/env python3
"""
Seed a pre-existing test player file for the integration test.

Usage: python3 seed-test-player.py <player_dir> <name> <password>

Creates player/<first_letter>/<Name> with the given name, a DES-crypt
password hash, and places the character in ROOM_VNUM_SCHOOL (4900).
"""

import os
import sys

PLAYER_DIR = sys.argv[1]
NAME = sys.argv[2]
PASSWORD = sys.argv[3]

# Generate DES crypt hash using the name as salt.
# This mirrors the MUD's: pwdnew = crypt(argument, ch->name);
import ctypes
import ctypes.util
_libcrypt = ctypes.CDLL(ctypes.util.find_library("crypt") or "libcrypt.so")
_libcrypt.crypt.restype = ctypes.c_char_p
_libcrypt.crypt.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
pwd_hash = _libcrypt.crypt(PASSWORD.encode(), NAME.encode()).decode()

# Compute the canonical file name (cap_nocol: all lowercase, first letter upper).
name_canonical = NAME[0].upper() + NAME[1:].lower()
first_letter = name_canonical[0].lower()

player_subdir = os.path.join(PLAYER_DIR, first_letter)
os.makedirs(player_subdir, exist_ok=True)
player_file = os.path.join(player_subdir, name_canonical)

# ---- constants mirrored from src/headers/ ----
MAX_CLASS             = 6
MAX_REMORT            = MAX_CLASS * 2   # 12
MAX_REINCARNATE       = 10
MAX_COLOR             = 16
MAX_SUPERBOSS         = 5
QUEST_MAX_QUESTS      = 5
QUEST_MAX_TEMPLATES   = 200
QUEST_CARTOGRAPHY_BYTES = 256           # 2048 / 8
CLASS_WAR             = 3
ROOM_VNUM_SCHOOL      = 4900
ROOM_VNUM_TEMPLE      = 1209
POS_STANDING          = 7
SAVE_REVISION         = 15
# CONFIG_BLANK|CONFIG_COMBINE|CONFIG_PROMPT|CONFIG_MAPPER = 8|32|64|2048
CONFIG_DEFAULT        = 8 | 32 | 64 | 2048

CLASS = CLASS_WAR
LEVEL = 5
ROOM  = ROOM_VNUM_SCHOOL

# class_level array (indices 0-5: MAG CLE CIP WAR PSI PUG)
mc = ' '.join(str(LEVEL if i == CLASS else -1) for i in range(MAX_CLASS))

# Remort class_level (CLASS_SOR..CLASS_SOR+MAX_REMORT-1, all -1)
remort = ' '.join(['-1'] * MAX_REMORT)

# Adept class_level (CLASS_GMA..CLASS_GMA+MAX_CLASS-1, all -1)
adept = ' '.join(['-1'] * MAX_CLASS)

# Druid class_level (CLASS_DRU..CLASS_HIE = 4 entries, all -1)
druids = ' '.join(['-1'] * 4)

# Druid reincarnations (same 4 indices, all 0)
druid_reinc = ' '.join(['0'] * 4)

reincs       = ' '.join(['0'] * MAX_CLASS)
remort_reincs = ' '.join(['0'] * MAX_REMORT)
adept_reincs  = ' '.join(['0'] * MAX_CLASS)
reinc_data    = ' '.join(['0'] * MAX_REINCARNATE)
colors        = ' '.join(['0'] * MAX_COLOR)
superboss     = ' '.join(['0'] * MAX_SUPERBOSS)
cart_bits     = ' '.join(['0'] * QUEST_CARTOGRAPHY_BYTES)

quest_lines = []
for i in range(QUEST_MAX_QUESTS):
    quest_lines += [
        f"PropType{i}    0",
        f"PropDone{i}    0",
        f"PropTargets{i} 0",
        f"PropKillNeed{i} 0",
        f"PropKillGot{i}  0",
        f"PropStaticId{i} -1",
        f"PropRewardGold{i} 0",
        f"PropRewardQp{i} 0",
        f"PropRewardItem{i} 0 0",
        f"PropStaticOfferer{i} 0",
        f"PropCartArea{i} 0",
        f"PropCartRooms{i} 0",
        f"PropCartSeen{i} 0",
        f"PropCartBits{i} {cart_bits}",
    ]
quest_block = '\n'.join(quest_lines)

content = f"""\
#PLAYER
Revision     {SAVE_REVISION}
Name         {name_canonical}~
ShortDescr   ~
LongDescr    ~
Description  ~
Prompt       ~
Sex          1
LoginSex     1
Class        {CLASS}
Race         0
Level        {LEVEL}
Sentence     0
Invis        0
Incog        0
m/c          {mc}
Remort       {remort}
Adept       {adept}
Druidlevels  {druids}
Druidreinc   {druid_reinc}
Overgrowth   0
Reincarnations {reincs}
Remortreincarnations {remort_reincs}
Adeptreincarnations {adept_reincs}
Reinc_data  {reinc_data}
Adeptlevel   -1
Trust        0
Wizbit       0
Played       0
Note         0
Room         {ROOM}
HpManaMove   {LEVEL * 20} {LEVEL * 20} 100 100 100 100
Exp          0
Act          0
Config       {CONFIG_DEFAULT}
Gold         100
AffectedBy   0
Position     {POS_STANDING}
Practice     5
SavingThrow  0
Alignment    0
Hitroll      0
Damroll      0
Armor        100
Wimpy        0
Deaf         0
Generation   0
Clan         0
Mkills       0
Mkilled      0
Pkills       0
Pkilled      0
Password     {pwd_hash}~
Bamfin       ~
Bamfout      ~
Roomenter    ~
Roomexit     ~
Title        ~
Immskll      ~
Keep         0
KeepHealerBought 0
KeepHealerVnum 0
Whoname      Woff~
Monitor      0
Host         Unknown!~
Failures     0
LastLogin    Unknown!~
HiCol        y~
DimCol       b~
TermRows    25
TermColumns    80
Email   not set~
EmailValid    0
colors
{colors}
AttrPerm     13 13 13 13 13
AttrMod      0 0 0 0 0
AttrMax      18 18 18 18 18
Hasexpfix     0
Questpoints   0
InvasionPoints 0
InvasionRewards 0 0 0
QuestPoints 0
SuperbossKills {superboss}
PropDynCooldown 0
{quest_block}
PropStaticDoneCap {QUEST_MAX_TEMPLATES}
RecallVnum    {ROOM_VNUM_TEMPLE}
GainMana      -1
GainHp        -1
GainMove      -1
RulerRank    0
Pagelen      20
Pflags       0
End

#END
"""

with open(player_file, 'w') as f:
    f.write(content)

print(f"seed-test-player: created {player_file} (room={ROOM}, level={LEVEL})")

# If ACK_DB_CONF is set, also insert the player into the database so that the
# async DB login path can find the player.  The raw_save text is exactly the
# same content written to the flat file.
ack_db_conf = os.environ.get('ACK_DB_CONF')
if ack_db_conf:
    import subprocess

    # Read the connection string from the conf file.
    try:
        with open(ack_db_conf) as f:
            connstr = f.read().strip()
    except OSError as e:
        print(f"seed-test-player: warning: could not read ACK_DB_CONF ({ack_db_conf}): {e}")
        sys.exit(0)

    # Use dollar-quoting with a tag that will never appear in a player file.
    sql = (
        "INSERT INTO players (name, pwd_hash, raw_save) "
        f"VALUES ('{name_canonical}', '{pwd_hash}', $PLAYERRAW${content}$PLAYERRAW$) "
        "ON CONFLICT (name) DO UPDATE SET "
        "pwd_hash = EXCLUDED.pwd_hash, raw_save = EXCLUDED.raw_save;\n"
    )

    result = subprocess.run(
        ['psql', connstr, '-q', '-c', sql],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"seed-test-player: warning: DB insert failed: {result.stderr.strip()}")
    else:
        print(f"seed-test-player: inserted {name_canonical} into DB")
