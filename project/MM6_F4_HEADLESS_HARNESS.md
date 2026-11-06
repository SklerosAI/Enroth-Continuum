# MM6 F4a headless game-test harness

Status: **MM6 startup and scoped first-world headless checks pass; the full-world probe requires a dedicated process because its global state does not roll back** (2026-11-06). This implements the [headless integration policy](../.llm-wiki/MM6-Headless-Integration-Policy.md) without changing the normal MM7 game executable or removing any MM6 gameplay guard.

## Entry and isolation

`OpenEnroth_GameTest` now accepts `--game mm6`. That choice requires `--headless`, an explicit `--data-path`, and an explicit `--rules-path` before `GameStarter` can run; `GameStarter` validates the selected title's data. The test runner continues to force in-memory user data and defaults to MM7. MM7 rejects `--rules-path`. When no GoogleTest filter is supplied, it selects only `MM6*.*` suites for MM6 and excludes them for MM7. An explicit MM6 run rejects positive GoogleTest filter patterns outside `MM6*`; the `GameTest` fixture also skips a wrong-title suite as a second boundary. This prevents an MM7 trace/save test from running against MM6 or a wrong-title-only filter from reporting a false pass.

The runner reads exactly eight private MM6 manifests from `--rules-path`: `creation.json`, `spells.json`, `first-aid.json`, `power-cure.json`, `timed-cures.json`, `combat-buffs.json`, `house-rooms.json`, and `transport.json`. Each must be a regular file between 1 byte and 1 MiB. It checks the complete set before copying them to `ramfs://rules/` through a narrow `GameStarter` callback immediately after user-FS creation and before engine initialization; it does not write to the source directory or the installed profile. The first six are required at engine startup; the last two support later house and transport scenarios. The test reset preserves this in-memory rules directory while clearing saves and configuration. The rule-seeding tests cover a missing manifest, an oversize manifest, successful exact-set staging, and the required CLI option.

The first `MM6Headless.StartupAndChestGuard` game test asserts the selected engine title, `NullPlatform`, `NullEventLoop`, `NullWindow`, null native window handle, `NullRenderer`, `MemoryFileSystem`, and staged rule presence. `PlatformApplication` exposes proxies through the globals, so the test follows each proxy chain to its bounded leaf before checking the null platform, event loop and window. It checks that the existing MM6 chest-generation guard still throws, then ticks one synthetic engine frame. The null event loop receives no physical OS messages; this check does not render a view or exercise a map load.

## First-map route and current stop

The normal new-game route leaves the main-menu FSM for party creation. After the party is accepted, `Game::loop()` chooses `oute3.odm` as the MM6 pending transition, then calls `saveNewGame()` **before** entering `gameLoop()`. `saveNewGame()` deliberately throws for MM6 until the EC6 new-game transaction is complete. If that gate were removed today, the first map's level preparation would still reach the guarded `mm6::generateChestItemsForTitle(GameTitle::MM6)` call. Neither guard is opened by this harness.

`MM6Headless.NewGameSaveGuard` invokes the same save entry point in the instrumented null-platform engine and asserts that it throws without creating a save directory in ramfs. This is a bounded expected-blocker scenario, not a completed UI new-game or first-map run. The next live scenario can use `EngineController::startNewGame()` only after the EC6 save transaction and chest generation have passed their own focused gates; it should then assert the first map ID, party state, save round-trip and no native input/window activity.

A separate [first-map resource preflight](MM6_F4_FIRST_MAP_PREFLIGHT.md) reads the original map and related resource entries without entering the world. It binds the MM6 full-screen loading background and the first map's explicit sky name, while recording the live delta and chest gates.

Example command, using the owner's private MM6 data and rules paths without placing either in Git:

```sh
OpenEnroth_GameTest --game mm6 --headless --data-path "$MM6_PRIVATE_DATA" \
  --rules-path "$MM6_PRIVATE_RULES" \
  --test-path test/Data --gtest_filter=MM6Headless.StartupAndChestGuard
```

The current MM6 save guard prevents the normal New Game route from entering play. A separate scoped test can prepare the first outdoor world in memory; it does not touch the installed MM7 application or profile.

Headless startup also logs missing menu art because the current menu requests MM7-specific icon names that the private MM6 data does not supply. This is a presentation blocker for a future graphical menu, separate from the save/chest gameplay gates. The private assets and their inventories remain outside Git.

## Verification and remaining gate

The MM7 linked headless control passed. MM6's title selector, private-rule seed and null-platform checks have passed in the linked arm64 runner. The [first-map packet](MM6_F4_FIRST_MAP_PREFLIGHT.md) records a monitored first outdoor preparation pass and a later scoped full-world pass with actors and chests. A combined MM6 suite then exposed process-global state retained by the full-world probe: subsequent save and chest tests failed. That probe is disabled by default and must run alone in a fresh process:

```sh
OpenEnroth_GameTest --game mm6 --headless --data-path "$MM6_PRIVATE_DATA" \
  --rules-path "$MM6_PRIVATE_RULES" --test-path test/Data \
  --gtest_also_run_disabled_tests \
  --gtest_filter=MM6Headless.DISABLED_ScopedFirstOutdoorWorld
```

Do not combine that filter with other tests. The default MM6 suite skips the disabled probe. The linked arm64 standalone probe passed 1/1, and the normal MM6 suite passed 38/38 in a separate process with the private map source available for its save test. Host monitors observed 44 and 1,899 samples respectively, with no focus change, no on-screen process window and unchanged frontmost PID. Neither mode proves durable save/reload or graphical play.
