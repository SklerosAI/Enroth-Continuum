---
type: analysis
up: "[[Agent-Handoff]]"
derived_from: ["[[Campaign-Status]]", "[[Runtime-Isolation]]", "[[MM6-F2-Structural-Findings]]"]
tags: [mm6, gaps, handoff]
---

# Open Questions

This is a dated, public gap register for the current MM6 F2 handoff, not a substitute for the full implementation checklist or private per-ID reports.

## Question

What must agents resolve before the consolidated MM6 F3 preflight can validate a complete F2 specification?

## Context

The active MM6 adapter reaches title/party screens but not a playable map. The new [F2 structural findings](MM6-F2-Structural-Findings.md) narrow several inherited-routing gaps; semantic behavior, campaign correctness, and save compatibility remain open. [Plan §18.1](../project/plan.md) requires F2 reconciliation before one deliberate F3 original-file review. The [MM7 audit](../project/MM7_AUDIT_CHECKLIST.md) is a separate planned effort.

## Analysis

| Owner lane | Open question (2026-11-05) | Source of gap and closure evidence |
| --- | --- | --- |
| A | What are the MM6 creation defaults, class formulas, spell/item effects and potion recipes? The inherited potion UI predicates miss MM6 herbs/bottles and gate mixing on an Alchemy skill absent from the MM6 table. | [Class/skill spec](../project/MM6_CLASS_SKILL_SPEC.md) and [class/skill source summary](Source-MM6-Class-Skill-Spec.md); reconcile the shipped matrices and acceptance scenarios without assuming MM7 behavior. |
| B | What do nonzero MM6 map-state bytes and the final delta tail mean, and how are they preserved through resets and `.ec6` saves? Baseline fixed regions are zero-filled, so their boundaries are stronger evidence than their field labels. | [Save spec](../project/MM6_SAVE_SPEC.md) and [save source summary](Source-MM6-Save-Spec.md); use nonzero synthetic round trips and later source checks. Native import/export remains unproven. |
| C | How do MM6 Question answer/cancel paths work, and how should labeled NPC topics without a current global event or special handler behave? Are other missing opcode/variable semantics and campaign links resolved? | [Event/campaign spec](../project/MM6_EVENT_CAMPAIGN_SPEC.md) and [event source summary](Source-MM6-Event-Campaign-Spec.md). The reproducible `scripts/mm6_probe_dialogue_events.py` reports structural routing only; unmatched topics are not confirmed original-game defects. |
| D | Which MM6 movie and house names, sound aliases, spell cues, portraits, fonts and UI screens should each consumer use? The inherited lookup and descriptor paths do not match retained indexes. | [Media/UI spec](../project/MM6_MEDIA_UI_SPEC.md) and [media source summary](Source-MM6-Media-UI-Spec.md); F3 checks raw archive names, F5 checks seen/heard playback. Music file presence alone is not playback. |
| Integrator | Has every item, spell, map, event, NPC, media, UI, save, and ending family been reconciled, with contradictions and uncertainty explicit? | Consolidate A–D into [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md); only then define the F3 read-only preflight sample and acceptance record. |

MM7's full resource/rules/campaign audit, MM8's engine adapter, and the title-aware Atlas assembler remain separate later work. None is evidenced complete by the MM6 F2 probes. The Atlas currently has only MM7 Play; adding another action requires that game's own play/save gate.

## Conclusion

F2 is incomplete. Agents should close the scoped evidence gaps and hand them to the integrator. The original-data F3 preflight and full F4 implementation follow a reviewed, consolidated F2 specification.

## Open follow-ups

- Recheck this page against new checklist and status commits before assigning lanes; remove a question only when its closure evidence is linked.
- At F3, record discrepancies, checksums, and raw findings only in ignored local storage; update this public page with conclusions and remaining uncertainty.
- Keep the protected MM7 baseline available while planning its separate M7-A0–A5 audit.

See also: [Agent Handoff](Agent-Handoff.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md), [Campaign Status](Campaign-Status.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
