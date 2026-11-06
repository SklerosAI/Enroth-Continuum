---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, maps, persistence]
---

# MM6 World F2 Closure Source

The [world closure packet](../project/MM6_WORLD_F2_CLOSURE.md) reconciles retained MM6 map-delta sections with current C++ loaders, reconstruction, resets and save paths. Its tracked [world roundtrip probe](../scripts/mm6_probe_world_roundtrip.py) strictly split and reassembled all 67 retained indoor/outdoor delta companions byte for byte, passed synthetic nonzero sentinels in opaque sections, and rejected malformed input. The detailed section positions and hashes remain in an ignored local report.

This establishes bounded **wire-section preservation in the Python probe**. It does not establish reset-header, persistent-byte or tail meanings; those baseline regions are zero-filled. It does not verify MM6 C++ decode, object reconstruction, mutation, save/reload, native `.mm6` compatibility or a playable map. The packet lists field-consumer and F4/F5 obligations, including indoor base decoding, actor/item/chest/door state, reset ordering, `.ec6` isolation and recovery, and graphical validation.

The later [world field map](../project/MM6_WORLD_FIELD_MAP_F2.md) corrects the chest interpretation: 14×10 is separate layout metadata, **not a proved delta grid**. The final 280 bytes of each MM6 delta chest have two unresolved splits, and both candidate grid regions are zero in all retained baselines. Preserve them as opaque bytes until changed-state evidence settles the live grid.

Authority: [project/MM6_WORLD_F2_CLOSURE.md](../project/MM6_WORLD_F2_CLOSURE.md), read with the [save specification](../project/MM6_SAVE_SPEC.md) and [implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md). The closure packet does not itself advance F2 or F3.

See also: [MM6 World Field Map F2 Source](Source-MM6-World-Field-Map-F2.md), [MM6 Save and Map Specification Source](Source-MM6-Save-Spec.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md).
