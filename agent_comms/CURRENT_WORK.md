# Current work board — 2026-11-23

The owner's delivery order remains MM7 working, MM6 working, full MM7 audit,
then full MM6 audit. Stage 1 closed on 2026-11-11 and Stage 2 is active. This
board is the current coordination surface, and `PLAN.md` holds the stage
gates. Live notes between the agents go in the ignored `agent_comms/live/`
mailbox.

## Title authority and worktrees

- Codex is the sole arbiter and delivery owner for MM7. Codex assigns and
  accepts MM7 work.
- Claude Code is the sole arbiter and delivery owner for MM6. Claude assigns
  and accepts MM6 work.
- There is no worktree or branch count cap while a round is running. The
  10 GB free-disk floor is hard. Each round ends with one branch and one
  worktree per title. `tools/check_worktree_budget.py` still enforces an
  older five-per-owner cap, so read its counts, not its verdict.

Current checkouts: the main checkout on `codex/mm7` and
`enroth-worktrees/mm6-control-center` on `claude/mm6`. No lane worktrees.

## Active and queued work

| Title | Owner | Current state | Next coordination point |
| --- | --- | --- | --- |
| MM7 | Codex (idle since 2026-11-22) | Working gate closed 2026-11-11. Light and Dark chains C0-C7, both endings, every promotion and every map have headless coverage. `ECv0.1.0` and `ECv0.1.1` are tagged. The installed Atlas carries a Release MM7 helper from `014514250`. Fortune's Turn (with chest reroll) and the default-off Dragoon class are integrated. | The full MM7 audit has not started. Its start date is the owner's call, see `project/MM7_FULL_AUDIT_NEXT_WEEK_PLAN.md`. The playthrough verdict is in `.llm-wiki/MM7-Playthrough-Go-No-Go.md`. |
| MM6 | Claude Code | Stage 2 active on `claude/mm6`, pushed to `origin` on 2026-11-23. Chain C0-C9 passes headlessly. An end-to-end playthrough is NO-GO: reading a spellbook, scrolls, wands, enchanted weapons and a few events end the game. | Fix the playthrough blockers, B1 first. See `docs/mm6-playthrough-go-no-go.md` and `docs/mm6-campaign-status.md` on `claude/mm6`. |
| Integration | Both title owners | `master` is neutral and has neither title's latest work. `claude/mm6` consolidated its docs into `docs/` on 2026-11-19, while `codex/mm7` still uses `.llm-wiki/` and `project/`. | Reconcile the two doc layouts when the branches next meet on `master`. |
