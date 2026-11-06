---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, monsters, combat]
---

# MM6 Monster and Combat Specification Source

This F2 source compares retained MM6 monster tables with the current loader and shared combat parser vocabulary.

The title-specific binary description boundary is recognized structurally, while the text-table adapter still feeds MM7-oriented runtime fields and parsers. Active MM6 damage, missile, spell and special-attack labels can silently fall back to physical, no projectile, no spell or no special effect. Monster family grouping, hostility, resistance and loot consumers also retain MM7 assumptions or temporary defaults. This is parser and source coverage only: no real MM6 encounter, damage result, projectile, faction behavior or drop has been verified.

The later [combat semantics packet](../project/MM6_COMBAT_SEMANTICS_F2.md) joins all 173 retained rows to current consumer paths and the guide's qualitative behavior leads. Numeric triplets are candidate families; inherited MM7 identity, six-channel resistance aliasing, Energy ambiguity, faction/AI defaults and fixed-drop ID routing remain unresolved. Its tracked [probe](../scripts/mm6_probe_combat_semantics.py) stores exact per-row joins privately. Source diagnoses do not establish original AI thresholds, hit/damage formulas, loot or live combat.

Authority: [project/MM6_MONSTER_COMBAT_SPEC.md](../project/MM6_MONSTER_COMBAT_SPEC.md) and repeatable aggregate-only [monster probe](../scripts/mm6_probe_monster_rules.py), reviewed 2026-11-05. Per-row labels, hashes and output remain in ignored local reports.

See also: [MM6 Combat Semantics F2 Source](Source-MM6-Combat-Semantics-F2.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md).
