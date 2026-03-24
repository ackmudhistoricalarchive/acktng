# Proposal: First-Start Guide

## Problem

There is no single document that walks a new developer or operator through everything needed to get the MUD booting from a fresh clone. The required steps (installing dependencies, creating the PostgreSQL database, applying the schema, seeding data, creating directories, writing `data/db.conf`, building the binary, and launching the server) are scattered across CLAUDE.md, integration test scripts, and source code comments. A newcomer has to reverse-engineer the boot process to get a working server.

## Approach

Create `docs/first-start.md` — a step-by-step guide covering:

1. **Prerequisites** — OS packages (`build-essential`, `libcrypt-dev`, `zlib1g-dev`, `libssl-dev`, `pkg-config`, `libpq-dev`, `postgresql`, `postgresql-client`, `clang-format`, `python3`)
2. **PostgreSQL setup** — Create a database role, create the `acktng` database, apply `area/schema.sql`, grant permissions
3. **Seed data** — Load `fixtures/test_data.sql` to populate the minimal set of areas, rooms, mobs, objects, socials, help entries (including login greetings), and sysdata needed for boot
4. **Configure the database connection** — Create `data/db.conf` with the libpq connection string
5. **Ensure required directories exist** — `player/a` through `player/z`, `log/`
6. **Build the server** — `cd src && make ack`
7. **Launch** — `cd area && ../src/ack <port>` with notes on optional TLS/WebSocket flags
8. **Verify** — Connect via telnet, see the login greeting, create a character
9. **Optional: TLS setup** — Generating or placing cert/key for `--tls-port` / `--wss-port`
10. **Troubleshooting** — Common failure modes (missing `db.conf`, schema version mismatch, missing player dirs, port conflicts)

## Affected Files

- `docs/first-start.md` (new file)

## Trade-offs

- The guide targets Debian/Ubuntu since that's what CI uses. Other distros will need to adapt package names.
- Uses `fixtures/test_data.sql` as the seed data, which provides a minimal but functional world. A production deployment would need full area data loaded, but that's out of scope for a "first boot" guide.
- Keeps TLS setup optional since plain telnet is sufficient for local development.
