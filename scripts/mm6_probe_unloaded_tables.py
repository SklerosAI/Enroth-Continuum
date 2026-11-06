#!/usr/bin/env python3
"""Inspect six retained MM6 tables without literal C++ loaders.

This is a structural F2 probe. Detailed table text, matrix tokens, and row
differences are written only under ignored local-game-data/mm6. The meanings
of `useitems.txt` tokens and the original game's table-use decisions remain
unproven.
"""

import argparse
import csv
import hashlib
import json
from collections import Counter
from pathlib import Path


UNLOADED = (
    "autonotes.txt", "npcbtb.txt", "passwords.txt",
    "proftext.txt", "useitems.txt", "errorlog.txt",
)
COMPARISON = ("autonote.txt", "npcprof.txt", "items.txt")
SOURCE_FILES = (
    "scripts/mm6_probe_unloaded_tables.py",
    "src/Engine/Engine.cpp",
    "src/Engine/Tables/AutonoteTable.cpp",
    "src/Engine/Tables/NPCTable.cpp",
    "src/GUI/GUIWindow.cpp",
    "src/GUI/UI/UIDialogue.cpp",
    "src/GUI/UI/UIPopup.cpp",
    "src/Engine/Tables/MM6ItemTable.cpp",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def table_rows(path: Path) -> list[list[str]]:
    return list(csv.reader(path.read_bytes().decode("cp1252", errors="replace").splitlines(), delimiter="\t"))


def numeric_rows(rows: list[list[str]]) -> dict[int, list[str]]:
    result = {}
    for row in rows:
        if row and row[0].strip().isdigit():
            key = int(row[0].strip())
            if key in result:
                raise ValueError("duplicate numeric ID in retained text table")
            result[key] = row
    return result


def normalized(cell: str) -> str:
    return cell.strip().strip('"').casefold()


def token_kind(token: str) -> str:
    if token.isdigit():
        return "numeric_candidate_item_id"
    if len(token) == 2 and token[0].isalpha() and token[1].isdigit():
        return "letter_digit_marker"
    if len(token) == 2 and token.isalpha():
        return "two_letter_marker"
    return "other_unclassified"


def compare_rows(left: dict[int, list[str]], right: dict[int, list[str]]) -> dict:
    common = sorted(left.keys() & right.keys())
    changed = []
    for row_id in common:
        left_cells = tuple(normalized(cell) for cell in left[row_id][1:])
        right_cells = tuple(normalized(cell) for cell in right[row_id][1:])
        if left_cells != right_cells:
            changed.append({"id": row_id, "left_cells": left[row_id], "right_cells": right[row_id]})
    return {
        "left_numeric_rows": len(left),
        "right_numeric_rows": len(right),
        "shared_ids": len(common),
        "shared_unchanged_rows": len(common) - len(changed),
        "left_only": [{"id": row_id, "cells": left[row_id]} for row_id in sorted(left.keys() - right.keys())],
        "right_only": [{"id": row_id, "cells": right[row_id]} for row_id in sorted(right.keys() - left.keys())],
        "changed_shared_rows": changed,
    }


def useitems_matrix(rows: list[list[str]], item_ids: set[int]) -> dict:
    indexed = numeric_rows(rows)
    ids = sorted(indexed)
    if len(ids) != 29 or ids != list(range(ids[0], ids[0] + 29)):
        raise ValueError("useitems numeric row axis is not 29 consecutive IDs")
    if not set(ids) <= item_ids:
        raise ValueError("useitems row IDs are absent from items.txt")
    if any(len(row) != 35 for row in indexed.values()):
        raise ValueError("useitems numeric row width is not 35 columns")

    matching_headers = []
    for line_number, row in enumerate(rows, 1):
        if row and row[0].strip().isdigit() or len(row) < 35:
            continue
        axis_cells = [cell.strip().strip('"') for cell in row[6:35]]
        if all(cell.isdigit() for cell in axis_cells) and [int(cell) for cell in axis_cells] == ids:
            matching_headers.append(line_number)
    if len(matching_headers) != 1:
        raise ValueError("useitems has no unique matching numeric column axis")

    matrix = [[indexed[row_id][column].strip().strip('"') for column in range(6, 35)]
              for row_id in ids]
    if any(not token for row in matrix for token in row):
        raise ValueError("useitems matrix contains an empty cell")
    if any(matrix[i][j] != matrix[j][i] for i in range(29) for j in range(29)):
        raise ValueError("useitems matrix is not symmetric")
    diagonal = {matrix[i][i] for i in range(29)}
    if len(diagonal) != 1:
        raise ValueError("useitems diagonal is not uniform")

    unique_tokens = {token for row in matrix for token in row}
    numeric_tokens = {int(token) for token in unique_tokens if token.isdigit()}
    token_classes = {kind: sorted(token for token in unique_tokens if token_kind(token) == kind)
                     for kind in {token_kind(token) for token in unique_tokens}}
    cell_class_counts = dict(sorted(Counter(token_kind(token) for row in matrix for token in row).items()))

    return {
        "row_ids": ids,
        "column_ids": ids,
        "matching_header_line": matching_headers[0],
        "matrix_tokens": matrix,
        "token_frequencies": dict(sorted(Counter(token for row in matrix for token in row).items())),
        "token_classes": token_classes,
        "cell_class_counts": cell_class_counts,
        "numeric_tokens_in_items_table": len(numeric_tokens & item_ids),
        "numeric_tokens_in_matrix_axis": len(numeric_tokens & set(ids)),
        "diagonal_token": next(iter(diagonal)),
        "numeric_row_metadata": [{"id": row_id, "cells_before_matrix": indexed[row_id][:6]}
                                 for row_id in ids],
        "summary": {"rows": 29, "columns": 29, "populated_cells": 841,
                    "matching_header_axes": 1, "symmetric_cells": 841,
                    "distinct_tokens": len(unique_tokens),
                    "numeric_candidate_item_ids": len(numeric_tokens),
                    "numeric_candidates_in_items_table": len(numeric_tokens & item_ids),
                    "numeric_candidates_in_matrix_axis": len(numeric_tokens & set(ids)),
                    "letter_digit_marker_codes": len(token_classes.get("letter_digit_marker", [])),
                    "two_letter_marker_codes": len(token_classes.get("two_letter_marker", [])),
                    "uniform_diagonal": True},
    }


def shape_summary(rows: list[list[str]]) -> dict:
    nonempty = [row for row in rows if any(cell.strip() for cell in row)]
    numeric = numeric_rows(nonempty)
    return {
        "physical_lines": len(rows),
        "nonempty_lines": len(nonempty),
        "numeric_rows": len(numeric),
        "column_widths": sorted({len(row) for row in nonempty}),
        "percent13_cells": sum("%13" in cell for row in nonempty for cell in row),
        "numeric_ids": sorted(numeric),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="retained ignored MM6 research directory")
    parser.add_argument("report", type=Path, help="detailed JSON report inside that directory")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    report = args.report.resolve()
    if not root.is_relative_to(allowed) or root == allowed:
        parser.error("reference must be an ignored MM6 research directory")
    if not report.is_relative_to(root) or report.suffix.lower() != ".json":
        parser.error("JSON report must stay inside the ignored reference directory")
    if not report.parent.is_dir():
        parser.error("report parent does not exist")

    icons = root / "icons"
    input_names = UNLOADED + COMPARISON
    paths = {name: icons / name for name in input_names}
    missing = [name for name, path in paths.items() if not path.is_file()]
    if missing:
        parser.error("missing retained text tables: " + ", ".join(missing))
    tables = {name: table_rows(path) for name, path in paths.items()}
    source_paths = {name: repo / name for name in SOURCE_FILES}
    missing_sources = [name for name, path in source_paths.items() if not path.is_file()]
    if missing_sources:
        parser.error("missing tracked source files: " + ", ".join(missing_sources))

    note_comparison = compare_rows(numeric_rows(tables["autonotes.txt"]),
                                   numeric_rows(tables["autonote.txt"]))
    profession_comparison = compare_rows(numeric_rows(tables["proftext.txt"]),
                                         numeric_rows(tables["npcprof.txt"]))
    matrix = useitems_matrix(tables["useitems.txt"], set(numeric_rows(tables["items.txt"])))
    shapes = {name: shape_summary(tables[name]) for name in UNLOADED}
    summary = {
        "unloaded_tables": len(UNLOADED),
        "useitems_matrix": matrix["summary"],
        "autonote_shared_ids": note_comparison["shared_ids"],
        "autonote_shared_identical": note_comparison["shared_unchanged_rows"],
        "autonote_shared_different": len(note_comparison["changed_shared_rows"]),
        "autonote_singular_only": len(note_comparison["right_only"]),
        "profession_shared_ids": profession_comparison["shared_ids"],
        "npcbtb_percent13_cells": shapes["npcbtb.txt"]["percent13_cells"],
        "password_numeric_rows": shapes["passwords.txt"]["numeric_rows"],
        "errorlog_nonempty_lines": shapes["errorlog.txt"]["nonempty_lines"],
    }
    result = {
        "format": 1,
        "scope": "retained MM6 text tables only; no original archives opened",
        "warning": "Structure does not prove token meanings, original table use, or correct current behavior.",
        "summary": summary,
        "table_shapes": shapes,
        "useitems": matrix,
        "autonote_plural_vs_singular": note_comparison,
        "proftext_vs_npcprof": profession_comparison,
        "input_sha256": {name: sha256(path) for name, path in paths.items()},
        "source_sha256": {name: sha256(path) for name, path in source_paths.items()},
    }
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
