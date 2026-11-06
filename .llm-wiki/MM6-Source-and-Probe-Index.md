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
- [F2 gate review](../project/MM6_F2_GATE_REVIEW.md) is the integration decision record. F2 is open and unfrozen; this review does not authorize F3.
- [F3 preflight matrix](../project/MM6_F3_PREFLIGHT_MATRIX.md) is a draft one-pass original-file question list, not F2 or F3 gate completion.
- [Local status](../project/LOCAL_STATUS.md) records observed Mac behavior and its limits.
- [OpenEnroth credit](../CREDITS.md) and [upstream relationship](../project/UPSTREAM.md) govern attribution.

## Focused F2 sources

| Question | Public specification |
| --- | --- |
| Manual/guide categories and campaign milestones | [Guide reconciliation](../project/MM6_GUIDE_RECONCILIATION.md) |
| Fourteen guide-family chapter/consumer ledger and remaining claim review | [Guide F2 review](../project/MM6_GUIDE_F2_REVIEW.md) |
| Class, skill, creation, inherited potion UI | [Class and skill](../project/MM6_CLASS_SKILL_SPEC.md) |
| Create Party, Quick Start, starting stats, items and spells | [Creation F2 audit](../project/MM6_CREATION_F2.md) |
| Starting-array schema lead, potion-guide reconciliation and rules acceptance | [Rules F2 closure packet](../project/MM6_RULES_F2_CLOSURE.md) |
| All 99 spell IDs, inherited effect and item-range collisions | [Spell effect F2 audit](../project/MM6_SPELL_EFFECT_F2.md) |
| Map deltas, reset, versioned save, persistence | [Save and map](../project/MM6_SAVE_SPEC.md) |
| Retained delta section roundtrip and world consumer obligations | [World F2 closure packet](../project/MM6_WORLD_F2_CLOSURE.md) |
| Record field hypotheses and chest delta-grid ambiguity | [World field map F2](../project/MM6_WORLD_FIELD_MAP_F2.md) |
| EVT, Question, NPC topics, structural campaign links | [Event and campaign](../project/MM6_EVENT_CAMPAIGN_SPEC.md) |
| All active EVT opcode widths, case presence, confidence and scenarios | [Opcode semantics F2 packet](../project/MM6_OPCODE_SEMANTICS_F2.md) |
| Campaign milestones, NPC interception, travel and finale seams | [Campaign F2 closure packet](../project/MM6_CAMPAIGN_F2_CLOSURE.md) |
| Movies, houses, audio, portraits, fonts, UI | [Media and UI](../project/MM6_MEDIA_UI_SPEC.md) |
| Portrait/icon/font/voice consumer name joins | [Media F2 closure packet](../project/MM6_MEDIA_F2_CLOSURE.md) |
| Text-table loader and no-literal-load questions, item matrix, NPC text | [Table consumers](../project/MM6_TABLE_CONSUMER_SPEC.md) |
| Thirty-family parser/consumer register and award-row mismatch | [Table semantic F2 packet](../project/MM6_TABLE_SEMANTIC_F2.md) |
| Monster parser labels, taxonomy, combat gaps | [Monster and combat](../project/MM6_MONSTER_COMBAT_SPEC.md) |
| Retained monster rows, attack/resistance/AI/loot consumer map | [Combat semantics F2](../project/MM6_COMBAT_SEMANTICS_F2.md) |

## Tracked probe entry points

| Evidence family | Reproducible scripts | What they can establish |
| --- | --- | --- |
| Archive/map structure | [map inventory](../scripts/mm6_probe_maps.py), [indoor](../scripts/mm6_probe_indoors.py), [outdoor](../scripts/mm6_probe_outdoors.py), [world roundtrip](../scripts/mm6_probe_world_roundtrip.py), [world fields](../scripts/mm6_probe_world_fields.py) | Bounds, field spans, byte-exact retained delta section reassembly and record hypotheses; chest grid and other nonzero field semantics remain unresolved. |
| Event structure | [event inventory](../scripts/mm6_probe_events.py), [parser comparison](../scripts/mm6_probe_event_parser.py), [opcode semantics](../scripts/mm6_probe_opcode_semantics.py), [variables](../scripts/mm6_probe_event_variables.py), [dialogue](../scripts/mm6_probe_dialogue_events.py), [quest graph](../scripts/mm6_probe_quest_graph.py), [campaign routes](../scripts/mm6_probe_campaign_routes.py) | All-active opcode frame widths and current-case register, plus structural routing links; not complete side effects or quest reachability. |
| Rules and tables | [creation rules](../scripts/mm6_probe_creation_rules.py), [item ranges](../scripts/mm6_probe_item_ranges.py), [spells](../scripts/mm6_probe_spells.py), [spell effects](../scripts/mm6_probe_spell_effects.py), [table consumers](../scripts/mm6_probe_table_consumers.py), [table semantics](../scripts/mm6_probe_table_semantics.py), [unloaded tables](../scripts/mm6_probe_unloaded_tables.py), [potion guide](../scripts/mm6_probe_potion_guide.py), [monster labels](../scripts/mm6_probe_monster_rules.py), [combat semantics](../scripts/mm6_probe_combat_semantics.py), [binary tables](../scripts/mm6_probe_binary_tables.py) | Starter-item/category, ID/schema, monster row/consumer, guide/table and parser comparisons; not correct creation, effects, awards, targeting or live combat. |
| Presentation | [media references](../scripts/mm6_probe_media_references.py), [media consumers](../scripts/mm6_probe_media_consumers.py), [house routes](../scripts/mm6_probe_house_routes.py), [strings](../scripts/mm6_probe_strings.py), [fonts](../scripts/mm6_probe_fonts.py) | Resource presence, format boundaries, portrait/icon/font/voice name joins and static house routing; not route reachability or seen/heard playback. |
| Owned guides | [guide coverage](../scripts/mm6_probe_guide_coverage.py) | PDF identity, page/text indexing and inspection flags; not complete claim-level review or runtime truth. |

Run each script only with its documented local ignored reference and output arguments. The scripts and specs can change; record their Git SHA and the private report's source fingerprints before trusting a result. A stale report must be regenerated or explicitly marked stale. Never stage a private report, raw table, decoded archive, guide text, save, screenshot or trace. A new probe belongs in this index only after its tracked version and output-boundary behavior have been reviewed.

For exact proprietary-derived rows or uncertainty decisions, open `local-game-data/private-wiki/INDEX.md` locally and follow its report registry. That path is ignored, local only, and **not backed up by Git**; protect it with an owner-approved local backup. It is not a GitHub Wiki or public source. The [private companion rules](Private-Companion-Wiki.md) define its entry contract; this tracked page remains safe to publish.

See also: [Agent Handoff](Agent-Handoff.md), [Engine Architecture and Runtime Map](Engine-Architecture-and-Runtime-Map.md), [Private Companion Wiki](Private-Companion-Wiki.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md).
