# Redmagic Trigger Bridge 0.2.0

Version 0.2.0 is the first public release candidate of the root companion
backend for Redmagic 11 Toolbox.

It converts the retained NX809J capacitive shoulder inputs into two independent
virtual touchscreen contacts. The daemon starts inactive and accepts ownership
only when Toolbox explicitly selects the module fallback for a configured
foreground game.

## Validated behavior

- Left and right taps, holds, and simultaneous contacts
- No injected contacts while inactive
- Immediate forced release during deactivation
- Direct SAR arming and restoration of captured hardware modes
- Dynamic input discovery without hard-coded event numbers
- Safe coexistence on stock firmware while native TGK remains preferred
- KernelSU 3.3.0 installation and boot lifecycle on stock Android 16

## Compatibility

This release is restricted to the REDMAGIC 11 Pro (`NX809J`). A compatible ROM
must retain the expected SAR devices and mode nodes, the Synaptics touchscreen
input description, `/dev/uinput`, and suitable root/SELinux access.

The custom-ROM automatic fallback path has not yet been validated on an actual
custom ROM, so this release is intentionally marked as a GitHub prerelease.

## Installation

Flash `Redmagic-Trigger-Bridge-v0.2.0.zip` through KernelSU, Magisk, or APatch,
then reboot. A SHA-256 sidecar is included for artifact verification. Keep the
module enabled and allow Redmagic 11 Toolbox to manage activation. Manual
activation is intended only for diagnostics.
