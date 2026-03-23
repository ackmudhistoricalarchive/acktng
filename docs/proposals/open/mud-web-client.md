# Proposal: ACK!MUD TNG Native Cross-Platform Client

**Date:** 2026-03-23
**Status:** Open — pending discussion
**Repos affected:** `acktng` (server media protocol extensions, asset manifest endpoint); `JBailes/tngclient` on GitHub (Flutter app)

---

## 1. Problem

Players have no first-party client. Raw telnet offers no media support; third-party MUD clients are desktop-only and don't support the V2 JSON protocol, graphics, video, or sound. A previous proposal suggested a JavaScript Progressive Web App, but that approach has three disqualifying limitations:

1. **App store distribution is not possible** — Apple and Google do not accept pure PWAs in their stores without a native wrapper, and that wrapper negates the "no build complexity" benefit.
2. **Bundled asset distribution is unreliable** — Service Worker caches are browser-managed and can be evicted at any time. There is no way to ship a first-run experience with pre-loaded art, music, and sound included in the download from the store.
3. **Asset update caching is second-class** — Browser cache APIs offer no guarantee of persistence. A native app can write to its own document/cache directory and keep assets indefinitely.

The replacement approach must support: true app store distribution, bundled initial assets, persistent incremental asset updates, cross-platform reach (Windows, macOS, Linux, Android, iOS), and a configurable multi-panel UI with rich media (graphics, video, sound).

---

## 2. Technology Choice: Flutter

**Flutter** (Google, Dart language) is recommended as the client framework.

### Why Flutter

| Requirement | Flutter |
|-------------|---------|
| iOS App Store distribution | Native ARM binary — passes review |
| Google Play distribution | Native ARM binary — passes review |
| Microsoft Store distribution | Native Windows binary |
| Mac App Store distribution | Native macOS binary |
| Linux desktop | Native binary (no app store, but installable package) |
| Web (browser) | Flutter Web compiles to Wasm/JS — works as fallback |
| Bundled assets | `flutter pub` includes assets in the APK/IPA at build time |
| Persistent local asset cache | Dart `path_provider` + direct file I/O to app cache directory |
| WebSocket | `web_socket_channel` package |
| Audio | `just_audio` package |
| Video | `video_player` package |
| 2D graphics / custom rendering | `CustomPainter` / `Canvas` API built into Flutter |
| Configurable panel layout | Flutter's flexible widget tree supports any layout |

### Why not the alternatives

| Framework | Problem |
|-----------|---------|
| React Native | JavaScript — same category as the rejected PWA; JS bundle delivery; Facebook-governed |
| Electron | JavaScript + Chromium — not accepted by Apple's App Store as-is; bundles a full browser |
| Godot | Excellent for games but designed around a scene/node editor, not a panel-driven app UI; Dart/Flutter is a better fit for a configurable multi-panel client with text I/O |
| Qt (C++) | Cross-platform but no official iOS/Android app store support path without Qt for Mobile license; C++ complexity |
| .NET MAUI | No Linux desktop support; Microsoft-only ecosystem feel |

Flutter is the only framework that targets all six platform targets with one codebase, compiles to native code on all of them, and has an established path through every major app store.

---

## 3. Asset Strategy

### 3.1 Bundled initial assets

Assets included in the app binary at build time via Flutter's `pubspec.yaml` `assets:` section. These ship with the app download from the store:

- Terrain tile images (minimap)
- UI sound effects (button clicks, connect/disconnect tones)
- Default theme music (or a short preview track)
- Default background images for major areas
- App icons, splash screen, fonts

These cover the first-run experience completely — no network access needed to start playing.

### 3.2 Downloadable asset updates

The server exposes an **asset manifest endpoint** (new, see §8) at `/assets/manifest.json`:

```json
{
  "version": 42,
  "assets": [
    {"path": "mp3/midgaard.mp3",      "hash": "a1b2c3d4", "size": 3145728},
    {"path": "sfx/combat_hit.mp3",    "hash": "e5f6a7b8", "size": 22016},
    {"path": "video/intro.mp4",       "hash": "c9d0e1f2", "size": 52428800},
    {"path": "img/bg/midgaard.jpg",   "hash": "01234567", "size": 204800},
    {"path": "img/terrain/city.png",  "hash": "89abcdef", "size": 4096}
  ]
}
```

On startup the client:
1. Fetches `/assets/manifest.json` from the configured server.
2. Compares each entry's hash against its local cached file hash (stored in a local SQLite database via `sqflite`).
3. Downloads only assets that are absent or whose hash has changed.
4. Stores downloaded files in the app's cache directory (`path_provider`'s `getApplicationSupportDirectory()`).
5. Updates the local hash database.

Assets are never re-downloaded unless their hash changes. The cache directory is not cleared by the OS without user action (unlike browser caches). On first run against a server the client has never seen before, all assets download in the background while the player is logging in.

### 3.3 Asset resolution at runtime

When the client needs to play `mp3/midgaard.mp3`:
1. Check local cache directory — if present, use it.
2. If absent (not yet downloaded), fall back to the bundled version if one exists.
3. If no bundled fallback, skip silently (no crash).

This means the app always works from the bundle and progressively enriches as downloads complete.

---

## 4. Configurable Panel System

The UI is a **dockable panel system**. Players can add, remove, resize, reorder, and float any panel. Layout is persisted to a local config file.

### 4.1 Available panel types

| Panel type | Content |
|------------|---------|
| `Terminal` | Main game text output (ANSI color, scrollback) |
| `Room` | Live room name, description, exits, mobs, players, objects |
| `Minimap` | Canvas tile map from V2 Map JSON |
| `Combat` | Combat-only text stream filtered from game output |
| `Chat` | Communication channels (tell, say, gossip, clan) filtered from game output |
| `Inventory` | Player inventory and equipment (populated via V2 protocol extension) |
| `Stats` | HP/MP/MV bars, level, XP — populated via V2 Vitals messages |
| `Input` | Command input bar (always present, cannot be removed) |
| `Media` | Now-playing track, volume controls, video viewport |

Any number of panels can be open simultaneously. Multiple `Terminal` panels can exist for split views. The minimum viable layout is `Input` + one `Terminal`.

### 4.2 Layout engine

Layout is defined as a tree of **splits** and **tabs**:

```
SplitH
├── SplitV
│   ├── Minimap (30%)
│   └── Room (70%)
└── SplitV
    ├── Terminal (60%)
    ├── SplitH
    │   ├── Combat (50%)
    │   └── Chat (50%)
    └── Input (fixed height)
```

- `SplitH` / `SplitV` are horizontal/vertical splits with draggable dividers.
- `Tab` groups multiple panels into a tabbed container.
- Panels can be dragged from one split to another, or torn off into a floating window (desktop only).
- On mobile, a simplified layout mode stacks panels vertically with swipe-to-switch navigation between them.

### 4.3 Layout persistence

The layout tree is serialised to JSON and saved to a local file (`layouts.json` in the app support directory). Multiple named layouts can be saved:

```json
{
  "active": "Default",
  "layouts": {
    "Default": { ... layout tree ... },
    "Combat Focus": { ... layout tree ... },
    "Mobile": { ... layout tree ... }
  }
}
```

Players can switch layouts from a toolbar menu. Layouts are per-server-address, so connecting to a different server can have a different default layout.

---

## 5. Media Protocol

The server side already has:
- Music: `{"type":"music","action":"play","url":"/assets/mp3/..."}` ✓
- Music stop: `{"type":"music","action":"stop"}` ✓

New message types (require server additions — see §8):

### 5.1 Sound effects

```json
{"type":"sfx","action":"play","url":"/assets/sfx/combat_hit.mp3","volume":0.6}
```

One-shot; multiple may overlap. Client resolves the URL against local cache first.

### 5.2 Video

```json
{"type":"video","action":"play","url":"/assets/video/intro.mp4","blocking":true}
{"type":"video","action":"stop"}
```

`blocking: true` plays fullscreen and pauses game input until dismissed. `blocking: false` plays in the `Media` panel without interrupting play.

### 5.3 Background image

```json
{"type":"background","action":"set","url":"/assets/img/bg/midgaard.jpg","opacity":0.12}
{"type":"background","action":"clear"}
```

Sets a faded background image visible behind the terminal text. Intended to set area atmosphere without obscuring the text.

### 5.4 V2 protocol additions

Two new V2 tags are needed to populate the `Stats` and `Inventory` panels:

**`Vitals`** — sent each prompt:
```json
{"v":2,"tag":"Vitals","data":{"hp":350,"max_hp":450,"mp":120,"max_mp":200,"mv":80,"max_mv":100}}
```

**`Inventory`** — sent after `inventory`, `equipment`, `get`, `drop`, `wear`, `remove`:
```json
{"v":2,"tag":"Inventory","data":{"carried":[...],"worn":[...],"gold":1500}}
```

These allow the Stats and Inventory panels to stay live without screen-scraping the terminal text.

---

## 6. Repository Structure

The client lives in `https://github.com/JBailes/tngclient` (separate from `acktng` to keep the C server repo clean and to allow independent versioning and CI):

```
tngclient/   (https://github.com/JBailes/tngclient)
├── lib/
│   ├── main.dart                   # Entry point
│   ├── app.dart                    # MaterialApp / theme
│   ├── connection/
│   │   ├── websocket_client.dart   # WebSocket lifecycle, reconnect
│   │   └── message_router.dart     # V2 JSON routing to panel streams
│   ├── panels/
│   │   ├── panel_registry.dart     # All available panel types
│   │   ├── terminal_panel.dart     # ANSI text display
│   │   ├── room_panel.dart         # Room data display
│   │   ├── minimap_panel.dart      # Canvas tile map
│   │   ├── combat_panel.dart       # Filtered combat text
│   │   ├── chat_panel.dart         # Filtered comms text
│   │   ├── inventory_panel.dart    # Inventory + equipment
│   │   ├── stats_panel.dart        # HP/MP/MV bars
│   │   └── media_panel.dart        # Now playing, video viewport
│   ├── layout/
│   │   ├── layout_engine.dart      # Split/Tab tree model
│   │   ├── layout_persistence.dart # JSON serialisation
│   │   └── layout_editor.dart      # Drag-to-rearrange UI
│   ├── assets/
│   │   ├── asset_manifest.dart     # Manifest fetch + diff
│   │   ├── asset_cache.dart        # Local file resolution
│   │   └── asset_downloader.dart   # Background download queue
│   ├── media/
│   │   ├── audio_manager.dart      # just_audio wrapper: music + SFX
│   │   └── video_manager.dart      # video_player wrapper
│   └── settings/
│       └── settings.dart           # Server address, theme, per-layout prefs
├── assets/                         # Bundled initial assets
│   ├── mp3/
│   ├── sfx/
│   ├── img/
│   │   ├── bg/
│   │   └── terrain/
│   └── fonts/
├── test/                           # Dart unit + widget tests
├── pubspec.yaml
├── android/                        # Android-specific configuration
├── ios/                            # iOS-specific configuration
├── macos/                          # macOS-specific configuration
├── windows/                        # Windows-specific configuration
├── linux/                          # Linux-specific configuration
└── web/                            # Flutter Web output (optional)
```

---

## 7. Server-Side Changes (`acktng` repo)

### 7.1 Asset manifest endpoint

Extend the HTTP handler in `comm.c` to serve `/assets/manifest.json`. This is a generated JSON file rebuilt whenever the `web/` asset directory changes. The simplest approach: a script (`tools/build_asset_manifest.py`) that walks `web/` and outputs `web/assets/manifest.json` with SHA-256 hashes. The HTTP handler serves the file statically.

### 7.2 Static asset serving

Extend the HTTP handler to serve files under `/assets/` from the `web/` directory (path traversal sanitised). This is the same small static file server described in the previous proposal revision, scoped to the `web/` subtree.

### 7.3 New media helpers in `socket.c`

```c
void ws_send_sfx(DESCRIPTOR_DATA *d, const char *url, float volume);
void ws_send_video(DESCRIPTOR_DATA *d, const char *url, bool blocking);
void ws_send_video_stop(DESCRIPTOR_DATA *d);
void ws_send_background(DESCRIPTOR_DATA *d, const char *url, float opacity);
void ws_send_background_clear(DESCRIPTOR_DATA *d);
```

### 7.4 V2 Vitals and Inventory messages

New helpers:
```c
void ws_send_vitals(DESCRIPTOR_DATA *d, CHAR_DATA *ch);     /* socket.c */
void ws_send_inventory(DESCRIPTOR_DATA *d, CHAR_DATA *ch);  /* socket.c */
```

`ws_send_vitals()` called from `send_prompt()` in `comm.c`.
`ws_send_inventory()` called after any command that changes carried/worn items (`do_get`, `do_drop`, `do_wear`, `do_remove`, `do_inventory`, `do_equipment`).

### 7.5 Affected server files

| File | Change |
|------|--------|
| `src/socket.c` | Add `ws_send_sfx()`, `ws_send_video()`, `ws_send_video_stop()`, `ws_send_background()`, `ws_send_background_clear()`, `ws_send_vitals()`, `ws_send_inventory()` |
| `src/headers/socket.h` | Declare new helpers |
| `src/comm.c` | Extend HTTP handler for `/assets/` static serving and `/assets/manifest.json`; call `ws_send_vitals()` from `send_prompt()` |
| `src/act_obj.c` | Call `ws_send_inventory()` after get/drop/wear/remove |
| `src/act_info.c` | Call `ws_send_inventory()` after inventory/equipment commands |
| `src/fight.c` | Emit SFX for hit/miss/death on WebSocket clients |
| `src/act_move.c` | Emit background on area entry |
| `src/tools/build_asset_manifest.py` | New script: walk `web/`, write `web/assets/manifest.json` |

---

## 8. Trade-offs and Risks

| Concern | Notes |
|---------|-------|
| **Dart/Flutter learning curve** | Flutter uses Dart, which is not widely known. However, Dart is simple and Flutter's documentation is excellent. The existing C server team would be maintaining a new language. |
| **App store review** | Apple and Google review native apps before publishing. Updates go through review (1–3 days for Apple, hours for Google). This is unavoidable for native app store distribution. |
| **Flutter Web** | Flutter Web is available but the output is not a traditional website — it compiles to WebAssembly + Canvas. It will work in a browser but is heavier than a native web app. It is included as a convenience fallback, not a primary target. |
| **Asset manifest script** | `build_asset_manifest.py` must be run whenever assets change. This should be automated via a Makefile rule or git hook. |
| **Initial app size** | Bundling initial art/music will increase the app download size. Typical MUD UI assets (terrain tiles, SFX, one theme track) should stay under 50MB — acceptable for a store download. |
| **Video file sizes** | Video cutscenes can be large. These should not be bundled — they should be download-on-demand assets. |
| **Background downloads on mobile** | iOS restricts background network activity. Large asset downloads should be initiated while the app is foregrounded and use `WorkManager`/`BGTaskScheduler` for continuation. |
| **Layout complexity** | A full dockable panel system is non-trivial to implement. A phased approach is sensible: ship fixed layouts first, add drag-to-rearrange in a later release. |

---

## 9. Out of Scope

- The server binary itself — only the WebSocket protocol extensions and HTTP asset serving are in scope
- In-game area editor or admin tools
- Push notifications
- Account creation via the client (handled in-game via the existing login flow)
- Multiplexed connections (connecting to more than one server at once)

---

## 10. Implementation Order

### Phase 1 — Core client (playable)
1. New Flutter project in `JBailes/tngclient`; all six platform targets configured
2. Connection screen (server address, connect button)
3. WebSocket connection + reconnect logic
4. Terminal panel with ANSI color rendering
5. Input bar with command history
6. Fixed two-panel default layout (Terminal + Input)
7. Music playback (existing server protocol)

### Phase 2 — Structured panels
8. V2 Room and Map message handling (server already sends these)
9. Room panel implementation
10. Minimap canvas implementation
11. Stats panel + server `ws_send_vitals()`
12. Inventory panel + server `ws_send_inventory()`

### Phase 3 — Media and assets
13. Asset manifest endpoint + `build_asset_manifest.py`
14. Asset cache + downloader in client
15. SFX playback + server `ws_send_sfx()` hooks in `fight.c`
16. Background image support + server `ws_send_background()` hooks in `act_move.c`
17. Video playback + server `ws_send_video()` + wizard command

### Phase 4 — Configurable layout
18. Layout engine (split/tab tree model)
19. Layout persistence to local file
20. Panel drag-to-rearrange (desktop)
21. Mobile swipe-to-switch panel navigation
22. Named layout presets (save/load/switch)

### Phase 5 — Distribution
23. Android: signed APK/AAB, Google Play listing
24. iOS: provisioning profiles, App Store Connect submission
25. Windows: MSIX package, Microsoft Store listing
26. macOS: notarisation, Mac App Store submission
27. Linux: `.deb`/`.rpm` packages and/or Flatpak
