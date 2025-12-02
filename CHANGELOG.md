# ADRD3161 Firmware Changelog

## 1.1.0

Features:
* Add OD entries mapping directly to TMC9660 parameters. Entries 0x2001 - 0x214E can be read/written to read TMC9660 parameters 0x1 - 0x14E.
* Add OD persistent storage, including for initializing TMC9660 RWE parameters at startup
* Add python canopen scripts for parameter provisioning and example use
* Default to NMT Pre-Operational to visually indicate whether the board was initialized by the host or not

Bugfixes:
* Fix CAN timing issues by adding MAX32662 IPO calibration routine

