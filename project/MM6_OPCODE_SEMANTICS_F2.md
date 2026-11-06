# MM6 active EVT opcode semantics (F2)

Status: **bounded payload inventory, not a completed behavior specification** (2026-11-05). This packet covers the 67 retained listed-map `.evt` files and `global.evt`: 14,477 framed records and all 43 active opcode values. `scripts/mm6_probe_opcode_semantics.py` writes exact event IDs, steps, offsets, and payload bytes to ignored `local-game-data/mm6/research-2026-11-05/opcode-semantics-probe.json`. The table contains only aggregate shapes and source-linked leads. The probe checks SHA-256 against the earlier retained inventory, never reads the original archive, and does not run a game.

**Source hierarchy.** Payload widths and frequencies are measured from retained bytes. Current behavior is from `EvtEnums.h`, `EvtInstruction.cpp::parse`, `EvtInterpreter.cpp::execute`, and `EvtVariables.cpp`; a C++ case means only that a branch exists. The [MM6 editor's command catalog and argument definitions](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java) are leads, with several explicitly marked unverified. [MMExtension's event command reference](https://grayface.github.io/mm/ext/ref/) distinguishes MM6-only outdoor texture/facet operations and MM7+ movie/monster-bit operations. Neither reference proves how this owner's original executable handles every branch. `event-parser-probe.json`, `event-variable-probe.json`, and `dialogue-event-probe.json` hold complementary evidence.

Width below is bytes **after** the five-byte EVT frame header (length, event ID, step, opcode). `P/E` means a current C++ parse/execute switch case exists; it is not a compatibility verdict. Confidence `B` means only the frame width is bounded, `L` means an editor/MMExtension semantic lead also exists, and `C` means the retained layout and source lead align but execution remains unverified. Every row requires the indicated F4 scenario **and** all-record bounded decode with exact consumption, with title-specific state changes compared against the original behavior during F5. No row is marked implemented.

| Op | Active | Width | Primary MM6 interpretation / current C++ label | P/E | Conf. | Required F4 scenario |
| ---: | ---: | --- | --- | :---: | :---: | --- |
| 1 | 1609 | 1 | End event / `Exit` | Y/Y | C | End flow and preserve following event state. |
| 2 | 674 | 4 | Enter house / `SpeakInHouse` | Y/Y | C | MM6 house, transport, and terminal IDs. |
| 3 | 8 | 12 | Play sound / `PlaySound` | Y/Y | L | Positional and nonpositional audio. |
| 4 | 2134 | 1 | Mouse-over hint / `MouseOver` | Y/Y | C | Hint display, no executable step collision. |
| 5 | 56 | 1 | Location name / `LocationName` | Y/Y | C | MM6 location text and out-of-range reference. |
| 6 | 211 | 28, 33–38 | Map transition / `MoveToMap` | Y/Y | C | Same-map, cross-map, house dialog and cancel. |
| 7 | 361 | 1 | Open chest / `OpenChest` | Y/Y | C | Open/closed/looted and persisted inventory. |
| 8 | 47 | 2 | Face expression / `ShowFace` | Y/Y | L | Target character and portrait frame. |
| 9 | 123 | 6 | Character damage / `ReceiveDamage` | Y/Y | C | MM6 damage type, resistances and death. |
| 10 | 2 | 2 | Snow/weather / `SetSnow` | Y/Y | L | Weather toggle and map reload. |
| 11 | 215 | 10–14 | Indoor facet texture / `SetTexture` | Y/Y | C | Facet change and save/reload persistence. |
| **12** | **12** | **15** | **MM6 outdoor model/facet texture; inherited `ShowMovie` is the wrong operation** | **Y/Y** | **C** | **Model/facet lookup, texture change, save/reload; no movie.** |
| 13 | 97 | 7, 11–14 | Decoration sprite / `SetSprite` | Y/Y | C | Hide/show/name and reload. |
| 14 | 1943 | 6 | MM6 variable comparison / `Compare` | Y/Y | C | Typed MM6 compare and branch boundaries. |
| 15 | 1112 | 2 | Door state / `ChangeDoorState` | Y/Y | C | Open/close/toggle and moving door. |
| 16 | 1076 | 5 | Add MM6 variable / `Add` | Y/Y | C | Every active variable class and persistence. |
| 17 | 326 | 5 | Subtract MM6 variable / `Subtract` | Y/Y | C | Every active variable class, underflow rules. |
| 18 | 940 | 5 | Set MM6 variable / `Set` | Y/Y | C | Every active variable class, exact write. |
| **19** | **272** | **15** | **Create monsters: type/subtype/count and three coordinates; inherited summon expects 23 payload bytes** | **Y/Y** | **C** | **Spawn count, placement, grouping and reload on representative scripts.** |
| **20** | **7** | **5** | **Editor `ModifyItem` lead (four bytes + one byte suggested); no inherited enum/cases** | **N/N** | **L** | **Determine target item, operand roles, mutation and persistence before effect code.** |
| 21 | 129 | 27 | Event cast spell / `CastSpell` | Y/Y | L | MM6 spell ID/mastery and effect at source/target. |
| 22 | 15 | 4 | Speak NPC / `SpeakNPC` | Y/Y | L | NPC dialog transition and script continuation. |
| 23 | 32 | 9 | Indoor facet attribute / `SetFacesBit` | Y/Y | C | Indoor facet flag, visibility/collision and save. |
| **24** | **6** | **13** | **MM6 outdoor model/facet/bit/on candidate; inherited `ToggleActorFlag` consumes only 9 bytes** | **Y/Y** | **L** | **Resolve target semantics; outdoor facet effect if confirmed, with reload.** |
| 25 | 88 | 6 | Random branch / `RandomGoTo` | Y/Y | L | Each nonzero candidate and branch distribution. |
| **26** | **19** | **13** | **MM6 Question: three four-byte local text refs plus target step; inherited `InputString` consumes four bytes and asserts** | **Y/Y** | **C** | **Correct, incorrect and cancel paths, displayed text, target and continuation.** |
| **27** | **1** | **2** | **Editor `RndPassword` lead; no inherited enum/cases** | **N/N** | **L** | **Trace surrounding script and establish generated password side effect.** |
| **28** | **1** | **1** | **Editor `RndAnswer` lead; no inherited enum/cases** | **N/N** | **L** | **Trace answer generation/comparison and restart persistence.** |
| 29 | 723 | 4 | Status text / `StatusText` | Y/Y | C | Local/global string selection and display. |
| 30 | 676 | 4 | Message/scroll text / `ShowMessage` | Y/Y | C | Local/global text and dialog continuation. |
| 31 | 42 | 10 | Timer / `OnTimer` | Y/Y | L | Trigger schedule, repeat and save/reload. |
| 32 | 312 | 5 | Indoor light / `ToggleIndoorLight` | Y/Y | C | Light toggle and persistence. |
| 33 | 77 | 1 | Press any key / `PressAnyKey` | Y/Y | C | Wait, input and event resume. |
| 34 | 45 | 22 | Spawn object / `SummonItem` | Y/Y | L | Sprite, position, velocity and count. |
| 35 | 359 | 1 | Select party member / `ForPartyMember` | Y/Y | C | Subsequent command target and reset scope. |
| 36 | 362 | 1 | Jump / `Jmp` | Y/Y | C | Forward/backward step and loop guard. |
| 37 | 59 | 1 | Map reload hook / `OnMapReload` | Y/Y | C | Once/per-reload effect and saved state. |
| 38 | 23 | 10 | Long timer / `OnLongTimer` | Y/Y | L | Repeat cadence and save/reload. |
| 39 | 132 | 9 | NPC topic change / `SetNPCTopic` | Y/Y | C | Topic visibility, dispatch and quest state. |
| 40 | 29 | 8 | Move NPC / `MoveNPC` | Y/Y | L | House/map placement and persistence. |
| 41 | 80 | 6 | Give/create item / `GiveItem` | Y/Y | L | MM6 item IDs, inventory and overflow. |
| 42 | 41 | 4 | Change map event / `ChangeEvent` | Y/Y | L | Event binding and save/reload. |
| 43 | 1 | 7 | Skill check / `CheckSkill` | Y/Y | L | MM6 skill/mastery threshold and branch. |

## Decode and side-effect collisions to resolve before F4

1. **12 / 11:** Opcode 12 has a fixed 15-byte payload compatible with two four-byte outdoor identifiers followed by a NUL-terminated texture name. The inherited parser reads two bytes as movie flags and expects a movie name, so all 12 active records fail its end-of-record check. MMExtension explicitly marks `ShowMovie` MM7+ and `SetTextureOutdoors` MM6; the editor source contains conflicting 11/12 catalog versus argument-table comments. Use the retained bytes and original behavior to settle the exact MM6 field layout, then dispatch by title. Opcode 11 is the indoor texture operation and must remain distinct.
2. **19:** All 272 active records have 15 payload bytes (`u8, u8, u8, i32, i32, i32` candidate), whereas the inherited parser demands 23 bytes (additional group/name fields). It therefore rejects every active opcode-19 record. Spawn behavior, coordinate sign, subtype mapping and persistence need F4/F5 evidence.
3. **20 / 27 / 28:** These nine records are absent from the inherited enum, parser and executor. The MM6 editor names are hypotheses, and its item layout is explicitly unverified. Preserve unknown bytes and surrounding control flow; do not silently no-op these opcodes.
4. **24:** All six payloads are 13 bytes and leave four bytes under the inherited actor-flag parse. The layout fits `model:u32, facet:u32, bit:u32, on:u8`; MMExtension documents MM6 outdoor facet attributes while `SetMonsterBit` is MM7+. Confirm the actual MM6 target and bit namespace before changing world state.
5. **26:** All 19 retained Question records have three in-range local-string references and a valid target step. The current `InputString` parse leaves nine bytes and its executor asserts. The editor's order of failure/status and answer fields is unverified; retain exact text and targets privately and test correct, incorrect and cancel paths.
6. **14 / 16 / 17 / 18:** Their MM6 wire widths are already title-dispatched (one-byte variable ID), but the executor forwards values to the inherited `EvtVariables.cpp` MM7 vocabulary. The separate variable probe finds 4,285 operands, 127 IDs, and 1,609 operands in MM6-specific ranges that collide with MM7 meanings. Implement title-specific typed lookup and effects, including 100 map bytes, party/condition/skill/resistance ranges and quest bits; prove save/load for each writable family. A parser case is not sufficient.
7. **Other side effects:** Current enum names match many editor leads, but title-specific item, spell, NPC, house, timer, door, map, chest, texture and light state is still unresolved. Every active opcode needs an F4 executed case and F5 visible or saved-state check where applicable. The event campaign spec's NPC-topic precedence and terminal-house collisions remain separate blockers.

## Acceptance gate

F4 should decode all 14,477 active frames with exact byte consumption and no unknown command, then exercise every row's side effect/branch against explicit MM6 state. The private report's source and input hashes bind this matrix to the retained snapshot. F3's **single** original-installation preflight should compare those retained hashes, opcode widths and ambiguous operands; discrepancies reopen this matrix. F5 then checks graphics, dialogs, question flow, saving, and campaign milestones on the live MM6 build. None of those gates has been run by this packet.
