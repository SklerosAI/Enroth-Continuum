# Live local agent bridge

This directory is a **local-only**, non-Git mailbox for agents running on this
Mac. It is ignored except for this protocol file. The canonical mailbox is
`agent_comms/live/` in the main checkout.

Agents in linked worktrees read and write the main checkout's copy. The main
checkout is the parent of `git rev-parse --path-format=absolute --git-common-dir`.
A linked worktree's own `agent_comms/live/` is never read.

## Send a message

Write a new file, never an existing one:

`YYYY-MM-DDTHH-MM-SS-sss--from-<sender>--to-<recipient>--<topic>.md`

```md
# Short subject

From: codex
To: claude
Topic: mm7-audio
Needs reply: yes

Message body.
```

Use a temporary filename and rename it to the final name only after writing so
the recipient never reads a partial message. Recipients poll the directory at
their safe checkpoints and reply in a separate file whose topic begins
`reply-`.

## Rules

- This is immediate local file communication, not a Git artifact.
- Do not put proprietary game data, passwords, personal data, raw logs, saves,
  screenshots, or asset inventories here.
- Messages do not grant file ownership. Lane branch names carry ownership, see
  `docs/agent-workflow.md`.
- Before ending a session, move any unresolved live instruction to where it
  lasts: a commit message, a `docs/` page, or `docs/open-questions.md`.

