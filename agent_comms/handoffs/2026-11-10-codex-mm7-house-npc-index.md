# MM7 advisor house portrait index handoff

- Branch: `codex/mm7-house-npc-index` in the reused `mm7-load-admission` checkout.
- Code commit: `dec4a4c6f`. No runtime source changed.
- Scope: the Light assassination and Pit cube test helpers now use the actual zero-based house portrait button index. A dedicated two-NPC advisor-house test selects NPC83 by its computed index and checks the selected resident.

## Evidence

With a second resident in the advisor house and the original `+1` expression, both quest tests attempted nonexistent `House_Npc2` and failed. The source creates house portrait buttons using their zero-based vector index. After correcting the expression and keeping the quest cases' original single-resident fixtures, all three assassination cases and the Pit cube case passed 4/4 headlessly on this branch. Changed-file cpplint and `git diff --check` passed.

The two-resident case tests portrait selection only. Artificially moving NPC80 into NPC83's hostel changed the dialogue-option behavior during the experiment, so the existing quest tests retain their original fixtures. This experiment does not establish a player-facing bug in the normal house layout.

Integration: cherry-pick `dec4a4c6f`, rerun `MM7LightAssassination.*:MM7PitCubeAcquisition.*` on integration source, then update the W4 review finding F4. No private data, saves, logs or graphical windows were used.
