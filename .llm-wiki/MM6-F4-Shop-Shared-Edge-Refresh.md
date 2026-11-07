---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Purchase-Write-Gate]]", "[[MM6-F4-Shop-Guild-Clock-Save]]", "[[MM6-F4-Shop-Shared-Row-Access-Audit]]"]
tags: [mm6, f4, shop, guild, refresh, save]
---

# MM6 F4 Shop Shared-Edge Refresh

The [source packet](../project/MM6_F4_SHOP_SHARED_EDGE_REFRESH.md) records how the original MM6 shop and guild menu branches arbitrate one shared stock row and deadline.

The last shop row's second bank and the first guild row address the same twelve 28-byte records and one 64-bit deadline. Both shop choices regenerate both banks when that deadline is strictly older than game time; the guild branch regenerates guild stock under the same test. Otherwise each skips generation and reads the shared row. The service that first refreshes after expiry therefore controls the observed shared row until a later expiry in these paths. A nonzero native save and played-session behavior remain unchecked.

The detached [refresh seam](../src/Engine/MM6/MM6ShopRefreshBoundary.h) joins the raw [stock owner](../src/Engine/MM6/MM6ShopStockFields.h) and [clock owner](../src/Engine/MM6/MM6ShopGuildClockFields.h) on copies. It preserves strict expiry and shop two-bank order, and rejects foreign titles, unready owners and invalid next deadlines. Generator failure leaves both detached owners unchanged. Its integrated focused shop/clock suites passed **8/8** and the private-backed MM6/LOD gate **338/338**. It does not implement item generation, live house entry or SaveLoad; `SHOP` remains outside the required known-save manifest.

An opt-in `OnEntry` policy bypasses the deadline check on each caller-defined vendor entry while retaining shop-bank order, the shared shop/guild stock row and its next deadline. `WhenExpired` remains the default. The title-local launch flag is `ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY=1`; the parser does not enable it for MM7. The live MM6 house entry path does not yet call this boundary, so the Atlas toggle must be treated as a future MM6 mod setting until title-owned stock and the save route are ready. A live hook must invoke it once per actual entry, never once per subpanel redraw. Synthetic tests for repeated visits, shared-row replacement and exception rollback passed syntax checking; linked integration is pending.

The [shared-row access audit](MM6-F4-Shop-Shared-Row-Access-Audit.md) maps the direct stock-base source families and the remaining indirect-write and item-conversion limits.

See also: [Shop Purchase-Write Gate](MM6-F4-Shop-Purchase-Write-Gate.md), [Shop Guild Clock Save](MM6-F4-Shop-Guild-Clock-Save.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
