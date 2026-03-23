#!/bin/sh
# Generate (or validate) the self-signed TLS certificate and private key.
#
# Regenerates when:
#   - key.pem or cert.pem does not exist
#   - cert.pem expires within 30 days
#   - cert.pem and key.pem do not share the same public key
#   - cert.pem SAN does not cover ackmud.com (hostname mismatch check)
#
# Usage:
#   data/tls/generate-certs.sh [--cert <path>] [--key <path>]
#
# Defaults:
#   cert: <script-dir>/cert.pem
#   key:  <script-dir>/key.pem

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CERT="$SCRIPT_DIR/cert.pem"
KEY="$SCRIPT_DIR/key.pem"

# Parse optional --cert / --key overrides
while [ $# -gt 0 ]; do
    case "$1" in
        --cert) CERT="$2"; shift 2 ;;
        --key)  KEY="$2";  shift 2 ;;
        *) echo "generate-certs.sh: unknown argument: $1" >&2; exit 1 ;;
    esac
done

generate() {
    echo "startup: generating self-signed TLS certificate..."
    echo "         (replace $CERT / $KEY with a CA-signed cert for production)"
    mkdir -p "$(dirname "$CERT")" "$(dirname "$KEY")"
    openssl req -x509 -newkey rsa:4096 \
        -keyout "$KEY" \
        -out "$CERT" \
        -days 3650 \
        -nodes \
        -subj "/CN=ackmud.com/O=ACK!MUD TNG/C=US" \
        -addext "subjectAltName=DNS:ackmud.com,DNS:localhost,IP:127.0.0.1" \
        2>/dev/null
    chmod 600 "$KEY"
    echo "startup: TLS certificate written to $CERT (valid 10 years)"
}

# Missing files → generate
if [ ! -f "$KEY" ] || [ ! -f "$CERT" ]; then
    generate
    exit 0
fi

# Expiry check: regenerate if cert expires within 30 days
if ! openssl x509 -in "$CERT" -noout -checkend 2592000 >/dev/null 2>&1; then
    echo "startup: TLS certificate expires within 30 days — regenerating..."
    generate
    exit 0
fi

# SAN check: regenerate if cert does not cover ackmud.com
if ! openssl x509 -in "$CERT" -noout -text 2>/dev/null | grep -q "DNS:ackmud.com"; then
    echo "startup: TLS certificate SAN missing ackmud.com — regenerating..."
    generate
    exit 0
fi

# Key-pair match: compare public key fingerprints from cert and key
CERT_PUB="$(openssl x509 -in "$CERT" -noout -pubkey 2>/dev/null | openssl pkey -pubin -noout -text 2>/dev/null | md5sum)"
KEY_PUB="$(openssl pkey -in "$KEY" -pubout 2>/dev/null | openssl pkey -pubin -noout -text 2>/dev/null | md5sum)"
if [ "$CERT_PUB" != "$KEY_PUB" ]; then
    echo "startup: TLS cert/key mismatch — regenerating..."
    generate
    exit 0
fi

echo "startup: TLS certificate OK ($(openssl x509 -in "$CERT" -noout -enddate 2>/dev/null | cut -d= -f2))"
