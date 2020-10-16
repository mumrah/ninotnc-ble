#include "BM70.h"

BM70::BM70()
{ }

BM70::BM70(HardwareSerial * initSerial, uint32_t baudrate, RxCallback callback)
{
	serial = initSerial;
    rxCallback = callback;
	serial->begin (baudrate);
	currentStatus = BM70_STATUS_UNKNOWN;
	connectionHandle = 0x00;
}

/**
 * Read some data off the BM70 UART buffer.
 * 
 * Some events will be automatically handled to update our internal state,
 * others will be passed on to the callback.
 */
uint8_t BM70::read()
{
    if (read(&lastResult) != 0)
        return -1;
    
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
                connectionHandle = lastResult.params[1];
                uint64_t address = 0;
				for (uint8_t i = 0; i < 6; i++)
					address += (((uint64_t) lastResult.params[i + 4]) << (8 * i));
                connectionAddress = address;
            }
            else
            {
                // failed, ignore
            }
            break;
        case 0x72:
            // LE Disconnect Event
            connectionHandle = 0x00;
            connectionAddress = 0x00;
            break;
        case 0x80:
        {
            // Command Complete Event, ignore for now
            uint8_t command = lastResult.params[0];
            uint8_t result = lastResult.params[1];
            break;
        }   
        case 0x81:
            // Status Report Event
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
    uint64_t t = millis();
    while (serial->available() == 0)
    {
        if ((millis() - t) > timeout) 
            return -1;
    }  

    uint8_t checksum = 0;
    uint8_t d = 0;

    // read until we find sync word 0xAA
    while (d != 0xAA) 
    {
        if ((millis() - t) > timeout) 
            return -1;
        d = serial->read();
        delay(1);
    }

    uint8_t h = serial->read();
    uint8_t l = serial->read();
    result->opCode = serial->read();
    result->len = (((short) h) << 8 ) + l - 1;
    uint8_t n = serial->readBytes(result->params, result->len);
    
    result->checksum = serial->read();

    if (n < result->len) {
        // underflow
        return -2;
    }

    checksum += (h + l + result->opCode);
    for (uint8_t i=0; i<result->len; i++) {
        checksum += result->params[i];
    }
    checksum += result->checksum;

    if (checksum != 0) {
        // error, what now?
        return -3;
    }
    return 0;
}

void BM70::write0 (uint8_t opCode)
{
    write(opCode, NULL, 0);
}

void BM70::write1 (uint8_t opCode, uint8_t param)
{
    uint8_t params[1];
    params[0] = param;
    write(opCode, params, 1);
}

void BM70::write (uint8_t opCode, uint8_t * params, uint8_t len)
{
    write(opCode, params, len, BM70_DEFAULT_TIMEOUT);
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
    
    uint64_t t = millis();

    while (serial -> availableForWrite() < len + 5)
    {
        if ((millis() - t) > timeout) 
            return -1;
        delay(1);
    }
    serial -> write(buffer, len + 5); 
    return len + 5;
}

// Commands and such

void BM70::reset()
{
    write0(BM70_OP_RESET);
}

void BM70::updateStatus()
{
    write0(BM70_OP_READ_STATUS);
    //readCommandResponse();
}

uint8_t BM70::status() {
    return currentStatus;
}

void BM70::enableAdvertise()
{
    write1(BM70_OP_ADVERTISE_ENABLE, 0x01); 
    //readCommandResponse();
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
            delay(10);
            offset = 3;
        }
    }
    
    if (offset > 3)
    {
        write(BM70_OP_SEND_CHAR, params, offset);
        delay(10);
    }
}
