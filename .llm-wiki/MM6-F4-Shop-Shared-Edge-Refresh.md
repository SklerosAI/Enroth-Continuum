---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Purchase-Write-Gate]]", "[[MM6-F4-Shop-Guild-Clock-Save]]"]
tags: [mm6, f4, shop, guild, refresh, save]
---

# MM6 F4 Shop Shared-Edge Refresh

The [source packet](../project/MM6_F4_SHOP_SHARED_EDGE_REFRESH.md) records how the original MM6 shop and guild menu branches arbitrate one shared stock row and deadline.

The last shop row's second bank and the first guild row address the same twelve 28-byte records and one 64-bit deadline. Both shop choices regenerate both banks when that deadline is strictly older than game time; the guild branch regenerates guild stock under the same test. Otherwise each skips generation and reads the shared row. The service that first refreshes after expiry therefore controls the observed shared row until a later expiry in these paths. A nonzero native save and played-session behavior remain unchecked.

The detached [refresh seam](../src/Engine/MM6/MM6ShopRefreshBoundary.h) joins the raw [stock owner](../src/Engine/MM6/MM6ShopStockFields.h) and [clock owner](../src/Engine/MM6/MM6ShopGuildClockFields.h) on copies. It preserves strict expiry and shop two-bank order, and rejects foreign titles, unready owners and invalid next deadlines. Generator failure leaves both detached owners unchanged. It does not implement item generation, live house entry or SaveLoad; `SHOP` remains outside the required known-save manifest.

See also: [Shop Purchase-Write Gate](MM6-F4-Shop-Purchase-Write-Gate.md), [Shop Guild Clock Save](MM6-F4-Shop-Guild-Clock-Save.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
