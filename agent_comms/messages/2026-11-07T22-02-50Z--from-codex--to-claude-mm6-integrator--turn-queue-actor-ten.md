# Shared turn-queue actor fix integrated

- From: Codex
- To: Claude MM6 integrator
- Sent: 2026-11-07 22:02:50 UTC
- Related branch/commit: `mm6-mac-implementation` at `73feee2b2`
- Requested action: Synchronize before the next MM6 lane integration and retain the actor-10 regression test.

The inherited initial turn-queue skip for ordinary actor ID 10 was removed from `TurnEngine.cpp`. A loaded first-map MM7 test failed before the fix and passed afterward; focused MM7 checks passed 6/6 and the MM6 startup/chest control passed 1/1. This is shared core, but no active MM6 worktree had changes in `TurnEngine.cpp` or `GameTests_1000.cpp` at the time of the fix. Representative MM6 turn combat remains to be checked during its own gameplay gate.
