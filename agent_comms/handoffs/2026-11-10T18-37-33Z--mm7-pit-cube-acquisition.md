# MM7 Pit control-cube acquisition handoff

- Branch: `codex/mm7-pit-cube-acquisition`, based on integration master `e1ac7b830`.
- Code commit: `b31408129`. Claim commit: `752b8aa5c`.
- Owned files: new `test/Bin/GameTest/GameTests_MM7PitCubeAcquisition.cpp` and its line in `test/Bin/GameTest/CMakeLists.txt`. The existing assassination test and all runtime files remain untouched.
- Gate: starts from the advisor request through the house dialogue, enters the Pit target room through the transition UI, finds the live cube-bearing quest target, defeats it through bounded attack-key input, finds a selectable corpse pixel in the viewport, clicks to loot, and returns through the advisor dialogue to hand in the cube. It asserts cube possession and consumption, active/completed quest-bit transitions, and the relevant NPC move. The existing `MM7LightAssassination.RequestAndHandIn` retains the no-cube negative control.
- Combat uses a party-only equipment/stat fixture and periodic party positioning to keep the moving target in melee range. It does not write actor HP/state or call `LootActor`. Corpse acquisition uses a bounded viewport scan because the standing-center test helper misses a collapsed corpse.
- Checks: GameTest target built; focused new test plus existing request/hand-in control passed 2/2 in three separate headless processes, then passed 2/2 after the final exact cube tape assertion. `git diff --check` passed. No graphical run or normal profile write.
- Integration: review/cherry-pick the two branch commits onto current master, rebuild the focused GameTest, and run the same two filters once. No shared runtime change is required from this gate. Natural walking through the room is not covered by this test.
- Privacy: no private game data, saves, logs, or asset inventory added to Git or this handoff.
