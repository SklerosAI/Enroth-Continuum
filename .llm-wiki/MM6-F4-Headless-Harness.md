---
type: synthesis
up: "[[MM6-Headless-Integration-Policy]]"
related: ["[[Agent-Handoff]]", "[[MM6-F4-Offline-Seams]]"]
tags: [mm6, f4, headless, tests]
---

# MM6 F4 Headless Harness

The [public packet](../project/MM6_F4_HEADLESS_HARNESS.md) records the title-selected, null-platform MM6 game-test smoke and its private-rule startup dependency.

`--game mm6` requires `--headless`, an explicit private data path and an explicit private `--rules-path`. The runner reads only eight named rule manifests from that path, checks all are regular files of 1 byte to 1 MiB, then stages them in the in-memory user filesystem through a `GameStarter` callback immediately after user-FS creation and before engine startup. The source directory and installed profile remain read-only. The runner defaults to MM7, rejects a rules path for MM7, rejects an explicit non-MM6 test filter, and skips wrong-title suites as a second boundary. The first MM6 suite checks null platform, event loop, window handle, renderer, in-memory rules and the still-closed live chest guard.

The next bounded suite checks the **new-game save guard** in the live null-platform engine. After party creation, `Game::loop()` selects `oute3.odm` and calls `saveNewGame()` before the map loop; the EC6 guard throws there. The test asserts no ramfs save directory is created. The map path also retains the MM6 chest-generation guard. These are explicit readiness gates, so no first-map play is claimed. See [public harness packet](../project/MM6_F4_HEADLESS_HARNESS.md) for the route and future acceptance points.

The first linked MM6 process stayed headless during 75 host samples (`focusChanged=false`, `maxOnScreenWindows=0`) but stopped before the smoke body at missing `ramfs://rules/creation.json`; the first seed ran too late in the constructor lifecycle. A second monitored attempt remained headless across 295 samples but failed at the same point. The constructor-time callback let a third linked attempt reach both bodies: the save-guard test passed, while direct null-leaf casts failed because the platform globals point to `PlatformApplication` proxies. That run showed no focus change or on-screen window across 97 samples. The bounded proxy-leaf assertions await linked rerun. The linked MM7 headless control passed 1/1. Headless startup logs also identify MM7 menu icon requests absent from MM6 data; this is a later graphical presentation blocker, and no private assets or detailed inventory are tracked. The MM6 result is not map, graphics, audio or campaign acceptance.

See also: [Headless Integration Policy](MM6-Headless-Integration-Policy.md), [F4 Offline Seams](MM6-F4-Offline-Seams.md), [Agent Handoff](Agent-Handoff.md).
