# MM7 Overthruster fresh-process checkpoint handoff

- Owner: Codex subagent, branch `codex/mm7-overthruster-checkpoint` from master `6731dac3a`.
- Claim commit: `89f020ceb`. Code commit: `95814f299`.
- Integration: pending parent and Claude review. No merge, push, graphical run, normal-profile startup, wiki, PLAN or runtime edit.

## Scope

`GameTests_MM7OverthrusterRoute.cpp` now reuses the existing input-driven Lincoln path for both its normal case and an exact disabled writer. The route preserves the unpowered pedestal and one-unsuited exit negatives, powers the console by input, retrieves the Overthruster from the pedestal, then accepts the Shoals exit with all four suits. The writer checks all route assertions before calling production `saveGame` for `saves/save000.mm7`, verifies a nonempty save result and MM7 structural preflight, and requires the item in the party's held-item slot.

An exact disabled reader runs in a separate process. It selects the numbered slot through the normal Load menu and verifies Shoals location, Light alignment, active and completed retrieval bits, four equipped suits, the Overthruster still held rather than in inventory, and unchanged slot bytes. `GameTestMain.cpp` adds only the two exact filters to the existing fail-closed disposable-profile gate.

## Focused evidence

- `OpenEnroth_GameTest` built and linked.
- Existing `MM7OverthrusterRoute.LincolnRetrievalAndExit` passed 1/1 after refactor.
- In a fresh owner-only marked profile, production writer and fresh-process normal reader passed 1/1 each; the slot digest was unchanged after Load.
- A mismatched filter with the disposable profile and the exact writer filter without that profile were each rejected before startup.
- `MM7OverthrusterRoute.*:Issues.Issue402:Issues.Issue403_970` passed 6/6.
- Changed-file `cpplint.py` and `git diff --check` passed.
- Private data, saves, profiles, raw logs and checksums stayed outside Git.

## Reproduction and boundary

Use a fresh owner-only `mm7-save-gate-*` directory directly under system temp with only `.ec-mm7-disposable-save-gate` containing `EnrothContinuum-MM7-save-gate-v1`. Supply local MM7 data, `--game mm7 --headless --test-path test/Data --mm7-disposable-profile`, and `--gtest_also_run_disabled_tests`. Run exact filters `MM7OverthrusterRoute.DISABLED_WriteFreshCheckpoint` and `MM7OverthrusterRoute.DISABLED_ReloadFreshCheckpoint` in distinct processes. The reader accepts only the marked profile, MM7 ini and recognized save/backup names; symlinks and unexpected entries are rejected.

This is a checkpoint for the isolated Lincoln retrieval slice. Earlier Light chapter outcomes, natural L8-to-L9 continuity, L15 hand-in/ending, and an uninterrupted campaign are not proven by it. The slot is disposable test data, not an owner save.
