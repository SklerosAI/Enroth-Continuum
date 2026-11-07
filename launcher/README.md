# The Atlas on macOS

`Enroth Continuum.app` is a local AppKit window containing an HTML interface.
It launches this fork's bundled, title-locked arm64 MM7 engine with an explicit MM7
data path and user path. It does not package original Might & Magic data.

## Current behavior

- MM7 launches, saves, exits, and reloads through The Atlas.
- MM6 and MM8 cards detect locally extracted core data. Their engine adapters,
  play buttons, saves, and character transfer are still to be implemented.
- The Vault lists MM7 saves and creates local backup copies.
- The Atelier provides four static PNG party portraits (one per party slot),
  one MM7 armor template AC override, an optional fresh-vendor-stock-on-entry
  setting for MM6 and MM7, and a Vanilla/Private mods switch.
  Portraits replace the normal image in party creation and game play. Voices
  and special death and eradication art remain original. An armor AC change
  applies to every copy of that item type while the profile is active.
  MM7's vendor option applies on the next Private mods launch. The MM6 choice
  is retained locally for its future playable engine; MM6 shops are not live yet.

## Private data

Game data, saves, backups, and mod files stay in the user's home folder.
The launcher never writes game archives. The mod folder is
`~/Library/Application Support/Enroth Continuum/Mods/MM7` and contains up to
four `portrait-N.png` files and an `armor.tsv` file. The engine reads that
folder only when the Private mods switch is on. The armor file format is:

```text
# item_id<TAB>base_ac<TAB>bonus_ac
66<TAB>20<TAB>3
```

Use actual tab characters in the file. The Atelier writes the file for you.
Invalid values are rejected and the original game values remain in use. The
app keeps the active game data paths and mod switch in local macOS preferences.
The vendor option is also stored in preferences and does not edit game archives
or existing saves. When MM7 Private mods is active, Atlas passes
`ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY=1` to its bundled engine. The corresponding
MM6 engine setting is `ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY=1` once MM6 launches.

The source defaults for a future MM7 Atlas build are
`~/Library/Application Support/Enroth Continuum/Game Data/MM7` and the fresh
`~/Library/Application Support/Enroth Continuum/Profiles/MM7-Atlas` profile.
Atlas checks the same eight required MM7 archives as the engine before launch.
An explicitly chosen data folder remains valid if it contains those archives.
MM7 on macOS does not automatically copy saves from the data folder into the
fresh profile. Copy older saves into `Profiles/MM7-Atlas/saves` by hand.

The installed Atlas packages a Release build of the `MM7` target. Build it in a
separate tree with `cmake -B build-release -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
-DOE_BUILD_TESTS=OFF`, then pass `--mm7-app build-release/src/Bin/OpenEnroth/MM7.app`.
A Debug helper is about three times larger and noticeably slower.

## Local build

Build the fork's `MM7` target with CMake, then run
`launcher/build.sh --mm7-app build/src/Bin/OpenEnroth/MM7.app`. The assembler
checks the bundle ID, arm64 executable and title lock before staging the
signed app at `build/Atlas/Enroth Continuum.app`. Its helper is
`Contents/Helpers/MM7.app`; it cannot launch MM6 data. The bundle includes
the upstream license, CREDITS and upstream notes in
`Contents/Resources/Notices`. A wrong-title helper or failed build leaves the
previous staged Atlas intact. It locates the installed Apple SDK and Swift
compiler through `xcrun`, and packages the tracked `AtlasIcon.icns`, generated
from the editable `AtlasIcon.svg`. Packaging does not require Homebrew or SVG
conversion tools. If the icon art changes, regenerate and review the tracked
ICNS before committing the artwork.

When staging an MM7 helper linked to the repository-owned media libraries,
pass `--mm7-deps-build-root` with the output directory from
[`build_mm7_macos_dependencies.py`](../tools/build_mm7_macos_dependencies.py).
The assembler checks the report's source pins and hashes of all nine static
archives and copied source notices, then includes those notices in
`Contents/Resources/Notices/MM7-Media`. This option is explicit because an
older cached-library helper cannot be identified as a source-built helper from
its bundle metadata alone. Keep the recipe output available until the staged
Atlas is assembled.

For a future title update, pass `--reuse-from` with an existing Atlas and
`--mm6-app` or `--mm8-app` only after that title passes its play and save gate.
The assembler carries forward any existing sibling helper not replaced by an
explicit input. It always writes a staged bundle, never directly to
`/Applications`; install only after verifying the playable title's launch and
save/load behavior.

The source of OpenEnroth and its contributor history remain part of this
fork. See [../CREDITS.md](../CREDITS.md) and [../project/UPSTREAM.md](../project/UPSTREAM.md).
