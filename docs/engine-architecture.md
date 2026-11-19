# Engine architecture and runtime map

This page maps the engine from process start to save file: binaries, startup, the game loop, title selection, asset
loading, rendering, saves and events, with links to where each piece lives in `src/`.

It is a map, not a tutorial. MM6-specific code is covered in [MM6 Port Architecture](mm6-port-architecture.md).

## Code organization

`src/` follows the upstream OpenEnroth layout. The layer rules (what `Utility`, `Library` and `Core` may depend on, one
`CMakeLists.txt` per folder, one class per file, full include paths from `src`) are in
[HACKING.md](../HACKING.md#code-organization). They are not repeated here.

| Folder | Holds |
|---|---|
| `src/Utility` | Domain-free helpers: strings, streams, memory, filesystem paths. |
| `src/Library` | Independent libraries: `Lod`, `LodFormats`, `Snd`, `Vid`, `Platform` (SDL plus `Null`), `FileSystem`, `Config`, `Cli`, `Fsm` and more. |
| `src/Core` | OpenEnroth-specific building blocks such as time and event traces. |
| `src/Engine` | Game state and rules: party, characters, actors, items, spells, events, tables, maps, rendering, saves. |
| `src/Engine/MM6` | The MM6 port: original-executable rules, the EC6 save, MM6 world and runtime state. |
| `src/Engine/MM7` | MM7 save validation and atomic save writing. |
| `src/Engine/Mods` | The private MM7 mod hooks, see [Mods and Atlas](mods-and-atlas.md). |
| `src/GUI` | Windows, buttons, HUD resources, and every screen under `GUI/UI`, including houses under `GUI/UI/Houses`. |
| `src/Media` | Audio (`Media/Audio`, OpenAL) and movie playback (`MediaPlayer`, FFmpeg). |
| `src/Application` | `Game` (the main loop and message processing), `GameConfig`, the startup FSM states and `Startup/`. |
| `src/Io`, `src/Scripting`, `src/Arcomage` | Input mapping, the Lua scripting system, the Arcomage minigame. |
| `src/Bin` | Executables: `OpenEnroth` (and its macOS `MM6`/`MM7` siblings), `LodTool`, `CodeGen`. |

Code with an original-executable counterpart often carries a `//----- (0x...)` offset marker, which stays put as
`HACKING.md` describes. Function names such as `MM6_Initialize` and `MM7_Initialize` in
[Engine.cpp](../src/Engine/Engine.cpp) come from those markers. They are MM7 startup steps and have nothing to do with
title selection.

## Binaries and targets

| Target | Source | Purpose |
|---|---|---|
| `OpenEnroth` | [src/Bin/OpenEnroth](../src/Bin/OpenEnroth/CMakeLists.txt) | Engine binary. `--game mm6` or `mm7` (default). `play` and `retrace` replay traces. |
| `MM7` (macOS) | same sources, `EC_MM7_ONLY=1` | Title-locked MM7 app bundle that Atlas packages. Accepts only `--game mm7`. |
| `MM6` (macOS) | same sources, `EC_MM6_ONLY=1` | Title-locked MM6 app bundle. Accepts only `--game mm6` and refuses `play` and `retrace`, since traces still read MM7 saves. |
| `OpenEnroth_UnitTest` | [test/Bin/UnitTest](../test/Bin/UnitTest/CMakeLists.txt) | Google Test binary linking every `*_ut.cpp`. Run with `Run_UnitTest`. |
| `OpenEnroth_GameTest` | [test/Bin/GameTest](../test/Bin/GameTest/CMakeLists.txt) | Instrumented engine plus game tests. Run with `Run_GameTest_Headless_Parallel`. |
| `OpenEnroth_RetraceTest` | [test/Bin/RetraceTest](../test/Bin/RetraceTest/CMakeLists.txt) | Retraces the trace files in `test/Data`. |
| `LodTool`, `CodeGen` | [src/Bin](../src/Bin/CMakeLists.txt) | Developer tools for LOD archives and generated code. Built only with `OE_BUILD_TOOLS`. |
| Atlas | [launcher/](../launcher/README.md) | Swift macOS launcher. Not a CMake target, built by `launcher/build.sh` and `launcher/assemble.py`. |

`EC_MM6_ONLY` and `EC_MM7_ONLY` are mutually exclusive, enforced by an `#error` in
[OpenEnrothOptions.cpp](../src/Bin/OpenEnroth/OpenEnrothOptions.cpp). The test targets and their options are covered
in [Testing](testing.md). Build steps are in [Build and Dev Setup](build-and-dev-setup.md).

There is no MM8 engine. `--game` accepts only `mm6` and `mm7`. `PathResolver` has an MM8 resolver, but nothing calls it.

## Startup

`main` lives in [OpenEnroth.cpp](../src/Bin/OpenEnroth/OpenEnroth.cpp). It parses `OpenEnrothOptions` and hands them to
[GameStarter](../src/Application/Startup/GameStarter.cpp), which builds the process in this order:

1. For MM6, resets the raw cursor shop owners and sets `gameStartingYear` to the MM6 calendar start.
2. Resolves the user path and creates the user filesystem `ufs`. A RAM filesystem replaces it for traces and tests
   (`GameStarterOptions::ramFsUserData`).
3. Loads the config. The file name depends on the title, see [Build and Dev Setup](build-and-dev-setup.md). MM6 gets
   its own keybinding defaults (`setMM6KeybindingDefaults`).
4. Starts the logger, then resolves and validates the data path and creates the data filesystem `dfs`.
5. Creates the platform. Headless runs use `NullPlatform`. Headless and offscreen runs set `ALSOFT_DRIVERS=null`.
6. Installs the application components: engine control (test thread), trace recorders and players, the deterministic
   and random components, keyboard, window handler and, for an offscreen capture run, `OffscreenCaptureGuard`.
7. Creates the renderer (`RENDERER_NULL` when headless), the overlay system, input mapping and mouse.
8. Constructs `Engine` with the selected `GameTitle` and calls `Engine::Initialize`, then creates `Game` and the Lua
   scripting system.

[GameStarterOptions.h](../src/Application/Startup/GameStarterOptions.h) lists the switches: title, data and user
paths, log level, RAM user data, headless, offscreen capture, tracing RNG and quick start.

## Title selection

`GameTitle` is a two-value enum, `MM6` and `MM7`, in [GameTitle.h](../src/Engine/GameTitle.h). It is chosen once on the
command line (or fixed at compile time for the `MM6` and `MM7` apps) and stored in the engine. Code reads it as
`engine->gameTitle()`.

The title seams are spread through shared code rather than isolated behind an interface. The densest are, by count of
title checks:

| Area | Main files |
|---|---|
| Events | [EvtInterpreter.cpp](../src/Engine/Evt/EvtInterpreter.cpp), [EvtInstruction.cpp](../src/Engine/Evt/EvtInstruction.cpp), [Processor.cpp](../src/Engine/Evt/Processor.cpp) |
| Engine init, world preparation, timed effects | [Engine.cpp](../src/Engine/Engine.cpp) |
| GUI and HUD | [GUIWindow.cpp](../src/GUI/GUIWindow.cpp), [UIGame.cpp](../src/GUI/UI/UIGame.cpp), `GUI/Hud*Resources.h` |
| Rules | [Character.cpp](../src/Engine/Objects/Character.cpp), [Actor.cpp](../src/Engine/Objects/Actor.cpp), [CastSpellInfo.cpp](../src/Engine/Spells/CastSpellInfo.cpp), `Party.cpp` |
| Main loop and messages | [Game.cpp](../src/Application/Game.cpp) |
| Outdoor and indoor maps | [Outdoor.cpp](../src/Engine/Graphics/Outdoor.cpp), [Indoor.cpp](../src/Engine/Graphics/Indoor.cpp) |
| Startup and paths | [GameStarter.cpp](../src/Application/Startup/GameStarter.cpp), [PathResolver.cpp](../src/Application/Startup/PathResolver.cpp) |
| Table loaders | [TableSerialization.cpp](../src/Engine/Snapshots/TableSerialization.cpp), `Engine/Tables/*` |
| Saves | [SaveLoad.cpp](../src/Engine/SaveLoad.cpp), [UISaveLoad.cpp](../src/GUI/UI/UISaveLoad.cpp) |

MM6 map IDs reuse the `MapId` type but not the MM7 enum values. Title-aware helpers such as
`isMapOutdoor(GameTitle, MapId)` in [MapEnumFunctions.h](../src/Engine/MapEnumFunctions.h) exist for that reason, and
MM6 code looks maps up by file name through the map table.

## Data paths and filesystems

`dfs` merges two layers ([FileSystemStarter.cpp](../src/Application/Startup/FileSystemStarter.cpp)): the game data
folder seen through a lowercase-name filesystem, and the resources embedded from `resources/` at build time. Embedded
resources win unless `debug.override_built_in_resources` is set.

The data folder is resolved per title by [PathResolver.cpp](../src/Application/Startup/PathResolver.cpp): the
`--data-path` option first. Then an env override, which becomes the only candidate when set: `OPENENROTH_MM6_PATH`
for MM6, and for MM7 `ENROTH_CONTINUUM_MM7_PATH` (macOS only) before `OPENENROTH_MM7_PATH`. Otherwise the candidates
are the current folder, platform install locations and the macOS support folder, which MM7 on macOS checks first. A
folder is accepted only if it holds the title's required files. The exact macOS order is on
[Build and Dev Setup](build-and-dev-setup.md#runtime-paths-on-macos).

| Title | Required files |
|---|---|
| MM7 | `anims/magic7.vid`, `anims/might7.vid`, `data/bitmaps.lod`, `data/events.lod`, `data/games.lod`, `data/icons.lod`, `data/sprites.lod`, `sounds/audio.snd` |
| MM6 | `anims/anims1.vid`, `anims/anims2.vid`, `data/bitmaps.lod`, `data/games.lod`, `data/icons.lod`, `data/sprites.lod`, `sounds/audio.snd` |

MM6 also needs `MM6.exe` in the data folder. It is not part of the validation list. `mm6::executable()` reads it on
first use and throws a clear error if it is missing.

User paths, config names and log names per title are on [Build and Dev Setup](build-and-dev-setup.md).

## Asset loading

[ResourceManager](../src/Engine/Resources/ResourceManager.cpp) owns the LOD readers and is opened with the title:

| Reader | MM7 file | MM6 file |
|---|---|---|
| Events (tables, `.evt` scripts, `.bin` frame tables) | `data/events.lod` | `data/icons.lod`. MM6 keeps tables and scripts beside its images. |
| Icons | `data/icons.lod` | `data/icons.lod` |
| Bitmaps | `data/bitmaps.lod` | `data/bitmaps.lod` |
| Sprites | `data/sprites.lod` | `data/sprites.lod` |
| Maps | `data/games.lod` (`pGames_LOD`, [LOD.cpp](../src/Engine/Resources/LOD.cpp)) | `data/games.lod`, MM6 archive encoding |

MM6 icons whose palette entry zero is the cyan MM6 background colour get that colour masked. A few MM6 top-panel
images are blitted with index zero transparent although their headers don't say so.

Tables load in two steps in [Engine.cpp](../src/Engine/Engine.cpp):

- `Engine::MM7_Initialize` deserializes the binary frame and object tables (`dsft.bin`, `dtft.bin`, `dtile.bin`,
  `dpft.bin`, `dift.bin`, `ddeclist.bin`, `dobjlist.bin`, `dmonlist.bin`, `doverlay.bin`, `dsounds.bin`). Several take
  the title. For MM6 it first loads creation and spell rules from `MM6.exe`.
- `Engine::SecondaryInitialization` parses the text tables (`MapStats.txt`, `monsters.txt`, `spells.txt`, items, NPCs,
  `2dEvents.txt`, quests, awards, transitions, scrolls) and `global.evt`, then opens `games.lod`. MM6 branches call
  `InitializeMm6` loaders or skip tables MM6 doesn't have (`placemon.txt`, `history.txt`, `hostile.txt`).

Map files are read in [Outdoor.cpp](../src/Engine/Graphics/Outdoor.cpp) and
[Indoor.cpp](../src/Engine/Graphics/Indoor.cpp). For MM6 they go through `decodeMM6ArchiveEntry`
([MM6Archive.h](../src/Engine/MM6/MM6Archive.h)) and the MM6 snapshot decoders in `src/Engine/Snapshots`.

Sound comes from `sounds/audio.snd` through `Library/Snd`. Movies come from the anims VID archives through `Library/Vid`
and [MediaPlayer](../src/Media/MediaPlayer.cpp). The MM6 movie name lookup is on
[MM6 Media and UI](mm6-media-and-ui.md).

## Main loop

[Game::run](../src/Application/Game.cpp) drives a state machine built by
[GameFsmBuilder](../src/Application/GameStates/GameFsmBuilder.cpp): `Start`, the logo and intro videos, `LoadStep2`,
`MainMenu`, `LoadSlot` and `Credits`. Each frame clears the target, updates the FSM and the GUI windows, flushes, draws
overlays and swaps.

When the FSM exits for a new game or a load, `Game::loop` takes over:

- New game runs `PartyCreationUI_Loop`, sets the first map (`oute3.odm` for MM6, Emerald Island for MM7), calls
  `saveNewGame` and enters `Game::gameLoop`.
- Load calls `loadGame` and enters `Game::gameLoop`.

`Game::gameLoop` prepares the world (`PrepareWorld` and `DoPrepareWorld` in `Engine.cpp`), then runs the frame loop:
input, `processQueuedMessages` for GUI messages, world update, timed effects
(`_494035_timed_effects__water_walking_damage__etc`), event timers and drawing through `Engine::Draw`.

Game tests run on a separate control thread that the `EngineControlComponent` steps one frame at a time, see
[Testing](testing.md).

## Rendering

[RendererFactory](../src/Engine/Graphics/Renderer/RendererFactory.cpp) makes an `OpenGLRenderer` (OpenGL or OpenGL ES,
per `graphics.renderer`) or a `NullRenderer` for headless runs. `Engine::Draw` draws the world (`drawWorld`, the BSP and
outdoor terrain renderers in `Engine/Graphics`), then the HUD (`drawHUD`, `DrawGUI`), then overlays.

An offscreen capture run is a game-test mode, not a renderer. It keeps the OpenGL renderer but hides the SDL window,
forces 640x480 windowed mode, and installs `OffscreenCaptureGuard`, which refuses window reveal, cursor changes, mouse
grabs and native message boxes. It requires MM6 or MM7 and is refused together with headless. See
[Testing](testing.md#hidden-frame-captures).

## Saves

Entry points are in [SaveLoad.h](../src/Engine/SaveLoad.h): `loadGame`, `saveGame`, `autoSave`, `doSavegame`,
`saveNewGame`, `quickSaveGame` and `quickLoadGame`.

- MM7 saves are `.mm7` files. Loads are preflighted by `mm7::preflightNativeSaveStructure`. When a native profile root
  is set, `writeMm7Save` writes the slot through `mm7::writeAtomicSaveSlot`
  ([MM7AtomicSaveFile.h](../src/Engine/MM7/MM7AtomicSaveFile.h)), otherwise it writes to `ufs` directly.
- MM6 branches out at the top of each entry point. `loadGame` calls `mm6::loadFirstMapManualSlot`
  ([MM6FirstMapManualSave.h](../src/Engine/MM6/MM6FirstMapManualSave.h)). `doSavegame` and `autoSave` call
  `mm6SaveFromMenu` and `mm6AutoSave` in [MM6SaveLoadMenu.cpp](../src/GUI/UI/MM6SaveLoadMenu.cpp). `saveNewGame`
  starts a session through `mm6::startNewGameSession`. `createSaveData` and `saveGame` throw for MM6. MM6 saves use the
  `.ec6` extension. The container is on [MM6 Save Format](mm6-save-format.md).

## Event interpreter

Event scripts (`.evt`) are parsed by `EvtProgram::load` ([EvtProgram.cpp](../src/Engine/Evt/EvtProgram.cpp)), which
takes the title because MM6 instructions are framed differently. For example, the variable operand is one byte in MM6
and two in MM7 ([EvtInstruction.cpp](../src/Engine/Evt/EvtInstruction.cpp)).

[EvtInterpreter](../src/Engine/Evt/EvtInterpreter.cpp) runs one event's instructions and returns an `EvtFlow` per
step. [Processor.cpp](../src/Engine/Evt/Processor.cpp) is the outer layer: `runEvent`, map load and leave hooks,
timers, decoration events and topic checks. [EvtVariables.cpp](../src/Engine/Evt/EvtVariables.cpp) reads and writes
the MM7 variables. MM6 variables dispatch through `MM6EventVariableDispatch` and the other `Evt/MM6*` files. Opcodes
and variables are on [MM6 Events](mm6-events.md).

## Source

- [OpenEnroth.cpp](../src/Bin/OpenEnroth/OpenEnroth.cpp), [OpenEnrothOptions.cpp](../src/Bin/OpenEnroth/OpenEnrothOptions.cpp)
- [src/Bin/OpenEnroth/CMakeLists.txt](../src/Bin/OpenEnroth/CMakeLists.txt)
- [GameStarter.cpp](../src/Application/Startup/GameStarter.cpp), [GameStarterOptions.h](../src/Application/Startup/GameStarterOptions.h)
- [PathResolver.cpp](../src/Application/Startup/PathResolver.cpp), [FileSystemStarter.cpp](../src/Application/Startup/FileSystemStarter.cpp)
- [Game.cpp](../src/Application/Game.cpp), [GameFsmBuilder.cpp](../src/Application/GameStates/GameFsmBuilder.cpp)
- [Engine.cpp](../src/Engine/Engine.cpp), [Engine.h](../src/Engine/Engine.h), [GameTitle.h](../src/Engine/GameTitle.h), [MapEnumFunctions.h](../src/Engine/MapEnumFunctions.h)
- [ResourceManager.cpp](../src/Engine/Resources/ResourceManager.cpp), [LOD.cpp](../src/Engine/Resources/LOD.cpp), [MM6Archive.h](../src/Engine/MM6/MM6Archive.h)
- [RendererFactory.cpp](../src/Engine/Graphics/Renderer/RendererFactory.cpp), [NullRenderer.cpp](../src/Engine/Graphics/Renderer/NullRenderer.cpp)
- [SaveLoad.cpp](../src/Engine/SaveLoad.cpp), [MM6SaveLoadMenu.cpp](../src/GUI/UI/MM6SaveLoadMenu.cpp)
- [EvtProgram.cpp](../src/Engine/Evt/EvtProgram.cpp), [EvtInterpreter.cpp](../src/Engine/Evt/EvtInterpreter.cpp), [Processor.cpp](../src/Engine/Evt/Processor.cpp)
- [HACKING.md](../HACKING.md)

See also: [MM6 Port Architecture](mm6-port-architecture.md), [Testing](testing.md), [Build and Dev Setup](build-and-dev-setup.md), [MM6 Save Format](mm6-save-format.md)
