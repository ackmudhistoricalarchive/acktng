#!/bin/sh
#
# Shared helpers for integration test scripts.
#
# Callers must set before sourcing or before calling setup_test_environment:
#   SCRIPT_DIR   — absolute path to the repo root
#   AREA_DIR     — $SCRIPT_DIR/area
#   TEST_DIR     — mktemp -d scratch directory
#   TEST_AREA_DIR, TEST_DATA_DIR, TEST_PLAYER_DIR — subdirs of TEST_DIR

# ---------------------------------------------------------------------------
# setup_test_environment
#
# Populates TEST_DIR with an isolated server environment:
#   area/   — symlinks to every file in the real area/
#   data/   — symlinks to every file in the real data/ except chest/ and
#             db.conf (chest/ is replaced with an empty directory so no
#             production keep-chest flat files are loaded; db.conf is
#             excluded so the server never auto-connects to a production
#             database when ACK_DB_CONF is absent)
#   help    — symlink to the real help/ directory
#   shelp   — symlink to the real shelp/ directory
#   player/ — fresh directory tree with per-letter subdirectories (a-z)
# ---------------------------------------------------------------------------
setup_test_environment() {
    mkdir -p "$TEST_AREA_DIR"
    for f in "$AREA_DIR"/*; do
        ln -s "$f" "$TEST_AREA_DIR/$(basename "$f")"
    done

    mkdir -p "$TEST_DATA_DIR"
    for f in "$SCRIPT_DIR/data"/*; do
        bname=$(basename "$f")
        [ "$bname" = "chest" ]  && continue
        [ "$bname" = "db.conf" ] && continue
        ln -s "$f" "$TEST_DATA_DIR/$bname"
    done
    mkdir -p "$TEST_DATA_DIR/chest"

    ln -s "$SCRIPT_DIR/help"  "$TEST_DIR/help"
    ln -s "$SCRIPT_DIR/shelp" "$TEST_DIR/shelp"

    mkdir -p "$TEST_PLAYER_DIR"
    for letter in a b c d e f g h i j k l m n o p q r s t u v w x y z; do
        mkdir -p "$TEST_PLAYER_DIR/$letter"
    done
}
