# Enroth Continuum roadmap

This document records intent only; it does not claim implementation.

## Foundation

- Maintain a healthy, attributable OpenEnroth fork.
- Keep a native Apple Silicon macOS build as a first-class validation target.
- Give owners a local-only place to organize their MM6, MM7, and MM8 data.
- Preserve original, unmodified campaign play as the default.
- Open a local, HTML-style Mac launcher called The Atlas from the Applications folder.

## The Atlas

The launcher presents MM6, MM7 and MM8 as three campaign cards. It shows the
real readiness of each engine and local data set; MM7 becomes the first Play
button. Its Atelier manages private, per-game mod profiles, including party
portraits and armor settings as those engine overrides become functional. A
prominent Vanilla option bypasses them. The Hall of Echoes hosts the optional
legacy transfer preview. The Vault shows local saves and backups. The screen
is bundled in a native Mac app and uses no online service or public asset
storage. See [plan.md](plan.md) for the staged acceptance checks.

## Trilogy support

The MM7 Mac baseline and first private mod overrides are working in this fork.
MM6 is the active next engine goal, using the owner's local GOG data. MM7 is
frozen as its own complete runtime while MM6 is built from a separate source
copy. MM8 later gets an equally separate runtime. The Atlas ultimately holds
three sibling engine apps and three independent sets of data, saves, mods and
preferences, so all campaigns can work at once. We can diverge from
OpenEnroth's implementation and schedule while preserving its license, history
and credit. The Atlas will offer MM6 only after an observed campaign session
and save/load check; full MM6 support means the campaign can reach its ending.
Cross-game character transfer follows working source and destination games.
See [plan.md](plan.md), section 18.1, for gates and checks.

## Optional legacy campaign

The prospective legacy mode is an opt-in export/import system, not direct save-file
conversion. A proposed conservative preset is:

- imported heroes are capped at level 5;
- imported skills are capped at Expert;
- each imported character may select one eligible heirloom item;
- a destination-specific capped gold allowance applies;
- quest state, keys, map state, and unapproved items never transfer.

The exact mappings, eligible heirlooms, balance rules, and story presentation remain
future design work. Original campaigns must remain available without these rules.
The Hall of Echoes will preview a versioned character blueprint and destination
mapping before a fresh destination save is created. It will keep source saves
untouched and leave transfer unavailable until both games and their adapters
have passed play, export, import, and save/load checks.
