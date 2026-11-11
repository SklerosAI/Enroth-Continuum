#!/usr/bin/env python3
"""Run MM6MapSweep.DISABLED_EnterTickAndFireEvents over every MM6 map, one headless process per map.

Each map runs with actors first. A map that doesn't finish runs again without actors, then without the timer wait if a
timer ended that run, then once more past each face or decoration event that ended a run, so that its timers and events
still get swept. Per-map records, logs and the summary go to a Git-ignored folder, by default
local-game-data/mm6/sweep-YYYY-MM-DD. The summary ranks first-throw categories by the main-quest maps they block, read
from a private --main-quest list. Set MM6_DATA_PATH and MM6_RULES_PATH as for run_mm6_firstmap_gate.py.
"""

import argparse
import concurrent.futures
import datetime
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[1]
GAME_TEST = ROOT / "build/test/Bin/GameTest/OpenEnroth_GameTest"
TEST = "MM6MapSweep.DISABLED_EnterTickAndFireEvents"
LOG_LINE = re.compile(r"^\[\d{4}-\d\d-\d\d [\d:.]+\] \[(\w+)\] (.*)$")
MISSING = re.compile(r"fail(ed)? to (load|open|find)|not found|missing|could not|couldn't|can't|cannot|no such", re.I)
IGNORED_LOG = re.compile(r"configuration file|Face has invalid event ID", re.I)


def run_game_test(data, rules, env_extra, log_path, timeout, test=TEST):
    env = {key: value for key, value in os.environ.items() if not key.startswith("ENROTH_MM6_")}
    env.update(env_extra)
    command = [str(GAME_TEST), "--game", "mm6", "--headless", "--data-path", str(data), "--rules-path", str(rules),
               "--test-path", "test/Data", "--gtest_also_run_disabled_tests", "--gtest_filter=" + test, "--gtest_brief=1"]
    start = time.monotonic()
    with open(log_path, "w") as log:
        try:
            result = subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=timeout,
                                    check=False)
            code = result.returncode
        except subprocess.TimeoutExpired:
            code = "timeout"
    return code, time.monotonic() - start


def process_death(log_text, code):
    """The exception or crash that ended a process early, and the source function of a crash."""
    for line in log_text.splitlines():
        match = LOG_LINE.match(line)
        if match and match.group(2).startswith("Terminated with exception: "):
            return match.group(2).removeprefix("Terminated with exception: "), "", False
    lines = log_text.splitlines()
    for index, line in enumerate(lines):
        if not line.startswith("Crashed because of"):
            continue
        reason = next((previous for previous in reversed(lines[:index]) if previous.strip()), line)
        reason = re.sub(r"^\S+\.h:\d+: ", "", reason) if not LOG_LINE.match(reason) else line
        frames = [re.search(r" in (.+?) at ", frame) for frame in lines[index + 2:index + 40]]
        names = [frame.group(1) for frame in frames if frame and not frame.group(1).startswith(("std::", "__"))]
        return reason.strip(), names[0] if names else "", True
    if code == "timeout":
        return "timeout", "", True
    return "", "", code != 0


def missing_assets(log_text):
    found = set()
    for line in log_text.splitlines():
        match = LOG_LINE.match(line)
        if match and match.group(1) in ("warning", "error", "critical") and MISSING.search(match.group(2)) and \
                not IGNORED_LOG.search(match.group(2)):
            found.add(match.group(2))
    return sorted(found)


def sweep_one(args, map_id, stem, suffix, actors=True, skip_timers=False, skip_events=()):
    record_path = args.out / "records" / f"{stem}{suffix}.json"
    log_path = args.out / "logs" / f"{stem}{suffix}.log"
    record_path.unlink(missing_ok=True)
    env = {"ENROTH_MM6_MAP_SWEEP_MAP": str(map_id), "ENROTH_MM6_MAP_SWEEP_RESULT": str(record_path),
           "ENROTH_MM6_MAP_SWEEP_MAX_FRAMES": str(args.max_frames)}
    if not actors:
        env["ENROTH_MM6_MAP_SWEEP_NO_ACTORS"] = "1"
    if skip_timers:
        env["ENROTH_MM6_MAP_SWEEP_SKIP_TIMERS"] = "1"
    if skip_events:
        env["ENROTH_MM6_MAP_SWEEP_SKIP_EVENTS"] = ",".join(map(str, skip_events))
    code, seconds = run_game_test(args.data, args.rules, env, log_path, args.timeout)
    log_text = log_path.read_text(errors="replace")
    record = json.loads(record_path.read_text()) if record_path.is_file() else {"phase": "no record"}
    record["exitCode"] = code
    record["seconds"] = round(seconds, 1)
    record["missingAssets"] = missing_assets(log_text)
    record["death"], record["crashSource"], record["crash"] = "", "", False
    if record.get("phase") != "done" and not record.get("loadError"):
        record["death"], record["crashSource"], record["crash"] = process_death(log_text, code)
    record_path.write_text(json.dumps(record, indent=1) + "\n")
    return record


def category(text):
    return re.sub(r"(?<![A-Za-z])-?\d+", "N", text).strip()


def throw_site(text, cache={}):
    # The first throw statement in src/ holding the longest literal piece of the message.
    if text in cache:
        return cache[text]
    pieces = sorted((piece.strip() for piece in re.split(r"\bN\b|:", text)), key=len, reverse=True)
    site = ""
    for piece in pieces[:2]:
        if len(piece) < 12:
            break
        result = subprocess.run(["git", "grep", "-n", "-F", piece, "--", "src"], cwd=ROOT, capture_output=True, text=True)
        hits = [line for line in result.stdout.splitlines() if "throw" in line or "Exception(" in line]
        if hits:
            site = ":".join(hits[0].split(":")[:2])
            break
    cache[text] = site
    return site


def first_throw(record):
    if record.get("loadError"):
        stage = record.get("loadStage", "")
        event = record.get("onLoadThrowEvent") or 0
        return "load", record["loadError"], f"{stage}" + (f" event {event}" if event else "")
    if record.get("death"):
        source = [f"in {record['crashSource']}"] if record.get("crashSource") else []
        if record.get("nextFrameTimers"):
            source.append("timer events " + ",".join(map(str, record["nextFrameTimers"])))
        if record.get("attackingMonsters"):
            source.append("attacking monsters " + ",".join(map(str, record["attackingMonsters"])))
        if record.get("phase") == "events" and record.get("currentEvent"):
            source.append(f"event {record['currentEvent']}")
        return record.get("phase", ""), ("crash: " if record["crash"] else "") + record["death"], "; ".join(source)
    if record.get("eventThrows"):
        first = record["eventThrows"][0]
        return "events", first["error"], f"{first['kind']} event {first['event']}"
    return "", "", ""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT / f"local-game-data/mm6/sweep-{datetime.date.today().isoformat()}")
    parser.add_argument("--maps", nargs="*", help="map file stems or IDs to run, all maps by default")
    parser.add_argument("--main-quest", type=Path, help="private file listing main-quest map stems, one per line")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--timeout", type=int, default=900, help="seconds per process")
    parser.add_argument("--max-frames", type=int, default=3000, help="cap on frames waited for timers")
    parser.add_argument("--event-retries", type=int, default=8, help="reruns past events that end the process")
    args = parser.parse_args()

    for key in ("MM6_DATA_PATH", "MM6_RULES_PATH"):
        if not os.environ.get(key) or not Path(os.environ[key]).is_dir():
            print(f"Set {key} to an existing private directory.", file=sys.stderr)
            return 2
    args.data, args.rules = Path(os.environ["MM6_DATA_PATH"]), Path(os.environ["MM6_RULES_PATH"])
    if not GAME_TEST.is_file():
        print("Build OpenEnroth_GameTest first.", file=sys.stderr)
        return 2
    args.out = args.out.resolve()
    if subprocess.run(["git", "check-ignore", "-q", str(args.out / "summary.md")], cwd=ROOT).returncode != 0:
        print(f"{args.out} is not Git-ignored, refusing to write private results there.", file=sys.stderr)
        return 2
    (args.out / "records").mkdir(parents=True, exist_ok=True)
    (args.out / "logs").mkdir(exist_ok=True)

    started = time.monotonic()
    list_path = args.out / "maps.txt"
    run_game_test(args.data, args.rules, {"ENROTH_MM6_MAP_SWEEP_MAP": "list", "ENROTH_MM6_MAP_SWEEP_RESULT": str(list_path)},
                  args.out / "logs" / "list.log", 120)
    maps = [(int(line.split()[0]), line.split()[1]) for line in list_path.read_text().splitlines() if line.strip()]
    if args.maps:
        maps = [(map_id, name) for map_id, name in maps if name.rsplit(".", 1)[0] in args.maps or str(map_id) in args.maps]
    main_quest = set()
    if args.main_quest:
        main_quest = {line.strip() for line in args.main_quest.read_text().splitlines() if line.strip()}

    def sweep(entry):
        # With actors, then without actors when that didn't finish, then without the timer wait when timers ended the
        # run, then again past each event that ended the run.
        map_id, name = entry
        stem = name.rsplit(".", 1)[0]
        passes = [sweep_one(args, map_id, stem, "")]
        if passes[-1].get("phase") != "done":
            passes.append(sweep_one(args, map_id, stem, "-noactors", actors=False))
            if passes[-1].get("phase") in ("frame", "timers"):
                passes.append(sweep_one(args, map_id, stem, "-events", actors=False, skip_timers=True))
            skipped = []
            while passes[-1].get("phase") == "events" and passes[-1].get("currentEvent") and len(skipped) < args.event_retries:
                skipped.append(passes[-1]["currentEvent"])
                passes.append(sweep_one(args, map_id, stem, f"-events{len(skipped)}", actors=False, skip_timers=True,
                                        skip_events=skipped))
        print(f"{stem}: " + ", ".join(str(record.get("phase")) for record in passes), flush=True)
        return stem, passes

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = sorted(pool.map(sweep, maps), key=lambda item: item[0])
    total = time.monotonic() - started

    rows = []
    categories = {}
    event_categories = {}
    for stem, passes in results:
        first = passes[0]
        stage, error, source = first_throw(first)
        site = throw_site(category(error)) if error and not first.get("crash") else first.get("crashSource", "")
        if error:
            entry = categories.setdefault(category(error), {"site": site, "maps": set(), "mainQuest": set()})
            entry["maps"].add(stem)
            if stem in main_quest:
                entry["mainQuest"].add(stem)
        timed = next((record for record in passes if record.get("phase") not in ("first-map", "load", "load failed", "frame",
                                                                                  "no record")), passes[-1])
        evented = [record for record in passes if record.get("phase") in ("events", "done")]
        throws, crashes = {}, {}
        for record in evented:
            for value in record.get("eventThrows", []):
                throws[value["event"]] = value
            if record.get("phase") == "events" and record.get("currentEvent"):
                crashes[record["currentEvent"]] = ("crash: " if record["crash"] else "") + record["death"]
        for value in list(throws.values()) + [{"error": text} for text in crashes.values()]:
            entry = event_categories.setdefault(category(value["error"]), {"maps": set(), "mainQuest": set(), "events": 0})
            entry["maps"].add(stem)
            entry["events"] += 1
            if stem in main_quest:
                entry["mainQuest"].add(stem)
        last = evented[-1] if evented else {}
        no_actors = passes[1] if len(passes) > 1 else None
        rows.append({
            "map": stem, "mainQuest": stem in main_quest, "loads": not first.get("loadError") and first.get("phase") not in ("first-map", "load", "no record"), "phase": first.get("phase"),
            "exit": first.get("exitCode"), "crash": first.get("crash"), "actors": first.get("actorCount"),
            "firstThrowStage": stage, "firstThrow": error, "throwSite": site, "source": source,
            "noActorsPhase": no_actors.get("phase") if no_actors else "",
            "noActorsThrow": first_throw(no_actors)[1] if no_actors else "",
            "timers": len(timed.get("timers", [])), "timersFired": sum(t["fired"] or (timed.get("phase") == "timers" and t["event"] in timed.get("nextFrameTimers", []))
                              for t in timed.get("timers", [])),
            "timerPass": "with actors" if timed is first else "without actors", "tickStop": timed.get("tickStop", ""),
            "eventsTotal": sum(last.get(key, 0) for key in ("plateEvents", "clickEvents", "otherFaceEvents", "decorationEvents")),
            "eventsMissing": last.get("missingEvents", 0), "eventsFired": last.get("eventsFired", 0),
            "eventsFinished": last.get("phase") == "done", "eventThrows": len(throws), "eventDeaths": len(crashes),
            "missingAssets": sorted({asset for record in passes for asset in record.get("missingAssets", [])}),
            "passes": len(passes), "seconds": round(sum(record.get("seconds", 0) for record in passes), 1),
        })

    ranked = sorted(categories.items(), key=lambda item: (-len(item[1]["mainQuest"]), -len(item[1]["maps"]), item[0]))
    ranked_events = sorted(event_categories.items(), key=lambda item: (-len(item[1]["mainQuest"]), -len(item[1]["maps"]), item[0]))
    summary = {"maps": len(rows), "loaded": sum(row["loads"] for row in rows), "done": sum(row["phase"] == "done" for row in rows),
               "crashes": [row["map"] for row in rows if row["crash"]], "seconds": round(total, 1), "rows": rows,
               "categories": [{"category": name, "site": entry["site"], "maps": sorted(entry["maps"]),
                               "mainQuest": sorted(entry["mainQuest"])} for name, entry in ranked],
               "eventCategories": [{"category": name, "site": throw_site(name), "events": entry["events"], "maps": sorted(entry["maps"]),
                                    "mainQuest": sorted(entry["mainQuest"])} for name, entry in ranked_events]}
    (args.out / "summary.json").write_text(json.dumps(summary, indent=1) + "\n")

    lines = [f"# MM6 map sweep, {datetime.date.today().isoformat()}", "",
             f"{summary['maps']} maps, {summary['loaded']} loaded, {summary['done']} finished every phase with actors, "
             f"{len(summary['crashes'])} ended without an exception. Total {total / 60:.1f} minutes.", "",
             "| Map | MQ | Loads | Phase | Actors | First throw | Site | Source | Without actors | Timers fired "
             "| Events fired | Event throws | Event deaths | Missing assets |",
             "| --- | --- | --- | --- | ---: | --- | --- | --- | --- | --- | --- | ---: | ---: | ---: |"]
    for row in rows:
        lines.append(f"| {row['map']} | {'yes' if row['mainQuest'] else ''} | {'yes' if row['loads'] else 'no'} | {row['phase']} "
                     f"| {row['actors']} | {row['firstThrowStage']}: {row['firstThrow']} | {row['throwSite']} | {row['source']} "
                     f"| {row['noActorsPhase']} {row['noActorsThrow']} "
                     f"| {row['timersFired']}/{row['timers']} {row['timerPass']}, {row['tickStop']} "
                     f"| {row['eventsFired']}/{row['eventsTotal']}{'' if row['eventsFinished'] else ' unfinished'} "
                     f"| {row['eventThrows']} | {row['eventDeaths']} | {len(row['missingAssets'])} |")
    lines += ["", "## First-throw categories", "", "| Category | Site | Main-quest maps | All maps |", "| --- | --- | ---: | ---: |"]
    for name, entry in ranked:
        lines.append(f"| {name} | {entry['site']} | {len(entry['mainQuest'])} | {len(entry['maps'])} |")
    lines += ["", "## Face and decoration event throws", "", "| Category | Site | Events | Main-quest maps | All maps |",
              "| --- | --- | ---: | ---: | ---: |"]
    for name, entry in ranked_events:
        lines.append(f"| {name} | {throw_site(name)} | {entry['events']} | {len(entry['mainQuest'])} | {len(entry['maps'])} |")
    (args.out / "summary.md").write_text("\n".join(lines) + "\n")
    print(f"{summary['maps']} maps, {summary['loaded']} loaded, {summary['done']} done, {len(summary['crashes'])} crashes, "
          f"{total / 60:.1f} minutes. Summary in {args.out / 'summary.md'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
