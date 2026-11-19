# Docs schema

The rules for writing and maintaining `docs/`, the project's single knowledge base for people and agents.

`docs/` follows the LLM wiki pattern of the University of Notre Dame CRC
[llm-wiki-memory-template](https://github.com/crcresearch/llm-wiki-memory-template): a schema, an index, an
append-only log and topic pages, kept current as work lands rather than re-derived every session. The
[llm-wiki pattern](https://github.com/tobi/llm-wiki) is the underlying idea.

## Source of truth

Current source and tests outrank every claim in `docs/`. When a page and the code disagree, the code is right and
the page is a bug. Fix the page in the same change that finds the disagreement.

The order of authority is:
1. Current source, tests and build files on the branch you're on.
2. The original game executables and data, read locally and never copied into the repository.
3. Public external references, linked with a date or a pinned version, for example the GrayFace patch.
4. `docs/`.

Commit messages hold the reasoning behind a change. `docs/` holds durable facts. Neither repeats the other.

## Layout

| File | Purpose |
|---|---|
| [README.md](README.md) | Home and entry point. Reading order, page map, history pointer. |
| [SCHEMA.md](SCHEMA.md) | This file. |
| [index.md](index.md) | Catalog of every page with a one-line summary, by category. |
| [log.md](log.md) | Append-only record of dated milestones, one line each. |
| `*.md` | Topic pages, one subject each. |

Topic page names are lowercase and hyphenated, like `mm6-save-format.md`. Title-specific pages start with the title,
`mm6-` or `mm7-`. There are no subfolders. A page that grows past what one reader wants splits into two pages, not a
folder.

## Page format

- `# Title` in sentence case, then one sentence saying what the page covers.
- `##` sections. Tables where they help.
- A `## Source` section near the end that links the key files and tests, relative to `docs/`, like
  `[MM6Executable.h](../src/Engine/MM6/MM6Executable.h)`.
- The last line is `See also:` with links to related pages.
- No YAML frontmatter.

## What goes on a page

A page states facts the code can't say: binary layouts, original-game behaviour, why a rule is what it is, where the
original-executable evidence lives, how pieces fit together. It links into source for the rest. It does not:
- walk through a function body the reader can open,
- narrate how a feature was built, packet by packet,
- carry test counts, gate tallies or other numbers that go stale on the next merge,
- claim anything about game data or runtime behaviour that nobody checked. MM6, MM7 and MM8 claims name the title.

Status lives on the status pages ([MM6 campaign status](mm6-campaign-status.md), [MM7 status](mm7-status.md),
[open questions](open-questions.md)). Topic pages describe how things work.

## Writing rules

- No semicolons in prose. Write two sentences.
- Short and plain, present tense. No emojis.
- Prose wraps at about 120 columns. No line exceeds 200, the repository-wide limit from [HACKING.md](../HACKING.md).
- A past bug is named as a bug, not as a choice.
- Never copy proprietary game data, guide or manual text, saves, logs, screenshots or asset inventories. See
  [source and privacy](source-and-privacy.md).

## Operations

**Update.** When a change alters a fact on a page, the same change updates the page. When it adds a durable fact
that no page holds, add it to the page that owns the subject, or create a page and list it in [index.md](index.md).

**Log.** When a dated milestone lands, such as a release, a campaign chain closing, or a lane batch merging into an
integration branch, append one line to [log.md](log.md). Routine fixes don't get a line. Git history has them.

**Status.** When a lane merges, its result goes into the title's status page, replacing the line it supersedes.
There are no per-lane handoff files.

**Lint.** Every few sessions, check for pages that no other page links to, dead links, claims the code contradicts,
and subjects mentioned on several pages that deserve one of their own.

See also: [README](README.md), [index](index.md), [log](log.md), [agent workflow](agent-workflow.md)
