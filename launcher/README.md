# The Atlas on macOS

`Enroth Continuum.app` is a local AppKit window containing an HTML interface.
It launches this fork's bundled arm64 MM7 engine with an explicit MM7
data path and user path. It does not package original Might & Magic data.

## Current behavior

- The established local MM7 Atlas build launches, saves, exits, and reloads.
  The newly staged source-only candidate still needs that visible check.
- MM6 and MM8 cards detect locally extracted core data. Their engine adapters,
  play buttons, saves, and character transfer are still to be implemented.
- The Vault lists MM7 saves and creates local backup copies.
- The Atelier provides four static PNG party portraits (one per party slot),
  one MM7 armor template AC override, and a Vanilla/Private mods switch.
  Portraits replace the normal image in party creation and game play. Voices
  and special death and eradication art remain original. An armor AC change
  applies to every copy of that item type while the profile is active.

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
The new Atlas profile is
`~/Library/Application Support/Enroth Continuum/Profiles/MM7-Atlas`, and its
default game-data location is
`~/Library/Application Support/Enroth Continuum/Game Data/MM7`. Existing
legacy saves are not merged automatically; review and copy them locally before
switching from an older installed app.

## Local build

Run `python3 tools/build_mm7_macos.py /absolute/new/output-directory` on
Apple silicon with Python 3, CMake, Ninja and Apple's command-line tools
available. Use `--cmake` and `--ninja` when those executables are not in PATH.
The command builds five pinned source families into nine static libraries,
compiles the `MM7` helper, and stages `Atlas/Enroth Continuum.app` under the
new output directory. It checks the local link inputs and includes upstream
license, credits, provenance and media-library notices in the staged app.
No original game files or saves are bundled. The output is ad hoc signed for
local testing; verify visible play and save/quit/reload before replacing an
installed app. See [the clean build record](../project/MM7_CLEAN_OFFLINE_BUILD.md).

The source of OpenEnroth and its contributor history remain part of this
fork. See [../CREDITS.md](../CREDITS.md) and [../project/UPSTREAM.md](../project/UPSTREAM.md).
