---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, character, potion]
---

# MM6 Class and Skill Specification Source

This F2 source compares retained MM6 character tables and guide rules with the current inherited character and item UI consumers.

The six three-tier class families and MM6 skill order require separate mappings, promotion and mastery rules, and a six-class creation flow. The inherited default party, class selection, and skill-description loading are MM7-oriented. A deeper source review also finds the MM7 potion UI's item predicates and Alchemy-skill gate unsuitable for MM6's herb/bottle interaction. Exact starting vectors, Quick Start state, potion recipes, side effects, and persistence still need reconciliation. No runtime code was changed by the audit.

The [rules closure packet](../project/MM6_RULES_F2_CLOSURE.md) adds a primary MMExtension starting-stat/skill array schema lead and an offline guide-to-matrix check: 25 numeric-result input pairs match, while two result rows disagree. Neither source supplies the exact starting vectors or proves potion effects; marker meanings and disputed outcomes stay open. Exact recipes stay private.

The later [creation audit](../project/MM6_CREATION_F2.md) finds the inherited MM7 Thief default, no distinct MM6 Quick Start path, seven starter spellbook literals outside the retained MM6 book band and a bottle constant that classifies as an MM6 scroll. These source/category collisions do not identify the correct starting items or party.

Authority: [project/MM6_CLASS_SKILL_SPEC.md](../project/MM6_CLASS_SKILL_SPEC.md), reviewed 2026-11-05. Its checked ID tables and page locators belong there; do not duplicate private source rows or guide text here.

See also: [MM6 Creation F2 Source](Source-MM6-Creation-F2.md), [MM6 Rules F2 Closure Source](Source-MM6-Rules-F2-Closure.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md).
