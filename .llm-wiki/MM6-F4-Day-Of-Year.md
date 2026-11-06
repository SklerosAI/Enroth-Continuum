---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, calendar]
---

# MM6 F4 Day of Year

The [public implementation packet](../project/MM6_F4_DAY_OF_YEAR.md) covers raw EVT ID 19. Frozen retained scripts use it in 592 Compare instructions across four resources; all 87 distinct operands lie in the 336-day MM6 calendar. [MMExtension](https://grayface.github.io/mm/ext/ref/) identifies the ID as `DayOfYearIs`.

The MM6-only comparison checks one-based day 1–336 against party time and bypasses inherited character iteration. Unsupported writes fail before mutation. The aggregate checker rehashes all 68 active EVT files and runs in the offline suite. Arm64 MM6/MM7/unit targets built, and three focused native tests passed. The private companion has the exact resource distribution and handoff. F5 must observe an actual date branch and save/reload across a boundary; the current result is an offline rule seam.
