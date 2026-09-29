# Changelog

## 0.2.0 — 2026-09-29

First public release candidate.

### Added

- Arm64 Android daemon for `nubia_tgk_aw_sar0_ch0` (`KEY_F7`) and
  `nubia_tgk_aw_sar1_ch0` (`KEY_F8`).
- Two-slot `/dev/uinput` touchscreen supporting independent, held, and
  simultaneous trigger contacts.
- Dynamic evdev discovery by kernel input name.
- Runtime touchscreen-axis discovery and normalized targets for all four
  display rotations.
- Direct SAR trigger arming with restoration of the modes captured at
  activation.
- Explicit `on`, `off`, `reload`, and `status` control contract for Redmagic 11
  Toolbox.
- Inactive-by-default boot behavior and forced virtual-contact release during
  deactivation, shutdown, reload, and input failure.
- Optional exclusive physical-device grabs to prevent duplicate handling.
- Bounded daemon restart backoff, log rotation, root-only state, and
  KernelSU/Magisk/APatch module packaging.

### Verified

- NX809J stock REDMAGIC Android 16 with KernelSU 3.3.0.
- Left, right, held, and simultaneous contacts.
- No virtual contacts while inactive.
- Forced release when deactivated while a trigger remains held.
- Coexistence while installed and enabled as Toolbox selects native TGK.

### Known boundary

- Automatic fallback is implemented in Redmagic 11 Toolbox but has not yet
  been validated end to end on a custom ROM without native TGK.
- Native TGK-only haptics, rapid-fire behavior, and system visual effects are
  not emulated.
