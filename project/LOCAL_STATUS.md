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

## Trilogy isolation checkpoint — 2026-11-05

- MM7 source is independently preserved on branch `mm7-isolated-runtime` at
  commit `020cecf49`, in a separate managed worktree. Its staged Atlas bundle
  contains `Contents/Helpers/MM7.app` with a distinct bundle identifier.
- The new MM7 profile is under
  `~/Library/Application Support/Enroth Continuum/Profiles/MM7`; the two
  existing save files were copied there and SHA-256 matched both the originals
  and a separate local backup. The old `~/.openenroth` profile remains intact.
- The isolated MM7 app built and passed a live graphical check: The Atlas
  launched MM7, an existing save loaded on Emerald Island, and quitting
  returned to The Atlas. The installed `/Applications` app was left in place
  during this verification.
- The private MM6 reference folder
  `local-game-data/mm6/research-2026-11-05` holds extracted copies of four
  LOD archives and an ignored per-file/per-table catalog. Archive entry lists
  contain 2,703 icons, 134 maps, 1,958 bitmaps and 4,314 sprites. The catalog
  indexes 9,128 extracted and manifest files and every nonempty row in 30 text
  tables. See [MM6_IMPLEMENTATION_CHECKLIST.md](MM6_IMPLEMENTATION_CHECKLIST.md)
  for implementation coverage and remaining audit work.
- MM6 is still not playable. The isolated MM7 bundle is a protected baseline;
  MM6 and later MM8 will be separate sibling engines and profiles, with their
  own Atlas actions once their campaign checks pass.
- Three source checkouts now exist simultaneously: `mm6-mac-implementation`
  (active adapter), `mm7-isolated-runtime` (verified baseline), and
  `mm8-isolated-runtime` (reserved untouched baseline). MM8 has no adapted
  engine or bundled app yet.
- Offline MM6 probes cover all 134 retained map/delta entries, all 52 indoor
  `.blv` layouts and 52 `.dlv` layouts, and the 67 listed map scripts plus
  `global.evt`. Every indoor base and delta layout reaches its exact decoded
  end. Two `.blv` entries contain
  complete DEFLATE streams followed by extra bytes inside the archive entry;
  their runtime decoding policy remains open. Three opcodes used in listed map
  scripts still lack MM6-specific C++ parsing and execution. These findings are
  recorded in the checklist; no installed runtime changed.
- A locally retained, ignored copy of the 412-page Prima strategy guide is
  text-searchable. Its chapter outline has been converted into explicit
  campaign-coverage work in the checklist. This is source indexing, not yet a
  page-by-page reconciliation or proof of any implemented behavior.
