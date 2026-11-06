# MM6 F4 offscreen first-map frame capture

Status: **monitored hidden OpenGL capture passes; visual correctness and normal new-game entry remain open** (2026-11-06). This is a test-only macOS diagnostic. It does not enable the guarded MM6 new-game or chest path, change MM7 startup, or place a proprietary screenshot in Git.

## Capture contract

`GameTestOptions` accepts `--offscreen-gl` only for MM6 and only with `--gtest_also_run_disabled_tests --gtest_filter=MM6OffscreenFrame.DISABLED_FirstOutdoorCapture`. `GameStarter` uses the standard SDL platform and OpenGL renderer, installs `OffscreenCaptureGuard` before configuring the window, forces windowed mode and no mouse grab, and routes audio to the null driver. SDL creates its OpenGL window hidden. The guard refuses show/raise, cursor-visibility changes, mouse grabs/relative mode/warps and modal message boxes. The PNG destination must be a normalized absolute `.png` path under `/tmp` or `/private/tmp`.

The isolated test enters party creation from the menu, stages private game deltas and a deterministic MM6 chest owner in memory, then calls `GameUI_LoadPlayerPortraitsAndVoices` before scoped `DoPrepareWorld`, matching `Game::gameLoop`'s setup order. It draws the first outdoor world and HUD into the hidden OpenGL context, reads pixels before swap, requires a 640×480 nonblank image with nonblack world and HUD regions, and writes the PNG only to the temporary path. Existing null-renderer tests retain their setup and platform; this process-global world test is disabled by default and must run alone.

A private macOS monitor sampled the process every 10 ms, comparing the frontmost application PID and counting its on-screen windows through `CGWindowListCopyWindowInfo`. The final arm64 run passed **1/1**, with **390 samples**, **zero on-screen windows** and **no focus change**. The first hidden-GL attempt also kept focus and windows unchanged but aborted while loading a directional actor sprite. The diagnostic PNG was inspected locally and remains ignored.

The same custom linked binary passed the MM7 `Inventory.Initialization` headless control **1/1** with no on-screen window or focus change (45 samples). A negative MM6 `--offscreen-gl` invocation with a different test filter was rejected before SDL startup. Parent integration should rerun the focused cases after cherry-picking; the standalone link reused the current green arm64 libraries and replaced only the changed objects.

## First render blocker and source-backed correction

The first world draw aborted at `SpriteFrameTable::InitializeSprite` when the MM6 `pfemsta` frame requested an unsuffixed sprite that the retained archive does not contain. The retained MM6 `dsft.bin` frame points to a seven-character directional texture prefix; the retained sprite archive contains its numbered octants. An aggregate check of all **981** eligible seven-character directional prefixes in that table found a numbered zero-octant entry for every prefix and no unsuffixed exact entry. The inherited `Sprites.cpp` path appended an octant only when the prefix length was **less than** seven. It now appends at seven for MM6 only; MM7 retains its original condition. A focused MM6 headless sprite test passed **1/1** with all eight octant pointers nonnull and no window/focus change. This correction is based on the retained original table/archive relationship; the accepted executable's sprite-loading branch was not independently reconstructed here.

## What the captured pixels show

The capture has recognizable first-map sky, buildings, outdoor geometry, automap and HUD. It also visibly has **black party portrait ovals**, **white ground or water patches**, and **intensely red foliage**. The portrait-load call did run, so its omission is not the explanation for the black ovals. This fixture bypasses normal party finalization, but that causal link is unproved. The log reports missing inherited water-texture names; their relationship to the white patches needs a title-specific resource audit. The foliage cause is unknown. These symptoms prevent a visually correct or playable claim even though the offscreen capture mechanism works.

Next: determine portrait owner state after real party finalization, reconcile MM6 water/terrain and foliage rendering against original assets and a private original-frame reference, then repeat the hidden capture. A foreground frame and input acceptance remain separate gates.
