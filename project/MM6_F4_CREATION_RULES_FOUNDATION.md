# MM6 F4 creation and class-rule foundation

Status: **offline implementation, 2026-11-06; not a playable MM6 or a verified original HP/SP formula**. This seam consumes the corrected, ignored F3 PE v2 creation arrays without copying their exact values into tracked source. The [F3 executable follow-up](MM6_F3_EXECUTABLE_FOLLOWUP.md) supplies the eighteen-class mana-attribute dispatch. No original file was reopened or MM6 process launched for this work.

## Private data boundary

Run `scripts/mm6_f4_export_creation_rules.py` with the ignored `F3_PE_RECONCILIATION_2026-11-05_v2.json` as `--report` and the MM6 profile's `rules/creation.json` as `--output`. The exporter checks the corrected report status, six family vectors, eighteen factors, seven stats per family, 31 skill codes per family, 73 base stat points, two granted skills and nine selectable skills. It writes a new mode-0600 file and refuses overwrite. The runtime reads this derived manifest from the MM6 user profile through `ufs`; it does not read the original executable. The profile path is determined by `resolveMm6UserPath`, and its exact numeric contents remain outside Git.

The MM6 engine fails early if this private manifest is absent or invalid. It validates the format, bounded array dimensions and values before publishing the rules object. MM7 does not read this file. Regenerate it only when the authoritative F3 report changes and after reviewing the new source identity.

## Implemented MM6-only paths

- Six families and three tiers are represented as MM6 class IDs 0–17; Create Party exposes only the six starting IDs. The MM6 class and skill text parsers accept the original two-column and five-column rows, including source skill ID 30 mapped to the runtime Learning slot instead of MM7 Dodge. The six source-backed class icons are selected by family.
- Class change applies the F3 family starting stats and granted skill codes, clears old visible skills, and leaves nine eligible optional choices. The shared 50-point pool uses one-point changes, base minus two as the lower bound and 25 as the upper bound.
- HP/SP uses the private family bases and class factors. Mana chooses None, Personality, Intellect or both by the executable's eighteen-class dispatch; the MM6 Cleric ID no longer falls into the inherited MM7 no-mana branch.
- A bounded read-only follow-up on the F3-identity-matched original executable confirmed that the final maximum-health result is clamped to **at least 1**. The MM6 branch now uses that floor; MM7 keeps its own rule. This alone does not establish every preceding bonus or promotion term.
- Bodybuilding and Meditation bonuses now multiply by the same private MM6 class-specific HP/SP factors used in the provisional maximum formulas. The inherited MM7 four-ID class tables no longer contribute these two bonuses to MM6 characters.
- Reset and default party use legal MM6 class IDs and the previously verified portrait range. The current default composition remains **provisional**. A later [static Quick Start audit](MM6_F4_QUICK_START_AUDIT.md) recovered the original route and four characters' class, face, name reference, stats and skills, while inventory, voice and final spell state remain unproved. None of that partial fixture has been applied to the runtime.

## Verification and remaining work

The exporter has three synthetic tests covering valid private output, refusal to overwrite and malformed source shape. An offline check against the ignored F3 reports verified the private profile manifest, all eighteen mana groups, six family mappings, retained 18/31 class/skill text rows and six icon names. The five modified C++ translation units passed syntax-only compilation with the project's compile commands; no native MM6 build or UI render was claimed.

After the Bodybuilding/Meditation factor route was added, the arm64 MM6, MM7 and unit targets built from the integrated tree. The ignored private creation manifest has eighteen HP and eighteen SP factor entries. The complete HP/SP formula and live character sheet are still unverified.

[GrayFace’s mechanics reference](https://grayface.github.io/mm/mechanics/) states that Endurance’s stat effect is multiplied by the class HP factor, while Intellect and/or Personality effects use the class mana factor. This independently supports the factor × stat-bonus shape already used by the MM6 branch. It does not establish base HP/SP, level indexing, promotion retroactivity, equipment/buff order or the displayed final value.

The [HP/SP initialization audit](MM6_F4_CREATION_HP_SP_INIT_GATE.md) now matches the original Quick Start health terms at a structural level and moves the current MM6 creation refill after all starting-item grants, while preserving MM7 timing. The exact SP bonus and promotion formula remains to be traced and measured; current code combines the confirmed F3 base/factor arrays with the inherited bonus structure **provisionally**. Other class consumers, mastery ceilings, training services, starter items/spells, complete Quick Start state and input route, class promotion events, skill ID serialization, generated inventory and full screen/input behavior are still F4/F5 work. The [F4 build graph](MM6_F4_BUILD_GRAPH.md) and [F3 gate](MM6_F3_GATE_REVIEW.md) remain the acceptance authority. No MM7 or MM8 runtime was changed.
