# Redmagic Trigger Bridge

Root companion module for the REDMAGIC 11 Pro (`NX809J`) that converts the
phone's capacitive shoulder-trigger events into independent multitouch contacts.

The project is intended to provide a trigger backend for Redmagic 11 Toolbox on
stock firmware and custom ROMs that retain the NX809J vendor and kernel stack.

## Hardware confirmed on stock Android 16

| Function | Interface |
|---|---|
| Trigger 0 | `nubia_tgk_aw_sar0_ch0`, `KEY_F7` |
| Trigger 1 | `nubia_tgk_aw_sar1_ch0`, `KEY_F8` |
| Trigger arming | `/sys/class/leds/sar0/mode_operation`, `/sys/class/leds/sar1/mode_operation` |
| Touchscreen | `synaptics_tcm_touch` |
| Virtual input | `/dev/uinput` |
| Trigger haptics | `/sys/class/leds/zte_vibrator/{duration,gain,activate}` |

The daemon discovers event nodes by device name; it never assumes that the
current `event4`, `event5`, or `event9` numbering remains stable.

## Status

Version 0.3.0 replaces the separate trigger-only virtual touchscreen with a
merged touchscreen proxy. While active, it exclusively reads the physical
Synaptics touchscreen and both shoulder sensors, then publishes physical
contacts and two reserved trigger contacts through one uinput device. This
allows movement, aiming, multi-finger input, and both shoulder triggers to
coexist in games that reject contacts split across multiple touchscreen
devices.

Version 0.3.1 adds optional press-edge haptic feedback through the
NX809J vibrator interface. Feedback is rate-limited, never runs for key-repeat
or release events, and automatically disables itself without affecting touch
input if the vibrator interface is missing or rejects a write. The daemon
commits the virtual touch-down frame before starting vibrator I/O so haptics
cannot suppress short trigger contacts. Rotation tracking falls back from the
vendor `SurfaceOrientation` field to Android window/display rotation when the
vendor field is absent.

The merged path has been verified on an NX809J running stock Android 16 with
KernelSU 3.3.0. Testing covered physical-touch-only input, repeated taps and
swipes, continuous thumbstick movement, left/right taps and holds, both
triggers together, two physical fingers plus both triggers, contact replacement,
and a two-minute combined-input soak without lost touches or camera snapping.

The release includes:

- an arm64 Android daemon that mirrors the physical multitouch slots and
  reserves two additional slots for the shoulder triggers;
- protocol-B slot selection that keeps physical movement isolated from trigger
  contacts;
- runtime cloning of the physical touchscreen axes and capabilities;
- inactive-by-default ownership controlled explicitly by Toolbox;
- safe touch release during shutdown and input-device reconnects;
- normalized per-rotation target coordinates;
- optional exclusive grabs to prevent duplicate stock handling;
- KernelSU, Magisk, and APatch-compatible packaging;
- a GitHub Actions build producing a flashable module ZIP.

The automatic fallback path is implemented in Redmagic 11 Toolbox, but it has
not yet been exercised end to end on a custom ROM that lacks native TGK. The
merged input engine itself is verified on stock firmware by forcing Toolbox to
select the module backend.

## Installation

1. Download `Redmagic-Trigger-Bridge-v0.3.1.zip` from the GitHub release.
   Verify it against the adjacent `.sha256` file.
2. Install it from KernelSU, Magisk, or APatch.
3. Reboot once.
4. Leave the module enabled. Redmagic 11 Toolbox activates it only when native
   TGK is unavailable and a configured game owns the foreground.

Do not manually activate the bridge during normal Toolbox use. The module
manager action button and `bridge-control.sh` commands are retained for
diagnostics and development testing.

## Compatibility boundary

- Device support is intentionally restricted to `NX809J`.
- Stock firmware continues to use native TGK when that backend applies and
  verifies successfully.
- A custom ROM must retain the NX809J SAR input devices, writable trigger-mode
  nodes, the Synaptics touchscreen input description, `/dev/uinput`, and SELinux
  access compatible with its root implementation.
- The fallback supplies merged physical and trigger contacts. It does not yet
  reproduce native TGK rapid-fire modes or system-server visual effects. Its
  vibrator pulse approximates trigger feedback but does not reproduce ZTE's
  proprietary TGK waveform.
- Root is required. This is not a generic Android trigger module.

## Safety model

The daemon refuses to run on devices other than `NX809J`. It dynamically checks
all required input and sysfs interfaces, releases every virtual contact before
exit, and destroys its uinput device on shutdown. The module supervisor uses a
bounded restart delay rather than a tight crash loop.

The installed daemon boots inactive without a virtual touchscreen. Creating
the private `active` marker—or calling `bridge-control.sh on`—creates the merged
uinput device, arms and acquires both triggers, and temporarily acquires the
physical touchscreen so all contacts can be forwarded together. Calling
`bridge-control.sh off` releases every contact, destroys the virtual device,
relinquishes all physical inputs, and restores the hardware modes observed at
activation. This is the control boundary used by Redmagic 11 Toolbox when a
configured game enters or leaves the foreground.

See [`docs/CONTROL.md`](docs/CONTROL.md) for the command interface and Toolbox
integration contract.

See [`CHANGELOG.md`](CHANGELOG.md) for release history and
[`docs/HARDWARE.md`](docs/HARDWARE.md) for the verified device interfaces.

## Attribution

The design was informed by public NX809J device-tree research by IronShing,
including the discovery of the SAR input names, F7/F8 events, trigger arming
nodes, and the feasibility of a uinput multitouch bridge. This repository uses a
new implementation intended for root-module and Toolbox integration.

## License

GPL-3.0-or-later. See `LICENSE`.

### Root module boot startup

With compatible Toolbox and Trigger Bridge builds installed, the module initializes Toolbox background services after Android finishes booting and user 0 first unlocks. No Toolbox screen opens. Root/system/shell-only receiver access, NX809J validation, and the existing per-boot startup claim protect this fallback. It respects saved lighting/trigger settings and keeps game mappings under normal foreground ownership. Grant required permissions and configure Toolbox once before using automatic startup. This also works while Native TGK owns the triggers.

The module makes up to five broadcast attempts and records the latest result in `/data/adb/redmagic_trigger_bridge/toolbox-boot.log`. Broadcast delivery does not guarantee every configured service started; confirm device behavior after reboot. Older APKs lack this receiver. Both updated APK and module are required.


## Notification listener recovery (test build)

Requires the matching Toolbox test APK with `notification-listener-recovery` in its no-backup directory. The independent module supervisor checks this event-written, notification-free state every 20 seconds while enabled, verifying boot ID, package UID, PID and process start time. It does not poll Android notification dumps, hold a wake lock, or read message content.

After listener loss it uses the existing NX809J AutoLaunch/startup path, then resets only Toolbox's listener binding using the app's UID. The uploaded NX809J framework exposes requestUnbindListenerComponent (83) and requestBindListener (81); ordinary requestRebind is a no-op when the component is already unsnoozed. Firmware hashes must match exactly before these Binder numbers can be used. Access and feature enablement are required and rechecked; no notification permission is granted/revoked and no secure setting is rewritten. Recovery is successful only after the APK records Android's connection callback. Five failed attempts back off for five minutes.

This supervisor does not write LEDs, fan/pump nodes, SAR nodes, or trigger mappings. Existing LED timing/priority/cooling code remains responsible for hardware. Module disable/removal stops supervision. It cannot prevent SIGKILL, reconstruct notifications missed while unbound, run during CPU suspend, or guarantee Binder/root availability. Normal exact-deadline cleanup is still in the APK. Recovery latency includes the check interval and connection grace periods; this is a recovery mechanism, not uninterrupted notification delivery.

Firmware provenance: uploaded framework.jar SHA-256 f7aafb0787ae83d014ff44dcc1543273294a2e0d68725d589a3d97ce79e122df; services.jar SHA-256 e0e1e0a8ed7c887eddf4eec150845b99c7bacc245ac3024df3852364fe3a5a7e. Connection reset and root-manager UID switching require NX809J phone validation before merging.

Diagnostic log: `/data/adb/redmagic_trigger_bridge/notification-supervisor.log` (64 KiB plus one rotated copy). This test does not modify the gameplay supervisor or native input daemon.
