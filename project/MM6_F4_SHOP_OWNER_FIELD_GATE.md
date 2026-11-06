# MM6 `SHOP` owner field gate — F4

Status, 2026-11-06: **no new `SHOP` field is safe to add to detached `.ec6` staging**. The only complete source-backed shop/guild owner field is the keyed 64-bit refresh deadline already encoded in required `SCLK` v1. Normal MM6 SaveLoad and house services remain guarded.

## Evidence and decision

The accepted original-executable trace and retained 557-row house matrix establish the active house row as the stock and refresh key. Shop rows 1–47 and spell-guild rows 119–140 have proved refresh-deadline reads/writes; the final shop and first guild address the same physical deadline word. `ShopGuildClockFields` preserves those **68 distinct words**, tests nonzero round-trip and rejects late truncation without mutating its destination. This is a complete bounded field, already implemented; adding a second copy under `SHOP` would create two authorities for one physical word.

| Candidate new field | What the source proves | Missing for a complete owner |
| --- | --- | --- |
| One shop or guild stock item ID | Both shop banks and the guild display scan a 12-position physical stride of 28-byte records and skip a slot whose first 32-bit item ID is zero. The current house row is used in the address formula. | The remaining 24 bytes are not typed. A nonzero ID alone would discard enchantments, charges or other item state and cannot represent a full stocked slot. The purchase write and following read are untraced, so zero-ID sold-out mutation is unproved. |
| Raw 28-byte stock record | Physical record stride and original raw `party.bin` inclusion are established. | The two shop banks meet a guild-family edge; the last-shop/first-guild effective use or alias is unresolved. A blind pair of 47-row arrays could duplicate or overwrite one physical slot. The fork has no MM6-keyed runtime owner or service writer to keep an opaque record current. |
| Ban, in-shop or bounty tuple | The inherited MM7 code has analogous state families. | Its `HouseId` intervals do not identify MM6 spell guilds or candidate town halls. Original MM6 row keys, widths, lifetime and write semantics for these fields are unproved. |

The existing partial session coordinator intentionally rejects nonempty inherited MM7-indexed shop/guild/bounty arrays. A detached codec could make its own local copy of raw bytes, but that would establish only copy atomicity: after a purchase or refresh, no proved MM6 owner could supply the updated bytes, and no verified key mapping could restore them. Therefore **no new `SHOP` section, provisional item-ID field or MM7 array adapter is added**.

## Shortest unlock

1. Trace the original purchase write and next display read for each of the two shop banks and one spell-guild bank. Record which full 28-byte fields change, whether a sold slot clears the ID, and which branch uses the shared edge. Verify those results against an owner-local nonzero `party.bin` sample from the user's owned game data.
2. Build a title-local owner keyed by proved MM6 house rows, with a single policy for any physically shared slot. Have current MM6 service generation and purchase paths write that owner; validate every nonzero item template against the loaded MM6 item table. Keep bans and bounty separate until their own row and lifetime traces are complete.
3. Only then add a mandatory detached `SHOP` section. Stage all rows and both banks on a copy, reject missing/duplicate keys and invalid full item records, and inject failure at the final row to prove that neither the live owner nor the previous disk slot changes. Verify save–quit–reload after all remaining `.ec6` owners and world commit are ready.

## Checks performed

The ignored owner-local stock checker revalidated the accepted executable identity, retained shop/guild row bands, bank/stride/display reads, family-edge arithmetic, raw Party save span and current cross-map autosave order. It passed on 2026-11-06. The linked `MM6ShopGuildClockFields.*` native suite passed 2/2, including nonzero deadline round-trip and late-failure atomicity. No native MM6 save sample, game process or proprietary bytes were added to Git. Source evidence: [shop refresh trace](MM6_F4_SHOP_REFRESH_PE_TRACE.md), [stock tuple and autosave](MM6_F4_SHOP_STOCK_TUPLE_AND_AUTOSAVE.md), [save-key matrix](MM6_F4_SHOP_SAVE_KEY_MATRIX.md), and [SCLK contract](MM6_F4_SHOP_GUILD_CLOCK_SAVE.md).
