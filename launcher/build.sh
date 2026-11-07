#!/bin/zsh
set -euo pipefail
export PATH="/opt/homebrew/bin:$PATH"

script_dir=${0:A:h}
exec python3 "$script_dir/assemble.py" "$@"
