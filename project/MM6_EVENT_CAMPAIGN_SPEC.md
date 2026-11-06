# MM6 events, dialogue and campaign specification (F2)

Status: **structural audit and source-consumer reconciliation, 2026-11-05**.
This document records implementation obligations, not a functioning MM6 event
engine or proven campaign route. Detailed event records, NPC text, quest IDs,
and dependency graphs remain in ignored local reports. F3's consolidated
original-file preflight has not happened.

## Evidence and scope

The retained, ignored `event-probe.json`, `event-parser-probe.json`, and
`event-variable-probe.json` cover the 67 listed map scripts plus `global.evt`.
The reproducible `scripts/mm6_probe_dialogue_events.py` writes a detailed,
ignored `dialogue-event-probe.json` and prints only aggregate Question and NPC
topic counts; it records source and input hashes and requires the report to
stay under the ignored MM6 reference tree.
Their parser/source hashes were checked against the current C++ tree. The
owned guide's campaign milestones are indexed in
[MM6_GUIDE_RECONCILIATION.md](MM6_GUIDE_RECONCILIATION.md), but are not yet
linked into a complete shipped-event dependency graph. The current adapter
has not reached a playable MM6 map.

| Area | Confirmed from retained records and current source | Semantics still to establish |
| --- | --- | --- |
| Parser coverage | At least 318 deterministic parser failures affect 26 of the 68 active EVT files, all map scripts. `EvtProgram.cpp` aborts on a parse error, and `EvtInstruction.cpp` rejects unread payload bytes. `global.evt` has none of these specific framing failures. | Operand meaning and execution for every active opcode, even those that parse without error. A loaded global program does not prove map scripts can load. |
| MM6 Question, opcode 26 | Nineteen records across six maps have a 13-byte payload. Each contains three in-range local-string references and an existing target step. Current `EVENT_InputString` consumes only one four-byte argument and asserts in the interpreter. | Exact operand order, answer matching, cancel/incorrect path, target-step behavior, UI and save consequences. [MMExtension's event reference](https://grayface.github.io/mm/ext/ref/) describes a three-text-parameter Question; the [MM6 editor source](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java) proposes a layout but marks it unverified. |
| Missing opcodes 20, 27 and 28 | Nine active records use these absent enum values: seven, one and one respectively. The parser's default path rejects them. The opcode-20 records have five-byte payloads; candidate item-ID interpretation is not yet a rule. | Item modification and password/answer behavior suggested by the editor source require surrounding-script and original-behavior checks. Do not implement the labels as proved semantics. |
| Variables and quest state | The retained audit covers 4,285 compare/add/subtract/set operands using 127 variable IDs; 1,609 use MM6-specific ranges that collide with inherited MM7 meanings. Quest-bit variable use reaches 208 bit values; only 52 have visible quest-note text. | MM6-specific condition, skill, resistance, party-state and 100 map-byte mapping; effects and persistence of script-only quest bits. `EvtVariables.cpp` currently has no MM6 title dispatch. |
| NPC topics | A retained NPC/topic comparison finds 298 nonzero first-three topic references. Of the 128 without a `global.evt` event, 93 fall into current special handler ranges; 35 references across 18 IDs have nonempty labels but no matching global event or current special handler. | Whether those 35 are intentional text-only choices, missing MM6 special dialogues, or other behavior. `canShowTopic()` can offer a nonzero topic without an event; `runEvent()` then does nothing. Check every topic's intended behavior before declaring a defect. |
| Campaign | Guide milestones cover the opening, High Council, Oracle, Varn/Control Center, Archibald/Hive, promotions and regional quests. | The exact map/NPC/house/item/variable dependencies, allowed order and branch outcomes from shipped scripts. Quest-note rows alone cannot identify all script-used state. |

### Structural campaign graph

`scripts/mm6_probe_quest_graph.py` joins the retained map, house, event,
dialogue and quest-note indexes into an ignored `quest-graph-probe.json` with
input/source hashes. It records 2,860 active event functions and their
structural references. All 93 explicit script cross-map targets and all 41
nonzero house exit-map IDs resolve to listed MM6 maps. Another 118
MoveToMap records stay on the current map. House origin grid codes provide
537 candidate map links; 20 other origin labels require manual interpretation.

The graph sees 208 quest-bit values in scripts: 138 are read, 204 written,
134 both, four read without a script write and 70 written without a script
read. The one-sided values may be initialized, consumed elsewhere or designed
as terminal state; they are **not** automatically defects. Six guide milestone
keyword groups produce private candidate quest/map seeds, not event-control
flow or a playable route. Among 132 SetNPCTopic instructions, two nonzero
target topics lack a global event; one falls in the current teacher-special
range, and the other needs MM6-specific dialogue classification. No original
game behavior has been inferred from these counts alone.

## Current C++ consumers

- `src/Engine/Evt/EvtProgram.cpp` and `EvtInstruction.cpp` frame and decode
  scripts. `EvtInterpreter.cpp` executes them; it currently has an unimplemented
  `EVENT_InputString` path. MM6 needs title-specific operands, validation and
  execution for all active opcodes, with unknown commands rejected safely.
- `src/Engine/Evt/EvtVariables.cpp` interprets compare/add/subtract/set IDs
  through MM7 ranges and storage. MM6 event IDs must use an explicit MM6
  vocabulary, coordinated with the 200-byte map-state region in
  [MM6_SAVE_SPEC.md](MM6_SAVE_SPEC.md).
- `src/GUI/UI/NPCTopics.cpp` offers topic IDs using `canShowTopic()` and routes
  selected topics to MM7 teacher, arena, guild and Oracle special cases or to
  `global.evt`. `EvtInterpreter::canShowTopic()` does not require an existing
  event, and `execute()` returns immediately when none exists. MM6 requires a
  complete classification of every nonzero topic reference; do not map a
  campaign dialogue through an unrelated MM7 special case.
- `src/Engine/Tables/NPCTable.cpp` loads topic references. The retained
  comparison covers its first-three fields; remaining dialogue fields and
  title-specific NPC state need their own coverage before F2 closes.

## F2 closure and F3 preflight

1. Define a bounded MM6 operand schema and side effect for each of the 43
   active opcodes. For opcode 26, test correct, incorrect and cancel answers
   against the retained branch targets without copying local strings into Git.
   For opcodes 20/27/28, retain hypotheses until script context and behavior
   support them.
2. Map all 127 active MM6 variable IDs to typed effects and save locations;
   include the 100 map-byte range and adjacent invalid IDs. Tie every
   script-used quest bit to its producer, consumer and campaign role, including
   values without a visible quest note.
3. Classify each nonzero NPC topic as a global script, MM6-specific special,
   or evidenced text-only action. Decide visibility, selection and follow-up
   UI behavior, and give every campaign-critical topic an acceptance scenario.
4. Build a private campaign dependency graph from retained map, NPC, house,
   travel, text and EVT references. Publicly report coverage and unresolved
   links, not proprietary strings or a per-ID catalog. The structural graph
   above is a starting index; add actual branch conditions, producers,
   consumers and route order, including alternate quest order, failure states,
   promotions and the ending.
5. In the single F3 read-only original-file preflight, verify event framing,
   string targets, opcode lengths and graph links against the owned archives;
   record discrepancies only in ignored local storage. Targeted later rechecks
   remain possible when evidence warrants them.

## F4 and live acceptance

Offline F4 checks must parse all 68 active EVT files and exercise every
active opcode and variable family, not merely return zero parser errors. Run
scenario checks for question branches, item modification, password flow,
NPC topics, trainer/guild behavior, map transitions, quest flags and ending
reachability. At F5, inspect real dialogue and the graphical view, complete
the opening and later milestone chains, and save/quit/relaunch/load after each
major state change. A quest text row or successfully parsed event is not proof
that the player can reach the ending.

See also: [MM6 implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md),
[guide reconciliation](MM6_GUIDE_RECONCILIATION.md),
[save specification](MM6_SAVE_SPEC.md), and [local status](LOCAL_STATUS.md).
