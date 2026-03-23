#!/bin/sh
#
# Integration test: build, start, boot, log in as a new player via WebSocket,
# and run for 2 seconds checking for crashes.
#
# Exit codes:
#   0 - MUD booted, accepted a player login, and ran without crashing
#   1 - build failed, MUD crashed, or the login happy-path was not reached

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
AREA_DIR="$SCRIPT_DIR/area"
PLAYER_DIR="$SCRIPT_DIR/player"
RUN_SECONDS=2
LOG_FILE="/tmp/mud-integration-test-$$.log"

# Test player name (3-12 alpha chars, not a reserved name, unlikely to clash
# with any mob name).
TEST_PLAYER="Integrat"
TEST_PASSWORD="integrationpass"

# Pre-seeded existing player used for the existing-character login test.
# Room is ROOM_VNUM_SCHOOL (4900) so the character starts in the school.
SEED_PLAYER="Loadchar"
SEED_PASSWORD="loadcharpass"

# Ask the OS for a free ephemeral port to avoid collisions on shared CI hosts.
if command -v python3 >/dev/null 2>&1; then
    TEST_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
    HTTP_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
else
    TEST_PORT=$((RANDOM % 16383 + 49152))
    HTTP_PORT=$((RANDOM % 16383 + 49152))
fi

# ---------------------------------------------------------------------------
# Cleanup helper – always runs on exit to stop a stray server process.
# ---------------------------------------------------------------------------
cleanup() {
    if [ -n "$MUD_PID" ] && kill -0 "$MUD_PID" 2>/dev/null; then
        kill "$MUD_PID" 2>/dev/null || true
        wait "$MUD_PID" 2>/dev/null || true
    fi
    rm -f "$LOG_FILE"
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Step 1: build
# ---------------------------------------------------------------------------
echo "integration-test: building MUD..."
if ! (cd "$SRC_DIR" && make ack); then
    echo "integration-test: FAILED - build step failed"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 2: remove any leftover player files so the login flows are always the
# new-character path (idempotent test runs).
# Also seed the pre-existing player file used by the existing-player login test.
# ---------------------------------------------------------------------------
player_lower=$(echo "$TEST_PLAYER" | tr '[:upper:]' '[:lower:]')
first_letter=$(echo "$player_lower" | cut -c1)
rm -f "$PLAYER_DIR/$first_letter/$player_lower"

seed_lower=$(echo "$SEED_PLAYER" | tr '[:upper:]' '[:lower:]')
seed_first=$(echo "$seed_lower" | cut -c1)
rm -f "$PLAYER_DIR/$seed_first/${SEED_PLAYER}"
python3 "$SCRIPT_DIR/seed-test-player.py" "$PLAYER_DIR" "$SEED_PLAYER" "$SEED_PASSWORD"

# ---------------------------------------------------------------------------
# Step 3: launch
# The startup script runs the binary from the area/ directory so that
# relative paths to area files, data files, and player directories resolve
# correctly.
# ---------------------------------------------------------------------------
echo "integration-test: starting MUD on port $TEST_PORT..."
(cd "$AREA_DIR" && ../src/ack "$TEST_PORT" --http-port "$HTTP_PORT") >"$LOG_FILE" 2>&1 &
MUD_PID=$!

echo "integration-test: MUD started (PID $MUD_PID), waiting for boot..."

# ---------------------------------------------------------------------------
# Step 4: wait until the server is ready (game loop started, max 90 s).
# ---------------------------------------------------------------------------
boot_wait=0
while [ "$boot_wait" -lt 90 ]; do
    if grep -q "MUD is ready on port" "$LOG_FILE" 2>/dev/null; then
        break
    fi
    if ! kill -0 "$MUD_PID" 2>/dev/null; then
        wait "$MUD_PID"
        echo "integration-test: FAILED - MUD crashed during boot"
        echo "--- MUD output ---"
        cat "$LOG_FILE"
        echo "------------------"
        exit 1
    fi
    sleep 1
    boot_wait=$((boot_wait + 1))
done

if [ "$boot_wait" -ge 90 ]; then
    echo "integration-test: FAILED - MUD did not reach ready state after 90s"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

echo "integration-test: MUD is up, validating websocket login flow for '${TEST_PLAYER}'..."

# ---------------------------------------------------------------------------
# Step 5: WebSocket handshake + walk the new-player login flow (happy path).
# ---------------------------------------------------------------------------
python3 "$SCRIPT_DIR/websocket-test-client.py" "$TEST_PORT" "$TEST_PLAYER" "$TEST_PASSWORD"

LOGIN_STATUS=$?
if [ "$LOGIN_STATUS" -ne 0 ]; then
    echo "integration-test: FAILED - websocket login sequence did not complete"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 5b: WebSocket login as the pre-seeded existing character.
# ---------------------------------------------------------------------------
echo "integration-test: validating websocket login flow for existing player '${SEED_PLAYER}'..."
python3 "$SCRIPT_DIR/websocket-test-client.py" "$TEST_PORT" "$SEED_PLAYER" "$SEED_PASSWORD" --existing

SEED_LOGIN_STATUS=$?
if [ "$SEED_LOGIN_STATUS" -ne 0 ]; then
    echo "integration-test: FAILED - existing-player login sequence did not complete"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 6: let the MUD keep running and watch for crashes.
# ---------------------------------------------------------------------------
echo "integration-test: monitoring MUD for ${RUN_SECONDS}s..."
elapsed=0
crashed=0
while [ "$elapsed" -lt "$RUN_SECONDS" ]; do
    sleep 1
    elapsed=$((elapsed + 1))
    if ! kill -0 "$MUD_PID" 2>/dev/null; then
        crashed=1
        break
    fi
done

if [ "$crashed" -eq 1 ]; then
    wait "$MUD_PID"
    exit_code=$?
    echo "integration-test: FAILED - MUD exited after ${elapsed}s (exit code ${exit_code})"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

echo "integration-test: MUD ran for ${RUN_SECONDS}s without crashing, stopping..."
# cleanup trap will kill the server and remove the log file
exit 0
