---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, monsters, combat]
---

# MM6 Combat Semantics F2 Source

The [combat semantics packet](../project/MM6_COMBAT_SEMANTICS_F2.md) and tracked [probe](../scripts/mm6_probe_combat_semantics.py) join all 173 retained monster rows to current C++ parser and consumer paths, with per-row detail in an ignored report. The rows form 57 complete numeric triplets plus a two-row tail; that pattern is a candidate family grouping, not verified identity or spawn mapping. Active labels expose deterministic damage, projectile, spell and special-attack fallbacks. The source also shows inherited MM7 family IDs, resistance aliases, friendly monster-to-monster default hostility and fixed-drop routing; guide descriptions are behavior leads, not measured combat.

The packet gives F4 contracts for title-specific identity, exhaustive active label mapping, six-channel resistance with unresolved Energy behavior, AI/faction/recovery and loot/persistence. F5 requires visible encounters, status/audio checks, both timing modes, save/re-entry and no loot reroll. No monster encounter, damage formula, projectile hit, AI threshold or drop has been verified in a running MM6 map. Exact labels, row IDs, guide observations and source hashes remain ignored locally.

The separate [description and spawn-link packet](../project/MM6_MONSTER_SPAWN_LINKS_F2.md) joins all 173 retained description names to text rows and 1,382 of 1,384 animation names to the extended sprite index. A retained baseline actor carries candidate ID 173 under a pinned external MM6 record layout, while the current MM6 loader drops description 173. This is a source/data mismatch to confirm at F3 and repair offline at F4; it is not a live rendering or combat result. Actual textures and scripted spawns remain unjoined.

The [monster-effect packet](../project/MM6_MONSTER_EFFECT_SEAMS_F2.md) adds all-173 target/special/loot source-risk joins. Six guide-checked male-target preferences reach an inherited Monk predicate; 26 numeric multi-target preferences are parsed but have no runtime reader. Three weapon-break rows reach an inverted eligibility guard, and 12 inherited fixed-drop family rows would use item IDs in MM6's spell-scroll band. Original rules and visible encounter outcomes remain unverified; exact per-ID joins stay ignored locally.

Authority: [project/MM6_COMBAT_SEMANTICS_F2.md](../project/MM6_COMBAT_SEMANTICS_F2.md), read with [description/spawn links](../project/MM6_MONSTER_SPAWN_LINKS_F2.md), [monster/combat specification](../project/MM6_MONSTER_COMBAT_SPEC.md), [spell effect audit](../project/MM6_SPELL_EFFECT_F2.md), [world field map](../project/MM6_WORLD_FIELD_MAP_F2.md), and [F2 gate review](../project/MM6_F2_GATE_REVIEW.md). This packet does not close F2 or authorize F3.

See also: [MM6 Monster and Combat Specification Source](Source-MM6-Monster-Combat-Spec.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
