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

# Photos

![IMG_2488 2](https://user-images.githubusercontent.com/55116/218492105-87959d4b-2ce6-4e80-bd81-f676048063d9.jpg)

![IMG_2489](https://user-images.githubusercontent.com/55116/218492130-1c20ec6f-75b8-4714-a3e1-dc45aa7b9023.jpg)

![IMG_5311](https://user-images.githubusercontent.com/55116/218492169-c724f344-5984-4dfa-bfe3-4c4532da6e52.jpg)

![IMG_0753](https://user-images.githubusercontent.com/55116/218492350-b8926ebc-5c25-4017-8e1e-997665a8e2ad.JPG)
