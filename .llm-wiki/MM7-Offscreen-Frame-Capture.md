---
type: synthesis
up: "[[MM7-Clean-Offline-Build]]"
related: ["[[MM6-F4-Offscreen-Render-Capture]]", "[[MM7-Runtime-Path-Cutover]]"]
tags: [mm7, mac, graphics, verification]
---

# MM7 Offscreen Frame Capture

An explicit GameTest rendered one MM7 first-map frame through hidden macOS OpenGL without a visible window or focus change on 2026-11-07.

The [public capture packet](../project/MM7_OFFSCREEN_FRAME_CAPTURE.md) is the source for the test contract. `--game mm7 --offscreen-gl` requires an explicit data path, the exact disabled test filter and a normalized temporary PNG destination. The existing `OffscreenCaptureGuard` blocks reveal/raise, cursor and mouse changes, and native message boxes; audio uses a null driver. The test starts a game, checks the first outdoor map and game screen, draws the world and HUD, flushes queued 2D work, then reads the hidden frame before display swap. Its MM6 counterpart remains title-locked.

The local-only Mac GameTest build passed **1/1** with private MM7 data. The private 640×480 frame was inspected locally: world, minimap, HUD and four portraits rendered. A host monitor took **98** samples at 10 ms intervals, finding **zero on-screen test windows** and **zero frontmost-app changes**. A wrong-filter invocation rejected before SDL startup. No capture, proprietary asset or private log is tracked.

The MM7 health and mana bars flank the portrait art consistently. Their inherited render coordinates repeat at 115-pixel intervals. A slight visual offset remains a user observation, but the capture and source do not establish a better target position, so HUD geometry was left unchanged. A separate 1–2 pixel hover-hitbox discrepancy does not account for visible placement.

This diagnostic does not prove visible candidate-app launch, normal audio or input, save/quit/reload, installed-app cutover or a complete campaign. It rebuilt GameTest after copying test and startup files into the earlier temporary source extraction; that extraction was marked modified and is no longer pristine build evidence. The separately staged Atlas app was untouched. Continue using the protected installed MM7 app until candidate visible play and save/reload pass.

See also: [MM7 Clean Offline Build](MM7-Clean-Offline-Build.md), [MM7 Runtime Path Cutover](MM7-Runtime-Path-Cutover.md), [MM6 Offscreen Render Capture](MM6-F4-Offscreen-Render-Capture.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Agent Handoff](Agent-Handoff.md).
