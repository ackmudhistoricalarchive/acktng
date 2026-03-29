#!/bin/bash
set -euo pipefail
TARGET="deploy@10.1.0.241"
SSH="ssh -o StrictHostKeyChecking=no"
rsync -a --exclude='.git' --exclude='player/' --exclude='log/' --exclude='area/core' \
    -e "$SSH" ./ "$TARGET":/opt/mud/src/
$SSH "$TARGET" "sudo systemctl restart mud"
echo "Deployed to acktng"
