#include "BM70.h"
#include "timer1.h"
#include "config.h"

BM70::BM70()
{ }

BM70::BM70(HardwareSerial * initSerial, uint32_t baudrate, RxCallback callback)
{
	serial = initSerial;
    rxCallback = callback;
	serial->begin (baudrate);
	currentStatus = BM70_STATUS_UNKNOWN;
	connectionHandle = 0x00;
    lastStatusUpdateMs = 0L;
    lastReadMs = 0L;
    readPos = 0;
}

/**
 * Read one byte off the BM70 UART buffer, if available.
 * 
 * If this data completes a BM70 payload, validate it and and pass to the handler.
 * 
 * @return 0 if data was read but no payloads were decoded
 *         1 if data was read and a payload was decoded
 *         -1 if no data was read
 *         -2 if there was a read sync error
 */
uint8_t BM70::read()
{
    uint8_t b = serial->read();
    if (b == 0xFF)
        return -1;
    
    unsigned long now = millis1();

    if (readPos != 0 && (now - lastReadMs) > BM70_DEFAULT_TIMEOUT)
    {
        DebugSerial.println("Timed out reading BLE payload, resetting buffer");
        result.len = -1;
        result.opCode = -1;
        readPos = 0;
    }
    lastReadMs = now;

    if (readPos == 0) 
    {
        if (b == 0xAA) {
            // Normal case
            result.buffer[readPos++] = 0xAA;
            result.opCode = -1;
            result.len = -1;
        } else {
            DebugSerial.println("Continue reading until sync");
        }
    }
    else 
    {
        if (b == 0xAA) 
        {
            // We over-read and hit the next payload
            DebugSerial.println("Out of sync");
            readPos = 0;
            result.buffer[readPos++] = 0xAA;
            result.opCode = -1;
            result.len = -1;
            return -2;
        } 
        else {
            result.buffer[readPos++] = b;
            if (readPos == 3) {
                // Read h + l
                result.len = (((short) result.buffer[1]) << 8 ) + result.buffer[2] + 4;
            } else if (readPos == 4) {
                // Read op code
                result.opCode = b;
            } else if (readPos == result.len) {
                // Read the whole payload
                DebugSerial.println("Read full payload");
                handleResult(&result);
                readPos = 0;
                result.opCode = -1;
                result.len = -1;
                return 1;
            }
        }
    }

    return 0;
}

void BM70::handleResult(Result *result)
{
    DebugSerial.print("Validating: ");
    for (uint16_t i = 0; i<readPos; i++) 
    {
      DebugSerial.print(result->buffer[i], HEX); DebugSerial.print(" ");
    }
    DebugSerial.println();

    if (result->len != readPos) 
    {
        DebugSerial.println("Bad length");
        return;
    } 

    uint8_t checksum = 0;
	for (uint16_t i = 2; i <= result->len - 1; i++)
		checksum += result->buffer[i];

	if (checksum != 0)
    {
        DebugSerial.println("Bad checksum");
        return;
    } 

    switch (result->opCode)
    {
        case 0x70:
            // Advertising report event, someone noticed us
            break;
        case 0x71:
            // LE Connection Complete Event
            if (result->params(0) == 0x00)
            {
                // success
                DebugSerial.println("BLE Connected");
                connectionHandle = result->params(1);
                uint64_t address = 0;
				for (uint8_t i = 0; i < 6; i++)
					address += (((uint64_t) result->params(i + 4)) << (8 * i));
                connectionAddress = address;
                currentStatus = BM70_STATUS_CONNECTED;
            }
            else
            {
                // failed, ignore
                DebugSerial.print("BLE Connection Failed, param: "); DebugSerial.println(result->params(0), HEX);
                currentStatus = BM70_STATUS_UNKNOWN;
                delay1(50);
            }
            break;
        case 0x72:
            // LE Disconnect Event
            DebugSerial.println("BLE Disconnected");
            connectionHandle = 0x00;
            connectionAddress = 0x00;
            currentStatus = BM70_STATUS_UNKNOWN;
            break;
        case 0x80:
        {
            // Command Complete Event, ignore for now
            uint8_t commandOp = result->params(0);
            uint8_t commandResult = result->params(1);
            DebugSerial.print("Command Complete: "); 
            DebugSerial.print("command: "); DebugSerial.print(commandOp, HEX);
            DebugSerial.print(" result: "); DebugSerial.println(commandResult, HEX);
            break;
        }   
        case 0x81:
            // Status Report Event
            DebugSerial.print("BLE Status Report: "); DebugSerial.println(result->params(0));
            lastStatusUpdateMs = millis1();
            if (currentStatus != result->params(0))
            {
                currentStatus = result->params(0);
            }  
            break;
        case 0x98:
        {
            // Client Write Characteristic
            // uint8_t handle = result.params[0];
            // TODO check connection handle and characteristic handle
            uint16_t char_handle = 0;
            char_handle += (result->params(1) << 8) + result->params(2);
            for (uint16_t i = 0; i < result->len - 6; i++)
                rxBuffer[i] = result->params(i + 3);
            rxCallback(rxBuffer, result->len - 6);
            break;
        }
        default:
            // Unsupported, ignore
            DebugSerial.print("Ignoring unsupported opCode "); DebugSerial.println(result->opCode);
            break;
    }   
}

uint8_t BM70::read(Result *result) 
{ 
    return read(result, BM70_DEFAULT_TIMEOUT);    
}

uint8_t BM70::read(Result *result, uint16_t timeout)
{
    uint64_t t = millis1();
    while (serial->available() == 0)
    {
        if ((millis1() - t) > timeout)
            return -1;
        delay1(2);
    }  

    DebugSerial.print("BM70::read() -> ");

    uint8_t checksum = 0;
    uint8_t b = serial->read();

    // read until we find sync word 0xAA
    while (b != 0xAA) 
    {
        DebugSerial.print("skipping -> "); DebugSerial.println(b, HEX);
        b = serial->read();
        if (b == 0xFF)
        {
            DebugSerial.println("BM70::read() no data!");
            return -1;
        }
    }

    uint8_t h = serial->read();
    DebugSerial.print("h: "); DebugSerial.print(h);
    uint8_t l = serial->read();
    DebugSerial.print(", l: "); DebugSerial.print(l);
    result->opCode = serial->read();
    DebugSerial.print(", op: "); DebugSerial.print(result->opCode, HEX);
    result->len = (((short) h) << 8 ) + l - 1;
    DebugSerial.print(", len: "); DebugSerial.print(result->len);
    uint16_t i = 0;
    DebugSerial.print(", params: ");
    for (i = 0; serial->available() && i < result->len; i++) {
        result->buffer[i] = serial->read();
        DebugSerial.print(result->buffer[i], HEX); DebugSerial.print(" ");
        delay1(2);
    }
    uint8_t n = i;
    DebugSerial.print(", n: "); DebugSerial.print(n);
    
    if (n < result->len) {
        // If we read a large payload, truncate it and skip checksum (TODO improve this)
        DebugSerial.print("\nread underflow!! Expected "); DebugSerial.print(result->len);
        DebugSerial.print(" but only read "); DebugSerial.println(n);
        result->len = n;
        //return -2;
        return 0;
    }

    result->checksum = serial->read();
    DebugSerial.print(", checksum: "); DebugSerial.println(result->checksum);

    checksum += (h + l + result->opCode);
    for (uint8_t i=0; i<n; i++) {
        checksum += result->buffer[i];
    }
    checksum += result->checksum;

    if (checksum != 0) {
        DebugSerial.print("checksum error, saw "); DebugSerial.println(checksum, HEX);
        return -3;
    }
    return 0;
}

uint8_t BM70::readCommandResponse(uint8_t opCode) {
    unsigned long start = millis1();
    while ((millis1() - start) < BM70_DEFAULT_TIMEOUT) {
        if (read() != -1) {
            // Read something
            if (result.opCode == 0x80) {
                uint8_t commandCode = result.buffer[0];
                uint8_t commandResult = result.buffer[1];
                if (commandCode == opCode) {
                    return commandResult;
                }
            }
        }   
    }  
    return -1; 
}

uint8_t BM70::write0 (uint8_t opCode)
{
    return write(opCode, NULL, 0);
}

uint8_t BM70::write1 (uint8_t opCode, uint8_t param)
{
    uint8_t params[1];
    params[0] = param;
    return write(opCode, params, 1);
}

uint8_t BM70::write (uint8_t opCode, uint8_t * params, uint8_t len)
{
    return write(opCode, params, len, BM70_DEFAULT_TIMEOUT);
}

uint8_t BM70::write (uint8_t opCode, uint8_t * params, uint8_t len, uint16_t timeout)
{
    uint8_t h  = (uint8_t) ((1 + len) >> 8);
	uint8_t l  = (uint8_t) (1 + len);
	uint8_t checksum = 0 - h - l - opCode;
    
    uint8_t buffer[len + 5];
    buffer[0] = 0xAA;
    buffer[1] = h;
    buffer[2] = l;
    buffer[3] = opCode;

	for (uint16_t i = 0; i < len; i++)
    {
        checksum -= params[i];
		buffer[i + 4] = params[i];
    }
	buffer[len + 4] = checksum;
    
    uint64_t start = millis1();
    while (serial -> availableForWrite() < len + 5)
    {
        if ((millis1() - start) > timeout) 
            return -1;
        delay1(1);
    }
    DebugSerial.print("BM70::write() ->");
    DebugSerial.print(" opCode: "); DebugSerial.print(opCode, HEX);
    DebugSerial.print(" params: "); printBytes(params, len); DebugSerial.println();
    serial -> write(buffer, len + 5); 
    serial -> flush();
    return len + 5;
}

// Commands and such

void BM70::reset()
{
    write0(BM70_OP_RESET);
    currentStatus = BM70_STATUS_UNKNOWN;
	connectionHandle = 0x00;
    lastStatusUpdateMs = 0L;
}

bool BM70::updateStatus()
{
    if (currentStatus == BM70_STATUS_CONNECTED && connectionHandle == 0x00) {
        DebugSerial.println("In connected state but no connection handle. Resetting");
        reset();
        return true;
    }

    unsigned long now = millis1();
    if (currentStatus == BM70_STATUS_UNKNOWN || (now - lastStatusUpdateMs) > BM70_STATUS_MAX_AGE_MS) {
        DebugSerial.println("Reading BLE status");
        write0(BM70_OP_READ_STATUS);
        lastStatusUpdateMs = now;
        return true;
    }

    return false;
}

uint8_t BM70::status() {
    return currentStatus;
}

void BM70::enableAdvertise()
{
    write1(BM70_OP_ADVERTISE_ENABLE, 0x01); 
    delay1(50);
    if (readCommandResponse(BM70_OP_ADVERTISE_ENABLE) == 0) {
        currentStatus = BM70_STATUS_STANDBY;
    }
}

uint8_t BM70::connection()
{
    return connectionHandle;
}


// TODO probably don't need this
void BM70::discoverCharacteristics(const uint8_t * serviceUUID)
{
    uint8_t params[17]; // connection handle byte + 128 bit uuid
    params[0] = connectionHandle;
    memcpy(params, serviceUUID, 16);
    write(0X31, params, 17);
    //readCommandResponse();
}

void BM70::readCharacteristicValue(const uint8_t * serviceUUID) {
    if (connectionHandle != 0x00) {
        uint8_t params[17]; // connection handle byte + 128 bit uuid
        params[0] = connectionHandle;
        memcpy(params+1, serviceUUID, 16);
        write(BM70_OP_READ_CHAR_UUID, params, 17);  
    }
}

void BM70::send(const uint8_t * data, uint8_t len)
{
    if (currentStatus != BM70_STATUS_CONNECTED)
        return;

    uint8_t params[23];
    params[0] = connectionHandle;
    params[1] = 0x80; // char handle hi
    params[2] = 0x04; // char handle lo

    uint8_t offset = 3;
    for (uint8_t i=0; i<len; i++)
    {
        params[offset++] = data[i];
        if (offset == 23)
        {
            write(BM70_OP_SEND_CHAR, params, offset);
            delay1(50);
            readCommandResponse(BM70_OP_SEND_CHAR);
            delay1(50);
            offset = 3;
        }
    }
    
    if (offset > 3)
    {
        write(BM70_OP_SEND_CHAR, params, offset);
        delay1(50);
        readCommandResponse(BM70_OP_SEND_CHAR);
        delay1(50);
    }
}
