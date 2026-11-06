#!/usr/bin/env python3
"""Validate a private visual guide review against retained MM6 item evidence.

This F2 probe reads ignored extracts and a private visual-review override. It
does not open the original installation. Exact names, values and event links
are written only to an ignored report in the same reference directory.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def normalized(value: str) -> str:
    return "".join(value.split()).casefold()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("override", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    allowed = (repo / "local-game-data/mm6").resolve(strict=True)
    root = args.reference.resolve(strict=True)
    override_path = args.override.resolve(strict=True)
    report_path = args.report.resolve()
    if root == allowed or not root.is_relative_to(allowed):
        parser.error("reference must be inside ignored local-game-data/mm6")
    if (not override_path.is_relative_to(root) or not report_path.is_relative_to(root)
            or report_path in (override_path, root / "item-monster-guide-queue.json")
            or report_path.suffix != ".json" or not report_path.parent.is_dir()):
        parser.error("override and JSON report must stay in the ignored reference directory")

    queue_path = root / "item-monster-guide-queue.json"
    guide_path = root / "mm6-prima-guide.pdf"
    queue = json.loads(queue_path.read_text())
    override = json.loads(override_path.read_text())
    if digest(guide_path) != override["guide_sha256"] or digest(queue_path) != override["queue_sha256"]:
        parser.error("private review is stale against guide or retained queue")
    items = queue["items"]
    if len(items) != 581 or [r["id"] for r in items] != list(range(581)):
        parser.error("unexpected item queue IDs")
    unreviewed = [r for r in items if r["guide_semantic_status"] == "unreviewed"]
    matched = [r for r in unreviewed if r["catalog_match_status"] == "matched"]
    uncertain = [r for r in unreviewed if r["catalog_match_status"] in ("ambiguous", "absent")]
    special = [r for r in items if r["retained_data_role"] == "special_misc_role_unresolved"]
    if (len(unreviewed), len(matched), len(uncertain), len(special)) != (164, 43, 19, 67):
        parser.error("item review queues changed; regenerate manual review")
    if any(r["visual_review_page"] not in range(224, 231) for r in matched):
        parser.error("reviewed late-catalog entries moved to another guide page")

    role_ids = set(override["qualitative_role_claim_ids"])
    if len(role_ids) != 8 or not role_ids.issubset({r["id"] for r in matched}):
        parser.error("qualitative-role override is not the reviewed eight-ID subset")
    gem_claims = {int(k): v for k, v in override["gem_table_claims"].items()}
    if len(gem_claims) != 10 or set(gem_claims) != set(range(436, 446)):
        parser.error("private gem table must cover ten retained candidate IDs")
    absent_followup = {int(k): v for k, v in override["absent_followup"].items()}
    uncertain_ids = {r["id"] for r in uncertain}
    if set(absent_followup) != uncertain_ids - set(gem_claims):
        parser.error("private follow-up must cover the remaining uncertain IDs")

    gem_matches = []
    gem_mismatches = []
    for item_id, claim in gem_claims.items():
        row = items[item_id]
        same_name = normalized(row["name"]) == normalized(claim["guide_name"])
        same_value = int(row["retained_base_value"]) == claim["guide_value"]
        record = {"id": item_id, "retained_name": row["name"], "retained_value": row["retained_base_value"],
                  "guide_name": claim["guide_name"], "guide_value": claim["guide_value"],
                  "page": 232, "same_name": same_name, "same_value": same_value}
        (gem_matches if same_name and same_value else gem_mismatches).append(record)
    if (len(gem_matches), len(gem_mismatches)) != (9, 1):
        parser.error("gem transcription comparison changed; review rendered page")

    special_records = []
    signatures = Counter()
    referenced_files = set()
    for row in special:
        refs = row["retained_event_held_item_references"]
        ops = sorted({e["opcode"] for e in refs})
        signature = "+".join(map(str, ops)) if ops else "none"
        signatures[signature] += 1
        referenced_files.update(e["event"].split(":", 1)[0] for e in refs)
        special_records.append({"id": row["id"], "name": row["name"],
                                "guide_page": row["visual_review_page"],
                                "event_signature": signature, "event_references": refs,
                                "retained_quest_description_marker": row["retained_quest_description_marker"]})

    page_hashes = {}
    for page in override["visually_checked_pages"]:
        candidates = [root / f"item-guide-batch-{page}.png",
                      root / f"guide-check-treasure-{page}.png",
                      root / f"item-guide-followup-{page}.png"]
        image = next((path for path in candidates if path.is_file()), None)
        if image is None:
            parser.error(f"missing local rendered visual page {page}")
        page_hashes[str(page)] = digest(image)

    summary = {
        "identified_entries_without_prior_field_check": len(matched),
        "identified_entries_with_qualitative_role_claim": len(role_ids),
        "identified_entries_without_explicit_rule_on_checked_page": len(matched) - len(role_ids),
        "uncertain_ocr_rows": len(uncertain),
        "gem_table_rows_checked": len(gem_claims),
        "gem_name_value_matches": len(gem_matches),
        "gem_name_or_value_disagreements": len(gem_mismatches),
        "other_uncertain_rows_with_followup": len(absent_followup),
        "broad_special_misc_rows": len(special),
        "broad_special_misc_with_event_operands": sum(bool(r["retained_event_held_item_references"]) for r in special),
        "broad_special_misc_event_references": sum(len(r["retained_event_held_item_references"]) for r in special),
        "broad_special_misc_referenced_evt_files": len(referenced_files),
        "broad_special_misc_event_signatures": dict(sorted(signatures.items())),
    }
    payload = {"format": 1, "scope": "retained extracts, owned guide images, current source; no original install",
               "summary": summary,
               "identified_entries": [{"id": r["id"], "name": r["name"], "page": r["visual_review_page"],
                                       "guide_evidence": "qualitative_role_claim" if r["id"] in role_ids else "description_without_explicit_rule"}
                                      for r in matched],
               "gem_table": gem_matches + gem_mismatches,
               "other_uncertain": [{"id": i, "name": items[i]["name"], **note}
                                   for i, note in sorted(absent_followup.items())],
               "special_misc": special_records,
               "sha256": {"queue": digest(queue_path), "guide": digest(guide_path),
                          "override": digest(override_path), "rendered_pages": page_hashes}}
    report_path.write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    main()
