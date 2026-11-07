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
available for comparison. A real loaded-table Create Party probe must report
which ring/enchantment outcomes pass and must keep failing unsupported draws
closed. Production SaveLoad and Atlas remain guarded until full live owner
capture, world publication and reload pass their gates.
