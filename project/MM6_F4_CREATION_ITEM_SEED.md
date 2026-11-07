# MM6 first-map creation item seed

`UIPartyCreation.cpp` currently gives each character a generated ring and then
skill-dependent equipment, books or reagents. Those `Item` values enter the
shared inventory without `mm6RawRecord`; the native v2 first-map writer
correctly rejects them. The new detached `seedMM6CreationItems()` adapter
copies a fresh MM6 Party, constructs zero-based 28-byte records for backpack
items, retains each physical item index and grid footprint, and attaches the
records to the copied `Item` values. It supplies a fresh empty MM6 cursor.
The live Party is never changed.

Admission is title- and table-locked. Each proposed record must stage through
the existing MM6 bounded item decoder and reproduce **every** represented
logical `Item` field. The full four-character raw inventory must stage and
recapture byte-for-byte. Plain common items, standard MM6 attribute bonuses,
and the already bounded Antique bonus can pass. Unsupported specials, potions,
artifacts, inherited raw records, equipped items, occupied cursors and
out-of-range physical placement reject. The adapter does not draw a new random
ring; it carries the actual ring selected by creation.

The returned `SeededCreationItems::party`, `inventory`, and `cursor` are inputs
to `prepareOfflineRawNewGameAutosave()`. The existing first-map known seed may
be captured from the original Party because the adapter changes only the raw
carrier and physical inventory capacity. The native writer still checks the
seed's logical Party values and the raw inventory replay before creating bytes.

Synthetic linked arm64 tests passed **3/3**, covering a standard-bonus ring,
a plain item, physical index 137 and grid continuation, exact recapture,
source immutability, foreign title/table, unsupported special, inherited raw
record and occupied cursor. A supported starter also flowed through the full
offline native v2 first-map candidate. These prove a canonical first-map seed
round trip through the current schema. They do **not** establish byte parity with an
original MM6 New Game save: no native nonzero creation inventory record is
available for comparison. Production SaveLoad and Atlas remain guarded until full live owner
capture, world publication and reload pass their gates.

## Loaded-table Create Party probe

The disabled `MM6ZCreationItemSeedHeadless.RealStarterItemsStageWithoutDisk`
GameTest runs alone with the owner's MM6 tables and private creation rules in
the null-platform harness. It sets the sequential random engine, follows the
real Main Menu → editable Create Party → post-creation callback, and calls the
adapter on the complete Party and then on each item in isolation. It prints
only category counts. The callback stages the already established first-map
cache in memory so the game routine can leave creation; it never calls a save
writer. The test asserts that the in-memory user filesystem has no save slot.

Two fresh headless runs returned the same aggregate result: **20/20** starter
items admitted, comprising **16 plain** and **4 standard-bonus**; **0** Antique,
potion, special, rarity or other rejections. The complete Party was admitted.
Both tests passed 1/1. The host monitor recorded 31 and 18 samples with zero
on-screen game windows and no focus change. No private rows, item identities,
raw records or assets entered Git. This verifies one deterministic editable
creation outcome against loaded tables, not every random ring draw or original
MM6 native-save byte parity. It does not write or reload a v2 slot.
