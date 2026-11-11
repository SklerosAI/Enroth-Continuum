#!/usr/bin/env python3
"""Synthetic checks for tools/mm7_playtest_capture.sh.

Nothing here records the screen. A stub stands in for screencapture and
writes small chunk files, a sleeping process stands in for MM7, and every
session lives in a temporary directory.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools/mm7_playtest_capture.sh"

SCREENCAPTURE_STUB = r"""#!/bin/bash
# Stand-in for screencapture: one chunk per call, CHUNK_SLEEP seconds long.
out="${@: -1}"
printf '%s\n' "$*" >> "$STUB_DIR/screencapture.calls"
[[ "${STUB_MODE:-ok}" == ok ]] || exit 1
name=$(basename "$out")
trap 'exit 143' TERM
printf 'chunk %s pid %s start\n' "$name" "$$" > "$out"
head -c 4096 /dev/urandom >> "$out"
sleep "${CHUNK_SLEEP:-1}" &
wait $!
printf 'complete\n' >> "$out"
/bin/cp "$out" "$STUB_DIR/ledger/${name#.}"
"""

DF_STUB = r"""#!/bin/bash
# Reports DF_AVAIL_KB free when that file exists, otherwise the real value.
if [[ -f "$STUB_DIR/df-avail-kb" ]]; then
    printf 'Filesystem 1024-blocks Used Available Capacity Mounted on\n'
    printf 'stub 999999999 1 %s 1%% /\n' "$(cat "$STUB_DIR/df-avail-kb")"
else
    exec /bin/df "$@"
fi
"""

LN_STUB = r"""#!/bin/bash
# Fails like a volume without hard links when NO_HARD_LINKS exists.
[[ ! -f "$STUB_DIR/no-hard-links" ]] || exit 1
exec /bin/ln "$@"
"""

CP_STUB = r"""#!/bin/bash
# Copies slowly when SLOW_CP exists, so a stop can land mid-copy.
if [[ -f "$STUB_DIR/slow-cp" ]]; then
    : > "$STUB_DIR/cp-started"
    sleep 3
fi
exec /bin/cp "$@"
"""


def write_executable(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    path.chmod(0o755)


def wait_for(condition, timeout: float, step: float = 0.1) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if condition():
            return True
        time.sleep(step)
    return condition()


class CaptureCase(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = Path(tempfile.mkdtemp(prefix="mm7-capture-test-")).resolve()
        self.stub = self.temp / "stub"
        (self.stub / "ledger").mkdir(parents=True)
        (self.stub / "bin").mkdir()
        write_executable(self.stub / "screencapture", SCREENCAPTURE_STUB)
        write_executable(self.stub / "bin/df", DF_STUB)
        write_executable(self.stub / "bin/ln", LN_STUB)
        write_executable(self.stub / "bin/cp", CP_STUB)
        self.script = self.temp / "tools/mm7_playtest_capture.sh"
        self.script.parent.mkdir()
        self.script.write_text(SCRIPT.read_text().replace("/usr/sbin/screencapture", str(self.stub / "screencapture")))

        self.profile = self.temp / "profile"
        (self.profile / "logs").mkdir(parents=True)
        (self.profile / "saves").mkdir()
        self.app = self.temp / "Atlas.app"
        write_executable(self.app / "Contents/MacOS/EnrothContinuum", "#!/bin/sh\n")
        write_executable(self.app / "Contents/Helpers/MM7.app/Contents/MacOS/MM7", "#!/bin/sh\n")
        self.recordings = self.temp / "recordings"
        self.recordings.mkdir()
        self.session = self.recordings / "session"
        self.home = self.temp / "home"
        self.reports = self.home / "Library/Logs/DiagnosticReports"
        self.reports.mkdir(parents=True)
        self.env = dict(os.environ, STUB_DIR=str(self.stub), CHUNK_SLEEP="1", HOME=str(self.home),
                        PATH=f"{self.stub / 'bin'}:{os.environ['PATH']}")
        self.game = subprocess.Popen(["/bin/sleep", "600"])
        self.recorder: subprocess.Popen | None = None

    def tearDown(self) -> None:
        for process in (self.recorder, self.game):
            if process and process.poll() is None:
                process.kill()
                process.wait()
        subprocess.run(["pkill", "-f", str(self.stub / "screencapture")], check=False)
        subprocess.run(["rm", "-rf", str(self.temp)], check=True)

    def cli(self, *args: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(["/bin/bash", str(self.script), *args, "--session", str(self.session)],
                              env=self.env, text=True, capture_output=True, check=True)

    def start(self, *extra: str, video: bool = True, check: bool = True) -> None:
        args = ["/bin/bash", str(self.script), "start", "--profile", str(self.profile),
                "--session", str(self.session), "--app", str(self.app),
                "--game-pid", str(self.game.pid), "--display", "1", "--video-chunk-seconds", "60",
                "--poll-seconds", "1", "--min-free-mib", "128", *extra]
        if video:
            args.append("--video")
        with (self.temp / "recorder.log").open("a") as log:
            self.recorder = subprocess.Popen(args, env=self.env, stdout=log, stderr=log)
        if check:
            self.assertTrue(wait_for(lambda: self.status().get("state") == "running", 5))

    def finish(self, timeout: float = 30) -> None:
        assert self.recorder
        self.recorder.wait(timeout=timeout)

    def identity(self) -> dict[str, str]:
        lines = (self.session / "identity.txt").read_text().splitlines()
        return dict(line.split("=", 1) for line in lines)

    def timeline(self) -> str:
        return (self.session / "timeline.tsv").read_text()

    def status(self) -> dict:
        try:
            return json.loads((self.session / "status.json").read_text())
        except (OSError, ValueError):
            return {}

    def completed(self) -> list[str]:
        return sorted(path.name for path in (self.session / "video").glob("chunk-*.mov"))

    def video_files(self) -> list[Path]:
        return [path for path in (self.session / "video").iterdir() if path.suffix == ".mov"]

    def clips(self) -> list[Path]:
        return sorted(path for path in (self.session / "clips").iterdir() if path.is_dir())

    def wait_for_completed(self, count: int, timeout: float = 20) -> None:
        self.assertTrue(wait_for(lambda: len(self.calls()) > count and len(self.completed()) >= min(count, 2), timeout))

    def calls(self) -> list[str]:
        path = self.stub / "screencapture.calls"
        return path.read_text().splitlines() if path.exists() else []

    def assert_clip_matches_ledger(self, clip: Path) -> list[str]:
        names = sorted(path.name for path in clip.iterdir())
        self.assertFalse([name for name in names if name.startswith(".")], "temporary files left in clip")
        for name in names:
            recorded = self.stub / "ledger" / name.replace(".mov", ".recording.mov")
            self.assertEqual((clip / name).read_bytes(), recorded.read_bytes(), name)
            self.assertTrue((clip / name).read_bytes().endswith(b"complete\n"), name)
        return names

    def assert_buffer_gone(self) -> None:
        self.assertEqual(self.video_files(), [])
        self.assertEqual(list((self.session / "video").glob(".*")), [])

    def assert_no_capture_running(self) -> None:
        result = subprocess.run(["pgrep", "-f", str(self.stub / "screencapture")],
                                capture_output=True, text=True)
        self.assertEqual(result.stdout.strip(), "")


class RollingReplayTest(CaptureCase):
    def test_roll_keeps_at_most_two_completed_chunks(self) -> None:
        # The buffer must never hold more than two completed chunks and the one being recorded.
        self.start()
        most_completed = most_files = 0
        deadline = time.monotonic() + 9
        while time.monotonic() < deadline:
            most_completed = max(most_completed, len(self.completed()))
            most_files = max(most_files, len(self.video_files()))
            time.sleep(0.05)
        self.assertGreaterEqual(len(self.calls()), 5, "the stub should have recorded several chunks")
        self.assertEqual(most_completed, 2)
        self.assertLessEqual(most_files, 3)
        self.cli("stop", "--game-exit", "normal")
        self.finish()

    def test_clip_request_copies_buffer_intact(self) -> None:
        # A clip holds the two completed chunks at the request and the chunk being recorded.
        self.env["CHUNK_SLEEP"] = "3"
        self.start()
        self.wait_for_completed(3)
        started = len(self.calls())
        self.assertTrue(wait_for(lambda: len(self.calls()) > started, 10))
        before = self.completed()
        self.cli("clip")
        self.assertTrue(wait_for(lambda: self.clips() and len(list(self.clips()[0].glob("*.mov"))) == 3, 10))
        names = self.assert_clip_matches_ledger(self.clips()[0])
        numbers = [int(name[6:12]) for name in names]
        self.assertEqual(names[:2], before[-2:])
        self.assertEqual(numbers, list(range(numbers[0], numbers[0] + 3)))
        time.sleep(7)  # The roll continues past the clip.
        self.assertEqual(len(self.assert_clip_matches_ledger(self.clips()[0])), 3)
        self.cli("stop", "--game-exit", "normal")
        self.finish()
        self.assertEqual(len(self.clips()), 1)
        self.assert_buffer_gone()
        self.assertEqual(self.status()["clips"], 1)

    def test_abnormal_exit_keeps_clip(self) -> None:
        # A crash keeps the buffer, including the chunk recording when MM7 died.
        self.start()
        self.wait_for_completed(3)
        self.game.kill()
        self.game.wait()
        self.cli("stop", "--game-exit", "abnormal")
        self.finish()
        status = self.status()
        self.assertEqual(status["video"], "kept")
        self.assertEqual(len(self.clips()), 1)
        names = self.assert_clip_matches_ledger(self.clips()[0])
        self.assertEqual(len(names), 3)
        self.assertEqual(int(names[-1][6:12]), len(self.calls()))
        self.assert_buffer_gone()
        self.assert_no_capture_running()
        self.assertIn("abnormal exit", (self.session / "timeline.tsv").read_text())

    def test_unreported_game_exit_keeps_clip(self) -> None:
        # With no stop request the exit status is unknown, so the buffer is kept.
        self.start()
        self.wait_for_completed(2)
        self.game.kill()
        self.game.wait()
        self.finish(timeout=40)
        self.assertEqual(self.status()["state"], "game-exited")
        self.assertEqual(self.status()["video"], "kept")
        self.assertEqual(len(self.clips()), 1)
        self.assert_clip_matches_ledger(self.clips()[0])
        self.assert_buffer_gone()

    def test_normal_exit_discards_video(self) -> None:
        # A normal exit deletes the buffer and keeps logs and saves.
        (self.profile / "saves/save000.mm7").write_bytes(b"save one")
        (self.profile / "logs/enroth-continuum-mm7_1.log").write_text("line one\n")
        self.start()
        self.wait_for_completed(3)
        self.game.terminate()
        self.game.wait()
        self.cli("stop", "--game-exit", "normal")
        self.finish()
        status = self.status()
        self.assertEqual(status["video"], "discarded")
        self.assertEqual(status["clips"], 0)
        self.assertEqual(self.clips(), [])
        self.assert_buffer_gone()
        self.assert_no_capture_running()
        self.assertEqual([path.read_bytes() for path in (self.session / "saves").glob("*.mm7")], [b"save one"])
        self.assertEqual([path.read_text() for path in (self.session / "logs").glob("*.log.part")], ["line one\n"])
        self.assertFalse((self.session / "warnings.txt").exists())

    def test_low_space_stops_recording(self) -> None:
        # Free space under the floor stops the recorder and its capture child.
        self.start()
        self.wait_for_completed(2)
        (self.stub / "df-avail-kb").write_text(str(100 * 1024))
        self.finish()
        status = self.status()
        self.assertEqual(status["state"], "low-space")
        self.assertIn("free space fell below 128 MiB", (self.session / "warnings.txt").read_text())
        self.assert_buffer_gone()
        self.assert_no_capture_running()
        self.assertTrue(self.game.poll() is None, "the recorder must never stop MM7")

    def test_stop_during_clip_copy_is_safe(self) -> None:
        # A stop and a signal that land during a slow clip copy leave whole files only.
        self.start()
        self.wait_for_completed(3)
        (self.stub / "no-hard-links").touch()
        (self.stub / "slow-cp").touch()
        self.cli("clip")
        self.assertTrue(wait_for(lambda: (self.stub / "cp-started").exists(), 10))
        self.cli("stop", "--game-exit", "normal")
        assert self.recorder
        self.recorder.send_signal(signal.SIGTERM)
        self.finish(timeout=60)
        (self.stub / "slow-cp").unlink()
        status = self.status()
        self.assertEqual(status["state"], "stopped")
        self.assertIsNot(status["ended_at"], None)
        self.assertEqual(len(self.clips()), 1)
        names = self.assert_clip_matches_ledger(self.clips()[0])
        self.assertEqual(len(names), 3, "the pending clip still receives the chunk being recorded")
        self.assert_buffer_gone()
        self.assert_no_capture_running()

    def test_failed_video_warns_about_permission(self) -> None:
        # A capture that writes nothing warns about Screen Recording permission and keeps logging.
        self.env["STUB_MODE"] = "fail"
        (self.profile / "logs/enroth-continuum-mm7_1.log").write_text("line one\n")
        self.start()
        self.assertTrue(wait_for(lambda: self.status().get("video") == "failed", 10))
        self.assertIn("Screen Recording permission", (self.session / "warnings.txt").read_text())
        self.assertEqual(len(self.calls()), 1, "a failed capture is not retried")
        self.cli("stop", "--game-exit", "abnormal")
        self.finish()
        self.assertEqual(self.status()["video"], "failed")
        self.assertEqual(self.clips(), [])
        self.assertEqual(len(list((self.session / "logs").glob("*.log.part"))), 1)

    def test_no_video_option_records_no_video(self) -> None:
        # Without --video the recorder never starts a capture.
        self.start(video=False)
        time.sleep(2)
        self.cli("clip")
        self.cli("stop", "--game-exit", "abnormal")
        self.finish()
        self.assertEqual(self.calls(), [])
        self.assertEqual(self.status()["video"], "off")
        self.assertEqual(self.clips(), [])


class DiagnosticsTest(CaptureCase):
    def add_upstream_helper(self) -> None:
        write_executable(self.app / "Contents/Helpers/OpenEnroth.app/Contents/MacOS/OpenEnroth", "#!/bin/sh\n")
        info = self.app / "Contents/Resources/Helpers/OpenEnroth-BUILD-INFO.txt"
        info.parent.mkdir(parents=True)
        info.write_text("upstream: https://github.com/OpenEnroth/OpenEnroth.git\ncommit: 8010ac364d9e\nversion: nightly\n")

    def write_crash_report(self, name: str, pid: int) -> Path:
        report = self.reports / name
        report.write_text('{"app_name":"MM7","name":"MM7"}\n{\n  "uptime" : 100,\n  "procName" : "MM7",\n'
                          f'  "pid" : {pid},\n  "exception" : {{"type" : "EXC_BAD_ACCESS"}}\n}}\n')
        return report

    def test_diagnostics_without_video(self) -> None:
        # Diagnostics alone copy logs and saves, sample the game and never touch the screen.
        (self.profile / "saves/save000.mm7").write_bytes(b"save one")
        (self.profile / "logs/enroth-continuum-mm7_1.log").write_text(
            "OpenEnroth, revision 5e3d2cf built on 2026-11-10\n")
        self.start(video=False)
        self.assertTrue(wait_for(lambda: (self.session / "samples.tsv").exists(), 10))
        self.cli("stop", "--game-exit", "normal", "--exit-detail", "exit status 0")
        self.finish()
        self.assertEqual(self.calls(), [])
        identity = self.identity()
        self.assertEqual(identity["build"], "mm7")
        self.assertEqual(identity["diagnostics_requested"], "1")
        self.assertEqual(identity["video_requested"], "0")
        self.assertEqual(identity["engine_revision"], "5e3d2cf")
        self.assertIn("sleep 600", identity["launch_command"])
        self.assertTrue(identity["helper_path"].endswith("Contents/Helpers/MM7.app"))
        self.assertEqual(len(list((self.session / "saves").glob("*.mm7"))), 1)
        self.assertEqual(len(list((self.session / "logs").glob("*.log.part"))), 1)
        sample = (self.session / "samples.tsv").read_text().splitlines()[0].split("\t")
        self.assertEqual(len(sample), 3)
        self.assertIn("stopped (normal exit, exit status 0)", self.timeline())
        self.assertFalse((self.session / "crash-reports").exists())

    def test_video_without_diagnostics(self) -> None:
        # Video alone keeps the replay but copies no logs or saves.
        (self.profile / "saves/save000.mm7").write_bytes(b"save one")
        (self.profile / "logs/enroth-continuum-mm7_1.log").write_text("line one\n")
        self.start("--no-diagnostics")
        self.wait_for_completed(2)
        self.game.kill()
        self.game.wait()
        self.cli("stop", "--game-exit", "abnormal", "--exit-detail", "uncaught signal 9")
        self.finish()
        self.assertEqual(self.status()["video"], "kept")
        self.assertEqual(len(self.clips()), 1)
        self.assertEqual(list((self.session / "saves").iterdir()), [])
        self.assertEqual(list((self.session / "logs").iterdir()), [])
        self.assertFalse((self.session / "samples.tsv").exists())
        self.assertFalse((self.session / "crash-reports").exists())
        self.assertEqual(self.identity()["diagnostics_requested"], "0")

    def test_neither_toggle_is_refused(self) -> None:
        # A recorder with nothing to record refuses to start.
        self.start("--no-diagnostics", video=False, check=False)
        self.finish()
        assert self.recorder
        self.assertEqual(self.recorder.returncode, 2)
        self.assertFalse(self.session.exists())

    def test_upstream_build_session(self) -> None:
        # An upstream session reads upstream's own logs and records its commit.
        self.add_upstream_helper()
        (self.profile / "logs/openenroth_2026_11_10_21_00_00.log").write_text(
            "OpenEnroth, revision 8010ac3 built on 2026-11-10\n")
        (self.profile / "logs/enroth-continuum-mm7_1.log").write_text("fork log\n")
        self.start("--build", "openenroth", video=False)
        self.assertTrue(wait_for(lambda: list((self.session / "logs").glob("*.log.part")), 10))
        self.cli("stop", "--game-exit", "normal")
        self.finish()
        identity = self.identity()
        self.assertEqual(identity["build"], "openenroth")
        self.assertTrue(identity["helper_path"].endswith("Contents/Helpers/OpenEnroth.app"))
        self.assertEqual(identity["helper_commit"], "8010ac364d9e")
        self.assertEqual(identity["engine_revision"], "8010ac3")
        parts = [path.read_text() for path in (self.session / "logs").glob("*.log.part")]
        self.assertEqual(parts, ["OpenEnroth, revision 8010ac3 built on 2026-11-10\n"])

    def test_upstream_build_needs_its_helper(self) -> None:
        # Without OpenEnroth.app in the bundle an upstream session is refused.
        self.start("--build", "openenroth", video=False, check=False)
        self.finish()
        assert self.recorder
        self.assertEqual(self.recorder.returncode, 2)
        self.assertIn("OpenEnroth helper", (self.temp / "recorder.log").read_text())
        self.assertFalse(self.session.exists())

    def test_crash_report_is_copied(self) -> None:
        # An abnormal exit copies the helper's own new crash report and nothing else.
        old = self.write_crash_report("MM7-2026-01-01-000000.ips", self.game.pid)
        os.utime(old, (1_000_000_000, 1_000_000_000))
        self.start(video=False)
        self.write_crash_report("MM7-2026-11-10-213455.ips", self.game.pid)
        self.write_crash_report("MM7-2026-11-10-213456.ips", self.game.pid + 100000)
        self.write_crash_report("Safari-2026-11-10-213457.ips", self.game.pid)
        self.game.kill()
        self.game.wait()
        self.cli("stop", "--game-exit", "abnormal", "--exit-detail", "uncaught signal 11")
        self.finish()
        copied = sorted(path.name for path in (self.session / "crash-reports").iterdir())
        self.assertEqual(copied, ["MM7-2026-11-10-213455.ips"])
        self.assertEqual((self.session / "crash-reports" / copied[0]).read_bytes(),
                         (self.reports / copied[0]).read_bytes())
        self.assertIn("abnormal exit, uncaught signal 11", self.timeline())

    def test_late_crash_report_is_waited_for(self) -> None:
        # macOS writes the report seconds after the crash, so an abnormal exit waits for it.
        self.start(video=False)
        self.game.kill()
        self.game.wait()
        self.cli("stop", "--game-exit", "abnormal")
        time.sleep(3)
        self.write_crash_report("MM7-2026-11-10-213455.ips", self.game.pid)
        self.finish()
        self.assertEqual([path.name for path in (self.session / "crash-reports").iterdir()],
                         ["MM7-2026-11-10-213455.ips"])

    def test_normal_exit_skips_crash_reports(self) -> None:
        # A normal exit never copies crash reports.
        self.start(video=False)
        self.write_crash_report("MM7-2026-11-10-213455.ips", self.game.pid)
        self.cli("stop", "--game-exit", "normal")
        self.finish()
        self.assertFalse((self.session / "crash-reports").exists())

    def test_diagnostics_low_space_stop(self) -> None:
        # The storage floor also stops a diagnostics-only session.
        self.start(video=False)
        (self.stub / "df-avail-kb").write_text(str(100 * 1024))
        self.finish()
        self.assertEqual(self.status()["state"], "low-space")
        self.assertTrue(self.game.poll() is None)


if __name__ == "__main__":
    unittest.main(verbosity=2)
