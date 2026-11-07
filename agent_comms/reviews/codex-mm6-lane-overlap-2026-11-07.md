# Read-only MM6 lane overlap check — 2026-11-07

Owner: Codex. Scope: the seven active MM6 worktrees listed in
`project/SESSION_HANDOFF_2026-11-07.md`. This review did not edit any lane.

All seven worktrees contain uncommitted work. `git diff --check` reported no
whitespace errors in their tracked diffs at this snapshot. It does not inspect
untracked files or prove compilation and gameplay behavior.

Integration hotspots from the current changed-file lists:

| File | Lanes with pending edits |
| --- | --- |
| `src/Application/Game.cpp` | normal-save UI, house services, spells/combat |
| `test/Bin/GameTest/CMakeLists.txt` | normal-save UI, house services, spells/combat |
| `src/Engine/MM6/CMakeLists.txt` | first-map attempt, normal-save UI, inventory, transition owners |
| `src/Engine/Evt/EvtInterpreter.cpp` | event variables, spells/combat |
| `test/Bin/GameTest/GameTests_MM6V5AfterActionFreshChildReplay.cpp` | first-map attempt, inventory |

The MM7 save-file module added only `src/Engine/MM7/` and the parent
`src/Engine/CMakeLists.txt`, outside these pending MM6 edits. The MM7 artifact
fix touches `Item.cpp`, the artifact spawn branch of `Indoor.cpp`, and
`GameTests_0500.cpp`, also outside the listed pending MM6 edits. The shared
turn-queue fix in `TurnEngine.cpp` remains a separate MM6 regression risk and
has a passed MM6 startup control, not a representative combat acceptance.

Review each incoming MM6 patch against the then-current integration tree;
the overlap list is a merge warning, not a suggested order or an acceptance
decision. Re-run the named MM6 gate and MM7 control after shared-core merges.
