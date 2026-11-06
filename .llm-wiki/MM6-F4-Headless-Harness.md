---
type: synthesis
up: "[[MM6-Headless-Integration-Policy]]"
related: ["[[Agent-Handoff]]", "[[MM6-F4-Offline-Seams]]"]
tags: [mm6, f4, headless, tests]
---

# MM6 F4 Headless Harness

The [public packet](../project/MM6_F4_HEADLESS_HARNESS.md) records an isolated source candidate for a title-selected, null-platform MM6 game-test smoke.

`--game mm6` requires `--headless` and an explicit private data path. The runner keeps an in-memory user filesystem, defaults to MM7, and prevents wrong-title game-test suites from running even with a caller-supplied filter. The first MM6 suite checks null platform, event loop, window handle and renderer, then confirms the live chest guard remains closed.

Four C++ units passed focused arm64 syntax checks against the integration build's compile commands. A linked MM6 test process has **not** run, and the result is not a map, graphics, audio or campaign acceptance. The next gate is a linked no-window/no-focus process check with the owner's private data and a known-good MM7 headless control.

See also: [Headless Integration Policy](MM6-Headless-Integration-Policy.md), [F4 Offline Seams](MM6-F4-Offline-Seams.md), [Agent Handoff](Agent-Handoff.md).
