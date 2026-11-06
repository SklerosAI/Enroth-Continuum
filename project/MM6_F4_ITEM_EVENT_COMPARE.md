# MM6 item event comparison

Status: F4 bounded read-only event path, 2026-11-06. Item grants and removals remain guarded; no MM6 gameplay or save/reload has been observed.

The F3-matched retained active EVT corpus has **641 raw variable ID 17 uses across 41 files**: 195 Compare, 290 Add and 156 Subtract. There is no Set. The instructions name 165 distinct item IDs, all in 1–578; the retained `icons/items.txt` table has numbered IDs 0–580. [MMExtension's event-variable reference](https://grayface.github.io/mm/ext/ref/) identifies this variable as `Inventory/Items`. The [tracked aggregate checker](../scripts/mm6_f4_check_item_event_operands.py) rehashes the frozen report, item table and all 68 active EVT resources, then checks this inventory without publishing private locations or game data.

MM6 Compare now uses a title-specific read-only helper. For each character chosen by the interpreter's existing event target selection, it checks that character's inventory and the party's held cursor item. Dialog-item comparison checks the same condition over each party character. Operands outside raw ID 17 and item IDs 1–580 reject. This preserves the existing selection and held-item scope while avoiding inherited MM7 variable dispatch for the 195 active comparisons. The path does not mutate items, generate artifacts, play sounds or change quest state. MM7 continues through its inherited executor.

MM6 Add, Subtract and Set reject before the inherited item operations. The **446 active writes** cannot yet safely use the MM7 behavior: an Add generates an item, marks an artifact and moves the cursor item; a Subtract can take inventory or cursor contents and has additional MM7-specific quest behavior in the interpreter. A correct MM6 transaction needs source-backed rules for generation, cursor displacement, target selection and removal priority, failure/shortfall flow, artifact bookkeeping and durable save/reload. Set has no retained active use, so its semantics are also unproved.

The arm64 MM6, OpenEnroth and unit targets built; two focused native tests passed for inventory, held item, absence, bounds and read-only preservation. The private retained checker passed and is wired into the offline runner. These checks do not demonstrate that an MM6 quest branch or item reward works in play.

Next: prove item grant/removal rules against retained MM6 data and independent source, implement a title-specific transaction with representative branch/state tests, then inspect reward/handoff and save/quit/relaunch/load in F5. Keep MM7 regression checks alongside that work.
