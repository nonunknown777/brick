#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
konsole --new-tab \
        --title " Libraries: Meta-C" \
        --workdir "$SCRIPT_DIR" \
        -e opencode
