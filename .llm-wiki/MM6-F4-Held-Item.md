---
type: synthesis
up: "[[MM6-F4-Save-Remainder]]"
tags: [mm6, save, inventory, held-item]
---

# MM6 F4 Held Item

This page records the standalone `.ec6` field for the item held on the cursor and its restoration boundary.

`Party::pPickedItem` is outside character inventory `ITEM`. Character, chest, shop, NPC and popup actions read or transfer it. The required `HELD` v1 section stores an empty marker or every current `Item` scalar: ID, potion/gold values, enchantments, charges, raw flags, lich-jar index and expiry. It rejects unsupported IDs and a nondefault MM7 lich-jar index. A present record requires a loaded MM6 item template with a usable icon footprint before staging.

Capture and copy-staging require the active MM6 title. Staging changes only `pPickedItem` on a party copy. It never calls `Party::setHoldingItem`, which would place/drop an old item and mutate the global mouse. The full transaction must first commit the staged party; afterward its UI step should set the icon and clear the transient click offset. Pointer recentering remains a first-frame UI policy to verify. The partial required-section assembler now includes `HELD`; SaveLoad remains guarded.

The [cursor/action gate packet](../project/MM6_F4_HELD_CURSOR_ACTION_GATE.md) adds an explicit detached post-commit cursor intent. `stageHeldCursorRestorePlan()` validates the whole HELD record against an explicit loaded MM6 item table, selects `MICON1` or an owned template icon name, and plans to clear the transient click offset. It rejects an observed pending spell, targeted/enchanting UI, turn queue/actions, the turn engine's pending flag, simulation timer turn mode, active rest or an open rest window, either frame of GUI messages, movement actions, a delayed enchant message, attack list or Party selection state. A read-only spell-queue query exposes the previously private ten-slot occupancy; `GUIMessageQueue::hasPendingForSave()` reports both frame queues without consuming them; and `capturePendingActionStateForSave()` collects the live MM6 action observation. Turn engine counters/timers can remain nonzero after `End()` and are not treated as live work. The test-only first-map save wrapper checks this before private map reads or disk writes. No mouse, icon cache or GUI is changed during staging; normal SaveLoad remains closed.

Two standalone C++ tests pass empty/present round-trip and malformed/wrong-title no-mutation checks. The retained F3-matched item-footprint result supplies the bounded item-table expectation. `LoadItemSizes` now preflights nonempty MM6 template icon names against `icons.lod` and rejects a missing icon before assigning any footprint; blank placeholders and MM7 behavior are unchanged. The focused icon guard test passes in the arm64 unit target. The later linked action-gate suite passed 4/4, and monitored scoped save passed 1/1 over 1,174 samples without a visible window or focus change. Icon rendering and modded save/reload still need F5 checks. The original installation was not reread for this action-gate work. See the [implementation packet](../project/MM6_F4_INVENTORY_RESTORE.md) for footprint evidence and remaining checks.

The [action-observation stability audit](MM6-F4-Action-Observation-Stability.md) records why the idle probe alone cannot protect a map-entry callback after disk/cache publication. A production route needs an enforced no-reentry safe point or complete action/UI rollback.

The later [raw HELD v2 candidate](MM6-F4-Raw-Held-Item-Section.md) is detached from this live route. It keeps the complete 28-byte native cursor record even when absent, preserving the three bytes left behind by the original clear operation. The current v1 known-save decoder still rejects v2; the final manifest must revise `ITEM` and `HELD` together.

See also: [Save Remainder](MM6-F4-Save-Remainder.md), [Inventory Restore](../project/MM6_F4_INVENTORY_RESTORE.md), [Cursor and Action Gate](../project/MM6_F4_HELD_CURSOR_ACTION_GATE.md), [Action Observation Stability](MM6-F4-Action-Observation-Stability.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
