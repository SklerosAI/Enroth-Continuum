---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, items, monsters, guides]
---

# MM6 Item and Monster Guide Claims F2 Source

The [item and monster guide packet](../project/MM6_ITEM_MONSTER_GUIDE_CLAIMS_F2.md) and [queue probe](../scripts/mm6_probe_item_monster_guide_claims.py) place all 581 item IDs and 173 monster IDs in a private claim-review queue with explicit OCR match status and page locators. Thirty-two guide pages and selected fields for 16 item and 39 monster IDs were visually checked. Among selected claims, three item base values and 14 monster selected-field values disagree with retained data; 163 other item base values agree through OCR extraction alone. Exact rows and values stay private. OCR presence, value agreement and numeric triplets are partial evidence, not per-ID effect proof. The queue still marks 402 item and 134 monster semantic rows unreviewed; live combat remains open.

A subsequent static pass classifies retained role and current C++ consumer for 404 high-risk item IDs. Sixteen of 25 wand spell tokens disagree with inherited lookup; all 99 MM6 scrolls, 99 books, 30 artifacts/relics and 50 message scrolls fall outside their inherited type ranges. Ninety-eight special-band IDs have held-item event operands. Sixty-seven special/misc roles and all per-item runtime effects still need resolution; the role pass did not add guide semantic checks.

See also: [MM6 Combat Semantics F2 Source](Source-MM6-Combat-Semantics-F2.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
