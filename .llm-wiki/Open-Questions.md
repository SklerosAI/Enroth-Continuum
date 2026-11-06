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
| A | What are the MM6 creation defaults, class formulas, spell/item effects and potion recipes? A retained `useitems.txt` item-combination matrix is structurally complete and symmetric, but its result tokens and original behavior are unknown; the inherited potion UI still uses MM7 rules. | [Class/skill spec](../project/MM6_CLASS_SKILL_SPEC.md), [table-consumer spec](../project/MM6_TABLE_CONSUMER_SPEC.md), and their source summaries; reconcile token meanings and acceptance scenarios without assuming MM7 behavior. |
| A | What are MM6 monster damage, projectile, spell, special, resistance, faction and loot meanings? Active labels can silently fall through shared parsers; no combat behavior has been observed. | [Monster/combat spec](../project/MM6_MONSTER_COMBAT_SPEC.md) and [source summary](Source-MM6-Monster-Combat-Spec.md); map labels to title-specific consumers and later offline/live checks. |
| B | What do nonzero MM6 map-state bytes and the final delta tail mean, and how are they preserved through resets and `.ec6` saves? Baseline fixed regions are zero-filled, so their boundaries are stronger evidence than their field labels. | [Save spec](../project/MM6_SAVE_SPEC.md) and [save source summary](Source-MM6-Save-Spec.md); use nonzero synthetic round trips and later source checks. Native import/export remains unproven. |
| C | How do MM6 Question answer/cancel paths work, and how should labeled NPC topics without a current global event or special handler behave? The structural graph resolves explicit map links but not one-sided quest-bit meanings, branching or ending reachability. | [Event/campaign spec](../project/MM6_EVENT_CAMPAIGN_SPEC.md), [event source summary](Source-MM6-Event-Campaign-Spec.md), and [graph probe](../scripts/mm6_probe_quest_graph.py). Structural routing is not a confirmed original-game defect or playable campaign. |
| C/D | Which source supplies MM6 NPC dismissal and other profession dialogue? The current initializer leaves a field unset that the UI reads. | [Table-consumer spec](../project/MM6_TABLE_CONSUMER_SPEC.md) and [source summary](Source-MM6-Table-Consumer-Spec.md); trace source and verify displayed behavior before declaring a defect. |
| D | Which MM6 movie and house names, sound aliases, spell cues, portraits, fonts and UI screens should each consumer use? The inherited lookup and descriptor paths do not match retained indexes. | [Media/UI spec](../project/MM6_MEDIA_UI_SPEC.md) and [media source summary](Source-MM6-Media-UI-Spec.md); F3 checks raw archive names, F5 checks seen/heard playback. Music file presence alone is not playback. |
| Across A–D | Which retained text tables are parsed with the correct MM6 schema and actually consumed by the intended title-specific behavior? A static scan finds literal C++ loads for 24 of 30 table names and no literal load for six; neither result settles semantics or non-use. | [Table-consumer spec](../project/MM6_TABLE_CONSUMER_SPEC.md) and [source summary](Source-MM6-Table-Consumer-Spec.md); trace indirect loads and downstream consumers, keeping the detailed report ignored locally. |
| Integrator | Has every item, spell, map, event, NPC, media, UI, save, and ending family been reconciled, with contradictions and uncertainty explicit? | Consolidate A–D into [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md); only then define the F3 read-only preflight sample and acceptance record. |

MM7's full resource/rules/campaign audit, MM8's engine adapter, and the title-aware Atlas assembler remain separate later work. None is evidenced complete by the MM6 F2 probes. The Atlas currently has only MM7 Play; adding another action requires that game's own play/save gate.

## Conclusion

F2 is incomplete. Agents should close the scoped evidence gaps and hand them to the integrator. The original-data F3 preflight and full F4 implementation follow a reviewed, consolidated F2 specification.

## Open follow-ups

- Recheck this page against new checklist and status commits before assigning lanes; remove a question only when its closure evidence is linked.
- At F3, record discrepancies, checksums, and raw findings only in ignored local storage; update this public page with conclusions and remaining uncertainty.
- Keep the protected MM7 baseline available while planning its separate M7-A0–A5 audit.

See also: [Agent Handoff](Agent-Handoff.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md), [Campaign Status](Campaign-Status.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
