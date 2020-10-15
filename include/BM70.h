#ifndef BM70_H
#define BM70_H

#include <Arduino.h>

#define BM70_DEFAULT_TIMEOUT          50
#define BM70_RESPONSE_BUFF_SIZE       1  // 3
#define BM70_TRANSPARENT_BUFF_SIZE    1  // 10
#define BM70_RESPONSE_MAX_SIZE        30 // 50
#define BM70_TRANSPARENT_MAX_SIZE     20 // 50
#define BM70_BUFFER_EMPTY_DELAY       1000

#define BM70_STATUS_UNKNOWN           0x00 // Status is unknown
#define BM70_STATUS_SCANNING          0x01 // Scanning mode
#define BM70_STATUS_CONNECTING        0x02 // Connecting mode
#define BM70_STATUS_STANDBY           0x03 // Standby mode
#define BM70_STATUS_BROADCAST         0x05 // Broadcast mode
#define BM70_STATUS_TRANSCOM          0x08 // Transparent Service enabled mode
#define BM70_STATUS_IDLE              0x09 // Idle mode
#define BM70_STATUS_SHUTDOWN          0x0A // Shutdown mode
#define BM70_STATUS_CONFIG            0x0B // Configure mode
#define BM70_STATUS_CONNECTED         0x0C // BLE Connected mode

#define BM70_TYPE_RESPONSE            0x80 // Command complete event
#define BM70_TYPE_STATUS              0x81 // Status report event
#define BM70_TYPE_ADVERT_REPORT       0x70 // Advertising report event
#define BM70_TYPE_CONNECTION_COMPLETE 0x71 // LE connection complete event
#define BM70_TYPE_PAIR_COMPLETE       0x61 // Pair complete event
#define BM70_TYPE_PASSKEY_REQUEST     0x60 // Passkey entry request event
#define BM70_TYPE_TRANSPARENT_IN      0x9A // Received transparent data event

#define BM70_OP_READ_LOCAL_INFO       0x01
#define BM70_OP_RESET				  0x02
#define BM70_OP_READ_STATUS           0x03

#define BM70_OP_DISCONNECT            0x1B
#define BM70_OP_ADVERTISE_ENABLE      0x1C

#define BM70_OP_READ_CHAR             0x32
#define BM70_OP_READ_CHAR_UUID        0x33
#define BM70_OP_WRITE_CHAR            0x34
#define BM70_OP_SEND_CHAR             0x38 // Use this to send to TNC
#define BM70_OP_UPDATE_CHAR           0x39

#define BM70_EVENT_DISCONNECT         0x72

#define BM70_EVENT_CMD_COMPLETE       0x80
#define BM70_EVENT_STATUS_REPORT      0x81
#define BM70_EVENT_CMD_COMPLETE       0x80
#define BM70_EVENT_CLIENT_CHAR_WRITE  0x98

struct Result {
  size_t opCode;
  uint8_t params[100];
  uint8_t len;
  uint8_t checksum;
};

typedef void (*EventCallback)(struct Result *result);

typedef void (*RxCallback)(uint8_t * buffer, uint8_t len);


class BM70
{
public:
	BM70();
	BM70(HardwareSerial * initSerial, uint32_t baudrate, RxCallback callback);

	// Low level functions
	void write0 (uint8_t opCode);
    void write1 (uint8_t opCode, uint8_t param);
	void write (uint8_t opCode, uint8_t * params, uint8_t len);
    uint8_t write (uint8_t opCode, uint8_t * params, uint8_t len, uint16_t timeout);
	uint8_t read ();
	uint8_t read (Result *result);
	uint8_t read (Result *result, uint16_t timeout);

	void reset();
	void updateStatus();
	void enableAdvertise();
	void discoverCharacteristics(const uint8_t * serviceUUID);

	void send(const uint8_t * data, uint8_t len);

	uint8_t status();
	uint8_t connection();

private:
	HardwareSerial * serial;
	RxCallback rxCallback;

	uint8_t rxBuffer[200];
	uint8_t currentStatus;
	Result lastResult;

	uint64_t connectionAddress;
	uint8_t connectionHandle;

	uint8_t rxCharHandle;
	uint8_t txCharHandle;
};

#endif // ifndef BM70_H