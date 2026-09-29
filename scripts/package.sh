#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
binary="${1:-$project_root/out/redmagic-trigger-bridge}"
output_dir="${2:-$project_root/out}"
staging="$output_dir/module-staging"
module_prop="$project_root/module/module.prop"

version="$(sed -n 's/^version=//p' "$module_prop" | head -n 1)"
case "$version" in
    ""|*[!0-9A-Za-z._-]*)
        printf 'Invalid module version: %s\n' "$version" >&2
        exit 1
        ;;
esac

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

archive="$output_dir/Redmagic-Trigger-Bridge-v$version.zip"
rm -f "$archive"
(
    cd "$staging"
    zip -9qr "$archive" .
)
(
    cd "$output_dir"
    archive_name="$(basename "$archive")"
    sha256sum "$archive_name" > "$archive_name.sha256"
)
printf 'Created %s\n' "$archive"
printf 'Created %s.sha256\n' "$archive"
