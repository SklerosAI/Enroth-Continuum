# Claude — MM6 evening session, 2026-11-12

- Owner: Claude Code, MM6 owner. Recipient: the next Claude session, and Codex for awareness. Updated after each merge, so the last entry is the current state.
- Owner priority since about 19:45: playtest party creation, New Sorpigal and its dungeons on 2026-11-13. Board: `agent_comms/MM6_PLAYTEST_2026-11-13.md`. The owner also released shared MM6/MM7 code to Claude while Codex is idle (last Codex commit 14:56). Every shared-core merge ran the MM7 controls.

## Merged on master tonight, in order

| Merge | What | Gate evidence |
| --- | --- | --- |
| `5fc13796c` | Chain C5, the Oracle and Crystal Alpha | lane: chain C0 to C5, MM6 179 |
| `77481a2cc` | Control Center access, blasters, the skill chime | lane: MM6 183, MM7 controls 147 |
| `29e0c384e` | Question prompts as MM6.exe, hireling name reset | lane: MM6 184, MM7 controls 147 |
| `ecd098ec8` | Event spells batch 2, six maps unblocked | lane: MM6 179, MM7 controls 147. Master gate: MM6 188, MM7 147, sweep 67/67 clean |
| `4d13f1e35` | Player spells batch 1, 23 spells, MM6 monster resistance | master gate: unit 1123, MM6 200, MM7 147 |
| `743e7e646` | Chain C6, the three castle crystals | lane: chain C0 to C6, MM6 188 |
| `25649b9b7` | Buff slots: Dispel Magic, Shield, buff save gate pair | lane: MM6 202 |
| `a1f307c43` | UI leftovers: guild, bank, stable menus, chest exit, zoom art, professions, start capture | master gate: unit 1123, MM6 208, MM7 147 |
| `381d7ddf4` | Visit stamps on first-map saves and the death map, timer rule pinned | lane: MM6 190, first-map gate green |
| `5001f152c` | Bank deposit and withdraw, services audit | lane: MM6 211 |
| `66e71b82c` | First combat: potions, turn-based mode, volcanic rest cost, no-food sound | lane: MM6 212, MM7 controls 147 |

The first-map gate passed every fresh-process child and the MM7 controls on `f88268730` and again on `6361f74a5` (39 children, with the buff and first-map visit pairs). Master gate on `6361f74a5`: unit 1123, MM6 210, MM7 147. The Release MM6 app in `build-release/` was rebuilt at 20:02 and must be rebuilt again on the final master before the playtest, then launched once to the main menu.

## Parked

- Chain C7 on `claude/mm6-chain-c7`: written and passed on a provisional C6 profile. Drop its `fcff1b354` provisional commit and the CMake line, rebase onto master, run `--through 7 --profile /var/folders/qc/fg92hjk146s3g36x68b8n1lh0000gr/T/mm6-restart-chain-c6rebase` (saves 0 to 6 from the real C6). Then C8 (NPC 12 release) and C9 (hive finale) per the route table in `2026-11-12-claude-mm6-control-center.md`.

## Running lanes at the last update

| Lane | Branch / worktree | Scope |
| --- | --- | --- |
| Services 2 | `claude/mm6-services-2` in `../enroth-worktrees/mm6-leftovers` | guild joins (topics 381 to 397), tavern room, food, drink, tip, temple donate, expert teachers |
| Dungeons | `claude/mm6-dungeons` in `../enroth-worktrees/mm6-chain-c4` | d02, d01, d18 end to end, the town's side quests, dungeon rest |
| Rest rules | `claude/mm6-rest-rules` in `../enroth-worktrees/mm6-spells` | new-party days without rest, Wait until Dawn and the rest encounter, wake-all and recovery across rest, the death rule, the 3 am daily rules, turn-based start order |
| Guild menus | `claude/mm6-guild-menus` in `../enroth-worktrees/mm6-control-center` | Mercenary and Thieves guild menus (patch from services 1), Self guild door, four layout checks, menu focus colours |
| Menus and hirelings | `claude/mm6-menus-hirelings` in `../enroth-worktrees/mm6-indoor-light` | main menu, escape menu and options, hirelings, news and rumours |

Each lane commits on its branch, never pushes, and writes `agent_comms/handoffs/2026-11-12-claude-mm6-<lane>.md`. To integrate one: review the diff, `git rebase master` in its worktree, syntax-check the shared files from `build/compile_commands.json`, `git merge --ff-only`, then run the gate script pattern (unit, `check_style`, MM6 suite, MM7 controls) on the main checkout.

## Known open items for the playtest

- Still "not available yet" until services 2 lands: guild joins, tavern rent room, food, drink and tip, temple donate, expert teachers, Bounty Hunt.
- Empty menus at the Mercenary and Thieves guilds until the guild lane lands.
- Unverified against MM6: text wrapping at the 148 px column, the tall staff in the magic shop's special stock, the grey band under the sell grid, the portrait selection ring.
- Town Portal, Stun, Shield and the rest of the spellbook are refused. Scroll casts are refused.
- Guardian Angel has no shared buff slot. Actor buff 2 is read as Curse where the shared slot is Summoned.
- The `startNewGameThroughInput` test helper exists in three copies.

## Machine

- `~/enroth-tools/env.sh` now caps ccache at 2 GB (it said 8 GB and let the cache grow to 3 GB). Disk was 13 to 20 GiB free all evening.
- The capture loop runs every five minutes until about 00:30 (`touch /private/tmp/enroth-capture-loop.stop` to end it). Chain profiles under `$TMPDIR/mm6-restart-chain-*`: only `c6rebase` is kept.
- `git stash` is shared across worktrees here and two lanes popped each other's entries. Lanes are told to commit WIP instead.
