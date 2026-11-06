# MM6 day-of-year event variable

Status: F4 bounded comparison seam, 2026-11-06. No MM6 date gate has been observed in play.

The F3-matched retained EVT ledger contains **592 uses of raw variable ID 19**, all Compare, in four active resources. They use 87 distinct one-based day values from 76 through 335. Three resources each contain 196 uses and the fourth has four. The [MMExtension event variable reference](https://grayface.github.io/mm/ext/ref/) names ID 19 `DayOfYearIs`; the inherited engine's `Time` calendar has twelve 28-day months. These independent leads support a 336-day year and an equality comparison, while actual quest consequences remain unobserved.

`mm6::compareDayOfYear` now checks raw ID 19, an operand in 1–336 and a nonnegative party time before comparing `playingTime.toDays() % 336 + 1`. The MM6 interpreter calls it once before character selection for ordinary and dialogue-topic comparisons. Add, Subtract and Set on this raw ID explicitly reject before inherited execution; none occurs in the retained active scripts. MM7 keeps its previous path. The pure helper has no state mutation or notification side effect.

The [aggregate verifier](../scripts/mm6_f4_check_day_of_year_operands.py) checks the format-2 report, map index and all 68 retained EVT fingerprints before asserting every active ID-19 operand and operation. It is wired into the one-command offline runner and prints only aggregate coverage. Two focused native tests cover the one-based endpoints, 336-day rollover and invalid input; a separate variable guard test checks the operand range. The arm64 MM6, OpenEnroth and unit targets built and all three focused cases passed. The full integrated runner and gameplay date scenarios remain separate gates.

Next: replay representative retained date gates through the event interpreter, including a day boundary and year rollover; confirm the expected reward or branch and any map reset interactions; then observe a date gate in F5 and save/quit/relaunch across it. The exact four resource names and use sites remain in ignored local reports.
