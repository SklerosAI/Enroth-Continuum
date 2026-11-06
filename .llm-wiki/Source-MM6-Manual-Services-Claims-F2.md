---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, manual, services]
---

# MM6 Manual and Services Claims F2 Source

The [manual/services packet](../project/MM6_MANUAL_SERVICES_CLAIMS_F2.md) indexes all 61 owner-manual pages, five reference-card pages and 32 service-guide pages at locator level, with selected detailed text and visual checks. It maps controls, rest, shops, NPC hiring, temples, training, guilds, banks, transport and saves to current C++ consumers and explicit F3/F4/F5 checks. Locator coverage does not establish every numerical rule.

The clearest static collision is training: retained MM6 training houses are IDs 79–88, while the inherited cap table covers 89–98 and is indexed from `houseId()`. Temple restrictions and several NPC profession effects likewise need MM6-specific data and consumer joins. These are source-level risks, not observed MM6 crashes or service results.

The [full owner manual/card review](../project/MM6_MANUAL_CARD_FULL_REVIEW_F2.md) directly classified all 61 manual and five card pages, reading gameplay prose and recording exact page status privately. It adds card-only `Y` yell, item-in-hand and target shortcuts, NPC social/reputation choices, options/autosave ordering and independent recovery interactions to F4/F5 acceptance. Its Sword/Dagger mastery prose conflicts internally; another page misnames a spell school, and an XP example is too imprecise for a threshold rule. The [11-page NWC guide review](../project/MM6_NWC_GUIDE_FULL_REVIEW_F2.md) independently supports the manual's Sword/Dagger table and retained skill text over the conflicting example, and its fourteen named numeric potion results match retained matrix tokens. It omits eleven skills and cannot prove original effects. Guide-only potion actions and one dungeon-name alias remain open.

Authority: [project/MM6_MANUAL_SERVICES_CLAIMS_F2.md](../project/MM6_MANUAL_SERVICES_CLAIMS_F2.md), [full manual/card review](../project/MM6_MANUAL_CARD_FULL_REVIEW_F2.md) and [NWC guide review](../project/MM6_NWC_GUIDE_FULL_REVIEW_F2.md), reviewed 2026-11-05. Exact guide passages, per-ID ledger and original assets remain ignored locally.

See also: [MM6 UI Screen Audit F2 Source](Source-MM6-UI-Screen-Audit-F2.md), [MM6 Transport Fields F2 Source](Source-MM6-Transport-Fields-F2.md), [Open Questions](Open-Questions.md).
