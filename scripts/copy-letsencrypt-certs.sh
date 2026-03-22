#!/bin/sh
#
# copy-letsencrypt-certs.sh — pull Let's Encrypt certs from the cert server
#
# Run this on the GAME SERVER to copy the current Let's Encrypt certificates
# from the cert server (192.168.1.113) into data/tls/.
#
# Usage:
#   scripts/copy-letsencrypt-certs.sh [--domain <domain>] [--cert-server <host>]
#
# Defaults:
#   domain:      ackmud.com
#   cert-server: 192.168.1.113
#   dest:        <repo-root>/data/tls/
#
# Requirements:
#   - SSH key-based auth must be set up: root@192.168.1.113 must accept the
#     game server's SSH public key without a password prompt.
#   - Run as the same user that owns data/tls/ (typically the mud user).
#
# After copying, the game server will use the new certs on its next startup.
# A running server is NOT signalled — restart it manually if you want it to
# pick up the new cert immediately.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DEST_DIR="$REPO_ROOT/data/tls"

DOMAIN="ackmud.com"
CERT_SERVER="192.168.1.113"
CERT_SERVER_USER="root"

# Parse optional overrides
while [ $# -gt 0 ]; do
    case "$1" in
        --domain)      DOMAIN="$2";           shift 2 ;;
        --cert-server) CERT_SERVER="$2";      shift 2 ;;
        *) echo "copy-letsencrypt-certs.sh: unknown argument: $1" >&2; exit 1 ;;
    esac
done

REMOTE_DIR="/etc/letsencrypt/live/$DOMAIN"
REMOTE="$CERT_SERVER_USER@$CERT_SERVER"

echo "copy-letsencrypt-certs: copying $DOMAIN certs from $CERT_SERVER..."

mkdir -p "$DEST_DIR"

# Copy certificate chain (fullchain includes the cert + intermediates)
scp "$REMOTE:$REMOTE_DIR/fullchain.pem" "$DEST_DIR/cert.pem"

# Copy private key
scp "$REMOTE:$REMOTE_DIR/privkey.pem" "$DEST_DIR/key.pem"

# Restrict private key permissions
chmod 600 "$DEST_DIR/key.pem"

echo "copy-letsencrypt-certs: cert  -> $DEST_DIR/cert.pem"
echo "copy-letsencrypt-certs: key   -> $DEST_DIR/key.pem"
echo "copy-letsencrypt-certs: done (restart the game server to apply)"
