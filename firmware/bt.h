#ifndef BT_H
#define BT_H

#include "btstack.h"

#define ADC_CHANNEL_TEMPSENSOR 4

extern int le_notification_enabled;
extern uint8_t const profile_data[];

typedef void (*BlockingTxCallback)(uint8_t * buffer, uint16_t len);

typedef void (*ConnectionCallback)(bool disconnected);

typedef void (*PinCallback)(uint32_t pin);

void init_spp(BlockingTxCallback spp_callback, ConnectionCallback con_callback, PinCallback pin_callback);

void init_ble(ConnectionCallback con_callback);

void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

bool has_connection();

void handle_tnc_rx_data(uint8_t * buffer, uint8_t size);

bool ble_has_tx_data();

int handle_ble_tx_data(BlockingTxCallback ble_callback);


#endif
