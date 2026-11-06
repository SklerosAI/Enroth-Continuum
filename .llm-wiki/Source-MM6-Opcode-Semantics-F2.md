---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, events, opcodes]
---

# MM6 Opcode Semantics F2 Source

The [active-opcode packet](../project/MM6_OPCODE_SEMANTICS_F2.md) and tracked [probe](../scripts/mm6_probe_opcode_semantics.py) register retained payload widths, current C++ parse/execute case presence, confidence and acceptance scenarios for all 43 opcode values across 68 active EVT files and 14,477 framed records. This is a complete **structural register**, not a completed behavior specification. A present switch case or bounded frame is not evidence that its MM6 side effect is correct; exact operands and offsets stay in the ignored local report.

Seven high-conflict opcode values need title-specific decoding or execution: an outdoor texture operation currently labeled as a movie, a shorter monster-spawn record, three values absent from inherited enum/cases, an outdoor facet candidate parsed as an actor flag, and a Question record whose inherited input-string path leaves bytes unread and asserts. The candidate item/password/answer and outdoor facet meanings require original/script-context checks. Variable compare/add/subtract/set frames have title-specific widths, but execution still forwards values into the inherited MM7 variable vocabulary. Active item, spell, map, NPC, house, timer and other side effects likewise need F4 and F5 checks.

The later [event execution audit](../project/MM6_EVENT_EXECUTION_SEAMS_F2.md) joins these failures to the whole-program loader: 318 rejected records occur in 26 of 67 listed map EVT resources, and `EvtProgram::load` aborts on the first malformed record. Those map event programs cannot fully load under the current parser. A separate zero-mastery CheckSkill branch cannot succeed under the inherited exact-mastery comparison. Both are source/data findings, not witnessed MM6 crashes or quest outcomes; F3 must confirm original operand semantics.

Authority: [project/MM6_OPCODE_SEMANTICS_F2.md](../project/MM6_OPCODE_SEMANTICS_F2.md), read with [event/campaign specification](../project/MM6_EVENT_CAMPAIGN_SPEC.md), [implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md) and the [F2 gate review](../project/MM6_F2_GATE_REVIEW.md). F2 remains open; the gate review does not authorize F3.

See also: [MM6 Event and Campaign Specification Source](Source-MM6-Event-Campaign-Spec.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
