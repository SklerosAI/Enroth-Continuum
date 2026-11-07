# MM6 raw item authority — F4 schema decision

Status, 2026-11-06: a shared 28-byte MM6 value type is integrated for detached shop stock. It is a byte-preserving structural owner, not a complete live item adapter or a claim that all original contexts have identical save framing.

The accepted original-executable shop trace transfers all 28 bytes on purchase and clears only source bytes 0–24. `ShopStockFields` now stores `ItemRecord28` values; its `SHOP` v1 codec still writes every byte in physical row order, including the shared shop/guild row and the trailing bytes of sold slots. The type exposes six little-endian words and bytes 24–27 without assigning MM7 item meanings. Its linked focused shop/item tests passed 10/10, and the private-backed MM6/LOD gate passed 345/345.

The provisional `.ec6` `ITEM` v1 and `HELD` v1 fields are logical snapshots of the current `Item` runtime. `MM6DurableSaveFields::DurableItem` stores separate potion/gold/enchantment scalars, flags and an eight-byte expiration; `MM6DurableSaveRuntime` captures them from `Item` and reconstructs a new `Item`. `MM6HeldItemSaveFields` does the same for the cursor. Neither logical form contains original MM6 byte 26 or 27, and the added expiration has no source in a 28-byte MM6 record. Their current-runtime round trips do not prove native MM6 item fidelity. `MM6ChestRuntimeProjection` keeps the full raw 28-byte chest record as a sidecar but only checks a 24-byte `Item_MM7` projection. The shop, chest and future inventory/held/ground adapters must therefore not treat `Item_MM7` as the byte authority.

A follow-up [detached INVR section](MM6_F4_RAW_INVENTORY_SECTION.md) preserves the 138 physical item slots, 126 grid words and 16 equipment words described by the pinned MMExtension MM6 layout. A read-only original-executable trace confirms the 138-slot allocator, so MM6 party zeroing and reset now use 138 runtime inventory records while MM7 remains at 126. INVR remains outside live Party raw ownership, the known-save manifest and native-save import.

A [detached plain-item projection](MM6_F4_PLAIN_ITEM_PROJECTION.md) now uses MM6 item-table types instead of MM7 numeric potion ranges, retains the original 28-byte record and accepts only fields with a bounded view. It rejects nonzero bonus unions pending source proof and has no live owner or SaveLoad route.

A separate [standard-item projection](MM6_F4_STANDARD_ITEM_PROJECTION.md) uses the original equipment-type branch and centralized 14-index MM6 bonus map for positive-strength standard-enchanted records. It retains every source byte and rejects special bonuses and unsupported category/field changes. It is detached from the live character inventory and save transaction.

The [value provenance carrier](MM6_F4_ITEM_VALUE_PROVENANCE.md) now attaches exact source bytes to staged plain and common standard-enchanted `Item` values, preserving those bytes through shared value copies. Capture rejects missing or substituted carriers, but no live producer, unique identity, physical slot-clear policy or all-context save commit is defined.

## Migration order

1. Keep `ItemRecord28` as the exact byte value for source-proven 28-byte records. Retain context ownership: SHOP physical rows and chest/map sidecars have different lifetimes and mutation rules.
2. Use the third-party layout-backed detached inventory owner to preserve physical bytes while proving native inventory, held and ground mutation paths against the accepted source. Capture nonzero examples privately; publish only aggregate findings.
3. Define per-context title-specific views for display, equipment, generation and effects. Each view may interpret only fields whose category and flags are source-backed; keep unknown bytes with the raw owner and update only the bytes a proved operation changes.
4. Revise the final `.ec6` manifest so each item-bearing context can round-trip its raw owner plus placement/index information. Supersede the provisional `ITEM`/`HELD` v1 payloads rather than silently treating their logical fields as native MM6 records. Reject older provisional saves before live mutation unless an explicit migration can recover every required byte.
5. Add late-failure and nonzero mutation tests for transfer, save, quit, reload and title isolation before opening normal MM6 SaveLoad.

This is a schema direction for the personal fork. It does not modify the protected MM7 serializer or authorize a live shop, inventory, chest or cursor route. See [shop field matrix](MM6_F4_SHOP_ITEM_RECORD_CONVERSION_GATE.md), [durable fields](MM6_EC6_DURABLE_FIELDS_F4.md), [inventory restore](MM6_F4_INVENTORY_RESTORE.md), [chest projection](MM6_F4_CHEST_RUNTIME_PROJECTION.md) and [save transaction checklist](../.llm-wiki/MM6-F4-Save-Transaction-Checklist.md).
