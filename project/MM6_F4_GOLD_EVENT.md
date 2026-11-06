# MM6 fixed-gold event variable

Status: F4 bounded party-value seam, 2026-11-06. No MM6 reward, payment or save/reload outcome has been observed in play.

The F3-matched retained EVT ledger contains **147 uses of raw variable ID 21** across twelve active resources: 14 Compare, 111 Add and 22 Subtract. Operands range from 1 to 250,000; there is no active Set. [MMExtension's event variable reference](https://grayface.github.io/mm/ext/ref/) names ID 21 `Gold`. The engine stores a single party gold balance, and the partial `.ec6` core codec copy-stages it. The retained operations and source support a party-scoped threshold, grant and affordable payment, while exact MM6 status/audio presentation remains open.

The MM6 interpreter now handles ID 21 before character iteration. Compare checks party balance against the operand. Add checks nonnegative input and signed 32-bit overflow before applying the new balance once. Subtract checks affordability once, stops the event when short, and otherwise applies the remaining balance once. Writes use `SetGold(..., true)` to avoid inherited MM7 sound and status cues. A shipped-script-absent Set rejects before inherited execution. The pure numeric helper rejects negative balance/operand and overflow without mutating party state. MM7 retains its prior path. Title-specific MM6 reward/payment presentation remains open.

The [aggregate verifier](../scripts/mm6_f4_check_gold_event_operands.py) checks the format-2 report, map index and all 68 retained EVT fingerprints before asserting the 147-use operation inventory. It is wired into the one-command offline runner and prints only aggregates. Focused native tests cover an equal threshold, grant, affordable and short payment, zero, negatives and overflow. The exact event sites remain in the ignored private report.

The arm64 MM6, OpenEnroth and unit targets built, and three focused gold/operand cases passed. The standalone retained verifier passed; the next integrated offline runner is a separate check.

Next: exercise representative retained Compare/Add/Subtract instructions through the interpreter with character selectors, prove one party mutation and event stop on a short payment, implement the MM6 status/audio message and gold UI response, and round-trip a nonzero balance through a complete `.ec6` save. F5 must observe a reward and both successful and failed payments after save/quit/relaunch, while confirming MM7 isolation.
