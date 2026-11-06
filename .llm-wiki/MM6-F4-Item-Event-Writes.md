---
type: synthesis
up: "[[MM6-F4-Item-Event-Compare]]"
tags: [mm6, f4, events, items, campaign, transaction]
---

# MM6 F4 Item Event Writes

The [public write audit](../project/MM6_F4_ITEM_EVENT_WRITE_AUDIT.md) and [read-only comparison packet](../project/MM6_F4_ITEM_EVENT_COMPARE.md) distinguish supported comparisons from guarded rewards/handoffs. The frozen private reports contain 290 Add and 156 Subtract sites for raw ID 17. An extended aggregate checker joins the variable and opcode records and asserts that all 156 Subtracts have an earlier same-item Compare in the same event. This is script topology, not a proven control-flow or removal rule.

The inherited Add can generate random item parameters, set artifact flags, move an old cursor item to inventory or ground and replace the cursor. The inherited Subtract ignores the selected player in favor of a party scan, favors inventory over cursor and returns success even on a miss. Neither behavior is established for MM6. `HELD` is copy-staged in partial `.ec6`, but MM6 artifact-generation flags and live SaveLoad remain open. MM6 writes therefore stay guarded.

Next agents should use the ignored local companion at `local-game-data/private-wiki/mm6/F4_ITEM_EVENT_WRITES_2026-11-06.md` and the public audit's four-part implementation gate. First prove cursor displacement, owner selection, duplicate priority, failure flow and artifact persistence; then implement an atomic MM6 transaction and inspect reward/handoff plus save/reload in F5.
