# Redmagic Trigger Bridge 0.3.0

Version 0.3.0 is the stable merged-touch release of the root companion backend
for Redmagic 11 Toolbox.

The daemon now combines the physical Synaptics touchscreen and both capacitive
shoulder triggers into one protocol-B uinput device while the module backend is
active. Games receive a single coherent multitouch stream, allowing movement,
aiming, other screen contacts, and both triggers to operate simultaneously.

## Highlights

- Continuous thumbstick movement while pressing or holding either trigger
- Two physical fingers and both shoulder triggers at the same time
- Correct saved L/R target coordinates in landscape
- No virtual touchscreen or physical input grab while inactive
- Clean contact release and virtual-device destruction during deactivation
- Dynamic input discovery with no hard-coded Linux event numbers
- KernelSU, Magisk, and APatch-compatible module packaging

## Important fixes since 0.2.0

- Replaced the separate two-slot trigger touchscreen with a merged proxy.
- Mirrored legacy primary coordinates required by games such as COD Mobile.
- Matched the physical touchscreen's actual axis capabilities.
- Prevented stale coordinates when physical slots are reused.
- Preserved protocol-B slot selection after trigger injection, eliminating the
  intermittent camera/aim snap seen during simultaneous movement and firing.
- Corrected the NX809J landscape coordinate transform.

## Validation

Validated on a REDMAGIC 11 Pro (`NX809J`) running stock Android 16 with
KernelSU 3.3.0. The final test included repeated taps, swipes, continuous
thumbstick movement, independent trigger taps and holds, both triggers, two
physical fingers plus both triggers, repeated finger replacement, and a
two-minute combined-input soak. Touch remained continuous, trigger targets were
correct, and no unexpected aim snapping occurred.

## Installation

Install `Redmagic-Trigger-Bridge-v0.3.0.zip` through KernelSU, Magisk, or
APatch, then reboot once. Verify the ZIP with the published SHA-256 sidecar.
Leave the module enabled and allow Redmagic 11 Toolbox to activate it only when
the module backend is selected for a configured foreground game.

## Compatibility boundary

This release is restricted to the NX809J hardware interfaces. The forced
module-backend path is verified on stock firmware. End-to-end automatic
fallback on an actual custom ROM without native TGK remains unverified. Native
TGK haptics, rapid-fire behavior, and vendor visual effects are not emulated.
