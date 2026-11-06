---
type: synthesis
up: "[[MM6-F4-Beacon-Visual-Save]]"
related: ["[[MM6-F4-Save-Transaction-Checklist]]", "[[MM6-Headless-Integration-Policy]]"]
tags: [mm6, f4, save, beacon, renderer]
---

# MM6 F4 Beacon Renderer Contract

The [renderer candidate packet](../project/MM6_F4_BEACON_RENDERER_CONTRACT.md) records the API required after detached `BEAC` CPU image preparation. `GraphicsImage::Create()` immediately requests an OpenGL texture, whose current result is unchecked. ID zero currently passes `TextureRenderId::isValid()`. The installed GLAD post-call callback drains errors for warnings, so a later `glGetError()` is not a reliable success check. Candidate destruction queues texture deletion in an allocating vector; its contents drain only on buffer swap.

No generic renderer patch was made in this lane. A checked texture result, controlled unpublished-candidate disposal or pre-reserved deferred deletion, fault-injection and Mac OpenGL tests, and an MM7 graphical regression check are required before GPU candidates can join the all-owner MM6 save transaction. The existing BEAC and live SaveLoad guards remain closed. See [beacon save boundary](MM6-F4-Beacon-Visual-Save.md) for the codec and CPU stage.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Headless Integration Policy](MM6-Headless-Integration-Policy.md).
