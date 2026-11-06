---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, spells, rules]
---

# MM6 Spell Effect F2 Source

The [spell effect audit](../project/MM6_SPELL_EFFECT_F2.md) and tracked [probe](../scripts/mm6_probe_spell_effects.py) cover all 99 retained MM6 spell IDs across nine schools as static data and source-consumer comparisons. Matching labels or numeric switch paths do not prove matching effects. The MM6 table branch fills text and damage labels, while inherited `pSpellDatas` still supplies costs, recovery, damage factors, flags and mastery. Some active damage labels fall back to physical. Event and monster casts also need title-specific ID and label mappings.

The MM6 item index has 99 scrolls and 99 spellbooks in ranges that differ from inherited item-kind mapping. All retained spell items lie outside the corresponding inherited mapping ranges. Four inherited numeric IDs are marked as inventory-item targets, but the retained MM6 labels at those IDs differ; this is a targeting-label collision, not yet a semantic classification. Each spell's effects, targets, three mastery ranks, scroll/book use and relevant monster/event casts need F2/F3 reconciliation, full F4 offline coverage, and representative F5 graphical and audio checks.

Authority: [project/MM6_SPELL_EFFECT_F2.md](../project/MM6_SPELL_EFFECT_F2.md), read with [rules closure](../project/MM6_RULES_F2_CLOSURE.md), [monster/combat spec](../project/MM6_MONSTER_COMBAT_SPEC.md) and [implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md). Exact per-ID labels, guide passages and joins stay ignored locally. No spell gameplay has been verified.

See also: [MM6 Rules F2 Closure Source](Source-MM6-Rules-F2-Closure.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
