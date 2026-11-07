---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Saved-Actor-PID-Identity-Gate]]", "[[MM6-F4-Active-World-PID-Restore]]"]
tags: [mm6, f4, save, pid, world]
---

# MM6 F4 Post-Rebuild PID Slots

The [implementation packet](../project/MM6_F4_POST_REBUILD_PID_SLOTS.md) adds a read-only MM6 refusal gate for a future post-`DoPrepareWorld(true)` handoff. It compares the detached saved DLV/DDM and CHST sidecar actor/sprite counts and per-index anchors with the final actor and sprite arrays, requiring the loaded map name and title to match. Two linked synthetic tests passed for indoor/outdoor agreement and late slot mismatches. No live world or save path calls the gate yet.

Matching anchors cannot prove that the saved delta survived respawn or that a slot was never reused. Map reload can legitimately change arrays and cause a conservative refusal. The earlier active VISU prohibition remains unchanged; this helper does not release overlays or rebind object PIDs. A fresh-child loader still needs selected-delta provenance, no-reuse evidence, remaining owner/resource gates and a post-rebuild graphical check.

See [Saved Actor PID Identity Gate](MM6-F4-Saved-Actor-PID-Identity-Gate.md) and [Active World PID Restore](MM6-F4-Active-World-PID-Restore.md).
