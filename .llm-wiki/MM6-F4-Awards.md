---
type: implementation
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, awards, text, ui]
---

# MM6 F4 Awards

The [award packet](../project/MM6_F4_AWARD_TABLE.md) records a title-specific loader for the F3-matched 100-row `awards.txt`. It retains 86 nonempty texts under their numbered IDs, validates the complete table before mutation, and leaves all blank MM6 priority cells at a neutral storage value. The inherited MM7 loader keeps its existing behavior.

Seven MM6 texts need numeric formatting that the inherited MM7 award pane cannot map reliably. They are excluded from MM6 pane rendering until their counters are established. Unformatted MM6 text bypasses the inherited MM7 numeric-counter switch even where IDs collide. Remaining text is available, but display order, color, event effects and live save/reload are still F4/F5 obligations. Exact rows and fingerprints remain in the ignored private companion.
