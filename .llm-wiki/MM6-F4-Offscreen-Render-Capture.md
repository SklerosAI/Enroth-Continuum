---
type: synthesis
up: "[[MM6-F4-UI-Resource-Blockers]]"
tags: [mm6, f4, render, macos, headless]
---

# MM6 F4 Offscreen Render Capture

The [public packet](../project/MM6_F4_OFFSCREEN_FRAME_CAPTURE.md) records the test-only hidden SDL OpenGL contract. The `MM6OffscreenFrame.DISABLED_FirstOutdoorCapture` GameTest is explicitly opt-in, isolated in a fresh process and restricted to a normalized temporary PNG path. `OffscreenCaptureGuard` blocks native show/raise, cursor and mouse changes, and modal boxes while leaving the SDL OpenGL context usable. The normal MM6 and MM7 launch paths and the null-renderer suite do not select this mode.

The first hidden draw reached a retained MM6 seven-character directional sprite prefix and aborted because inherited `Sprites.cpp` did not append its octant. Retained `dsft.bin` and sprite-archive aggregate evidence supports appending at seven for MM6: 981 eligible prefixes had numbered zero-octant entries and none had an exact unsuffixed entry. MM7's existing `<7` condition is retained. A focused MM6 headless test passed 1/1 with eight nonnull octants; accepted executable control flow for this lookup was not independently recovered.

The initial hidden OpenGL capture passed 1/1 but showed black party portrait ovals because the test read pixels before `OpenGLRenderer::flushAndScale` submitted queued `DrawQuad2D` portrait vertices. This supersedes the earlier claim that the black portraits were an unexplained runtime defect. The test now follows `Engine::Draw` order through HUD flush and overlay draw before readback. A four-face chromatic pixel assertion distinguishes the corrected result from the prior black ovals. The corrected capture passed 1/1 in a monitored arm64 process: 164 ten-millisecond samples, zero on-screen windows and no focus change. Its private 640×480 PNG was inspected locally and shows four colored portraits alongside first-map geometry and HUD. No screenshot or detailed proprietary asset inventory entered Git. White ground/water patches and intensely red foliage remain; missing water names appear in the log, but their relationship to those patches is not yet established. Do not infer visual correctness, normal new-game entry or gameplay from this diagnostic capture.

The custom linked binary also passed the MM7 `Inventory.Initialization` headless control 1/1 (45 samples, no window/focus change). A wrong-filter `--offscreen-gl` invocation was rejected before SDL startup. Parent integration must still build and rerun the focused capture from the committed source.
