---
type: synthesis
up: "[[MM6-F4-Raw-Held-Item-Section]]"
related: ["[[MM6-F4-Raw-Item-Transfer-Semantics]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, save, cursor, raw-item]
---

# MM6 F4 Raw Cursor Owner

The detached MM6 cursor owner retains the original 28-byte record even when the visible shared `Item` is empty.

The [source packet](../project/MM6_F4_RAW_CURSOR_OWNER.md) defines its bounded copy and clear contract. It starts ready with zero bytes, copies all 28 bytes on a set into an empty cursor, and clears only bytes 0–24 while retaining the final three. The owner and `HELD` v2 value are separately staged; a later failed check leaves the source owner unchanged. Occupied-cursor relocation is outside this helper because it also needs an inventory or ground destination.

This owner is not attached to the live `Party`. `stageRawHeldParty()` now projects a ready physical cursor onto a detached Party copy through the bounded item adapter and requires exact 28-byte replay; an empty cursor clears only the detached item. Three linked synthetic tests passed. `Party::takeHoldingItem()` resets the shared item, `Party::setHoldingItem()` publishes a mouse icon and may move an older item, and event rollback directly restores `pPickedItem`. Those live paths and the current `HELD` v1 `KnownSaveFields` assembly must be revised as one all-owner transaction before live save/load. A present v1 cursor cannot be losslessly converted to the raw v2 record.

The synthetic focused tests validate exact byte retention, MM6 title isolation, invalid-owner rejection and late staging without source mutation. They are code-level evidence only, not a live cursor or saved-campaign check.

The later [cursor/shop handoff](MM6-F4-Raw-Cursor-Shop-Handoff.md) checks this owner against decoded `HELD` v2 and the separate `SHOP`/`SCLK` owner before any live publication.

The [detached cursor image plan](../project/MM6_F4_RAW_CURSOR_IMAGE_PLAN.md) derives a template icon name and broken/identified tint only after exact raw/Party held-item replay. An occupied pickup offset is unresolved because EC6 v2 does not store it. The plan neither resolves an asset nor changes the live mouse cursor; a full-session commit still needs to choose and verify that visible placement.

For an empty cursor only, a post-world check now compares the plan with the
actual mouse state after world preparation. It accepts either the native arrow
or a decoded nonempty custom `micon1` bitmap, with zero pickup offset and no
stale Party item payload. Focused checks pass 5/5 and a disposable v1 Main
Menu QuickLoad control passes 1/1. Occupied cursor and GPU presentation remain
open; the v2 child loader does not yet call this gate.

See also: [Raw Held Item Section](MM6-F4-Raw-Held-Item-Section.md), [Raw Item Transfer Semantics](MM6-F4-Raw-Item-Transfer-Semantics.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
