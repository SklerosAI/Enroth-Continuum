---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, spells, guides]
---

# MM6 Spell Guide Claims F2 Source

The [spell guide packet](../project/MM6_SPELL_GUIDE_CLAIMS_F2.md) and [probe](../scripts/mm6_probe_spell_guide_claims.py) give all 99 retained MM6 spell IDs a guide heading locator. All 56 spell-section pages were rendered and inspected. Each of the 99 visually checked guide base costs matches the retained MM6 table; 22 differ from inherited MM7 static base costs. A private matrix now summarizes all 297 Normal/Expert/Master clauses: 207 quantitative, 87 qualitative and three explicitly guide-unknown. Four guide/table rank-effect disputes and other cost, target and damage ambiguities remain private. A narrow same-ID C++ screen flags 58 label/base-mana conflicts and 41 effects not demonstrated; no spell effect is verified correct. Full C++ semantics, executable-only constants and runtime casts remain open.

The later six-facet C++ source screen covers every spell ID's dispatch, mana, recovery, targeting, damage/duration and effect branch. It classifies 70 source effects as conflicting, 27 as limited candidates and two UI continuations as unresolved; recovery is unresolved for 96. These categories are source comparisons, not original or current runtime outcomes. The private matrix holds locators and per-ID F4/F5 checks.

See also: [MM6 Spell Effect F2 Source](Source-MM6-Spell-Effect-F2.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
