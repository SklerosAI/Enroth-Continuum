# Claude MM6 wall plates lane, 2026-11-18

Branch `claude/mm6-wall-plates`, off master `23f6eb0c9`. Follows the parked W2 investigation of the `event-member`
lane, which found the bug.

## Commits

- `800cbb284` Fire MM6 wall pressure plates when the party runs into them. The indoor collision pass reported the event
  of a pressure plate the party ran into, and `BLV_ProcessPartyActions` threw it away (`faceEvent2`, upstream #2244).
  It now reports only plates that aren't floors or walkable slopes, and MM6 runs that event at the end of the frame.
  MM7 still ignores it. `MM6WallPlate.WalkingIntoATempleDoorOpensIt` walks the party into door 2 of the Abandoned
  Temple and tapes doors 2 and 3 and whether the party got through.

## Verified against MM6.exe and the data

- Indoor party movement in the GOG `MM6.exe`:
  - The step-on check at `0x463d01` tests the plate bit (`0x4000000` at face +0x1c) when the floor face differs from
    the last one (`0x908cd8`). It stores the face's event in a local.
  - The collision loop tests the same bit at `0x464ae1` on every face the party collides with whose id differs from
    `0x908cd8`. It writes the same local. Floors (type 3 at face +0x4c), slopes (type 4, only when still moving) and
    every other type (walls, ceilings) all reach that test.
  - `0x908cd8` is written only by the floor check (`0x463d29`). So a wall never becomes the floor face, and a wall
    plate fires on every frame the party pushes against it.
  - The one stored event runs once at the end of the function (`0x464dd8`).
- MM7: the decompiled original in this repo's first `Indoor.cpp` (`03842a0b1`) does the same in its three collision
  branches. Upstream OpenEnroth `5284aecf8` (Fix #2244) moved the collision result into `faceEvent2` so a lift's
  floor plate in MM7 wouldn't retrigger, and that dropped wall plates with it. `upstream/master` still drops them.
  MM7 behaviour is left as is.
- All 67 MM6 maps were loaded with a temporary sweep hook (removed) that listed every pressure-plate face with its
  polygon type. 54 are walls: cd1 2 (events 13, 14, MoveToMap), cd2 1 (44, MoveToMap), cd3 5 (11, 17, doors),
  d01 Goblinwatch 12 (64, a one-shot CastSpell trap), d02 Abandoned Temple 16 (2 to 9, doors), d04 Hall of the
  Fire Lord 2 (10 warded door, 53 chest 10), d08 2 (1, doors), d17 1 (12, door), hive 2 (58, map byte), pyramid
  Tomb of VARN 7 (1, Captain's Code question), sci-fi 2 (1, status text), t6 1 (3, door), t7 1 (11, Compare then
  ReceiveDamage or door). pyramid also has 3 ceiling plates (event 13) that the fix covers too. Scripts read from the
  GOG `.evt` files under `local-game-data/mm6/research-2026-11-05/icons`. The GrayFace diffs don't touch these events.
- Only MM6 data was scanned for the plates. MM7 wasn't, since its behaviour doesn't change.

## Fixtures

None. New game through input, the oute3 door to d02 with Space, then a teleport in front of door 2.

## Test results

All on the Debug build in `build/`, one process at a time.

- `MM6WallPlate.WalkingIntoATempleDoorOpensIt`: fails without the fix (door tape stays `{DOOR_OPEN, DOOR_OPEN}`, the
  party stays east of the door), passes with it. Checked by stashing the `src/` changes, rebuilding and running it.
- MM6 game tests, `--gtest_filter='MM6*'`: 258 ran, 256 passed, 2 skipped (`MM6ZSaveHeadless` private-research
  cases that need an environment input), 0 failed. 126 disabled.
- `OpenEnroth_UnitTest`: 1170 ran, 1125 passed, 45 skipped, 0 failed.
- MM7 controls, `Prs.Pr2354:Prs.Pr2956:Issues.Issue840` plus `Issues.Issue2244`, `Issues.Issue2244b` (the lift
  the discard was for) and `Issues.Issue1290d` (MM7 floor plates): 6/6 passed.
- `check_style`: clean. `check_tidy` not run, clang-tidy 22 isn't installed locally.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/wall-plates/before.png` and `after.png`, 8 frames into walking at
door 2. Before: the party nose to the shut door, nothing moves. After: the door has slid aside and the room behind it
shows, and the minimap has the room drawn.
