# MM6 shop, guild and bounty save identity audit — F4

Status: source and accepted-F3 evidence audit, 2026-11-06. This is a **save blocker**, not an `.ec6` codec or permission to enter the inherited service screens. No original installation was reread and no MM6 process was launched.

## Current runtime owner and lifetime

| Current owner | Current consumer | Why the value may survive a save |
| --- | --- | --- |
| `PartyTimeStruct::shopNextRefreshTime`, `Party::standartItemsInShops` and `specialItemsInShops`, twelve `Item` slots per shop/special array | `Shops.cpp` checks the refresh time, regenerates both stocks and sets the next time using `houseTable[houseId()].generation_interval_days`; purchase and popup paths consume the resulting stock | Purchased/removed stock must not reappear before the next refresh; regeneration must not reroll on load. |
| `PartyTimeStruct::guildNextRefreshTime`, `Party::spellBooksInGuilds`, twelve slots per guild | `MagicGuild.cpp` displays prior stock until expiry and otherwise generates books and advances the clock | Bought books and next generation time must remain coupled. |
| `PartyTimeStruct::shopBanTimes`, `Party::InTheShopFlags` | `Shops.cpp` sets a one-day ban when caught stealing; house/shop entry checks expiration. Other dialogue paths reset `InTheShopFlags`. | The ban clock is timed campaign state. The in-shop flag's save lifetime is unresolved and should not be inferred from MM7 serialization. |
| `PartyTimeStruct::bountyHuntNextGenTime`, `Party::monster_id_for_hunting`, `monster_for_hunting_killed` | `TownHall.cpp` generates a new target on a monthly boundary and pays/clears the completed target; `Actor.cpp` marks a matching target killed | Clock, target and killed status form one durable per-hall tuple. |

The current `Party.h` arrays are indexed by `HOUSE_FIRST_SHOP..HOUSE_LAST_SHOP`, `HOUSE_FIRST_MAGIC_GUILD..HOUSE_LAST_MAGIC_GUILD` and `HOUSE_FIRST_TOWN_HALL..HOUSE_LAST_TOWN_HALL`. `HouseEnums.h` explicitly ends the shop array at `HOUSE_52` for vanilla MM7; guild and town-hall bounds are likewise named for MM7 locations. The inherited MM7 snapshot copies these arrays exactly. A title marker on that layout would still omit or misassign MM6 houses. The current MM6 house path is display-only; service actions remain gated, so the inherited consumers are evidence of required state families, not proved MM6 behavior.

## Accepted F3 evidence and missing join

The ignored accepted F3 logical `.ec6` manifest specifies party resources, inventory/item generation and durable save scenarios at a family level. It explicitly did not read a native whole-save sample or define per-field widths/house keys. The corrected F3 PE report and signoff map candidate room descriptors and transport records, and the retained MM6 house table has 557 rows. Those reports do **not** identify an MM6 shop/guild/town-hall state key, stock slot cardinality, refresh/ban clock shape, bounty target array or persistence rule. The room descriptor join is not a shop-stock save join.

## Gate before a `SHOP`/bounty section

1. Establish a title-specific stable key for every active MM6 shop, guild and town hall from accepted house evidence and the actual service selector. Reject duplicate, out-of-range or unclassified keys; do not cast the 557-row table through MM7 `HOUSE_FIRST_*` ranges.
2. Give the MM6 `Party` owner storage keyed to those houses. Prove current stock count, ordinary/special and guild item roles, refresh/ban clocks, bounty target and killed status from original or explicitly accepted current-runtime behavior. Decide whether `InTheShopFlags` is transient.
3. Encode clocks together with their stocks/targets; validate MM6 item templates and monster IDs before staging copies. Reject a missing house tuple, a foreign title or malformed slot without mutating the party. Test nonzero purchase/refresh, ban and bounty values and a late invalid field.
4. Only after the full mandatory save manifest and all-owner commit are ready may the MM6 SaveLoad guard open. The current sixteen-section candidate omits this owner entirely.

See the [save remainder](MM6_SAVE_REMAINDER_F4.md), [transaction checklist](../.llm-wiki/MM6-F4-Save-Transaction-Checklist.md), accepted F3 signoff in the ignored private companion, and current `Party.h`, `Shops.cpp`, `MagicGuild.cpp`, `TownHall.cpp` and `EntitySnapshots.cpp` for the source paths audited here.
