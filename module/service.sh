#!/system/bin/sh

MODDIR="${0%/*}"
STATE_DIR="/data/adb/redmagic_trigger_bridge"
CONFIG_FILE="$STATE_DIR/config.conf"
PID_FILE="$STATE_DIR/bridge.pid"
LOG_FILE="$STATE_DIR/bridge.log"
STOP_FILE="$STATE_DIR/disabled"
ACTIVE_FILE="$STATE_DIR/active"

mkdir -p "$STATE_DIR"
chmod 0700 "$STATE_DIR"

if [ ! -f "$CONFIG_FILE" ]; then
    cp "$MODDIR/config.conf" "$CONFIG_FILE"
    chmod 0600 "$CONFIG_FILE"
fi

rotate_log() {
    [ -f "$LOG_FILE" ] || return 0
    size="$(wc -c < "$LOG_FILE" 2>/dev/null)"
    case "$size" in
        ""|*[!0-9]*) return 0 ;;
    esac
    if [ "$size" -ge 131072 ]; then
        mv -f "$LOG_FILE" "$LOG_FILE.previous"
    fi
}

detect_rotation() {
    rotation="$(
        dumpsys input 2>/dev/null |
            sed -n \
                's/.*SurfaceOrientation:[[:space:]]*\([0-3]\).*/\1/p' |
            head -n 1
    )"

    case "$rotation" in
        0|1|2|3)
            printf '%s\n' "$rotation"
            return 0
            ;;
    esac

    rotation="$(
        dumpsys window displays 2>/dev/null |
            sed -n 's/.*mRotation=\([0-3]\).*/\1/p' |
            head -n 1
    )"

    case "$rotation" in
        0|1|2|3)
            printf '%s\n' "$rotation"
            return 0
            ;;
    esac

    dumpsys display 2>/dev/null |
        sed -n 's/.*mCurrentOrientation=\([0-3]\).*/\1/p' |
        head -n 1
}

rotation_watcher() {
    previous=""
    while [ ! -e "$STOP_FILE" ]; do
        if [ ! -e "$ACTIVE_FILE" ]; then
            previous=""
            sleep 5
            continue
        fi

        current="$(detect_rotation)"
        case "$current" in
            0|1|2|3)
                if [ "$current" != "$previous" ]; then
                    setprop sys.rm.trig_rot "$current"
                    previous="$current"
                fi
                ;;
        esac
        sleep 2
    done
}

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
done

[ -e "$STOP_FILE" ] && exit 0

# Independent from backend ownership: also useful on stock Native TGK.
sh "$MODDIR/toolbox-boot.sh" &

rm -f "$ACTIVE_FILE"

rotate_log
rotation_watcher &
rotation_pid="$!"

delay=1
while [ ! -e "$STOP_FILE" ]; do
    "$MODDIR/bin/redmagic-trigger-bridge" >> "$LOG_FILE" 2>&1 &
    bridge_pid="$!"
    printf '%s\n' "$bridge_pid" > "$PID_FILE"
    wait "$bridge_pid"
    status="$?"
    rm -f "$PID_FILE"

    [ -e "$STOP_FILE" ] && break
    printf '%s daemon exited status=%s; restart in %ss\n' \
        "$(date '+%Y-%m-%d %H:%M:%S')" "$status" "$delay" >> "$LOG_FILE"
    sleep "$delay"
    delay=$((delay * 2))
    [ "$delay" -gt 30 ] && delay=30
done

kill "$rotation_pid" 2>/dev/null
wait "$rotation_pid" 2>/dev/null
rm -f "$PID_FILE"
