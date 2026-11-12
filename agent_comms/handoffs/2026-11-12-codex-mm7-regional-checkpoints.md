# MM7 regional fresh-process checkpoints handoff

- Owner: Codex, sole MM7 delivery owner.
- Branch/worktree: `codex/mm7-dark-chain` in `mm7-combat-fixes`.
- Code commits: `5bdb1d87e`, `3fe0ab3d6`, `01e549a8f`.
- Integration: neutral `master` merge `8e4d5fffb`; wiki `57cb4ad78`; log `2d3f76c2b`.

## Scope

Season's Stole, Kerrid's letter and Parsons' Quill each have an exact disabled
writer and fresh-process reader under `MM7SideQuestCheckpoint`. Writers run the
real covered route, leave dialogue, use production MM7 Save for
`saves/save000.mm7`, and require structural preflight. Readers select the slot
through the normal Load menu and assert durable map, quest, item, reward and
award state. Kerrid includes the all-food cost; Parsons includes final Erathia
reputation -5. Deterministic checkpoint-only XP baselines make exact persisted
reward assertions independent of random starting XP.

## Evidence

- The ordinary Season, Kerrid and Parsons route tests each passed after helper refactoring.
- All six exact writer/reader processes passed in fresh marked owner-only profiles.
- Every save digest was unchanged after its reader.
- Writer admission without the temporary profile and reader admission without a candidate save were rejected before startup.
- `MM7SideQuests.*:MM7Promotions.*` passed 10/10 headlessly.
- The full `check_style` target passed 157/157.
- Disposable profiles were removed. Private data, saves, checksums and raw logs stayed outside Git.

## Boundary and next work

These are three independent completed-route checkpoints, not continuous
regional progression. The next claimed headless audit slice is Missing
Contestants: physical Contestant Shield acquisition, a meaningful missing-item
refusal, Markham return/rewards and its own fresh-process durability boundary.
Graphical installed-app checks remain deferred under the owner's headless-only
instruction.
