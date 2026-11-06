import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "mm6_map_name_manifest.py"
SPEC = importlib.util.spec_from_file_location("mm6_map_name_manifest", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class MapNameManifestTest(unittest.TestCase):
    def test_exact_set_and_rejections(self):
        names = [f"synthetic{i}.dlv" for i in range(67)]
        manifest = {"entries": [{"name": name} for name in names]}
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / "candidate.json"
            candidate.write_text(json.dumps(list(reversed(names))))
            MODULE.validate_candidate(manifest, candidate)

            altered = names.copy()
            altered[-1] = "absent.dlv"
            candidate.write_text(json.dumps(altered))
            with self.assertRaisesRegex(ValueError, "differ"):
                MODULE.validate_candidate(manifest, candidate)

            altered[-1] = altered[0]
            candidate.write_text(json.dumps(altered))
            with self.assertRaisesRegex(ValueError, "unique"):
                MODULE.validate_candidate(manifest, candidate)

            candidate.write_text(json.dumps(names[:-1]))
            with self.assertRaisesRegex(ValueError, "67"):
                MODULE.validate_candidate(manifest, candidate)


if __name__ == "__main__":
    unittest.main()
