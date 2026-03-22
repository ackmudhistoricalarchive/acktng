# Native WSS Port (`--wss-port`)

## Problem

The current WebSocket setup uses a **loopback-only** plain WS port (`--ws-loopback 18890`)
that requires an nginx reverse proxy to terminate TLS before forwarding to the game server:

```
Browser → wss://ackmud.com:9890 → nginx (TLS termination) → ws://127.0.0.1:18890 → server
```

This works, but it adds an operational dependency: nginx must be installed, configured,
and kept running. The server already has full OpenSSL/TLS support (used by `--tls-port`)
including certificate loading and non-blocking handshake advancement. Adding a native
`--wss-port` would allow the server to accept WSS connections directly, without a proxy.

## Goals

1. A browser can connect via `wss://ackmud.com:<port>/` directly to the game server
   with no nginx proxy in the path.
2. TLS is terminated by the server itself, reusing the same cert/key as `--tls-port`.
3. The WebSocket upgrade handshake and all existing WS protocol logic are unchanged.
4. The existing `--ws-loopback` option is retained for deployments that prefer the
   proxy model.
5. No new library dependencies — OpenSSL is already required when `HAVE_OPENSSL` is set.

## Port Assignment

A new fixed port:

| Port  | Protocol | Purpose          | Bind     |
|-------|----------|------------------|----------|
| 9891  | WSS      | Native WebSocket+TLS | `0.0.0.0` (public) |

This port is documented in `docs/ports.md` alongside the existing entries.

## How It Works

The existing code already has two independent mechanisms:

1. **TLS handshake** — triggered in `new_descriptor()` when `is_tls=TRUE`.
   After a successful TLS handshake, `read_from_descriptor` / `write_to_descriptor`
   use `SSL_read` / `SSL_write` transparently.

2. **WebSocket handshake** — triggered in `read_from_descriptor()` when
   `!d->websocket_handshake_complete`. If the incoming data starts with `"GET "`,
   `handle_websocket_handshake()` sends the HTTP 101 Upgrade response and sets
   `d->websocket_active = TRUE`. All subsequent I/O is then framed as WebSocket.

For a WSS connection these two mechanisms compose naturally:

```
accept() → TLS handshake (new_descriptor, is_tls=TRUE)
         → browser sends HTTP Upgrade over TLS
         → handle_websocket_handshake() responds with 101 (via SSL_write)
         → websocket_active = TRUE
         → all subsequent I/O: SSL_read/write + WebSocket frames
```

No new code path is needed beyond wiring up the new listening socket and passing
`is_tls=TRUE` when calling `new_descriptor` for it.

## src/ Changes

### 1. `src/comm.c` — parse `--wss-port`, open socket

Add argument parsing for `--wss-port <port>`:

```c
int wss_port = -1;

/* in argument parsing loop: */
else if (!strcmp(argv[i], "--wss-port") && i + 1 < argc)
{
    wss_port = atoi(argv[++i]);
    if (wss_port <= 1024)
    {
        fprintf(stderr, "--wss-port must be above 1024.\n");
        exit(1);
    }
}
```

Open the socket (public bind, TLS required):

```c
int control_wss = -1;
...
if (wss_port > 0)
{
#ifdef HAVE_OPENSSL
    if (tls_ctx_ok)
        control_wss = init_socket(wss_port, INADDR_ANY);
    else
        fprintf(stderr, "Warning: TLS context init failed; --wss-port ignored.\n");
#else
    fprintf(stderr, "Warning: OpenSSL not compiled in; --wss-port ignored.\n");
#endif
}
```

Pass `control_wss` and `wss_port` through to `game_loop()` and store in
`global_wss_port`.

### 2. `src/socket.c` — `game_loop()` handles a fifth control socket

Add `control_wss` parameter to `game_loop()`. In the select loop:

```c
if (control_wss >= 0)
{
    FD_SET(control_wss, &in_set);
    maxdesc = UMAX(maxdesc, control_wss);
}
...
if (control_wss >= 0 && FD_ISSET(control_wss, &in_set))
    new_descriptor(control_wss, TRUE, FALSE);   /* is_tls=TRUE, do_sniff=FALSE */
```

In the SIGUSR1 socket-reopen path, reopen `control_wss` just like `control_tls`:

```c
if (control_wss >= 0)
{
    close(control_wss);
    control_wss = init_socket(global_wss_port, INADDR_ANY);
}
```

### 3. `src/socket.c` — add `global_wss_port`

```c
int global_wss_port = -1;
```

### 4. `src/headers/socket.h` — export new globals and update signature

```c
extern int global_wss_port;
void game_loop(int control, int control_ws, int control_tls, int control_sniff,
               int control_http, int control_wss);
```

### 5. Startup scripts

**`startup`** (dev):
```sh
WSS_PORT="${WSS_PORT:-9891}"
...
../src/ack $PORT --ws-loopback $WS_PORT --wss-port $WSS_PORT \
    --tls-port $TLS_PORT --tls-cert ... --tls-key ...
```

**`scripts/startup`** (production):
```sh
wss_port=9891
...
../src/ack $telnet_port --ws-loopback $ws_port --wss-port $wss_port \
    --tls-port $tls_port --tls-cert ... --tls-key ...
```

### Affected Files

| File | Change |
|------|--------|
| `src/comm.c` | Parse `--wss-port`; open `control_wss`; pass to `game_loop()`; store `global_wss_port` |
| `src/socket.c` | `game_loop()` gains `control_wss` param; select loop; SIGUSR1 reopen; `global_wss_port` |
| `src/headers/socket.h` | Export `global_wss_port`; update `game_loop()` signature |
| `startup` | Add `WSS_PORT` variable; pass `--wss-port` |
| `scripts/startup` | Add `wss_port`; pass `--wss-port` |
| `docs/ports.md` | Document port 9891 (WSS) |

### Unit Test Coverage

- `--wss-port` is OpenSSL-gated; no new TLS logic is introduced.
- The existing `test_websocket_validation.c` and `test_websocket_sanitize.c` cover the
  WS handshake layer (unchanged).
- The existing `test_sniff_is_tls.c` covers TLS detection (unchanged).
- Integration test exercises full boot + WebSocket login, which exercises the code path
  end-to-end when `--ws-loopback` is active. Because `--wss-port` requires a real TLS
  cert the integration test continues to use the loopback path; no new integration test
  is required.

## Trade-offs

| Option | Pro | Con |
|--------|-----|-----|
| Native `--wss-port` (this proposal) | No nginx dependency for WSS; simpler deployment; consistent with `--tls-port` for telnet | TLS cert must be valid (Let's Encrypt or self-signed) for native WSS |
| nginx proxy (existing `--ws-loopback`) | Mature, widely understood; HTTP-layer logging and rate-limiting | nginx must be installed and kept running |

Both modes can coexist: deployments with nginx keep `--ws-loopback`; bare-metal/container
deployments without nginx can use `--wss-port` instead.

## Summary

The change is small and surgical — it wires up one more listening socket in the same
pattern as the existing `--tls-port` (TLS telnet). The WebSocket upgrade logic requires
no modifications because TLS is transparent to the layers above it.
