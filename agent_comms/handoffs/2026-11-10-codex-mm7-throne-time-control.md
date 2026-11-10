# MM7 Lambent throne time control handoff

- Branch: `codex/mm7-throne-time-control` in reused `mm7-load-admission`.
- Code commit: `5f475d95a`. No runtime source changed.
- Scope: `MM7LightWalls.ThroneClosedAtDawn` now checks the same Lambent throne door in two fresh iterations. Waiting to hour 5 keeps the door closed; the new game's later hour opens the exact throne house. The Walls quest stays inactive in both checks.
- Evidence: the focused throne and Light route tests passed 2/2 headlessly. Changed-file cpplint and `git diff --check` passed.
- Integration: cherry-pick `5f475d95a`, rerun `MM7LightWalls.ThroneClosedAtDawn:MM7LightWalls.LightRoute` on integration source. This closes W4 review finding F3's missing positive control. The earlier fresh-process Walls checkpoint remains independent and its runtime path was not changed.

No private data, saves, logs or graphical windows were used.
