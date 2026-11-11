# MM6 award-animation crash handoff

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm6-award-anim` in the `mm7-save-durability` checkout, commit `dbb0b95e7` from `master` `a5b09b294`. Not pushed.
- Files: `src/Engine/SpellFxRenderer.cpp`, `src/Engine/Tables/IconFrameTable.cpp` (shared core), `test/Bin/GameTest/GameTests_MM6EventVariables.cpp`.

## Defect and evidence

An MM6 award or stat event called `SetPlayerBuffAnim`, which looks up an icon animation by name. MM6's icon table has six animations and none of the 22 names the engine requests, while MM7 and MM8 have all of them. The lookup returned -1 and `IconFrameTable` read `_frames[-1]`, a hard abort on 17 maps including the first. The original MM6 executable instead adds overlay 20 at the character's portrait (a sparkle sprite) and plays the quest sound.

## Fix

MM6 skips the icon buff animation, keeping the sound and the speech reaction. `IconFrameTable::animationLength` and `animationFrame` assert the id is in range. `animationId` still returns -1 for a missing name, because callers rely on it.

## Checks

- `MM6EventVariables.FirstMapStatBonusAward`: New Game through creation onto the first map, select a character by digit key, click the real event face, and check the exact might bonus, the quest sound and that the screen stays in game. Revert-and-fail aborts on the new assert or on the original out-of-bounds read.
- Sweep rerun on the 18 affected maps: 17 load and all 17 finish with no crash, 15 with no throw. Two now reach the known damage-channel and event-spell blockers. The NPC pool overflow is unchanged.
- MM6 suite 94 passed. MM6 first-map gate passed with the MM7 controls. MM7 in-process 107 passed. MM7 headless parallel 540/540. Unit tests 1,087 passed. `check_style` clean.

## Follow-ups

- MM6 portrait overlay rendering (the original's award sparkle). The engine's active overlay list is neither ticked nor drawn, and it is in the MM6 save.
- Latent: MM6's HUD looks up torch light, wizard eye, fly and water walk icons that MM6 lacks, and would hit the new assert once those buffs are drawn.

Privacy: addresses, IDs and map stems only. No game text or data in Git.
