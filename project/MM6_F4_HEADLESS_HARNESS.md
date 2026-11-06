# MM6 F4a headless game-test harness

Status: **MM7 linked control passed; MM6 linked attempts stayed headless but stopped during startup because its in-memory profile lacked MM6 rules; constructor-time rule seeding awaits a linked run** (2026-11-06). This implements the [headless integration policy](../.llm-wiki/MM6-Headless-Integration-Policy.md) without changing the normal MM7 game executable or removing any MM6 gameplay guard.

## Entry and isolation

`OpenEnroth_GameTest` now accepts `--game mm6`. That choice requires `--headless`, an explicit `--data-path`, and an explicit `--rules-path` before `GameStarter` can run; `GameStarter` validates the selected title's data. The test runner continues to force in-memory user data and defaults to MM7. MM7 rejects `--rules-path`. When no GoogleTest filter is supplied, it selects only `MM6*.*` suites for MM6 and excludes them for MM7. An explicit MM6 run rejects positive GoogleTest filter patterns outside `MM6*`; the `GameTest` fixture also skips a wrong-title suite as a second boundary. This prevents an MM7 trace/save test from running against MM6 or a wrong-title-only filter from reporting a false pass.

The runner reads exactly eight private MM6 manifests from `--rules-path`: `creation.json`, `spells.json`, `first-aid.json`, `power-cure.json`, `timed-cures.json`, `combat-buffs.json`, `house-rooms.json`, and `transport.json`. Each must be a regular file between 1 byte and 1 MiB. It checks the complete set before copying them to `ramfs://rules/` through a narrow `GameStarter` callback immediately after user-FS creation and before engine initialization; it does not write to the source directory or the installed profile. The first six are required at engine startup; the last two support later house and transport scenarios. The test reset preserves this in-memory rules directory while clearing saves and configuration. The rule-seeding tests cover a missing manifest, an oversize manifest, successful exact-set staging, and the required CLI option.

The first `MM6Headless.StartupAndChestGuard` game test asserts the selected engine title, `NullPlatform`, `NullEventLoop`, `NullWindow`, null native window handle, `NullRenderer`, `MemoryFileSystem`, and staged rule presence. It checks that the existing MM6 chest-generation guard still throws, then ticks one synthetic engine frame. The null event loop receives no physical OS messages; this check does not render a view or exercise a map load.

## First-map route and current stop

The normal new-game route leaves the main-menu FSM for party creation. After the party is accepted, `Game::loop()` chooses `oute3.odm` as the MM6 pending transition, then calls `saveNewGame()` **before** entering `gameLoop()`. `saveNewGame()` deliberately throws for MM6 until the EC6 new-game transaction is complete. If that gate were removed today, the first map's level preparation would still reach the guarded `mm6::generateChestItemsForTitle(GameTitle::MM6)` call. Neither guard is opened by this harness.

`MM6Headless.NewGameSaveGuard` invokes the same save entry point in the instrumented null-platform engine and asserts that it throws without creating a save directory in ramfs. This is a bounded expected-blocker scenario, not a completed UI new-game or first-map run. The next live scenario can use `EngineController::startNewGame()` only after the EC6 save transaction and chest generation have passed their own focused gates; it should then assert the first map ID, party state, save round-trip and no native input/window activity.

Example command, using the owner's private MM6 data and rules paths without placing either in Git:

```sh
OpenEnroth_GameTest --game mm6 --headless --data-path "$MM6_PRIVATE_DATA" \
  --rules-path "$MM6_PRIVATE_RULES" \
  --test-path test/Data --gtest_filter=MM6Headless.StartupAndChestGuard
```

The current MM6 guard prevents actual map play. The installed MM7 application and profile are not touched by this candidate.

## Verification and remaining gate

The first linked run's host monitor observed 75 samples with `focusChanged=false` and `maxOnScreenWindows=0`; engine startup then failed at `ramfs://rules/creation.json` before the smoke body. A second monitored attempt with `--rules-path` stayed headless across 295 samples but failed at the same file because the seed ran **after** the `GameStarter` constructor, whose engine initialization had already read it. The callback moves that seed before engine initialization. A known-good MM7 headless control passed 1/1 in the integration checkout. The isolated worktree did not perform a duplicate full CMake build, link or MM6 process run. The integrator must link this callback follow-up, run CLI rejection cases and both MM6 guards with the explicit private rules path, and observe no native window/focus/input takeover. A linked pass would establish only the bounded startup and guard checks, not map or campaign play.
