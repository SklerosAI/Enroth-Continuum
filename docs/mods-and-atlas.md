# Mods and the Atlas launcher

This page covers The Atlas, the macOS launcher app that runs the fork's title-locked engines, and the private mods it
can switch on.

## The Atlas at a glance

`Enroth Continuum.app` is a small AppKit window that hosts an HTML page in a `WKWebView`. The page posts named actions
(`play-mm7`, `mods-on`, `open-mm7-mods` and so on) to the Swift host, which launches helper apps, stores settings in
macOS user defaults and manages local files. The Atlas never packages or writes original game archives.

| Source | Role |
|---|---|
| `launcher/AtlasApp.swift` | App delegate: page bridge, play actions, data paths, recording, Atelier and Vault actions |
| `launcher/GameBuild.swift` | `GameBuild` enum (`mm6`, `fork` = MM7, `upstream`): helper path, profile, arguments, child environment |
| `launcher/MM7ProfilePath.swift` | MM7 profile resolver, with a guarded disposable-profile override for visible gates |
| `launcher/Atlas/index.html` | The page itself (Play cards, recording toggles, The Vault, The Atelier) |
| `launcher/Info.plist`, `AtlasIcon.svg`, `AtlasIcon.icns`, `make_icon.py` | Bundle metadata and icon. `make_icon.py` packs rendered PNG sizes into the tracked ICNS |
| `launcher/assemble.py`, `launcher/build.sh` | Stage a signed Atlas bundle from verified helper apps |
| `launcher/tests/` | Swift checks for `GameBuild` and `MM7ProfilePath`, Node checks for the page controls |

## Helpers and title locks

The Atlas launches one of three helpers from `Contents/Helpers/<Name>.app`:

| Button | `GameBuild` | Helper | Arguments | Profile under `~/Library/Application Support/Enroth Continuum/` |
|---|---|---|---|---|
| Play MM7 | `fork` | `MM7.app` | `--game mm7 --data-path <data> --user-path <profile>` | `Profiles/MM7-Atlas` |
| Play MM6 | `mm6` | `MM6.app` | `--game mm6 --data-path <data> --user-path <profile>` | `Profiles/MM6-Atlas` |
| Play upstream OpenEnroth | `upstream` | `OpenEnroth.app` | `--data-path <data> --user-path <profile>` | `Profiles/MM7-OpenEnroth` |

`MM6.app` and `MM7.app` are the CMake targets `MM6` and `MM7` in `src/Bin/OpenEnroth/CMakeLists.txt`. They build on
macOS only, from the same sources as `OpenEnroth`, with `EC_MM6_ONLY=1` or `EC_MM7_ONLY=1` and their own bundle IDs
(`org.sklerosai.enrothcontinuum.mm6`, `org.sklerosai.enrothcontinuum.mm7`). `src/Bin/OpenEnroth/OpenEnrothOptions.cpp`
rejects a missing or different `--game` value with "This MM6 app only accepts --game mm6." or the MM7 equivalent.
That rejection is the title lock the assembler checks.

Each title keeps its own profile so saves and configs never mix. An MM7 Atlas profile holds
`enroth-continuum-mm7.ini` and `logs/enroth-continuum-mm7*.log` (names from `src/Application/Startup/GameStarter.cpp`).

### MM7

- Default data folder: `Game Data/MM7` under the Atlas support folder. A folder chosen in the page is stored in the
  `data.mm7` preference and wins.
- Before launch the Atlas checks for the eight archives the engine needs (`anims/magic7.vid`, `anims/might7.vid`,
  `data/bitmaps.lod`, `data/events.lod`, `data/games.lod`, `data/icons.lod`, `data/sprites.lod`, `sounds/audio.snd`),
  matching folder and file names case-insensitively.
- MM7 on macOS does not copy saves from the data folder into a fresh profile. Copy old saves into
  `Profiles/MM7-Atlas/saves` by hand.
- `ENROTH_CONTINUUM_MM7_DISPOSABLE_PROFILE` in the Atlas's own environment replaces the profile, for visible gates. The
  path must be absolute, an immediate child of the system temp folder named `mm7-visible-gate-*`, owned by the user,
  mode `0700`, not a symlink, and must contain a `.ec-mm7-visible-gate` marker with the exact text
  `EnrothContinuum-MM7-visible-gate-v1`. Anything else makes the profile invalid and the Atlas refuses to launch.

### MM6

- Default data folder: `Game Data/MM6/app` under the Atlas support folder, or the `data.mm6` preference.
- The readiness check looks for `anims/anims1.vid`, `data/games.lod`, `data/icons.lod` and `sounds/audio.snd`.
- On launch the Atlas copies `openenroth.ini` from the integration profile `Profiles/MM6` into `Profiles/MM6-Atlas`
  only when the Atlas profile has none. It then applies two one-time window defaults (resizable, and windowed on first
  run), tracked by a version number in preferences, so a player's later window choice is kept. A resizable window is the
  player's route to fullscreen, since macOS greys out the fullscreen button of a fixed-size window.
- MM6 gets no recording and no Private mods switch. The child environment drops `ENROTH_MM7_MOD_DIR`,
  `ENROTH_MM7_ALL_ARTIFACTS_STORAGE`, `ENROTH_MM7_CHEST_REROLL`, `ENROTH_MM7_MONSTER_LOOT_REROLL`,
  `ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY` and `OPENENROTH_MM7_PATH`.
- MM6 honours four Atelier options of its own, set on every Play MM6 launch whatever the Private mods switch says:
  - Fresh vendor stock on entry. Atlas sets `ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY` to `1` or `0` from the preference
    `mods.mm6.vendorRefreshOnEntry`. See [Vendor refresh on entry](#vendor-refresh-on-entry).
  - All Artifacts. Atlas sets `ENROTH_MM6_ALL_ARTIFACTS` to `1` or `0` from the preference `mods.mm6.allArtifacts`.
    See [All Artifacts for MM6](#all-artifacts-for-mm6).
  - Reroll chest loot. Atlas sets `ENROTH_MM6_CHEST_REROLL` to `1` or `0` from the preference `mods.mm6.chestReroll`.
    See [Chest reroll](#chest-reroll).
  - Fortune's Turn. Atlas sets `ENROTH_MM6_MONSTER_LOOT_REROLL` to `1` or `0` from the preference
    `mods.mm6.monsterLootReroll`. See [Fortune's Turn](#fortunes-turn).
- Vendor refresh has one "MM6 + MM7" row in the Atelier that writes both titles' preferences. All Artifacts, chest
  reroll and Fortune's Turn each have their own "· MM6" row below the MM7 rows, and its status reads "On for next MM6
  launch" when on.
- Play MM6 is a playtest entry. MM8 is a data-detection card only. There is no MM8 helper.

### Upstream OpenEnroth

The upstream button is for playtesting current upstream on the same MM7 data. It prefers a bundled
`Contents/Helpers/OpenEnroth.app` and otherwise uses `/Applications/OpenEnroth.app`. Without either, the button stays
visible but disabled. Upstream plays MM7 only and has no `--game` option, so the Atlas passes only `--data-path` and
`--user-path`. Its profile `Profiles/MM7-OpenEnroth` is created before launch, because upstream requires an existing
folder, and is never the fork's `MM7-Atlas`.

The upstream child environment drops `OPENENROTH_MM7_PATH` (upstream's fallback data path) and every fork mod flag,
so upstream cannot fall back to `~/.openenroth` or another data folder. Upstream reads only the data folder and writes
its config, saves and logs into the profile. On its first start with an empty profile it copies any `saves` folder from
the data folder into the profile.

## Recording

Two persistent toggles, **Record video** and **Record diagnostics**, apply to Play MM7 and Play upstream OpenEnroth.
The Atlas bundles `tools/mm7_playtest_capture.sh` as `Contents/Resources/Atlas/mm7_playtest_capture.sh` and runs it
beside the game. Video keeps a rolling two-minute replay. **Command-Shift-R** keeps the current replay as a clip, and a
crash or abnormal exit keeps one automatically. Diagnostics keep log ranges, changed save snapshots, the build identity
and launch command, the exit status and crash reports. The hotkey uses the Carbon hot key API and is registered only
while a video session runs. Video needs macOS Screen Recording permission. Diagnostics do not. The recording folder,
its drive identity and the toggles live in `recording.mm7.*` preferences. Retention limits and the capture script's
own options are documented in [MM7_PLAYTEST_CAPTURE.md](../tools/MM7_PLAYTEST_CAPTURE.md) and on the testing page.

## Building and staging the Atlas

`launcher/build.sh` runs `python3 launcher/assemble.py` with the same arguments.

```sh
cmake -B build-release -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DOE_BUILD_TESTS=OFF
cmake --build build-release --target MM7
launcher/build.sh --mm7-app build-release/src/Bin/OpenEnroth/MM7.app
```

A Debug helper works but is larger and slower, so the installed Atlas packages a Release `MM7` build.

| Option | Meaning |
|---|---|
| `--mm7-app PATH` | Title-locked `MM7.app`. Required unless `--reuse-from` supplies one |
| `--mm6-app PATH` | `MM6.app`, only after its play gate |
| `--mm8-app PATH` | `MM8.app`, only after its play gate. No target builds one today |
| `--openenroth-app PATH` | Upstream `OpenEnroth.app`. A sibling `BUILD-INFO.txt` is copied to `Contents/Resources/Helpers/OpenEnroth-BUILD-INFO.txt`, where the recorder reads the upstream commit |
| `--mm7-deps-build-root DIR` | Output of `tools/build_mm7_macos_dependencies.py`. Verifies and bundles its source notices |
| `--reuse-from APP` | Existing Atlas whose helpers (and their `BUILD-INFO.txt`) are carried forward unless replaced explicitly |
| `--output PATH` | Default `build/Atlas/Enroth Continuum.app` |

What the assembler checks and does:

1. Finds the macOS SDK and `swiftc` through `xcrun`. It refuses an output under `/Applications`, so a build always
   stages first and installing is a separate, manual step after launch and save/load checks.
2. Verifies each helper: bundle ID and executable name from `Info.plist`, an arm64 slice (`lipo -archs`), and for
   `MM6`, `MM7` and `MM8` the title lock. It runs the helper with the other title's `--game` and `--headless` and
   expects the exact rejection text. A GUI bundle can stay resident after its `main` returns, so on a timeout it
   falls back to finding the rejection string in the binary. Upstream has no title lock to probe.
3. Validates the tracked `AtlasIcon.icns` header. Packaging needs no Homebrew or SVG tools.
4. With `--mm7-deps-build-root`, reads `build-report.json` and requires arm64, source commits equal to
   `thirdparty/repo-source/SOURCES.json`, exactly nine static archives with matching SHA-256 and no dylibs, and a
   matching notice set that includes each source's primary license. The notices land in
   `Contents/Resources/Notices/MM7-Media`. This is explicit because a helper linked against older cached libraries
   cannot be told apart from bundle metadata alone.
5. Compiles `AtlasApp.swift`, `MM7ProfilePath.swift` and `GameBuild.swift` with `-O` against AppKit, WebKit and Carbon
   into `Contents/MacOS/EnrothContinuum`. The Swift module cache lives in `~/Library/Caches/EnrothContinuum`.
6. Copies the page, the capture script, the icon, and `LICENSE`, `CREDITS.md` and `docs/upstream.md` (as
   `UPSTREAM.md`) into `Contents/Resources/Notices`.
7. Copies helpers with `ditto`, re-verifies them, and ad-hoc signs each helper and then the app (`codesign --sign -`).
8. Builds in a temporary folder beside the output and swaps it in with a rename. A failed build or a wrong-title helper
   leaves the previous staged Atlas intact.

The Atlas is ad-hoc signed, so macOS may ask again for Screen Recording permission after each new install.

## Launcher tests

Both checks run without building the app or the engine.

```sh
xcrun swiftc launcher/MM7ProfilePath.swift launcher/GameBuild.swift launcher/tests/main.swift -o "$TMPDIR/atlas-tests"
"$TMPDIR/atlas-tests"          # prints "MM7 profile path and game build checks passed"
node launcher/tests/atlas_page.js   # prints "Atlas page checks passed"
```

`tests/main.swift` checks the profile override rules (relative path, missing marker, wrong permissions), helper
discovery with stub executables (bundled first, external upstream fallback, disabled when absent), that upstream and MM6
never share the fork's profile, the exact argument lists per build, and the child environment per build. In
particular, upstream and MM6 receive no fork mod keys and no `OPENENROTH_MM7_PATH`, and vanilla MM7 gets the mod
flags forced to `0`. MM6 gets `ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY`, `ENROTH_MM6_ALL_ARTIFACTS`,
`ENROTH_MM6_CHEST_REROLL` and `ENROTH_MM6_MONSTER_LOOT_REROLL` from its own choices, and MM7 and upstream never get
them. `tests/atlas_page.js` runs the page script against a fake DOM and checks
the play, recording, vendor, artifact and reroll controls.

## MM7 private mods

All private mods are opt-in. Fresh vendor stock on entry, All Artifacts, chest reroll and Fortune's Turn also exist
for MM6, each with its own MM6 variable, see [Vendor refresh on entry](#vendor-refresh-on-entry),
[All Artifacts for MM6](#all-artifacts-for-mm6), [Chest reroll](#chest-reroll) and [Fortune's Turn](#fortunes-turn).
The MM7 mods live in the user's home folder, never in the repository or in game archives. The engine sees them only
through environment variables that the Atlas sets for a Play MM7 launch when the **Use private mods** switch is on
(preference `mods.mm7.enabled`). With the switch off (Vanilla), the Atlas removes `ENROTH_MM7_MOD_DIR` and sets the
four MM7 boolean flags to `0`.

| Variable | Set by the Atlas when | Read in |
|---|---|---|
| `ENROTH_MM7_MOD_DIR` | Private mods on. Value is `~/Library/Application Support/Enroth Continuum/Mods/MM7` | `AssetsManager::getCustomPortrait`, `ItemTable::ApplyPrivateArmorOverrides` |
| `ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY` | Private mods on and preference `mods.vendorRefreshOnEntry` | `GUIWindow_Shop::onHouseEntered`, `GUIWindow_MagicGuild::onHouseEntered` |
| `ENROTH_MM7_ALL_ARTIFACTS_STORAGE` | Private mods on and preference `mods.mm7.allArtifactsStorage` | `mm7::mods::allArtifactsStorageEnabled` |
| `ENROTH_MM7_CHEST_REROLL` | Private mods on and preference `mods.mm7.chestReroll` | `mm7::mods::chestRerollEnabled` |
| `ENROTH_MM7_MONSTER_LOOT_REROLL` | Private mods on and preference `mods.mm7.monsterLootReroll` | `mm7::mods::monsterLootRerollEnabled` |
| `ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY` | Every Play MM6 launch, `1` or `0` from preference `mods.mm6.vendorRefreshOnEntry` | `mm6::vendorRefreshPolicy` |
| `ENROTH_MM6_ALL_ARTIFACTS` | Every Play MM6 launch, `1` or `0` from preference `mods.mm6.allArtifacts` | `mm6::mods::allArtifactsEnabled` |
| `ENROTH_MM6_CHEST_REROLL` | Every Play MM6 launch, `1` or `0` from preference `mods.mm6.chestReroll` | `mm6::mods::chestRerollEnabled`, see [Chest reroll](#chest-reroll) |
| `ENROTH_MM6_MONSTER_LOOT_REROLL` | Every Play MM6 launch, `1` or `0` from preference `mods.mm6.monsterLootReroll` | `mm6::mods::monsterLootRerollEnabled`, see [Fortune's Turn](#fortunes-turn) |

Every MM7 reader checks `GameTitle::MM7` first, so the MM7 variables do nothing in an MM6 run, and the MM6 readers
check `GameTitle::MM6`. The boolean flags
accept only the exact string `1`. Values such as `true` or `01` leave the mod off. None of these settings is written
into saves, the engine config or game archives, so turning a mod off restores vanilla rules on the next launch. Saves
do keep what a mod built up while it was on, such as the All Artifacts ledgers and which chests can still be rerolled. A
direct diagnostic launch of the engine may set the same variables by hand.

The mods folder may hold only these files. Anything else is ignored.

```text
Mods/MM7/
  README.md                  written once by "Open mods folder", never overwritten
  armor.tsv                  armor AC overrides
  portrait-<1..4>.png        legacy single portrait per party slot
  portraits/<1..4>/<mood>.png
```

After an MM7 run with private mods, the Atlas scans the newest profile log for the engine's "Ignoring invalid MM7
portrait" and "Ignoring private armor overrides" warnings and reports them in the page.

### Portraits

`src/Engine/Mods/PrivateMods.{h,cpp}` loads portraits once per run, on the first `getCustomPortrait` call. There are
20 moods: normal, talk, smile, sad, hurt, pain, afraid, cast, sleep, unconscious, weak, cursed, drunk, insane, sick,
paralyzed, stone, zombie, dead and eradicated. `mm7::mods::portraitMood` maps the engine's `PortraitId` frame and the
character's `Condition` to a mood. Eradicated, dead and zombie conditions take priority over the frame, because a
zombie uses the normal frame in the original frame table.

Loading rules:

- A slot is active only when it has a valid normal image, from `portraits/<slot>/normal.png` or else the legacy
  `portrait-<slot>.png`. Other moods for that slot are loaded only when the normal image is present.
- A PNG must be at most 4 MiB and between 32 and 512 pixels in each dimension. An invalid file logs a warning without
  its path and is skipped.
- Fallbacks: pain uses hurt, unconscious uses sleep, then any missing mood uses normal. Dead and eradicated never fall
  back. Without their own PNGs the original death art shows.
- Voices and body art stay original. Portraits apply in party creation and in play.

The Atelier imports single portraits or a folder of named mood PNGs for one slot, converting each to PNG before
placing it. Changes take effect on the next launch.

### Armor overrides (`armor.tsv`)

`ItemTable::ApplyPrivateArmorOverrides` runs at the end of item table loading. Each non-comment line is
`item_id<TAB>base_ac<TAB>bonus_ac`. Lines starting with `#` and blank lines are skipped, and a trailing CR is allowed.
The override writes the item template's `damageDice` (base AC) and `damageMod` (bonus AC), so it changes every copy
of that item type while the profile runs with private mods.

The file is all or nothing. Any bad row rejects the whole file and the original values stay. A row is bad when it is
longer than 128 characters, does not have exactly three tab-separated integers, names an item outside the valid item
range or one whose type is not armour, repeats an ID, has an AC outside 0 to 100, or is the 33rd row. The Atelier
writes a one-row file for an item picked from its fixed armor list. A hand-edited file may hold up to 32 rows.

### Vendor refresh on entry

#### MM7

With the flag set, entering a weapon, armor, magic or alchemy shop regenerates both its standard and special stock
once, and entering a magic guild regenerates its spellbooks once. The next deadline is then set from the house's
normal generation interval. One `VendorRefreshVisit` (`src/GUI/UI/Houses/VendorRefreshMod.h`) belongs to each house
window, so switching to a buy tab in the same visit does not reroll again. Closed and banned shops never open a
window, and non-vendor houses have no hook. With the flag off the original refresh-on-expiry path in the buy dialogue
is unchanged.

#### MM6

`ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY` accepts only the exact string `1` (`mm6::vendorRefreshPolicy` in
`src/Engine/MM6/MM6ShopRefreshBoundary.{h,cpp}`). Each MM6 shop and spell guild window (`GUIWindow_MM6Shop`) reads it
when the party enters and holds an `OnEntry` policy for that visit. MM6 rolls stock when a buy screen opens, so the
first buy screen of the visit regenerates the stock through the same generator and title RNG as the vanilla deadline
refresh, and writes the ordinary next deadline. A shop rolls both banks, a guild its spellbooks. Later buy screens of
the visit go back to the deadline rule, so switching between Buy Items and Special Items doesn't reroll. The result
lives in the normal stock and clock owners and is saved in `SHOP` and `SCLK` like any other stock. With the flag off
the deadline schedule resumes from the last written deadline.

#### Atlas

The Atelier shows one toggle, "MM6 + MM7". It writes two preferences, `mods.vendorRefreshOnEntry` for MM7 and
`mods.mm6.vendorRefreshOnEntry` for MM6, so each title keeps its own saved choice. MM7 applies its choice only with
Private mods. MM6 has no Private mods and applies its choice on every Play MM6 launch, as
`ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY=1`, or `0` when off. The MM7 and upstream launches drop the MM6 variable.

### All Artifacts + Storage

This mod removes the vanilla cap on random artifacts and changes artifact uniqueness from "generated" to "collected".
Treasure levels, the tier-six artifact roll, loot families, item statistics and the random artifact list
(`allSpawnableArtifacts()`) do not change. Fixed-location and quest artifacts are never added to random loot.

- `Party::isArtifactGenerated` is still kept up to date. With the mod on, `Item::GenerateArtifact` and the tier-six
  path in `ItemTable` exclude items in `Party::isArtifactCollected` instead, so the `artifact_limit` config value no
  longer caps generation. The config value itself is not rewritten.
- `Party::recordArtifactAcquisition` marks an artifact collected when it goes onto the cursor
  (`Party::setHoldingItem`), into a character's inventory through `Party::addItemToParty`, or straight from a chest grab
  (`Chest.cpp`). Selling, dropping or storing it later does not clear the bit. An artifact that was generated but never
  picked up stays eligible.
- Once every eligible artifact is collected, tier-six rolls fall back to ordinary loot.
- Two world sources can hold the same uncollected artifact at once. The mod does not track or remove world instances.

The collected ledger is saved in 39 previously unused bytes of `Party_MM7::field_7d7` in
`src/Engine/Snapshots/EntitySnapshots.{h,cpp}`, with a version marker, so `party.bin` keeps its native size and layout.
Once written, the ledger survives with the mod off. On first opt-in an older save is migrated conservatively. Only
artifacts in backpacks, equipment and the cursor are marked. Artifacts left in chests are marked when next taken.
Migration never moves, deletes or duplicates an item.

The mod adds no storage UI. Castle Harmondale's existing chests are the storage. The engine already saves every chest's
item records and grid in the indoor map delta, and the game tests check that a boundary chest keeps its items and the
collection ledger across a save round trip.

### Chest reroll

#### MM7

The Atelier's "Reroll chest loot · MM7" toggle adds a small Reroll button, in brass and dark wood, to the lower wooden
border of an MM7 chest that still holds an authored random-loot marker. The button is absent when the flag is off and
when no marker remains.

- The chest keeps each authored random marker as a hidden record. Its generated items link back to it through
  `Item::chestRerollSource`, saved in the formerly unused pad byte at offset 0x1B of `Item_MM7`, so the native chest
  record keeps its size. 0 is an ordinary item, 1 to 140 names the marker slot and 255 marks a hidden source marker.
- Each click draws one value from the gameplay RNG to seed a fresh roll of the chest's unclaimed random slots through
  the ordinary MM7 chest generator (`Chest::reroll`). The marker and the map's current treasure level give the same
  treasure-level range as the first generation. The chest type, flags, fixed items, trap and map state stay.
- An artifact from a discarded roll that was never claimed has its generated flag released before the new roll. All
  Artifacts + Storage keeps using its collected ledger.
- Taking or depositing an item removes the hidden markers, so the chest can't be rerolled after it is looted. This
  holds across save and load.
- A save whose chest was generated before the mod was on has no marker for it. That chest can't be rerolled until its
  map respawns.

#### MM6

`ENROTH_MM6_CHEST_REROLL` accepts only the exact string `1` (`mm6::mods::chestRerollEnabled` in
`src/Engine/Mods/PrivateMods.{h,cpp}`). With it on, an MM6 chest whose random loot can still be rolled again shows a
Reroll button in the same brass and dark wood, on the chest's right-hand panel above the exit button. The page button
sits between them when the chest has more than one page.

- MM6 rolls a map's chests when the map loads, and the roll writes over the random-loot markers in the native chest
  record. MM6 data already uses the item record's pad byte, so MM6 keeps its own record of the roll instead. When a
  map's chests roll with the mod on, `mm6::noteRolledChests` keeps a layout for every chest that had a marker: per item
  slot, the marker (-1 to -7), an empty slot or a fixed item (`MM6ChestReroll.h`). With the mod off the roll forgets any
  layout those chests had.
- Each click puts the layout's markers and empty slots back over the chest's current record and runs the ordinary MM6
  chest generator and placement on it (`mm6::stageChestReroll`), with the chest random state and the map's treasure
  level, so the treasure-level ranges are those of the first roll. The chest type, flags, trap and fixed items stay.
- An artifact of the discarded roll comes off the generated list, also with
  [All Artifacts for MM6](#all-artifacts-for-mm6) on. The party never took it, so the collected ledger doesn't have it.
- MM6 chests only let items be taken out. Taking an item or a gold pile forgets the chest's layout, so a looted chest
  can't be rerolled, also after a save and a load.
- A chest rolled before the mod was on has no layout and can't be rerolled until its map refills.

The layouts are saved in the optional `CHRL` section of a v7 save, after `ARTC`, only when there are any, so a game
that never used the mod saves the same bytes as before. A save without `CHRL` loads with no rerollable chest. A
first-map v5 save doesn't carry it, see [MM6 save format](mm6-save-format.md).

### Fortune's Turn

The Atelier's "Fortune's Turn · MM7" toggle is the launch switch. In play, a dice checkbox on the left of the HUD, beside
the first party portrait, turns the popup on and off at once. It shows a gold X when on and is saved in the MM7 config
as `mm7_monster_loot_reroll_in_game`. Every eligible result opens the same paused stone frame (`UIMonsterLoot`) with
Reroll and Take. Item results show the item's art, name, attack or armor stats, magic bonus, value and description
without identifying the item. Escape takes nothing for a corpse and keeps the current result elsewhere.

| Source | What Reroll repeats | What stays |
|---|---|---|
| Dead monster with an ungenerated random reward | Gold dice, item-drop chance, treasure level and type | Take grants it once and removes the corpse. Previews restore the artifact ledger. |
| Clicked heap, decoration or face that yields a random item | The item, at the event's treasure level and type | The event ran once and its other effects don't repeat. Take puts the item in hand. |
| Monthly town-hall bounty | The monster, from that hall's normal eligible pool | The monthly deadline. A killed or claimed bounty can't be rerolled. |
| Ore smith services, global events 501 to 503 | The item, at the ore's treasure level and the smith's item type | The ore is consumed once. Take or Escape puts the selected item in hand. |
| MM6 Circus prize trades, global events 129 to 134 | The item, at the trade's treasure level and item type | The pyramid or keg is consumed once. Take or Escape puts the selected item in hand. |
| Successful Enchant Item on a regular weapon | The special enchantment, from the spell's weapon and mastery pool | The cast's mana and recovery, charged once, and the weapon |

Monsters with carried or stored items or gold, fixed drops, authored gold, failed casts, broken items, armor,
deterministic weapon effects and ground items that already exist keep their normal path. Starting a new MM7 game with
the mod on arms a one-time test setup. On the first visit to Emerald Isle's Dragon's Lair in that session the first live
dragon is already a lootable corpse. Loading another save cancels it.

#### MM6

`ENROTH_MM6_MONSTER_LOOT_REROLL` accepts only the exact string `1` (`mm6::mods::monsterLootRerollEnabled` in
`src/Engine/Mods/PrivateMods.{h,cpp}`). The MM6 logic is in `src/GUI/UI/MM6FortunesTurn.{h,cpp}`, with thin
`GameTitle::MM6` hooks in the shared popup, `Actor`, the event interpreter and the HUD. It shares the MM7 popup, its
buttons and its exclusions, and rolls with MM6's own rules:

- A corpse rolls its gold dice and drop chance on the gameplay RNG and its item through MM6's item picker on the item
  LCG, the way `Actor::LootActor` does for MM6. Take gives both, removes the corpse, and marks a taken artifact
  generated in the `CHST` flags.
- A clicked decoration or face whose event runs GiveItem with no fixed item rolls through the same picker. The rest
  of the event runs as usual, once. Take puts the item on the cursor the way MM6 GiveItem does (`mm6::holdEventItem`),
  moving a held item to a backpack. GiveItem with a fixed item, which MM6 rolls and then renumbers, keeps its normal
  path. The shipped sources are the interactive decorations whose global events 426 to 428, 432 to 434, 437 and 439 to
  442 give random items, and map events of `cd2`, `d01`, `d02`, `d03`, `d05`, `d10`, `d12` to `d16`, `t3`, `t4` and
  `t5`, for example the hanging and bone cages of the Abandoned Temple (`d02`, events 16 to 23).
- The New Sorpigal, Misty Islands and Silver Cove town halls open the popup when the bounty topic shows a live bounty.
  Reroll draws from MM6's one bounty pool (MM6.exe 0x4A3238), and the NPC text follows the new monster.
- The Circus of the Sun's prize trades are MM6's closest match to the MM7 ore smith. Najat Ramadi, Kerman Murian and
  Irbil Baktarian, in the three Dragonsand tents (`outb3`, houses 267, 555 and 556), take a Golden Pyramid (item 472)
  for a level 6 armor, weapon or accessory (global events 129 to 131) or a Keg of Wine (473) for a level 4 one (132 to
  134). A trade that finds the prize consumes it once and opens the popup. Reroll rolls the item again through the same
  picker, and Take or Escape puts the shown item on the cursor the MM6 GiveItem way. `mm6::isCircusPrizeTrade` admits
  only the shipped scripts. A party without the prize gets the trader's refusal, as in vanilla.
- Each roll moves the item LCG on but leaves the generated artifact flags and the All Artifacts collected ledger alone,
  so a preview never uses up an artifact. A taken item carries the native record the picker made, so the game saves
  with it.
- Escape takes nothing from a corpse or a clicked source, keeps the bounty the popup showed, and takes the item a
  Circus trade showed.

The checkbox sits in the marble right of the fourth portrait's bars, with a dice under it, and is saved in the engine
config as `mm6_monster_loot_reroll_in_game`. Its button exists only while the launch flag is set. MM6 has no Enchant
Item player spell (`mm6::isVerifiedPlayerSpell`), so the enchantment reroll has no MM6 counterpart, and there is no
new-game corpse setup.

### Candidate MM7 mods

- A read-only game-menu panel that lists the MM7 options Atlas supplied at launch and the live Fortune's Turn checkbox.
  The owner selected it for the backlog. It is not implemented.
- Loot reroll for MM8 is future work.
- From the save-and-reload tactics reviewed on 2026-11-20: an in-shop reroll button needs a purchase guard and is
  deferred, since vendor refresh on entry already covers the shortcut. Rerolling hireling professions would mutate the
  map's generated NPCs and is deferred. Repeating a dragon corpse's payout after Take is excluded.
- An equipment override for one item needs its own durable item identity. The `Item_MM7` pad byte is taken by chest
  reroll.

### All Artifacts for MM6

`ENROTH_MM6_ALL_ARTIFACTS` accepts only the exact string `1` (`mm6::mods::allArtifactsEnabled` in
`src/Engine/Mods/PrivateMods.{h,cpp}`). It is the cap-removal and collected-uniqueness half of All Artifacts + Storage.
MM6 has no storage half, see below.

MM6's random artifacts are items 400 to 429, its 15 artifacts and 15 relics, all 30 of which the generator can roll
(`items.txt` rarity, the `RNG % 30` candidate draw in MM6.exe's item picker). MM6.exe has two artifact pickers. The
item picker gives the drawn candidate at tier 6 with a 5% chance when it wasn't generated and fewer than 13
were, and a tier-7 chest marker goes to a direct picker that skips the artifacts the party carries. The port's safe
direct picker also skips generated ones and caps their union at 13. With the mod on:

- Both pickers skip collected artifacts instead of generated ones, with no cap (`pickChestBaseItemId`,
  `pickDirectChestArtifactSafePolicy`). The direct picker still skips what the party carries. The draws are the same
  as without the mod, so a roll that would land on an uncollected artifact gives it.
- The generated flags in `CHST` are still kept up to date, so turning the mod off returns to MM6.exe's rule on the
  next launch with an honest generated list.
- The rule reaches chest generation (`applyActiveChestLiveOwnerHandoff`), event GiveItem (`mm6::giveEventItem`),
  monster loot (`mm6::generateTitleItem`) and the tier-7 direct picker of random ground treasure
  (`mm6::spawnRandomTreasure`), so a party can get artifacts from monsters too. Shops and ground items never ask the
  generator for any item, so they roll no artifact either way.
- An artifact is collected when it reaches the cursor or a backpack, through `Party::recordArtifactAcquisition`. Taking
  it from a chest puts it on the cursor. Throwing, selling or losing it later doesn't clear the mark. An artifact that
  was rolled but never picked up stays eligible, and two copies can then exist in the world.
- The first time the mod is consulted in a game, the ledger starts with the artifacts the party carries, in backpacks,
  on paperdolls and on the cursor (`mm6::initializeArtifactCollection` in `MM6ArtifactCollection.h`).
- Once all 30 are collected, rolls fall back to ordinary loot.

The ledger is saved in the optional `ARTC` section of a v7 save, after `TIMR`, only once the mod started it, so a game
that never ran the mod saves the same bytes as before. A load takes it from the save, and a new game or a save without
`ARTC` starts empty. Once written, the ledger survives with the mod off. A first-map v5 save doesn't carry it, a game
loaded from one rebuilds it from what the party carries. See [MM6 save format](mm6-save-format.md).

MM6 has no storage the way MM7 has Castle Harmondale. The party owns no house, the port's MM6 chests only let items be
taken out, and every MM6 map refills, which regenerates its chests (`mapstats.txt` refill days 1 to 672, all 67 maps).
The four backpacks are the only place to keep artifacts, and a save today refuses a party that carries one, see
[Open questions](open-questions.md).

The Atelier shows "All Artifacts · MM6" next to the MM7 toggle. It writes `mods.mm6.allArtifacts`, and Play MM6 passes
it as `ENROTH_MM6_ALL_ARTIFACTS=1`, or `0` when off. The MM7 and upstream launches drop the variable.

## Source

- [AtlasApp.swift](../launcher/AtlasApp.swift), [GameBuild.swift](../launcher/GameBuild.swift),
  [MM7ProfilePath.swift](../launcher/MM7ProfilePath.swift), [index.html](../launcher/Atlas/index.html)
- [assemble.py](../launcher/assemble.py), [build.sh](../launcher/build.sh), [make_icon.py](../launcher/make_icon.py),
  [launcher README](../launcher/README.md)
- [tests/main.swift](../launcher/tests/main.swift), [tests/atlas_page.js](../launcher/tests/atlas_page.js)
- [OpenEnroth CMakeLists.txt](../src/Bin/OpenEnroth/CMakeLists.txt) (`MM6`, `MM7` targets),
  [OpenEnrothOptions.cpp](../src/Bin/OpenEnroth/OpenEnrothOptions.cpp) (title lock)
- [PrivateMods.h](../src/Engine/Mods/PrivateMods.h), [PrivateMods.cpp](../src/Engine/Mods/PrivateMods.cpp),
  [PrivateMods_ut.cpp](../src/Engine/Mods/PrivateMods_ut.cpp)
- [MM6ArtifactCollection.h](../src/Engine/MM6/MM6ArtifactCollection.h),
  [MM6ArtifactCollectionSection.cpp](../src/Engine/MM6/MM6ArtifactCollectionSection.cpp),
  [MM6ChestBaseItemPicker.h](../src/Engine/MM6/MM6ChestBaseItemPicker.h),
  [MM6ChestDirectArtifactPicker.h](../src/Engine/MM6/MM6ChestDirectArtifactPicker.h)
- [Chest.cpp](../src/Engine/Objects/Chest.cpp), [UIChest.cpp](../src/GUI/UI/UIChest.cpp),
  [UIMonsterLoot.cpp](../src/GUI/UI/UIMonsterLoot.cpp), [TownHall.cpp](../src/GUI/UI/Houses/TownHall.cpp),
  [MM6FortunesTurn.cpp](../src/GUI/UI/MM6FortunesTurn.cpp), [MM6House.cpp](../src/GUI/UI/Houses/MM6House.cpp)
- [AssetsManager.cpp](../src/Engine/AssetsManager.cpp), [ItemTable.cpp](../src/Engine/Tables/ItemTable.cpp),
  [Item.cpp](../src/Engine/Objects/Item.cpp), [Party.cpp](../src/Engine/Party.cpp),
  [EntitySnapshots.cpp](../src/Engine/Snapshots/EntitySnapshots.cpp)
- [VendorRefreshMod.h](../src/GUI/UI/Houses/VendorRefreshMod.h),
  [VendorRefreshMod_ut.cpp](../src/GUI/UI/Houses/Tests/VendorRefreshMod_ut.cpp),
  [Shops.cpp](../src/GUI/UI/Houses/Shops.cpp), [MagicGuild.cpp](../src/GUI/UI/Houses/MagicGuild.cpp),
  [MM6ShopRefreshBoundary.h](../src/Engine/MM6/MM6ShopRefreshBoundary.h),
  [MM6Shop.cpp](../src/GUI/UI/Houses/MM6Shop.cpp)
- Game tests: [GameTests_MM7Vendor.cpp](../test/Bin/GameTest/GameTests_MM7Vendor.cpp),
  [GameTests_MM7AllArtifactsEngine.cpp](../test/Bin/GameTest/GameTests_MM7AllArtifactsEngine.cpp),
  [GameTests_MM7HarmondaleStorage.cpp](../test/Bin/GameTest/GameTests_MM7HarmondaleStorage.cpp). The artifact tests
  skip unless `ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1` is set (one control test skips when it is set). The same file's
  `MM7ChestReroll`, `MM7MonsterLootReroll` and `MM7FortunesTurn` tests skip unless their own flag is set, except the
  preview test, which always runs. The vendor tests
  skip unless `ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY=1` is set. The MM6 vendor tests in
  [GameTests_MM6Houses.cpp](../test/Bin/GameTest/GameTests_MM6Houses.cpp) (`MM6Houses.*VendorRefresh*`) and the MM6
  artifact tests in [GameTests_MM6AllArtifacts.cpp](../test/Bin/GameTest/GameTests_MM6AllArtifacts.cpp), the MM6
  Fortune's Turn tests in [GameTests_MM6FortunesTurn.cpp](../test/Bin/GameTest/GameTests_MM6FortunesTurn.cpp) and the
  MM6 chest reroll tests (`MM6ChestReroll.*`) in [GameTests_MM6Chest.cpp](../test/Bin/GameTest/GameTests_MM6Chest.cpp)
  set their own flag. The MM6 ledger's fresh-process pair runs in `tools/run_mm6_firstmap_gate.py`.

See also: [Build setup](build-and-dev-setup.md), [Testing](testing.md), [MM7 status](mm7-status.md), [Upstream](upstream.md), [Source and privacy](source-and-privacy.md)
