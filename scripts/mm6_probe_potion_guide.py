#!/usr/bin/env python3
"""Compare a private manual transcription of Prima's MM6 safe recipes with retained data.

This F2 check never opens an original archive or launches the game. Exact
recipes and disagreements are written only under ignored local-game-data/mm6.
The guide chart and table bytes do not prove the original runtime's effects.
"""

import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="ignored retained MM6 research directory")
    parser.add_argument("report", type=Path, help="ignored detailed JSON report in reference directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    reference = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if reference == allowed or not reference.is_relative_to(allowed):
        parser.error("reference must be an ignored MM6 research directory")
    if not report.is_relative_to(reference) or report.suffix.lower() != ".json":
        parser.error("report must be JSON inside the ignored reference directory")
    if not report.parent.is_dir():
        parser.error("report parent does not exist")

    recipe_path = reference / "prima-potion-recipes-private.json"
    matrix_path = reference / "unloaded-table-probe.json"
    guide_path = reference / "mm6-prima-guide.pdf"
    for path in (recipe_path, matrix_path, guide_path):
        if not path.is_file():
            parser.error(f"missing local input: {path.name}")

    recipe_data = json.loads(recipe_path.read_text())
    matrix_data = json.loads(matrix_path.read_text())["useitems"]
    ids = matrix_data["row_ids"]
    matrix = matrix_data["matrix_tokens"]
    if ids != matrix_data["column_ids"] or len(ids) != 29 or len(matrix) != 29:
        raise ValueError("retained matrix axes are not the expected 29-by-29 grid")
    if any(len(row) != len(ids) for row in matrix):
        raise ValueError("retained matrix row width changed")
    if recipe_data.get("source_pdf_pages") != [238, 239]:
        raise ValueError("guide page provenance changed")

    positions = {item_id: index for index, item_id in enumerate(ids)}
    observed = {}
    for i, first in enumerate(ids):
        for j in range(i + 1, len(ids)):
            token = matrix[i][j]
            if token.isdigit():
                observed[(first, ids[j])] = int(token)

    guide = {}
    for row in recipe_data["recipes"]:
        pair = tuple(sorted((row["first_item"], row["second_item"])))
        if pair[0] == pair[1] or pair in guide or any(item not in positions for item in pair):
            raise ValueError("duplicate, same-item or out-of-axis guide recipe")
        if row["pdf_page"] not in (238, 239):
            raise ValueError("guide recipe has unexpected page locator")
        guide[pair] = row

    comparisons = []
    for pair, row in sorted(guide.items()):
        actual = matrix[positions[pair[0]]][positions[pair[1]]]
        comparisons.append({
            "first_item": pair[0], "second_item": pair[1],
            "guide_result": row["result_item"], "retained_result_token": actual,
            "guide_pdf_page": row["pdf_page"],
            "classification": "match" if actual == str(row["result_item"]) else "disagreement",
        })
    outcomes = Counter(row["classification"] for row in comparisons)
    summary = {
        "guide_recipe_rows": len(guide),
        "retained_numeric_result_pairs": len(observed),
        "guide_pairs_present_as_numeric_pairs": len(set(guide) & set(observed)),
        "guide_pairs_absent_from_numeric_pairs": len(set(guide) - set(observed)),
        "retained_numeric_pairs_absent_from_guide": len(set(observed) - set(guide)),
        "matching_result_rows": outcomes["match"],
        "disagreeing_result_rows": outcomes["disagreement"],
        "non_numeric_token_meanings_verified": False,
    }
    payload = {
        "format": 1,
        "scope": "retained table and manually checked guide chart; no original archive or game runtime opened",
        "warning": "The guide may err; marker effects and original runtime behavior remain unverified.",
        "summary": summary,
        "comparisons": comparisons,
        "guide_pairs_absent_from_numeric_pairs": [list(pair) for pair in sorted(set(guide) - set(observed))],
        "retained_numeric_pairs_absent_from_guide": [list(pair) for pair in sorted(set(observed) - set(guide))],
        "input_sha256": {path.name: sha256(path) for path in (recipe_path, matrix_path, guide_path)},
        "source_sha256": {"scripts/mm6_probe_potion_guide.py": sha256(Path(__file__))},
    }
    report.write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
