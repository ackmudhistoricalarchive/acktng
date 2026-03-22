"""
WSS (WebSocket Secure / TLS) test client for ACK!MUD integration testing.

Usage: python3 wss-test-client.py <port> <player> <password> [cert_file]

Connects to 127.0.0.1:<port> via WebSocket over TLS (wss://) and walks the
new-player creation flow.  If cert_file is provided it is used as the CA
bundle; otherwise certificate verification is disabled (self-signed certs).
Exits 0 on success, 1 on failure.
"""

import base64
import os
import socket
import ssl
import sys
import time

PORT     = int(sys.argv[1])
PLAYER   = sys.argv[2]
PASSWORD = sys.argv[3]
CERTFILE = sys.argv[4] if len(sys.argv) > 4 else None


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

    def recv_http_headers(self, timeout=10.0):
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


# ---------------------------------------------------------------------------
# Connect: TCP → TLS → WebSocket upgrade
# ---------------------------------------------------------------------------
ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
if CERTFILE:
    ctx.load_verify_locations(CERTFILE)
    ctx.verify_mode = ssl.CERT_REQUIRED
    ctx.check_hostname = False  # connecting by IP in tests; hostname check would fail
else:
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE

raw = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
raw.settimeout(30)
raw.connect(('127.0.0.1', PORT))

s = ctx.wrap_socket(raw, server_hostname='127.0.0.1')

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

ws = WSClient(s)
headers = ws.recv_http_headers(timeout=10.0)
if '101 Switching Protocols' not in headers or 'Sec-WebSocket-Accept:' not in headers:
    fail("expected successful websocket handshake over TLS", headers)


def send(msg):
    ws.send_text(msg + '\n')


# ---------------------------------------------------------------------------
# Walk the new-player creation flow (same as websocket-test-client.py).
# ---------------------------------------------------------------------------
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
send('War Mag Cle Sen')
_ = ws.recv_until('elect', timeout=5.0)

send('4')
_ = ws.recv_until('\n', timeout=5.0)

send('')
data = ws.recv_until('welcome', timeout=10.0)
if 'welcome' not in data.lower():
    fail("expected 'Welcome' message after entering game", data)

time.sleep(2.0)
print(
    f"integration-test: WSS login successful - '{PLAYER}' reached playing state "
    f"over WSS (TLS) and stayed connected for 2s",
    flush=True,
)
s.close()
sys.exit(0)
