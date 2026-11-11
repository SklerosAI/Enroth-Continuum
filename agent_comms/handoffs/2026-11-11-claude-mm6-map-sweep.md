# MM6 map sweep handoff

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm6-map-sweep` in the `mm7-save-durability` checkout, commit `fe66c3ca3` from `master` `f0db53a79`. Test-only, no `src/`.
- Files: `test/Bin/GameTest/GameTests_MM6MapSweep.cpp` (`MM6MapSweep.DISABLED_EnterTickAndFireEvents`, driven by `ENROTH_MM6_MAP_SWEEP_*` environment variables), `test/Bin/GameTest/MM6MapEntranceForTest.h` (entrance lookup shared with the world load probe, whose `d07` output is unchanged), `tools/run_mm6_map_sweep.py` (one process per map, three at a time, with reruns without actors or timers and past fatal events). Results go to a Git-ignored folder.
- Results summary: `PLAN.md` stage 2, "Map sweep".
- Checks: `check_style` clean, unit tests 1,087 passed, MM7 headless parallel 540/540, MM6 default suite 93 passed. The full sweep takes about 10-16 minutes.
- Harness findings: a probe must clear the first map's chest positions before loading an indoor map with chests, and the NPC pool overflow happens straight from a creation party, not as first-map leftovers.

Privacy: map stems and counts only. No game text, results or data in Git.
