---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, npc, professions]
---

# MM6 NPC Profession F2 Source

The [profession packet](../project/MM6_NPC_PROFESSION_F2.md) joins all 77 retained MM6 profession rows to current C++ loaders and consumers. Fifty-one benefit-bearing names match the owner's rendered guide in order. The other 26 rows have generic occupations and no guide benefit entry, but some retained NPCs reference them and carry a join flag; their actual hire routes are not proved.

Seven generic MM6 IDs alias named MM7 special-effect professions in inherited code. The MM6 loader omits several display and generation fields, and source-level bonus/action paths diverge from selected guide and retained-table effects. Navigator and Scout values disagree between guide and retained data. F3 must reconcile original IDs and benefit evidence; F4 must title-scope effects and make generic rows safe; F5 must observe hire/fire, service, commission and persistence. This is a static audit, not live hireling validation.

Authority: [project/MM6_NPC_PROFESSION_F2.md](../project/MM6_NPC_PROFESSION_F2.md), reviewed 2026-11-05. Exact per-ID and guide text remain in the ignored private ledger.

See also: [MM6 Manual and Services Claims F2 Source](Source-MM6-Manual-Services-Claims-F2.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md).
