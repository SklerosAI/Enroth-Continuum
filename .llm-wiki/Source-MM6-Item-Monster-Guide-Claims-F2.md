---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, items, monsters, guides]
---

# MM6 Item and Monster Guide Claims F2 Source

The [item and monster guide packet](../project/MM6_ITEM_MONSTER_GUIDE_CLAIMS_F2.md) and [queue probe](../scripts/mm6_probe_item_monster_guide_claims.py) place all 581 item IDs and 173 monster IDs in a private claim-review queue with explicit OCR match status and page locators. The later rendered catalog pass directly identified 460 individual item entries: 403 selected field sets match retained data, 12 disagree, two have identity-only checks and 43 lack a field/effect comparison. Another 97 IDs have only generic category coverage, two are confirmed absent from printed item tables, 19 have unresolved OCR locators, and three retained rows are blank. The queue now marks 164 item semantic rows unreviewed, down from 402. The older selected monster pass checked 39 IDs and still marks 134 semantically unreviewed. Exact rows and values stay private. Printed entry identity and selected field agreement are not executable item effects or live combat.

The separate [monster catalog packet](../project/MM6_MONSTER_GUIDE_SEMANTICS_F2.md) later directly checked all 171 printed variants' identity and selected fields. The older queue's monster semantic status is narrower and should not be read as a current count of IDs without any direct catalog comparison. Neither packet establishes complete monster effects or live combat.

A subsequent static pass classifies retained role and current C++ consumer for 404 high-risk item IDs. Sixteen of 25 wand spell tokens disagree with inherited lookup; all 99 MM6 scrolls, 99 books, 30 artifacts/relics and 50 message scrolls fall outside their inherited type ranges. Ninety-eight special-band IDs have held-item event operands. Sixty-seven special/misc roles and all per-item runtime effects still need resolution; the role pass itself did not add guide semantic checks. The later catalog pass reduced the semantic queue but did not settle those runtime effects.

The [item-effect packet](../project/MM6_ITEM_EFFECT_SEAMS_F2.md) triages all 164 still-unreviewed rows by retained role. Its source join finds that the seven current miscellaneous direct-use constants all exceed MM6's maximum item ID, and the inherited message-scroll window guard rejects all 50 MM6 message-scroll IDs. These are deterministic current-source path mismatches, not confirmed original item actions or witnessed UI failures; exact per-ID roles/events remain local.

The [late item-guide packet](../project/MM6_ITEM_GUIDE_CLOSURE_F2.md) directly classifies 43 individually printed late entries: eight offer qualitative role clues, while 35 give no explicit action rule. A separate gem table matches nine of ten retained name/value pairs and differs on one identity. The retained event graph contains 400 held-item operands for 50 of 67 broad special/misc IDs, grouped by opcode signature privately. These checks narrow the F3 questions without resolving the 164 original item effects.

See also: [MM6 Combat Semantics F2 Source](Source-MM6-Combat-Semantics-F2.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
