# MM6 damage channels handoff

- Owner: Claude Code lane, reviewed by Claude lead. Recipient: the integration lane, or Codex on its return.
- Branch: `claude/mm6-damage-channels` in the `mm7-save-durability` checkout, commit `ecf9d182f` from `master` `e4ad7b90a`. Not pushed.
- Shared code, thin: `src/Engine/Evt/EvtInstruction.cpp` translates the MM6 kind byte when parsing, and `src/Engine/Evt/EvtInterpreter.cpp` drops an MM6 damage gate (three lines). The rest is in `src/Engine/MM6/`.

## Cause and original channels

MM6 numbers damage kinds 0 Physical, 1 Magic, 2 Fire, 3 Electricity, 4 Cold, 5 Poison, 6 Energy. The event gate passed only raw bytes 0 and 4, which MM7 reads as Fire and Physical, so the two passing kinds were misapplied too: Physical rolled fire resistance and Cold had none. The original reads a per-channel resistance from stats, items, a party buff and the Enchanter bonus. Physical returns damage unchanged, Energy has zero resistance but still rolls luck, and up to four `rand % (luck + res + 30) >= 30` rolls each halve the damage. Event targets are one character, the current character, all four, or random. Monster attack labels Pois, Magic and Ener use the same numbering. Addresses are in the commit message.

## Checks

- New unit tests for channels, buffs and the roll. Game tests `MM6CombatHeadless.DISABLED_ElectricTrapIsResistedByElectricity` (a shipped t6 trap with per-channel resistances taped) and `DISABLED_PoisonAndMagicMonstersHitParty`. Revert-and-fail on both, and on the kind translation alone.
- Sweep: `pyramid`, `t6` and `d09` now finish with no throw. `t7` gets past its entrance and stops on event spells.
- Unit tests 1,092 passed. MM6 suite 94 passed. MM7 `MM7*:VendorRefreshIntegration.*` 107 passed. MM6 first-map gate passed with the MM7 controls. `check_style` clean.

## Next blockers and follow-ups

- t7 trapped door event 1 ran 12,195 times in four frames and killed the party. A repeat trigger, possibly monster or object face triggers. New lane.
- Event spell dispatch on t7 plates and elsewhere. New lane.
- Energy monster projectiles and Pois or Magic special attacks are still gated.
- When the event targets the current character and none is active, the original hits nobody. We still pick one at random.
- Party protection buff slots must match the original order when MM6 spells start writing them. The MM6 artifact item resistances are not re-checked.

Privacy: addresses, IDs and map stems only. No game text or data in Git.
