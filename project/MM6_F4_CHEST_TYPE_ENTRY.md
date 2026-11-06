# MM6 chest type, icon and entry contract — F4

Status: **detached title-specific type adapter only; live MM6 chest guard closed** (2026-11-06). A read-only trace of the F3-identity-matched staged executable was joined to the retained `dchest.bin` and all 67 accepted map deltas. Exact addresses, per-type counts and a focused checker remain in ignored owner-local research. No MM6 process or original installation was opened.

## Reconciled dimensions

The retained MM6 chest table contains eight fixed records. Every record's size metadata says **14×10**, and its icon field is a one-based number. The original executable's placement and chest display paths instead index separate hardcoded arrays for **9×9** dimensions for all eight types, plus type-specific pixel offsets. The display reads the table's icon number and formats the corresponding chest icon name; it does not use the record's display-name text as an image name. The first eight corresponding staged icons exist. The metadata's 14×10 is therefore not the active original placement or display area.

The original chest-open entry tests the initialized bit and calls placement when clear **before** checking for an active character or resolving trap/open behavior. Placement uses the hardcoded width × height, so its shuffle and item loop cover the first **81** fixed item slots. The original in-memory chest still has 140 28-byte item slots and 140 signed grid words; the extra storage is not evidence that placement scans 140 positions. The previous [placement packet](MM6_F4_CHEST_PLACEMENT.md) remains useful for the placement algorithm, but its 14×10 example is a detached geometry case, not the observed original caller configuration.

All 1,340 retained chest records use valid table types; seven of the eight types occur. Their 1,223 nonempty fixed item slots end before index 81, and all retained grid bytes are zero. This does **not** establish how changed saves persist anchors or unplaced items. The inherited `EntitySnapshots.cpp` chest reconstruction bounds positive anchors by width × height − 1; that is index 80 for this observed 9×9 runtime shape. The retained zero-grid fixture cannot test the changed-state boundary, so the inherited MM7 reconstruction must not be treated as a proved MM6 load path.

## Detached adapter

[`mm6::chestTypeLayout`](../src/Engine/MM6/MM6ChestTypeLayout.h) takes a complete caller-supplied chest table, MM6 title and type index. It validates table framing and all eight retained icon IDs before returning the original 9×9 dimensions, pixel offset and icon name. It does not touch a live chest, RNG, assets or saves. Three native unit cases passed for all eight type layouts, late malformed table rejection, foreign-title rejection and the 81-item scope when passed to the existing detached placement model. A separate native smoke harness using the retained table passed, and an ignored checker passed the original executable, table, entry/UI and 1,340 retained-record assertions. A fresh full CMake build was deferred under the current disk constraint.

Before wiring this into live MM6: prove complete item-generation and entry transaction, changed-grid save/reload, item references beyond the 81-cell area, trap/open ordering and correct rendering. Keep the existing MM6 chest generation guard closed; leave MM7/MM8 unchanged.
