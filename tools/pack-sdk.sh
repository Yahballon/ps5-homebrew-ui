#!/usr/bin/env bash
# ps5-homebrew-ui - Package library objects and public headers as an SDK.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later

set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"

# First run "make app" to cross-compile the same objects as the working app.
[[ -d build/obj/src && -f dist/PPSA99999.zip ]] || {
    # The Title ID can be customized, so validate any app ZIP instead.
    shopt -s nullglob
    packages=(dist/PPSA*.zip)
    (( ${#packages[@]} > 0 )) || {
        echo "Build the app first: make app" >&2
        exit 2
    }
}
ar_tool=$(command -v llvm-ar || command -v llvm-ar-18 || command -v ar)
[[ -n "$ar_tool" ]] || { echo "llvm-ar or ar required" >&2; exit 2; }

stage="$root/dist/sdk"
rm -rf -- "$stage"
mkdir -p "$stage/include" "$stage/lib" "$stage/share"
objects=()
for component in audio core gfx ui sdk platform/ps5 third_party/stb; do
    folder="$root/build/obj/src/$component"
    [[ -d $folder ]] || continue
    while IFS= read -r -d '' obj; do objects+=("$obj"); done < <(
        find "$folder" -type f -name '*.o' -print0 | sort -z
    )
done
(( ${#objects[@]} > 0 )) || { echo "No compiled SDK objects" >&2; exit 2; }
"$ar_tool" crs "$stage/lib/libps5hui.a" "${objects[@]}"
for component in audio core gfx ui sdk platform/ps5 third_party/stb; do
    mkdir -p "$stage/include/$component"
    find "src/$component" -type f \( -name '*.hpp' -o -name '*.h' \) | while IFS= read -r file; do
        destination="$stage/include/${file#src/}"
        mkdir -p "$(dirname "$destination")"
        cp "$file" "$destination"
    done
done
cp LICENSE THIRD_PARTY_NOTICES.md "$stage/share/"
cp docs/SDK.md "$stage/share/SDK.md"
cp -r assets "$stage/share/"
cp -r src/runtime "$stage/share/runtime-sources"
tar -czf "$root/dist/ps5-homebrew-ui-sdk.tar.gz" -C "$root/dist" sdk
printf 'SDK: %s (%s objects)\n' "$root/dist/ps5-homebrew-ui-sdk.tar.gz" "${#objects[@]}"
