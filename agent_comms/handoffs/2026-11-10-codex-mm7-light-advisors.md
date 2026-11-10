# MM7 Light advisor L5 hand-ins

- Branch: `codex/mm7-light-advisors`, based on master `71282f2e5` in the reused `mm7-death-regression` worktree.
- Code commit: `e63eeae69` (`GameTests_MM7LightAdvisors.cpp` and its CMake registration). Claim commit: `5df0760ec`.
- Headless focused result: `MM7LightAdvisors.*` passed 3/3 against the local MM7 data. The isolated `OpenEnroth_GameTest` target compiled and linked. `cpplint` and `git diff --check` passed.
- Reproduce: `build/test/Bin/GameTest/OpenEnroth_GameTest --game mm7 --headless --data-path "$MM7_DATA_PATH" --test-path test/Data '--gtest_filter=MM7LightAdvisors.*' --gtest_brief=1 --log-level=error`.
- Coverage: starts at a post-Walls Light guild fixture, reaches each Celeste hostel door by input, accepts each NPC quest topic through the house UI, and submits the return topic. Altar pieces require both quest items; the one-piece negative leaves the item and quest unfinished. Wine Cellar hand-in requires the vampire quest bit; the missing-bit negative stays unfinished. Soul jars require the case item; the missing-case negative stays unfinished. Positive hand-ins assert completion and item consumption where relevant.
- Boundary: Light alignment and Walls traversal belong to `MM7LightWalls.LightRoute`; the test deliberately fixtures that prior state. Quest item acquisition, Wine Cellar monster combat, the Wine Cellar exit's kill-bit behavior, and the later Xenofex tasks remain unverified here. No private data or logs were added to Git. No GUI run or runtime source edit was made.
- Integration: cherry-pick `e63eeae69` only for the code slice; the claim and this handoff can be carried separately if desired. Rerun the focused 3-case filter on integration master.
