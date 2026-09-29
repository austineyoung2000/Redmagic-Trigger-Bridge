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

Initial development scaffold. Raw F7/F8 events, simultaneous holds, direct SAR
arming, and uinput availability have been verified on an NX809J running stock
Android 16 with KernelSU.

The first release will include:

- an arm64 Android daemon using two fixed multitouch slots;
- safe touch release during shutdown and input-device reconnects;
- normalized per-rotation target coordinates;
- optional exclusive grabs to prevent duplicate stock handling;
- KernelSU, Magisk, and APatch-compatible packaging;
- a GitHub Actions build producing a flashable module ZIP.

## Safety model

The daemon refuses to run on devices other than `NX809J`. It dynamically checks
all required input and sysfs interfaces, releases every virtual contact before
exit, and destroys its uinput device on shutdown. The module supervisor uses a
bounded restart delay rather than a tight crash loop.

## Attribution

The design was informed by public NX809J device-tree research by IronShing,
including the discovery of the SAR input names, F7/F8 events, trigger arming
nodes, and the feasibility of a uinput multitouch bridge. This repository uses a
new implementation intended for root-module and Toolbox integration.

## License

GPL-3.0-or-later. See `LICENSE`.
