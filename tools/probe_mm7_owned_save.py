#!/usr/bin/env python3
"""Probe a copy of one owned MM7 save through fresh-process normal Load."""

import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile


MARKER = ".ec-mm7-disposable-save-gate"
MARKER_TEXT = "EnrothContinuum-MM7-save-gate-v1\n"
FILTERS = (
    "MM7OwnedSaveProbe.DISABLED_LoadCopiedPrimary",
    "MM7OwnedSaveProbe.DISABLED_LoadCopiedBackup",
)


def fingerprint(path: Path) -> tuple[tuple[int, int, int, int], bytes]:
    info = path.stat()
    metadata = (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns)
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return metadata, digest.digest()


def run_probe(binary: Path, data: Path, test_data: Path, source: Path,
              baseline: tuple[tuple[int, int, int, int], bytes], use_backup: bool) -> None:
    with tempfile.TemporaryDirectory(prefix="mm7-save-gate-probe-") as temporary:
        profile = Path(temporary)
        (profile / MARKER).write_text(MARKER_TEXT)
        saves = profile / "saves"
        saves.mkdir(mode=0o700)
        copied = saves / ("save000.mm7.bak" if use_backup else "save000.mm7")
        shutil.copyfile(source, copied)
        copied.chmod(0o600)
        if fingerprint(source) != baseline or fingerprint(copied)[1] != baseline[1]:
            raise RuntimeError("source save changed during copying; probe was not run")
        if use_backup:
            (saves / "save000.mm7").write_bytes(b"corrupt temporary primary")
        command = (
            str(binary), "--game=mm7", "--headless", f"--data-path={data}",
            f"--test-path={test_data}", f"--mm7-disposable-profile={profile}",
            f"--gtest_filter={FILTERS[use_backup]}", "--gtest_also_run_disabled_tests",
            "--log-level=critical",
        )
        print("Running copied backup probe" if use_backup else "Running copied primary probe", flush=True)
        subprocess.run(command, check=True)
        if fingerprint(source) != baseline:
            raise RuntimeError("source save changed during the probe; result is inconclusive")
        if fingerprint(copied)[1] != baseline[1]:
            raise RuntimeError("temporary save copy changed during the probe")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--data-path", type=Path, required=True)
    parser.add_argument("--test-path", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True, help="owned .mm7 slot to read; never modified")
    args = parser.parse_args()
    for path in (args.binary, args.data_path, args.test_path):
        if not path.exists():
            parser.error(f"missing path: {path}")
    source = args.source.expanduser()
    if source.is_symlink() or not source.is_file() or source.suffix.lower() != ".mm7":
        parser.error("source must be a regular, non-symlink .mm7 file")
    source = source.resolve(strict=True)
    baseline = fingerprint(source)
    if not 1024 < baseline[0][2] <= 64 * 1024 * 1024:
        parser.error("source save size is outside the bounded probe range")
    for use_backup in (False, True):
        run_probe(args.binary.resolve(), args.data_path.resolve(), args.test_path.resolve(),
                  source, baseline, use_backup)
    print("Both copied-slot probes passed; source save remained unchanged", flush=True)


if __name__ == "__main__":
    main()
