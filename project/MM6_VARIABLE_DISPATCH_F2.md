# MM6 event-variable dispatch contract (F2)

Status: **source and retained-script diagnosis, not an implemented MM6 variable engine** (2026-11-05). This packet narrows the variable portion of [the event specification](MM6_EVENT_CAMPAIGN_SPEC.md). It does not close F2, perform the one original-installation F3 pass, or establish that a quest branch runs. The detailed 127-ID inventory and 4,285 use sites stay in ignored `local-game-data/mm6/research-2026-11-05/event-variable-probe.json`.

## Evidence and reproducibility

Run `scripts/mm6_probe_event_variables.py` against the retained, ignored research directory. Report format 2 fingerprints `mapstats.txt`, each of the 68 active EVT files, `EvtEnums.h`, `EvtVariables.cpp` and `EvtInterpreter.cpp`; re-run when any of these change. It indexes operands of opcode 14 Compare, 16 Add, 17 Subtract and 18 Set. The numeric MM6 family labels are **leads** from the [MM6 event editor's format definitions](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java), cross-checked against [MMExtension's `evt.VarNum` reference](https://grayface.github.io/mm/ext/ref/). They are not proof of original side effects or save locations.

The retained scan found 4,285 variable operands, 127 distinct byte IDs, and no framing failures. **1,609 operands are in MM6-specific ID ranges where the inherited enum cannot be trusted; many, but not all, collide.** The count is a source/data warning, not a count of observed bad quest outcomes. The current [EVT parser](../src/Engine/Evt/EvtInstruction.cpp) has a one-byte variable ID path for MM6, but [the interpreter](../src/Engine/Evt/EvtInterpreter.cpp) forwards it to [the shared variable executor](../src/Engine/Evt/EvtVariables.cpp), which branches on [MM7-oriented names and ranges](../src/Engine/Evt/EvtEnums.h). Value validation also uses those names, so an operand can be rejected, compared, written or saved under the wrong meaning even when the EVT frame parses.

| MM6 candidate ID family | Active uses / IDs | Current collision and required interpretation |
| --- | ---: | --- |
| 0–45, common-looking party, character, item and quest fields | 2,676 / 39 | Some numeric labels agree, but MM6 class IDs, item IDs, award/quest semantics and target selection still require title-specific validation. A same-name enum is not sufficient. |
| 46–55, five base and five temporary resistances | 53 / 10 | MM6's Electricity, Cold, Poison and Magic channels do not align with the inherited Air/Water/Earth/Spirit and later MM7 channels. The present `compare`/`set`/`add`/`subtract` code uses MM7 character resistance fields. |
| 56–86, 31 MM6 skill slots | 40 / 2 | Both active IDs collide with inherited MM7 resistance/spell-skill meanings. The joined level/mastery validation currently selects its skill range from MM7 enum bounds; MM6 has Normal/Expert/Master only. |
| 87–104, conditions | 54 / 15 | These IDs land on inherited skills or condition names shifted by MM7's extra skill slots. The MM6 order and `good`/major-condition behavior require title-specific status operations. |
| 105–204, 100 map bytes | 1,240 / 51 | IDs 105–122 are interpreted as MM7 conditions before the current 75+25 map-variable split begins at 123. Remaining IDs use a different offset. Preserve all 100 MM6 map bytes with map reset/reload and save semantics; the retained all-zero baseline cannot prove nonzero behavior. |
| 205–226, autonote, character/party, NPC, reputation, day and other state | 222 / 10 | All active IDs land on MM7 decoration variables or unrelated later fields. Exact storage, valid values and date/reputation effects need F3/F4 confirmation. |
| 227–255, timers and later fields | 0 / 0 | No active retained use was found. Do not infer absence from the format or import MM7 semantics by default. |

The 100 map-byte interpretation and high-ID labels come from editor/MMExtension references and retained use patterns. F3 must compare the original installation's script identity and, where possible, executable variable dispatch against this lead. If an exact side effect cannot be extracted in that read-only pass, keep the family explicitly unresolved and make an offline F4 behavior fixture plus a live F5 scenario for it.

## F4 dispatch and save contract

1. Decode the raw MM6 variable ID into a title-specific, typed descriptor before any value check or execution. The descriptor names target scope (party, selected character, map, NPC, timer), operation support, value type/range, and save field. Keep MM7 dispatch and its existing save records separate.
2. Define `compare`, `add`, `subtract` and `set` for every **active** ID and valid operand; preserve control-flow consequences, including a failed subtract stopping the event and the selected-character behavior in [the interpreter](../src/Engine/Evt/EvtInterpreter.cpp). Reject unknown active IDs or invalid values explicitly instead of treating them as an MM7 enum or silently skipping them.
3. Cover quest bits and inventory with MM6 IDs. A script-only quest bit is still state; a missing visible quest note does not make it unused. Save all writable fields, including map bytes and party/character state, in `.ec6`; reject a cross-title save before mutation.
4. Exercise each active ID and operation present in the private catalog with bounded offline fixtures derived from local report locators. Include negative/overflow cases, map refill/revisit, character selector, item removal failure and save/reload. A parser-only pass does not satisfy this contract.

## F5 visible acceptance

Use real MM6 quest and interaction checkpoints to observe a variable comparison branch, a reward addition, item/gold subtraction success and failure, a condition/resistance change, a persistent map object state, a companion or reputation effect, and a timer/day gate. Save, quit, relaunch and load across at least one changed map byte and quest bit. Compare visible outcomes with the private source-located expectation. If a read-only diagnostic snapshot is added, use it to explain the view, not to replace it.

This packet feeds [the open F2 gate](MM6_F2_GATE_REVIEW.md) and [the draft F3 matrix](MM6_F3_PREFLIGHT_MATRIX.md). It is not authorization to run F3 before the full F2 freeze review.
