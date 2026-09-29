# Redmagic Trigger Bridge

Root companion module for the REDMAGIC 11 Pro (NX809J) that converts its capacitive shoulder-trigger events into independent multitouch contacts.

Designed as a stock/custom-ROM trigger backend for Redmagic 11 Toolbox. Development is based on hardware observations from stock Android 16 and public NX809J device-tree research.

Initial goals:

- dynamically discover the SAR trigger and touchscreen event nodes;
- arm both shoulder sensors without relying on Game Space;
- inject safe, simultaneous two-slot multitouch through uinput;
- support normalized targets for all four rotations;
- package for KernelSU, Magisk, and APatch.

Status: early development. Do not flash until the first tested release is published.
