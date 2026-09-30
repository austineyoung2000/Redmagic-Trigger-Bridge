# Changelog

## 0.3.1-dev — 2026-09-29

### Added

- Optional shoulder-trigger haptic feedback through the confirmed NX809J
  `zte_vibrator` duration, gain, and activate nodes.
- Configurable haptic gain and duration with safe range clamping.

### Safety

- Emit feedback only on a new trigger-down edge, with a shared 90 ms limiter.
- Continue trigger and merged-touch processing if haptic nodes are missing or
  a vibrator write fails.
- Re-probe haptic availability on each bridge activation.

## 0.3.0 — 2026-09-29

Stable merged-touch release.

### Added

- A single uinput touchscreen that combines physical Synaptics contacts with
  two reserved shoulder-trigger contacts.
- Runtime cloning of physical touchscreen ranges and supported axes.
- Active-only creation and destruction of the merged virtual touchscreen.
- Per-contact coordinate validity tracking for safely reused physical slots.

### Fixed

- Preserve continuous thumbstick and multi-finger input while either or both
  shoulder triggers are pressed.
- Derive combined `BTN_TOUCH` and `BTN_TOOL_FINGER` state from all active
  physical and trigger contacts.
- Mirror the primary physical contact through legacy `ABS_X` and `ABS_Y` for
  games that require those axes.
- Match the physical touchscreen capability bitmap instead of advertising an
  unsupported multitouch-pressure axis.
- Reselect the physical protocol-B slot after trigger injection, preventing
  later physical movement from altering a trigger contact and snapping the
  in-game camera.
- Correct landscape target conversion for the orientation reported by the
  NX809J display stack.

### Verified

- Physical touchscreen input with the merged backend active and no triggers
  pressed.
- Continuous thumbstick movement with repeated left/right trigger taps and
  holds.
- Both triggers together with one and two physical screen contacts.
- Repeated physical contact removal and replacement during trigger activity.
- Accurate saved trigger targets in COD Mobile at rotation 1.
- Two-minute combined-input stress test without dropped touch, stuck contacts,
  or unexpected aim/camera movement.
- Clean activation on game entry and deactivation after returning to Termux.

### Known boundary

- The forced module backend is verified on stock NX809J firmware. Automatic
  selection on a custom ROM without native TGK remains unverified.
- Native TGK haptics, rapid fire, and vendor visual effects are not yet
  reproduced by the module.

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
