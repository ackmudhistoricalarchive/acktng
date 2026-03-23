#!/bin/sh
#
# Integration test: WSS (WebSocket Secure) login flow.
#
# Builds the server, starts it with --wss-port (direct TLS WebSocket,
# mirroring production port 9891), walks a full new-player login over WSS,
# and monitors for crashes.
#
# Port assignments (production):
#   8890  plain telnet  ← integration-test-telnet.sh
#   9890  TLS telnet    ← integration-test-telnet-tls.sh
#   9891  WSS           ← this test
#   18890 WebSocket     ← integration-test.sh (ws://)
#
# If openssl is not available the test is skipped (exit 0).
#
# Exit codes:
#   0 - MUD booted, accepted a player login via WSS, and ran without crashing
#       (or openssl unavailable and test was skipped)
#   1 - build failed, MUD crashed, or the login happy-path was not reached

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
AREA_DIR="$SCRIPT_DIR/area"
RUN_SECONDS=2
LOG_FILE="/tmp/mud-integration-test-wss-$$.log"

# Isolated scratch directory so the server never touches production data.
TEST_DIR=$(mktemp -d)
TEST_AREA_DIR="$TEST_DIR/area"
TEST_DATA_DIR="$TEST_DIR/data"
TEST_PLAYER_DIR="$TEST_DIR/player"

WSS_TEST_PLAYER="Wssplayer"
WSS_TEST_PASSWORD="wsspass"

TLS_CERT="/tmp/mud-wss-cert-$$.pem"
TLS_KEY="/tmp/mud-wss-key-$$.pem"

if command -v python3 >/dev/null 2>&1; then
    PLAIN_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
    WSS_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
    HTTP_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
else
    PLAIN_PORT=$((RANDOM % 16383 + 49152))
    WSS_PORT=$((RANDOM % 16383 + 49152))
    HTTP_PORT=$((RANDOM % 16383 + 49152))
fi

remove_player_file() {
    _lower=$(echo "$1" | tr '[:upper:]' '[:lower:]')
    rm -f "$TEST_PLAYER_DIR/$(echo "$_lower" | cut -c1)/$_lower"
}

cleanup() {
    if [ -n "$MUD_PID" ] && kill -0 "$MUD_PID" 2>/dev/null; then
        kill "$MUD_PID" 2>/dev/null || true
        wait "$MUD_PID" 2>/dev/null || true
    fi
    rm -f "$LOG_FILE" "$TLS_CERT" "$TLS_KEY"
    rm -rf "$TEST_DIR"
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Step 1: check availability
# ---------------------------------------------------------------------------
if ! command -v openssl >/dev/null 2>&1; then
    echo "integration-test-wss: openssl not available; skipping WSS test."
    exit 0
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "integration-test-wss: python3 not available; skipping WSS test."
    exit 0
fi

if ! openssl req -x509 -newkey rsa:2048 \
       -keyout "$TLS_KEY" -out "$TLS_CERT" \
       -days 1 -nodes -subj '/CN=localhost' \
       -addext 'subjectAltName=IP:127.0.0.1,DNS:localhost' >/dev/null 2>&1; then
    # Older openssl without -addext: try without SAN (Python will skip verification)
    if ! openssl req -x509 -newkey rsa:2048 \
           -keyout "$TLS_KEY" -out "$TLS_CERT" \
           -days 1 -nodes -subj '/CN=localhost' >/dev/null 2>&1; then
        echo "integration-test-wss: cert generation failed; skipping WSS test."
        exit 0
    fi
fi

# ---------------------------------------------------------------------------
# Step 2: build
# ---------------------------------------------------------------------------
echo "integration-test-wss: building MUD..."
if ! (cd "$SRC_DIR" && make ack); then
    echo "integration-test-wss: FAILED - build step failed"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 3: set up isolated test environment.
# ---------------------------------------------------------------------------
. "$SCRIPT_DIR/test-helpers.sh"
setup_test_environment

# ---------------------------------------------------------------------------
# Step 4: remove any leftover player files.
# ---------------------------------------------------------------------------
remove_player_file "$WSS_TEST_PLAYER"

# ---------------------------------------------------------------------------
# Step 5: launch from the isolated area directory.
# ---------------------------------------------------------------------------
echo "integration-test-wss: starting MUD on plain $PLAIN_PORT / WSS $WSS_PORT..."
(cd "$TEST_AREA_DIR" && "$SRC_DIR/ack" "$PLAIN_PORT" \
    --wss-port "$WSS_PORT" --tls-cert "$TLS_CERT" --tls-key "$TLS_KEY" \
    --http-port "$HTTP_PORT") >"$LOG_FILE" 2>&1 &
MUD_PID=$!

echo "integration-test-wss: MUD started (PID $MUD_PID), waiting for boot..."

boot_wait=0
while [ "$boot_wait" -lt 90 ]; do
    if grep -q "MUD is ready" "$LOG_FILE" 2>/dev/null; then
        break
    fi
    if ! kill -0 "$MUD_PID" 2>/dev/null; then
        wait "$MUD_PID"
        echo "integration-test-wss: FAILED - MUD crashed during boot"
        cat "$LOG_FILE"
        exit 1
    fi
    sleep 1
    boot_wait=$((boot_wait + 1))
done

if [ "$boot_wait" -ge 90 ]; then
    echo "integration-test-wss: FAILED - MUD did not reach ready state after 90s"
    cat "$LOG_FILE"
    exit 1
fi

if ! grep -q "(WSS)" "$LOG_FILE" 2>/dev/null; then
    echo "integration-test-wss: FAILED - WSS port not active in server log"
    cat "$LOG_FILE"
    exit 1
fi

echo "integration-test-wss: MUD is up, validating WSS login for '${WSS_TEST_PLAYER}'..."

# ---------------------------------------------------------------------------
# Step 5: WSS login via wss-test-client.py
# ---------------------------------------------------------------------------
if ! python3 "$SCRIPT_DIR/wss-test-client.py" "$WSS_PORT" "$WSS_TEST_PLAYER" "$WSS_TEST_PASSWORD" "$TLS_CERT"; then
    echo "integration-test-wss: FAILED - WSS login did not complete"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 6: let the MUD keep running and watch for crashes.
# ---------------------------------------------------------------------------
echo "integration-test-wss: monitoring MUD for ${RUN_SECONDS}s..."
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
    echo "integration-test-wss: FAILED - MUD exited after ${elapsed}s (exit code ${exit_code})"
    cat "$LOG_FILE"
    exit 1
fi

echo "integration-test-wss: MUD ran for ${RUN_SECONDS}s without crashing, stopping..."
exit 0
