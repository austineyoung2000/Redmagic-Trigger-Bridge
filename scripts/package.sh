#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
binary="${1:-$project_root/out/redmagic-trigger-bridge}"
output_dir="${2:-$project_root/out}"
staging="$output_dir/module-staging"

if [[ ! -x "$binary" ]]; then
    printf 'Missing executable daemon: %s\n' "$binary" >&2
    exit 1
fi

rm -rf "$staging"
mkdir -p "$staging/bin" "$output_dir"
cp -a "$project_root/module/." "$staging/"
cp "$binary" "$staging/bin/redmagic-trigger-bridge"
chmod 0755 "$staging/bin/redmagic-trigger-bridge" \
    "$staging/service.sh" "$staging/uninstall.sh" \
    "$staging/bridge-control.sh" "$staging/action.sh"

archive="$output_dir/Redmagic-Trigger-Bridge-v0.2.0-dev.zip"
rm -f "$archive"
(
    cd "$staging"
    zip -9qr "$archive" .
)
printf 'Created %s\n' "$archive"
