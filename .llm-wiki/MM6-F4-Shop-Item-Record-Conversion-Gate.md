---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Owner-Field-Gate]]", "[[MM6-F4-Shop-Shared-Row-Access-Audit]]"]
tags: [mm6, f4, shop, item, conversion, save]
---

# MM6 F4 Shop Item Record Conversion Gate

The [field matrix](../project/MM6_F4_SHOP_ITEM_RECORD_CONVERSION_GATE.md) compares the accepted F3-matched original 28-byte stock item with the fork's 36-byte `Item_MM7` snapshot, using the original executable plus pinned MMExtension and editor schemas. Original generation writes item ID, bonus, bonus strength, second bonus/gold, charges and condition words. Purchase copies all 28 bytes and clears source bytes 0–24, retaining the final three bytes.

The mapping into live `Item` is incomplete: MM7's category-dependent bonus unions and flag meanings are not proved for every MM6 item; its byte 26 means lich-jar character index while MMExtension labels MM6 byte 26 an uncertain owner; MM7 adds an eight-byte expiration absent from MM6. MM6 bytes 24–27 must remain raw until equipped-slot, maximum-charge, owner and padding behavior is independently established. The bounded chest projection's guarded first-24-byte conversion is specific to chests.

Decision: `ShopStockFields` is the sole physical 28-byte authority, including the shop/guild alias and sold-slot trailing bytes. The detached `SHOP` section may preserve raw records; it stays outside required known-save and live service paths. The next unlock is a title-local item mapping with nonzero generated/purchased records, complete category and flag validation, exact raw-tail preservation and late-failure tests.

See also: [Shop Owner Field Gate](MM6-F4-Shop-Owner-Field-Gate.md), [Shared Row Access Audit](MM6-F4-Shop-Shared-Row-Access-Audit.md), [Shop Purchase-Write Gate](MM6-F4-Shop-Purchase-Write-Gate.md).
