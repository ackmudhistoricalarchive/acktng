#!/bin/sh
#
# Integration test: build, start, boot, log in as a new player via raw
# telnet (plain TCP), and run for 2 seconds checking for crashes.
#
# Exit codes:
#   0 - MUD booted, accepted a player login via telnet, and ran without crashing
#   1 - build failed, MUD crashed, or the login happy-path was not reached

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
AREA_DIR="$SCRIPT_DIR/area"
PLAYER_DIR="$SCRIPT_DIR/player"
RUN_SECONDS=2
LOG_FILE="/tmp/mud-integration-test-telnet-$$.log"

# Different player name from other tests to avoid save-file collisions.
TEST_PLAYER="Telnetrat"
TEST_PASSWORD="telnetpass"

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
    rm -f "$LOG_FILE"
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Step 1: build
# ---------------------------------------------------------------------------
echo "integration-test-telnet: building MUD..."
if ! (cd "$SRC_DIR" && make ack); then
    echo "integration-test-telnet: FAILED - build step failed"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 2: remove any leftover player files (idempotent test runs).
# ---------------------------------------------------------------------------
remove_player_file "$TEST_PLAYER"

# ---------------------------------------------------------------------------
# Step 3: launch on sniff port so plain-telnet clients can connect.
# ---------------------------------------------------------------------------
echo "integration-test-telnet: starting MUD on port $TEST_PORT..."
(cd "$AREA_DIR" && ../src/ack --sniff-port "$TEST_PORT" --http-port "$HTTP_PORT") >"$LOG_FILE" 2>&1 &
MUD_PID=$!

echo "integration-test-telnet: MUD started (PID $MUD_PID), waiting for boot..."

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
        echo "integration-test-telnet: FAILED - MUD crashed during boot"
        echo "--- MUD output ---"
        cat "$LOG_FILE"
        echo "------------------"
        exit 1
    fi
    sleep 1
    boot_wait=$((boot_wait + 1))
done

if [ "$boot_wait" -ge 90 ]; then
    echo "integration-test-telnet: FAILED - MUD did not reach ready state after 90s"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

echo "integration-test-telnet: MUD is up, validating telnet login flow for '${TEST_PLAYER}'..."

# ---------------------------------------------------------------------------
# Step 5: raw telnet (plain TCP) new-player login flow.
# ---------------------------------------------------------------------------
python3 - "$TEST_PORT" "$TEST_PLAYER" "$TEST_PASSWORD" <<'PYEOF'
import socket
import sys
import time

PORT     = int(sys.argv[1])
PLAYER   = sys.argv[2]
PASSWORD = sys.argv[3]

# Telnet control bytes
IAC  = 0xFF
WILL = 0xFB
WONT = 0xFC
DO   = 0xFD
DONT = 0xFE
SB   = 0xFA  # subnegotiation begin
SE   = 0xF0  # subnegotiation end

def fail(msg, context=""):
    print(f"integration-test-telnet: FAILED - {msg}", flush=True)
    if context:
        print(f"  received: {repr(context[-300:])}", flush=True)
    sys.exit(1)

class TelnetClient:
    def __init__(self, sock):
        self.s = sock
        self.raw = bytearray()   # unprocessed bytes from the wire
        self.text = ""           # decoded, IAC-stripped, ANSI-stripped text

    def _refill(self, timeout):
        self.s.settimeout(timeout)
        try:
            chunk = self.s.recv(4096)
        except socket.timeout:
            return
        if chunk:
            self.raw.extend(chunk)

    def _process_raw(self):
        """Parse self.raw: handle IAC sequences, decode the rest to text."""
        out = bytearray()
        i = 0
        while i < len(self.raw):
            b = self.raw[i]
            if b == IAC:
                if i + 1 >= len(self.raw):
                    break  # need more data
                cmd = self.raw[i + 1]
                if cmd == IAC:
                    # Escaped IAC – literal 0xFF character
                    out.append(IAC)
                    i += 2
                elif cmd in (WILL, WONT, DO, DONT):
                    if i + 2 >= len(self.raw):
                        break  # need more data
                    opt = self.raw[i + 2]
                    # Decline all options
                    if cmd == WILL:
                        self.s.sendall(bytes([IAC, DONT, opt]))
                    elif cmd == DO:
                        self.s.sendall(bytes([IAC, WONT, opt]))
                    i += 3
                elif cmd == SB:
                    # Subnegotiation: skip until IAC SE
                    end = i + 2
                    while end + 1 < len(self.raw):
                        if self.raw[end] == IAC and self.raw[end + 1] == SE:
                            end += 2
                            break
                        end += 1
                    else:
                        break  # need more data
                    i = end
                else:
                    # 2-byte command we don't handle; skip it
                    i += 2
            else:
                out.append(b)
                i += 1
        del self.raw[:i]

        # Decode to string, then strip ANSI escape sequences
        raw_str = out.decode('latin-1', errors='replace')
        import re
        clean = re.sub(r'\x1b\[[0-9;]*[A-Za-z]', '', raw_str)
        clean = re.sub(r'\x1b[^[\\[]', '', clean)
        self.text += clean

    def recv_until(self, marker, timeout=10.0):
        marker_lower = marker.lower()
        deadline = time.time() + timeout
        while time.time() < deadline:
            self._process_raw()
            if marker_lower in self.text.lower():
                snapshot = self.text
                self.text = ""
                return snapshot
            remaining = deadline - time.time()
            if remaining <= 0:
                break
            self._refill(min(0.25, remaining))
        # Return whatever we have even if marker not found
        snapshot = self.text
        self.text = ""
        return snapshot

    def send(self, msg):
        self.s.sendall((msg + '\r\n').encode('latin-1'))

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', PORT))
client = TelnetClient(s)

# Wait for the greeting; ends with "What is your name?"
data = client.recv_until('name', timeout=10.0)
if 'name' not in data.lower():
    fail("expected name prompt in greeting", data)

client.send(PLAYER)
data = client.recv_until('y/n', timeout=5.0)
if 'y/n' not in data.lower():
    fail("expected name confirmation (Y/N)", data)

client.send('Y')
data = client.recv_until('assword', timeout=5.0)
if 'assword' not in data:
    fail("expected password prompt", data)

client.send(PASSWORD)
data = client.recv_until('etype', timeout=5.0)
if 'etype' not in data:
    fail("expected retype-password prompt", data)

client.send(PASSWORD)
data = client.recv_until('elect', timeout=5.0)
if 'elect' not in data.lower():
    fail("expected character-creation menu", data)

client.send('1')
_ = client.recv_until('ex', timeout=5.0)
client.send('M')
_ = client.recv_until('elect', timeout=5.0)

client.send('2')
_ = client.recv_until('ace', timeout=5.0)
client.send('Hmn')
_ = client.recv_until('elect', timeout=5.0)

client.send('3')
_ = client.recv_until('lass', timeout=5.0)
client.send('War Mag Cle Sen')
_ = client.recv_until('elect', timeout=5.0)

client.send('4')
_ = client.recv_until('\n', timeout=5.0)

client.send('')
data = client.recv_until('welcome', timeout=10.0)
if 'welcome' not in data.lower():
    fail("expected 'Welcome' message after entering game", data)

time.sleep(2.0)
print(f"integration-test-telnet: telnet login successful - '{PLAYER}' reached playing state and stayed connected for 2s", flush=True)
s.close()
sys.exit(0)
PYEOF

LOGIN_STATUS=$?
if [ "$LOGIN_STATUS" -ne 0 ]; then
    echo "integration-test-telnet: FAILED - telnet login sequence did not complete"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

# ---------------------------------------------------------------------------
# Step 6: let the MUD keep running and watch for crashes.
# ---------------------------------------------------------------------------
echo "integration-test-telnet: monitoring MUD for ${RUN_SECONDS}s..."
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
    echo "integration-test-telnet: FAILED - MUD exited after ${elapsed}s (exit code ${exit_code})"
    echo "--- MUD output ---"
    cat "$LOG_FILE"
    echo "------------------"
    exit 1
fi

echo "integration-test-telnet: MUD ran for ${RUN_SECONDS}s without crashing, stopping..."
# cleanup trap will kill the server and remove the log file
exit 0
