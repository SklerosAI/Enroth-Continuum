---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Raw-Save-Runtime]]", "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"]
tags: [mm6, f4, save, menu]
---

# MM6 F4 Menu Slot Preflight

The [implementation packet](../project/MM6_F4_MENU_SLOT_PREFLIGHT.md) defines
a read-only v2 list that applies the same semantic primary/backup selection as
load preflight. It uses the selected candidate's HEAD and recovery flag, and
omits a slot when no candidate passes the caller's validator. Structural-only
`listRawSaveSlots()` is not a playable-slot inventory.

The visible MM6 load menu remains empty. A complete session and first-frame
admission validator is still needed before the menu can use the new list or
queue a filename. Synthetic tests verify selection and disk immutability;
they do not prove a real campaign load.

Normal MM6 Main Menu QuickLoad now refuses before checking a present
`quicksaveN.ec6`; the test-only `autosave.ec6` override is unchanged. A
headless disposable test with a UFS-visible RAM marker passed **1/1**: no
loading transition, pending filename or load callback, and unchanged marker.
MM7 control passed **1/1**. The [packet](../project/MM6_F4_MENU_SLOT_PREFLIGHT.md)
records the next UI seam: MM6 needs its own semantic `.ec6` inventory. The
prior paused-timer blocker is **superseded** by the exact first-map
`PausedSaveMenu` capture contract, verified in a real-menu headless child
**1/1** with the unpaused chain **7/7** and MM7 control **1/1**. The
disposable writer defaults to settled input, and its explicit paused-mode
test wrote only an existing owner-only candidate with exact primary/backup
checks. The independent fresh-child replay passed **1/1**. Normal save/load
stays closed.

See also: [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [V2 Fresh-Child Owner Audit](MM6-F4-V2-Fresh-Child-Owner-Audit.md).
