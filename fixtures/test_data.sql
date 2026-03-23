-- Minimal integration-test fixture.
--
-- Provides only what the server needs to boot and place a newly-created
-- character.  One area containing ROOM_VNUM_SCHOOL (4900) is the only
-- room the new-player login path exercises.  One row each in socials,
-- help_entries, and shelp_entries validates those load paths without
-- importing the entire game world.
--
-- Tables seeded by the schema itself (sysdata, schema_version) are
-- intentionally absent here.

-- -----------------------------------------------------------------------
-- areas — one minimal area covering vnum 4900 (ROOM_VNUM_SCHOOL)
-- -----------------------------------------------------------------------
INSERT INTO areas
    (name, min_vnum, max_vnum, keyword, level_label,
     area_number, level_min, level_max, map_offset,
     reset_rate, reset_msg, owner, can_read, can_write, music, flags)
VALUES
    ('Test School', 4900, 4999, 'testschool', '{1 10}',
     1, 1, 10, 0,
     15, 'The test school resets.', 'virant', 'all', 'all', '', 0);

-- -----------------------------------------------------------------------
-- rooms — just the school entrance (ROOM_VNUM_SCHOOL = 4900)
-- -----------------------------------------------------------------------
INSERT INTO rooms (vnum, area_id, name, description, room_flags, sector_type)
VALUES (4900,
        (SELECT id FROM areas WHERE min_vnum = 4900),
        'The Test School',
        'A bare room used for integration testing.\n',
        0, 0);

-- -----------------------------------------------------------------------
-- socials — one entry to verify the load path
-- -----------------------------------------------------------------------
INSERT INTO socials (name, char_no_arg, others_no_arg)
VALUES ('smile', 'You smile happily.', '$n smiles happily.');

-- -----------------------------------------------------------------------
-- help_entries — greeting1..6 are required: queue_login_greeting() picks
-- a random one and its text is the only thing that sends "What is your
-- name?" to the connecting client.  One extra entry exercises the general
-- help-load path.
-- -----------------------------------------------------------------------
INSERT INTO help_entries (filename, level, keywords, body) VALUES
    ('greeting1', -1, 'greeting1', 'Welcome to ACK!TNG.\n\nWhat is your name?'),
    ('greeting2', -1, 'greeting2', 'Welcome to ACK!TNG.\n\nWhat is your name?'),
    ('greeting3', -1, 'greeting3', 'Welcome to ACK!TNG.\n\nWhat is your name?'),
    ('greeting4', -1, 'greeting4', 'Welcome to ACK!TNG.\n\nWhat is your name?'),
    ('greeting5', -1, 'greeting5', 'Welcome to ACK!TNG.\n\nWhat is your name?'),
    ('greeting6', -1, 'greeting6', 'Welcome to ACK!TNG.\n\nWhat is your name?'),
    ('test_help',  0, 'TEST',      'This is a test help entry.');

-- -----------------------------------------------------------------------
-- quest_templates — one entry to verify the load path
-- -----------------------------------------------------------------------
INSERT INTO quest_templates
    (id, title, prerequisite_template_id, type, num_targets, target_vnums,
     kill_needed, min_level, max_level, offerer_vnum,
     reward_gold, reward_qp, reward_exp,
     accept_message, completion_message)
VALUES
    (1, 'Test Quest', -1, 1, 1, '{4900}',
     3, 1, 10, 0,
     100, 1, 500,
     'Go defeat the test creatures.', 'Well done, adventurer.');

-- -----------------------------------------------------------------------
-- shelp_entries — one entry to verify the load path
-- -----------------------------------------------------------------------
INSERT INTO shelp_entries (filename, level, keywords, body)
VALUES ('test_shelp', 104, 'TESTSTAFF', 'This is a test staff help entry.\n');
