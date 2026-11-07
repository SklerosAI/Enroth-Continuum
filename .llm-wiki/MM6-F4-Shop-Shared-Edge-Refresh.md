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

An opt-in `OnEntry` policy bypasses the deadline check on each caller-defined vendor entry while retaining shop-bank order, the shared shop/guild stock row and its next deadline. `WhenExpired` remains the default. The title-local launch flag is `ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY=1`; the parser does not enable it for MM7. `ShopRefreshVisit` is a future per-window guard: successful entry generation consumes the override once, a generator failure leaves it retryable, and normal deadline checks remain separate. Linked stock tests passed **8/8**, including new-window rerolls and same-window suppression. Atlas saves the shared choice but currently launches only MM7; no MM6 process receives the flag and no live MM6 house entry calls the guard. Title-owned stock/clock owners, complete save admission and actual house entry remain gates before this changes gameplay.

The [shared-row access audit](MM6-F4-Shop-Shared-Row-Access-Audit.md) maps the direct stock-base source families and the remaining indirect-write and item-conversion limits.

See also: [Shop Purchase-Write Gate](MM6-F4-Shop-Purchase-Write-Gate.md), [Shop Guild Clock Save](MM6-F4-Shop-Guild-Clock-Save.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
