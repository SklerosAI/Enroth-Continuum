---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, events, opcodes]
---

# MM6 Opcode Side Effects F2 Source

The [43-opcode side-effect packet](../project/MM6_OPCODE_SIDE_EFFECTS_F2.md) extends the structural opcode register with candidate state targets, current C++ control flow, persistence fields and F4/F5 acceptance scenarios for every active MM6 opcode. The detailed representative retained locators remain ignored locally. This is an implementation contract, not proof of original side effects or a playable interpreter.

Seven high-conflict opcode values still need title-specific decoding or original-behavior checks: 12, 19, 20, 24, 26, 27 and 28. Typed MM6 variable dispatch is also required for Compare/Add/Subtract/Set. F2 is frozen as a question specification; the planned single F3 original-file preflight has not run.

Superseding F4 evidence: the [original no-op trace](MM6-F4-Original-Noop-Events.md) found that this version of the F3-matched executable advances through values 20, 27 and 28 without an effect. Current title-specific parsing also loads all 68 active EVT resources. The paragraph above remains the historical F2 question set, not the current implementation state.

See also: [MM6 Opcode Semantics F2 Source](Source-MM6-Opcode-Semantics-F2.md), [MM6 Variable Dispatch F2 Source](Source-MM6-Variable-Dispatch-F2.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md).
