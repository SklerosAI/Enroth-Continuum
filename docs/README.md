# Enroth Continuum docs

The project's knowledge base: how the engine and the MM6 port work, where each title stands, and how to build, test
and contribute. Start here.

Enroth Continuum is a source fork of [OpenEnroth](https://github.com/OpenEnroth/OpenEnroth) that runs Might and Magic
VII and is porting Might and Magic VI, with VIII to follow. The [project overview](project-overview.md) has the scope
and the title boundaries.

## Reading order

For anyone new, human or agent:
1. [Project overview](project-overview.md), for what the fork is and who owns what.
2. [HACKING.md](../HACKING.md), the development rules. It is required reading before any change.
3. [Build and dev setup on macOS](build-and-dev-setup.md) and [testing](testing.md).
4. [Source and privacy](source-and-privacy.md), before touching game data.
5. The status page for your title, [MM6 campaign status](mm6-campaign-status.md) or [MM7 status](mm7-status.md).
6. For MM6 work, [MM6 port architecture](mm6-port-architecture.md) and then the topic page for your area.
7. For agents, [agent workflow](agent-workflow.md).

The [index](index.md) lists every page. The [schema](SCHEMA.md) says how pages are written and kept current. Its
first rule is that current source and tests outrank anything written here.

## Pages

- **Status:** [MM6 campaign status](mm6-campaign-status.md), [MM7 status](mm7-status.md),
  [open questions](open-questions.md), [releases](releases.md)
- **Engine:** [engine architecture and runtime map](engine-architecture.md)
- **MM6 port:** [port architecture](mm6-port-architecture.md), [rules from MM6.exe](mm6-executable-rules.md),
  [characters, items and spells](mm6-characters-items-spells.md), [events](mm6-events.md),
  [combat and monsters](mm6-combat-and-monsters.md), [houses and services](mm6-houses-and-services.md),
  [travel and maps](mm6-travel-and-maps.md), [media and UI](mm6-media-and-ui.md), [save format](mm6-save-format.md),
  [GrayFace patch reference](mm6-grayface-patch-reference.md)
- **Tooling:** [testing](testing.md), [build and dev setup](build-and-dev-setup.md),
  [mods and the Atlas launcher](mods-and-atlas.md)
- **Process:** [agent workflow](agent-workflow.md), [upstream relationship](upstream.md),
  [source and privacy](source-and-privacy.md)

## History

Until 2026-11-19 the project kept its notes in three trees: `.llm-wiki/` (an LLM wiki built on the CRC
llm-wiki-memory-template), `project/` (per-packet reverse-engineering notes, specs and status files) and
`agent_comms/` (Codex and Claude coordination, claims, handoffs and reviews). This folder replaced all three. Their
durable facts were distilled into the pages above, and the per-packet narratives were left in history.

The last commit that has the old trees is `a927bd004`. Read any old file with `git show`, for example:

```
git show a927bd004:project/MM6_SAVE_SPEC.md
git show a927bd004:.llm-wiki/log_Enroth-Continuum.md
git ls-tree -r --name-only a927bd004 -- project .llm-wiki agent_comms
```

The live agent mailbox in `agent_comms/live/` was never tracked and stays where it is. See
[agent workflow](agent-workflow.md).

See also: [index](index.md), [schema](SCHEMA.md), [log](log.md)
