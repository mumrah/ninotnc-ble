# NinoTNC APRS Expansion Board

This project brings robust and bi-directional APRS support to the best AX.25
TNC on the market, the [NinoTNC](https://tarpn.net/t/nino-tnc/nino-tnc.html). 

The total cost for setting up an APRS mobile station using this project 
(excluding radio and antenna system) is around $80. 

* NinoTNC ($45 for DIY, more for pre-built)
* NinoBLE Expansion Board ($25 for pre-built)
* Custom TNC cable (free/cheap for DIY)
* APRS App (aprs.fi for iOS, APRSDroid for Android)

This project is experimental and should be considered as-is. The hardware and
firmware are open source licensed under the permissive MIT License.

# Features

* iOS and Android support
* Bi-directional APRS (receive and transmit)
* High-quality AFSK modulation, thanks to the NinoTNC
* USB powered using the USB-B connector on the NinoTNC
* Data logging to an on-board SD card
* Optional user-installed OLED display for packet information
* Pass-through mode for updating NinoTNC firmware

# Photos

![IMG_4732](https://github.com/user-attachments/assets/819ed26d-e3d5-476c-a6b7-d64fd73c6b3c)

# Use Cases

## Mobile APRS

The most obvious and indeed the primary use case of this project. When set up
as described here, this board enables a very robust mobile APRS solution. Since
the NinoTNC is compatible with a wide range of 2m mobile radios (including
surplus commercial radios), the cost of the system can be kept low. 

The use of Bluetooth also allows for a "wireless" installation. There are no 
cables connected to the phone besides a charger. This allows for an arbitrary 
separation between the mobile phone and the TNC. The wireless range of the 
NinoBLE far exceeds any distance possible within a vehicle. 

## APRS Data Logging

The SD card located on the NinoBLE will log all packets sent and received.
This is quite useful when driving since logging in the APRS apps will
eventually roll off.

Each time the NinoBLE starts up, a new log file is created. This makes it
easy to find the most recent data, but also to retain the old data.

On-board data logging can also be useful for repeater operators who
occasionally have errant APRS traffic on their VHF repeaters. Logging
everything into the SD card makes it trivial to see who was accidentally
sending APRS traffic over the repeater.

## Portable APRS

If used with an HT, the NinoBLE can be used for portable APRS tracking. 
Since the APRS mobile apps use the phone's GPS, it is quite simple
to take this system into the field.

The same benefits of the Mobile APRS use case apply here. We can use 
inexpensive radios thanks to the high-quality DSP performed by the NinoTNC.

If the NinoBLE is equipped with the optional OLED screen, the NinBLE can be
used as a simple APRS receiver without a mobile phone.

## How it works

The primary function that this board adds to the NinoTNC is Bluetooth 
connectivity. There are many use cases for this besides APRS, but for now APRS 
is the focus of this project.

The expansion board is called the NinoBLE and connects physically to the NinoTNC
through the J5 header located under the USB to UART bridge. The NinoBLE board
replaces the USB to UART bridge that is normally installed on the TNC (at U4).

As KISS packets are received by the expansion board over Bluetooth, they are
decoded and forwarded to the UART interface. Similarly, when KISS data is
received over the UART, it is decoded and forwarded to the Bluetooth interface.

An optional SD card may be installed to capture the packets as they are sent andreceived.

## Hacking

The physical layout of the board is the only thing specific to the NinoTNC 
about this project. The electrical interface between the TNC and the expansion
board is 57600 baud UART and the data protocol used is KISS. This means that 
this project can be used to add Bluetooth to any KISS TNC that uses a UART.

The heart of this project is a RaspberryPi Pico W. Many of the GPIOs are unused,
so there is plenty of opportunity for expansion and tinkering.

