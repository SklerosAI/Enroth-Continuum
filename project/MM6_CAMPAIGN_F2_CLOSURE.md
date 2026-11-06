# MM6 campaign, dialogue and travel F2 closure matrix

Status: **source and retained-reference audit, 2026-11-05; F2 remains open**.
This is a handoff for full MM6 implementation. It does not establish that an
MM6 map can run, that a guide milestone is reachable, or that a candidate
quest bit has the guide's meaning. The original archives were not reopened.
Exact IDs, event offsets, table cells and dialogue text remain in ignored
`local-game-data/mm6/research-2026-11-05/` reports.

## Evidence and method

The tracked `scripts/mm6_probe_campaign_routes.py` reads the retained
`2devents.txt` and `trans.txt` copies plus the ignored event, dialogue and
quest-graph reports. It verifies the upstream report fingerprints, bounds
active `SpeakInHouse` payloads, and joins the guide keyword seeds to event
function read/write indexes. It writes detailed records and source hashes to
ignored `campaign-routes-probe.json`; stdout contains aggregate counts only.
This is **parse and source-consumer evidence**. The six milestone groups use
keyword candidates from `scripts/mm6_probe_quest_graph.py`, so an event-function
co-occurrence is a lead, never a control-flow or reachability proof.

The check found 674 active `SpeakInHouse` records. Two target IDs absent from
the MM6 house table are in the Hive script and use the engine's pseudo-ending
house IDs 600 and 601. All payloads inspected by this probe fit the expected
four-byte house-ID shape. The [event/campaign specification](MM6_EVENT_CAMPAIGN_SPEC.md)
still records 318 deterministic parser rejections in other opcode families;
this probe does not supersede that finding.

## Campaign dependency and acceptance matrix

| Milestone | Retained structural lead | Missing implementation proof | F4 offline and F5 graphical acceptance |
| --- | --- | --- | --- |
| Opening and Goblinwatch | Eight keyword candidate quest rows have script bit use, with read/write event functions in the private graph; two candidate map names. | Decode exact producers, prerequisite tests, item handoff, town/indoor transitions and reset state; inspect guide PDF pp. 55–59 with shipped events. | Create/Quick Start, enter town and Goblinwatch, fight, obtain and turn in the objective, train, save/quit/load; verify map and quest state. |
| High Council and traitor | Twelve of thirteen keyword candidate quest rows have script bit use; current C++ has no MM6 variable dispatch. | Trace all council vote flags, admission conditions, Nicolai/traitor branches and legal quest order; reconcile Prima printed pp. 273–286. | Complete six subquests in alternate orders, inspect votes and admission, save/reload through at least one branch. |
| Oracle and crystals | Twenty of twenty-three keyword candidate quest rows have script bit use; one map-name candidate. | Trace four crystal acquisitions/placements and Oracle dialogue; classify its topic IDs before using inherited MM7 special handling. | Deliver crystals, see state changes and route unlock, save/reload; lost-item behavior only if supported by MM6 evidence. |
| Varn and Control Center | One keyword candidate quest row with script bit use, two map-name candidates. | Resolve puzzle/question strings and branches, required flags/items and two map deltas; compare Prima printed pp. 293–298. | Complete correct, incorrect and cancel input branches; transition through both maps and reload persistent doors/flags. |
| Hive ending | Four keyword candidate quest rows with script bit use; two active pseudo-house instructions in the Hive EVT. | Prove full Archibald, Ritual, reactor and Queen dependencies and their event order. Replace or title-scope the MM7 end-window, certificate and respawn path. | Reach the actual MM6 finale after a loadable pre-finale save; inspect film/certificate/state, quit and reload without entering an MM7 map. |
| Promotions and regional quests | Five keyword candidate rows, four with script bit use; keyword coverage is incomplete for twelve promotion stages and regional quests. | Join each class promotion, Seer and regional quest to explicit event/house/NPC/award changes and correct training rules; compare Prima printed pp. 303–336. | Exercise first and second promotions for each class, representative order/failure cases, and region-specific rewards after save/reload. |

The aggregate candidate counts are useful for scheduling review, not a
completeness score. Every guide quest and all 208 script-used quest bits still
need a semantic role, producer/consumer chain and acceptance scenario; four
read-only and seventy write-only bits may have legitimate external uses.

## Critical current-consumer seams

| Seam | Confirmed current behavior | Required MM6 resolution |
| --- | --- | --- |
| EVT parsing and dispatch | `EvtInstruction.cpp` rejects active MM6 records; `EvtInterpreter.cpp` has an incomplete `EVENT_InputString`. The retained Question audit finds 19 thirteen-byte records with 57 in-range string references and all target steps present. | Define title-specific schemas and effects for all 43 active opcodes; test Question answer, incorrect and cancel paths without borrowing MM7 or MM8 operand meaning. |
| Variables and NPC topics | `EvtVariables.cpp` uses inherited MM7 IDs; the retained audit found 127 active IDs and 1,609 MM6-specific-range operands. `NPCTopics.cpp` can display a label without a global event; 35 references across 18 IDs lack both a global event and current special handler. Another 74 references across 43 IDs have a global event but fall in the inherited teacher-special range, whose selection handler runs before `runEvent()`. | Provide MM6 variable and map-byte semantics with persistence. Classify every nonzero NPC topic as global, MM6 special, or evidenced text-only; check current teacher/arena/guild/Oracle branches against MM6. The 74 references are a routing collision, not proof every script should execute or is reachable. |
| House entry and title-specific services | `EvtInterpreter.cpp` sends `SpeakInHouse` to `enterHouse()`, which dispatches `houseTable[houseId].uType`. The separate `scripts/mm6_probe_house_routes.py` audit identifies MM6 type labels that the inherited map does not recognize. | Map each active MM6 service/house type and preserve valid entry, price, schedule, dialogue, and return behavior. Do not treat an accepted house ID as a functioning service. |
| Stable and boat travel | Retained `2devents.txt` has 21 typed transport houses: nine stables and twelve boats. Six stable IDs lie outside `Transport.cpp`'s inherited route-array range 54–73. `Transport.cpp` indexes `transportRoutes[houseId()]` and hardcodes 35 MM7 map/schedule rows. `Engine.cpp` loads MM6 `trans.txt`, but `TransitionTable.cpp` parses it into display descriptions, not destination schedules. | Determine the MM6 route source, schedule/day rules, destination coordinates, quest gating, cost and time from owned evidence; use a title-scoped table with checked IDs. Test every active stable/boat, including the six out-of-range IDs, and save/reload after departure. |
| Finale pseudo houses | The two active Hive `SpeakInHouse` pseudo IDs 600/601 match `UIHouses.cpp`'s MM7 game-over shortcut. That opens `UIGameOver.cpp` and `GameOver.cpp`, which use MM7 alignment/certificate text and write `MM7_Win.Pcx`; `Game.cpp` then autosaves and respawns at Harmondale. | Define MM6 win triggers, film/certificate and post-finale state from shipped events and guides. Keep MM7's ending path intact and MM6 saves in its own profile. A direct call into the current path is not an MM6 ending implementation. |

The out-of-range transport indexing is a deterministic source-level risk:
`IndexedArray::operator[]` subtracts the first index and delegates to
`std::array::operator[]` without a bound check. No live crash is claimed.
Likewise, the MM7 finale path is established by source control flow, not by
an observed MM6 ending run.

## Gate remaining before implementation

1. Decode every active opcode and variable family, including conditional
   branches, Question, password/item operations, NPC mutation and map bytes.
2. Build a private dependency graph with exact quest, house, NPC, item and map
   nodes; resolve the six guide milestone families and every promotion and
   regional quest. Keep unknown original intent explicit.
3. Determine all MM6 transport routes and classify every house and topic.
   The loaded `trans.txt` is a text source, not a route table.
4. Inspect and specify the MM6 finale as a title-specific state transition.
   The two Hive pseudo-house records are structural leads, not a complete
   win-condition proof.
5. Run the planned single read-only F3 original-file preflight only after
   all F2 domains are broad enough. F4 then exercises every active family
   offline; F5 verifies the campaign graph and persistent saves in the
   graphical engine.

Sources: [MM6 guide reconciliation](MM6_GUIDE_RECONCILIATION.md),
[event/campaign specification](MM6_EVENT_CAMPAIGN_SPEC.md),
`src/Engine/Evt/EvtInstruction.cpp`, `src/Engine/Evt/EvtInterpreter.cpp`,
`src/GUI/UI/NPCTopics.cpp`, `src/Engine/Tables/NPCTable.cpp`,
`src/Engine/Tables/HouseTable.cpp`, `src/Engine/Tables/TransitionTable.cpp`,
`src/GUI/UI/Houses/Transport.cpp`, `src/GUI/UI/UIHouses.cpp`,
`src/GUI/UI/UIGameOver.cpp`, `src/Application/GameOver.cpp`, and
`src/Application/Game.cpp`; reviewed 2026-11-05 at branch base `8b2440333`.
