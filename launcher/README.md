# The Atlas on macOS

`Enroth Continuum.app` is a local AppKit window containing an HTML interface.
It launches this fork's bundled, title-locked arm64 MM7 engine with an explicit MM7
data path and user path. It does not package original Might & Magic data.

## Current behavior

- MM7 launches, saves, exits, and reloads through The Atlas.
- Optional **Record gameplay** starts a local passive MM7 session recorder with
  Play. Choose an existing folder on this Mac or an external drive before
  launching. The recorder keeps a rolling display replay of the last two
  minutes, writes game logs and stable save snapshots there, and makes a fresh
  save checkpoint every 10 minutes. Press **Command-Shift-R** while MM7 runs
  to keep the replay in a new clip folder. If MM7 crashes, quits on a signal
  or exits with an error, Atlas tells the recorder and the replay is kept as a
  clip automatically. After a normal exit the replay is deleted. Only clips
  accumulate. Atlas warns when recording storage or system memory runs low,
  and the recorder stops below 2 GB free. Recording stops when MM7 exits,
  while normal gameplay and saves stay in the MM7 profile. Video needs macOS
  Screen Recording permission for Enroth Continuum. Atlas is ad-hoc signed,
  so macOS may ask again after each new install.
  The hotkey uses the Carbon hot key API, which needs neither Accessibility
  nor Input Monitoring permission. Atlas registers it only while a recorded
  MM7 session runs, and other apps do not receive Command-Shift-R meanwhile.
  If an external drive is remounted or replaced, choose its folder again so
  Atlas can verify the destination before the next recorded launch.
- MM6 and MM8 cards detect locally extracted core data. Their engine adapters,
  play buttons, saves, and character transfer are still to be implemented.
- The Vault lists MM7 saves and creates local backup copies.
- The Atelier provides private portrait moods for four party positions,
  one MM7 armor template AC override, an optional fresh-vendor-stock-on-entry
  setting for MM6 and MM7, and a Vanilla/Private mods switch.
  Portrait moods can replace normal, condition and reaction art in party creation
  and game play. Voices and body art remain original; death and eradication art
  remain original unless their own mood PNGs are supplied. An armor AC change
  applies to every copy of that item type while the profile is active.
  MM7's vendor option applies on the next Private mods launch. The MM6 choice
  is retained locally for its future playable engine; MM6 shops are not live yet.

## Private data

Game data, saves, backups, and mod files stay in the user's home folder.
The launcher never writes game archives. The mod folder is
`~/Library/Application Support/Enroth Continuum/Mods/MM7` and may contain
`portraits/<slot>/<mood>.png`, legacy `portrait-N.png`, and `armor.tsv`. Supported
moods are normal, talk, smile, sad, hurt, pain, afraid, cast, sleep, unconscious,
weak, cursed, drunk, insane, sick, paralyzed, stone, zombie, dead and eradicated.
Each slot needs a valid normal PNG; the legacy file supplies normal when the new
file is absent or invalid. Optional missing moods use normal, with pain using
hurt and unconscious using sleep first. Dead and eradicated keep original art
when their own PNG is absent. The engine reads that folder only when the
Private mods switch is on. Use **Open mods folder** in
the Atelier to inspect or edit these files. Atlas converts imported portraits
to PNG before placing them there. The Atelier can import a folder of named mood
PNGs for one party position. Imported changes take effect on the next launch.
On its first use, **Open mods folder** creates a local `README.md` that records
the supported layouts and a multi-row armor example. It never overwrites an
existing guide. Other files in this folder are not loaded.
The armor file format is:

```text
# item_id<TAB>base_ac<TAB>bonus_ac
66<TAB>20<TAB>3
```

Use actual tab characters in the file. The Atelier writes a one-row file for you;
hand-edited files may contain up to 32 valid armor rows. Invalid values are rejected
and the original game values remain in use. The
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
