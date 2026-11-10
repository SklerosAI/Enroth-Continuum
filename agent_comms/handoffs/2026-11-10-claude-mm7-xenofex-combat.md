# MM7 Xenofex real combat handoff

- Owner: Claude Code lane subagent, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/mm7-xenofex-combat` from `master` `00b851628`, commit `0cd1d14aa`. Not pushed or merged.
- Files: new `test/Bin/GameTest/GameTests_MM7XenofexCombat.cpp` (`MM7XenofexCombat.KillAndLeave`), one line in `test/Bin/GameTest/CMakeLists.txt`. No runtime source.

## Behavior covered

One game enters Colony Zod twice. Leaving through the map's real exit face and transition dialog with Xenofex alive leaves `QBIT_XENOFEX_KILLED` clear. Killing him through attack-key input first, without NoDamage, sets it. The quest's `Actor::Die(0)` fixture is replaced for this step.

Data facts found by throwaway probes, MM7 only: the map-leave script checks actor index 0 by ID. Xenofex is that actor, his record's `monsterId` is `MONSTER_INVALID`, `monsterInfo.id` is `MONSTER_DEVIL_C` and about ten other actors share that monster ID, so the test finds him by unique name and asserts exactly one match. Colony Zod has exactly one exit face.

## Fixture

One party-strength fixture after New Game: Champion Sword, Grandmaster Sword 30, Might, Accuracy, Speed and Endurance 200, level 200, healed to maximum. The Pit cube fixture's level 100 without speed or endurance lost the whole party. Only `QBIT_XENOFEX_LIGHT_ACTIVE` is set, because the leave script reads no other quest state. One teleport into melee range.

## Checks

- `MM7XenofexCombat.KillAndLeave` 1/1. Xenofex died after 333 attack presses. Party HP went from 2915 to 1121, one character unconscious.
- Behavior-guard check: with the attacks skipped, the kill iteration fails, and with that assert softened every tape check fails (bit stays clear, no death, party untouched).
- `MM7LightXenofex.*:MM7LightSeam.*` 3/3.
- Full headless MM7 GameTest binary: 531 passed, 2 skipped (MM7Vendor mod-flag), 0 failed.
- cpplint clean on the new file. Full `check_style` not run on this branch.

## Remaining

Travel to Colony Zod is still a teleport. The fixture proves the kill and completion mechanism, not whether a party of realistic level can win, which is stage 3 balance. Chain chapter C6 can reuse `strengthenParty` and the exit helper.

Privacy: no game data, saves or logs in Git.
