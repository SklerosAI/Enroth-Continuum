---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, items, campaign]
---

# MM6 F4 Item Event Compare

The [public packet](../project/MM6_F4_ITEM_EVENT_COMPARE.md) records 641 active raw ID 17 uses: 195 Compare and 446 Add/Subtract. A frozen aggregate checker rehashes the private F3-matched event report, item table and all 68 active EVT resources. Only aggregate counts and limits enter Git; exact sites remain in the private companion.

The MM6 interpreter now evaluates Compare over its selected character inventory and the party's held item through a title-specific read-only helper. Dialog-item comparisons search the party. Item IDs must be 1–580. Native tests cover present/absent inventory and cursor items and invalid values. MM7 stays on the inherited path.

MM6 Add, Subtract and Set fail closed before inherited MM7 item mutations. The 446 active writes still block campaign completion. Resolve item generation and cursor displacement, removal target and priority, artifact bookkeeping, failed removal flow, and `.ec6` save/reload before enabling them. F5 must observe actual reward/handoff branches and visual inventory state.
