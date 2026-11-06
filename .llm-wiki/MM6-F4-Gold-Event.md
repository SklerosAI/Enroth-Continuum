---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, gold]
---

# MM6 F4 Gold Event

The [public packet](../project/MM6_F4_GOLD_EVENT.md) covers raw EVT ID 21. Frozen retained scripts use it 147 times: 14 Compare, 111 Add and 22 Subtract, with no Set. [MMExtension](https://grayface.github.io/mm/ext/ref/) identifies the ID as `Gold`; the engine stores this value on the party. The exact use sites stay in the ignored private report.

The MM6-only path checks party balance once per instruction. A grant preflights signed overflow; an unaffordable payment stops the event before mutation. Valid writes set the checked balance silently, leaving MM6 reward/payment cues for a later source and F5 check. Negative values and the unsupported Set operation reject. MM7 keeps its prior path. The aggregate checker rehashes all 68 active EVT files and runs in the offline suite. The arm64 MM6/MM7/unit targets built and three focused tests passed; complete `.ec6` saving and F5 reward/payment observations remain open.
