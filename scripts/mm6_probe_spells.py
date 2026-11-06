#!/usr/bin/env python3
"""Compare retained MM6 spell IDs with the inherited MM7 spell machinery.

Read-only. The per-spell report contains owner-provided game names and must be
written inside the Git-ignored MM6 reference directory. Name comparison is a
lexical signal, not a determination that two spells have equal behavior.
"""

import argparse
import csv
import hashlib
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


def normalized_name(value: str) -> str:
    return re.sub(r"[^a-z0-9]", "", value.casefold())


def enum_spell_ids(path: Path) -> dict[int, str]:
    pairs = re.findall(r"\b(SPELL_[A-Z0-9_]+)\s*=\s*([0-9]+)\b", path.read_text(encoding="utf-8"))
    return {int(raw): name for name, raw in pairs if 1 <= int(raw) <= 99}


def inherited_spell_data(path: Path) -> dict[str, tuple[int, int, int, int]]:
    source = path.read_text(encoding="utf-8")
    rows = re.findall(
        r"\{\s*(SPELL_[A-Z0-9_]+)\s*,\s*SpellData\(\s*"
        r"(-?[0-9]+)\s*,\s*(-?[0-9]+)\s*,\s*(-?[0-9]+)\s*,\s*(-?[0-9]+)\s*,",
        source,
    )
    return {name: tuple(map(int, mana)) for name, *mana in rows}


def mm6_spell_rows(path: Path) -> list[dict]:
    result = []
    for line, cells in enumerate(csv.reader(path.read_bytes().decode("cp1252", errors="replace").splitlines(),
                                            delimiter="\t"), start=1):
        if not cells or not cells[0].strip().isdigit():
            continue
        if len(cells) < 12:
            raise ValueError(f"spells.txt line {line} has only {len(cells)} columns")
        result.append({"id": int(cells[0].strip()), "name": cells[2].strip().strip('"'),
                       "damage_tag": cells[3].strip(),
                       "mana_novice_expert_master": [int(cells[i].strip()) for i in (5, 6, 7)],
                       "source_line": line})
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="output JSON inside that directory")
    args = parser.parse_args()
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if root not in report.parents:
        parser.error("report must remain in the private reference directory")
    repo = Path(__file__).resolve().parents[1]
    paths = {
        "mm6_spells": root / "icons/spells.txt",
        "inherited_enum": repo / "src/Engine/Spells/SpellEnums.h",
        "inherited_data": repo / "src/Engine/Spells/Spells.cpp",
        "inherited_casting": repo / "src/Engine/Spells/CastSpellInfo.cpp",
    }
    missing = [str(path) for path in paths.values() if not path.is_file()]
    if missing:
        parser.error("missing inputs: " + ", ".join(missing))

    mm6 = mm6_spell_rows(paths["mm6_spells"])
    enum = enum_spell_ids(paths["inherited_enum"])
    data = inherited_spell_data(paths["inherited_data"])
    if sorted(row["id"] for row in mm6) != list(range(1, 100)):
        raise ValueError("MM6 spells.txt does not enumerate IDs 1 through 99 exactly once")
    if sorted(enum) != list(range(1, 100)):
        raise ValueError("inherited SpellId enum does not enumerate IDs 1 through 99 exactly once")
    if set(data) != set(enum.values()):
        raise ValueError("inherited SpellData table does not cover exactly the 99 enum spell names")

    inherited_name_ids = defaultdict(list)
    for spell_id, enum_name in enum.items():
        # The school is the second token; the rest is the spell's English label.
        inherited_name_ids[normalized_name(enum_name.split("_", 2)[2])].append(spell_id)
    casting_source = paths["inherited_casting"].read_text(encoding="utf-8")
    casting_cases = set(re.findall(r"\bcase\s+(SPELL_[A-Z0-9_]+)\s*:", casting_source))

    classifications = Counter()
    mana_matches = 0
    damaging_tags_currently_default_physical = []
    results = []
    for row in mm6:
        spell_id = row["id"]
        enum_name = enum[spell_id]
        inherited_label = enum_name.split("_", 2)[2].replace("_", " ").title()
        mm6_key = normalized_name(row["name"])
        inherited_key = normalized_name(inherited_label)
        if mm6_key == inherited_key:
            classification = "same_normalized_label"
        elif inherited_name_ids.get(mm6_key):
            classification = "label_at_other_inherited_id"
        else:
            classification = "label_absent_or_renamed"
        classifications[classification] += 1
        inherited_mana = data[enum_name]
        same_mana = tuple(row["mana_novice_expert_master"]) == inherited_mana[:3]
        mana_matches += same_mana
        tag = row["damage_tag"].casefold()
        # SpellStats::Initialize recognizes full school names plus 'magic';
        # 'Elec', 'Cold', and 'Pois' therefore fall through to DAMAGE_PHYSICAL.
        wrong_damage_fallback = tag in {"elec", "cold", "pois"}
        if wrong_damage_fallback:
            damaging_tags_currently_default_physical.append(spell_id)
        results.append({**row, "inherited_enum": enum_name,
                        "inherited_label": inherited_label,
                        "name_classification": classification,
                        "inherited_ids_with_mm6_label": inherited_name_ids.get(mm6_key, []),
                        "inherited_mana_novice_expert_master_grandmaster": list(inherited_mana),
                        "same_first_three_mana_costs": same_mana,
                        "damage_tag_defaults_to_physical_in_current_parser": wrong_damage_fallback,
                        "enum_name_appears_in_casting_case": enum_name in casting_cases})

    summary = {
        "mm6_spells": len(mm6),
        "name_classifications": dict(sorted(classifications.items())),
        "same_first_three_mana_costs": mana_matches,
        "different_first_three_mana_costs": len(mm6) - mana_matches,
        "mm6_damage_tags": dict(sorted(Counter(row["damage_tag"] for row in mm6).items())),
        "nonphysical_damage_tags_defaulting_to_physical": len(damaging_tags_currently_default_physical),
        "ids_with_nonphysical_damage_fallback": damaging_tags_currently_default_physical,
        "inherited_enum_names_appearing_in_casting_cases": sum(row["enum_name_appears_in_casting_case"]
                                                                for row in results),
    }
    output = {"format": 1, "summary": summary, "spells": results,
              "input_sha256": {name: hashlib.sha256(path.read_bytes()).hexdigest()
                               for name, path in paths.items()}}
    report.write_text(json.dumps(output, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
