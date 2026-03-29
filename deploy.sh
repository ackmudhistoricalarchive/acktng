#!/bin/bash
set -euo pipefail
TARGET="deploy@10.1.0.241"
SSH="ssh -o StrictHostKeyChecking=no"
rsync -a --exclude='.git' --exclude='player/' --exclude='log/' --exclude='area/core' \
    -e "$SSH" ./ "$TARGET":/opt/mud/src/
# Trigger hotreboot (MUD checks for this file every second, issues
# 60-second player warning, then exec()s the new binary in-place)
$SSH "$TARGET" "touch /opt/mud/src/data/hotreboot.trigger"
echo "Deployed to acktng (hotreboot triggered, 60s countdown)"
