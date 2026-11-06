---
type: analysis
up: "[[Agent-Handoff]]"
derived_from: ["[[Campaign-Status]]", "[[Runtime-Isolation]]"]
tags: [mm6, gaps, handoff]
---

# Open Questions

This is a dated, public gap register for the current MM6 F2 handoff, not a substitute for the full implementation checklist or private per-ID reports.

## Question

What must agents resolve before the consolidated MM6 F3 preflight can validate a complete F2 specification?

## Context

The active MM6 adapter reaches title/party screens but not a playable map. Private offline probes established broad structural coverage; semantic behavior, campaign correctness, and save compatibility remain open. [Plan §18.1](../project/plan.md) requires F2 reconciliation before one deliberate F3 original-file review. The [MM7 audit](../project/MM7_AUDIT_CHECKLIST.md) is a separate planned effort.

## Analysis

| Owner lane | Open question (2026-11-05) | Source of gap and closure evidence |
| --- | --- | --- |
| A | Which exact MM6 class/skill, spell, item, mastery, trainer, potion, and combat semantics differ from inherited MM7 behavior? | [Class/skill spec](../project/MM6_CLASS_SKILL_SPEC.md) and [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md); link every shipped row family to a consumer and acceptance scenario. |
| B | How do MM6 indoor/outdoor deltas, unusual base-map compression cases, 200 persistent bytes, chests/doors, and a versioned `.ec6` save preserve state? Is any native MM6 save format evidence available? | [Save spec](../project/MM6_SAVE_SPEC.md); validate field meanings, title boundaries, and round trips before an F4 serializer. Native import/export remains unproven. |
| C | What are the semantics of the MM6-specific event opcodes and variable ranges, dialogue/input behavior, and the full quest chain to an ending? | [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md) and [guide reconciliation](../project/MM6_GUIDE_RECONCILIATION.md); trace scripts, text, map links, interpreter consumers, and representative branch scenarios. |
| D | Which MM6 portraits, UI layouts, house animations, movies, sound aliases, and music cues map to actual resources and behavior on this Mac? | [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md) and [local status](../project/LOCAL_STATUS.md); resolve resource references offline, reserve graphical and audible confirmation for F5. |
| Integrator | Has every item, spell, map, event, NPC, media, UI, save, and ending family been reconciled, with contradictions and uncertainty explicit? | Consolidate A–D into [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md); only then define the F3 read-only preflight sample and acceptance record. |

MM7's full resource/rules/campaign audit, MM8's engine adapter, and the title-aware Atlas assembler remain separate later work. None is evidenced complete by the MM6 F2 probes. The Atlas currently has only MM7 Play; adding another action requires that game's own play/save gate.

## Conclusion

F2 is incomplete. Agents should close the scoped evidence gaps and hand them to the integrator. The original-data F3 preflight and full F4 implementation follow a reviewed, consolidated F2 specification.

## Open follow-ups

- Recheck this page against new checklist and status commits before assigning lanes; remove a question only when its closure evidence is linked.
- At F3, record discrepancies, checksums, and raw findings only in ignored local storage; update this public page with conclusions and remaining uncertainty.
- Keep the protected MM7 baseline available while planning its separate M7-A0–A5 audit.

See also: [Agent Handoff](Agent-Handoff.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md), [Campaign Status](Campaign-Status.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
