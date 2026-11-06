# MM6 F4a headless game-test harness

Status: **isolated candidate, source syntax checked; no linked MM6 game-test process yet** (2026-11-06). This implements the [headless integration policy](../.llm-wiki/MM6-Headless-Integration-Policy.md) without changing the normal MM7 game executable or removing any MM6 gameplay guard.

## Entry and isolation

`OpenEnroth_GameTest` now accepts `--game mm6`. That choice requires `--headless` and an explicit `--data-path` before `GameStarter` can run; `GameStarter` validates the selected title's data. The test runner continues to force in-memory user data and defaults to MM7. When no GoogleTest filter is supplied, it selects only `MM6*.*` suites for MM6 and excludes them for MM7. The `GameTest` fixture also skips a wrong-title suite if a caller supplies an explicit filter, preventing an MM7 trace/save test from running against MM6.

The first `MM6Headless.StartupAndChestGuard` game test asserts the selected engine title, `NullPlatform`, `NullEventLoop`, `NullWindow`, null native window handle, `NullRenderer` and `MemoryFileSystem`. It checks that the existing MM6 chest-generation guard still throws, then ticks one synthetic engine frame. The null event loop receives no physical OS messages; this check does not render a view, exercise a map load, or establish that focus stayed unchanged in a real run.

Example future command, using the owner's private MM6 data path without placing it in Git:

```sh
OpenEnroth_GameTest --game mm6 --headless --data-path "$MM6_PRIVATE_DATA" \
  --test-path test/Data --gtest_filter=MM6Headless.StartupAndChestGuard
```

Run the linked MM7 headless control and this MM6 smoke only after inspecting the data path and host focus/window state. The current MM6 guard prevents actual map play. The installed MM7 application and profile are not touched by this candidate.

## Verification and remaining gate

The four changed C++ translation units passed arm64 Clang `-fsyntax-only` with the integration checkout's generated compile commands. The isolated worktree did not perform a duplicate full CMake build, link or game run. The integrator must link `OpenEnroth_GameTest`, run CLI rejection cases, verify a known-good MM7 headless control, run the MM6 smoke with private data, and observe no native window/focus/input takeover. If startup fails before the smoke body, preserve the failure as a separate startup defect rather than loosening the headless checks.
