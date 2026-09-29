# NX809J trigger hardware notes

These interfaces were captured on stock REDMAGIC Android 16 firmware with an
NX809J and KernelSU 3.3.0.

## Native inputs

| Sysfs input name | Event | Observed key |
|---|---|---|
| `nubia_tgk_aw_sar0_ch0` | dynamically discovered | `KEY_F7` |
| `nubia_tgk_aw_sar1_ch0` | dynamically discovered | `KEY_F8` |

Each press also reports `ABS_DISTANCE=1`; release reports `ABS_DISTANCE=0`.
The daemon intentionally keys on F7/F8 and ignores the distance event.

Both inputs support independent and simultaneous down states. Event-node numbers
are not ABI and must never be hard-coded.

## Hardware arming

The capacitive inputs are controlled through:

```text
/sys/class/leds/sar0/mode_operation
/sys/class/leds/sar1/mode_operation
```

Writing `1` arms the sensor. The stock Toolbox process is not required after the
hardware is armed. The root-module daemon performs this write itself.

## Touch injection

The stock touchscreen identifies as `synaptics_tcm_touch` and reports multitouch
axes. One observed unit exposed raw maxima of X=12159 and Y=26879. The daemon
queries `EVIOCGABS` at runtime rather than embedding those measurements.

`/dev/uinput` and `CONFIG_INPUT_UINPUT=y` were confirmed on the stock kernel.

### Merged protocol-B proxy

Games may not combine contacts from two independent touchscreen devices. While
the module backend is active, the daemon therefore grabs `synaptics_tcm_touch`,
clones its relevant axes into one uinput device, forwards its physical slots,
and reserves two additional slots for the shoulder triggers. When inactive, it
destroys the virtual device and releases the physical touchscreen unchanged.

Each forwarded slot-scoped event explicitly restores the corresponding virtual
`ABS_MT_SLOT`. This is required because an injected trigger event changes the
virtual device's selected slot while the physical protocol-B stream may continue
an existing contact without repeating its slot number. Preserving that state
prevents physical movement from corrupting a trigger contact and causing an
unexpected in-game camera jump.

Legacy `ABS_X` and `ABS_Y` follow the primary physical contact. A newly reused
physical slot is not eligible as primary until both of its current X/Y values
have arrived, preventing coordinates from a previous contact from being
published for a frame.

## Stock TGK Binder reference

The stock firmware also exposes proprietary InputManager TGK transactions. They
remain useful to Redmagic 11 Toolbox on stock, but the bridge daemon deliberately
does not depend on them. Its path is evdev -> uinput, which can also operate on a
custom ROM retaining the vendor/kernel trigger devices.

That compatibility statement describes the required architecture. Version
0.3.0 and its forced module-backend path have been validated on stock Android
16; end-to-end automatic fallback on an actual custom ROM remains pending.

## Shutdown invariant

Every active virtual slot must receive `ABS_MT_TRACKING_ID=-1`, followed by the
appropriate `BTN_TOUCH=0` and `SYN_REPORT`, before the uinput device is destroyed.
This prevents a killed or upgraded daemon from leaving a stuck virtual finger.
