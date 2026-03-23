#!/bin/sh
#
# Integration test: TLS telnet login flow (Mudlet "connect securely").
#
# Builds the server, starts it with a dedicated --tls-port (mirroring
# production port 9890), walks a full new-player login over TLS, and
# monitors for crashes.  Plain telnet and WebSocket are covered by
# separate integration tests.
#
# Port assignments (production):
#   8890  plain telnet  ← integration-test-telnet.sh
#   9890  TLS telnet    ← this test
#   18890 WebSocket     ← integration-test.sh (ws://)
#
# If openssl is not available the test is skipped (exit 0).
#
# Exit codes:
#   0 - MUD booted, accepted a player login via TLS telnet, and ran without crashing
#       (or openssl unavailable and test was skipped)
#   1 - build failed, MUD crashed, or the login happy-path was not reached

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
AREA_DIR="$SCRIPT_DIR/area"
PLAYER_DIR="$SCRIPT_DIR/player"
RUN_SECONDS=2
LOG_FILE="/tmp/mud-integration-test-telnet-tls-$$.log"

TLS_TEST_PLAYER="Tlstelnet"
TLS_TEST_PASSWORD="tlspass"

TLS_CERT="/tmp/mud-tls-cert-$$.pem"
TLS_KEY="/tmp/mud-tls-key-$$.pem"

# Ask the OS for free ephemeral ports to avoid collisions on shared CI hosts.
if command -v python3 >/dev/null 2>&1; then
    PLAIN_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
    TLS_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
    HTTP_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
else
    PLAIN_PORT=$((RANDOM % 16383 + 49152))
    TLS_PORT=$((RANDOM % 16383 + 49152))
    HTTP_PORT=$((RANDOM % 16383 + 49152))
fi

# ---------------------------------------------------------------------------
# remove_player_file <NAME> — delete saved player file so every run starts
# the new-character login flow (idempotent test runs).
# ---------------------------------------------------------------------------
remove_player_file() {
    _lower=$(echo "$1" | tr '[:upper:]' '[:lower:]')
    rm -f "$PLAYER_DIR/$(echo "$_lower" | cut -c1)/$_lower"
}

# ---------------------------------------------------------------------------
# Cleanup helper – always runs on exit to stop a stray server process.
# ---------------------------------------------------------------------------
cleanup() {
    if [ -n "$MUD_PID" ] && kill -0 "$MUD_PID" 2>/dev/null; then
        kill "$MUD_PID" 2>/dev/null || true
        wait "$MUD_PID" 2>/dev/null || true
    fi
    rm -f "$LOG_FILE" "$TLS_CERT" "$TLS_KEY"
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Step 1: check openssl availability
# ---------------------------------------------------------------------------
if ! command -v openssl >/dev/null 2>&1; then
    echo "integration-test-telnet-tls: openssl not available; skipping TLS test."
    exit 0
fi

if ! openssl req -x509 -newkey rsa:2048 \
       -keyout "$TLS_KEY" -out "$TLS_CERT" \
       -days 1 -nodes -subj '/CN=localhost' >/dev/null 2>&1; then
    echo "integration-test-telnet-tls: cert generation failed; skipping TLS test."
    exit 0
fi

# ---------------------------------------------------------------------------
# Step 2: build
# ---------------------------------------------------------------------------
echo "integration-test-telnet-tls: building MUD..."
if ! (cd "$SRC_DIR" && make ack); then
    echo "integration-test-telnet-tls: FAILED - build step failed"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 3: remove any leftover player files.
# ---------------------------------------------------------------------------
remove_player_file "$TLS_TEST_PLAYER"

# ---------------------------------------------------------------------------
# Step 4: launch
#
# $PLAIN_PORT is a required positional arg (plain telnet listener).
# $TLS_PORT mirrors production port 9890 (Mudlet "connect securely").
# ---------------------------------------------------------------------------
echo "integration-test-telnet-tls: starting MUD on plain $PLAIN_PORT / TLS $TLS_PORT..."
(cd "$AREA_DIR" && ../src/ack "$PLAIN_PORT" \
    --tls-port "$TLS_PORT" --tls-cert "$TLS_CERT" --tls-key "$TLS_KEY" \
    --http-port "$HTTP_PORT") >"$LOG_FILE" 2>&1 &
MUD_PID=$!

echo "integration-test-telnet-tls: MUD started (PID $MUD_PID), waiting for boot..."

# ---------------------------------------------------------------------------
# Step 5: wait until the server is ready (max 90 s).
# ---------------------------------------------------------------------------
boot_wait=0
while [ "$boot_wait" -lt 90 ]; do
    if grep -q "MUD is ready on port" "$LOG_FILE" 2>/dev/null; then
        break
    fi
    if ! kill -0 "$MUD_PID" 2>/dev/null; then
        wait "$MUD_PID"
        echo "integration-test-telnet-tls: FAILED - MUD crashed during boot"
        echo "--- MUD output ---"
        cat "$LOG_FILE"
        echo "------------------"
        exit 1
    fi
    sleep 1
    boot_wait=$((boot_wait + 1))
done

if [ "$boot_wait" -ge 90 ]; then
    echo "integration-test-telnet-tls: FAILED - MUD did not reach ready state after 90s"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

# Confirm TLS port is reported active in the server log.
if ! grep -q "(TLS)" "$LOG_FILE" 2>/dev/null; then
    echo "integration-test-telnet-tls: FAILED - TLS port not active in server log"
    cat "$LOG_FILE"
    exit 1
fi

echo "integration-test-telnet-tls: MUD is up, validating TLS login for '${TLS_TEST_PLAYER}'..."

# ---------------------------------------------------------------------------
# Step 6: TLS telnet login via tls-test-client.py (self-signed cert accepted).
# ---------------------------------------------------------------------------
if ! python3 "$SCRIPT_DIR/tls-test-client.py" "$TLS_PORT" "$TLS_TEST_PLAYER" "$TLS_TEST_PASSWORD"; then
    echo "integration-test-telnet-tls: FAILED - TLS login did not complete"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 7: let the MUD keep running and watch for crashes.
# ---------------------------------------------------------------------------
echo "integration-test-telnet-tls: monitoring MUD for ${RUN_SECONDS}s..."
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
    echo "integration-test-telnet-tls: FAILED - MUD exited after ${elapsed}s (exit code ${exit_code})"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

echo "integration-test-telnet-tls: MUD ran for ${RUN_SECONDS}s without crashing, stopping..."
exit 0
