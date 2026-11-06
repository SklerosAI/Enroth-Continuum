---
type: analysis
up: "[[MM6-F4-Creation-Rules]]"
derived_from: ["[[MM6-F4-Creation-Rules]]", "[[MM6-F4-Quick-Start]]"]
tags: [mm6, f4, creation, health, mana]
---

# MM6 F4 Creation HP/SP Init Gate

The [source packet](../project/MM6_F4_CREATION_HP_SP_INIT_GATE.md) audits initial health and mana assignment and records a bounded MM6-only refill-order correction without replacing provisional formulas.

## Question

Does the original Quick Start or editable creation initialization justify an MM6-only HP/SP formula correction now?

## Context

The corrected F3 private report supplies class HP/SP bases, factors and mana-stat dispatch, while explicitly leaving runtime formula and promotion retroactivity open. The current MM6 `Character` methods use those arrays with a provisional inherited bonus structure.

## Analysis

The original Quick Start finalizer writes class, stats, skills and starting items before filling current HP/SP. Its inspected health call chain uses actual Endurance and its bonus, actual level, class factor, family base, health skill/item bonuses and full-health bonus, then clamps to one. These match the current MM6 health expression at the term level. The original mana call enters a separate class-dispatched function with nested stat, age, skill and item paths that this audit has not fully reconciled. Current editable creation used to refill after the random ring but before skill-granted items; its MM6 branch now refills after all grants, while MM7 keeps the inherited earlier refill. The provisional default party is not the original Quick Start fixture, and first-map post-entry values remain unobserved.

## Conclusion

The MM6 creation refill now occurs after starting-item grants; its integrated focused arm64 unit test passed 1/1 and confirms that MM7 retains its prior timing. No arithmetic correction is supported yet. Keep the title-local HP/SP formulas unchanged while tracing the complete mana helper and obtaining a nonzero post-entry comparison.

## Open follow-ups

The [max-mana condition trace](MM6-F4-Max-Mana-Condition-Trace.md) closes the original outer returns and corrects MM6 actual-stat condition priority. Finish item/skill helper and promoted-tier mapping, then check Quick Start and editable creation after first-map entry and durable save/reload. Keep random starting inventory and fixture activation in the separate Quick Start gate.

See also: [Creation Rules](MM6-F4-Creation-Rules.md), [Quick Start](MM6-F4-Quick-Start.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
