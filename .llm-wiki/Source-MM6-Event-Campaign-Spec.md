---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, events, dialogue]
---

# MM6 Event and Campaign Specification Source

This F2 source links retained MM6 event structure, current parser/interpreter consumers, dialogue routing, and campaign obligations.

The retained Question instruction has more operands than the inherited input-string parser consumes, and its interpreter path is unfinished. Other active opcode families also lack title-specific parsing or proved side effects. A retained NPC-topic comparison finds labeled references without a current global event or special handler; they could be intentional text-only choices or missing MM6 behavior, not confirmed original-game defects. The tracked [dialogue probe](../scripts/mm6_probe_dialogue_events.py) reproduces aggregate structural findings and writes its detailed report only to ignored local storage. MM6 variable meanings conflict with inherited MM7 ranges.

The repeatable [campaign-graph probe](../scripts/mm6_probe_quest_graph.py) joins retained map, house, event, dialogue and quest-note indexes. Explicit script map targets and house exits resolve to listed maps; some origin labels and one-sided quest-bit references remain unclassified. Guide milestone matches are candidates, not event-control flow or a playable route. The graph does not prove quest or ending reachability.

The later [campaign closure packet](../project/MM6_CAMPAIGN_F2_CLOSURE.md) and [route probe](../scripts/mm6_probe_campaign_routes.py) identify title-routing seams: six of 21 typed transport house IDs are outside the inherited route array, 74 NPC-topic references with global event functions fall into an inherited teacher-special range, and two active Hive pseudo-house instructions enter MM7 finale code by current source control flow. These are static risks, not observed crashes, complete quest dependencies or a verified MM6 ending.

Authority: [project/MM6_EVENT_CAMPAIGN_SPEC.md](../project/MM6_EVENT_CAMPAIGN_SPEC.md), reviewed 2026-11-05. Keep detailed topic IDs, script offsets, strings and graph edges in ignored local reports.

See also: [MM6 Campaign F2 Closure Source](Source-MM6-Campaign-F2-Closure.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md).
