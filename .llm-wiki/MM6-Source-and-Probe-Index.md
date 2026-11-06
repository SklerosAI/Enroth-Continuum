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
- [F3 preflight matrix](../project/MM6_F3_PREFLIGHT_MATRIX.md) is a draft one-pass original-file question list, not F2 or F3 gate completion.
- [Local status](../project/LOCAL_STATUS.md) records observed Mac behavior and its limits.
- [OpenEnroth credit](../CREDITS.md) and [upstream relationship](../project/UPSTREAM.md) govern attribution.

## Focused F2 sources

| Question | Public specification |
| --- | --- |
| Manual/guide categories and campaign milestones | [Guide reconciliation](../project/MM6_GUIDE_RECONCILIATION.md) |
| Class, skill, creation, inherited potion UI | [Class and skill](../project/MM6_CLASS_SKILL_SPEC.md) |
| Starting-array schema lead, potion-guide reconciliation and rules acceptance | [Rules F2 closure packet](../project/MM6_RULES_F2_CLOSURE.md) |
| All 99 spell IDs, inherited effect and item-range collisions | [Spell effect F2 audit](../project/MM6_SPELL_EFFECT_F2.md) |
| Map deltas, reset, versioned save, persistence | [Save and map](../project/MM6_SAVE_SPEC.md) |
| Retained delta section roundtrip and world consumer obligations | [World F2 closure packet](../project/MM6_WORLD_F2_CLOSURE.md) |
| EVT, Question, NPC topics, structural campaign links | [Event and campaign](../project/MM6_EVENT_CAMPAIGN_SPEC.md) |
| Campaign milestones, NPC interception, travel and finale seams | [Campaign F2 closure packet](../project/MM6_CAMPAIGN_F2_CLOSURE.md) |
| Movies, houses, audio, portraits, fonts, UI | [Media and UI](../project/MM6_MEDIA_UI_SPEC.md) |
| Portrait/icon/font/voice consumer name joins | [Media F2 closure packet](../project/MM6_MEDIA_F2_CLOSURE.md) |
| Text-table loader and no-literal-load questions, item matrix, NPC text | [Table consumers](../project/MM6_TABLE_CONSUMER_SPEC.md) |
| Thirty-family parser/consumer register and award-row mismatch | [Table semantic F2 packet](../project/MM6_TABLE_SEMANTIC_F2.md) |
| Monster parser labels, taxonomy, combat gaps | [Monster and combat](../project/MM6_MONSTER_COMBAT_SPEC.md) |

## Tracked probe entry points

| Evidence family | Reproducible scripts | What they can establish |
| --- | --- | --- |
| Archive/map structure | [map inventory](../scripts/mm6_probe_maps.py), [indoor](../scripts/mm6_probe_indoors.py), [outdoor](../scripts/mm6_probe_outdoors.py), [world roundtrip](../scripts/mm6_probe_world_roundtrip.py) | Bounds, field spans and byte-exact retained delta section reassembly with synthetic sentinels; not field semantics or C++ map behavior. |
| Event structure | [event inventory](../scripts/mm6_probe_events.py), [parser comparison](../scripts/mm6_probe_event_parser.py), [variables](../scripts/mm6_probe_event_variables.py), [dialogue](../scripts/mm6_probe_dialogue_events.py), [quest graph](../scripts/mm6_probe_quest_graph.py), [campaign routes](../scripts/mm6_probe_campaign_routes.py) | Framing, literal routing and structural graph/transport/finale links; not full opcode semantics or quest reachability. |
| Rules and tables | [item ranges](../scripts/mm6_probe_item_ranges.py), [spells](../scripts/mm6_probe_spells.py), [spell effects](../scripts/mm6_probe_spell_effects.py), [table consumers](../scripts/mm6_probe_table_consumers.py), [table semantics](../scripts/mm6_probe_table_semantics.py), [unloaded tables](../scripts/mm6_probe_unloaded_tables.py), [potion guide](../scripts/mm6_probe_potion_guide.py), [monster labels](../scripts/mm6_probe_monster_rules.py), [binary tables](../scripts/mm6_probe_binary_tables.py) | ID/schema, guide/table disagreement and current parser-consumer comparisons; not correct effects, awards, targeting or combat. |
| Presentation | [media references](../scripts/mm6_probe_media_references.py), [media consumers](../scripts/mm6_probe_media_consumers.py), [house routes](../scripts/mm6_probe_house_routes.py), [strings](../scripts/mm6_probe_strings.py), [fonts](../scripts/mm6_probe_fonts.py) | Resource presence, format boundaries, portrait/icon/font/voice name joins and static house routing; not route reachability or seen/heard playback. |

Run each script only with its documented local ignored reference and output arguments. The scripts and specs can change; record their Git SHA and the private report's source fingerprints before trusting a result. A stale report must be regenerated or explicitly marked stale. Never stage a private report, raw table, decoded archive, guide text, save, screenshot or trace. A new probe belongs in this index only after its tracked version and output-boundary behavior have been reviewed.

For exact proprietary-derived rows or uncertainty decisions, open `local-game-data/private-wiki/INDEX.md` locally and follow its report registry. That path is ignored, local only, and **not backed up by Git**; protect it with an owner-approved local backup. It is not a GitHub Wiki or public source. The [private companion rules](Private-Companion-Wiki.md) define its entry contract; this tracked page remains safe to publish.

See also: [Agent Handoff](Agent-Handoff.md), [Engine Architecture and Runtime Map](Engine-Architecture-and-Runtime-Map.md), [Private Companion Wiki](Private-Companion-Wiki.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md).
