# MM6 detached raw cursor owner

Status: detached F4 source-backed slice. The MM6 `HELD` v2 packet preserves an entire 28-byte cursor record, including the final three bytes after the cursor becomes empty. This owner keeps those bytes independently of the shared visible `Item`, whose `Reset()` discards its optional raw carrier.

`RawHeldCursorOwner` is title-tagged and explicitly ready. A fresh MM6 owner starts with all bytes zero. An empty owner has item ID zero and bytes 0–24 zero; bytes 25–27 may be nonzero. A present owner has a structural item ID in 1–580. `stageRawHeldCursorSet` accepts only an empty owner and copies all 28 incoming bytes. `stageRawHeldCursorClear` clears only bytes 0–24 on a copy. `captureRawHeldCursor` and `stageRawHeldCursorFromFields` bridge the detached owner to the proposed `HELD` v2 value without changing a live Party or SaveLoad. A caller can defer publishing the staged copy until all other owners pass validation.

This deliberately omits relocation of an occupied cursor, source inventory mutation, ground fallback, item-category projection, icon loading, and pending click/action state. The original setter's occupied-cursor branch also moves the old item; a raw cursor-only set would lose it, so this helper rejects that case. The owner has no live attachment, and existing `Party::setHoldingItem`, `takeHoldingItem`, `Zero` and event rollback still lack coordinated raw writes. `KnownSaveFields` continues to use `HELD` v1 and provisional `ITEM`. A live transaction must stage this owner with all four raw inventories and world state, verify consistency with `Item::mm6RawRecord` and loaded MM6 templates, then revise `INVR`/`HELD` together before any normal save/load path is enabled. Present v1 cursors cannot be losslessly migrated to v2.

Synthetic tests cover full set, first-25-byte clear, absent-tail round trip, title isolation, malformed owner rejection and a late staged-failure path that leaves the source owner unchanged. They do not establish a playable campaign or a native save round trip.

The later post-world empty-cursor gate checks the detached image plan against
the actual `Io::Mouse` after `DoPrepareWorld(true)`. It accepts an initialized
native `MICON1` arrow with no bitmap, or the custom-arrow mode only when the
bound `micon1` image decodes to nonempty CPU pixels. It also requires zero
pickup offset and a completely empty Party held item. The linked cursor suite
passed **5/5**; separate disposable v1 writer and real Main Menu QuickLoad
abort controls passed **1/1** each with the GameTest custom arrow. The MM7
headless control passed **1/1**. This is a resource/owner check for an empty
cursor, not an occupied-item offset, GPU draw or v2 publication proof.
