---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, monsters, combat]
---

# MM6 Combat Semantics F2 Source

The [combat semantics packet](../project/MM6_COMBAT_SEMANTICS_F2.md) and tracked [probe](../scripts/mm6_probe_combat_semantics.py) join all 173 retained monster rows to current C++ parser and consumer paths, with per-row detail in an ignored report. The rows form 57 complete numeric triplets plus a two-row tail; that pattern is a candidate family grouping, not verified identity or spawn mapping. Active labels expose deterministic damage, projectile, spell and special-attack fallbacks. The source also shows inherited MM7 family IDs, resistance aliases, friendly monster-to-monster default hostility and fixed-drop routing; guide descriptions are behavior leads, not measured combat.

The packet gives F4 contracts for title-specific identity, exhaustive active label mapping, six-channel resistance with unresolved Energy behavior, AI/faction/recovery and loot/persistence. F5 requires visible encounters, status/audio checks, both timing modes, save/re-entry and no loot reroll. No monster encounter, damage formula, projectile hit, AI threshold or drop has been verified in a running MM6 map. Exact labels, row IDs, guide observations and source hashes remain ignored locally.

Authority: [project/MM6_COMBAT_SEMANTICS_F2.md](../project/MM6_COMBAT_SEMANTICS_F2.md), read with [monster/combat specification](../project/MM6_MONSTER_COMBAT_SPEC.md), [spell effect audit](../project/MM6_SPELL_EFFECT_F2.md), [world field map](../project/MM6_WORLD_FIELD_MAP_F2.md), and [F2 gate review](../project/MM6_F2_GATE_REVIEW.md). This packet does not close F2 or authorize F3.

See also: [MM6 Monster and Combat Specification Source](Source-MM6-Monster-Combat-Spec.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
