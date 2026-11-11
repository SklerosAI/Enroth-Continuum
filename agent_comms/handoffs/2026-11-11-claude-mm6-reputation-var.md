# MM6 reputation variable (215) handoff

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm6-reputation-var` in the `mm7-mod-access` checkout, from `master` `f0db53a79`. Commits `a95399e85` (executor) and `e8c45a076` (EC6 PART v4 persistence, separable). Not pushed.

## Original semantics (from the GOG MM6 executable)

One signed 32-bit party-wide value, where positive is good (the opposite of MM7). Set, Add and Subtract use only the operand's low byte and never clamp. Compare adds hired-NPC shifts (Bard +200, Pirate, Gypsy, Duper and Burglar -200 each) and tests equality, and a negative operand always passes. A whole-party target repeats the write once per character. The shipped scripts hold 42 uses, 30 Add and 12 Subtract, and three run with the whole party targeted. Disassembly addresses are in the commit message and in the private notes.

## Design

New MM6-owned `src/Engine/Evt/MM6ReputationEvent.{h,cpp}`. In `MM6EventVariableDispatch.cpp`, 215 joins the per-character family, so the interpreter's existing per-character loop repeats it as the original does. `EvtInterpreter.cpp` is unchanged. `Party` gains `mm6Reputation`, which MM7 never reads. The second commit adds it to the EC6 PART section as v4, 94 bytes, and v3 and older are rejected.

## Checks

Unit tests 1,093 passed and 45 skipped. Revert-and-fail for both commits. A game test runs three shipped records through the interpreter. MM6 first-map gate 25 children passed, including the MM7 controls. MM6 default suite 94 passed. MM7 `MM7*:VendorRefreshIntegration.*` 107 passed. `check_style` clean.

## Integration order

Merge after `claude/mm6-new-game`, then rerun the new-game gate children, because the PART bump changes the MM6 save written by production New Game.

## Follow-ups

- An input-driven Town Hall test once the award-animation crash is fixed (`claude/mm6-award-anim`). That event crashes before its reputation step today.
- MM6 reputation display in the quick reference, and the other producers: temple donation, peasant kills, attacking friendlies, jail, theft, the decay and clamp.
- Whether shipped whole-party gold writes should also land once per character as in the original.

Privacy: addresses, IDs and counts only. No game text or data in Git.
