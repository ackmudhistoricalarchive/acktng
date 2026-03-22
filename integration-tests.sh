#!/bin/sh
#
# integration-tests.sh — Parallel integration test runner with PostgreSQL.
#
# Creates an ephemeral database in the running PostgreSQL cluster, populates
# it via import_to_db, then runs all three integration tests in parallel
# against the same DB:
#   - integration-test.sh          (WebSocket login)
#   - integration-test-telnet.sh   (pure telnet login)
#   - integration-test-tls.sh      (TLS telnet login)
# Tears down the test database on exit.
#
# Requires a running PostgreSQL cluster accessible by the postgres OS user.
# Temporarily adds a local trust auth rule so the MUD process (which may run
# as a different OS user) can connect as the 'ack' database user.
#
# Exit codes:
#   0 - all integration tests passed
#   1 - PostgreSQL setup failed, or one or more tests failed

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
AREA_DIR="$SCRIPT_DIR/area"
DATA_DIR="$SCRIPT_DIR/data"
DB_CONF="$DATA_DIR/db.conf"
DB_CONF_BAK="$DATA_DIR/db.conf.integration-bak"

# Unique test database name to avoid collisions between parallel CI runs.
TEST_DB="acktng_test_$$"

# pg_hba.conf path (works for Debian/Ubuntu postgresql packages).
PG_HBA=""
for candidate in \
    /etc/postgresql/16/main/pg_hba.conf \
    /etc/postgresql/15/main/pg_hba.conf \
    /etc/postgresql/14/main/pg_hba.conf \
    /var/lib/pgsql/data/pg_hba.conf; do
    if [ -f "$candidate" ]; then
        PG_HBA="$candidate"
        break
    fi
done

# ---------------------------------------------------------------------------
# Locate PostgreSQL client binaries (prefer pg16 path, fall back to PATH).
# ---------------------------------------------------------------------------
PG_BIN=""
for candidate in \
    /usr/lib/postgresql/16/bin \
    /usr/lib/postgresql/15/bin \
    /usr/lib/postgresql/14/bin \
    /usr/local/lib/postgresql/bin \
    /usr/local/pgsql/bin; do
    if [ -x "$candidate/psql" ]; then
        PG_BIN="$candidate"
        break
    fi
done

if [ -z "$PG_BIN" ] && command -v psql >/dev/null 2>&1; then
    PG_BIN="$(dirname "$(command -v psql)")"
fi

if [ -z "$PG_BIN" ]; then
    echo "integration-tests: FAILED - cannot find psql; install postgresql-client"
    exit 1
fi

PSQL="$PG_BIN/psql"
CREATEDB="$PG_BIN/createdb"
DROPDB="$PG_BIN/dropdb"

# Run a command as the postgres OS user.
pg_as_postgres() {
    if [ "$(id -u)" -eq 0 ]; then
        su -s /bin/sh -c "$*" postgres
    else
        sudo -u postgres sh -c "$*"
    fi
}

# ---------------------------------------------------------------------------
# Cleanup helper.
# ---------------------------------------------------------------------------
DB_CREATED=0
HBA_MODIFIED=0
cleanup() {
    # Kill any test-script subprocesses left over.
    for pid in $TEST1_PID $TEST2_PID $TEST3_PID; do
        [ -n "$pid" ] && kill "$pid" 2>/dev/null || true
    done

    # Drop the ephemeral test database.
    if [ "$DB_CREATED" -eq 1 ]; then
        pg_as_postgres "$DROPDB $TEST_DB" 2>/dev/null || true
        DB_CREATED=0
    fi

    # Restore pg_hba.conf and reload if we modified it.
    if [ "$HBA_MODIFIED" -eq 1 ] && [ -n "$PG_HBA" ]; then
        if [ -f "${PG_HBA}.integration-bak" ]; then
            cp "${PG_HBA}.integration-bak" "$PG_HBA"
            if command -v pg_ctlcluster >/dev/null 2>&1; then
                pg_ctlcluster 16 main reload 2>/dev/null || \
                pg_ctlcluster 15 main reload 2>/dev/null || true
            fi
        fi
        HBA_MODIFIED=0
    fi

    # Restore data/db.conf from backup (or remove our temporary one).
    if [ -f "$DB_CONF_BAK" ]; then
        mv "$DB_CONF_BAK" "$DB_CONF"
    else
        rm -f "$DB_CONF"
    fi

    rm -f "/tmp/mud-it1-$$.log" "/tmp/mud-it2-$$.log" "/tmp/mud-it3-$$.log"
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Step 1: build MUD and import_to_db tool.
# ---------------------------------------------------------------------------
echo "integration-tests: building MUD..."
if ! (cd "$SRC_DIR" && make ack); then
    echo "integration-tests: FAILED - MUD build failed"
    exit 1
fi

echo "integration-tests: building import_to_db tool..."
if ! (cd "$SRC_DIR" && make tools/import_to_db); then
    echo "integration-tests: FAILED - import_to_db build failed"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 2: add trust auth rule so MUD process can connect as 'ack'.
# ---------------------------------------------------------------------------
if [ -n "$PG_HBA" ]; then
    cp "$PG_HBA" "${PG_HBA}.integration-bak"
    # Prepend a trust rule so it takes precedence over peer/scram rules.
    printf 'local all ack trust\n' | cat - "$PG_HBA" > /tmp/pg_hba_tmp_$$ && \
        cp /tmp/pg_hba_tmp_$$ "$PG_HBA" && rm -f /tmp/pg_hba_tmp_$$
    HBA_MODIFIED=1
    if command -v pg_ctlcluster >/dev/null 2>&1; then
        pg_ctlcluster 16 main reload 2>/dev/null || \
        pg_ctlcluster 15 main reload 2>/dev/null || true
    fi
fi

# ---------------------------------------------------------------------------
# Step 3: create test database, apply schema, run import.
# ---------------------------------------------------------------------------
echo "integration-tests: creating test database '$TEST_DB'..."
if ! pg_as_postgres "$CREATEDB -U postgres -O ack $TEST_DB" 2>/dev/null; then
    echo "integration-tests: FAILED - createdb failed"
    exit 1
fi
DB_CREATED=1

echo "integration-tests: applying schema..."
if ! pg_as_postgres "$PSQL -U postgres -d $TEST_DB -f $AREA_DIR/schema.sql -q" 2>/dev/null; then
    echo "integration-tests: FAILED - schema apply failed"
    exit 1
fi

# Grant all privileges on all tables/sequences to the 'ack' user.
pg_as_postgres "$PSQL -U postgres -d $TEST_DB -q -c \
    'GRANT ALL ON ALL TABLES IN SCHEMA public TO ack; \
     GRANT ALL ON ALL SEQUENCES IN SCHEMA public TO ack;'" 2>/dev/null || true

# Write db.conf (back up any existing one first).
if [ -f "$DB_CONF" ]; then
    cp "$DB_CONF" "$DB_CONF_BAK"
fi
printf 'dbname=%s user=ack\n' "$TEST_DB" > "$DB_CONF"

echo "integration-tests: importing flat-file data into database..."
IMPORT_LOG="/tmp/mud-import-$$.log"
if ! (cd "$AREA_DIR" && ../src/tools/import_to_db \
        "dbname=$TEST_DB user=ack" >"$IMPORT_LOG" 2>&1); then
    echo "integration-tests: FAILED - import_to_db failed:"
    cat "$IMPORT_LOG"
    rm -f "$IMPORT_LOG"
    exit 1
fi
rm -f "$IMPORT_LOG"

# ---------------------------------------------------------------------------
# Step 4: run all three integration tests in parallel.
# ---------------------------------------------------------------------------
echo "integration-tests: launching WebSocket, Telnet, and TLS tests in parallel..."

LOG1="/tmp/mud-it1-$$.log"
LOG2="/tmp/mud-it2-$$.log"
LOG3="/tmp/mud-it3-$$.log"

"$SCRIPT_DIR/integration-test.sh" >"$LOG1" 2>&1 &
TEST1_PID=$!

"$SCRIPT_DIR/integration-test-telnet.sh" >"$LOG2" 2>&1 &
TEST2_PID=$!

"$SCRIPT_DIR/integration-test-tls.sh" >"$LOG3" 2>&1 &
TEST3_PID=$!

# ---------------------------------------------------------------------------
# Step 5: wait for all three and collect results.
# ---------------------------------------------------------------------------
FAIL=0

wait "$TEST1_PID"
TEST1_STATUS=$?
if [ "$TEST1_STATUS" -ne 0 ]; then
    echo "integration-tests: integration-test.sh (WebSocket) FAILED (exit $TEST1_STATUS):"
    cat "$LOG1"
    FAIL=1
else
    tail -1 "$LOG1"
fi

wait "$TEST2_PID"
TEST2_STATUS=$?
if [ "$TEST2_STATUS" -ne 0 ]; then
    echo "integration-tests: integration-test-telnet.sh FAILED (exit $TEST2_STATUS):"
    cat "$LOG2"
    FAIL=1
else
    tail -1 "$LOG2"
fi

wait "$TEST3_PID"
TEST3_STATUS=$?
if [ "$TEST3_STATUS" -ne 0 ]; then
    echo "integration-tests: integration-test-tls.sh FAILED (exit $TEST3_STATUS):"
    cat "$LOG3"
    FAIL=1
else
    tail -1 "$LOG3"
fi

if [ "$FAIL" -eq 0 ]; then
    echo "integration-tests: all tests passed."
fi

exit "$FAIL"
