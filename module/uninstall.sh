#!/system/bin/sh

STATE_DIR="/data/adb/redmagic_trigger_bridge"
touch "$STATE_DIR/disabled" 2>/dev/null
rm -f "$STATE_DIR/active"

pid="$(cat "$STATE_DIR/bridge.pid" 2>/dev/null)"
case "$pid" in
    ""|*[!0-9]*) ;;
    *) kill "$pid" 2>/dev/null ;;
esac

printf '0\n' > /sys/class/leds/sar0/mode_operation 2>/dev/null
printf '0\n' > /sys/class/leds/sar1/mode_operation 2>/dev/null
rm -rf "$STATE_DIR"
