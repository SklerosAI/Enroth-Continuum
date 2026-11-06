---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-House-Service-Gate]]", "[[MM6-F4-Save-Remainder]]"]
tags: [mm6, f4, save, shop, guild, bounty]
---

# MM6 F4 Shop Save Key Matrix

The [public audit packet](../project/MM6_F4_SHOP_SAVE_KEY_MATRIX.md) joins the retained 557-row house table to active event references and current Party/serializer owners. Exact row identities and candidate tokens stay in an ignored owner-local matrix.

The retained `Type` classifier finds 47 shops and 22 spell guilds. An older count of 34 guild-labelled rows included 12 Mercenary/Thieves Guild rows: 14 guild labels fall inside the inherited MM7 guild range, but only two are spell-guild types. The remaining 20 spell guilds fall outside that range. All 47 shop-type rows fall inside the MM7 shop range, which also contains five transport rows. The MM7 bounty range contains none of the MM6 council or Town Hall label candidates. A numeric row overlap is not a persistent stock or bounty key.

Current `Party` stores shop stock/clocks for 52 MM7 shops, guild books/clocks for 32 MM7 guild IDs and bounty tuples for five MM7 town halls. The inherited serializer copies those arrays. The MM6 house presentation route does not enter the inherited service mutators. The later [targeted original PE trace](MM6-F4-Shop-Refresh-PE-Trace.md) proves retained column-15 values are refresh days in observed shop/guild branches, and the original raw `party.bin` block contains their clocks and stock. Exact stock tuple semantics and the boundary index are still open.

The next source unlock is an original purchase and stock-read trace, a nonzero native `party.bin` sample and a council/bounty route trace. The [SCLK section](MM6-F4-Shop-Guild-Clock-Save.md) now preserves only proved 64-bit refresh deadline words with their shared edge. Complete stock/bounty storage and item/monster validation still precede a full commerce codec. Live MM6 SaveLoad remains closed.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [House Service Gate](MM6-F4-House-Service-Gate.md), [Save Remainder](MM6-F4-Save-Remainder.md), [Original Refresh Trace](MM6-F4-Shop-Refresh-PE-Trace.md).
