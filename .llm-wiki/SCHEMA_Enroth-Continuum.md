---
type: reference
up: "[[Home_Enroth-Continuum]]"
---

# Wiki Schema — Enroth Continuum

> Conventions and workflows for LLM maintenance of this wiki.
> Generated from the University of Notre Dame CRC [llm-wiki-memory-template](https://github.com/crcresearch/llm-wiki-memory-template) `wiki/init-wiki.sh` on 2026-11-05, then adapted to the tracked `.llm-wiki/` layout used by [TDNet](https://github.com/T-Burton-ND/TDNet/tree/main/.llm-wiki). The underlying idea is the [llm-wiki pattern](https://github.com/tobi/llm-wiki).

## Purpose

This wiki is a persistent, compounding knowledge base. The LLM writes and maintains all pages. The human curates sources, directs analysis, and asks questions. Knowledge is compiled once and kept current, not re-derived every session.

## Source of Truth

**Raw sources** (authoritative; read before updating wiki claims):
- Tracked project code and documentation
- Public external references, with links and dates
- Private local game data only for local implementation checks, never for copying into the wiki

**The wiki** (LLM-owned — create, update, cross-reference, maintain):
- All `.md` files in this directory

This wiki is tracked inside the public Enroth repository so Codex can use it without a separate GitHub Wiki checkout. It does not include the CRC template's Claude/Cursor overlays or optional knowledge-graph tooling. Never copy private GOG assets, installers, saves, mods, logs, asset catalogs, or proprietary guide text here. See [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).

## Page Format

Every content page should include:

1. **Title** — `# Page Name` as H1
2. **Opening line** — One sentence summarizing what this page is about
3. **Body** — Tables, prose, code blocks as appropriate. Concise reference style.
4. **Cross-references** — `See also:` line at the bottom with `[Display Name](Page-Name.md)` links

## Frontmatter

Every page gets standard YAML frontmatter:

```markdown
---
type: concept | entity | source-summary | synthesis | analysis | decision | index | comparison | untyped
up: "[[Parent-Page]]"
tags: [topic-a, topic-b]
---
```

**Required fields**:
- `type:` — what kind of page this is (use `untyped` if unsure)
- `up:` — parent page in the hierarchy (usually a category page or index)

**Optional typed edges** (add when the relationship is clear):
- `source:` — literature or raw source this page summarizes
- `extends:` — concept or page this builds upon
- `supports:` / `criticizes:` — claim or page this provides evidence for or against
- `related:` — lateral connection (prefer specific edges above when possible)

**Rules**:
- Every page gets frontmatter — no exceptions
- Use `type: untyped` rather than skipping frontmatter entirely
- Cross-references in frontmatter use `[[Page-Name]]` wikilink format for Obsidian compatibility
- Cross-references in body text use `[Display](Page-Name.md)` for reliable links in the tracked repository
- Typed frontmatter preserves relation semantics; no knowledge-graph build is installed here

## Page types

Most page types (`concept`, `entity`, `synthesis`, etc.) have no required structure beyond the page format. Two query-driven types do, because they exist to capture content that would otherwise be dropped on the floor mid-session.

### `analysis`

A query-driven assessment or evaluation. Use when the user asks a synthesis question (`why X`, `compare A and B`, `should we ...`) and the answer is fileable.

**Required sections**:
1. **Question** — the synthesis question being answered, verbatim or close to it
2. **Context** — what background drove the question
3. **Analysis** — the body of the assessment
4. **Conclusion** — the bottom line
5. **Open follow-ups** — what is unresolved

**Required frontmatter**: `derived_from:` listing the source pages synthesised (one or more wikilinks). Without it, the analysis is unprovenanced.

### `decision`

A design choice with rationale. Use when the project picks one option over alternatives and the reasoning should outlast the decision.

**Required sections**:
1. **Question** — the choice being made
2. **Options considered** — each option with pros / cons
3. **Decision** — what was chosen
4. **Rejected alternatives** — what was not chosen, and why
5. **Revisit triggers** — conditions under which the choice should be re-opened

**Required frontmatter**: `decided_at: YYYY-MM-DD`. **Optional**: `superseded_by: [[Page]]` once a later decision replaces this one.

## Naming Convention

- Use `Title-Case-Hyphenated.md` for page files (e.g., `Neural-Embeddings.md`)
- Navigation files are namespaced: `index_Enroth-Continuum.md`, `log_Enroth-Continuum.md`, etc.

## Special Files

### index_Enroth-Continuum.md
- Catalog of every page with one-line descriptions
- Organized by category
- **Update on every ingest**

### log_Enroth-Continuum.md
- Append-only chronological record
- Format: `## [YYYY-MM-DD] verb | Subject` (verbs: ingest, query, lint, update, create)
- The first bullet of every entry is the `- by:` attribution line (see Log Entry Attribution)
- Then 2-5 bullet points describing the operation
- **Append on every operation**, and commit each entry on its own (see Log Entry Attribution)

### Home_Enroth-Continuum.md
- Human-facing entry point. Project description + a `## Categories` section mirroring the top-level categories in `index_Enroth-Continuum.md`, with 1-3 representative links per category. **Not** a comprehensive catalog; that is the Index's job.
- **Update when a new top-level category emerges in the Index, or when a page lands that is significant enough to be one of its category's representative links.** Routine page additions inside an existing category: Index-only, no Home update needed.
- Distinct from `Home.md` (the template's short redirect), which is normally left alone.

### Home.md
- Short redirect to [Home_Enroth-Continuum](Home_Enroth-Continuum.md)

## Cross-Referencing

- Use `[Display Text](Page-Name.md)` format in body text
- Use `[[Page-Name]]` wikilinks in frontmatter edge fields (Obsidian graph)
- Every page should have at least 2 inbound and 2 outbound links
- Link concepts on first mention within a page
- Bidirectional: if A links to B, B should link back to A

## Edges as Interface Operations

A typed edge is not a label that says "these two things are related." It is an **interface contract** that defines an *operation* the agent should perform when it traverses the edge. Each edge type has an expected behavior, an expected target type, and a semantic commitment that shapes how downstream retrieval and reasoning should handle it.

| Edge | Inverse | What it licenses the agent to do |
|---|---|---|
| `extends:` | `extendedBy` | Inherit semantic context from the parent. Treat the parent's claims as background assumptions for the current page. |
| `supports:` | `supportedBy` | Evidence aggregation. When traversing this edge, expect to combine claims; consistency across supporting pages is desirable. |
| `criticizes:` | `criticizedBy` | Contradiction detection. Expect an unresolved tension that should trigger conflict-resolution logic before combining evidence. |
| `source:` | (none — external) | Grounding check. The target is an external source; verify that any cited claim traces back to the source. |
| `up:` | (none — implicit) | Parent / breadcrumb. Navigate upward in the hierarchy for category context. |
| `related:` | (none — symmetric) | Fallback — no specific operation contract. Prefer a more specific edge type where one applies. |

**Inverse predicates are not authored.** Use the forward relation in frontmatter only when the claim is justified. Add a body backlink when it helps readers navigate. The 16-predicate vocabulary is in [Edge Types](Edge-Types.md). No knowledge-graph pipeline is installed in this repository.

## Cross-reference retrieval

For relationship questions, follow the index, typed frontmatter, and body links. For content questions, read the cited source file and the relevant wiki page. When the wiki and a current source disagree, correct the wiki and record the source date. Keep links reciprocal where useful, without inventing edges to meet a quota.

## Log Entry Attribution

The wiki is a shared memory across a team. Every log entry records who performed the operation, so provenance is answerable on the page itself and not only through `git blame`.

**The `by:` field.** The first bullet of every `log_Enroth-Continuum.md` entry is an attribution line:

```
- by: <human> via <agent>
```

- `<human>` is the value of `git config user.name` in the wiki repo at the time of the operation. Read it; do not invent it. If it does not match the identity that commits the entry, that is a bug to fix, not a value to guess.
- `<agent>` is the coding assistant the operation ran under, for example `claude-code` or `cursor`.

**One commit per log entry.** Never bundle multiple log operations into a single commit. Each append to `log_Enroth-Continuum.md` is committed on its own: commit the page and index changes first, then the log entry as its own commit. This keeps `git blame` on the log file a faithful per-entry record, one entry mapping to one commit and one author.

**Two records, one source of truth.** The `by:` field is the human-readable copy; git history is the verifiable record. They should always agree. If they disagree, trust git and correct the field.

## Operations

### Ingest (new work completed)

When new sources, experiments, or milestones arrive:

1. Read the source material
2. Discuss key takeaways with the user
3. Create new pages or update existing ones (with frontmatter)
4. Update cross-references on all affected pages
5. Update `index_Enroth-Continuum.md`
6. Append to `log_Enroth-Continuum.md`

A single ingest typically touches 5-15 pages.

### Query (answering questions)

1. Read `index_Enroth-Continuum.md` to find relevant pages
2. Read those pages and synthesize an answer
3. If the answer is valuable and reusable, offer to file it as a new page

### Lint (health check)

Periodically check for:
- Orphan pages (no inbound links)
- Dead links (links to non-existent pages)
- Stale claims (superseded by newer work)
- Missing pages (concepts mentioned but lacking their own page)
- Missing cross-references
- **Pages missing frontmatter** — add it based on page content (infer type, parent, tags)
- **Pages with `type: untyped`** — review and assign a proper type if now obvious

## When to Update

- **Always**: After completing significant work (experiments, milestones, key findings)
- **Always**: When findings contradict or supersede previous ones
- **Often**: When analytical questions produce reusable answers
- **Periodically**: Lint pass every few sessions

## When NOT to Update

- Routine debugging or code fixes (git history is enough)
- Temporary analysis that won't be referenced again
- Speculative plans that haven't been executed

## Git Workflow

After wiki updates:
1. Stage changed files by name
2. Commit with descriptive message
3. Do NOT push unless the user requests it

## Evolution

Update this schema as the project's needs change. It's a living document.
