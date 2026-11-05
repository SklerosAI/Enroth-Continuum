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

## MM6: implementation checkpoint, not yet playable

- Owned offline GOG installers were extracted into private, local folders under
  `~/Library/Application Support/Enroth Continuum/Game Data/`.
- Branch `mm6-mac-implementation` adds `--game mm6`, a separate MM6 user
  profile, MM6 table and event adapters, an MM6 archive decoder, and an MM6
  outdoor map decoder. The native arm64 build succeeds.
- The MM6 title screen renders from the owner's data. Party creation opens,
  but still uses MM7 layout and expects missing MM7 portraits and class icons.
  Pressing Return passes the default party through to the opening transition.
- The initial MM6 map attempt exposed the distinct compressed map wrapper. Its
  decoder now builds, but the map start change and decoder have not yet been
  checked together. The earlier attempt incorrectly entered MM7's first map
  and failed on its MM6 delta format. MM6 delta, indoor map, campaign behavior,
  and save/load support remain unfinished.
- The installed Atlas still has no MM6 Play button. The installed MM7 app was
  not replaced; its known-good save and launch path remain the baseline.
- The Atlas also detects MM8 core archives but has no MM8 Play button.
- MM8 game data includes `Music/2.mp3` through `Music/15.mp3`. A separate
  `mm_8_ost_flac.zip` download is in `~/Downloads` and contains FLAC soundtrack
  tracks; it has not been added to the game-data folder or Git.
- The MM8 offline installer did not list or extract a `Sounds/Audio.snd` archive.
  The soundtrack download establishes music availability, but the MM8 sound
  effects format/source still needs investigation when its engine adapter begins.
- Character transfer remains a previewed design in the Hall of Echoes. No save
  converter is enabled until a destination game can safely load its output.

The game archives, installers, save files, backups, and private mod files are
outside Git. `local-game-data/*` is ignored except its tracked README.
