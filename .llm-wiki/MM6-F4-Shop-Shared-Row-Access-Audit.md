---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Shared-Edge-Refresh]]", "[[MM6-F4-Shop-Owner-Field-Gate]]"]
tags: [mm6, f4, shop, guild, stock, save]
---

# MM6 F4 Shop Shared Row Access Audit

The [source packet](../project/MM6_F4_SHOP_SHARED_ROW_ACCESS_AUDIT.md) surveys direct stock-base references in the accepted F3-matched original executable. The identified families are new-game clear, shop and guild generation, menu display, successful purchase, listing/hover/pricing reads and raw Party persistence. Shop generation passes computed stock pointers to a general item generator, so a direct-address scan cannot prove every item-field writer.

The result is bounded: menu-level shared-deadline arbitration and physical stock identity are source-backed, but indirect aliases and downstream call effects are not exhausted. No owner-local standalone native save with nonzero shared stock was found in the staged MM6 files. The fork has neither a title-local live stock owner nor proved full conversion from the original 28-byte item record to its inherited 36-byte `Item_MM7` snapshot. The strict MM6 chest projection retains four raw tail bytes and does not license shop conversion.

Keep detached `SHOP` outside the required known-save manifest and live services. Next trace downstream calls receiving stock pointers and check a nonzero native save from a played session, then test a title-local service owner and full-record conversion with late-failure rejection.

See also: [Shared-Edge Refresh](MM6-F4-Shop-Shared-Edge-Refresh.md), [Shop Owner Field Gate](MM6-F4-Shop-Owner-Field-Gate.md), [Shop Purchase-Write Gate](MM6-F4-Shop-Purchase-Write-Gate.md).
