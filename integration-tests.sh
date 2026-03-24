#!/bin/sh
#
# integration-tests.sh — Parallel integration test runner with PostgreSQL.
#
# Creates an ephemeral database from fixtures/test_data.sql, then runs all
# four integration tests in parallel against it:
#   - integration-test.sh              (WebSocket login)
#   - integration-test-telnet.sh       (pure telnet login)
#   - integration-test-telnet-tls.sh   (TLS telnet login)
#   - integration-test-wss.sh          (WSS login)
# Tears down the test database on exit.
#
# Admin operations (createdb, schema, grants) run as the postgres OS user
# via "sudo -u postgres" so no password is required for the superuser.
# The MUD processes connect to the test DB via TCP with a known password
# (already permitted by the default pg_hba.conf scram-sha-256 rule).
# ACK_DB_CONF is exported pointing to a temp file so each MUD sub-process
# picks up the test connection string; the production data/db.conf is
# never modified.
#
# Prerequisites:
#   - PostgreSQL installed and running locally (port 5432)
#   - sudo access to run commands as the 'postgres' OS user
#   - ACK_DB_PASSWORD env var (default: acktest) used as the ack role's password
#
# The 'ack' database role is created automatically if it does not exist.
#
# Exit codes:
#   0 - all integration tests passed
#   1 - PostgreSQL setup failed, or one or more tests failed

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
AREA_DIR="$SCRIPT_DIR/area"
DATA_DIR="$SCRIPT_DIR/data"
FIXTURE="$SCRIPT_DIR/fixtures/test_data.sql"
# Temp file for the test DB connection string; exported as ACK_DB_CONF so
# the MUD processes find it without touching the production data/db.conf.
TEST_DB_CONF="/tmp/acktng-test-db-$$.conf"

DB_USER="${ACK_DB_USER:-ack}"
DB_PASS="${ACK_DB_PASSWORD:-acktest}"

# Unique test database name to avoid collisions between parallel CI runs.
TEST_DB="acktng_test_$$"

# ---------------------------------------------------------------------------
# Locate PostgreSQL client binaries.
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

# Run a command as the postgres OS user (peer auth via Unix socket).
pg_as_postgres() {
    if [ "$(id -u)" -eq 0 ]; then
        su -s /bin/sh postgres -c "$*"
    else
        sudo -u postgres sh -c "$*"
    fi
}

# ---------------------------------------------------------------------------
# Cleanup helper.
# ---------------------------------------------------------------------------
DB_CREATED=0
cleanup() {
    for pid in $TEST1_PID $TEST2_PID $TEST3_PID $TEST4_PID; do
        [ -n "$pid" ] && kill "$pid" 2>/dev/null || true
    done

    if [ "$DB_CREATED" -eq 1 ]; then
        pg_as_postgres "$DROPDB $TEST_DB" 2>/dev/null || true
        DB_CREATED=0
    fi

    rm -f "$TEST_DB_CONF"

    rm -f "/tmp/mud-it1-$$.log" "/tmp/mud-it2-$$.log" \
          "/tmp/mud-it3-$$.log" "/tmp/mud-it4-$$.log"
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Step 1: build MUD.
# ---------------------------------------------------------------------------
echo "integration-tests: building MUD..."
if ! (cd "$SRC_DIR" && make ack); then
    echo "integration-tests: FAILED - MUD build failed"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 2: verify PostgreSQL is reachable and ensure the ack role exists.
# ---------------------------------------------------------------------------
echo "integration-tests: checking PostgreSQL..."
if ! pg_as_postgres "$PSQL -c '\\q'" 2>/dev/null; then
    echo "integration-tests: FAILED - cannot connect to PostgreSQL (is it installed and running?)"
    exit 1
fi

if ! pg_as_postgres "$PSQL -tAc \"SELECT 1 FROM pg_roles WHERE rolname='$DB_USER'\"" 2>/dev/null | grep -q 1; then
    echo "integration-tests: creating database role '$DB_USER'..."
    if ! pg_as_postgres "$PSQL -c \"CREATE ROLE $DB_USER WITH LOGIN PASSWORD '$DB_PASS'\"" 2>/dev/null; then
        echo "integration-tests: FAILED - could not create role '$DB_USER'"
        exit 1
    fi
fi

# ---------------------------------------------------------------------------
# Step 3: create test database, apply schema, grant privileges.
# ---------------------------------------------------------------------------
echo "integration-tests: creating test database '$TEST_DB'..."
if ! pg_as_postgres "$CREATEDB -O $DB_USER $TEST_DB" 2>/dev/null; then
    echo "integration-tests: FAILED - createdb failed"
    exit 1
fi
DB_CREATED=1

echo "integration-tests: applying schema..."
if ! cat "$AREA_DIR/schema.sql" | pg_as_postgres "$PSQL -d $TEST_DB -q" 2>/dev/null; then
    echo "integration-tests: FAILED - schema apply failed"
    exit 1
fi

# Pipe the GRANT statement so postgres user doesn't need file-system access.
printf 'GRANT ALL ON ALL TABLES IN SCHEMA public TO %s;\nGRANT ALL ON ALL SEQUENCES IN SCHEMA public TO %s;\n' \
    "$DB_USER" "$DB_USER" | pg_as_postgres "$PSQL -d $TEST_DB -q" 2>/dev/null || true

# ---------------------------------------------------------------------------
# Step 4: load fixture data (MUD user connects via TCP with password).
# ---------------------------------------------------------------------------
echo "integration-tests: loading fixture data..."
if ! PGPASSWORD="$DB_PASS" "$PSQL" -h localhost -U "$DB_USER" \
        -d "$TEST_DB" -f "$FIXTURE" -q 2>/dev/null; then
    echo "integration-tests: FAILED - fixture load failed"
    exit 1
fi

# Write a temp conf file and export ACK_DB_CONF so every MUD process spawned
# by the sub-tests uses the test DB.  The production data/db.conf is not
# touched at all.
printf 'host=localhost dbname=%s user=%s password=%s\n' \
    "$TEST_DB" "$DB_USER" "$DB_PASS" > "$TEST_DB_CONF"
export ACK_DB_CONF="$TEST_DB_CONF"

# Tell sub-tests to skip their own build step — we already built above, and
# parallel `make ack` invocations race on the binary ("Text file busy").
export ACK_SKIP_BUILD=1

# ---------------------------------------------------------------------------
# Step 5: run all four integration tests in parallel.
# ---------------------------------------------------------------------------
echo "integration-tests: launching WebSocket, Telnet, TLS, and WSS tests in parallel..."

LOG1="/tmp/mud-it1-$$.log"
LOG2="/tmp/mud-it2-$$.log"
LOG3="/tmp/mud-it3-$$.log"
LOG4="/tmp/mud-it4-$$.log"

"$SCRIPT_DIR/integration-test.sh" >"$LOG1" 2>&1 &
TEST1_PID=$!

"$SCRIPT_DIR/integration-test-telnet.sh" >"$LOG2" 2>&1 &
TEST2_PID=$!

"$SCRIPT_DIR/integration-test-telnet-tls.sh" >"$LOG3" 2>&1 &
TEST3_PID=$!

"$SCRIPT_DIR/integration-test-wss.sh" >"$LOG4" 2>&1 &
TEST4_PID=$!

# ---------------------------------------------------------------------------
# Step 6: wait for all four and collect results.
# ---------------------------------------------------------------------------
FAIL=0

wait "$TEST1_PID"
if [ $? -ne 0 ]; then
    echo "integration-tests: integration-test.sh (WebSocket) FAILED:"
    cat "$LOG1"
    FAIL=1
else
    tail -1 "$LOG1"
fi

wait "$TEST2_PID"
if [ $? -ne 0 ]; then
    echo "integration-tests: integration-test-telnet.sh FAILED:"
    cat "$LOG2"
    FAIL=1
else
    tail -1 "$LOG2"
fi

wait "$TEST3_PID"
if [ $? -ne 0 ]; then
    echo "integration-tests: integration-test-telnet-tls.sh FAILED:"
    cat "$LOG3"
    FAIL=1
else
    tail -1 "$LOG3"
fi

wait "$TEST4_PID"
if [ $? -ne 0 ]; then
    echo "integration-tests: integration-test-wss.sh FAILED:"
    cat "$LOG4"
    FAIL=1
else
    tail -1 "$LOG4"
fi

if [ "$FAIL" -eq 0 ]; then
    echo "integration-tests: all tests passed."
fi

exit "$FAIL"
