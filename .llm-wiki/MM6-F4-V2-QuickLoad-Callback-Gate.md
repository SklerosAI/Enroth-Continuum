---
type: synthesis
up: "[[MM6-F4-New-Game-Autosave]]"
related: ["[[MM6-F4-Raw-Child-Publication-Gate]]", "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"]
tags: [mm6, f4, reload, quickload]
---

# MM6 F4 V2 QuickLoad Callback Gate

This page records the caught, read-only v2 load callback check in a disposable MM6 child.

The [fresh-child callback packet](../project/MM6_F4_V2_QUICKLOAD_CALLBACK_GATE.md)
documents a disabled headless GameTest that reuses the test-only `loadGame()`
override on a disposable native v2 slot. It selects and stages exact primary
bytes, inspects the child-publication admission, then catches its explicit
refusal before returning to `Game::gameLoop()`. Normal MM6 load remains
guarded; no live owner is assigned and no Main Menu QuickLoad event is sent.

`Game::gameLoop()` currently continues immediately after the void load
callback. A real event needs a complete test-only publication coordinator
before the callback can return safely. The v2 writer and read-only stage
milestones do not satisfy that gate. Keep the test disabled and run it only
with an owner-only disposable profile and null renderer.

The isolated linked three-process check passed writer, read-only reader and
caught callback **1/1** each. The callback admitted exact v2 primary bytes,
then the typed publication inspection reported unresolved world/resource
owners. Normal MM6 load remained guarded and sampled live owners and slot
bytes were unchanged. Host monitors saw zero on-screen windows and no focus
change in 26, 11 and 12 samples. See the public packet for a disposable
runner recipe. No Main Menu QuickLoad event or live publication was run.

See also: [Raw Child Publication Gate](MM6-F4-Raw-Child-Publication-Gate.md), [Fresh-Child Owner Audit](MM6-F4-V2-Fresh-Child-Owner-Audit.md), [New Game Autosave](MM6-F4-New-Game-Autosave.md).
