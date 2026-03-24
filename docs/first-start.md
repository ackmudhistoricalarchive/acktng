# First-Start Guide

How to boot ACK!MUD TNG from a fresh clone.

## 1. Install Build Dependencies

On Debian or Ubuntu:

```sh
sudo apt-get update
sudo apt-get install -y build-essential libcrypt-dev zlib1g-dev libssl-dev \
    pkg-config libpq-dev postgresql postgresql-client clang-format python3
```

Ensure PostgreSQL is running:

```sh
sudo systemctl start postgresql
sudo systemctl enable postgresql   # optional: start on boot
```

## 2. Build the Server

```sh
cd src
make ack
```

The binary is produced at `src/ack`.

## 3. Set Up PostgreSQL

### Create the database role

```sh
sudo -u postgres psql -c "CREATE ROLE ack WITH LOGIN PASSWORD 'acktest';"
```

### Create the database

```sh
sudo -u postgres createdb -O ack acktng
```

### Apply the schema

```sh
sudo -u postgres psql -d acktng -q -f area/schema.sql
```

### Grant permissions

```sh
sudo -u postgres psql -d acktng -c "GRANT ALL ON ALL TABLES IN SCHEMA public TO ack;"
sudo -u postgres psql -d acktng -c "GRANT ALL ON ALL SEQUENCES IN SCHEMA public TO ack;"
```

## 4. Seed the Database

The file `fixtures/test_data.sql` provides a minimal data set that is enough to
boot the server: one area, one room (the school at vnum 4900), a social, a help
entry, and a login greeting.

```sh
PGPASSWORD=acktest psql -h localhost -U ack -d acktng -q -f fixtures/test_data.sql
```

If you later load a full game world into the database, this step can be skipped
or replaced with the production data import.

## 5. Create the Database Connection Config

The server reads its connection string from `data/db.conf` (relative to the
repo root). Create the file:

```sh
printf 'host=localhost dbname=acktng user=ack password=acktest\n' > data/db.conf
```

The format is a standard
[libpq connection string](https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING).
You can also override the path by setting the `ACK_DB_CONF` environment
variable to an absolute file path.

## 6. Ensure Required Directories Exist

The server expects a `log/` directory. It should already exist in the
repository, but verify:

```sh
mkdir -p log
```

## 7. Start the Server

The server binary must run from the `area/` directory because it resolves
`../data/` and `../log/` relative to the working directory.

```sh
cd area
../src/ack 4000
```

You should see boot messages followed by the server listening on port 4000.

### Optional flags

| Flag | Description |
|------|-------------|
| `--tls-port <port>` | TLS-encrypted telnet (requires cert/key, see below) |
| `--wss-port <port>` | Secure WebSocket |
| `--ws-loopback <port>` | Plain WebSocket (for use behind an nginx TLS proxy) |
| `--sniff-port <port>` | Sniff/debug port |
| `--http-port <port>` | HTTP port (default 8080 when enabled) |
| `--tls-cert <path>` | Path to TLS certificate (default `../data/tls/cert.pem`) |
| `--tls-key <path>` | Path to TLS private key (default `../data/tls/key.pem`) |

Example with TLS:

```sh
cd area
../src/ack 4000 --tls-port 4443 --tls-cert ../data/tls/cert.pem --tls-key ../data/tls/key.pem
```

## 8. Verify

Connect with a telnet client:

```sh
telnet localhost 4000
```

You should see the login greeting. Type a new character name and walk through
character creation. The new character will be placed in the Test School
(room 4900).

## 9. Optional: TLS Setup

If you want to use `--tls-port` or `--wss-port`, you need a certificate and
private key. For local development, generate a self-signed pair:

```sh
mkdir -p data/tls
openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout data/tls/key.pem -out data/tls/cert.pem \
    -days 365 -subj '/CN=localhost'
```

## 10. Troubleshooting

**Server aborts immediately at boot**
- Check that PostgreSQL is running and reachable.
- Verify `data/db.conf` exists and contains a valid connection string.
- Confirm the schema has been applied (`schema_version` table should contain a
  row with `version = 8`).

**"Port number must be above 1024"**
- The plain telnet port must be between 1025 and 65534.

**No login greeting displayed**
- The greeting is served from the `help_entries` table (rows with filename
  `greeting1` through `greeting6`). Make sure you loaded the fixture data.

**Player cannot save**
- Player data is stored in the PostgreSQL `players` table. Verify the database
  is reachable and the schema has been applied (the `players` table must exist).

**Schema version mismatch**
- The server checks that the database schema version matches the compiled-in
  `DB_SCHEMA_VERSION` (currently 8). Re-apply `area/schema.sql` if you updated
  the codebase.

**Build fails with missing headers**
- Ensure all packages from step 1 are installed. OpenSSL (`libssl-dev`) and
  libpq (`libpq-dev`) are auto-detected; if they are missing, the build
  succeeds but TLS and database features are disabled.
