# Simple Example for an SDIO-attached SD card

### Overview

This program is a simple demonstration of writing to an SD card using the [FatFs API](http://elm-chan.org/fsw/ff/00index_e.html).
It initializes the standard input/output and real-time clock, mounts an SD card, writes to a file, and then unmounts the SD card.

### Features

* Mounts an SD card
* Opens a file in append mode and writes a message
* Closes the file and unmounts the SD card
* Prints success and failure messages to the STDIO console

### Requirements
* You will need to
[customize](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico#customizing-for-the-hardware-configuration)
the hardware configuration in the `main.c` file to match your hardware.
* PICO_SDK_PATH must be set in the Cmake: Configure Environment
