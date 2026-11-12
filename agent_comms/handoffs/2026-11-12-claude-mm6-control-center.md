# Claude — MM6 Control Center lane, 2026-11-12

- Owner: Claude Code, MM6 lane `mm6-control-center`, branch `claude/mm6-control-center` off master `7024a995c`.
- Recipient: the lead, and the C7 and C8 chapter lanes.
- Private trace with addresses and map facts: `local-game-data/private-wiki/mm6/CONTROL_CENTER_PE_TRACE_2026-11-12.md`.

## Commits

| Commit | What |
| --- | --- |
| `e30081bf2` | Test `MM6HouseExit.OracleExitNeedsItsQuestBit`: the Oracle's house 170 hides its exit without bit 169 and shows it with the bit (fixture), and the exit lands at sci-fi.blv's party start (−9280, 4160, 196.5). No engine change. |
| `c69ea917e` | Tests `MM6ControlCenter.OracleTakesTheCube` (global 76 hand-in, then 77 sets 169, exit taken) and `MM6ControlCenter.BlasterInStorageContainer` (trapped chest 1, item 64 into the backpack). No engine change. |
| `cfb2e0305` | Fix: an MM6 skill Set plays the award chime per character, as MM6.exe 0x440FA1 does. Test `MM6ControlCenter.TerminalTeachesBlaster`. |

## Route for C7 and C8

Event numbers are local to the map unless marked global. "Test" means a game test drives it by input against the real data,
"script" means read from the decoded scripts and the map data only.

| Step | Where | Event | Needs | Gives | Checked by |
| --- | --- | --- | --- | --- | --- |
| C7.1 Oracle wakes | oracle.blv crystal modules | 5, 15, 16, 17 (power on: panel 14) | items 550..553, bits 100..103 | +100000 exp each, Oracle topics 76 / 0 after all four | script |
| C7.2 Cube quest | Oracle (NPC 8, house 170, face event 3) | global 76 | bit 166 clear | bit 166, award 33, clears 162..165 | script |
| C7.3 Raise the Oracle | oracle.blv floor plate | 2 | step on it | door 2 lifts her screen, talkable after 4 frames | test |
| C7.4 Enter the Tomb | outb3.odm | 90 | — | pyramid.blv | script |
| C7.5 Keys and codes | pyramid.blv chests | 42 (bit 229), 43 (needs 576, bit 230), 44 (bit 231), 45 (bit 232), books 26 and 27 | — | keys 573, 574, 576, 577, codes 537..542, Crystal Skull 466 | script, chest contents read from the loaded map |
| C7.6 Codes into the pools | pyramid.blv | pools 21..25, then Well 1 last | each code item, a typed answer (Question) | MapByte27, then MapByte15 "The waters part" | script. No test types a Question answer yet |
| C7.7 Control room | pyramid.blv | 35, then chest 41 | MapByte15, item 577 | Control Cube 456 (chest 1), bits 42, 195 | script, chest contents read from the loaded map |
| C7.8 Hand in the cube | Oracle | global 76 | bit 166, item 456 | cube taken, 166 and 195 cleared, award 34, +500000 exp to all four, topics 77 / 78 | test (166 and cube by fixture) |
| C7.9 Kreegan topic | Oracle | global 77 | bits 305 and 96 clear | bit 169, bit 235, Nicolai topic 26 | test |
| C8.1 To the Control Center | Oracle house 170 exit | 2DEvents row 170 | bit 169 | sci-fi.blv party start | test |
| C8.2 Blaster skill | sci-fi.blv terminal, face 2888 | 61 | bit 276 clear | Blaster 1 for all four, bit 276, award chime each | test |
| C8.3 Blasters | sci-fi.blv storage containers | 30, 31, 33, 36, 37, 65, 66, 68, 69 (chests 1, 2, 4, 7, 8, 9, 10, 12, 13), all trapped | — | items 64 Blaster and 65 Blaster Rifle | test for event 30, the rest from the loaded map |
| C8.4 Equip | character screen paperdoll | — | Blaster skill | blaster in main hand | throwaway test only |
| C8.5 Back to the Oracle | sci-fi.blv | 70 | — | oracle.blv (−1433, 2204, −495) | script |
| C8.6 Third Eye | Nicolai (NPC 13) | global 26 | topic from C7.9 | bit 96 | script |
| C8.7 Well | outd3.odm | 231 | bit 96, not 305 | item 446, bits 305, 196 | script |
| C8.8 Tanir's Bell | Nicolai | global 27 | item 446 | item 461 to the active character, bit 197 | script |
| C8.9 Release Archibald | King's Library, house 168 → 553 | global 30 | item 461 | item 544 Ritual of the Void, bit 177, clears 235, −1000 reputation and +50000 exp each | test `MM6HouseExit.ArchibaldLeavesTheLibrary` (bell by fixture) |

The bit that opens the Control Center is set by the Kreegan topic (77), not by the cube hand-in (76), although message 91 says access
is granted. A chapter must click both topics.

## Verified against MM6.exe

- The Set opcode's party selector runs SetVariable 0x4407E0 for all four characters unconditionally.
- Skill ids 56..86 (0x440FA1) write the skill byte, then sparkle (0x4358F0, animation 20) and sound 20001, no reaction speech. Ported
  as the chime only, as for awards and resistances.
- The original ORs a plain level into the old level bits and drops mastery. Not ported, unreachable: the only shipped skill Set is the
  terminal, once, and nothing earlier teaches the base Blaster skill (all 83 MM6 evt files scanned).

## Fixtures

- `OracleExitNeedsItsQuestBit`: bit 169.
- `OracleTakesTheCube`: Oracle topics 76 / 0 (as the crystals leave them), bit 166, item 456 in the first backpack.
- `BlasterInStorageContainer`: `NoDamage`, a new party dies to the trap.
- All map entries by `teleportTo` with the chest opt-in, like the other MM6 house exit tests.

## Test results

- `Run_UnitTest`: 1122 passed, 45 skipped, 0 failed.
- Full MM6 suite: 183 passed, 2 skipped (the 4 new tests included).
- MM7 controls (EvtInterpreter.cpp is shared): 147 passed, 2 skipped.
- `check_style` clean. `check_tidy` not run, no clang-tidy 22 here.
- Revert check: with `cfb2e0305`'s source change reverted, `TerminalTeachesBlaster` fails with 0 chimes instead of 4. The three
  test-only tests pass on master, there was no fix to revert for them.
- Capture `local-game-data/progress-captures/20261112-185831-mm6-control-center.png` (first map, not this lane's screens). The food
  and gold counters next to the apple and the coins show no numbers. Everything else in the frame looks right.

## Open items

- Typed Question answers (pyramid pools and well) have no input test. C7 needs them. pyramid.str keeps the answers reversed (kriK,
  kcopS, uluS, aruhU, yttocS, yoccM), check what the player types against MM6.exe before writing the chapter.
- The terminal's portrait sparkle is not drawn, same as for every other MM6 variable write.
- 2DEvents row 170 column 24 "Access denied. You must have the Control Cube to gain access." is never shown. MM6 hides the exit
  without bit 169 (lane X trace of 0x43C140), so it may be dead data. Not traced.
- Opening a chest logs `Trying to load non-existent LOD entry ... icons.lod/ib-bcu-a` (`GUIWindow.cpp` loads an MM7 cancel button
  background). Harmless, not touched.
- Blaster Expert and Master teachers (2DEvents house 478 notes "44 (Blaster Expert)") are not part of this lane.
