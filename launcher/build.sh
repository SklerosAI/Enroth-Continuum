#!/bin/zsh
set -euo pipefail
export PATH="/opt/homebrew/bin:$PATH"

repo_root=${0:A:h:h}
engine_app="$repo_root/build/src/Bin/OpenEnroth/OpenEnroth.app"
output_app="$repo_root/build/Atlas/Enroth Continuum.app"
sdk=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk
cache="$HOME/Library/Caches/EnrothContinuum/SwiftModules"

if [[ ! -x "$engine_app/Contents/MacOS/OpenEnroth" ]]; then
  print -u2 "Build the fork's OpenEnroth.app first: $engine_app"
  exit 1
fi
if [[ ! -d "$sdk" ]]; then
  print -u2 "The tested macOS SDK is missing: $sdk"
  exit 1
fi

mkdir -p "$cache"
rm -rf "$output_app"
mkdir -p "$output_app/Contents/MacOS" "$output_app/Contents/Resources/Atlas" "$output_app/Contents/Helpers" "$output_app/Contents/Resources/Notices"

swiftc -O -sdk "$sdk" -module-cache-path "$cache" -framework AppKit -framework WebKit \
  "$repo_root/launcher/AtlasApp.swift" -o "$output_app/Contents/MacOS/EnrothContinuum"
cp "$repo_root/launcher/Info.plist" "$output_app/Contents/Info.plist"
cp "$repo_root/launcher/Atlas/index.html" "$output_app/Contents/Resources/Atlas/index.html"
cp "$repo_root/LICENSE" "$repo_root/CREDITS.md" "$repo_root/project/UPSTREAM.md" "$output_app/Contents/Resources/Notices/"
iconset="$repo_root/build/Atlas/AtlasIcon.iconset"
mkdir -p "$iconset"
for size in 16 32 128 256 512; do
  rsvg-convert -w "$size" -h "$size" "$repo_root/launcher/AtlasIcon.svg" -o "$iconset/icon_${size}x${size}.png"
  doubled=$((size * 2))
  rsvg-convert -w "$doubled" -h "$doubled" "$repo_root/launcher/AtlasIcon.svg" -o "$iconset/icon_${size}x${size}@2x.png"
done
python3 "$repo_root/launcher/make_icon.py" "$iconset" "$output_app/Contents/Resources/AtlasIcon.icns"
ditto "$engine_app" "$output_app/Contents/Helpers/OpenEnroth.app"

codesign --force --sign - "$output_app/Contents/Helpers/OpenEnroth.app"
codesign --force --sign - "$output_app"
codesign --verify --strict --verbose=2 "$output_app"
print "$output_app"
