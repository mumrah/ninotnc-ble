// Store configs at 512k
#define PCONFIG_MEM_OFFSET (512 * 1024) 

#define PCONFIG_VER 1

const static uint8_t MAGIC[5] = {0x4B, 0x34, 0x44, 0x42, 0x5A};
const static uint8_t MAGIC_LEN = 5;

typedef struct {
    char magic[7];
    uint8_t version;
    char callsign[10];
    uint8_t ssid;
} PackedConfig;

void init_config(PackedConfig * config);

bool check_config(PackedConfig * config);

void print_config(PackedConfig * config);

void read_config_from_mem(PackedConfig * config);

void write_config_to_mem(PackedConfig * config);