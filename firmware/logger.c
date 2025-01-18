#include "sd_card.h"
#include "ff.h"
#include "logger.h"
#include <stdlib.h>

static FRESULT fr;
static FATFS fs;
static FIL fp;
static FIL pcap_fp;
static char buf[100];
static char latest_log_name[30];
static int32_t latest_log = -1;  // If equal to -1, no SD card detected
static uint32_t lines_written = 0;
static uint32_t bytes_written = 0;

static const char * LATEST_FILE = "/data/latest";

/**
 * Create a new log file
 */
static bool roll_log_file() {
    // Update "latest" file
    printf("Rolling log file\n");
    fr = f_open(&fp, LATEST_FILE, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        printf("Could not truncate %d\n", LATEST_FILE);
        return false;
    }
    latest_log += 1;
    f_printf(&fp, "%d\n", latest_log);
    f_close(&fp);

    // Create the new log file
    memset(latest_log_name, '\0', 30);
    sprintf(latest_log_name, "/data/log.%d.txt", latest_log);
    
    // Open the file and leave it open
    fr = f_open(&fp, latest_log_name, FA_WRITE | FA_CREATE_NEW);
    if (fr != FR_OK) {
        printf("Could not create new log file\n");
        return false;
    }

    // Open the PCAP file
    memset(latest_log_name, '\0', 30);
    sprintf(latest_log_name, "/data/log.%d.pcap", latest_log);
    f_open(&pcap_fp, latest_log_name, FA_WRITE | FA_CREATE_NEW);
    if (fr != FR_OK) {
        printf("Could not create new pcap file\n");
        return false;
    }

    printf("Finished rolling log files\n");

    // PCAP header
    uint32_t magic = 0xa1b2c3d4;
    int written;
    f_write(&pcap_fp, &magic, 4, &written);
    uint16_t major = 2;
    f_write(&pcap_fp, &major, 2, &written); // major
    major = 4;
    f_write(&pcap_fp, &major, 2, &written); // minor
    magic = 0;
    f_write(&pcap_fp, &magic, 4, &written); // thiszone
    f_write(&pcap_fp, &magic, 4, &written); // sigfigs
    magic = 65535;
    f_write(&pcap_fp, &magic, 4, &written); // snaplen
    magic = 3;
    f_write(&pcap_fp, &magic, 4, &written); // network (AX25)
    f_sync(&pcap_fp);

    return true;
}

bool logger_init(void) {
    // Initialize driver
    if (!sd_init_driver()) {
        printf("Could not initialize SD card\n");
        return false;
    }

    // Mount drive
    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("Could not mount SD card\n");
        return false;
    }

    // Initialize directory 
    fr = f_mkdir("/data");
    if (fr != FR_OK && fr != FR_EXIST) {
        printf("Could not create data directory\n");
        return false;
    }

    // Find latest file
    fr = f_open(&fp, LATEST_FILE, FA_READ);
    if (fr != FR_OK) {
        printf("initializing 'latest' data file\n");
    } else {
        // Latest file is present, bump it
        printf("reading latest data file\n");  
        memset(buf, '\0', 20);
        f_gets(buf, 20, &fp);
        f_close(&fp);
        latest_log = atoi(buf);
        printf("loaded latest of %d\n", latest_log);
    }

    return roll_log_file();   
}

void logger_write_pcap(uint32_t msec, uint32_t usec, char * key, char * buf, uint16_t len) {
    int written;
    f_write(&pcap_fp, &msec, 4, &written); // ts_sec
    f_write(&pcap_fp, &usec, 4, &written); // ts_usec
    usec = len;
    f_write(&pcap_fp, &usec, 4, &written); // incl_len
    f_write(&pcap_fp, &usec, 4, &written); // orig_len
    f_write(&pcap_fp, buf, len, &written); // packet
    f_sync(&pcap_fp);
}

/**
 * Write the printable characters from buffer into the SD log file. Non-printable
 * characters will be replaced with a dot ".".
 */
void logger_write_ascii(uint32_t millis, char * key, char * buf, uint16_t len) {
    int written;
    f_printf(&fp, "%lu\t%s\t", millis, key);
    for (uint16_t i=0; i<len; i++) {
        if (buf[i] < ' ' || buf[i] > '~') {
            f_putc('.', &fp);
        } else {
            f_putc(buf[i], &fp);
        }
        
    }
    f_putc('\n', &fp);
    f_sync(&fp);
    lines_written ++;
    bytes_written += written;
}

void logger_deinit(void) {
    printf("Closing SD card. Total lines: %d, Total bytes: %d", lines_written, bytes_written);
    f_close(&fp);
    f_unmount("");
}
