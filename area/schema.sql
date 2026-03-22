-- ACK!TNG PostgreSQL Schema
-- Version 2
-- Apply with: psql -U ack -d acktng -f schema.sql
-- All vnums and integer flags are INTEGER (32-bit) unless noted as BIGINT (64-bit).
-- All text strings are TEXT. Auto-increment primary keys use SERIAL.

BEGIN;

-- -----------------------------------------------------------------------
-- 4.1 areas
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS areas (
    id          SERIAL  PRIMARY KEY,
    name        TEXT    NOT NULL,
    min_vnum    INTEGER NOT NULL,
    max_vnum    INTEGER NOT NULL,
    keyword     TEXT,
    level_label TEXT,
    area_number INTEGER,
    level_min   INTEGER,
    level_max   INTEGER,
    map_offset  INTEGER,
    reset_rate  INTEGER,
    reset_msg   TEXT,
    owner       TEXT,
    can_read    TEXT,
    can_write   TEXT,
    music       TEXT,
    flags       INTEGER NOT NULL DEFAULT 0,
    UNIQUE(min_vnum),
    UNIQUE(max_vnum),
    CHECK(min_vnum <= max_vnum)
);

-- -----------------------------------------------------------------------
-- 4.2 rooms
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS rooms (
    vnum         INTEGER PRIMARY KEY,
    area_id      INTEGER NOT NULL REFERENCES areas(id),
    name         TEXT    NOT NULL,
    description  TEXT    NOT NULL,
    room_flags   INTEGER NOT NULL DEFAULT 0,
    sector_type  INTEGER NOT NULL DEFAULT 0
);

-- -----------------------------------------------------------------------
-- 4.3 room_exits
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS room_exits (
    id          SERIAL  PRIMARY KEY,
    room_vnum   INTEGER NOT NULL REFERENCES rooms(vnum),
    direction   INTEGER NOT NULL,
    dest_vnum   INTEGER,
    exit_flags  INTEGER NOT NULL DEFAULT 0,
    key_vnum    INTEGER,
    keyword     TEXT,
    description TEXT,
    UNIQUE(room_vnum, direction)
);

-- -----------------------------------------------------------------------
-- 4.4 room_extra_descs
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS room_extra_descs (
    id          SERIAL  PRIMARY KEY,
    room_vnum   INTEGER NOT NULL REFERENCES rooms(vnum),
    keyword     TEXT    NOT NULL,
    description TEXT    NOT NULL
);

-- -----------------------------------------------------------------------
-- 4.5 mobiles
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS mobiles (
    vnum          INTEGER PRIMARY KEY,
    area_id       INTEGER NOT NULL REFERENCES areas(id),
    player_name   TEXT    NOT NULL,
    short_descr   TEXT    NOT NULL,
    long_descr    TEXT    NOT NULL,
    description   TEXT    NOT NULL,
    act_flags     BIGINT  NOT NULL DEFAULT 0,
    affected_by   INTEGER NOT NULL DEFAULT 0,
    alignment     INTEGER NOT NULL DEFAULT 0,
    level         INTEGER NOT NULL DEFAULT 1,
    sex           INTEGER NOT NULL DEFAULT 0,
    hp_mod        INTEGER NOT NULL DEFAULT 0,
    ac_mod        INTEGER NOT NULL DEFAULT 0,
    hr_mod        INTEGER NOT NULL DEFAULT 0,
    dr_mod        INTEGER NOT NULL DEFAULT 0,
    -- '!' extension fields
    class         INTEGER NOT NULL DEFAULT 0,
    clan          INTEGER NOT NULL DEFAULT 0,
    race          INTEGER NOT NULL DEFAULT 0,
    position      INTEGER NOT NULL DEFAULT 0,
    skills        INTEGER NOT NULL DEFAULT 0,
    cast_flags    INTEGER NOT NULL DEFAULT 0,
    def           INTEGER NOT NULL DEFAULT 0,
    -- '|' extension fields
    strong_magic  INTEGER NOT NULL DEFAULT 0,
    weak_magic    INTEGER NOT NULL DEFAULT 0,
    race_mods     INTEGER NOT NULL DEFAULT 0,
    power_skills  INTEGER NOT NULL DEFAULT 0,
    power_cast    INTEGER NOT NULL DEFAULT 0,
    resist        INTEGER NOT NULL DEFAULT 0,
    suscept       INTEGER NOT NULL DEFAULT 0,
    -- '+' extension fields
    spellpower    INTEGER NOT NULL DEFAULT 0,
    crit          INTEGER NOT NULL DEFAULT 0,
    crit_mult     INTEGER NOT NULL DEFAULT 0,
    spell_crit    INTEGER NOT NULL DEFAULT 0,
    spell_mult    INTEGER NOT NULL DEFAULT 0,
    parry         INTEGER NOT NULL DEFAULT 0,
    dodge         INTEGER NOT NULL DEFAULT 0,
    block         INTEGER NOT NULL DEFAULT 0,
    pierce        INTEGER NOT NULL DEFAULT 0,
    -- 'a' AI extension fields
    ai_knowledge  INTEGER NOT NULL DEFAULT 0,
    accent        INTEGER NOT NULL DEFAULT 0,
    ai_prompt     TEXT,
    -- loot table (denormalized to match MAX_LOOT=9 fixed array in C)
    loot_amount   INTEGER NOT NULL DEFAULT 0,
    loot_0        INTEGER NOT NULL DEFAULT 0,
    loot_1        INTEGER NOT NULL DEFAULT 0,
    loot_2        INTEGER NOT NULL DEFAULT 0,
    loot_3        INTEGER NOT NULL DEFAULT 0,
    loot_4        INTEGER NOT NULL DEFAULT 0,
    loot_5        INTEGER NOT NULL DEFAULT 0,
    loot_6        INTEGER NOT NULL DEFAULT 0,
    loot_7        INTEGER NOT NULL DEFAULT 0,
    loot_8        INTEGER NOT NULL DEFAULT 0,
    loot_chance_0 INTEGER NOT NULL DEFAULT 0,
    loot_chance_1 INTEGER NOT NULL DEFAULT 0,
    loot_chance_2 INTEGER NOT NULL DEFAULT 0,
    loot_chance_3 INTEGER NOT NULL DEFAULT 0,
    loot_chance_4 INTEGER NOT NULL DEFAULT 0,
    loot_chance_5 INTEGER NOT NULL DEFAULT 0,
    loot_chance_6 INTEGER NOT NULL DEFAULT 0,
    loot_chance_7 INTEGER NOT NULL DEFAULT 0,
    loot_chance_8 INTEGER NOT NULL DEFAULT 0
);

-- -----------------------------------------------------------------------
-- 4.6 objects
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS objects (
    vnum         INTEGER PRIMARY KEY,
    area_id      INTEGER NOT NULL REFERENCES areas(id),
    name         TEXT    NOT NULL,
    short_descr  TEXT    NOT NULL,
    description  TEXT    NOT NULL,
    item_type    INTEGER NOT NULL,
    extra_flags  BIGINT  NOT NULL DEFAULT 0,
    wear_flags   INTEGER NOT NULL DEFAULT 0,
    item_apply   INTEGER NOT NULL DEFAULT 0,
    value_0      INTEGER NOT NULL DEFAULT 0,
    value_1      INTEGER NOT NULL DEFAULT 0,
    value_2      INTEGER NOT NULL DEFAULT 0,
    value_3      INTEGER NOT NULL DEFAULT 0,
    value_4      INTEGER NOT NULL DEFAULT 0,
    value_5      INTEGER NOT NULL DEFAULT 0,
    value_6      INTEGER NOT NULL DEFAULT 0,
    value_7      INTEGER NOT NULL DEFAULT 0,
    value_8      INTEGER NOT NULL DEFAULT 0,
    value_9      INTEGER NOT NULL DEFAULT 0,
    weight       INTEGER NOT NULL DEFAULT 0,
    level        INTEGER NOT NULL DEFAULT 0
);

-- -----------------------------------------------------------------------
-- 4.7 object_extra_descs
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS object_extra_descs (
    id          SERIAL  PRIMARY KEY,
    obj_vnum    INTEGER NOT NULL REFERENCES objects(vnum),
    keyword     TEXT    NOT NULL,
    description TEXT    NOT NULL
);

-- -----------------------------------------------------------------------
-- 4.8 object_affects
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS object_affects (
    id        SERIAL  PRIMARY KEY,
    obj_vnum  INTEGER NOT NULL REFERENCES objects(vnum),
    location  INTEGER NOT NULL,
    modifier  INTEGER NOT NULL
);

-- -----------------------------------------------------------------------
-- 4.9 shops
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS shops (
    id           SERIAL  PRIMARY KEY,
    keeper_vnum  INTEGER NOT NULL REFERENCES mobiles(vnum),
    buy_type_0   INTEGER NOT NULL DEFAULT 0,
    buy_type_1   INTEGER NOT NULL DEFAULT 0,
    buy_type_2   INTEGER NOT NULL DEFAULT 0,
    buy_type_3   INTEGER NOT NULL DEFAULT 0,
    buy_type_4   INTEGER NOT NULL DEFAULT 0,
    profit_buy   INTEGER NOT NULL DEFAULT 100,
    profit_sell  INTEGER NOT NULL DEFAULT 100,
    open_hour    INTEGER NOT NULL DEFAULT 0,
    close_hour   INTEGER NOT NULL DEFAULT 23,
    UNIQUE(keeper_vnum)
);

-- -----------------------------------------------------------------------
-- 4.10 resets
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS resets (
    id        SERIAL  PRIMARY KEY,
    area_id   INTEGER NOT NULL REFERENCES areas(id),
    seq       INTEGER NOT NULL,
    command   CHAR(1) NOT NULL CHECK(command IN ('M','O','G','E','D','R','P','A')),
    ifflag    INTEGER NOT NULL DEFAULT 0,
    arg1      INTEGER NOT NULL DEFAULT 0,
    arg2      INTEGER NOT NULL DEFAULT 0,
    arg3      INTEGER NOT NULL DEFAULT 0,
    notes     TEXT,
    auto_msg  TEXT,
    UNIQUE(area_id, seq)
);

-- -----------------------------------------------------------------------
-- 4.11 mobile_specials
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS mobile_specials (
    mob_vnum  INTEGER PRIMARY KEY REFERENCES mobiles(vnum),
    spec_name TEXT    NOT NULL
);

-- -----------------------------------------------------------------------
-- 4.12 mob_scripts
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS mob_scripts (
    id       SERIAL  PRIMARY KEY,
    mob_vnum INTEGER NOT NULL REFERENCES mobiles(vnum),
    seq      INTEGER NOT NULL,
    trigger  TEXT    NOT NULL,
    args     TEXT    NOT NULL DEFAULT '',
    commands TEXT    NOT NULL,
    UNIQUE(mob_vnum, seq)
);

-- -----------------------------------------------------------------------
-- 4.13 object_functions
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS object_functions (
    obj_vnum  INTEGER PRIMARY KEY REFERENCES objects(vnum),
    fun_name  TEXT    NOT NULL
);

-- -----------------------------------------------------------------------
-- 4.14 help_entries
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS help_entries (
    id       SERIAL  PRIMARY KEY,
    filename TEXT    NOT NULL UNIQUE,
    level    INTEGER NOT NULL,
    keywords TEXT    NOT NULL,
    body     TEXT    NOT NULL
);

-- -----------------------------------------------------------------------
-- 4.15 shelp_entries
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS shelp_entries (
    id       SERIAL  PRIMARY KEY,
    filename TEXT    NOT NULL UNIQUE,
    level    INTEGER NOT NULL,
    keywords TEXT    NOT NULL,
    body     TEXT    NOT NULL
);

-- -----------------------------------------------------------------------
-- 4.16 lore_topics
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS lore_topics (
    id       SERIAL PRIMARY KEY,
    filename TEXT   NOT NULL UNIQUE,
    keywords TEXT   NOT NULL
);

-- -----------------------------------------------------------------------
-- 4.17 lore_entries
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS lore_entries (
    id        SERIAL  PRIMARY KEY,
    topic_id  INTEGER NOT NULL REFERENCES lore_topics(id),
    seq       INTEGER NOT NULL,
    flags     BIGINT  NOT NULL DEFAULT 0,
    body      TEXT    NOT NULL,
    UNIQUE(topic_id, seq)
);

-- -----------------------------------------------------------------------
-- 4.18 bans
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS bans (
    id         SERIAL  PRIMARY KEY,
    ban_type   INTEGER NOT NULL DEFAULT 0,
    address    TEXT    NOT NULL,
    banned_by  TEXT    NOT NULL DEFAULT ''
);

-- -----------------------------------------------------------------------
-- 4.19 socials
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS socials (
    id            SERIAL PRIMARY KEY,
    name          TEXT   NOT NULL UNIQUE,
    char_no_arg   TEXT   NOT NULL DEFAULT '',
    others_no_arg TEXT   NOT NULL DEFAULT '',
    char_found    TEXT   NOT NULL DEFAULT '',
    others_found  TEXT   NOT NULL DEFAULT '',
    vict_found    TEXT   NOT NULL DEFAULT '',
    char_auto     TEXT   NOT NULL DEFAULT '',
    others_auto   TEXT   NOT NULL DEFAULT ''
);

-- -----------------------------------------------------------------------
-- 4.20 boards
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS boards (
    id            SERIAL  PRIMARY KEY,
    vnum          INTEGER NOT NULL UNIQUE,
    expiry_days   INTEGER NOT NULL DEFAULT 10,
    min_read_lev  INTEGER NOT NULL DEFAULT 0,
    min_write_lev INTEGER NOT NULL DEFAULT 0,
    clan          INTEGER NOT NULL DEFAULT 0
);

-- -----------------------------------------------------------------------
-- 4.21 board_messages
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS board_messages (
    id         SERIAL      PRIMARY KEY,
    board_id   INTEGER     NOT NULL REFERENCES boards(id) ON DELETE CASCADE,
    posted_at  BIGINT      NOT NULL,
    author     TEXT        NOT NULL,
    title      TEXT        NOT NULL DEFAULT '',
    body       TEXT        NOT NULL DEFAULT '',
    seq        INTEGER     NOT NULL
);
CREATE INDEX IF NOT EXISTS board_messages_board_id_seq ON board_messages(board_id, seq);

-- -----------------------------------------------------------------------
-- 4.22 clans
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS clans (
    id            INTEGER PRIMARY KEY,
    name          TEXT    NOT NULL DEFAULT '',
    war_count     INTEGER NOT NULL DEFAULT 0,
    win_count     INTEGER NOT NULL DEFAULT 0,
    loss_count    INTEGER NOT NULL DEFAULT 0,
    member_count  INTEGER NOT NULL DEFAULT 0,
    gold          INTEGER NOT NULL DEFAULT 0,
    war_matrix    INTEGER[] NOT NULL DEFAULT '{}'
);

-- -----------------------------------------------------------------------
-- 4.23 rulers
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS rulers (
    id    SERIAL PRIMARY KEY,
    name  TEXT   NOT NULL UNIQUE
);

-- -----------------------------------------------------------------------
-- 4.24 brands
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS brands (
    id          SERIAL PRIMARY KEY,
    branded_by  TEXT   NOT NULL,
    item_name   TEXT   NOT NULL,
    brand_date  TEXT   NOT NULL,
    description TEXT   NOT NULL DEFAULT ''
);

-- -----------------------------------------------------------------------
-- 4.25 room_marks
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS room_marks (
    id        SERIAL  PRIMARY KEY,
    room_vnum INTEGER NOT NULL REFERENCES rooms(vnum),
    mark_text TEXT    NOT NULL
);

-- -----------------------------------------------------------------------
-- 4.26 corpses
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS corpses (
    id           SERIAL  PRIMARY KEY,
    where_vnum   INTEGER NOT NULL REFERENCES rooms(vnum),
    nest         INTEGER NOT NULL DEFAULT 0,
    name         TEXT    NOT NULL,
    short_descr  TEXT    NOT NULL,
    description  TEXT    NOT NULL,
    vnum         INTEGER NOT NULL DEFAULT 0,
    extra_flags  BIGINT  NOT NULL DEFAULT 0,
    wear_flags   INTEGER NOT NULL DEFAULT 0,
    wear_loc     INTEGER NOT NULL DEFAULT -1,
    class_flags  INTEGER NOT NULL DEFAULT 0,
    item_type    INTEGER NOT NULL DEFAULT 0,
    weight       INTEGER NOT NULL DEFAULT 0,
    level        INTEGER NOT NULL DEFAULT 0,
    timer        INTEGER NOT NULL DEFAULT 0,
    cost         INTEGER NOT NULL DEFAULT 0,
    value_0      INTEGER NOT NULL DEFAULT 0,
    value_1      INTEGER NOT NULL DEFAULT 0,
    value_2      INTEGER NOT NULL DEFAULT 0,
    value_3      INTEGER NOT NULL DEFAULT 0,
    value_4      INTEGER NOT NULL DEFAULT 0,
    value_5      INTEGER NOT NULL DEFAULT 0,
    value_6      INTEGER NOT NULL DEFAULT 0,
    value_7      INTEGER NOT NULL DEFAULT 0,
    value_8      INTEGER NOT NULL DEFAULT 0,
    value_9      INTEGER NOT NULL DEFAULT 0,
    parent_id    INTEGER REFERENCES corpses(id)
);

-- -----------------------------------------------------------------------
-- 4.27 sysdata
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS sysdata (
    id             INTEGER PRIMARY KEY DEFAULT 1 CHECK(id = 1),
    mud_name       TEXT    NOT NULL DEFAULT '',
    admin_email    TEXT    NOT NULL DEFAULT '',
    login_msg      TEXT    NOT NULL DEFAULT '',
    motd           TEXT    NOT NULL DEFAULT '',
    welcome        TEXT    NOT NULL DEFAULT '',
    news           TEXT    NOT NULL DEFAULT '',
    int_val_1      INTEGER NOT NULL DEFAULT 0,
    int_val_2      INTEGER NOT NULL DEFAULT 0,
    bln_val_0      BOOLEAN NOT NULL DEFAULT FALSE,
    bln_val_1      BOOLEAN NOT NULL DEFAULT FALSE,
    bln_val_2      BOOLEAN NOT NULL DEFAULT FALSE,
    bln_val_3      BOOLEAN NOT NULL DEFAULT FALSE,
    bln_val_4      BOOLEAN NOT NULL DEFAULT FALSE,
    bln_val_5      BOOLEAN NOT NULL DEFAULT FALSE,
    bln_val_6      BOOLEAN NOT NULL DEFAULT FALSE,
    bln_val_7      BOOLEAN NOT NULL DEFAULT FALSE
);
INSERT INTO sysdata (id) VALUES (1) ON CONFLICT DO NOTHING;

-- -----------------------------------------------------------------------
-- 4.28 players
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS players (
    id              SERIAL  PRIMARY KEY,
    name            TEXT    NOT NULL UNIQUE,
    pwd_hash        TEXT    NOT NULL,
    title           TEXT    NOT NULL DEFAULT '',
    description     TEXT    NOT NULL DEFAULT '',
    race            INTEGER NOT NULL DEFAULT 0,
    sex             INTEGER NOT NULL DEFAULT 0,
    class           INTEGER NOT NULL DEFAULT 0,
    level           INTEGER NOT NULL DEFAULT 0,
    trust           INTEGER NOT NULL DEFAULT 0,
    played          INTEGER NOT NULL DEFAULT 0,
    last_login      TIMESTAMP WITH TIME ZONE,
    hit             INTEGER NOT NULL DEFAULT 0,
    max_hit         INTEGER NOT NULL DEFAULT 0,
    mana            INTEGER NOT NULL DEFAULT 0,
    max_mana        INTEGER NOT NULL DEFAULT 0,
    move            INTEGER NOT NULL DEFAULT 0,
    max_move        INTEGER NOT NULL DEFAULT 0,
    gold            INTEGER NOT NULL DEFAULT 0,
    exp             INTEGER NOT NULL DEFAULT 0,
    act_flags       BIGINT  NOT NULL DEFAULT 0,
    affected_by     INTEGER NOT NULL DEFAULT 0,
    position        INTEGER NOT NULL DEFAULT 0,
    practice        INTEGER NOT NULL DEFAULT 0,
    quest_points    INTEGER NOT NULL DEFAULT 0,
    str             INTEGER NOT NULL DEFAULT 0,
    int_            INTEGER NOT NULL DEFAULT 0,
    wis             INTEGER NOT NULL DEFAULT 0,
    dex             INTEGER NOT NULL DEFAULT 0,
    con             INTEGER NOT NULL DEFAULT 0,
    str_mod         INTEGER NOT NULL DEFAULT 0,
    int_mod         INTEGER NOT NULL DEFAULT 0,
    wis_mod         INTEGER NOT NULL DEFAULT 0,
    dex_mod         INTEGER NOT NULL DEFAULT 0,
    con_mod         INTEGER NOT NULL DEFAULT 0,
    skills          JSONB   NOT NULL DEFAULT '{}',
    affects         JSONB   NOT NULL DEFAULT '[]',
    inventory       JSONB   NOT NULL DEFAULT '[]',
    raw_save        TEXT
);

-- -----------------------------------------------------------------------
-- 4.29 keep_chests
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS keep_chests (
    id           SERIAL  PRIMARY KEY,
    vnum         INTEGER NOT NULL UNIQUE,
    owner_name   TEXT    NOT NULL,
    max_items    INTEGER NOT NULL DEFAULT 50,
    created_at   TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    updated_at   TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);

-- -----------------------------------------------------------------------
-- 4.30 keep_chest_items
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS keep_chest_items (
    id          SERIAL  PRIMARY KEY,
    chest_id    INTEGER NOT NULL REFERENCES keep_chests (id) ON DELETE CASCADE,
    nest        INTEGER NOT NULL DEFAULT 0,
    parent_id   INTEGER REFERENCES keep_chest_items (id),
    name        TEXT    NOT NULL,
    short_descr TEXT    NOT NULL,
    description TEXT    NOT NULL,
    vnum        INTEGER NOT NULL DEFAULT 0,
    extra_flags BIGINT  NOT NULL DEFAULT 0,
    wear_flags  INTEGER NOT NULL DEFAULT 0,
    wear_loc    INTEGER NOT NULL DEFAULT -1,
    class_flags INTEGER NOT NULL DEFAULT 0,
    item_type   INTEGER NOT NULL DEFAULT 0,
    weight      INTEGER NOT NULL DEFAULT 0,
    level       INTEGER NOT NULL DEFAULT 0,
    timer       INTEGER NOT NULL DEFAULT -1,
    cost        INTEGER NOT NULL DEFAULT 0,
    value_0     INTEGER NOT NULL DEFAULT 0,
    value_1     INTEGER NOT NULL DEFAULT 0,
    value_2     INTEGER NOT NULL DEFAULT 0,
    value_3     INTEGER NOT NULL DEFAULT 0,
    value_4     INTEGER NOT NULL DEFAULT 0,
    value_5     INTEGER NOT NULL DEFAULT 0,
    value_6     INTEGER NOT NULL DEFAULT 0,
    value_7     INTEGER NOT NULL DEFAULT 0,
    value_8     INTEGER NOT NULL DEFAULT 0,
    value_9     INTEGER NOT NULL DEFAULT 0,
    objfun      TEXT,
    sort_order  INTEGER NOT NULL DEFAULT 0
);

-- -----------------------------------------------------------------------
-- 4.31 schema_version
-- -----------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS schema_version (
    version    INTEGER                  NOT NULL,
    applied_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);
INSERT INTO schema_version (version) VALUES (2);

COMMIT;
