# Live local agent bridge

This directory is a **local-only**, non-Git mailbox for agents running on this
Mac. It is ignored except for this protocol file. Use the canonical path:

`/Users/tburton2/Desktop/Repos/Enroth-Continuum/agent_comms/live/`

Agents in any worktree may read and write this exact path. Do not use a
worktree-relative `agent_comms/live/` path unless that worktree is the main
integration checkout.

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
- Messages do not grant file ownership; record durable ownership in the tracked
  `agent_comms/claims/` and `agent_comms/handoffs/` files.
- Before ending a session, convert any unresolved live instruction into a
  tracked handoff so it survives cleanup or a restarted agent.

