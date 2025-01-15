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
* Bi-directional APRS (received and transmit)
* High quality AX.25 modulation, thanks to the NinoTNC
* USB powered using the USB-B connector on the NinoTNC
* Data logging to an on-board SD card
* Optional user-installed OLED display for packet information
* Pass-through mode for updating NinoTNC firmware

# Photos

![IMG_4732](https://github.com/user-attachments/assets/819ed26d-e3d5-476c-a6b7-d64fd73c6b3c)


# How to get it



# Use Cases

## Mobile APRS

The most obvious and indeed the primary use case of this project. When setup
as described here, this board enables a very robust mobile APRS solution. Since
the NinoTNC is compatible with a wide range of 2m mobile radios (including
surplus commercial radios), the cost of the system can be kept low. 

The use of Bluetooth also allows for a "wireless" installation. There are no 
cables connected to the phone besides a charger. This allows for an arbitrary 
separation between the mobile phone and the TNC. The wireless range of the 
NinoBLE far exceeds any distance possible within a vehicle. 

## APRS Data Logging

The SD card located on the NinoBLE will log all packets sent and received.
This is quite useful when driving since the logging in the APRS apps will
eventually roll off.

Each time the NinoBLE starts up, it creates a new log file. This makes it
easy to find the most recent data, but also to retain the old data.

On-board data logging can also be useful for repeater operators who may
occasionally have errant APRS traffic on their VHF repeaters. By logging
everything to the SD card, it becomes trivial to see who was accidentally
sending APRS traffic over the repeater.

## Portable APRS

If used in conjunction with an HT, the NinoBLE can be used for portable APRS
tracking. Since the APRS mobile apps use the phone's GPS, it is quite simple
to take this system into the field.

The same benefits of the Mobile APRS use case apply here. We can use inexpensive
radios thanks to the high quality DPS done in the NinoTNC.

If the NinoBLE is equipped with the optional OLED screen, the NinBLE can be
used without a phone as a simple APRS receiver. 

## How it works

The primary function that this board provides is a adding Bluetooth connectivity
to the NinoTNC. There are many use cases for this besides APRS, but for now APRS
is the focus of the project.

The expansion board, called the NinoBLE, connects physically to the NinoTNC through
the J5 header located under the USB to UART bridge. In fact, this board replaces
the USB to UART bridge that is normally installed on the TNC (at U4).

As KISS packets are received by the expansion board over bluetooth, they are decoded
and forwarded to the UART interface. Similarly, when KISS data is receieved over
the UART, it is decoded and forwarded to the Bluetooth interface.

An optional SD card may be intalled to capture the packets as they are sent and 
received.

## Hacking

The physical layout of the board is the only NinoTNC specific feature of this project.
The electrical interface between the TNC and the expansion board is UART and the 
data protocol used is KISS. This means that this project can be used to add Bluetooth
to any KISS TNC that uses a UART.

The heart of this project is a RaspberryPi Pico W. Many of the GPIOs are unused, so there
is plenty of opprotunity for expansion and tinkering.

# Hardware

## Installation

Optional NinoTNC part

https://www.digikey.com/en/products/detail/mill-max-manufacturing-corp/319-10-104-00-006000/5176105

Increases board spacing from 4mm to 8mm and gives a nice seat for the pogo pins.

## Operation

 

# Software

## Apple iOS

## Android

## Pass-Through Mode

In order to facilitate firmware updates the the N9600A4, this board includes a 
UART pass-through mode. If selected, this mode will transparently pass data between
a host PC and a connected N9600A4 via the micro USB connector on the Pico.

## Serial debugging

A number of useful log messages are produced to the serial interface on the micro 
USB connector on the Pico. If connected to a host PC, a serial monitor can be used 
to read these messages. Something like the Arduino IDE, PlatformIO, or Visual Studio Code
can be used.

