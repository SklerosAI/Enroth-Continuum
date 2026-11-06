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
| A | What are exact MM6 starting-stat/skill arrays, Quick Start defaults, class formulas, spell/item effects and potion outcomes? A primary reference supplies array schemas, not values. The retained item matrix and owned guide match 25 numeric-result input pairs but disagree on two results; marker meanings and original behavior remain unknown. | [Rules closure](../project/MM6_RULES_F2_CLOSURE.md), [class/skill spec](../project/MM6_CLASS_SKILL_SPEC.md), [table-consumer spec](../project/MM6_TABLE_CONSUMER_SPEC.md), and [rules summary](Source-MM6-Rules-F2-Closure.md); retain exact recipes privately and settle conflicts in F3. |
| A | What are MM6 monster damage, projectile, spell, special, resistance, faction and loot meanings? Active labels can silently fall through shared parsers; no combat behavior has been observed. | [Monster/combat spec](../project/MM6_MONSTER_COMBAT_SPEC.md) and [source summary](Source-MM6-Monster-Combat-Spec.md); map labels to title-specific consumers and later offline/live checks. |
| A | How should all 99 MM6 spells map to effects, target types, three mastery ranks, costs, books, scrolls, event casts and monster casts? Inherited static spell data and item ranges differ; four item-target numeric IDs have different MM6 labels. | [Spell effect audit](../project/MM6_SPELL_EFFECT_F2.md), [spell source summary](Source-MM6-Spell-Effect-F2.md) and [probe](../scripts/mm6_probe_spell_effects.py); classify semantics before F4 offline coverage and F5 representative casts. |
| B | What do nonzero MM6 map-state bytes and the final delta tail mean, and how are they preserved through resets and `.ec6` saves? Baseline fixed regions are zero-filled. The offline world probe preserves synthetic sentinels byte for byte, but C++ decode, reconstruction, reset and save/reload are unverified. | [Save spec](../project/MM6_SAVE_SPEC.md), [world closure packet](../project/MM6_WORLD_F2_CLOSURE.md), and [closure summary](Source-MM6-World-F2-Closure.md); extend from Python wire sections to C++ mutation/roundtrip and then F5 transition/save checks. Native import/export remains unproven. |
| C | How do MM6 Question answer/cancel paths work, and how should labeled NPC topics without a current global event or special handler behave? The structural graph resolves explicit map links but not one-sided quest-bit meanings, branching or ending reachability. | [Event/campaign spec](../project/MM6_EVENT_CAMPAIGN_SPEC.md), [event source summary](Source-MM6-Event-Campaign-Spec.md), and [graph probe](../scripts/mm6_probe_quest_graph.py). Structural routing is not a confirmed original-game defect or playable campaign. |
| C | Which MM6 NPC topics should invoke global functions when their IDs overlap the inherited teacher-special range, and what are the complete travel schedules and MM6 finale transitions? Static probes expose 74 topic references, six stable IDs outside the route array and two Hive pseudo-house entries into inherited MM7 game-over control flow. | [Campaign closure](../project/MM6_CAMPAIGN_F2_CLOSURE.md), [route probe](../scripts/mm6_probe_campaign_routes.py), and [campaign summary](Source-MM6-Campaign-F2-Closure.md); determine intended routing and F5 acceptance without claiming those paths are reachable now. |
| C/D | Which source supplies MM6 NPC dismissal and other profession dialogue? The current initializer leaves a field unset that the UI reads. | [Table-consumer spec](../project/MM6_TABLE_CONSUMER_SPEC.md) and [source summary](Source-MM6-Table-Consumer-Spec.md); trace source and verify displayed behavior before declaring a defect. |
| D | Which MM6 movie and house names, sound aliases, spell cues, portraits, fonts and UI screens should each consumer use? The inherited lookup and descriptor paths do not match retained indexes. | [Media/UI spec](../project/MM6_MEDIA_UI_SPEC.md) and [media source summary](Source-MM6-Media-UI-Spec.md); F3 checks raw archive names, F5 checks seen/heard playback. Music file presence alone is not playback. |
| D | Which original archive and naming scheme supply MM6 portraits, class icons and voices? Inherited portrait names miss the retained icon index; generated voice-ID gaps are only possible-ID comparisons. | [Media closure packet](../project/MM6_MEDIA_F2_CLOSURE.md), [consumer probe](../scripts/mm6_probe_media_consumers.py), and [media closure summary](Source-MM6-Media-F2-Closure.md); verify source directories once at F3 and visible/audible behavior at F5, including vanilla assets before overrides. |
| C/D | Which service and UI should each active MM6 `SpeakInHouse` target open? The house-route probe finds 384 table-backed active targets whose nonempty type labels fall through the current parser to an inherited mercenary-guild UI. Some room IDs span multiple labels, and shop/guild helpers use MM7 numeric ranges. | [Media/UI spec](../project/MM6_MEDIA_UI_SPEC.md), [house-route probe](../scripts/mm6_probe_house_routes.py) and [media source summary](Source-MM6-Media-UI-Spec.md); classify intended MM6 house services and test routing before claiming a reachable crash or original-game defect. |
| Across A–D | Which retained text tables are parsed with the correct MM6 schema and actually consumed by the intended title-specific behavior? A static scan finds literal C++ loads for 24 of 30 table names and no literal load for six; neither result settles semantics or non-use. | [Table-consumer spec](../project/MM6_TABLE_CONSUMER_SPEC.md) and [source summary](Source-MM6-Table-Consumer-Spec.md); trace indirect loads and downstream consumers, keeping the detailed report ignored locally. |
| Across A–C | What default ordering and text behavior should MM6 awards use? The current loader skips all 100 numeric rows because their priority cell is empty; selected cross-table joins prove reference presence only. | [Table semantic packet](../project/MM6_TABLE_SEMANTIC_F2.md), [table source summary](Source-MM6-Table-Semantic-F2.md), and [probe](../scripts/mm6_probe_table_semantics.py); define a title-specific parser and verify award writes/display/persistence without inventing a priority. |
| Integrator | Has every item, spell, map, event, NPC, media, UI, save, and ending family been reconciled, with contradictions and uncertainty explicit? | Consolidate A–D into [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md); only then define the F3 read-only preflight sample and acceptance record. |

MM7's full resource/rules/campaign audit, MM8's engine adapter, and the title-aware Atlas assembler remain separate later work. None is evidenced complete by the MM6 F2 probes. The Atlas currently has only MM7 Play; adding another action requires that game's own play/save gate.

## Conclusion

F2 is incomplete. Agents should close the scoped evidence gaps and hand them to the integrator. The original-data F3 preflight and full F4 implementation follow a reviewed, consolidated F2 specification.

## Open follow-ups

- Recheck this page against new checklist and status commits before assigning lanes; remove a question only when its closure evidence is linked.
- At F3, record discrepancies, checksums, and raw findings only in ignored local storage; update this public page with conclusions and remaining uncertainty.
- Keep the protected MM7 baseline available while planning its separate M7-A0–A5 audit.

See also: [Agent Handoff](Agent-Handoff.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md), [Campaign Status](Campaign-Status.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
