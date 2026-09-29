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

## Stock TGK Binder reference

The stock firmware also exposes proprietary InputManager TGK transactions. They
remain useful to Redmagic 11 Toolbox on stock, but the bridge daemon deliberately
does not depend on them. Its path is evdev -> uinput, which can also operate on a
custom ROM retaining the vendor/kernel trigger devices.

That compatibility statement describes the required architecture. Version
0.2.0 has been validated on stock Android 16; end-to-end automatic fallback on
an actual custom ROM remains pending.

## Shutdown invariant

Every active virtual slot must receive `ABS_MT_TRACKING_ID=-1`, followed by the
appropriate `BTN_TOUCH=0` and `SYN_REPORT`, before the uinput device is destroyed.
This prevents a killed or upgraded daemon from leaving a stuck virtual finger.
