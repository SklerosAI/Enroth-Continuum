---
type: note
up: "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"
tags: [mm6, f4, save, headless]
---

# MM6 F4 post-world abort gate

The disposable MM6 test hook now returns an explicit `AbortToMainMenu` result.
`Game::gameLoop` handles it immediately after `DoPrepareWorld(true)` and before
the timer/input loop. An unarmed load and MM7 cannot request this result.
The separate disabled v1 fresh-child QuickLoad control exercises the actual
menu/game-loop edge and checks that the slot bytes remain unchanged. See
[the source-backed gate packet](../project/MM6_F4_POST_WORLD_ABORT_GATE.md).

This is a control seam only. V2 publication still refuses in the load callback
because resource, identity, event and transient owners have no complete
pre-world join. Do not interpret this test hook as a v2 reload or campaign
acceptance.
