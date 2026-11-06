# MM6 F4 event autonote variable

Status: bounded offline runtime seam. This does not establish visible campaign behavior or enable MM6 SaveLoad.

The ignored F3-matched event-variable ledger (`local-game-data/mm6/research-2026-11-05/event-variable-probe.json`) identifies raw variable ID 205 as the event autonote. The retained shipped script set has 101 uses (97 Set and four Add), with operands in 1–128. The F3-matched singular `icons/autonote.txt`, which the current table loader uses, has 128 rows. The similarly named plural table is a different resource and does not cover the entire active operand set. These aggregate facts are tracked here; the original table and per-site inventory remain private.

`describeMM6Variable(205)` now marks a party-scoped autonote. MM6 execution dispatches ID 205 before inherited MM7 variable validation and execution. A bounded executor requires ID 205 and an operand from 1 through 128, then reads or writes `Party::_autonoteBits` exactly once. Set/Add set the bit idempotently, Subtract clears it, and Compare tests it for the event branch. Invalid operands throw before state mutation. ID 206 onward stays guarded. MM7 follows the previous path. The dialog-topic compare path uses the same checked state, although the retained shipped ID 205 uses are only Set/Add.

The durable `.ec6` `QEST` section already captures and copy-stages the party's 208 autonote bits; the 128 shipped MM6 rows sit within that field. This is a storage connection, not an enabled live save/load path. Offline tests cover boundary operands, idempotence, invalid no-mutation, and preserving bits outside the shipped table. Existing title-specific variable tests continue to assert that no ID 46–226 enters MM7 execution.

The event UI notification and award sound for a newly learned MM6 note are unresolved. The bounded executor changes the durable bit only; it does not call the inherited `Character::giveAutonote` effect because that chooses MM7 presentation behavior. F5 must inspect a real event's note book, visual cue and sound, plus save/quit/relaunch/load. Remaining active variable IDs still need separate title-specific semantics and persistence review.
