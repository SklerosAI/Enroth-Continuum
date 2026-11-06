---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-House-Service-Gate]]", "[[MM6-F4-Save-Remainder]]"]
tags: [mm6, f4, save, shop, guild, bounty]
---

# MM6 F4 Shop Save Key Matrix

The [public audit packet](../project/MM6_F4_SHOP_SAVE_KEY_MATRIX.md) joins the retained 557-row house table to active event references and current Party/serializer owners. Exact row identities and candidate tokens stay in an ignored owner-local matrix.

The retained `Type` classifier finds 47 shops and 22 spell guilds. An older count of 34 guild-labelled rows included 12 Mercenary/Thieves Guild rows: 14 guild labels fall inside the inherited MM7 guild range, but only two are spell-guild types. The remaining 20 spell guilds fall outside that range. All 47 shop-type rows fall inside the MM7 shop range, which also contains five transport rows. The MM7 bounty range contains none of the MM6 council or Town Hall label candidates. A numeric row overlap is not a persistent stock or bounty key.

Current `Party` stores shop stock/clocks for 52 MM7 shops, guild books/clocks for 32 MM7 guild IDs and bounty tuples for five MM7 town halls. The inherited serializer copies those arrays. The MM6 house presentation route does not enter the inherited service mutators. Retained column-15 values are numeric for the 47 shops and 22 spell guilds, but the original MM6 refresh meaning and native save layout are unproved. The accepted F3 PE report covers house media and transport, not these state families.

The next source unlock is a targeted original shop/guild handler and save/load trace keyed by the retained row and column-15 accesses, plus a council/bounty route trace. Only after keyed MM6 Party storage, slot/clock semantics and item/monster validation are established should a mandatory detached `.ec6` codec be written. Live MM6 SaveLoad remains closed.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [House Service Gate](MM6-F4-House-Service-Gate.md), [Save Remainder](MM6-F4-Save-Remainder.md).
