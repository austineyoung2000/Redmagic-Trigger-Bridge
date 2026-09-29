# Activation and Toolbox integration

The module daemon starts at boot but does not arm or grab the shoulder triggers
or physical touchscreen. It waits without a virtual input device until an
explicit activation request. Activation creates the merged touchscreen proxy;
deactivation releases all contacts and destroys it.

## Installed command interface

The standard installed module path is:

```text
/data/adb/modules/redmagic_trigger_bridge/bridge-control.sh
```

Commands:

```sh
# Begin gameplay ownership.
su -c '/data/adb/modules/redmagic_trigger_bridge/bridge-control.sh on'

# Release contacts and physical inputs, destroy the proxy, and restore modes.
su -c '/data/adb/modules/redmagic_trigger_bridge/bridge-control.sh off'

# Reload config.conf. An active bridge is released before the new config
# is applied, then reacquired if the active marker still exists.
su -c '/data/adb/modules/redmagic_trigger_bridge/bridge-control.sh reload'

# Print active or inactive.
su -c '/data/adb/modules/redmagic_trigger_bridge/bridge-control.sh status'
```

KernelSU/APatch module managers may also expose `action.sh`. Its action button
toggles the same active marker, which is useful for manual development tests.

## Toolbox lifecycle contract

Redmagic 11 Toolbox should treat ownership as scoped to a configured gameplay
session:

1. Write the desired normalized targets to
   `/data/adb/redmagic_trigger_bridge/config.conf` using an atomic replacement.
2. Call `bridge-control.sh reload` after configuration changes.
3. Call `bridge-control.sh on` only after a configured game enters the
   foreground and the bridge backend is selected.
4. Call `bridge-control.sh off` when the game leaves the foreground, trigger
   mapping is disabled, the backend changes, or the gameplay runtime shuts
   down.
5. Make `off` part of every cleanup/error path. It is safe and idempotent.

On stock firmware, Toolbox prefers native TGK and keeps this bridge inactive.
Module installation or enablement in a root manager does not imply active input
ownership. The bridge is selected only when native TGK cannot be applied and
verified.

The module removes the active marker on a fresh boot or root-service restart.
If the daemon alone crashes during an active gameplay session, the supervisor
may restart it and the retained marker allows recovery. A full service restart
returns to the fail-safe inactive state.

## Configuration ownership

The private state directory is:

```text
/data/adb/redmagic_trigger_bridge
```

It is root-only (`0700`). `config.conf` should remain `0600`. Do not hard-code
Linux event numbers; the daemon discovers the physical trigger inputs and the
touchscreen by their kernel device names.
