# MM7 staged candidate validation handoff

- Owner: Codex lane B, `codex/mm7-candidate-validation`.
- Code commit: `bf2569dca` (`launcher/AtlasApp.swift` only). This adds an opt-in, marked, owner-only disposable MM7 profile for a staged Atlas. A set but invalid override refuses Play; an unset override retains the ordinary MM7-Atlas path. The installed app was not replaced.
- Coordination: lane claim released with this handoff. No MM7 runtime, campaign, save implementation, or raid files were edited.

## Checks completed

- Built the current integrated source with network denied into an outside-repository staged Atlas; the arm64 bundle, title lock, signatures, linked media libraries, and notices passed the packaging checks.
- Built `OpenEnroth_GameTest` locally. The disposable fresh-profile Save writer and fresh-process Load reader each passed 1/1; the reader did not change its source slot.
- A staged Atlas launched MM7 with the marked disposable `--user-path`, confirmed by process arguments and open log paths. New Game, party naming, Emerald Island rendering, keyboard movement/turning, guide dialogue, pause/resume, focus loss/return, and the opening movie rendered.
- A named manual save was written through the menu and loaded by a later Atlas-launched MM7 process. The loaded party retained the newly entered character name.
- In the armory, a shield purchase reduced gold from 200 to 50 and placed the item in the buyer's inventory. A second named save was written and loaded by a fresh process; the shop position, shield, and 50 gold persisted.
- A starter Torch Light spell was cast from the Fire spellbook and its world buff icon appeared.
- A copy of one preexisting installed-baseline save loaded in the disposable profile, rendering its distinct party and saved location. The source save was not edited; the local copy still matched it after Load.
- A staged Atlas with a set but invalid disposable-profile override refused Play and launched no MM7 process. All staged game and Atlas windows/processes were closed afterward.

## Open visible checks

- Audible music and effects await owner confirmation.
- One ordinary roaming NPC conversation and one monster fight were not completed in this session. The in-game guide dialogue did render; shop interaction and purchase passed.
- The staged candidate remains separate from the installed app. Installed-app cutover and its existing-save Load are W5 after integration review.

## Local profile side effect and privacy

One computer-use observation after MM7 Exit inadvertently relaunched the staged MM7 executable directly, without Atlas arguments. It opened the normal MM7-Atlas profile. No normal save was loaded or written; its existing save files retained their pre-session modification times. The uniquely new log was removed after the process closed. The normal profile's `enroth-continuum-mm7.ini` was modified at 2026-11-10 10:15:56 local time (26,583 bytes); prior bytes were unavailable, so that file was preserved. This side effect should be disclosed during review, not silently treated as a verified unchanged profile.

Private game data, saves, screenshots, and logs stayed outside Git. No private checksums or asset inventories are recorded here.
