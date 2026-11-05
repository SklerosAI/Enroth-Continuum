# Local status — 2026-11-05

This is a personal-machine validation note. It contains no game assets.

## MM7: playable on this M2 Mac

- macOS 27.0.1, arm64. The fork's native OpenEnroth engine and The Atlas launcher
  build as arm64 and are installed together at `/Applications/Enroth Continuum.app`.
- The installed app opens the three-campaign Atlas screen. The MM7 button
  launches the bundled engine from the installed app, shows Emerald Island,
  and returns to the launcher on exit.
- A manual MM7 save was created, then loaded after exit and relaunch. Saves are
  under `~/.openenroth/saves`; a separate local backup was made through The Vault.
- A synthetic PNG placed through The Atelier rendered in party slot 1 after
  loading that save. A Leather Armor template AC override was accepted by the
  engine and logged as item 66, base 20, bonus 3. The test portrait and armor
  file were removed; the installed app now shows Vanilla mode with no override.
- OpenAL found local audio devices. Playback was not checked by ear.

## MM6 and MM8: data staged, engine work pending

- Owned offline GOG installers were extracted into private, local folders under
  `~/Library/Application Support/Enroth Continuum/Game Data/`.
- The Atlas detects MM6 archives and MM8 core archives. It deliberately has no
  Play button for either title: this fork's OpenEnroth engine currently runs
  MM7 only.
- The MM8 offline installer did not list or extract a `Sounds/Audio.snd` archive.
  This is an unresolved data question for an eventual MM8 adapter.
- Character transfer remains a previewed design in the Hall of Echoes. No save
  converter is enabled until a destination game can safely load its output.

The game archives, installers, save files, backups, and private mod files are
outside Git. `local-game-data/*` is ignored except its tracked README.
