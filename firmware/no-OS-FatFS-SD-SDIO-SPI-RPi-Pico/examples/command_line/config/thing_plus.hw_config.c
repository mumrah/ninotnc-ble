/* hw_config.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use 
this file except in compliance with the License. You may obtain a copy of the 
License at

   http://www.apache.org/licenses/LICENSE-2.0 
Unless required by applicable law or agreed to in writing, software distributed 
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR 
CONDITIONS OF ANY KIND, either express or implied. See the License for the 
specific language governing permissions and limitations under the License.
*/
/*

This file should be tailored to match the hardware design.

See 
  https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration

*/

#include "hw_config.h"

/* Hardware Configuration of SPI "object" */
static spi_t spi = {
    .hw_inst = spi1,  // SPI component
    .miso_gpio = 12,  // GPIO number (not pin number)
    .mosi_gpio = 15,
    .sck_gpio = 14,
    .set_drive_strength = true,
    .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
    .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,

    .baud_rate = 25 * 1000 * 1000, // Actual frequency: 20833333. 
};

/* SPI Interface */
static sd_spi_if_t spi_if = {
    .spi = &spi,    // Pointer to the SPI driving this card
    .ss_gpio = 9,   // The SPI slave select GPIO for this SD card
    .set_drive_strength = true,
    .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA
};

/* Hardware Configuration of the SD Card "objects" */
static sd_card_t sd_card = {  
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,  // Pointer to the SPI interface driving this card
};

/* ********************************************************************** */

size_t sd_get_num() { return 1; }

sd_card_t *sd_get_by_num(size_t num) {
    if (num < sd_get_num()) {
        return &sd_card;
    } else {
        return NULL;
    }
}

/* [] END OF FILE */
