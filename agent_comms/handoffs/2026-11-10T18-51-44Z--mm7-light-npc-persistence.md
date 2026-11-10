# MM7 Light Walls NPC seam handoff

- Branch: `codex/mm7-light-npc-persistence`, based on integration master `a9af27ae6`.
- Claim commit: `5813779e2`. Code commit: `cb9c9bba3`.
- Scope: only `test/Bin/GameTest/GameTests_MM7LightWalls.cpp` and this lane's coordination files. No runtime source changed.

## Finding

The successful Walls throne hand-in clears the throne NPC's first topic before Save. The old checkpoint note attributed that cleared value to fresh Load. The local headless probe observed the same value before production Save, immediately after normal fresh-process Load, and after the next map frames. The adviser NPCs occupy the four intended Celeste hostel houses at the end of the real Walls route. Their relevant topic fields, including the NPC80 later-quest topic, also match across the checkpoint. The source's `EVENT_SetNPCTopic` handler writes the live NPC field, and the MM7 snapshot and reconstruction copy the house and topic fields both ways. No save/load defect was reproduced, so no runtime change is justified.

The new assertions tape the adviser house result and the throne topic transition during `LightRoute`. They check the exact post-hand-in house and topic values before Save, immediately after Load, and after the following frames. The existing no-kill Walls route, quest bits, map, selected slot and unchanged slot bytes remain covered.

## Verification

- Headless `MM7LightWalls.LightRoute`, `ThroneClosedAtDawn` and three adviser controls passed 5/5.
- Exact marked disposable-profile writer and separate fresh-process reader passed 1/1 each. The disposable profile was removed after the run.
- Changed-file `cpplint.py` and `git diff --check` passed. This build does not define `check_style` or `check_tidy` targets.
- No graphics, normal profile, private saves or private logs entered Git.

## Next seam

This checkpoint ends before the three adviser hand-ins and Tolberti's cube hand-in. NPC80's first topic and house after that later hand-in still need a produced-state assertion in the assassination or Pit route, followed by a later fresh-process checkpoint if persistence across that chapter boundary is required. The current Xenofex test writes its start state, so the Walls reader cannot prove the later seam by itself. Correct the older wiki sentence that says fresh Load reset the throne topic when integrating this lane.
