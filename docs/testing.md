# Testing

This page covers how tests are built, run and filtered for MM6 and MM7: unit and game tests, tapes, data-path env vars,
hidden frame captures, the gate runners in `tools/` and playtest recording.

## Rules live elsewhere

How to write a test is not repeated here:

- [HACKING.md](../HACKING.md#testing) covers the upstream test framework, traces and retracing.
- [.claude/CLAUDE.md](../.claude/CLAUDE.md) covers the rules for game tests: act through input, tick minimally, observe
  through tapes, name the test after its issue, and revert the fix to watch the test fail.

## Targets

All targets are defined under [test/Bin](../test/Bin/CMakeLists.txt). `OE_TESTDATA_PATH` is `test/Data` and is passed
as `--test-path`.

| Target | Runs |
|---|---|
| `Run_UnitTest` | `OpenEnroth_UnitTest`, every unit test in one process. |
| `Run_GameTest_Headless_Parallel` | `OpenEnroth_GameTest --headless` under `thirdparty/gtest_parallel`, one process per test. |
| `Run_GameTest`, `Run_GameTest_Headless`, `Run_GameTest_Parallel` | The same binary visible or in one process. |
| `Run_RetraceTest`, `Run_RetraceTest_Headless`, `Run_RetraceTest_Parallel`, `Run_RetraceTest_Headless_Parallel` | `OpenEnroth_RetraceTest` over the traces in `test/Data`. |
| `Run_AllTests_Headless_Parallel` | Unit tests, headless parallel game tests and headless parallel retrace tests. |

The game-test targets pass no `--game`, so they run MM7 only. With no filter, an MM7 run selects `*-MM6*.*` and an MM6
run selects `MM6*.*` ([GameTestMain.cpp](../test/Bin/GameTest/GameTestMain.cpp)). MM6 game tests are run by hand or by
the gate runners below.

MM7 game tests find their data through `ENROTH_CONTINUUM_MM7_PATH` or `OPENENROTH_MM7_PATH`, or `--data-path`.

## Unit tests

Unit tests are `*_ut.cpp` files in a `Tests/` folder next to the code. In `src/Engine/MM6` each test file is its own
`OBJECT` library linked into `OpenEnroth_UnitTest`. MM6 suite names start with `MM6`. To run only them:

```sh
build/test/Bin/UnitTest/OpenEnroth_UnitTest --gtest_filter='MM6*'
```

Most MM6 unit tests use synthetic bytes built in the test. Some run the production readers over the real install
through [MM6TestData](../test/Testing/Unit/MM6TestData.h), which opens the folder named by `OPENENROTH_MM6_PATH` and
exposes `games.lod`, `icons.lod`, decompressed table entries, map names and the list of active event scripts. Without
the variable these tests skip with "Set OPENENROTH_MM6_PATH to the MM6 data folder". CI has no MM6 data, so they skip
there. Tests that read `MM6.exe` (for example `MM6Executable.*`) use the same helper.

## Game tests

Game tests live in `test/Bin/GameTest/GameTests_*.cpp`. MM7 tests follow the upstream number-ranged files
(`GameTests_0000.cpp` and so on). MM6 tests are in `GameTests_MM6<Area>.cpp`, with suites named `MM6<Something>`. The
test framework is in [test/Testing/Game](../test/Testing/Game/GameTest.h): `GAME_TEST`, `TestController`,
`EngineController` and the tape recorders.

### Options

[GameTestOptions.cpp](../test/Bin/GameTest/GameTestOptions.cpp) defines them. Run `OpenEnroth_GameTest --help` for the
full list.

| Option | Meaning |
|---|---|
| `--test-path PATH` | Test data folder, normally `test/Data`. Required. |
| `--game mm6\|mm7` | Title. Defaults to `mm7`. |
| `--data-path PATH` | Game data. Required for MM6 and for MM7 `--offscreen-gl`. |
| `--headless` | Null platform and null renderer. |
| `--offscreen-gl` | Hidden OpenGL window for a capture test. macOS only. |
| `--mm7-disposable-profile PATH` | Real MM7 profile for the MM7 save gates and campaign chain. Needs `--headless` and `--data-path`. |
| `--mm6-chain-profile PATH` | Real MM6 profile for one MM6 campaign chain chapter. Needs `--game mm6`, `--headless` and `--data-path`. |
| `--speed`, `--tracing-rng`, `--log-level`, `-v` | Playback speed, tracing RNG, logging. |

User data lives in a RAM filesystem. `--mm7-disposable-profile` replaces it with the given folder. MM6 tests that
write real saves send them through the MM6 save code to a native temp profile instead.

### MM6 rules

- `--game mm6` needs `--headless` or `--offscreen-gl`, never both, and an explicit `--data-path`. Env vars are not read
  for MM6 game-test data.
- An MM6 filter must name only `MM6*` suites. Anything else is refused.
- `game.startNewGame()` is not used by MM6 tests. Each MM6 test file has a static helper that makes a party on the
  creation screen with input, `startNewGameThroughInput` or `enterFirstMapFromNewGame`.
- The MM6 save tests and the campaign chain write real `.ec6` files. The MM6 save code accepts a test profile root only
  if it is an owner-only `mm6-restart-*` directory directly under the system temp folder. These tests are `DISABLED_`
  and also check an `ENROTH_MM6_*_ENABLE` variable for an exact value, which the gate runner supplies.

Run all regular MM6 game tests in one process:

```sh
build/test/Bin/GameTest/OpenEnroth_GameTest --game mm6 --headless \
    --data-path "$MM6_DATA_PATH" --test-path test/Data
```

Or one process per test, the same way `Run_GameTest_Headless_Parallel` runs MM7:

```sh
python3 thirdparty/gtest_parallel/gtest-parallel build/test/Bin/GameTest/OpenEnroth_GameTest -- \
    --game mm6 --headless --data-path "$MM6_DATA_PATH" --test-path test/Data
```

Add `--gtest_filter=MM6Houses.*` (the `=` is required) to narrow the run.

### Tapes

A tape records one value every frame while taping is on. `GAME_TEST` hands the body three recorders: `tapes`
([CommonTapeRecorder](../test/Testing/Game/CommonTapeRecorder.h)), `charTapes`
([CharacterTapeRecorder](../test/Testing/Game/CharacterTapeRecorder.h)) and `actorTapes`
([ActorTapeRecorder](../test/Testing/Game/ActorTapeRecorder.h)). `tapes.custom(lambda)` tapes anything else. Nothing is
recorded until `test.startTaping()`. Compare whole tapes with `EXPECT_EQ`, `EXPECT_CONTAINS` or `delta()` as
[.claude/CLAUDE.md](../.claude/CLAUDE.md) describes.

`test.prepareForNextTest()` resets every config entry to its default, clears the RAM profile, goes back to the main
menu, zeroes the game clock and restarts the random number generators. A test that sets a config value, such as a
`gameplay.fix_*` flag, does so after the reset. A new game keeps the game clock, and AI and walking animations run off
it, so without the reset a test would depend on the tests that ran before it in the same process. The UI animation
clock isn't reset.

### Fixtures and test data

`test/Data` holds MM7 traces (`.json`) and saves (`.mm7`) only. MM6 has no trace support. The `MM6` app refuses the
`play` and `retrace` subcommands. MM6 game tests build their state in code from a new party. The campaign chain chapters
name their fixtures, such as raised stats or teleports, in a comment at the top of the test.

Two private inputs remain:

- `ENROTH_MM6_MAP_RESEARCH_ROOT` points three `MM6Save` game tests at a private research folder read by
  `mm6::PrivateMapSource`. Without it they skip.
- `tools/mm6_map_name_manifest.py` builds a private map-name manifest under `local-game-data/` for the same tests.
  Its own test is `tools/tests/test_mm6_map_name_manifest.py`.

## Hidden frame captures

A capture test renders real frames into a hidden window and writes PNGs, without taking focus or the mouse.
`--offscreen-gl` sets this up in [GameStarter.cpp](../src/Application/Startup/GameStarter.cpp):

- OpenGL renderer, 640x480 windowed, no mouse grab, not resizable, null audio.
- `SDL_MAC_BACKGROUND_APP=1`, so the process does not become the frontmost app.
- `OffscreenCaptureGuard` refuses showing or raising the window, cursor and mouse changes and native message boxes.

The runner admits `--offscreen-gl` only with `--gtest_also_run_disabled_tests` and an exact filter for one capture test:

| Title | Filters |
|---|---|
| MM6 | `MM6OffscreenFrame.DISABLED_` + one of the names below |
| MM7 | `MM7OffscreenFrame.DISABLED_` + `FirstOutdoorCapture`, `MapCapture` |

The MM6 names are `FirstOutdoorCapture`, `MapCapture`, `PartyCreationCapture`, `CharacterScreenCapture`,
`MissileCapture`, `ScreensCapture`, `NpcDialogueCapture`, `SpellbookCapture`, `SpellbookSchoolsCapture`,
`HousesCapture`, `QuestionCapture`, `TurnBasedCapture` and `MenusCapture`. The tests are spread over the area files
(`GameTests_MM6Houses.cpp`, `GameTests_MM6Menus.cpp` and so on).
The output path comes from an env var and must be a normalized `.png` under `/tmp/` or `/private/tmp/`
(`isOffscreenCapturePath` in [OffscreenCaptureForTest.h](../test/Bin/GameTest/OffscreenCaptureForTest.h)). Some MM6
tests write several frames and take a `/private/tmp/` prefix instead.

| Variable | Used by |
|---|---|
| `MM6_OFFSCREEN_CAPTURE_PATH` | Every MM6 capture test. Output PNG or prefix. |
| `MM6_OFFSCREEN_CAPTURE_MAP` | `MapCapture`. A map file name, or an index into the map table that wraps around. |
| `MM6_OFFSCREEN_BUTTONS_PATH` | `PartyCreationCapture`. Optional text dump of the screen's button rectangles and messages. |
| `MM6_OFFSCREEN_<PART>_CAPTURE_PATH`, `<PART>` one of `CHEST`, `CHARACTER`, `ARMOR`, `WEAPON`, `WEARABLE`, `CLOAK` | `FirstOutdoorCapture`. Optional extra frames. |
| `MM6_OFFSCREEN_SELECTION_ANIMATION_CHECK` | `FirstOutdoorCapture`. Set to `1` to also check the selected-portrait animation. |
| `MM7_OFFSCREEN_CAPTURE_PATH` | Both MM7 capture tests. |
| `MM7_OFFSCREEN_CAPTURE_MAP` | MM7 `MapCapture`. Map to capture from its first start point. |
| `MM7_OFFSCREEN_CAPTURE_SCENE` | MM7 `FirstOutdoorCapture`. Optional scene such as `breeding-zone`. |

Example:

```sh
MM6_OFFSCREEN_CAPTURE_PATH=/tmp/mm6-houses.png \
build/test/Bin/GameTest/OpenEnroth_GameTest --game mm6 --offscreen-gl --data-path "$MM6_DATA_PATH" \
    --test-path test/Data --gtest_also_run_disabled_tests --gtest_filter=MM6OffscreenFrame.DISABLED_HousesCapture
```

Captures contain proprietary game frames. Keep them out of Git, under `/tmp/` or the ignored `local-game-data/`.

## Gate runners

The runners in `tools/` drive `build/test/Bin/GameTest/OpenEnroth_GameTest` in fresh headless processes. None opens a
visible window or touches an installed profile. Each prints its options with `--help`.

### `run_mm6_firstmap_gate.py`

The MM6 save and reload gate. Each child is a separate `--game mm6 --headless` process that must report exactly one
passing test. A skip counts as a failure. Groups run in their own owner-only `mm6-restart-*` temp profile, removed
afterwards:

- the seven-child first-map chain, then the fault-retry and paperdoll children
- the normal Save and Load menu children, including a production load and resave
- a new game saved, loaded and saved again
- save anywhere after travel, buffs across a fresh process, autosave and quick save, map visits
- the All Artifacts ledger across a fresh process
- three MM7 control tests

```sh
MM6_DATA_PATH=... MM7_DATA_PATH=... python3 tools/run_mm6_firstmap_gate.py
python3 tools/run_mm6_firstmap_gate.py --list         # print the children, no build, no data
python3 tools/run_mm6_firstmap_gate.py --skip-build   # use the existing binaries
```

Without `--skip-build` it builds `OpenEnroth_GameTest` in `build/` with `~/enroth-tools/venv/bin/cmake` if present,
otherwise `cmake` from `PATH`, and refuses a CMake under `/opt/homebrew/`. `ENROTH_GATE_CHILD_TIMEOUT` raises the
per-child timeout (default 240 s, save and load children get at least 1500 s). `--stage-f5a-profile` keeps a
first-map save slot in a temp profile and prints the `MM6` app command to load it for a visible check.

### `run_mm6_map_sweep.py`

Runs `MM6MapSweep.DISABLED_EnterTickAndFireEvents` once per MM6 map, a process each. A map that does not finish runs
again without actors, then without the timer wait, then past each event that ended the run. Records, logs and a
`summary.md` go to `local-game-data/mm6/sweep-<date>` by default. The runner refuses an output folder Git does not
ignore. The summary ranks the first throw of each map by the main-quest maps it blocks, read from a private
`--main-quest` list.

```sh
MM6_DATA_PATH=... python3 tools/run_mm6_map_sweep.py --jobs 2 [--maps oute3 15] [--main-quest FILE]
```

It drives the test through `ENROTH_MM6_MAP_SWEEP_MAP` (`list` writes the map list), `_RESULT`, `_MAX_FRAMES`,
`_NO_ACTORS`, `_SKIP_TIMERS` and `_SKIP_EVENTS`.

### `run_mm6_combat_sweep.py`

Runs `MM6MapSweep.DISABLED_MonstersAttackParty` per map. Each kind of monster on the map gets the party to itself for
`--frames` frames. A throw ends the process, and the map runs again without that monster. Output goes to
`local-game-data/mm6/combat-sweep-<date>`. `--main-quest` is required and its maps are the default set. The test reads
`ENROTH_MM6_COMBAT_SWEEP_MAP`, `_RESULT`, `_FRAMES` and `_SKIP_MONSTERS`.

### `run_mm7_campaign_chain.py`

Runs the campaign chapters `<Suite>.DISABLED_ChapterN` in order, one fresh headless process each, keeping each
chapter's save in a marked owner-only temp profile. A chapter passes when its process exits cleanly, it wrote a
regular save, and earlier saves are unchanged. The runner then writes a `.passed` marker holding the save's SHA-256.
A rerun resumes after the last passed chapter.

| `--title` | Suite | Chapters | Profile option |
|---|---|---|---|
| `mm7` (default) | `MM7CampaignChain` | 8 | `--mm7-disposable-profile` |
| `mm7-dark` | `MM7DarkCampaignChain`, sharing the first 3 chapters of `MM7CampaignChain` | 8 | `--mm7-disposable-profile` |
| `mm7-war-dark` | `MM7DarkCampaignChain`, with `MM7CampaignChain` C0 to C2 and `MM7WarCampaignChain` C3 | 8 | `--mm7-disposable-profile` |
| `mm6` | `MM6CampaignChain` | 10 | `--mm6-chain-profile` |

```sh
python3 tools/run_mm7_campaign_chain.py --title mm6 --through 3            # data from MM6_DATA_PATH
python3 tools/run_mm7_campaign_chain.py --title mm7 --data-path ... --through 7
python3 tools/run_mm7_campaign_chain.py --title mm6 --profile /tmp/mm6-restart-chain-XXXX --through 5
python3 tools/run_mm7_campaign_chain.py --title mm6 --profile /tmp/mm6-restart-chain-XXXX --remove-profile
```

The per-chapter timeout is three hours. Status of the chains is on [MM6 Campaign Status](mm6-campaign-status.md) and
[MM7 Status](mm7-status.md).

### `run_mm7_pretag_checkpoints.py`

Runs every registered MM7 checkpoint writer and reader pair, one at a time, each in a fresh marked temporary profile,
and stops at the first failure. It is a local gate before a new release tag, never part of ordinary builds, default
game tests or CI. It never creates or moves a tag.

```sh
python3 tools/run_mm7_pretag_checkpoints.py --list
python3 tools/run_mm7_pretag_checkpoints.py --candidate-tag ECvX.Y.Z \
    --binary build/test/Bin/GameTest/OpenEnroth_GameTest --data-path "$OPENENROTH_MM7_PATH" --test-path test/Data
```

`run_mm7_priest_checkpoints.py` and `run_mm7_lich_checkpoint.py` run the promotion pairs on their own.
`probe_mm7_owned_save.py` loads a copy of one owner save as primary and as backup in fresh processes, see
[MM7 Status](mm7-status.md).

### `run_mm7_gameplay_harness.py`

Runs the `MM7GameplayHarness` and `MM7CombatHarness` suites, and with `--with-barrows` `MM7BarrowTraversal`, each in a
fresh headless MM7 process. It fails if the binary lacks a required suite.

```sh
python3 tools/run_mm7_gameplay_harness.py --data-path ...
python3 tools/run_mm7_gameplay_harness.py --list
```

The harness helpers (`driveMovementInput`, `fightActorWithAttackInput`) are in
[MM7GameplayHarness.h](../test/Bin/GameTest/MM7GameplayHarness.h) and
[MM7CombatHarness.h](../test/Bin/GameTest/MM7CombatHarness.h).

## Environment variables

| Variable | Read by | Meaning |
|---|---|---|
| `OPENENROTH_MM6_PATH` | Engine path resolver, `MM6TestData` | MM6 data folder. |
| `ENROTH_CONTINUUM_MM7_PATH`, `OPENENROTH_MM7_PATH` | Engine path resolver | MM7 data folder. The first is macOS only and wins when both are set. |
| `MM6_DATA_PATH`, `MM7_DATA_PATH` | `tools/run_*.py` only | Data folders the runners pass as `--data-path`. |
| `ENROTH_GATE_CHILD_TIMEOUT` | `run_mm6_firstmap_gate.py` | Per-child timeout in seconds. |
| `ENROTH_MM6_RESTART_PROFILE` | MM6 save game tests | The `mm6-restart-*` profile the gate created. |
| `ENROTH_MM6_*_ENABLE` | Individual `DISABLED_` MM6 tests | Admits one gate child, with the exact value the runner sets. |
| `ENROTH_MM6_MAP_SWEEP_*`, `ENROTH_MM6_COMBAT_SWEEP_*` | `MM6MapSweep` tests | Set by the sweep runners. |
| `ENROTH_MM6_MAP_RESEARCH_ROOT` | `MM6Save` game tests | Private research folder, see above. |
| `MM6_OFFSCREEN_*`, `MM7_OFFSCREEN_*` | Capture tests | See [Hidden frame captures](#hidden-frame-captures). |
| `ENROTH_MM7_MOD_DIR`, `ENROTH_MM7_ALL_ARTIFACTS_STORAGE`, `ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY` | Engine, MM7 mod tests | See [Mods and Atlas](mods-and-atlas.md). |
| `ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY` | Engine, MM6 vendor refresh tests | See [Mods and Atlas](mods-and-atlas.md). The tests set it themselves. |
| `EC_MM7_MAX_SAVE_SNAPSHOTS`, `EC_MM7_MAX_SESSIONS` | `mm7_playtest_capture.sh` | Recorder caps when the option is absent. |

`git grep -n "getenv\|OPENENROTH_\|ENROTH_" src test tools` lists every one.

## Playtest recording

[tools/mm7_playtest_capture.sh](../tools/mm7_playtest_capture.sh) passively records an owner-played MM7 session, either
the fork's `MM7` helper or the upstream OpenEnroth helper. It never starts or configures the game. Atlas starts it after
Play. It can also run by hand with the subcommands `start`, `note`, `clip`, `status` and `stop`.

A session keeps the build identity, a timeline, byte-range copies of the game logs, changed saves (newest 3 copies of
each), CPU and memory samples, crash reports after an abnormal exit and, with `--video`, a rolling screen recording that
keeps only the last two minutes. `clip`, or an abnormal exit, saves the rolling window into a kept clip. The newest 20
sessions are kept. The recorder refuses a destination tracked by Git or inside the MM7 profile, and stops if the
destination volume is missing or replaced.

The full option list, file layout and caps are in [tools/MM7_PLAYTEST_CAPTURE.md](../tools/MM7_PLAYTEST_CAPTURE.md).
`python3 tools/tests/test_mm7_playtest_capture.py` tests the recorder against stubs without recording the screen.

## Source

- [test/Bin/CMakeLists.txt](../test/Bin/CMakeLists.txt), [test/Bin/GameTest/CMakeLists.txt](../test/Bin/GameTest/CMakeLists.txt)
- [test/Bin/UnitTest/CMakeLists.txt](../test/Bin/UnitTest/CMakeLists.txt)
- [GameTestOptions.cpp](../test/Bin/GameTest/GameTestOptions.cpp), [GameTestMain.cpp](../test/Bin/GameTest/GameTestMain.cpp)
- [OffscreenCaptureForTest.h](../test/Bin/GameTest/OffscreenCaptureForTest.h)
- [GameTest.h](../test/Testing/Game/GameTest.h), [TestController.cpp](../test/Testing/Game/TestController.cpp), [TestTape.h](../test/Testing/Game/TestTape.h)
- [MM6TestData.h](../test/Testing/Unit/MM6TestData.h)
- [GameStarter.cpp](../src/Application/Startup/GameStarter.cpp), [PathResolver.h](../src/Application/Startup/PathResolver.h)
- [run_mm6_firstmap_gate.py](../tools/run_mm6_firstmap_gate.py), [run_mm6_map_sweep.py](../tools/run_mm6_map_sweep.py), [run_mm6_combat_sweep.py](../tools/run_mm6_combat_sweep.py)
- [run_mm7_campaign_chain.py](../tools/run_mm7_campaign_chain.py), [run_mm7_gameplay_harness.py](../tools/run_mm7_gameplay_harness.py)
- [mm7_playtest_capture.sh](../tools/mm7_playtest_capture.sh), [MM7_PLAYTEST_CAPTURE.md](../tools/MM7_PLAYTEST_CAPTURE.md)

See also: [Build and Dev Setup](build-and-dev-setup.md), [MM6 Port Architecture](mm6-port-architecture.md), [MM6 Campaign Status](mm6-campaign-status.md)
