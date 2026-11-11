#!/bin/bash
# Passive, local MM7 playtest evidence recorder. Compatible with macOS Bash 3.2.
set -euo pipefail
umask 077

usage() {
    cat <<'EOF'
Usage:
  mm7_playtest_capture.sh start --profile ABS_DIR --app ABS_ATLAS_APP --game-pid PID
      (--output ABS_EXISTING_BASE_DIR | --session ABS_NEW_DIR)
      [--build mm7 | --build openenroth] [--diagnostics | --no-diagnostics]
      [--video | --no-video] [--display N] [--video-chunk-seconds N]
      [--poll-seconds N] [--save-checkpoint-seconds N] [--min-free-mib N]
      [--expected-volume-id N]
  mm7_playtest_capture.sh note --session ABS_SESSION_DIR --text "owner note"
  mm7_playtest_capture.sh clip --session ABS_SESSION_DIR
  mm7_playtest_capture.sh stop --session ABS_SESSION_DIR [--game-exit normal|abnormal]
      [--exit-detail "exit status 0"]
  mm7_playtest_capture.sh status --session ABS_SESSION_DIR

The start command stays in the foreground, prints SESSION_DIR=..., and exits
after the game PID exits or stop is requested. It never launches or signals MM7.
--build names the Atlas helper being played: the fork's MM7.app (default) or
the upstream OpenEnroth.app. Diagnostics (logs, saves, crash reports and
process samples) are on by default. Video keeps a rolling buffer of the last two minutes. A clip request, an
abnormal game exit, or a game exit with no stop request keeps the buffer as a
clip. Otherwise the buffer is deleted when recording ends.
EOF
}

fail() { printf 'mm7 capture: %s\n' "$*" >&2; exit 2; }
utc() { date -u '+%Y-%m-%dT%H:%M:%SZ'; }
stamp() { date -u '+%Y%m%dT%H%M%SZ'; }
is_uint() { [[ "$1" =~ ^[0-9]+$ ]]; }
sha() { shasum -a 256 "$1" | awk '{print $1}'; }
device() { stat -f '%d' "$1" 2>/dev/null; }
plist_value() { plutil -extract "$2" raw -o - "$1" 2>/dev/null || printf 'unavailable'; }

# A session inside any Git checkout is allowed only when Git itself ignores it.
check_private_destination() {
    local parent="$1" candidate="$2" repo
    if repo=$(git -C "$parent" rev-parse --show-toplevel 2>/dev/null); then
        [[ "$candidate" == "$repo/local-game-data/"* ]] ||
            fail "a destination inside Git must be under ignored local-game-data"
        git -C "$repo" check-ignore -q -- "$candidate" ||
            fail "destination is in a tracked Git area; choose ignored local-game-data or a location outside Git"
    fi
}

canonical_existing_dir() {
    [[ "$1" == /* && -d "$1" ]] || fail "expected an existing absolute directory: $1"
    (cd "$1" && pwd -P)
}

validate_session() {
    [[ "$session_arg" == /* ]] || fail "--session must be absolute"
    [[ -d "$session_arg" && ! -L "$session_arg" ]] || fail "session directory is unavailable"
    session=$(canonical_existing_dir "$session_arg")
    [[ -f "$session/.ec-mm7-playtest-session" && ! -L "$session/.ec-mm7-playtest-session" ]] ||
        fail "not an MM7 playtest session"
    local parent
    parent=$(dirname "$session")
    check_private_destination "$parent" "$session"
    [[ $(device "$session") == "$(cat "$session/.volume-id")" ]] || fail "session volume changed or disconnected"
}

write_status() {
    local tmp="$session/.status.$$"
    printf '{"state":"%s","started_at":"%s","ended_at":%s,"video":"%s","clips":%s,"warnings":%s}\n' \
        "$state" "$started_at" "$ended_json" "$video_state" "$clips_saved" "$warning_count" > "$tmp"
    mv "$tmp" "$session/status.json"
}

event() {
    printf '%s\t%s\t%s\n' "$(utc)" "$1" "$2" >> "$session/timeline.tsv"
}

warn() {
    warning_count=$((warning_count + 1))
    printf '%s\t%s\n' "$(utc)" "$1" >> "$session/warnings.txt"
    event warning "$1"
    write_status
}

volume_ok() {
    [[ -d "$session" ]] && [[ $(device "$session") == "$volume_id" ]]
}

space_ok() {
    local available
    available=$(df -Pk "$session" 2>/dev/null | awk 'NR == 2 {print $4}')
    is_uint "${available:-}" && (( available >= min_free_mib * 1024 ))
}

check_destination() {
    if ! volume_ok; then
        printf 'mm7 capture: destination volume disconnected or replaced; stopping without further writes\n' >&2
        state="volume-lost"
        return 1
    fi
    if ! space_ok; then
        warn "Recording stopped: destination free space fell below ${min_free_mib} MiB."
        state="low-space"
        return 1
    fi
    return 0
}

# Saves must be unchanged across two reads and again after the copy.
snapshot() {
    local source="$1" category="$2" stable="$3" force="${4:-no}" key old hash before after copied tmp target ext now
    [[ -f "$source" && ! -L "$source" ]] || return 0
    key=$(printf '%s' "$source" | shasum -a 256 | awk '{print $1}')
    hash=$(sha "$source") || return 0
    old=""
    [[ ! -f "$session/state/$key" ]] || old=$(cat "$session/state/$key")
    [[ "$force" == yes || "$hash" != "$old" ]] || return 0
    if [[ "$stable" == yes ]]; then
        before=$(stat -f '%d:%i:%z:%m' "$source" 2>/dev/null) || return 0
        idle 1 || return 1
        after=$(stat -f '%d:%i:%z:%m' "$source" 2>/dev/null) || return 0
        [[ "$before" == "$after" ]] || return 0
        [[ "$hash" == "$(sha "$source")" ]] || return 0
    fi
    check_destination || return 1
    tmp=$(mktemp "$session/.incoming.XXXXXX") || return 1
    if ! cp "$source" "$tmp" 2>/dev/null; then
        unlink "$tmp" 2>/dev/null || true
        return 0
    fi
    copied=$(sha "$tmp") || { unlink "$tmp" 2>/dev/null || true; return 0; }
    if [[ "$stable" == yes ]]; then
        after=$(stat -f '%d:%i:%z:%m' "$source" 2>/dev/null) || after=""
        now=$(sha "$source" 2>/dev/null) || now=""
        if [[ "$before" != "$after" || "$hash" != "$copied" || "$hash" != "$now" ]]; then
            unlink "$tmp" 2>/dev/null || true
            return 0
        fi
    fi
    check_destination || { unlink "$tmp" 2>/dev/null || true; return 1; }
    sequence=$((sequence + 1))
    ext=".log"
    if [[ "$category" == saves ]]; then
        ext=".mm7"
        [[ "$source" != *.mm7.bak ]] || ext=".mm7.bak"
    fi
    target="$session/$category/$(stamp)-$(printf '%06d' "$sequence")$ext"
    mv "$tmp" "$target"
    printf '%s\t%s\t%s\t%s\t%s\n' "$(utc)" "$category" "$source" "$target" "$copied" >> "$session/snapshots.tsv"
    printf '%s\n' "$copied" > "$session/state/$key"
    if [[ "$force" == yes ]]; then
        event save-checkpoint "Scheduled save checkpoint copied."
    else
        event snapshot "$category"
    fi
}

# Logs are append-only byte ranges. A new inode, shorter file or changed prefix
# starts a new range at byte zero, covering common rotation/truncation cases.
capture_log() {
    local source="$1" key state_file identity size prefix prefix_len old_identity old_size old_prefix old_prefix_len
    local start length tmp copied target after_identity after_size comparison_prefix
    [[ -f "$source" && ! -L "$source" ]] || return 0
    key=$(printf '%s' "$source" | shasum -a 256 | awk '{print $1}')
    state_file="$session/state/$key.logstate"
    identity=$(stat -f '%d:%i' "$source" 2>/dev/null) || return 0
    size=$(stat -f '%z' "$source" 2>/dev/null) || return 0
    prefix_len="$size"
    (( prefix_len <= 4096 )) || prefix_len=4096
    if (( prefix_len > 0 )); then
        prefix=$(head -c "$prefix_len" "$source" | shasum -a 256 | awk '{print $1}') || return 0
    else
        prefix=$(printf '' | shasum -a 256 | awk '{print $1}')
    fi
    old_identity=""; old_size=0; old_prefix=""; old_prefix_len=0
    if [[ -f "$state_file" ]]; then
        IFS='|' read -r old_identity old_size old_prefix old_prefix_len < "$state_file" || true
    fi
    if (( old_prefix_len > 0 )); then
        comparison_prefix=$(head -c "$old_prefix_len" "$source" | shasum -a 256 | awk '{print $1}') || return 0
    else
        comparison_prefix=$(printf '' | shasum -a 256 | awk '{print $1}')
    fi
    if [[ "$old_identity" != "$identity" || "$size" -lt "$old_size" || "$comparison_prefix" != "$old_prefix" ]]; then
        [[ -z "$old_identity" ]] || event log-reset "Log rotated or truncated; starting a new byte range."
        start=0
    else
        start="$old_size"
    fi
    length=$((size - start))
    (( length > 0 )) || return 0
    check_destination || return 1
    tmp=$(mktemp "$session/.incoming.XXXXXX") || return 1
    # head bounds the copy to the measured size even if the live log grows.
    (tail -c "+$((start + 1))" "$source" | head -c "$length" > "$tmp") 2>/dev/null || true
    copied=$(stat -f '%z' "$tmp" 2>/dev/null) || copied=0
    after_identity=$(stat -f '%d:%i' "$source" 2>/dev/null) || after_identity=""
    after_size=$(stat -f '%z' "$source" 2>/dev/null) || after_size=0
    if [[ "$copied" -ne "$length" || "$after_identity" != "$identity" || "$after_size" -lt "$size" ]]; then
        unlink "$tmp" 2>/dev/null || true
        return 0
    fi
    check_destination || { unlink "$tmp" 2>/dev/null || true; return 1; }
    sequence=$((sequence + 1))
    target="$session/logs/$(stamp)-$(printf '%06d' "$sequence").log.part"
    mv "$tmp" "$target"
    printf '%s\tlogs\t%s\t%s\t%s\t%s\t%s\n' "$(utc)" "$source" "$target" "$(sha "$target")" "$start" "$length" >> "$session/snapshots.tsv"
    printf '%s|%s|%s|%s\n' "$identity" "$size" "$prefix" "$prefix_len" > "$state_file.tmp"
    mv "$state_file.tmp" "$state_file"
    event log-delta "Recorded $length changed log bytes."
}

capture_once() {
    local force_save_checkpoint="${1:-no}" path
    if [[ -d "$profile/logs" ]]; then
        while IFS= read -r -d '' path; do
            capture_log "$path" || return 1
        done < <(find "$profile/logs" -maxdepth 1 -type f -name "$log_pattern" -print0 2>/dev/null)
    fi
    if [[ -d "$profile/saves" ]]; then
        while IFS= read -r -d '' path; do
            snapshot "$path" saves yes "$force_save_checkpoint" || return 1
        done < <(find "$profile/saves" -maxdepth 1 -type f \( -name '*.mm7' -o -name '*.mm7.bak' \) -print0 2>/dev/null)
    fi
}

# Both engines log their revision at startup, so the first log ranges hold it.
record_engine_revision() {
    local part revision="" checked=0
    grep -q '^engine_revision=' "$session/identity.txt" && return 0
    for part in "$session"/logs/*.log.part; do
        [[ -f "$part" && -z "$revision" && checked -lt 3 ]] || break
        revision=$(sed -n 's/.*OpenEnroth, revision \([0-9a-fA-F][0-9a-fA-F]*\) built on.*/\1/p' "$part" | head -n 1)
        checked=$((checked + 1))
    done
    [[ -z "$revision" ]] || printf 'engine_revision=%s\n' "$revision" >> "$session/identity.txt"
}

sample_process() {
    local sample
    sample=$(ps -p "$game_pid" -o %cpu= -o rss= 2>/dev/null) || return 0
    set -- $sample
    (( $# == 2 )) || return 0
    printf '%s\t%s\t%s\n' "$(utc)" "$1" "$2" >> "$session/samples.tsv"
}

# macOS names a report after the process, such as MM7-2026-11-10-213455.ips,
# and writes it a few seconds after the crash.
collect_crash_reports() {
    local wait_seconds="$1" reports="$HOME/Library/Logs/DiagnosticReports" report found=0 waited=0
    while true; do
        if [[ -d "$reports" ]]; then
            while IFS= read -r -d '' report; do
                grep -Eq "\"pid\" *: *${game_pid}([^0-9]|\$)|\[${game_pid}\]" "$report" 2>/dev/null || continue
                [[ ! -e "$session/crash-reports/$(basename "$report")" ]] || { found=1; continue; }
                check_destination || return 1
                mkdir -p "$session/crash-reports"
                cp "$report" "$session/crash-reports/" 2>/dev/null || continue
                event crash-report "Copied $report"
                found=1
            done < <(find "$reports" -maxdepth 1 -type f \( -name "${helper_name}-*.ips" -o -name "${helper_name}_*.crash" \) \
                -newer "$session/.ec-mm7-playtest-session" -print0 2>/dev/null)
        fi
        (( found == 0 && waited < wait_seconds )) || break
        sleep 1
        waited=$((waited + 1))
    done
    (( found )) || event crash-report "No macOS crash report for PID $game_pid."
}

video_pid=""
video_file=""
video_counter=0
video_state="off"
video_enabled=0
video_restart=1
pending_clip=""
clip_counter=0
clips_saved=0
last_clip_saved=0
video_running() {
    local status
    [[ -n "$video_pid" ]] || return 1
    status=$(ps -p "$video_pid" -o stat= 2>/dev/null) || return 1
    status=${status//[[:space:]]/}
    [[ -n "$status" && "$status" != Z* ]]
}

# Completed chunks, oldest first. Zero-padded names sort in recording order.
completed_chunks() {
    local chunk
    for chunk in "$session"/video/chunk-*.mov; do
        [[ -f "$chunk" ]] && printf '%s\n' "$chunk"
    done
}

prune_video_buffer() {
    local keep="$1" chunks=() chunk extra
    while IFS= read -r chunk; do chunks+=("$chunk"); done < <(completed_chunks)
    extra=$(( ${#chunks[@]} - keep ))
    while (( extra > 0 )); do
        unlink "${chunks[$((extra - 1))]}" 2>/dev/null || true
        extra=$((extra - 1))
    done
}

# A hard link costs no space and survives the roll. exFAT and other volumes
# without hard links get a copy instead.
add_to_clip() {
    local source="$1" clip="$2" target tmp
    target="$clip/$(basename "$source")"
    [[ ! -e "$target" ]] || return 0
    ln "$source" "$target" 2>/dev/null && return 0
    check_destination || return 1
    tmp=$(mktemp "$clip/.incoming.XXXXXX") || return 1
    if cp "$source" "$tmp" 2>/dev/null && mv "$tmp" "$target"; then
        return 0
    fi
    unlink "$tmp" 2>/dev/null || true
    warn "Could not copy a video chunk into $clip."
}

# The chunk being recorded joins the clip when it completes.
open_clip() {
    local reason="$1" chunk
    clip_counter=$((clip_counter + 1))
    pending_clip="$session/clips/clip-$(stamp)-$(printf '%03d' "$clip_counter")"
    mkdir "$pending_clip" || { pending_clip=""; return 1; }
    while IFS= read -r chunk; do
        add_to_clip "$chunk" "$pending_clip" || return 1
    done < <(completed_chunks)
    event video-clip "Clip opened ($reason): $pending_clip"
    [[ -n "$video_pid" ]] || close_clip
}

close_clip() {
    local count
    [[ -n "$pending_clip" ]] || return 0
    count=$(find "$pending_clip" -maxdepth 1 -type f -name 'chunk-*.mov' | wc -l | tr -d ' ')
    if (( count == 0 )); then
        rmdir "$pending_clip" 2>/dev/null || true
        last_clip_saved=0
        event video-clip "No video was available for the clip."
    else
        clips_saved=$((clips_saved + 1))
        last_clip_saved=1
        event video-clip "Clip saved with $count chunk(s): $pending_clip"
        write_status
    fi
    pending_clip=""
}

finish_video_chunk() {
    local exit_status=0 target
    [[ -n "$video_pid" ]] || return 0
    wait "$video_pid" 2>/dev/null || exit_status=$?
    video_pid=""
    target="$session/video/chunk-$(printf '%06d' "$video_counter").mov"
    if [[ "$exit_status" -ne 0 || ! -s "$video_file" ]]; then
        unlink "$video_file" 2>/dev/null || true
        video_enabled=0
        video_state="failed"
        warn "Screen recording produced no video; check macOS Screen Recording permission for Enroth Continuum. Logs and saves continue."
    else
        # The buffer keeps video_keep completed chunks, which with the chunk
        # being recorded hold at least the last two minutes.
        prune_video_buffer $((video_keep - 1))
        mv "$video_file" "$target"
        event video "Rolling video chunk completed."
        if [[ -n "$pending_clip" ]]; then
            add_to_clip "$target" "$pending_clip" || return 1
        fi
    fi
    video_file=""
    close_clip
}

# Stops the chunk being recorded and deletes it. Clips never include it.
stop_video() {
    local tries=0
    [[ -n "$video_pid" ]] || return 0
    if video_running; then
        kill -TERM "$video_pid" 2>/dev/null || true  # Recorder-owned capture only, never the game.
        while video_running && (( tries < 5 )); do
            sleep 1
            tries=$((tries + 1))
        done
        if video_running; then kill -KILL "$video_pid" 2>/dev/null || true; fi
    fi
    wait "$video_pid" 2>/dev/null || true
    video_pid=""
    [[ -z "$video_file" ]] || unlink "$video_file" 2>/dev/null || true
    video_file=""
}

check_video() {
    [[ "$video_enabled" == 1 ]] || return 0
    if [[ -n "$video_pid" ]] && ! video_running; then
        finish_video_chunk || return 1
    fi
    if [[ "$video_enabled" == 1 && "$video_restart" == 1 && -z "$video_pid" ]] && (( requested == 0 )); then
        check_destination || return 1
        video_counter=$((video_counter + 1))
        video_file="$session/video/.chunk-$(printf '%06d' "$video_counter").recording.mov"
        local video_command=(/usr/sbin/screencapture -x -v "-V${video_chunk_seconds}" "-D${display}")
        "${video_command[@]}" "$video_file" \
            >> "$session/video-command.log" 2>&1 &
        video_pid=$!
        video_state="rolling"
        event video "Rolling video chunk started."
        write_status
    fi
}

take_clip_request() {
    [[ -f "$session/.save-clip-requested" ]] || return 0
    unlink "$session/.save-clip-requested" 2>/dev/null || true
    if [[ "$video_enabled" != 1 ]]; then
        event video-clip "Clip requested while video is not recording."
    elif [[ -n "$pending_clip" ]]; then
        event video-clip "Clip request joined the clip in progress."
    else
        open_clip "owner request" || return 1
    fi
}

service_video() {
    take_clip_request || return 1
    check_video
}

# Sleeps in one-second steps so a finished chunk is replaced at once.
idle() {
    local remaining="$1"
    while (( remaining > 0 )); do
        sleep 1 || true
        (( requested == 0 )) || return 0
        service_video || return 1
        remaining=$((remaining - 1))
    done
}

# An abnormal exit waits for the chunk being recorded to complete, so the
# clip holds the moments around the exit.
drain_video_into_clip() {
    local waited=0 limit=$((video_chunk_seconds + 15)) signals="$requested"
    while video_running && (( waited < limit && requested == signals )); do
        sleep 1 || true
        waited=$((waited + 1))
    done
    if [[ -n "$video_pid" ]] && ! video_running; then finish_video_chunk || return 1; fi
}

mode="${1:-}"
[[ -n "$mode" ]] || { usage; exit 2; }
shift
profile_arg=""; output_arg=""; session_arg=""; app_arg=""; game_pid=""; note_text=""; expected_volume_id=""; game_exit=""
build=mm7; diagnostics_enabled=1; exit_detail=""
display=1; video_chunk_seconds=60; poll_seconds=5; save_checkpoint_seconds=600; min_free_mib=2048
while (( $# > 0 )); do
    case "$1" in
        --profile|--output|--session|--app|--game-pid|--display|--video-chunk-seconds|--poll-seconds|--save-checkpoint-seconds|--min-free-mib|--expected-volume-id|--text|--game-exit|--build|--exit-detail)
            (( $# >= 2 )) || fail "missing value for $1"
            case "$1" in
                --profile) profile_arg="$2" ;; --output) output_arg="$2" ;;
                --session) session_arg="$2" ;; --app) app_arg="$2" ;;
                --game-pid) game_pid="$2" ;; --display) display="$2" ;;
                --video-chunk-seconds) video_chunk_seconds="$2" ;; --poll-seconds) poll_seconds="$2" ;;
                --save-checkpoint-seconds) save_checkpoint_seconds="$2" ;;
                --min-free-mib) min_free_mib="$2" ;; --text) note_text="$2" ;;
                --expected-volume-id) expected_volume_id="$2" ;; --game-exit) game_exit="$2" ;;
                --build) build="$2" ;; --exit-detail) exit_detail="$2" ;;
            esac
            shift 2 ;;
        --video) video_enabled=1; shift ;;
        --no-video) video_enabled=0; shift ;;
        --diagnostics) diagnostics_enabled=1; shift ;;
        --no-diagnostics) diagnostics_enabled=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) fail "unknown argument: $1" ;;
    esac
done

if [[ "$mode" == note || "$mode" == clip || "$mode" == stop || "$mode" == status ]]; then
    [[ -n "$session_arg" ]] || fail "$mode requires --session"
    validate_session
    case "$mode" in
        note)
            [[ -n "$note_text" ]] || fail "note requires --text"
            note_text=${note_text//$'\n'/ }
            note_text=${note_text//$'\t'/ }
            note_text=${note_text:0:500}
            event owner-note "$note_text" ;;
        clip) : > "$session/.save-clip-requested" ;;
        stop)
            [[ -z "$game_exit" || "$game_exit" == normal || "$game_exit" == abnormal ]] ||
                fail "--game-exit must be normal or abnormal"
            exit_detail=${exit_detail//$'\n'/ }
            exit_detail=${exit_detail//$'\t'/ }
            printf '%s\n%s\n' "$game_exit" "${exit_detail:0:200}" > "$session/.stop-requested.$$"
            mv "$session/.stop-requested.$$" "$session/.stop-requested" ;;
        status) cat "$session/status.json" ;;
    esac
    exit 0
fi
[[ "$mode" == start ]] || fail "expected start, note, clip, stop, or status"
[[ -n "$profile_arg" && -n "$app_arg" && -n "$game_pid" ]] || fail "start requires --profile, --app, and --game-pid"
[[ -n "$output_arg" && -z "$session_arg" || -z "$output_arg" && -n "$session_arg" ]] ||
    fail "start requires exactly one of --output or --session"
for number in "$game_pid" "$display" "$video_chunk_seconds" "$poll_seconds" "$save_checkpoint_seconds" "$min_free_mib"; do
    is_uint "$number" || fail "PID and numeric options must be nonnegative integers"
done
[[ -z "$expected_volume_id" ]] || is_uint "$expected_volume_id" || fail "--expected-volume-id must be an integer"
(( game_pid > 1 && display > 0 && video_chunk_seconds >= 10 && video_chunk_seconds <= 3600 &&
   poll_seconds >= 1 && poll_seconds <= 60 && save_checkpoint_seconds >= 1 &&
   save_checkpoint_seconds <= 3600 && min_free_mib >= 128 )) || fail "numeric option is out of range"
video_keep=$(( (120 + video_chunk_seconds - 1) / video_chunk_seconds ))
case "$build" in
    mm7) helper_name=MM7; log_pattern='enroth-continuum-mm7*.log' ;;
    openenroth) helper_name=OpenEnroth; log_pattern='openenroth*.log' ;;
    *) fail "--build must be mm7 or openenroth" ;;
esac
(( video_enabled || diagnostics_enabled )) || fail "nothing to record: video and diagnostics are both off"
[[ "$video_enabled" != 1 || -x /usr/sbin/screencapture ]] || fail "macOS screencapture is unavailable"
profile=$(canonical_existing_dir "$profile_arg")
app=$(canonical_existing_dir "$app_arg")
helper="$app/Contents/Helpers/$helper_name.app"
[[ -x "$app/Contents/MacOS/EnrothContinuum" && -x "$helper/Contents/MacOS/$helper_name" ]] ||
    fail "--app must be an Atlas bundle with its $helper_name helper"
if [[ -n "$output_arg" ]]; then
    base=$(canonical_existing_dir "$output_arg")
    [[ -z "$expected_volume_id" || "$(device "$base")" == "$expected_volume_id" ]] || fail "recording destination volume changed"
    check_private_destination "$base" "$base/.ec-mm7-playtest-check"
    [[ "$base" != "$profile" && "$base" != "$profile/"* ]] || fail "output base must be outside the MM7 profile"
    session=$(mktemp -d "$base/mm7-playtest-$(stamp)-XXXXXX") || fail "cannot create session directory"
else
    [[ "$session_arg" == /* && ! -e "$session_arg" && ! -L "$session_arg" ]] || fail "--session must name a new absolute directory"
    base=$(canonical_existing_dir "$(dirname "$session_arg")")
    [[ -z "$expected_volume_id" || "$(device "$base")" == "$expected_volume_id" ]] || fail "recording destination volume changed"
    session="$base/$(basename "$session_arg")"
    check_private_destination "$base" "$session"
    [[ "$session" != "$profile" && "$session" != "$profile/"* ]] || fail "session must be outside the MM7 profile"
    mkdir "$session" || fail "cannot create session directory"
fi
volume_id=$(device "$session")
if [[ -n "$expected_volume_id" && "$volume_id" != "$expected_volume_id" ]]; then
    rmdir "$session" 2>/dev/null || true
    fail "recording destination volume changed during session creation"
fi
[[ -n "$volume_id" && "$volume_id" == "$(device "$base")" ]] || fail "session is not on the selected destination volume"
for folder in logs saves state video clips; do mkdir "$session/$folder"; done
printf 'EnrothContinuum-MM7-playtest-v1\n' > "$session/.ec-mm7-playtest-session"
printf '%s\n' "$volume_id" > "$session/.volume-id"
: > "$session/timeline.tsv"; : > "$session/snapshots.tsv"
started_at=$(utc); ended_json=null; state=running; warning_count=0; sequence=0
if [[ "$video_enabled" == 1 ]]; then video_state=ready; fi
write_status
printf 'SESSION_DIR=%s\n' "$session"

script_path="$(cd "$(dirname "$0")" && pwd -P)/$(basename "$0")"
source_root=$(cd "$(dirname "$script_path")/.." && pwd -P)
source_revision=$(git -C "$source_root" rev-parse HEAD 2>/dev/null || printf 'unavailable')
atlas_id=$(plist_value "$app/Contents/Info.plist" CFBundleIdentifier)
helper_id=$(plist_value "$helper/Contents/Info.plist" CFBundleIdentifier)
helper_version=$(plist_value "$helper/Contents/Info.plist" CFBundleShortVersionString)
helper_commit=unavailable
build_info="$app/Contents/Resources/Helpers/$helper_name-BUILD-INFO.txt"
if [[ -f "$build_info" ]]; then
    helper_commit=$(sed -n 's/^commit: *//p' "$build_info" | head -n 1)
fi
{
    printf 'started_at=%s\nprofile=%s\noutput=%s\n' "$started_at" "$profile" "$session"
    printf 'source_revision=%s\nscript_sha256=%s\n' "$source_revision" "$(sha "$script_path")"
    printf 'atlas_path=%s\natlas_id=%s\natlas_sha256=%s\n' "$app" "$atlas_id" "$(sha "$app/Contents/MacOS/EnrothContinuum")"
    printf 'build=%s\nhelper_path=%s\nhelper_id=%s\nhelper_version=%s\nhelper_commit=%s\nhelper_sha256=%s\n' \
        "$build" "$helper" "$helper_id" "$helper_version" "${helper_commit:-unavailable}" "$(sha "$helper/Contents/MacOS/$helper_name")"
    printf 'launch_command=%s\n' "$(ps -p "$game_pid" -o args= 2>/dev/null || printf 'unavailable')"
    printf 'game_pid=%s\nvolume_id=%s\nvideo_requested=%s\ndiagnostics_requested=%s\n' \
        "$game_pid" "$volume_id" "$video_enabled" "$diagnostics_enabled"
} > "$session/identity.txt"
event start "Passive MM7 recording started."
if [[ "$video_enabled" == 1 ]]; then
    event notice "Video records the selected display; other visible desktop content may appear."
fi

game_start=$(ps -p "$game_pid" -o lstart= 2>/dev/null || true)
requested=0
next_save_checkpoint_epoch=$(( $(date +%s) + save_checkpoint_seconds ))
next_sample_epoch=0
trap 'requested=$((requested + 1))' INT TERM
stop_reason="stopped"
while true; do
    if (( requested > 0 )) || [[ -f "$session/.stop-requested" ]]; then
        stop_reason="stopped"
        break
    fi
    current_start=$(ps -p "$game_pid" -o lstart= 2>/dev/null || true)
    if [[ -z "$game_start" || "$current_start" != "$game_start" ]]; then
        stop_reason="game-exited"
        break
    fi
    if ! check_destination; then
        stop_reason="$state"
        break
    fi
    service_video || { stop_reason="$state"; break; }
    now_epoch=$(date +%s)
    force_save_checkpoint=no
    if (( now_epoch >= next_save_checkpoint_epoch )); then
        force_save_checkpoint=yes
        while (( next_save_checkpoint_epoch <= now_epoch )); do
            next_save_checkpoint_epoch=$((next_save_checkpoint_epoch + save_checkpoint_seconds))
        done
    fi
    if (( diagnostics_enabled )); then
        capture_once "$force_save_checkpoint" || { stop_reason="$state"; break; }
        record_engine_revision
        if (( now_epoch >= next_sample_epoch )); then
            sample_process
            next_sample_epoch=$((now_epoch + 30))
        fi
    fi
    idle "$poll_seconds" || { stop_reason="$state"; break; }
done
video_restart=0
if volume_ok; then
    # Atlas reports how MM7 exited through the stop request, which can land
    # just after the recorder notices the exit itself.
    waited=0
    while [[ "$stop_reason" == game-exited && ! -f "$session/.stop-requested" ]] &&
          (( waited < 10 && requested == 0 )); do
        idle 1 || true
        waited=$((waited + 1))
    done
    take_clip_request || true
    game_exit=""; exit_detail=""
    if [[ -f "$session/.stop-requested" ]]; then
        game_exit=$(sed -n 1p "$session/.stop-requested")
        exit_detail=$(sed -n 2p "$session/.stop-requested")
    fi
    keep_video=0
    if [[ "$stop_reason" == stopped || "$stop_reason" == game-exited ]]; then
        if [[ "$game_exit" == abnormal || ( -z "$game_exit" && "$stop_reason" == game-exited ) ]]; then
            keep_video=1
        fi
        [[ -z "$pending_clip" ]] || keep_video=1
    fi
    if (( diagnostics_enabled )) && [[ "$stop_reason" != low-space ]]; then
        capture_once || true
        record_engine_revision
    fi
    if (( keep_video )) && [[ "$video_enabled" == 1 ]]; then
        [[ -n "$pending_clip" ]] || open_clip "game exit ${game_exit:-unreported}" || true
        drain_video_into_clip || true
    fi
    stop_video
    close_clip
    if (( diagnostics_enabled )) && [[ "$stop_reason" != low-space && "$game_exit" != normal ]]; then
        crash_wait=0
        [[ "$game_exit" != abnormal ]] || crash_wait=15
        collect_crash_reports "$crash_wait" || true
    fi
    while IFS= read -r chunk; do unlink "$chunk" 2>/dev/null || true; done < <(completed_chunks)
    if [[ "$video_state" != off && "$video_state" != failed ]]; then
        if (( keep_video && last_clip_saved )); then video_state="kept"; else video_state="discarded"; fi
        event video "Rolling video buffer deleted ($video_state)."
    fi
    state="$stop_reason"
    ended_json="\"$(utc)\""
    event stop "$state${game_exit:+ ($game_exit exit${exit_detail:+, $exit_detail})}"
    write_status
else
    stop_video
fi
printf 'mm7 capture: %s (%s)\n' "$stop_reason" "$session" >&2
