#!/system/bin/sh

ui_print "- Checking REDMAGIC device compatibility"

device="$(getprop ro.product.device)"
model="$(getprop ro.product.model)"
arch="$(getprop ro.product.cpu.abi)"

if [ "$device" != "NX809J" ] && [ "$model" != "NX809J" ]; then
    abort "! Unsupported device: model=$model device=$device"
fi

if [ "$arch" != "arm64-v8a" ]; then
    abort "! Unsupported architecture: $arch"
fi

if [ ! -e /dev/uinput ]; then
    abort "! /dev/uinput is unavailable"
fi

set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/bin/redmagic-trigger-bridge" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755

ui_print "- NX809J trigger bridge installed"
