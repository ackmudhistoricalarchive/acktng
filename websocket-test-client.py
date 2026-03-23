"""
WebSocket test client for ACK!MUD integration testing.

Usage: python3 websocket-test-client.py <port> <player> <password>

Connects to 127.0.0.1:<port> via WebSocket and walks the new-player
creation flow.  Exits 0 on success, 1 on failure.
"""

import base64
import os
import socket
import sys
import time

PORT     = int(sys.argv[1])
PLAYER   = sys.argv[2]
PASSWORD = sys.argv[3]
# Pass --existing as the 4th argument to use the existing-player login flow
# (password prompt directly, no character-creation menu).
EXISTING = len(sys.argv) > 4 and sys.argv[4] == '--existing'

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

# Walk the login flow.
data = ws.recv_until('name', timeout=10.0)
if 'name' not in data.lower():
    fail("expected name prompt in greeting", data)

send(PLAYER)

if EXISTING:
    # Existing-player flow: server sends "Password: " directly (no Y/N confirmation).
    data = ws.recv_until('assword', timeout=5.0)
    if 'assword' not in data:
        fail("expected password prompt for existing player", data)

    send(PASSWORD)

    # The nanny() login state machine falls through from CON_GET_OLD_PASSWORD directly
    # into CON_READ_MOTD in the same call, so the MOTD, Welcome message, room description,
    # and prompt are all flushed in a single WebSocket frame.  No Enter press needed.
    data = ws.recv_until('welcome', timeout=10.0)
    if 'welcome' not in data.lower():
        fail("expected 'Welcome' message after entering game", data)

    time.sleep(2.0)
    print(f"integration-test: websocket login successful - existing player '{PLAYER}' reached playing state and stayed connected for 2s", flush=True)

else:
    # New-player creation flow.
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
    send('War Mag Cle Sen')
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
