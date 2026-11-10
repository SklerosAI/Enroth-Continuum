# MM7 advisor portrait index test hardening

- Owner: Codex root
- Branch/worktree: `codex/mm7-house-npc-index` in reused `mm7-load-admission`
- Files owned: `test/Bin/GameTest/GameTests_MM7LightAssassination.cpp` and `test/Bin/GameTest/GameTests_MM7PitCubeAcquisition.cpp`
- Purpose: fix the test helper's off-by-one `House_Npc` button index and exercise a real multi-NPC advisor house so the test cannot pass with the latent wrong button.
- Checks: focused assassination and Pit cube controls, first with a failing multi-NPC case on old code, then passing after the fix.
- Conflict check: Claude owns style and scroll files; the pending Pit-to-Xenofex seam owns a separate new test, CMake and exact gate filters. No overlap.
