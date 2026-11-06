---
type: synthesis
up: "[[Agent-Handoff]]"
tags: [mm6, f2, gaps]
---

# MM6 F2 Structural Findings

The current F2 reviews identify concrete MM7-to-MM6 routing gaps, while leaving exact game behavior and the F3 original-file preflight unresolved.

| Area | Confirmed structural finding | Still unproven |
| --- | --- | --- |
| Party and potion UI | MM6 class/skill order and creation rules differ from the inherited MM7 tables. The current potion interaction is gated by MM7 item categories and an Alchemy skill absent from MM6's skill table. A retained `useitems.txt` table has a complete symmetric two-axis matrix in the herb/bottle band, with no MM6 loader. | Exact creation constants, Quick Start state, matrix token meanings, potion recipes, failure effects, and live UI behavior. |
| Map state and saves | Retained MM6 deltas have title-specific boundaries. Their fixed opening, persistent region, and tail are zero-filled in baseline data; inherited MM7 save/reset paths do not preserve or interpret them safely for MM6. | Nonzero field meanings, reset rules, lossless round trips, and a working versioned MM6 save. Native `.mm6` compatibility is not established. |
| Events and dialogue | Retained MM6 Question instructions contain operands the inherited parser and interpreter do not handle. Some labeled NPC topics lack both a current global event and a current special handler. A structural graph links retained map, house, event, dialogue and quest-note indexes; explicit script map targets and house exits resolve to listed maps. | Answer/cancel branches, whether unmatched topics are text-only or missing behavior, one-sided quest-bit meanings, and actual quest/ending reachability. This is not proof of an original-game defect. |
| NPC text | The MM6 initializer leaves a dismissal-text field unset while the dialogue UI reads it. | Which MM6 table supplies the text and whether a player sees blank dismissal dialogue; source inspection alone cannot establish the original behavior. |
| Monsters and combat | Retained monster records have bounded structural coverage, but active attack, projectile, spell and special labels can silently fall through shared MM7 parsers. Family, hostility, resistance and loot paths retain title assumptions or temporary defaults. | MM6-specific taxonomy and behavior; no encounter, damage, projectile, AI or drop has been observed in MM6 play. |
| Media and presentation | Retained movie names and inherited suffix-based lookup disagree; MM7 house descriptors do not map to MM6 video names. Sound-table alias/missing-name policy and spell sound routing remain unresolved. Installed map music files have been found. | Raw VID/SND directory details at F3, MM6-specific mappings, and actual video/audio playback at F5. |
| Text-table consumers | A static scan finds direct literal C++ loads for 24 of 30 retained text-table names; six have no literal load. | Correct parsing, complete downstream use, and whether the six have indirect consumers or are unused remain unproved. |

These are **source and retained-index findings**, not implemented fixes. The active MM6 adapter remains unplayable beyond title/party screens. [MM6 implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md) is the public coverage authority; detailed per-record evidence stays in ignored local reports. Do not infer an item or event is implemented because its table row or instruction parses.

Sources reviewed 2026-11-05: [class/skill spec](../project/MM6_CLASS_SKILL_SPEC.md), [save spec](../project/MM6_SAVE_SPEC.md), [event/campaign spec](../project/MM6_EVENT_CAMPAIGN_SPEC.md), [media/UI spec](../project/MM6_MEDIA_UI_SPEC.md), [table-consumer spec](../project/MM6_TABLE_CONSUMER_SPEC.md), [monster/combat spec](../project/MM6_MONSTER_COMBAT_SPEC.md), [dialogue probe](../scripts/mm6_probe_dialogue_events.py), [campaign-graph probe](../scripts/mm6_probe_quest_graph.py), and [local status](../project/LOCAL_STATUS.md).

See also: [Agent Handoff](Agent-Handoff.md), [Open Questions](Open-Questions.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
