#!/bin/bash
# Passive, local MM7 playtest evidence recorder. Compatible with macOS Bash 3.2.
set -euo pipefail
umask 077

usage() {
    cat <<'EOF'
Usage:
  mm7_playtest_capture.sh start --profile ABS_DIR --app ABS_ATLAS_APP --game-pid PID
      (--output ABS_EXISTING_BASE_DIR | --session ABS_NEW_DIR)
      [--video | --no-video] [--display N] [--video-chunk-seconds N]
      [--poll-seconds N] [--save-checkpoint-seconds N] [--min-free-mib N]
      [--expected-volume-id N]
  mm7_playtest_capture.sh note --session ABS_SESSION_DIR --text "owner note"
  mm7_playtest_capture.sh clip --session ABS_SESSION_DIR
  mm7_playtest_capture.sh stop --session ABS_SESSION_DIR
  mm7_playtest_capture.sh status --session ABS_SESSION_DIR

The start command stays in the foreground, prints SESSION_DIR=..., and exits
after the game PID exits or stop is requested. It never launches or signals MM7.
EOF
}

fail() { printf 'mm7 capture: %s\n' "$*" >&2; exit 2; }
utc() { date -u '+%Y-%m-%dT%H:%M:%SZ'; }
stamp() { date -u '+%Y%m%dT%H%M%SZ'; }
is_uint() { [[ "$1" =~ ^[0-9]+$ ]]; }
sha() { shasum -a 256 "$1" | awk '{print $1}'; }
device() { stat -f '%d' "$1" 2>/dev/null; }

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
    printf '{"state":"%s","started_at":"%s","ended_at":%s,"video":"%s","warnings":%s}\n' \
        "$state" "$started_at" "$ended_json" "$video_state" "$warning_count" > "$tmp"
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
        sleep 1
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
        done < <(find "$profile/logs" -maxdepth 1 -type f -name 'enroth-continuum-mm7*.log' -print0 2>/dev/null)
    fi
    if [[ -d "$profile/saves" ]]; then
        while IFS= read -r -d '' path; do
            snapshot "$path" saves yes "$force_save_checkpoint" || return 1
        done < <(find "$profile/saves" -maxdepth 1 -type f \( -name '*.mm7' -o -name '*.mm7.bak' \) -print0 2>/dev/null)
    fi
}

video_pid=""
video_file=""
video_slot=""
next_video_slot="A"
video_state="off"
video_enabled=0
video_running() {
    local status
    [[ -n "$video_pid" ]] || return 1
    status=$(ps -p "$video_pid" -o stat= 2>/dev/null) || return 1
    status=${status//[[:space:]]/}
    [[ -n "$status" && "$status" != Z* ]]
}
discard_current_video() {
    [[ -n "$video_file" && -e "$video_file" ]] || return 0
    unlink "$video_file" 2>/dev/null || true
}

finish_video_chunk() {
    local exit_status=0 target
    [[ -n "$video_pid" ]] || return 0
    wait "$video_pid" 2>/dev/null || exit_status=$?
    video_pid=""
    target="$session/video/rolling-$video_slot.mov"
    if [[ "$exit_status" -ne 0 || ! -s "$video_file" ]]; then
        discard_current_video
        video_enabled=0
        video_state="failed"
        warn "Screen recording produced no rolling video; check macOS Screen Recording permission. Logs and saves continue."
    else
        mv "$video_file" "$target"
        video_state="rolling"
        event video "Rolling video chunk completed."
    fi
    video_file=""
    video_slot=""
}

save_rolling_video_clip() {
    local clip_dir slot source tmp copied=0
    [[ -f "$session/.save-clip-requested" ]] || return 0
    unlink "$session/.save-clip-requested" 2>/dev/null || true
    check_destination || return 1
    clip_dir="$session/clips/clip-$(stamp)-$(printf '%06d' "$sequence")"
    mkdir "$clip_dir" || return 1
    for slot in A B; do
        source="$session/video/rolling-$slot.mov"
        [[ -s "$source" ]] || continue
        tmp=$(mktemp "$clip_dir/.incoming.XXXXXX") || return 1
        if cp "$source" "$tmp" 2>/dev/null && mv "$tmp" "$clip_dir/rolling-$slot.mov"; then
            copied=$((copied + 1))
        else
            unlink "$tmp" 2>/dev/null || true
        fi
    done
    if (( copied == 0 )); then
        rmdir "$clip_dir" 2>/dev/null || true
        warn "Rolling-video save requested before a complete video chunk was available."
        return 0
    fi
    event video-clip "Saved $copied rolling video chunk(s) to $clip_dir."
}

stop_video() {
    [[ -n "$video_pid" ]] || return 0
    local was_running=0
    if video_running; then
        was_running=1
        kill -INT "$video_pid" 2>/dev/null || true  # Recorder-owned capture only, never the game.
        local tries=0
        while video_running && (( tries < 20 )); do
            sleep 1
            tries=$((tries + 1))
        done
        if video_running; then
            kill -TERM "$video_pid" 2>/dev/null || true
            if volume_ok; then
                warn "Screen recorder needed forced termination; its last video chunk may be incomplete."
            fi
            tries=0
            while video_running && (( tries < 5 )); do
                sleep 1
                tries=$((tries + 1))
            done
            if video_running; then
                kill -KILL "$video_pid" 2>/dev/null || true
                if volume_ok; then warn "Screen recorder did not stop after TERM; final video chunk is unreliable."; fi
            fi
        fi
    fi
    if (( was_running )); then
        video_state="interrupted"
        if volume_ok; then event video "Discarded incomplete rolling video chunk at session end."; fi
    fi
    wait "$video_pid" 2>/dev/null || true
    video_pid=""
    discard_current_video
    video_file=""
    video_slot=""
}

check_video() {
    [[ "$video_enabled" == 1 ]] || return 0
    if [[ -n "$video_pid" ]] && ! video_running; then
        finish_video_chunk
    fi
    if [[ "$video_enabled" == 1 && -z "$video_pid" ]]; then
        check_destination || return 1
        sequence=$((sequence + 1))
        video_slot="$next_video_slot"
        if [[ "$next_video_slot" == A ]]; then next_video_slot=B; else next_video_slot=A; fi
        video_file="$session/video/.rolling-$video_slot-recording.mov"
        local video_command=(/usr/sbin/screencapture -x -v "-V${video_chunk_seconds}" "-D${display}")
        "${video_command[@]}" "$video_file" \
            >> "$session/video-command.log" 2>&1 &
        video_pid=$!
        video_state="rolling"
        event video "Rolling video chunk started."
        write_status
    fi
}

mode="${1:-}"
[[ -n "$mode" ]] || { usage; exit 2; }
shift
profile_arg=""; output_arg=""; session_arg=""; app_arg=""; game_pid=""; note_text=""; expected_volume_id=""
display=1; video_chunk_seconds=60; poll_seconds=5; save_checkpoint_seconds=600; min_free_mib=2048
while (( $# > 0 )); do
    case "$1" in
        --profile|--output|--session|--app|--game-pid|--display|--video-chunk-seconds|--poll-seconds|--save-checkpoint-seconds|--min-free-mib|--expected-volume-id|--text)
            (( $# >= 2 )) || fail "missing value for $1"
            case "$1" in
                --profile) profile_arg="$2" ;; --output) output_arg="$2" ;;
                --session) session_arg="$2" ;; --app) app_arg="$2" ;;
                --game-pid) game_pid="$2" ;; --display) display="$2" ;;
                --video-chunk-seconds) video_chunk_seconds="$2" ;; --poll-seconds) poll_seconds="$2" ;;
                --save-checkpoint-seconds) save_checkpoint_seconds="$2" ;;
                --min-free-mib) min_free_mib="$2" ;; --text) note_text="$2" ;;
                --expected-volume-id) expected_volume_id="$2" ;;
            esac
            shift 2 ;;
        --video) video_enabled=1; shift ;;
        --no-video) video_enabled=0; shift ;;
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
        stop) : > "$session/.stop-requested" ;;
        status) cat "$session/status.json" ;;
    esac
    exit 0
fi
[[ "$mode" == start ]] || fail "expected start, note, stop, or status"
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
[[ "$video_enabled" != 1 || -x /usr/sbin/screencapture ]] || fail "macOS screencapture is unavailable"
profile=$(canonical_existing_dir "$profile_arg")
app=$(canonical_existing_dir "$app_arg")
[[ -x "$app/Contents/MacOS/EnrothContinuum" && -x "$app/Contents/Helpers/MM7.app/Contents/MacOS/MM7" ]] ||
    fail "--app must be an Atlas bundle with its MM7 helper"
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
atlas_id=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$app/Contents/Info.plist" 2>/dev/null || printf 'unavailable')
helper_id=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$app/Contents/Helpers/MM7.app/Contents/Info.plist" 2>/dev/null || printf 'unavailable')
{
    printf 'started_at=%s\nprofile=%s\noutput=%s\n' "$started_at" "$profile" "$session"
    printf 'source_revision=%s\nscript_sha256=%s\n' "$source_revision" "$(sha "$script_path")"
    printf 'atlas_path=%s\natlas_id=%s\natlas_sha256=%s\n' "$app" "$atlas_id" "$(sha "$app/Contents/MacOS/EnrothContinuum")"
    printf 'helper_id=%s\nhelper_sha256=%s\n' "$helper_id" "$(sha "$app/Contents/Helpers/MM7.app/Contents/MacOS/MM7")"
    printf 'game_pid=%s\nvolume_id=%s\nvideo_requested=%s\n' "$game_pid" "$volume_id" "$video_enabled"
} > "$session/identity.txt"
event start "Passive MM7 recording started."
if [[ "$video_enabled" == 1 ]]; then
    event notice "Video records the selected display; other visible desktop content may appear."
fi

game_start=$(ps -p "$game_pid" -o lstart= 2>/dev/null || true)
requested=0
next_save_checkpoint_epoch=$(( $(date +%s) + save_checkpoint_seconds ))
trap 'requested=1' INT TERM
stop_reason="stopped"
while true; do
    if [[ "$requested" == 1 || -f "$session/.stop-requested" ]]; then
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
    check_video || { stop_reason="$state"; break; }
    save_rolling_video_clip || { stop_reason="$state"; break; }
    now_epoch=$(date +%s)
    force_save_checkpoint=no
    if (( now_epoch >= next_save_checkpoint_epoch )); then
        force_save_checkpoint=yes
        while (( next_save_checkpoint_epoch <= now_epoch )); do
            next_save_checkpoint_epoch=$((next_save_checkpoint_epoch + save_checkpoint_seconds))
        done
    fi
    capture_once "$force_save_checkpoint" || { stop_reason="$state"; break; }
    sleep "$poll_seconds" || true
done
if volume_ok; then
    if [[ "$stop_reason" != low-space ]]; then capture_once || true; fi
    stop_video
    state="$stop_reason"
    ended_json="\"$(utc)\""
    event stop "$state"
    write_status
else
    stop_video
fi
printf 'mm7 capture: %s (%s)\n' "$stop_reason" "$session" >&2
