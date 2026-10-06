#!/system/bin/sh
# Optional Toolbox startup; never launches an Activity or arms trigger mappings.
MODDIR="${0%/*}"
PACKAGE_NAME=com.elitedarkkaiser.redmagic
STATE_DIR=/data/adb/redmagic_trigger_bridge
LOG_FILE="$STATE_DIR/toolbox-boot.log"
PATH=/system/bin:/system_ext/bin:/vendor/bin:/product/bin
export PATH

# Credential-encrypted app preferences become available after first unlock.
while true; do
    [ -e "$MODDIR/disable" ] && exit 0
    [ -e "$MODDIR/remove" ] && exit 0
    state="$(su 2000 -c 'am get-started-user-state 0' 2>/dev/null)"
    case "$state" in *RUNNING_UNLOCKED*) break ;; esac
    sleep 15
done

pm path "$PACKAGE_NAME" >/dev/null 2>&1 || exit 0
mkdir -p "$STATE_DIR"
# Bounded retries; no permanent package/settings polling after startup.
attempt=1
while [ "$attempt" -le 5 ]; do
    [ -e "$MODDIR/disable" ] && exit 0
    [ -e "$MODDIR/remove" ] && exit 0
    su 2000 -c "service call AutoLaunch 6 s16 $PACKAGE_NAME" >/dev/null 2>&1
    output="$(su 2000 -c "am broadcast --user 0 --include-stopped-packages -a com.elitedarkkaiser.redmagic.ROOT_BOOT_STARTUP -n $PACKAGE_NAME/.RootBootReceiver" 2>&1)"
    status=$?
    printf '%s attempt=%s status=%s %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$attempt" "$status" "$output" > "$LOG_FILE"
    case "$output" in
        *'Broadcast completed'*) [ "$status" -eq 0 ] && exit 0 ;;
    esac
    attempt=$((attempt + 1))
    sleep 15
done
exit 1
