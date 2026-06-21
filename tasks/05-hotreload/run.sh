#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
konsole --new-tab \
        --title "🔄 Meta-C: Hot Reload" \
        --workdir "$SCRIPT_DIR" \
        -e opencode
