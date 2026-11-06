---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, monsters, combat]
---

# MM6 Monster and Combat Specification Source

This F2 source compares retained MM6 monster tables with the current loader and shared combat parser vocabulary.

The title-specific binary description boundary is recognized structurally, while the text-table adapter still feeds MM7-oriented runtime fields and parsers. Active MM6 damage, missile, spell and special-attack labels can silently fall back to physical, no projectile, no spell or no special effect. Monster family grouping, hostility, resistance and loot consumers also retain MM7 assumptions or temporary defaults. This is parser and source coverage only: no real MM6 encounter, damage result, projectile, faction behavior or drop has been verified.

Authority: [project/MM6_MONSTER_COMBAT_SPEC.md](../project/MM6_MONSTER_COMBAT_SPEC.md) and repeatable aggregate-only [monster probe](../scripts/mm6_probe_monster_rules.py), reviewed 2026-11-05. Per-row labels, hashes and output remain in ignored local reports.

See also: [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md).
