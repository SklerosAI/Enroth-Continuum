# MM6 F4a headless game-test harness

Status: **MM7 linked control passed; first MM6 linked smoke stayed headless but stopped during startup because its in-memory profile lacked MM6 rules; explicit rule seeding is syntax checked and awaits a linked run** (2026-11-06). This implements the [headless integration policy](../.llm-wiki/MM6-Headless-Integration-Policy.md) without changing the normal MM7 game executable or removing any MM6 gameplay guard.

## Entry and isolation

`OpenEnroth_GameTest` now accepts `--game mm6`. That choice requires `--headless`, an explicit `--data-path`, and an explicit `--rules-path` before `GameStarter` can run; `GameStarter` validates the selected title's data. The test runner continues to force in-memory user data and defaults to MM7. MM7 rejects `--rules-path`. When no GoogleTest filter is supplied, it selects only `MM6*.*` suites for MM6 and excludes them for MM7. An explicit MM6 run rejects positive GoogleTest filter patterns outside `MM6*`; the `GameTest` fixture also skips a wrong-title suite as a second boundary. This prevents an MM7 trace/save test from running against MM6 or a wrong-title-only filter from reporting a false pass.

The runner reads exactly eight private MM6 manifests from `--rules-path`: `creation.json`, `spells.json`, `first-aid.json`, `power-cure.json`, `timed-cures.json`, `combat-buffs.json`, `house-rooms.json`, and `transport.json`. Each must be a regular file between 1 byte and 1 MiB. It checks the complete set before copying them to `ramfs://rules/`; it does not write to the source directory or the installed profile. The first six are required at engine startup; the last two support later house and transport scenarios. The test reset preserves this in-memory rules directory while clearing saves and configuration. The rule-seeding tests cover a missing manifest, an oversize manifest, successful exact-set staging, and the required CLI option.

The first `MM6Headless.StartupAndChestGuard` game test asserts the selected engine title, `NullPlatform`, `NullEventLoop`, `NullWindow`, null native window handle, `NullRenderer`, `MemoryFileSystem`, and staged rule presence. It checks that the existing MM6 chest-generation guard still throws, then ticks one synthetic engine frame. The null event loop receives no physical OS messages; this check does not render a view or exercise a map load.

Example command, using the owner's private MM6 data and rules paths without placing either in Git:

```sh
OpenEnroth_GameTest --game mm6 --headless --data-path "$MM6_PRIVATE_DATA" \
  --rules-path "$MM6_PRIVATE_RULES" \
  --test-path test/Data --gtest_filter=MM6Headless.StartupAndChestGuard
```

The current MM6 guard prevents actual map play. The installed MM7 application and profile are not touched by this candidate.

## Verification and remaining gate

Five changed C++ translation units passed arm64 Clang `-fsyntax-only` with the integration checkout's generated compile commands; seven changed/new source and header files passed cpplint. The isolated worktree did not perform a duplicate full CMake build or link. In the first linked run, a host monitor observed 75 samples with `focusChanged=false` and `maxOnScreenWindows=0`; engine startup then failed at `ramfs://rules/creation.json` before the smoke body. A known-good MM7 headless control passed 1/1 in the integration checkout. The integrator must link this rule-seeding follow-up, run CLI rejection cases, run the MM6 smoke with the explicit private rules path, and observe no native window/focus/input takeover. A linked smoke pass would establish only the bounded startup and guard check, not map or campaign play.
