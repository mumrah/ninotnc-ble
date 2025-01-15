#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h" // for the flash erasing and writing
#include "hardware/sync.h" // for the interrupts

#include "pconfig.h"

/**
 * Bootstrap some defaults into the config struct
 */
void init_config(PackedConfig * config) {
    // v1: magic, version, callsign, ssid
    memcpy(config->magic, MAGIC, MAGIC_LEN);
    config->version = PCONFIG_VER;
    memcpy(config->callsign, "N0CALL\0", 7);
    config->ssid = 0;
}

/** 
 * Check if the config struct is valid
 */
bool check_config(PackedConfig * config) {
    return memcmp(config->magic, MAGIC, MAGIC_LEN) == 0 && config->version == 1;
}

void print_config(PackedConfig * config) {
    if (!check_config(config)) {
        printf("config struct is invalid\n");
    } else {
        printf("config:\n");
        printf("version=%d\n", config->version);
        printf("callsign=%s\n", config->callsign);
        printf("ssid=%d\n", config->ssid);
    }
}

void read_config_from_mem(PackedConfig * config) {
    const uint8_t * pconfig_data_ptr = (const uint8_t *) (XIP_BASE + PCONFIG_MEM_OFFSET);
    memcpy(config, pconfig_data_ptr, sizeof(config));
}

void write_config_to_mem(PackedConfig * config) {
    int size = sizeof(config);
    int page_count = (size / FLASH_PAGE_SIZE) + 1; // how many flash pages we're gonna need to write
    int sector_count = ((page_count * FLASH_PAGE_SIZE) / FLASH_SECTOR_SIZE) + 1; // how many flash sectors we're gonna need to erase
        
    printf("Programming flash target region...\n");

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(PCONFIG_MEM_OFFSET, FLASH_SECTOR_SIZE * sector_count);
    flash_range_program(PCONFIG_MEM_OFFSET, (uint8_t *) config, FLASH_PAGE_SIZE * page_count);
    restore_interrupts(interrupts);

    printf("Done.\n");
}