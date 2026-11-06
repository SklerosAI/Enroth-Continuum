---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Party-Movement-Save]]", "[[MM6-F4-Awards]]"]
tags: [mm6, f4, save, party, counters]
---

# MM6 F4 Justice Counter Save

`PART` v3 preserves four current-runtime `Party` counters that the previous `.ec6` core payload omitted.

The fields are `uFine`, `uNumDeaths`, `uNumPrisonTerms` and `uNumBountiesCollected`. The inherited snapshot copies all four, and current award/UI and event-variable code reads or changes them. `uFine` also affects the town-hall/justice route. The `.ec6` core codec writes each as a signed 32-bit word after the v2 flag words, for a 90-byte payload. Fine must be nonnegative; the remaining counters preserve signed values because current event subtraction can produce them. A valid whole container and payload are decoded before a detached `Party` stage receives the values. Old `PART` versions reject; no live `.ec6` slot needs migration.

This is **current-runtime state preservation**, not proof that inherited award or justice behavior is correct for MM6. The [award audit](../project/MM6_F4_AWARD_TABLE.md) found an MM6 bounty label whose meaning disagrees with the current bounty-reward accumulator. Shop/guild stock, house-keyed bounty clocks/targets and original award producers remain outside this change. Live SaveLoad and MM6 campaign play stay guarded.

The three changed production/test translation units syntax compiled with integrated arm64 flags. A standalone native codec probe passed nonzero v3 round-trip, exact length, v2 rejection, and a valid-checksum negative-fine failure without destination mutation. Linked unit and headless tests remain for main integration.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Party Movement Save](MM6-F4-Party-Movement-Save.md), [Awards](MM6-F4-Awards.md).
