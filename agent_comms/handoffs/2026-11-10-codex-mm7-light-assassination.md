# MM7 Light assassination L6–L7 handoff

- Owner: Codex subagent, branch `codex/mm7-light-assassination` from master `0166b5bd1`.
- Code commit: `a7144b9df`. Claim commit: `1f476ef43`.
- Integration: pending parent and Claude review. No merge, push, graphical run, wiki, PLAN or runtime edit.

## Scope

A new `GameTests_MM7LightAssassination.cpp` and its CMake registration cover two input-driven cases. The Celeste door case presses Robert's door with the three advisor completions versus one missing and checks the resulting story bit. The request/hand-in case enters the advisor's house through a clickable map face, selects the assassination topic, uses Tolberti's Pit door and accepts the transition into the small house, then returns to the advisor. Without the control cube, hand-in leaves the task active; with it, hand-in consumes the item, clears the active bit, sets the completion bit, and moves the next advisor to the throne room. Quest-bit tapes check transitions rather than only final state.

L4's move of the advisor into the hostel and the three L5 outcomes are fixture prerequisites. The test teleports between chapters and fixtures the control cube at hand-in. It does not prove ordinary travel through the hostile Pit, killing the cube carrier, looting the cube, or uninterrupted New Game-to-L7 continuity. The map face lookup uses the loaded event program and asserts the real house reached by input; no private map inventory or raw script is copied here.

## Focused evidence

- `OpenEnroth_GameTest` built and linked.
- `MM7LightAssassination.*` passed 2/2.
- Combined `MM7LightAssassination.*:MM7LightWalls.LightRoute:MM7LightWalls.ThroneClosedAtDawn` passed 4/4.
- Changed-file `cpplint.py` and `git diff --check` passed.
- Private game data and raw logs stayed local outside Git.

## Next boundary

Integrate after review, then connect the L5 advisor route and add a separate carrier-combat and input-loot test if the stage-1 gate requires full L7 acquisition. The later Overthruster chapter and its fresh-process checkpoint remain separate scopes.
