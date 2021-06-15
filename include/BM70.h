#ifndef BM70_H
#define BM70_H

#include <stdint.h>
#include <HardwareSerial.h>

#define BM70_DEFAULT_TIMEOUT          200
#define BM70_STATUS_MAX_AGE_MS        10000

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
#define BM70_OP_RESET				  				0x02
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

// These UUIDs are defined https://github.com/hessu/aprs-specs/blob/master/BLE-KISS-API.md
const uint8_t KTS_SERVICE_UUID[16] = {0x00, 0x00, 0x00, 0x01, 0xba, 0x2a, 0x46, 0xc9, 0xae, 0x49, 0x01, 0xb0, 0x96, 0x1f, 0x68, 0xbb};
const uint8_t KTS_RX_CHAR_UUID[16] = {0x00, 0x00, 0x00, 0x03, 0xba, 0x2a, 0x46, 0xc9, 0xae, 0x49, 0x01, 0xb0, 0x96, 0x1f, 0x68, 0xbb};
const uint8_t KTS_TX_CHAR_UUID[16] = {0x00, 0x00, 0x00, 0x02, 0xba, 0x2a, 0x46, 0xc9, 0xae, 0x49, 0x01, 0xb0, 0x96, 0x1f, 0x68, 0xbb};


struct Result {
  uint8_t opCode;
  uint8_t buffer[300];
  uint16_t len;
  uint8_t checksum;

	uint8_t params(uint8_t i)
	{
			// Example 6 byte payload: AA 0 2 81 3 7A, only 0 is a valid index
			if (i > len - 6) 
					return -1;
			return buffer[i + 4];
	}
};

typedef void (*EventCallback)(struct Result *result);

typedef void (*RxCallback)(uint8_t * buffer, uint8_t len);


class BM70
{
public:
	BM70();
	BM70(HardwareSerial * initSerial, uint32_t baudrate, RxCallback callback);

	// Low level functions
	uint8_t write0 (uint8_t opCode);
	uint8_t write1 (uint8_t opCode, uint8_t param);
	uint8_t write (uint8_t opCode, uint8_t * params, uint8_t len);
  uint8_t write (uint8_t opCode, uint8_t * params, uint8_t len, uint16_t timeout);
	uint8_t read ();
	uint8_t read (uint16_t timeout);
	uint8_t readCommandResponse(uint8_t opCode);

	void reset();
	bool updateStatus();
	void enableAdvertise();

	void discoverCharacteristics(const uint8_t * serviceUUID);
	void readCharacteristicValue(const uint8_t * serviceUUID);

	void send(const uint8_t * data, uint8_t len);

	uint8_t status();
	uint8_t connection();

private:
	HardwareSerial * serial;
	RxCallback rxCallback;

	uint8_t currentStatus;

	Result result;
	uint8_t readPos;

	uint64_t connectionAddress;
	uint8_t connectionHandle;
	uint8_t lastCommandSent;

	uint8_t rxCharHandle;
	uint8_t txCharHandle;

	unsigned long lastStatusUpdateMs;
	unsigned long lastReadMs;

	void handleResult(Result *result);
};

#endif // ifndef BM70_H