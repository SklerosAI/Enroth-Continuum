# MM6 boats and stables handoff

- Owner: Claude Code lane, reviewed and integrated by the lead.
- Branch: `claude/mm6-transport`, commits `57aea80c0`, `a45c51482`, `566fe2e61`, `bb965aadf`, on `master` as `251afc203`, `6cb755017`, `7b619bba9`, `d9cad0ae3`.
- Shared code: one MM6-gated line in `BaseRenderer.cpp` (MM6 ships use the level flag MM7 reserves for its obelisk chest, so they were hidden and unclickable), and `PortraitFrameTable.cpp` now accepts an animation repeating its own portrait, which MM6's yes animation does and MM7 data never contains. `Transport.cpp` is MM7-only again. New `src/GUI/UI/Houses/MM6Transport.{h,cpp}`, pure rules in `MM6TransportRules`.

## Original behavior (MM6 executable)

A 36-record route table (destination map, weekday flags, days, arrival point and yaw, quest bit) and a three-routes-per-house index for houses 48-68, both matching the private transport rules byte for byte. A slot shows its route when that weekday is flagged, its quest bit is clear or the party has it, and it differs from the last route shown. Otherwise "come back another day". Price is 25 for stables and 50 for boats times the house value, minus merchant percent, floored at a third of full price, and house 59 is free. Days drop for Horseman at stables, for Sailor, Navigator and Pirate on boats, and for Explorer on both, minimum 1. Travel takes the gold, sets the arrival, advances days × 1440 minutes with the party rest, and uses no food. The gated routes are 49B (qbit 178), 60B (qbit 179) and 59A-C (qbit 168).

## Checks

`MM6Transport.StableToNeighbour`, `BoatToMistAndBack` and `QuestRouteToHermitsIsle`, input-driven from production New Game, with revert-and-fail for each fix. On the branch: unit tests 1,119, MM6 suite 110, MM6 first-map gate 29 with the MM7 controls and save-anywhere children, MM7 in-process 107, full MM7 540/540, `check_style` clean.

## Open

The stable "fix prices" step (dialogue 74, qbits 147-155 to 117, a main-quest blocker) continues on `claude/mm6-fix-prices`. Same-map arrival, the gold refusal, hireling reductions and the oute1 routes have no test. No MM6 house plays its greeting sound. The cost line layout is not matched. MM6 saves from before this fix may carry ships marked invisible.

Privacy: addresses, IDs and map stems only.
