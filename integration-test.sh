#!/bin/sh
#
# Integration test: build, start, boot, log in as a new player, validate
# WebSocket connectivity, and run for 2 seconds checking for crashes.
#
# Exit codes:
#   0 - MUD booted, accepted a player login, and ran without crashing
#   1 - build failed, MUD crashed, or the login happy-path was not reached

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
AREA_DIR="$SCRIPT_DIR/area"
NPC_DIR="$SCRIPT_DIR/npcs"
RUN_SECONDS=2
LOG_FILE="/tmp/mud-integration-test-$$.log"

# Test player name (3-12 alpha chars, not a reserved name, unlikely to clash
# with any mob name).
TEST_PLAYER="Integrat"
TEST_PASSWORD="integrationpass"

# Ask the OS for a free ephemeral port to avoid collisions on shared CI hosts.
if command -v python3 >/dev/null 2>&1; then
    TEST_PORT=$(python3 -c \
        "import socket; s=socket.socket(); s.bind(('', 0)); print(s.getsockname()[1]); s.close()")
else
    TEST_PORT=$((RANDOM % 16383 + 49152))
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
# Step 2: remove any leftover player file so the login flow is always the
# new-character path (idempotent test runs).
# ---------------------------------------------------------------------------
player_lower=$(echo "$TEST_PLAYER" | tr '[:upper:]' '[:lower:]')
first_letter=$(echo "$player_lower" | cut -c1)
rm -f "$NPC_DIR/$first_letter/$player_lower"

# ---------------------------------------------------------------------------
# Step 3: launch
# The startup script runs the binary from the area/ directory so that
# relative paths to area files, data files, and player directories resolve
# correctly.
# ---------------------------------------------------------------------------
echo "integration-test: starting MUD on port $TEST_PORT..."
(cd "$AREA_DIR" && ../src/ack "$TEST_PORT") >"$LOG_FILE" 2>&1 &
MUD_PID=$!

echo "integration-test: MUD started (PID $MUD_PID), waiting for boot..."

# ---------------------------------------------------------------------------
# Step 4: wait until the server is accepting TCP connections (max 30 s).
# ---------------------------------------------------------------------------
boot_wait=0
while [ "$boot_wait" -lt 30 ]; do
    if python3 -c "
import socket, sys
s = socket.socket()
s.settimeout(1)
try:
    s.connect(('127.0.0.1', $TEST_PORT))
    s.close()
    sys.exit(0)
except Exception:
    sys.exit(1)
" 2>/dev/null; then
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

if [ "$boot_wait" -ge 30 ]; then
    echo "integration-test: FAILED - MUD did not accept connections after 30s"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

echo "integration-test: MUD is up, logging in as new player '${TEST_PLAYER}'..."

# ---------------------------------------------------------------------------
# Step 5: walk the new-player login flow (happy path).
#   greeting0 ends with "What is your name?"
#   CON_CONFIRM_NEW_NAME  -> "Did I get that right, <name> (Y/N)?"
#   CON_GET_NEW_PASSWORD  -> "Give me a password for <name>:"
#   CON_CONFIRM_NEW_PASSWORD -> "Please retype password:"
#   CON_MENU (show_menu_to) -> shows numbered options including "elect"
#     1 -> sex menu     -> send M
#     2 -> race menu    -> send Hmn  (Human, 3-letter race_name)
#     3 -> class menu   -> send "War Mag Cle Thi"  (MAX_PC_CLASS = 4)
#     4 -> CON_READ_MOTD
#   CON_READ_MOTD         -> any input -> CON_PLAYING
#   CON_PLAYING           -> server sends "Welcome to ACK!TNG" + room look
# ---------------------------------------------------------------------------
python3 - "$TEST_PORT" "$TEST_PLAYER" "$TEST_PASSWORD" <<'PYEOF'
import socket
import sys
import time

PORT     = int(sys.argv[1])
PLAYER   = sys.argv[2]
PASSWORD = sys.argv[3]

def recv_until(s, marker, timeout=10.0):
    """Read bytes until *marker* appears or *timeout* elapses."""
    s.settimeout(0.25)
    data = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
            if marker.encode('latin-1') in data:
                return data.decode('latin-1', errors='replace')
        except socket.timeout:
            pass
    return data.decode('latin-1', errors='replace')

def send(s, msg):
    s.sendall((msg + '\r\n').encode('latin-1'))

def fail(msg, context=""):
    print(f"integration-test: FAILED - {msg}", flush=True)
    if context:
        print(f"  received: {repr(context[-300:])}", flush=True)
    sys.exit(1)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', PORT))

# Wait for the greeting to finish; greeting0 ends with "What is your name?"
data = recv_until(s, 'name', timeout=10.0)
if 'name' not in data.lower():
    fail("expected name prompt in greeting", data)

# --- Send player name -------------------------------------------------------
send(s, PLAYER)
data = recv_until(s, 'Y/N', timeout=5.0)
if 'Y/N' not in data:
    fail("expected name confirmation (Y/N)", data)

# --- Confirm name -----------------------------------------------------------
send(s, 'Y')
data = recv_until(s, 'assword', timeout=5.0)
if 'assword' not in data:
    fail("expected password prompt", data)

# --- Set password -----------------------------------------------------------
send(s, PASSWORD)
data = recv_until(s, 'etype', timeout=5.0)
if 'etype' not in data:
    fail("expected retype-password prompt", data)

# --- Confirm password -------------------------------------------------------
send(s, PASSWORD)
data = recv_until(s, 'elect', timeout=5.0)   # "Select" in the menu
if 'elect' not in data.lower():
    fail("expected character-creation menu", data)

# --- Pick sex (option 1) → M ------------------------------------------------
send(s, '1')
data = recv_until(s, 'ex', timeout=5.0)      # "sex"/"Sex"
send(s, 'M')
data = recv_until(s, 'elect', timeout=5.0)

# --- Pick race (option 2) → Human (3-letter code "Hmn") --------------------
send(s, '2')
data = recv_until(s, 'ace', timeout=5.0)     # "race"/"Race"
send(s, 'Hmn')
data = recv_until(s, 'elect', timeout=5.0)

# --- Pick class order (option 3) → four classes by abbreviation -------------
send(s, '3')
data = recv_until(s, 'lass', timeout=5.0)    # "class"/"Class"
send(s, 'War Mag Cle Thi')
data = recv_until(s, 'elect', timeout=5.0)

# --- Enter game (option 4) --------------------------------------------------
send(s, '4')
# Server sends MOTD / newun help text then waits in CON_READ_MOTD
data = recv_until(s, '\n', timeout=5.0)

# --- Pass the MOTD ----------------------------------------------------------
send(s, '')
data = recv_until(s, 'Welcome', timeout=10.0)
if 'Welcome' not in data:
    fail("expected 'Welcome' message after entering game", data)

print(f"integration-test: login successful - '{PLAYER}' reached playing state",
      flush=True)
s.close()
sys.exit(0)
PYEOF

LOGIN_STATUS=$?
if [ "$LOGIN_STATUS" -ne 0 ]; then
    echo "integration-test: FAILED - login sequence did not complete"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 6: validate WebSocket connectivity (connect + handshake + hold 2s).
# ---------------------------------------------------------------------------
echo "integration-test: validating WebSocket connectivity for 2s..."
python3 - "$TEST_PORT" <<'PYEOF'
import base64
import os
import socket
import sys
import time

PORT = int(sys.argv[1])

def fail(msg, context=""):
    print(f"integration-test: FAILED - {msg}", flush=True)
    if context:
        print(f"  context: {context}", flush=True)
    sys.exit(1)

key = base64.b64encode(os.urandom(16)).decode("ascii")
request = (
    "GET / HTTP/1.1\r\n"
    f"Host: 127.0.0.1:{PORT}\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    f"Sec-WebSocket-Key: {key}\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "\r\n"
)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(5.0)
s.connect(("127.0.0.1", PORT))
s.sendall(request.encode("ascii"))

response = b""
deadline = time.time() + 5.0
while b"\r\n\r\n" not in response and time.time() < deadline:
    chunk = s.recv(4096)
    if not chunk:
        break
    response += chunk

decoded = response.decode("latin-1", errors="replace")
if "101 Switching Protocols" not in decoded:
    fail("expected HTTP 101 Switching Protocols from websocket handshake", decoded[:300])
if "Sec-WebSocket-Accept:" not in decoded:
    fail("expected Sec-WebSocket-Accept header", decoded[:300])

time.sleep(2.0)
s.close()
print("integration-test: websocket handshake successful and connection held for 2s", flush=True)
sys.exit(0)
PYEOF

WEBSOCKET_STATUS=$?
if [ "$WEBSOCKET_STATUS" -ne 0 ]; then
    echo "integration-test: FAILED - websocket connectivity check failed"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 7: let the MUD keep running and watch for crashes.
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
