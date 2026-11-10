# MM7 Castle rebuild fresh-process checkpoint handoff

- Owner: Codex lane, branch `codex/mm7-castle-checkpoint` from integrated master `5bd23b424`.
- Code commit: `498f2737b`.
- Integration: pending parent review. No merge, push, app replacement, or graphical run was performed.

## Scope

`GameTestMain.cpp` admits only the two exact disabled Castle checkpoint filters through the existing owner-only, marked disposable MM7 profile gate. The reader also accepts a regular `autosave.mm7.bak`, which this multi-map route can create; unexpected entries and symlinks still fail before startup. Neither test can run through the ordinary profile path.

The writer reuses the input-driven positive Castle rebuild route, checks its assertions before saving, and writes `save000.mm7` through production `saveGame`. The separate reader selects that slot through the Load menu, verifies the rebuilt quest bit, rescue and Oracle completion, awards, consumed elixir and rescued-NPC flags, envoys, gold, and Stone City location. It compares the slot bytes before and after Load without printing or storing them.

## Focused results

- Isolated `OpenEnroth_GameTest` target built and linked.
- New writer and fresh-process reader passed 1/1 each; slot bytes were unchanged after Load.
- Prior `MM7DisposableSave` writer and reader passed 1/1 each.
- Wrong marker, unexpected save entry, save symlink, and invalid filter were each rejected before a test ran.
- Changed-file `cpplint.py` and `git diff --check` passed.

The original `MM7CastleRebuild.Rescue` test remains registered with both positive and one-missing cases. The positive path is exercised by the new writer. A separate run of its one-missing case was not part of this checkpoint gate.

## Reproduction boundary

Use a fresh owner-only `mm7-save-gate-*` directory directly under the system temp directory, containing only the exact marker `.ec-mm7-disposable-save-gate` with `EnrothContinuum-MM7-save-gate-v1`. Supply local MM7 data, `--game mm7 --headless --test-path`, `--mm7-disposable-profile`, and `--gtest_also_run_disabled_tests`. Run exact filters `MM7CastleRebuild.DISABLED_WriteFreshCheckpoint`, then `MM7CastleRebuild.DISABLED_ReloadFreshCheckpoint` in separate processes. Keep data, saves, profiles, logs, and checksums local and out of Git.
