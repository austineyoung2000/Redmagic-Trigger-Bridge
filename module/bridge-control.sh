#!/system/bin/sh

STATE_DIR="/data/adb/redmagic_trigger_bridge"
ACTIVE_FILE="$STATE_DIR/active"
PID_FILE="$STATE_DIR/bridge.pid"

notify_daemon() {
    pid="$(cat "$PID_FILE" 2>/dev/null)"
    case "$pid" in
        ""|*[!0-9]*) return 0 ;;
        *) kill -USR1 "$pid" 2>/dev/null || true ;;
    esac
}

mkdir -p "$STATE_DIR" || exit 1
chmod 0700 "$STATE_DIR"

case "${1:-status}" in
    on|enable|activate)
        : > "$ACTIVE_FILE" || exit 1
        chmod 0600 "$ACTIVE_FILE"
        notify_daemon
        printf 'active\n'
        ;;
    off|disable|deactivate)
        rm -f "$ACTIVE_FILE"
        notify_daemon
        printf 'inactive\n'
        ;;
    reload)
        notify_daemon
        printf 'reloaded\n'
        ;;
    status)
        if [ -e "$ACTIVE_FILE" ]; then
            printf 'active\n'
        else
            printf 'inactive\n'
        fi
        ;;
    *)
        printf 'Usage: %s {on|off|reload|status}\n' "$0" >&2
        exit 2
        ;;
esac
