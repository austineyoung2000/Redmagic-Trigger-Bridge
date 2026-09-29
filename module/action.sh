#!/system/bin/sh

MODDIR="${0%/*}"
STATE_DIR="/data/adb/redmagic_trigger_bridge"

if [ -e "$STATE_DIR/active" ]; then
    exec "$MODDIR/bridge-control.sh" off
else
    exec "$MODDIR/bridge-control.sh" on
fi
