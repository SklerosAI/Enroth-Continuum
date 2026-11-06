---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, rules, potion]
---

# MM6 Rules F2 Closure Source

The [rules closure packet](../project/MM6_RULES_F2_CLOSURE.md) joins retained item, spell, class and monster evidence to current consumers and future acceptance checks. Its tracked [potion-guide probe](../scripts/mm6_probe_potion_guide.py) confirms that the guide chart and retained item matrix have the same 25 unordered numeric-result input pairs, but two result rows disagree. The disputed results and exact recipes stay private. The shipped table is stronger evidence for the owner's installation; original-game action and marker effects still require F5 checks. The [F4 matrix loader](MM6-F4-Potion-Matrix.md) now provides bounded outcomes; a suitable MM6 potion UI and effects do not exist yet.

A primary MMExtension reference documents MM6 starting-stat and starting-skill array *schemas* and skill-code meanings. It does not provide this project's exact six-class values, Quick Start defaults, equipment, spellbooks or promotion rules. Those remain extraction and reconciliation targets. The closure packet is a consumer-to-acceptance matrix, not an implemented rule set or an F2 gate completion.

The later [spell effect audit](../project/MM6_SPELL_EFFECT_F2.md) covers all 99 retained spell IDs as static source/data joins. It identifies inherited `pSpellDatas`, spell-target, scroll/book-range and monster/event routing assumptions; each MM6 effect and mastery meaning remains unverified. Its [probe](../scripts/mm6_probe_spell_effects.py) retains per-ID detail only in an ignored local report.

Authority: [project/MM6_RULES_F2_CLOSURE.md](../project/MM6_RULES_F2_CLOSURE.md), read with the [class/skill specification](../project/MM6_CLASS_SKILL_SPEC.md), [table-consumer specification](../project/MM6_TABLE_CONSUMER_SPEC.md), and [implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md). Exact recipe triples and guide transcription stay ignored locally.

See also: [MM6 Spell Effect F2 Source](Source-MM6-Spell-Effect-F2.md), [MM6 Class and Skill Specification Source](Source-MM6-Class-Skill-Spec.md), [MM6 Text Table Consumer Specification Source](Source-MM6-Table-Consumer-Spec.md), [Open Questions](Open-Questions.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md).
