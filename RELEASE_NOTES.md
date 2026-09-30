# Redmagic Trigger Bridge 0.3.1

Version 0.3.1 adds validated shoulder-trigger haptics and robust automatic
rotation detection to the stable merged-touch backend for Redmagic 11 Toolbox.

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
- Optional trigger-down vibration using the confirmed NX809J `zte_vibrator`
  interface
- Low-latency press-edge pulses with no repeat or release vibration
- Automatic landscape detection when the vendor `SurfaceOrientation` field is
  unavailable

## Important fixes since 0.2.0

- Replaced the separate two-slot trigger touchscreen with a merged proxy.
- Mirrored legacy primary coordinates required by games such as COD Mobile.
- Matched the physical touchscreen's actual axis capabilities.
- Prevented stale coordinates when physical slots are reused.
- Preserved protocol-B slot selection after trigger injection, eliminating the
  intermittent camera/aim snap seen during simultaneous movement and firing.
- Corrected the NX809J landscape coordinate transform.
- Commit trigger contact frames before vibrator I/O so haptics never suppress
  mapped touch input.
- Fall back to Android window and display rotation state when the firmware does
  not expose `SurfaceOrientation` through `dumpsys input`.

## Validation

Validated on a REDMAGIC 11 Pro (`NX809J`) running stock Android 16 with
KernelSU 3.3.0. The final test included repeated taps, swipes, continuous
thumbstick movement, independent trigger taps and holds, both triggers, two
physical fingers plus both triggers, repeated finger replacement, and a
two-minute combined-input soak. Version 0.3.1 was additionally validated with
automatic rotation changing from unset to rotation 1, simultaneous haptic and
mapped trigger actions, held contacts, and uninterrupted physical touch. Trigger
targets remained correct and no unexpected aim snapping occurred.

## Installation

Install `Redmagic-Trigger-Bridge-v0.3.1.zip` through KernelSU, Magisk, or
APatch, then reboot once. Verify the ZIP with the published SHA-256 sidecar.
Leave the module enabled and allow Redmagic 11 Toolbox to activate it only when
the module backend is selected for a configured foreground game.

## Compatibility boundary

This release is restricted to the NX809J hardware interfaces. The forced
module-backend path is verified on stock firmware. End-to-end automatic
fallback on an actual custom ROM without native TGK remains unverified. Module
haptics approximate trigger feedback through the hardware vibrator but do not
reproduce ZTE's proprietary TGK waveform. Rapid-fire behavior and vendor visual
effects are not emulated.
