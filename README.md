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

The daemon discovers event nodes by device name; it never assumes that the
current `event4`, `event5`, or `event9` numbering remains stable.

## Status

Version 0.2.0 is the first public release candidate. Raw F7/F8 events,
simultaneous holds, direct SAR arming, uinput availability, inactive-by-default
startup, explicit activation, and forced contact release have been verified on
an NX809J running stock Android 16 with KernelSU 3.3.0. The module remained
installed and enabled without interfering while Redmagic 11 Toolbox selected
the stock native TGK backend.

The release includes:

- an arm64 Android daemon using two fixed multitouch slots;
- inactive-by-default ownership controlled explicitly by Toolbox;
- safe touch release during shutdown and input-device reconnects;
- normalized per-rotation target coordinates;
- optional exclusive grabs to prevent duplicate stock handling;
- KernelSU, Magisk, and APatch-compatible packaging;
- a GitHub Actions build producing a flashable module ZIP.

The automatic fallback path is implemented in Redmagic 11 Toolbox, but it has
not yet been exercised end to end on a custom ROM that lacks native TGK. For
that reason, the GitHub release is marked as a prerelease even though the module
metadata uses the final `0.2.0` version.

## Installation

1. Download `Redmagic-Trigger-Bridge-v0.2.0.zip` from the GitHub release.
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
- The fallback supplies independent touch contacts. It does not reproduce
  native TGK haptics, rapid-fire modes, or system-server visual effects.
- Root is required. This is not a generic Android trigger module.

## Safety model

The daemon refuses to run on devices other than `NX809J`. It dynamically checks
all required input and sysfs interfaces, releases every virtual contact before
exit, and destroys its uinput device on shutdown. The module supervisor uses a
bounded restart delay rather than a tight crash loop.

The installed daemon boots inactive. Creating the private `active` marker—or
calling `bridge-control.sh on`—arms and exclusively acquires the triggers.
Calling `bridge-control.sh off` releases every virtual contact, relinquishes
both physical input devices, and restores the hardware modes observed at
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
