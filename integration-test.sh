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

echo "integration-test: MUD is up, validating websocket login flow for '${TEST_PLAYER}'..."

# ---------------------------------------------------------------------------
# Step 5: WebSocket handshake + walk the new-player login flow (happy path).
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
import base64
import os
import socket
import sys
import time

PORT     = int(sys.argv[1])
PLAYER   = sys.argv[2]
PASSWORD = sys.argv[3]

def fail(msg, context=""):
    print(f"integration-test: FAILED - {msg}", flush=True)
    if context:
        print(f"  received: {repr(context[-300:])}", flush=True)
    sys.exit(1)

class WSClient:
    def __init__(self, sock):
        self.s = sock
        self.pending = bytearray()

    def recv_exact(self, n, timeout=5.0):
        deadline = time.time() + timeout
        while len(self.pending) < n:
            remaining = deadline - time.time()
            if remaining <= 0:
                break
            self.s.settimeout(min(0.25, remaining))
            try:
                chunk = self.s.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                break
            self.pending.extend(chunk)
        if len(self.pending) < n:
            raise RuntimeError("socket closed before expected bytes")
        data = bytes(self.pending[:n])
        del self.pending[:n]
        return data

    def recv_http_headers(self, timeout=5.0):
        deadline = time.time() + timeout
        marker = b"\r\n\r\n"
        while marker not in self.pending:
            remaining = deadline - time.time()
            if remaining <= 0:
                break
            self.s.settimeout(min(0.25, remaining))
            try:
                chunk = self.s.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                break
            self.pending.extend(chunk)
        idx = self.pending.find(marker)
        if idx == -1:
            return ""
        end = idx + len(marker)
        headers = bytes(self.pending[:end])
        del self.pending[:end]
        return headers.decode('latin-1', errors='replace')

    def send_text(self, msg):
        payload = msg.encode('latin-1')
        mask = os.urandom(4)
        frame = bytearray()
        frame.append(0x81)
        length = len(payload)
        if length < 126:
            frame.append(0x80 | length)
        elif length <= 0xFFFF:
            frame.append(0x80 | 126)
            frame.extend(length.to_bytes(2, 'big'))
        else:
            raise RuntimeError("payload too large")
        frame.extend(mask)
        frame.extend(bytes(payload[i] ^ mask[i % 4] for i in range(length)))
        self.s.sendall(frame)

    def send_pong(self, payload=b""):
        mask = os.urandom(4)
        frame = bytearray()
        frame.append(0x8A)
        length = len(payload)
        if length < 126:
            frame.append(0x80 | length)
        elif length <= 0xFFFF:
            frame.append(0x80 | 126)
            frame.extend(length.to_bytes(2, 'big'))
        else:
            raise RuntimeError("pong payload too large")
        frame.extend(mask)
        frame.extend(bytes(payload[i] ^ mask[i % 4] for i in range(length)))
        self.s.sendall(frame)

    def read_text_frame(self, timeout=5.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                b0, b1 = self.recv_exact(2, timeout=deadline - time.time())
            except Exception:
                return ""

            opcode = b0 & 0x0F
            masked = (b1 & 0x80) != 0
            length = b1 & 0x7F
            if length == 126:
                length = int.from_bytes(self.recv_exact(2, timeout=deadline - time.time()), 'big')
            elif length == 127:
                length = int.from_bytes(self.recv_exact(8, timeout=deadline - time.time()), 'big')

            mask = self.recv_exact(4, timeout=deadline - time.time()) if masked else b""
            payload = self.recv_exact(length, timeout=deadline - time.time()) if length > 0 else b""
            if masked:
                payload = bytes(payload[i] ^ mask[i % 4] for i in range(length))

            if opcode == 0x8:
                return ""
            if opcode == 0x9:
                self.send_pong(payload)
                continue
            if opcode in (0x1, 0x0, 0x2):
                return payload.decode('latin-1', errors='replace')
        return ""

    def recv_until(self, marker, timeout=10.0):
        marker_lower = marker.lower()
        data = ""
        deadline = time.time() + timeout
        while time.time() < deadline:
            chunk = self.read_text_frame(timeout=min(0.5, deadline - time.time()))
            if chunk:
                data += chunk
                if marker_lower in data.lower():
                    return data
        return data

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', PORT))
ws = WSClient(s)

key = base64.b64encode(os.urandom(16)).decode('ascii')
request = (
    "GET / HTTP/1.1\r\n"
    f"Host: 127.0.0.1:{PORT}\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    f"Sec-WebSocket-Key: {key}\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "\r\n"
)
s.sendall(request.encode('ascii'))

headers = ws.recv_http_headers(timeout=5.0)
if '101 Switching Protocols' not in headers or 'Sec-WebSocket-Accept:' not in headers:
    fail("expected successful websocket handshake", headers)

def send(msg):
    ws.send_text(msg + '\n')

# Wait for the greeting to finish; greeting0 ends with "What is your name?"
data = ws.recv_until('name', timeout=10.0)
if 'name' not in data.lower():
    fail("expected name prompt in greeting", data)

send(PLAYER)
data = ws.recv_until('y/n', timeout=5.0)
if 'y/n' not in data.lower():
    fail("expected name confirmation (Y/N)", data)

send('Y')
data = ws.recv_until('assword', timeout=5.0)
if 'assword' not in data:
    fail("expected password prompt", data)

send(PASSWORD)
data = ws.recv_until('etype', timeout=5.0)
if 'etype' not in data:
    fail("expected retype-password prompt", data)

send(PASSWORD)
data = ws.recv_until('elect', timeout=5.0)
if 'elect' not in data.lower():
    fail("expected character-creation menu", data)

send('1')
_ = ws.recv_until('ex', timeout=5.0)
send('M')
_ = ws.recv_until('elect', timeout=5.0)

send('2')
_ = ws.recv_until('ace', timeout=5.0)
send('Hmn')
_ = ws.recv_until('elect', timeout=5.0)

send('3')
_ = ws.recv_until('lass', timeout=5.0)
send('War Mag Cle Thi')
_ = ws.recv_until('elect', timeout=5.0)

send('4')
_ = ws.recv_until('\n', timeout=5.0)

send('')
data = ws.recv_until('welcome', timeout=10.0)
if 'welcome' not in data.lower():
    fail("expected 'Welcome' message after entering game", data)

time.sleep(2.0)
print(f"integration-test: websocket login successful - '{PLAYER}' reached playing state and stayed connected for 2s", flush=True)
s.close()
sys.exit(0)
PYEOF

LOGIN_STATUS=$?
if [ "$LOGIN_STATUS" -ne 0 ]; then
    echo "integration-test: FAILED - websocket login sequence did not complete"
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
