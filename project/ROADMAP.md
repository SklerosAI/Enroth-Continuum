# Enroth Continuum roadmap

This document records intent only; it does not claim implementation.

## Foundation

- Maintain a healthy, attributable OpenEnroth fork.
- Keep a native Apple Silicon macOS build as a first-class validation target.
- Give owners a local-only place to organize their MM6, MM7, and MM8 data.
- Preserve original, unmodified campaign play as the default.

## Trilogy support

OpenEnroth currently supports playable MM7. MM6 and MM8 work belongs to the
upstream compatibility roadmap. Enroth Continuum may contribute compatible,
well-scoped engine work, but will not misrepresent planned support as complete.

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
