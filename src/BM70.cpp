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
}

/**
 * Read some data off the BM70 UART buffer.
 * 
 * Some events will be automatically handled to update our internal state,
 * others will be passed on to the callback.
 */
uint8_t BM70::read()
{  
    uint8_t readResult = read(&lastResult);
    if (readResult != 0) {
        return readResult;
    }

    if (lastResult.opCode == 0) 
        return -1;

    switch (lastResult.opCode)
    {
        case 0x70:
            // Advertising report event, someone noticed us
            break;
        case 0x71:
            // LE Connection Complete Event
            if (lastResult.params[0] == 0x00)
            {
                // success
                DebugSerial.println("BLE Connected");
                connectionHandle = lastResult.params[1];
                uint64_t address = 0;
				for (uint8_t i = 0; i < 6; i++)
					address += (((uint64_t) lastResult.params[i + 4]) << (8 * i));
                connectionAddress = address;
                currentStatus = BM70_STATUS_CONNECTED;
            }
            else
            {
                // failed, ignore
                DebugSerial.print("BLE Connection Failed, param: "); DebugSerial.println(lastResult.params[0], HEX);
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
            uint8_t command = lastResult.params[0];
            uint8_t result = lastResult.params[1];
            DebugSerial.print("Command Complete: "); 
            DebugSerial.print("command: "); DebugSerial.print(command, HEX);
            DebugSerial.print(" result: "); DebugSerial.println(result, HEX);
            break;
        }   
        case 0x81:
            // Status Report Event
            DebugSerial.print("BLE Status Report: "); DebugSerial.println(lastResult.params[0]);
            lastStatusUpdateMs = millis1();
            if (currentStatus != lastResult.params[0])
            {
                currentStatus = lastResult.params[0];
            }  
            break;
        case 0x98:
        {
            // Client Write Characteristic
            // uint8_t handle = lastResult.params[0];
            // TODO check connection handle and characteristic handle
            uint16_t char_handle = 0;
            char_handle += (lastResult.params[1] << 8) + lastResult.params[2];
            for (uint8_t i = 0; i < lastResult.len - 3; i++)
                rxBuffer[i] = lastResult.params[i + 3];
            rxCallback(rxBuffer, lastResult.len - 3);
            break;
        }
        default:
            // Unsupported, ignore
            break;
    }
    return 0;
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
    }  

    DebugSerial.print("BM70::read() -> ");

    uint8_t checksum = 0;
    uint8_t b = serial->read();

    // read until we find sync word 0xAA
    while (b != 0xAA) 
    {
        if ((millis1() - t) > timeout) 
        {
            DebugSerial.println("BM70::read() timeout!");
            return -1;
        }
        DebugSerial.print("skipping -> "); DebugSerial.println(b, HEX);
        b = serial->read();
        if (b == 0xFF)
        {
            DebugSerial.println("BM70::read() no data!");
            return -1;
        }
    }

    uint8_t h = serial->read();
    DebugSerial.print("h: "); DebugSerial.print(h, HEX);
    uint8_t l = serial->read();
    DebugSerial.print(", l: "); DebugSerial.print(l, HEX);
    result->opCode = serial->read();
    DebugSerial.print(", op: "); DebugSerial.print(result->opCode, HEX);
    result->len = (((short) h) << 8 ) + l - 1;
    DebugSerial.print(", len: "); DebugSerial.print(result->len);
    uint8_t n = readBytes(serial, result->params, result->len, 2000);
    DebugSerial.print(", n: "); DebugSerial.print(n);
    DebugSerial.print(", params: ");
    for (uint8_t i=0; i<n; i++) {
        DebugSerial.print(result->params[i], HEX); DebugSerial.print(" ");
    }

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
        checksum += result->params[i];
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
            if (lastResult.opCode == 0x80) {
                uint8_t command = lastResult.params[0];
                uint8_t result = lastResult.params[1];
                if (command == opCode) {
                    return result;
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
