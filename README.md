# NinoTNC BLE daughter board

This board connects to the NinoTNC N9600A4 in place of the USB bridge IC. It provides a BLE
interface which allows communication between the NinoTNC and an iOS device. Using the aprs.fi
app, the NinoTNC can be used as part of a mobile or portable APRS setup.

The board uses an ATMega328pb. This IC has two hardware uarts which were needed to efficiently
handle the communication between the NinoTNC and the BM70 module. 

Only a bare minimum of the BLE spec is implemented. A hard-coded serivce UUID is also used 
which is required for communication with aprs.fi. 

Fuses are: E:F5, H:D6, and L:FF

PCB is shared on OSHPark https://oshpark.com/shared_projects/s0RX6naD

This project is experimental and should be considered as-is. The software and board design are
licensed under the permissive Creative Commons Share Alike.
