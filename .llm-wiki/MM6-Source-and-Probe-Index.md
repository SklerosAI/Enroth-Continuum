---
type: index
up: "[[Agent-Handoff]]"
tags: [mm6, sources, probes]
---

# MM6 Source and Probe Index

This index directs agents to the current public specifications and reproducible offline probes; detailed outputs and original game material stay in the ignored private companion.

## Authority before a probe

- [Plan §18.1](../project/plan.md) defines F0–F6 gates and the one deliberate F3 original-data preflight.
- [MM6 implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md) is the public coverage and gap register; its checked research rows are not playable-rule claims.
- [Local status](../project/LOCAL_STATUS.md) records observed Mac behavior and its limits.
- [OpenEnroth credit](../CREDITS.md) and [upstream relationship](../project/UPSTREAM.md) govern attribution.

## Focused F2 sources

| Question | Public specification |
| --- | --- |
| Manual/guide categories and campaign milestones | [Guide reconciliation](../project/MM6_GUIDE_RECONCILIATION.md) |
| Class, skill, creation, inherited potion UI | [Class and skill](../project/MM6_CLASS_SKILL_SPEC.md) |
| Map deltas, reset, versioned save, persistence | [Save and map](../project/MM6_SAVE_SPEC.md) |
| EVT, Question, NPC topics, structural campaign links | [Event and campaign](../project/MM6_EVENT_CAMPAIGN_SPEC.md) |
| Movies, houses, audio, portraits, fonts, UI | [Media and UI](../project/MM6_MEDIA_UI_SPEC.md) |
| Text-table loader and no-literal-load questions, item matrix, NPC text | [Table consumers](../project/MM6_TABLE_CONSUMER_SPEC.md) |
| Monster parser labels, taxonomy, combat gaps | [Monster and combat](../project/MM6_MONSTER_COMBAT_SPEC.md) |

## Tracked probe entry points

| Evidence family | Reproducible scripts | What they can establish |
| --- | --- | --- |
| Archive/map structure | [map inventory](../scripts/mm6_probe_maps.py), [indoor](../scripts/mm6_probe_indoors.py), [outdoor](../scripts/mm6_probe_outdoors.py) | Bounds, field spans, references and anomaly candidates; not live map behavior. |
| Event structure | [event inventory](../scripts/mm6_probe_events.py), [parser comparison](../scripts/mm6_probe_event_parser.py), [variables](../scripts/mm6_probe_event_variables.py), [dialogue](../scripts/mm6_probe_dialogue_events.py), [quest graph](../scripts/mm6_probe_quest_graph.py) | Framing, literal routing and structural graph links; not full opcode semantics or quest reachability. |
| Rules and tables | [item ranges](../scripts/mm6_probe_item_ranges.py), [spells](../scripts/mm6_probe_spells.py), [table consumers](../scripts/mm6_probe_table_consumers.py), [unloaded tables](../scripts/mm6_probe_unloaded_tables.py), [monster labels](../scripts/mm6_probe_monster_rules.py), [binary tables](../scripts/mm6_probe_binary_tables.py) | ID/schema and current parser-consumer comparisons; not correct effects or combat. |
| Presentation | [media references](../scripts/mm6_probe_media_references.py), [strings](../scripts/mm6_probe_strings.py), [fonts](../scripts/mm6_probe_fonts.py) | Resource presence, format boundaries and name links; not seen or heard playback. |

Run each script only with its documented local ignored reference and output arguments. The scripts and specs can change; record their Git SHA and the private report's source fingerprints before trusting a result. A stale report must be regenerated or explicitly marked stale. Never stage a private report, raw table, decoded archive, guide text, save, screenshot or trace. A new probe belongs in this index only after its tracked version and output-boundary behavior have been reviewed.

For exact proprietary-derived rows or uncertainty decisions, open `local-game-data/private-wiki/INDEX.md` locally and follow its report registry. That path is ignored, local only, and **not backed up by Git**; protect it with an owner-approved local backup. It is not a GitHub Wiki or public source. The [private companion rules](Private-Companion-Wiki.md) define its entry contract; this tracked page remains safe to publish.

See also: [Agent Handoff](Agent-Handoff.md), [Engine Architecture and Runtime Map](Engine-Architecture-and-Runtime-Map.md), [Private Companion Wiki](Private-Companion-Wiki.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md).
