import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

from scripts import mm6_f4_export_creation_rules as exporter


def synthetic_report():
    return {
        "status": "corrected_authoritative_static_analysis",
        "creation": {
            "arrays": {
                "hp_base": [0] * 6,
                "sp_base": [0] * 6,
                "hp_factor": [1] * 18,
                "sp_factor": [1] * 18,
            },
            "starting_stats_by_family": [[11, 11, 11, 10, 10, 10, 10] for _ in range(6)],
            "starting_skills_by_family": [[1, 1] + [2] * 9 + [0] * 20 for _ in range(6)],
        },
    }


class ExportCreationRulesTest(unittest.TestCase):
    def test_valid_shape_and_private_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            report = root / "report.json"
            output = root / "profile" / "rules" / "creation.json"
            report.write_text(json.dumps(synthetic_report()))
            with patch.object(sys, "argv", ["export", "--report", str(report), "--output", str(output)]):
                exporter.main()
            self.assertEqual(json.loads(output.read_text())["format"], 1)
            self.assertEqual(os.stat(output).st_mode & 0o777, 0o600)
            with patch.object(sys, "argv", ["export", "--report", str(report), "--output", str(output)]):
                with self.assertRaises(SystemExit):
                    exporter.main()

    def test_rejects_invalid_family_shape(self):
        report = synthetic_report()
        report["creation"]["starting_skills_by_family"][0][2] = 3
        with self.assertRaises(ValueError):
            exporter.build_manifest(json.dumps(report).encode())

    def test_rejects_uncorrected_source(self):
        report = synthetic_report()
        report["status"] = "candidate"
        with self.assertRaises(ValueError):
            exporter.build_manifest(json.dumps(report).encode())


if __name__ == "__main__":
    unittest.main()
