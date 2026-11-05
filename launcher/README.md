# The Atlas on macOS

`Enroth Continuum.app` is a local AppKit window containing an HTML interface.
It launches this fork's bundled arm64 OpenEnroth engine with an explicit MM7
data path and user path. It does not package original Might & Magic data.

## Current behavior

- MM7 launches, saves, exits, and reloads through The Atlas.
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

## Local build

Build the fork's `OpenEnroth` target first with CMake, then run
`launcher/build.sh`. The build script creates `build/Atlas/Enroth Continuum.app`
and includes the OpenEnroth GPL license, CREDITS, and upstream notes inside
`Contents/Resources/Notices`. It uses the local Command Line Tools macOS 26.5
SDK and `rsvg-convert` currently installed on this machine. The output is ad
hoc signed for local use. To install, copy the verified app to
`/Applications/Enroth Continuum.app`.

The source of OpenEnroth and its contributor history remain part of this
fork. See [../CREDITS.md](../CREDITS.md) and [../project/UPSTREAM.md](../project/UPSTREAM.md).
