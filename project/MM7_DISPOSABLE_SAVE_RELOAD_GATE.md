# MM7 disposable fresh-process save and reload gate

Status (2026-11-07): **passed for the local-only GameTest binary** in two
separate headless processes. This does not accept the staged Atlas helper or
visible MM7 play/save/reload.

The opt-in `MM7DisposableSave` tests use an explicit, marked, owner-only
system-temp profile through `NativeFileSystem`. The test runner accepts
`--mm7-disposable-profile` only with MM7, `--headless`, an explicit licensed
`--data-path`, `--gtest_also_run_disabled_tests` and one exact writer or reader
filter. Before startup it canonicalizes the profile, checks its temp parent,
name, permissions, marker and allowed contents, and rejects symlinks. The
writer starts a new MM7 game, sets a known test-only Party value and calls the
normal MM7 disk save writer. After that process exits, the reader retains only
the disposable `saves` directory through the ordinary test reset, opens the
normal main-menu Load screen, selects the saved slot and checks the loaded map,
screen, file name and Party value. The original save file stayed byte-identical
across the reader process. Both tests use the null renderer and no native
window.

The writer and reader each passed **1/1** against the owner's private MM7
data using a GameTest linked against the tracked-source local-only Mac
dependencies. A wrong filter and an unexpected temporary-profile entry both
failed before engine startup. Two ordinary MM7 headless controls passed after
the test-harness change. The temporary profiles and their save files were
removed after verification. The protected installed MM7 app, existing saves
and staged Atlas bundle were not changed. The test binary was rebuilt in the
already marked, diagnostic-modified temporary source extraction; it is not a
new pristine-source proof.

This gate proves a bounded fresh-process disk save/reload for one first-map
fixture in the local-only test engine. It does not test the staged Atlas helper,
visible window and input, sound, an existing personal save, extended gameplay
or campaign completion. Those remain separate acceptance steps before an
installed-app cutover.

See [MM7 clean offline build](MM7_CLEAN_OFFLINE_BUILD.md), [hidden frame
capture](MM7_OFFSCREEN_FRAME_CAPTURE.md), and [runtime path
cutover](MM7_RUNTIME_PATH_CUTOVER.md).
