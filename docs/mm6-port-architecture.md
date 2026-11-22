# MM6 port architecture

This page says where the MM6 port lives in the source tree, how MM6 code is kept apart from MM7, how GrayFace fixes are
switched, and how MM6 code is named and driven in tests.

For the engine as a whole see [Engine Architecture](engine-architecture.md).

## Where MM6 code lives

MM6 runs on the shared OpenEnroth engine. There is no separate MM6 engine. MM6 behaviour is added in three ways:

- MM6-only files with an `MM6` prefix, next to the subsystem they extend.
- Title checks inside shared code, `engine->gameTitle() == GameTitle::MM6`, that call into an MM6 helper.
- Title-aware loaders that take a `GameTitle` argument, such as `EvtProgram::load`, `deserialize(..., gameTitle())` and
  `MapTable::Initialize`.

| Folder | MM6 files | What they hold |
|---|---|---|
| [src/Engine/MM6](../src/Engine/MM6/CMakeLists.txt) | about 300 | The port's core: `MM6.exe` access, the EC6 save, world and map state, chests, shops, items, monsters, party rules. Tour below. |
| [src/Engine/Evt](../src/Engine/Evt/CMakeLists.txt) | `MM6*.cpp/h` | Event variables and their dispatch, per-opcode helpers such as `MM6GiveItemEvent`, quest, autonote and map byte state. |
| [src/Engine/Spells](../src/Engine/Spells/CMakeLists.txt) | `MM6*.cpp/h` | Spell rules decoded from `MM6.exe` (`MM6SpellRules`), cure and buff rules, first aid. |
| [src/Engine/Tables](../src/Engine/Tables/CMakeLists.txt) | `MM6ItemTable.cpp`, `MM6PotionCombinations` | Item table parser and potion mixing. Other tables have `InitializeMm6` methods. |
| [src/Engine/Snapshots](../src/Engine/Snapshots/CMakeLists.txt) | `MM6IndoorSnapshot`, `MM6Outdoor*Snapshot` | MM6 map file layouts. Frame tables are title-aware in `TableSerialization.cpp`. |
| [src/GUI](../src/GUI/CMakeLists.txt) | `MM6HudLayout.h`, `MM6HudActionControls.h`, `MM6HudBookControls.h`, `MM6TopPanelResources.h` | HUD geometry and resources. |
| [src/GUI/UI](../src/GUI/UI/CMakeLists.txt) | `MM6*.cpp/h` | Character screen, spellbook, save/load menu, NPC and street NPC dialogue, room descriptors, paperdoll art, layouts. |
| [src/GUI/UI/Houses](../src/GUI/UI/Houses/CMakeLists.txt) | `MM6*.cpp/h` | `GUIWindow_MM6House` and its subclasses for shops, guilds, skill guilds, temples, taverns, training and transport. |
| [src/Media](../src/Media/MM6MovieLookup.h) | `MM6MovieLookup.h` | Movie name lookup in the MM6 VID archives. |

Each of these folders has a `Tests/` subfolder with `MM6*_ut.cpp` unit tests. Game tests are
`test/Bin/GameTest/GameTests_MM6*.cpp`. Media, HUD and house presentation details are on
[MM6 Media and UI](mm6-media-and-ui.md).

## Title gating

`GameTitle` is fixed for the life of the process. Four mechanisms keep MM6 and MM7 apart:

- **Build time.** The macOS `MM6` app is compiled with `EC_MM6_ONLY=1` and accepts only `--game mm6`. It refuses the
  `play` and `retrace` subcommands, since traces still read MM7 saves.
- **Runtime checks.** Shared code branches on `engine->gameTitle()`. When the branch is more than a line it calls an
  `mm6::` function, so the MM6 rule sits in an MM6 file and the shared file only routes.
- **Title-locked helpers.** Many functions in `src/Engine/MM6` take a `GameTitle` parameter and throw unless it is
  `GameTitle::MM6`. Callers pass `GameTitle::MM6` explicitly. A call that reaches MM6 code from an MM7 path fails loudly
  instead of corrupting MM7 state.
- **Separate profiles.** MM6 uses its own user path, config and save extension (`.ec6`), so an MM6 process never reads
  or writes MM7 saves. Paths are on [Build and Dev Setup](build-and-dev-setup.md).

MM6 map IDs reuse the `MapId` type but not the MM7 enum values. Use the title-aware helpers in
[MapEnumFunctions.h](../src/Engine/MapEnumFunctions.h), and look MM6 maps up by file name through the map table.

### Fail closed

An MM6 path whose original behaviour is not established is refused rather than left to fall through to MM7 code. Some
examples from the source:

- `refuseUnverifiedMM6Spell` in [Game.cpp](../src/Application/Game.cpp) shows "This Might & Magic VI spell is not
  available yet" for a spell `mm6::shouldRefusePlayerSpell` rejects.
- [MM6CombatRules.h](../src/Engine/MM6/MM6CombatRules.h) refuses a party attack that could reach an unverified damage
  channel before it touches the monster, the party or the RNG.
- An MM6 house service without a port shows "This service is not available yet"
  ([MM6House.cpp](../src/GUI/UI/Houses/MM6House.cpp)).
- `createSaveData` and `saveGame` in [SaveLoad.cpp](../src/Engine/SaveLoad.cpp) throw for MM6. MM6 saves take their
  own route.

## Rules from the original executable

MM6 reads its rule tables from the `MM6.exe` of the GOG release, version 1.2, in the game data folder.
`mm6::executable()` in [MM6Executable.h](../src/Engine/MM6/MM6Executable.h) reads and checks the file on first use.
Decoders take an `Executable &` and read by virtual address:

| Decoder | Caller | Tables |
|---|---|---|
| `mm6::loadCreationRules` | `Engine::MM7_Initialize` | Class creation arrays |
| `mm6::decodeSpellRules`, `mm6::loadSpellRules` | `Engine::MM7_Initialize` | Spell costs and recovery |
| `decodeShopStockRules` | [MM6ShopStock.cpp](../src/GUI/UI/Houses/MM6ShopStock.cpp) | Shop stock |
| `decodeTransportRules` | [MM6TransportRules.cpp](../src/GUI/UI/Houses/MM6TransportRules.cpp) | Coach and boat routes |
| `decodeRoomDescriptors` | [MM6HouseServiceData.cpp](../src/GUI/UI/MM6HouseServiceData.cpp) | House rooms |

There is no rules JSON and no `--rules-path` option. The addresses, layouts and evidence are on
[MM6 Executable Rules](mm6-executable-rules.md).

## GrayFace fix flags

A behaviour change that follows the GrayFace MM6 patch is a `gameplay.fix_*` entry in
[GameConfig.h](../src/Application/GameConfig.h), in the `Gameplay` section.

| Aspect | Convention |
|---|---|
| Key | `fix_<what>`, snake case, for example `fix_map_timers`. The member is `Fix<What>`, for example `FixMapTimers`. |
| Default | `true`. The game plays with the GrayFace behaviour unless the player turns it off. |
| Description | Says "MM6 only" when it is, what the fix does, "as the GrayFace patch does" when it follows the patch, and what vanilla did. |
| Gate | Read at the one decision point. In shared code the title check sits next to it: `gameTitle() == GameTitle::MM6 && ...FixMapTimers.value()`. In an MM6-only file the flag alone does. |
| Two rules | When both are real rules, the flag picks an enum for the MM6 helper, e.g. `MONTH_START_GRAYFACE` or `MONTH_START_VANILLA`. |

Current MM6 flags, by key:

| Key | Read in |
|---|---|
| `fix_stronger_shorter_buff` | [Spells.cpp](../src/Engine/Spells/Spells.cpp) |
| `fix_inactive_party_events`, `fix_door_state_switch` | [EvtInterpreter.cpp](../src/Engine/Evt/EvtInterpreter.cpp) |
| `fix_wasted_black_potion` | [Character.cpp](../src/Engine/Objects/Character.cpp) |
| `fix_shops_buying_blasters` | [MM6Shop.cpp](../src/GUI/UI/Houses/MM6Shop.cpp) |
| `fix_condition_spells` | [CastSpellInfo.cpp](../src/Engine/Spells/CastSpellInfo.cpp) |
| `fix_turn_based_turn_time` | [TurnEngine.cpp](../src/Engine/TurnEngine/TurnEngine.cpp) |
| `fix_recovery_enchant` | [Character.cpp](../src/Engine/Objects/Character.cpp) |
| `fix_new_day_at_month_start` | [Engine.cpp](../src/Engine/Engine.cpp) |
| `fix_sunrise_light` | [Outdoor.cpp](../src/Engine/Graphics/Outdoor.cpp) |
| `fix_buff_duration_text` | [GUIWindow.cpp](../src/GUI/GUIWindow.cpp) |
| `fix_stats_screen_condition` | [MM6CharacterScreen.cpp](../src/GUI/UI/MM6CharacterScreen.cpp) |
| `fix_map_timers` | [Processor.cpp](../src/Engine/Evt/Processor.cpp) |
| `fix_travel_autosave`, `fix_travel_speech_delay` | [MM6Transport.cpp](../src/GUI/UI/Houses/MM6Transport.cpp) |
| `fix_flying_border_offer` | [Outdoor.cpp](../src/Engine/Graphics/Outdoor.cpp) |
| `fix_chest_overflow` | [Chest.cpp](../src/Engine/Objects/Chest.cpp) |

`fix_water_walk_mana_drain` is an upstream flag that applies to every title.

In a game test, `test.prepareForNextTest()` resets every config entry to its default. A test that pins both behaviours
loops over the flag, sets it after the reset and before the game starts:

```cpp
for (bool fix : {true, false}) {
    test.prepareForNextTest();
    engine->config->gameplay.FixTurnBasedTurnTime.setValue(fix);
    startNewGameThroughInput(game);
    // ...
}
```

See `MM6TurnBased.RoundTakesFiftyGameSeconds` in
[GameTests_MM6FirstCombat.cpp](../test/Bin/GameTest/GameTests_MM6FirstCombat.cpp). Which GrayFace items are ported is
tracked on [GrayFace Patch Reference](mm6-grayface-patch-reference.md).

## Naming conventions

| Thing | Convention | Example |
|---|---|---|
| MM6-only source file | `MM6` prefix, in the folder of the subsystem it extends | `src/Engine/Evt/MM6SkillEvent.cpp` |
| Namespace | `mm6`. Some older entry points are global with `MM6` or `mm6` in the name. | `mm6::outdoorSky`, `decodeMM6ArchiveEntry`, `mm6SaveFromMenu` |
| GUI window | `GUIWindow_MM6<Name>`, subclassing the shared window | `GUIWindow_MM6House`, `GUIWindow_MM6GameOver` |
| Unit test file | `Tests/MM6<Name>_ut.cpp`, suite name starts with `MM6` | `src/Engine/MM6/Tests/MM6Executable_ut.cpp` |
| Game test file | `test/Bin/GameTest/GameTests_MM6<Area>.cpp`, suite name starts with `MM6` | `GameTests_MM6Houses.cpp` |
| Test seam | `*ForTest` name, `mm6::testing` namespace, often its own `*ForTest.h` | `MM6PreEventCheckpointForTest.h` |
| Fix flag | `gameplay.fix_<what>` | `fix_map_timers` |
| CMake | Many `src/Engine/MM6` pieces are static libraries (`engine_mm6_*`). Each unit test file is an `OBJECT` library linked into `OpenEnroth_UnitTest`. | `engine_mm6_executable` |

The game-test runner relies on the `MM6` suite prefix. An unfiltered MM6 run selects `MM6*.*`, an unfiltered MM7 run
selects `*-MM6*.*`, and an MM6 run with a filter that names a non-MM6 suite is refused
([GameTestMain.cpp](../test/Bin/GameTest/GameTestMain.cpp)).

### `ForTest` seams

Some MM6 paths need a test to observe or stop the engine at a point no input can reach, for example right after a load
rebuilds the map and before map events run. Those points are seams:

- The engine calls a `mm6::testing::*ForTest(GameTitle::MM6)` hook at a fixed place, mostly in `DoPrepareWorld`
  ([Engine.cpp](../src/Engine/Engine.cpp)) and `Game::gameLoop` ([Game.cpp](../src/Application/Game.cpp)).
- The hook does nothing unless a test armed it. Arming is usually a scoped object such as
  `ScopedPreEventCheckpointForTest`, which is one-shot, not nestable and title-locked.
- Test-side helpers with the same suffix live in `test/Bin/GameTest/*ForTest.h`.

## Headless driving

MM6 game tests run without a visible window. The rules are in
[GameTestOptions.cpp](../test/Bin/GameTest/GameTestOptions.cpp) and
[GameTestMain.cpp](../test/Bin/GameTest/GameTestMain.cpp):

- `--game mm6` requires `--headless` or `--offscreen-gl`, never both, and an explicit `--data-path`.
- Headless runs use `NullPlatform` and the null renderer. Synthetic input goes to the test controller only.
- `--offscreen-gl` is macOS-only and accepts exactly one of the `MM6OffscreenFrame.DISABLED_*` filters with
  `--gtest_also_run_disabled_tests`.
- Tests that write real saves are `DISABLED_` and need an env var to run. They write only into an owner-only
  `mm6-restart-*` directory directly under the system temp folder. The MM6 save code refuses any other test profile
  root ([MM6FirstMapManualSave.cpp](../src/Engine/MM6/MM6FirstMapManualSave.cpp),
  [MM6AtomicSaveFile.cpp](../src/Engine/MM6/MM6AtomicSaveFile.cpp)).
- The campaign chain takes `--mm6-chain-profile` and its exact `MM6CampaignChain.DISABLED_ChapterN` filter.

MM6 game tests start a party through the creation screen with input. Each file has a small static helper for it,
`startNewGameThroughInput` or `enterFirstMapFromNewGame`. No MM6 test calls `game.startNewGame()`.

Commands, env vars and the gate runners are on [Testing](testing.md).

## Tour of `src/Engine/MM6`

The folder is grouped by file name. Families, not files:

| Family | Files | Role |
|---|---|---|
| Executable | `MM6Executable` | Reads `MM6.exe`, see above. |
| Rules | `MM6CreationRules`, `MM6CombatRules`, `MM6ConsumableRules`, `MM6Potion`, `MM6Spell*`, `MM6PlayerSpell`, `MM6EventSpell` | Character, spell and item rules, and refusal guards. |
| Monsters | `MM6Monster*`, `MM6EncounterMonster`, `MM6NativeActor*` | Monster attacks, damage, special attacks, spells, projectiles, preferences and resistances. |
| Items | `MM6ItemGenerator`, `MM6*ItemProjection`, `MM6ItemRecord28`, `MM6StandardBonusMap`, `MM6SpellItems` | Item generation, and conversion between the 28-byte MM6 item record and `Item`. |
| Chests | `MM6Chest*` | Chest types, grid layout, traps, disarm, first-open item generation and the map-load handoff. |
| Shops | `MM6Shop*` | Shop stock identity, purchase, refresh boundary and title state. The house windows are in `GUI/UI/Houses`. |
| Party and time | `MM6PartyRest`, `MM6PartyDeath`, `MM6PartyHostility`, `MM6NewDay`, `MM6Calendar`, `MM6Finale` | Rest, death, hostility, the MM6 calendar and the ending. |
| Maps and travel | `MM6MapTransition`, `MM6Outdoor*`, `MM6Indoor*`, `MM6MapVisit`, `MM6Visited*`, `MM6HouseExit` | Border travel, sky, map visits, visited-map cache, house exits. |
| Map sources | `MM6Archive`, `MM6LoadedMapSource`, `MM6PrivateMapSource` | `games.lod` entry encoding and map pairs. Production uses `LoadedMapSource`, tests alone reach `PrivateMapSource`. |
| Presentation | `MM6Portraits`, `MM6StreetNpcPortraits`, `MM6VoiceSound`, `MM6RoomMovie`, `MM6RawCursor*` | Portrait, voice, room movie and cursor image plans. |
| Save container and fields | `MM6SaveContainer`, `MM6*SaveFields`, `MM6*SaveRuntime`, `MM6AtomicSaveFile`, `MM6SaveMetadata*` | The `.ec6` container, per-section codecs and the atomic slot writer. |
| Save sessions | `MM6SessionSave`, `MM6NewGameSession`, `MM6ScopedNewGameSave`, `MM6FirstMap*`, `MM6ParkedMapSection`, `MM6MapTimerSection` | Saving and loading a whole session. |
| Staged save versions | `MM6Raw*`, `MM6V3*` to `MM6V6*`, `MM6Pre*`, `MM6Post*`, `MM6Persistent*` | Decode, validate and publish stages per save version, and checks around the world rebuild. |
| Test seams | `*ForTest` | See above. |

The save versions and sections are on [MM6 Save Format](mm6-save-format.md).

## Source

- [MM6Executable.h](../src/Engine/MM6/MM6Executable.h), [src/Engine/MM6/CMakeLists.txt](../src/Engine/MM6/CMakeLists.txt)
- [GameConfig.h](../src/Application/GameConfig.h)
- [GameTitle.h](../src/Engine/GameTitle.h), [MapEnumFunctions.h](../src/Engine/MapEnumFunctions.h), [OpenEnrothOptions.cpp](../src/Bin/OpenEnroth/OpenEnrothOptions.cpp)
- [Engine.cpp](../src/Engine/Engine.cpp), [Game.cpp](../src/Application/Game.cpp), [SaveLoad.cpp](../src/Engine/SaveLoad.cpp)
- [MM6PreEventCheckpointForTest.h](../src/Engine/MM6/MM6PreEventCheckpointForTest.h)
- [GameTestOptions.cpp](../test/Bin/GameTest/GameTestOptions.cpp), [GameTestMain.cpp](../test/Bin/GameTest/GameTestMain.cpp)
- [GameTests_MM6FirstCombat.cpp](../test/Bin/GameTest/GameTests_MM6FirstCombat.cpp), [GameTests_MM6PlayerSpells.cpp](../test/Bin/GameTest/GameTests_MM6PlayerSpells.cpp)

See also: [Engine Architecture](engine-architecture.md), [MM6 Executable Rules](mm6-executable-rules.md), [MM6 Save Format](mm6-save-format.md), [Testing](testing.md)
