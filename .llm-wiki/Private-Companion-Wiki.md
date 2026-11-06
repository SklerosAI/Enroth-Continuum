---
type: concept
up: "[[Agent-Handoff]]"
tags: [privacy, provenance, local-wiki]
---

# Private Companion Wiki

The public LLM wiki is paired with an ignored, owner-local knowledge base at `local-game-data/private-wiki/INDEX.md`, with `mm6/`, `mm7/` and `mm8/` namespaces.

This location is covered by the repository's `local-game-data/*` ignore rule. It exists locally with `INDEX.md`, `mm6/ENGINE_HANDOFF.md`, `mm6/REPORTS.md`, `mm7/STATUS.md` and `mm8/STATUS.md` as of 2026-11-05. It is for exact proprietary-derived inventories, token and field interpretations, per-record discrepancies, local screenshots or trace references, and report navigation that cannot enter public Git. The MM6 report registry points to ignored research outputs rather than duplicating them. The MM7 and MM8 namespaces hold status pointers, not completed audits. **Git does not back up any of this private companion.** Keep an owner-approved local backup and verify it separately. This tracked page defines the schema and navigation boundary, not the completeness of private findings.

## Entry contract

Each private finding should record:

| Field | Requirement |
| --- | --- |
| Identity | Game, subsystem, stable local page name, date, and author/agent. |
| Provenance | Exact owned source or retained report locator, original-file hash when available, tracked probe/source SHA, method and any guide page locator. Avoid unneeded verbatim guide prose. |
| Claim level | Separate **observed bytes/index**, **parser result**, **inferred meaning**, **runtime behavior**, and **campaign play**. Mark uncertainty and competing interpretations. |
| Consumer | C++ path/function and any UI, save or media surface that depends on the finding. |
| Cross-reference | Links to related private pages, the public spec/checklist row, and the reproducible probe or reason no probe exists. |
| Closure | Open question, synthetic/offline check, F3 original-file check, F4 implementation check, F5 graphical/audio/save check, and superseding evidence. |

The private `INDEX.md` should map each subsystem to its private page and report, state which reports are fresh or stale, and point back to the public [MM6 source/probe index](MM6-Source-and-Probe-Index.md). A future agent first reads the public [handoff](Agent-Handoff.md), checks `git status` and source SHA, then reads only the private pages needed for its assigned lane. It returns public aggregate conclusions, not raw private rows, strings or images, to the integration owner.

## Boundary checks

- Keep all private pages, reports, saves, screenshots, recordings and game assets under the ignored `local-game-data/` tree or another owner-approved local-only location. Never use `git add -f` on them, attach them to issues/PRs, or send them to cloud storage or public tools.
- Preserve the private companion and its report registry with an owner-approved local backup. A clean public Git worktree says nothing about whether these ignored files are present or recoverable.
- Check `git check-ignore -v local-game-data/private-wiki/INDEX.md` before adding content. Review `git status --short` and staged paths before every commit; only public summaries and code belong in Git.
- Do not infer original-game bugs from absent current fork handlers, zero-filled baselines, a static graph, or a missing literal load. Carry those uncertainties through to a specific F3/F4/F5 check.
- Preserve OpenEnroth credit and the owner's game rights boundary in [CREDITS.md](../CREDITS.md); do not change the owned archives or source PDFs.

See also: [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Agent Handoff](Agent-Handoff.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Agent Play and Observation Roadmap](Agent-Play-and-Observation-Roadmap.md).
