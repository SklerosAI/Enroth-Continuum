# MM7 Three Statuettes handoff — 2026-11-12

## Outcome

- Replaced the Angel-only acquisition probe with `MM7SideQuests.ThreeStatuettesRequireChestsAndShrines`.
- The route reaches Thom Lumbra's real Tularean Forest offer, then proves his visible return topic refuses the incomplete state.
- It takes the Knight, Eagle and Angel statuettes through the authored Grand Temple of the Sun, Grand Temple of the Moon and Maze chest UIs.
- It locates and clicks the authored outdoor shrine faces in Avlee, Tatalia and Bracada Desert, verifying each placement bit and item consumption.
- Thom's final return clears the quest, grants 50,000 gold and 50,000 XP to each character, and gives the award to all four characters.

## Verification

- Focused route: 1/1 passed headlessly.
- Exact production-Save writer and separate normal-Load reader: 1/1 each in a marked owner-only temporary profile; the completed state survived and the slot stayed byte-identical.
- Writer without a profile and reader with no save: both rejected.
- Regional side/promotion selection: 12/12 passed.
- Changed-file `cpplint`, `git diff --check`, and full `check_style`: 157/157 passed.
- Both disposable profiles were moved to Trash and remain recoverable.
- Integrated on neutral `master` at `a21c0dff8`.

## Boundary

This is a complete independent quest transaction and durable checkpoint, not continuous regional progression. No visible or offscreen-OpenGL process ran, and no private game data, save, checksum or image entered Git.
