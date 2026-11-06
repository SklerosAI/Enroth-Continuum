---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, events, variables]
---

# MM6 Variable Dispatch F2 Source

The [variable-dispatch packet](../project/MM6_VARIABLE_DISPATCH_F2.md) maps the 127 variable IDs used by 4,285 retained MM6 Compare/Add/Subtract/Set event operands to current parser, interpreter, executor and save obligations. Its [probe](../scripts/mm6_probe_event_variables.py) now fingerprints all 68 active EVT inputs and source consumers; exact operands and event offsets remain in the ignored local report.

The 1,609 operands in MM6-specific ranges cannot be interpreted from the inherited MM7 enum alone. Resistance, skill, condition, 100 map-byte and party-state ranges include substantial title collisions. The public packet defines typed MM6 dispatch, value validation, branch behavior, persistence and F4/F5 scenarios. Editor and MMExtension names are leads, not proof of the original executable's effects. F2 is frozen for F3; original effects remain unverified.

See also: [MM6 Opcode Semantics F2 Source](Source-MM6-Opcode-Semantics-F2.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Agent Handoff](Agent-Handoff.md).
