#!/bin/sh
#
# Integration test: build, start, boot, log in as a new player via TLS
# (telnet-over-TLS on the dedicated TLS port), and run for 2 seconds
# checking for crashes.
#
# Skipped gracefully if OpenSSL is unavailable or TLS was not compiled in.
#
# Exit codes:
#   0 - MUD booted, accepted a TLS login, and ran without crashing
#   1 - build failed, MUD crashed, or the login happy-path was not reached

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
AREA_DIR="$SCRIPT_DIR/area"
PLAYER_DIR="$SCRIPT_DIR/player"
RUN_SECONDS=2
LOG_FILE="/tmp/mud-integration-test-tls-$$.log"

# Unique player name to avoid save-file collisions with other tests.
TEST_PLAYER="Tlsconn"
TEST_PASSWORD="tlspassword"

# Ask the OS for free ephemeral ports.
if command -v python3 >/dev/null 2>&1; then
    WS_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
    TLS_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
    HTTP_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
else
    WS_PORT=$((RANDOM % 16383 + 49152))
    TLS_PORT=$((RANDOM % 16383 + 49152))
    HTTP_PORT=$((RANDOM % 16383 + 49152))
fi

# ---------------------------------------------------------------------------
# Generate a self-signed certificate for this run.
# If openssl is unavailable or cert generation fails, skip the test.
# ---------------------------------------------------------------------------
TLS_CERT="/tmp/mud-tls-cert-$$.pem"
TLS_KEY="/tmp/mud-tls-key-$$.pem"
HAS_TLS=0

if command -v openssl >/dev/null 2>&1; then
    if (openssl genrsa -traditional -out "$TLS_KEY" 2048 2>/dev/null || \
        openssl genrsa -out "$TLS_KEY" 2048 2>/dev/null) && \
       openssl req -x509 -new -key "$TLS_KEY" -out "$TLS_CERT" \
           -days 1 -subj '/CN=localhost' >/dev/null 2>&1; then
        HAS_TLS=1
    fi
fi

if [ "$HAS_TLS" -eq 0 ]; then
    echo "integration-test-tls: OpenSSL unavailable or cert generation failed; skipping TLS test."
    exit 0
fi

# ---------------------------------------------------------------------------
# Cleanup helper.
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
# Step 1: build (skipped when ACK_SKIP_BUILD=1, e.g. called from integration-tests.sh)
# ---------------------------------------------------------------------------
if [ "${ACK_SKIP_BUILD:-0}" != "1" ]; then
    echo "integration-test-tls: building MUD..."
    if ! (cd "$SRC_DIR" && make ack); then
        echo "integration-test-tls: FAILED - build step failed"
        exit 1
    fi
fi

# ---------------------------------------------------------------------------
# Step 2: remove any leftover player files (idempotent test runs).
# ---------------------------------------------------------------------------
player_lower=$(echo "$TEST_PLAYER" | tr '[:upper:]' '[:lower:]')
first_letter=$(echo "$player_lower" | cut -c1)
rm -f "$PLAYER_DIR/$first_letter/$player_lower"

# ---------------------------------------------------------------------------
# Step 3: launch with a dedicated TLS port.
# ---------------------------------------------------------------------------
echo "integration-test-tls: starting MUD on WS port $WS_PORT, TLS port $TLS_PORT..."
(cd "$AREA_DIR" && ../src/ack "$WS_PORT" \
    --tls-port "$TLS_PORT" --tls-cert "$TLS_CERT" --tls-key "$TLS_KEY" \
    --http-port "$HTTP_PORT") >"$LOG_FILE" 2>&1 &
MUD_PID=$!

echo "integration-test-tls: MUD started (PID $MUD_PID), waiting for boot..."

# ---------------------------------------------------------------------------
# Step 4: wait until the server is ready (max 90 s).
# ---------------------------------------------------------------------------
boot_wait=0
while [ "$boot_wait" -lt 90 ]; do
    if grep -q "MUD is ready on port" "$LOG_FILE" 2>/dev/null; then
        break
    fi
    if ! kill -0 "$MUD_PID" 2>/dev/null; then
        wait "$MUD_PID"
        echo "integration-test-tls: FAILED - MUD crashed during boot"
        echo "--- MUD output ---"
        cat "$LOG_FILE"
        echo "------------------"
        exit 1
    fi
    sleep 1
    boot_wait=$((boot_wait + 1))
done

if [ "$boot_wait" -ge 90 ]; then
    echo "integration-test-tls: FAILED - MUD did not reach ready state after 90s"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

# Verify the TLS port actually opened (requires TLS compiled in + cert loaded).
if ! grep -q "(TLS)" "$LOG_FILE" 2>/dev/null; then
    echo "integration-test-tls: TLS port not active (OpenSSL not compiled in?); skipping TLS test."
    exit 0
fi

echo "integration-test-tls: MUD is up, validating TLS login flow for '${TEST_PLAYER}'..."

# ---------------------------------------------------------------------------
# Step 5: TLS telnet new-player login flow.
# ---------------------------------------------------------------------------
python3 "$SCRIPT_DIR/tls-test-client.py" "$TLS_PORT" "$TEST_PLAYER" "$TEST_PASSWORD"

LOGIN_STATUS=$?
if [ "$LOGIN_STATUS" -ne 0 ]; then
    echo "integration-test-tls: FAILED - TLS login sequence did not complete"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

echo "integration-test-tls: TLS login successful - '${TEST_PLAYER}' reached playing state over TLS and stayed connected for 2s"

# ---------------------------------------------------------------------------
# Step 6: let the MUD keep running and watch for crashes.
# ---------------------------------------------------------------------------
echo "integration-test-tls: monitoring MUD for ${RUN_SECONDS}s..."
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
    echo "integration-test-tls: FAILED - MUD exited after ${elapsed}s (exit code ${exit_code})"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

echo "integration-test-tls: MUD ran for ${RUN_SECONDS}s without crashing, stopping..."
exit 0
