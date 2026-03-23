# Proposal: MUD Web Client with Rich Media Support

**Date:** 2026-03-23
**Status:** Open — pending discussion
**Repos affected:** `acktng` (server extensions, static file serving, media assets)

---

## 1. Problem

Players currently connect to ACK!MUD TNG via:
- A raw telnet client (no cross-platform GUI, no media support)
- Custom MUD clients like MUSHclient or Mudlet (desktop only, Windows-centric)
- The WebSocket protocol already implemented in the server, but with no first-party client

There is no official, polished client that works across all major platforms (Windows, macOS, Linux, Android, iOS) and in a web browser without installation. The server already speaks WebSocket and WSS, already sends V2 JSON room/map data, and already has a music protocol — but these capabilities have no front end to surface them. We need a first-party web client that exposes all of this and adds support for graphics, video, and sound.

---

## 2. Goals

- **Universal**: Works in any modern browser (Chrome, Firefox, Safari, Edge). No plugins required.
- **Cross-platform installable**: Can be installed as a native-like app on Windows, macOS, Linux, Android, and iOS via Progressive Web App (PWA) technology.
- **Rich media**: Supports background music, sound effects, video cutscenes, and graphical overlays.
- **Structured UI**: Uses the V2 JSON protocol to populate a live room panel and minimap alongside the text terminal.
- **Maintainable**: Built with vanilla HTML/CSS/JavaScript (no build pipeline required) so it can be maintained without Node.js tooling.

---

## 3. Approach Overview

A single-page web application (SPA) built as a Progressive Web App:

| Layer | Technology | Rationale |
|-------|-----------|-----------|
| Terminal display | [xterm.js](https://xtermjs.org/) | Full ANSI/VT100 color support; scrollback buffer; keyboard handling |
| WebSocket connection | Native browser `WebSocket` API | No library needed; already supported by the server |
| Audio | Web Audio API + `<audio>` element | Handles music + SFX; works across all modern browsers |
| Video | HTML5 `<video>` element | Universally supported; no plugin required |
| Graphics / minimap | HTML5 `<canvas>` | Hardware-accelerated; no library needed for 2D tile rendering |
| Layout | CSS Grid + Flexbox | Responsive; no framework needed |
| PWA packaging | Service Worker + Web App Manifest | Install-to-homescreen on all platforms |

The client lives in a `web/client/` directory in this repo and is served as static files. No build step (no Webpack, Vite, etc.) is required — all JS is plain ES2020 modules.

### Platform Delivery

| Platform | Delivery method |
|----------|----------------|
| Any browser | Visit URL — works immediately |
| Android / iOS | "Add to Home Screen" (PWA) — full-screen, offline-capable |
| Windows / macOS / Linux | PWA install via Chrome/Edge "Install App" prompt |
| App Store / Google Play (future) | Wrap with Capacitor (same HTML/JS/CSS, no code changes) |

This means one codebase serves all six platform targets. Capacitor wrapping is an optional future step — no Capacitor work is in scope for this proposal.

---

## 4. UI Layout

The client uses a three-panel responsive layout:

```
┌────────────┬──────────────┬────────────────────────┐
│  #map      │  #room       │  #terminal             │
│  (canvas)  │  (live data) │  (xterm.js scrollback) │
│            │              ├────────────────────────┤
│            │              │  #input (command bar)  │
└────────────┴──────────────┴────────────────────────┘
```

- On viewports narrower than 768px (phones), panels stack vertically: terminal on top, room panel below, map at bottom.
- Panels are resizable via drag dividers; sizes are persisted in `localStorage`.
- Map and room panels are collapsible — they disappear for players who prefer a pure-text experience.
- A settings drawer provides: server address, font size, volume controls (music, SFX), colour theme (dark/light).

### Terminal panel

- Uses xterm.js for rendering — handles all `@@color` codes after server-to-ANSI conversion (already done for WebSocket clients), scrollback, copy/paste, and keyboard events.
- Supports the full existing game output unchanged.

### Room panel

- Populated by the existing V2 `Room` JSON messages the server already sends.
- Shows room name, description, exit buttons (clickable → sends direction), and collapsible lists of mobs, players, and objects — each with action dropdowns.

### Map panel

- Canvas-based tile map populated by the existing V2 `Map` JSON messages.
- Terrain tiles are small PNGs (32×32); the canvas falls back to solid fill colours if assets are absent.
- Current room is highlighted; mob-count badges shown on adjacent rooms.

---

## 5. Media Protocol Extensions

The server already has a music protocol:

```json
{"type":"music","action":"play","url":"/web/mp3/theme.mp3"}
{"type":"music","action":"stop"}
```

Three new message types extend this for full rich-media support. These require small server-side additions (see §8):

### 5.1 Sound Effects

```json
{"type":"sfx","action":"play","url":"/web/sfx/combat_hit.mp3","volume":0.6}
```

- One-shot playback; no looping.
- Volume is 0.0–1.0, defaulting to 1.0 if absent.
- SFX are played concurrently — multiple hits in a combat round can overlap.

### 5.2 Video

```json
{"type":"video","action":"play","url":"/web/video/intro.mp4","blocking":true}
{"type":"video","action":"stop"}
```

- `blocking: true` overlays the video fullscreen over the client; the command input is disabled until the video ends or the player dismisses it.
- `blocking: false` (default) plays the video in a small overlay in the corner without blocking input.
- Used for story cutscenes, area intros, etc.

### 5.3 Background Image

```json
{"type":"background","action":"set","url":"/web/img/midgaard.jpg","opacity":0.15}
{"type":"background","action":"clear"}
```

- Sets a parallax background image behind the terminal, faded to `opacity` (default 0.1).
- `clear` removes the background.
- Used to give each major area a visual identity without obscuring the text.

---

## 6. Progressive Web App (PWA)

Two files make the client installable:

### `manifest.json`

```json
{
  "name": "ACK!MUD TNG",
  "short_name": "ACK!MUD",
  "start_url": "/",
  "display": "standalone",
  "background_color": "#0a0a0a",
  "theme_color": "#1a1a2e",
  "icons": [
    {"src": "/icons/icon-192.png", "sizes": "192x192", "type": "image/png"},
    {"src": "/icons/icon-512.png", "sizes": "512x512", "type": "image/png"}
  ]
}
```

### `service-worker.js`

Caches all client assets (HTML, CSS, JS, lib, icons) on first visit. Subsequent loads are instant and work offline (the terminal shows a "reconnecting" state when the server is unreachable). The service worker uses a cache-first strategy for static assets and network-first for API/WebSocket connections.

---

## 7. File Structure

All files live under `web/` in this repository:

```
web/
├── client/
│   ├── index.html              # Entry point; loads all panels
│   ├── manifest.json           # PWA manifest
│   ├── service-worker.js       # PWA offline caching
│   ├── css/
│   │   ├── client.css          # Layout, theme variables, responsive rules
│   │   └── xterm.css           # xterm.js stylesheet (vendored)
│   ├── js/
│   │   ├── main.js             # Bootstrap: connects WS, wires panels
│   │   ├── connection.js       # WebSocket lifecycle, reconnect, V2 routing
│   │   ├── terminal.js         # xterm.js wrapper; ANSI rendering
│   │   ├── room.js             # Room panel: render, delta updates
│   │   ├── map.js              # Canvas minimap: BFS layout, tile render
│   │   ├── audio.js            # Music + SFX via Web Audio API
│   │   ├── video.js            # Video overlay
│   │   ├── background.js       # Background image management
│   │   └── settings.js         # Settings drawer; localStorage persistence
│   ├── lib/
│   │   ├── xterm.min.js        # xterm.js (vendored, no CDN dependency)
│   │   └── xterm-addon-fit.min.js  # Auto-resize addon
│   └── icons/
│       ├── icon-192.png        # PWA icon
│       └── icon-512.png        # PWA icon (splash)
├── mp3/                        # Music files (already referenced by server)
├── sfx/                        # Sound effect files
├── video/                      # Video files
├── img/                        # Background images, terrain tiles
│   └── terrain/                # 32×32 PNG terrain tiles for canvas map
└── README.md                   # Deployment instructions
```

`web/mp3/` already exists conceptually (server sends URLs pointing to it); this proposal formalises the directory structure.

---

## 8. Server-Side Changes

The server needs minimal additions to support the new media message types. All changes are conditional on `d->websocket_active` — telnet clients are unaffected.

### 8.1 New message-sending helpers in `socket.c`

```c
void ws_send_sfx(DESCRIPTOR_DATA *d, const char *url, float volume);
void ws_send_video(DESCRIPTOR_DATA *d, const char *url, bool blocking);
void ws_send_video_stop(DESCRIPTOR_DATA *d);
void ws_send_background(DESCRIPTOR_DATA *d, const char *url, float opacity);
void ws_send_background_clear(DESCRIPTOR_DATA *d);
```

These build the JSON messages described in §5 and call `write_websocket_frame()`.

### 8.2 Static file serving

The server's existing HTTP listener (on `--http-port`) currently handles only `/gsgp` and `/who`. We extend it to serve static files from the `web/` directory:

- Any request not matching `/gsgp` or `/who` is treated as a static file request.
- The server resolves the URL path against the `web/` directory (relative to the `area/` working directory via `../web/`).
- Path traversal (`../`) is sanitised before opening any file.
- MIME type is inferred from file extension (`.html`, `.js`, `.css`, `.mp3`, `.mp4`, `.png`, `.jpg`, `.json`).
- No directory listing is served — unknown paths return 404.

This allows the client to be served by the game server itself without requiring a separate nginx instance, which simplifies development and small deployments. Production deployments can still put nginx in front for performance.

### 8.3 Trigger points for new media messages

Server code that triggers media events:
- `send_area_music()` already exists and covers background music. No changes needed.
- New SFX: combat hit/miss/death sounds can be emitted from `fight.c` for WebSocket clients.
- New video: area intro videos can be triggered in `act_move.c` on first entry to a key room, or via a new wizard command `do_playvideo`.
- New background: triggered by `send_area_music()` companion function `send_area_background()`, keyed on area name.

### 8.4 Affected server files

| File | Change |
|------|--------|
| `src/socket.c` | Add `ws_send_sfx()`, `ws_send_video()`, `ws_send_video_stop()`, `ws_send_background()`, `ws_send_background_clear()` |
| `src/headers/socket.h` | Declare new helpers |
| `src/comm.c` | Extend HTTP handler to serve static files from `web/`; add `get_mime_type()` |
| `src/fight.c` | Emit SFX for hit/miss/death on WebSocket clients (conditional) |
| `src/act_move.c` | Emit background/video on area entry (conditional) |

---

## 9. Connection Flow

```
Browser navigates to https://ackmud.com/
    → Service worker serves index.html from cache (instant)
    → index.html loads xterm.js, CSS, JS modules
    → main.js reads server address from localStorage or URL param
    → connection.js opens wss://ackmud.com:9891 (WSS port)
    → Server sends HTTP 101 Switching Protocols
    → Server sends greeting text and theme music JSON
    → Client plays music, displays greeting in terminal
    → Player enters name/password → enters game
    → Server starts sending V2 Room + Map JSON + game text
    → Client populates all three panels
```

The connection address is configurable (settings drawer) so the same client file can connect to any ACK!MUD instance.

---

## 10. Trade-offs and Risks

| Concern | Notes |
|---------|-------|
| **xterm.js dependency** | A third-party library adds maintenance burden if it breaks. Vendoring it (no CDN) avoids surprise breakage. Size: ~700KB minified. Justified by the complexity of correct ANSI rendering. |
| **No build pipeline** | Plain ES modules work in all modern browsers. This avoids npm/Webpack complexity but means no tree-shaking. Acceptable at this scale. |
| **iOS PWA limitations** | Safari does not support the full PWA install prompt; users must manually "Add to Home Screen". Push notifications are not available on iOS PWA. These are Apple platform limitations and cannot be fixed. |
| **Static file serving in C** | Adding a basic static file server to `comm.c` is straightforward but adds surface area to the C code. Production deployments should use nginx; the built-in server is for development convenience. |
| **Media asset hosting** | `web/mp3/`, `web/sfx/`, `web/video/` directories need to be populated with actual media files. The code scaffolding works without assets — audio/video simply doesn't play if files are absent. |
| **WSS self-signed cert** | The auto-generated cert in `data/tls/` is self-signed. Browsers will warn or refuse to connect on `wss://` with a self-signed cert. Production deployments need a real cert (Let's Encrypt). This is a deployment concern, not a client code concern. |
| **V2 protocol already implemented** | Room and Map panels require no new server work — the V2 protocol is already sending the right data. Only the media extensions (§8) require new server code. |

---

## 11. Out of Scope

- Capacitor wrapping for App Store / Google Play distribution (future phase)
- Server-side rendering or SSR
- Push notifications
- In-client editor for area files or help entries
- Any changes to telnet or TLS-telnet code paths
- Video production or sound design (this proposal provides the infrastructure; assets are separate)
- HTTPS serving (the built-in HTTP server is HTTP only; TLS for HTTPS requires a separate concern)

---

## 12. Implementation Order

1. Create `web/client/` directory structure with placeholder files
2. Build `index.html` + `client.css` layout (three panels, responsive)
3. Integrate xterm.js — connect to WSS, render game text
4. Wire V2 routing in `connection.js` — route Room/Map/Music messages
5. Implement room panel (`room.js`)
6. Implement canvas minimap (`map.js`)
7. Implement audio (`audio.js`) — music already works, add SFX
8. Add PWA files: `manifest.json`, `service-worker.js`, icons
9. Server: extend HTTP handler for static file serving (`comm.c`)
10. Server: add SFX helpers (`socket.c`) + fight.c hooks
11. Server: add video/background helpers + area-entry hooks
12. Populate `web/img/terrain/` with tile PNGs
13. Write `web/README.md` with deployment instructions
14. Run `make unit-tests` (server changes require unit test coverage for the static file path resolver)
