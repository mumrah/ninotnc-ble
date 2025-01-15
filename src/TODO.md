General
=======
Configure operation mode over serial
Configure operator callsign over serial
Store config in flash
Logging system? How to easily enable/disable verbose logging (at runtime?)


KISS
====
- [ ] Ack mode
- [ ] Reset after timeout
- [ ] Flow control. When using ack mode, need to not exceed some number of in-flight bytes


AX25
====
AX.25 digipeater
* Parse AX.25 packets
* Src/dest call, repeaters
* Beacon telemetry (number of packets, uptime, etc)

APRS
====
* Capture our position from TX'd APRS beacons
* Display distance and heading of incoming packets


NinoTNC
=======
Add mode for updating NinoTNC flash
Communicate with multiple TNCs over USB?


USB
===
https://hackaday.com/2022/12/28/usb-host-on-rp2040-with-pio/
https://github.com/sekigon-gonnoc/Pico-PIO-USB






Successfull BLE connection


ATT connected. address=20041d80 type=1 handle=0040
Unhandled event E7      HCI_EVENT_META_GAP
Unhandled event 3E      HCI_EVENT_LE_META
Unhandled event 3E      HCI_EVENT_LE_META
Unhandled event 3E      HCI_EVENT_LE_META
Unhandled event FF
Unhandled event 3E      HCI_EVENT_LE_META
ATT MTU exchange complete. handle=0040 MTU=255
Unhandled event 13
Unhandled event 13
Unhandled event 13
att_write_callback conn=0040 attr=0016
Unhandled event 13
Unhandled event 13
att_write_callback conn=0040 attr=0012


Rev 4
=====

* Flip Pico to bottom of board
* Use a surface mount right angle button
