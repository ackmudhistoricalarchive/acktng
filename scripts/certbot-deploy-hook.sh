#!/bin/sh
#
# certbot-deploy-hook.sh — push renewed Let's Encrypt certs to the game server
#
# Install this on the CERT SERVER (192.168.1.113) as a certbot deploy hook so
# that fresh certs are automatically pushed to the game server after each
# successful renewal.
#
# Installation (run on 192.168.1.113 as root):
#
#   cp certbot-deploy-hook.sh /etc/letsencrypt/renewal-hooks/deploy/copy-to-gameserver.sh
#   chmod +x /etc/letsencrypt/renewal-hooks/deploy/copy-to-gameserver.sh
#
# Requirements:
#   - SSH key-based auth must be set up: root@192.168.1.113 must be able to
#     SSH into GAME_SERVER_USER@GAME_SERVER_HOST without a password prompt.
#   - The private key used must be in root's SSH agent or ~/.ssh/id_*.
#
# Certbot calls deploy hooks with $RENEWED_DOMAINS and $RENEWED_LINEAGE set.
# This script only runs when ackmud.com is among the renewed domains.
#
# Configuration — edit these before installing:

DOMAIN="ackmud.com"

# SSH login for the game server
GAME_SERVER_USER="mud"
GAME_SERVER_HOST="<GAME_SERVER_IP>"

# Destination paths on the game server (absolute paths)
GAME_SERVER_CERT="/home/mud/acktng/data/tls/cert.pem"
GAME_SERVER_KEY="/home/mud/acktng/data/tls/key.pem"

# ---- no edits needed below this line ----------------------------------------

# Only act when this domain was actually renewed
echo "$RENEWED_DOMAINS" | tr ' ' '\n' | grep -qx "$DOMAIN" || exit 0

CERT_DIR="/etc/letsencrypt/live/$DOMAIN"

echo "certbot-deploy-hook: pushing $DOMAIN certs to $GAME_SERVER_HOST..."

scp "$CERT_DIR/fullchain.pem" \
    "$GAME_SERVER_USER@$GAME_SERVER_HOST:$GAME_SERVER_CERT"

scp "$CERT_DIR/privkey.pem" \
    "$GAME_SERVER_USER@$GAME_SERVER_HOST:$GAME_SERVER_KEY"

ssh "$GAME_SERVER_USER@$GAME_SERVER_HOST" \
    "chmod 600 '$GAME_SERVER_KEY'"

echo "certbot-deploy-hook: done — certs pushed to $GAME_SERVER_HOST"
echo "certbot-deploy-hook: restart the game server to apply the new cert"
