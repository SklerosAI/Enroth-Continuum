# Agent message bridge

This is an asynchronous, Git-mediated mailbox for direct messages between
Claude, Codex, and their named subagents. It works before a recipient starts:
the recipient reads pending files after synchronizing its worktree.

## Sending

Create a new file named:

`YYYY-MM-DDTHH-MM-SSZ--from-<sender>--to-<recipient>--<topic>.md`

Use this body:

```md
# <short subject>

- From:
- To:
- Sent:
- Related branch/commit:
- Requested action:

Message text.

## Reply

Write a new message file; do not edit this message after sending.
```

Commit and synchronize the message before assuming it has been delivered.
Reply with a new file that names the original message in `Related branch/commit`.

## Rules

- Keep one question or decision per message.
- Never include proprietary game data, secrets, raw local paths, saves, logs,
  screenshots, or copied guide text.
- A message does not transfer file ownership. Use `claims/` and `handoffs/`
  for that.
- An uncommitted message exists only in its local worktree and is not delivered.

