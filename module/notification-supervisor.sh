#!/system/bin/sh
# NX809J-only listener recovery. Never writes LED, fan, pump or trigger nodes.
MODDIR="${0%/*}"
STATE_DIR=/data/adb/redmagic_trigger_bridge
PACKAGE=com.elitedarkkaiser.redmagic
COMPONENT=com.elitedarkkaiser.redmagic.NotificationLightingService
APP_DIR=/data/user/0/$PACKAGE
BEACON=$APP_DIR/no_backup/notification-listener-recovery
PREFS=$APP_DIR/shared_prefs/notification_lighting.xml
LOG=$STATE_DIR/notification-supervisor.log
LOCK=$STATE_DIR/notification-supervisor.lock
PATH=/system/bin:/system_ext/bin:/vendor/bin:/product/bin
export PATH

stopping() { [ -e "$MODDIR/disable" ] || [ -e "$MODDIR/remove" ] || [ -e "$STATE_DIR/disabled" ]; }
log() {
    size=$(wc -c < "$LOG" 2>/dev/null)
    case "$size" in ''|*[!0-9]*) ;; *) [ "$size" -lt 65536 ] || mv -f "$LOG" "$LOG.previous" ;; esac
    printf '%s %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$*" >> "$LOG"
}
enabled() { grep -q '<boolean name="enabled" value="true"' "$PREFS" 2>/dev/null; }
approved() {
    listeners=$(timeout 8 su 2000 -c 'settings --user 0 get secure enabled_notification_listeners' 2>/dev/null)
    status=$?
    [ "$status" -eq 0 ] || { log "notification access lookup failed status=$status"; return 1; }
    case ":$listeners:" in *":$PACKAGE/$COMPONENT:"*|*":$PACKAGE/.NotificationLightingService:"*) return 0 ;; *) return 1 ;; esac
}
read_beacon() {
    version= boot= pid= uid= start= desired= connected=
    snapshot=$(cat "$BEACON" 2>/dev/null) || return 1
    while IFS='=' read -r key value; do
        case "$key" in
            version) version=$value ;; boot) boot=$value ;; pid) pid=$value ;;
            uid) uid=$value ;; start) start=$value ;; desired) desired=$value ;;
            connected) connected=$value ;;
        esac
    done <<EOF
$snapshot
EOF
    [ "$version" = 1 ] && [ "$boot" = "$CURRENT_BOOT" ] || return 1
    case "$pid:$uid:$start" in *[!0-9:]*|::*|*::*) return 1 ;; esac
    [ -n "$pid" ] && [ -n "$uid" ] && [ -n "$start" ] || return 1
    [ "$uid" = "$(stat -c %u "$APP_DIR" 2>/dev/null)" ] && [ "$uid" -ge 10000 ] || return 1
    [ "$desired" = true ] || return 1
}
healthy() {
    read_beacon && [ "$connected" = true ] || return 1
    # Fence PID reuse without reading notification contents or dumping Android services.
    [ "$(cat /proc/"$pid"/cmdline 2>/dev/null | tr '\000' '\n' | head -n 1)" = "$PACKAGE" ] || return 1
    current_stat=$(cat /proc/"$pid"/stat 2>/dev/null) || return 1
    current_stat=${current_stat##*) }
    set -- $current_stat
    shift 19
    [ "$1" = "$start" ]
}
binder_call() {
    # Values and ComponentName parcel layout verified from this exact uploaded firmware.
    # The proven one-shot allowance expires after 500ms; arm in the same root invocation as bind.
    launch_gate=""
    if [ "$1" = 81 ]; then
        launch_gate="su 2000 -c 'service call AutoLaunch 6 s16 $PACKAGE' >/dev/null 2>&1; "
    fi
    output=$(timeout 8 su -c "${launch_gate}su $uid -c 'service call notification $1 i32 1 s16 $PACKAGE s16 $COMPONENT'" 2>&1)
    status=$?
    [ "$status" -eq 0 ] || { log "Binder transaction $1 failed status=$status response=$(printf '%s' "$output" | head -c 1200 | tr '\n' ' ')"; return 1; }
    compact=$(printf '%s' "$output" | tr -d '[:space:]')
    case "$compact" in *"Result:Parcel(00000000"*|*"Result:Parcel(0x00000000:00000000"*) return 0 ;; *) log "Binder transaction $1 lacked no-exception receipt response=$(printf '%s' "$output" | head -c 1200 | tr '\n' ' ')"; return 1 ;; esac
}
recover() {
    enabled && approved || return 1
    log "listener unavailable; requesting Toolbox recovery"
    timeout 8 su 2000 -c "service call AutoLaunch 6 s16 $PACKAGE" >/dev/null 2>&1
    timeout 8 su 2000 -c "am broadcast --user 0 --include-stopped-packages -a com.elitedarkkaiser.redmagic.ROOT_BOOT_STARTUP -n $PACKAGE/.RootBootReceiver" >/dev/null 2>&1
    sleep 8
    healthy && return 0
    # Require the new APK's handoff and recheck access immediately before changing binding.
    read_beacon && enabled && approved || { log "no eligible current-boot APK handoff; recovery deferred"; return 1; }
    log "resetting only Toolbox listener binding as uid=$uid"
    binder_call 83
    unbind_status=$?
    # A missing receipt does not prove unbind did not happen. Always attempt rebind.
    binder_call 81
    bind_status=$?
    if [ "$unbind_status" -ne 0 ] || [ "$bind_status" -ne 0 ]; then
        identity=$(timeout 8 su -c "su $uid -c id" 2>&1 | head -c 400 | tr '\n' ' ')
        log "recovery caller=$identity; unbind=$unbind_status bind=$bind_status"
    fi
    sleep 8
    healthy || { log "listener callback absent after reset; unbind=$unbind_status bind=$bind_status"; return 1; }
    log "listener connected: callback acknowledged pid=$pid"
    return 0
}

acquire_lock() {
    if mkdir "$LOCK" 2>/dev/null; then printf '%s\n' "$$" > "$LOCK/pid"; return 0; fi
    previous=$(cat "$LOCK/pid" 2>/dev/null)
    case "$previous" in ''|*[!0-9]*) return 1 ;; esac
    # A reused PID is conservatively treated as alive; never kill a process to acquire ownership.
    kill -0 "$previous" 2>/dev/null && return 1
    rm -f "$LOCK/pid"
    rmdir "$LOCK" 2>/dev/null || return 1
    mkdir "$LOCK" 2>/dev/null || return 1
    printf '%s\n' "$$" > "$LOCK/pid"
}
release_lock() {
    [ "$(cat "$LOCK/pid" 2>/dev/null)" = "$$" ] || return
    rm -f "$LOCK/pid"
    rmdir "$LOCK" 2>/dev/null
}

device=$(getprop ro.product.device)
model=$(getprop ro.product.model)
[ "$device" = NX809J ] || [ "$model" = NX809J ] || exit 0
# Fail closed on different Binder layouts, including future firmware updates.
mkdir -p "$STATE_DIR"
chmod 0700 "$STATE_DIR"
log "notification supervisor startup context=$(id)"
[ "$(sha256sum /system/framework/framework.jar | cut -d ' ' -f 1)" = f7aafb0787ae83d014ff44dcc1543273294a2e0d68725d589a3d97ce79e122df ] || { log "firmware gate rejected framework.jar"; exit 0; }
[ "$(sha256sum /system/framework/services.jar | cut -d ' ' -f 1)" = e0e1e0a8ed7c887eddf4eec150845b99c7bacc245ac3024df3852364fe3a5a7e ] || { log "firmware gate rejected services.jar"; exit 0; }
mkdir -p "$STATE_DIR"
chmod 0700 "$STATE_DIR"
acquire_lock || exit 0
trap release_lock EXIT
trap 'exit 0' INT TERM
CURRENT_BOOT=$(cat /proc/sys/kernel/random/boot_id)
log "notification supervisor v1 started pid=$$"
failures=0
while ! stopping; do
    if ! enabled; then failures=0; sleep 60; continue; fi
    if healthy; then failures=0; sleep 20; continue; fi
    state=$(timeout 8 su 2000 -c 'am get-started-user-state 0' 2>/dev/null)
    case "$state" in *RUNNING_UNLOCKED*) ;; *) sleep 30; continue ;; esac
    if recover; then failures=0; else failures=$((failures + 1)); fi
    # No hot loop or wake lock. Failed batches back off five minutes after five attempts.
    if [ "$failures" -ge 5 ]; then log "five unsuccessful recoveries; backing off 300s"; sleep 300; failures=0; else sleep 20; fi
done
log "notification supervisor stopped"
