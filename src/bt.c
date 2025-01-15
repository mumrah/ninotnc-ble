#include <stdio.h>
#include "btstack.h"

#include "bt.h"
#include "ninoble_gatt_services.h"

#define RFCOMM_SERVER_CHANNEL 1

#define APP_AD_FLAGS 0x06
static uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Name
    0x08, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'N', 'i', 'n', 'o', 'B', 'L', 'E',
    // Service UUID needs to be advertised for APRS.fi to find it
    0x11, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0xBB, 0x68, 0x1F, 0x96, 0xB0, 0x01, 0x49, 0xAE, 0xC9, 0x46, 0x2A, 0xBA, 0x01, 0x00, 0x00, 0x00,
};
static const uint8_t adv_data_len = sizeof(adv_data);

static hci_con_handle_t con_handle;
static btstack_packet_callback_registration_t hci_event_callback_registration;

// Characteristic KTS_RX_CHAR_UUID 00000003-ba2a-46c9-ae49-01b0961f68bb
// This characteristic is used for receiving packets from the TNC over UART and forwarding to
// the BLE client.
static uint8_t     char_rx_value[400];                 // This buffer is also used for sending to SPP
static uint16_t    char_rx_value_len = 0;
static bool        char_rx_value_sending = false;
static uint16_t    char_rx_client_configuration;
static char *      char_rx_user_description = "rx";

// Method to call when we connect/disconnect BLE
static ConnectionCallback ble_con_callback;


// Characteristic KTS_TX_CHAR_UUID 00000002-ba2a-46c9-ae49-01b0961f68bb
// This characteristic is used for transmitting packets to the TNC.
uint8_t     char_tx_value[400];
uint16_t    char_tx_value_len = 0;
char *      char_tx_user_description = "tx";

// SPP
static uint16_t  rfcomm_channel_id;
static BlockingTxCallback rfcomm_rx_callback;
static ConnectionCallback rfcomm_con_callback;
static PinCallback rfcomm_pin_callback;


void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    //printf("packet_handler packet_type=%02X channel=%04X\n", packet_type, channel);
    UNUSED(size);
    UNUSED(channel);

    switch (packet_type) {
        case HCI_EVENT_PACKET:
            bd_addr_t local_addr;
            uint8_t event_type = hci_event_packet_get_type(packet);
            // SPP stuff
            bd_addr_t event_addr;
            uint8_t   rfcomm_channel_nr;
            switch (event_type) {
                // btstack events
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
                    gap_local_bd_addr(local_addr);
                    printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

                    // setup advertisements
                    uint16_t adv_int_min = 800;
                    uint16_t adv_int_max = 800;
                    uint8_t adv_type = 0;
                    bd_addr_t null_addr;
                    memset(null_addr, 0, 6);
                    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
                    assert(adv_data_len <= 31); // ble limitation
                    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
                    gap_advertisements_enable(1);
                    break;
                case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:
                    break;

                // HCI events
                case HCI_EVENT_PIN_CODE_REQUEST:
                    // inform about pin code request
                    printf("Pin code request - using '0000'\n");
                    hci_event_pin_code_request_get_bd_addr(packet, event_addr);
                    gap_pin_code_response(event_addr, "0000");
                    break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    // inform about user confirmation request
                    uint32_t pin = little_endian_read_32(packet, 8);
                    printf("SSP User Confirmation Request with numeric value '%06lu'\n", pin);
                    printf("SSP User Confirmation Auto accept\n");
                    rfcomm_pin_callback(pin);
                    break;

                case HCI_EVENT_TRANSPORT_PACKET_SENT:
                    // Called after BLE client reads from RX characteristic 
                    if (char_rx_value_sending) {
                        char_rx_value_len = 0;
                    }
                    break;
                case HCI_EVENT_CONNECTION_COMPLETE:
                    //printf("HCI_EVENT_CONNECTION_COMPLETE\n");               
                    break;
                case HCI_EVENT_DISCONNECTION_COMPLETE:
                    //printf("HCI_EVENT_DISCONNECTION_COMPLETE\n");
                    if (con_handle != 0 && ble_con_callback != NULL) {
                        ble_con_callback(true);
                    }
                    con_handle = 0;
                    char_rx_client_configuration = 0;
                    char_rx_value_len = 0;
                    char_rx_value_sending = false;
                    break;
                case HCI_EVENT_COMMAND_COMPLETE:
                    //printf("HCI_EVENT_COMMAND_COMPLETE\n");
                    break;
                case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
                    break;

                // ATT events
                case ATT_EVENT_CONNECTED:
                    bd_addr_t address;
                    att_event_connected_get_address(packet, address);
                    uint8_t address_type = att_event_connected_get_address_type(packet);
                    printf("ATT connected. address=%02x type=%d handle=%04x\n", address, address_type, att_event_connected_get_handle(packet));
                    break;
                case ATT_EVENT_DISCONNECTED:
                    printf("ATT disconnected. handle=%04x\n", att_event_disconnected_get_handle(packet));
                    break;
                case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
                    uint16_t mtu = att_event_mtu_exchange_complete_get_MTU(packet);
                    printf("ATT MTU exchange complete. handle=%04x MTU=%d\n", att_event_disconnected_get_handle(packet), mtu);
                    break;
                case ATT_EVENT_CAN_SEND_NOW:
                    // We might have data ready to send to the BLE client. Check if we have connection and data waiting.
                    if (con_handle && char_rx_client_configuration && char_rx_value_len > 0) {
                        printf("RX value to send to BLE client (%d bytes):\n", char_rx_value_len);
                        printf_hexdump(char_rx_value, char_rx_value_len);
                        printf("\n");
                        // Don't set char_rx_value_len to zero until we see the HCI_EVENT_TRANSPORT_PACKET_SENT
                        char_rx_value_sending = true; 
                        att_server_notify(con_handle, ATT_CHARACTERISTIC_00000003_ba2a_46c9_ae49_01b0961f68bb_01_VALUE_HANDLE, char_rx_value, char_rx_value_len);
                    }
                    break;

                // SPP
                case RFCOMM_EVENT_INCOMING_CONNECTION:
                    // data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
                    rfcomm_event_incoming_connection_get_bd_addr(packet, event_addr); 
                    rfcomm_channel_nr = rfcomm_event_incoming_connection_get_server_channel(packet);
                    rfcomm_channel_id = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);
                    printf("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
                    rfcomm_accept_connection(rfcomm_channel_id);
                    break;
                    
                case RFCOMM_EVENT_CHANNEL_OPENED:
                    // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
                    if (rfcomm_event_channel_opened_get_status(packet)) {
                        printf("RFCOMM channel open failed, status 0x%02x\n", rfcomm_event_channel_opened_get_status(packet));
                    } else {
                        rfcomm_channel_id = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
                        mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
                        printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_channel_id, mtu);
                        if (rfcomm_con_callback != NULL) {
                           rfcomm_con_callback(false);
                        }
                    }
                    break;

                case RFCOMM_EVENT_CAN_SEND_NOW:
                    // We might have data ready to send to the SPP client. Check if we have connection and data waiting.
                    if (rfcomm_channel_id) {
                        printf("RX value to send to SPP channel %04d (%d bytes):\n", rfcomm_channel_id, char_rx_value_len);
                        printf_hexdump(char_rx_value, char_rx_value_len);
                        printf("\n");
                        rfcomm_send(rfcomm_channel_id, char_rx_value, char_rx_value_len);
                        char_rx_value_len = 0;
                    }
                    break;

                case RFCOMM_EVENT_CHANNEL_CLOSED:
                    printf("RFCOMM channel closed\n");
                    rfcomm_channel_id = 0;
                    if (rfcomm_con_callback != NULL) {
                        rfcomm_con_callback(true);
                    }
                    break;

                default:
                    printf("Unhandled event %02X\n", event_type);
                    break;
            }
            break;
        case RFCOMM_DATA_PACKET:
            printf("SPP RX on channel %04X\n", rfcomm_channel_id);
            printf_hexdump(packet, size);
            // TODO feed into KISS parser to send to TNC
            printf("\n");
            rfcomm_rx_callback(packet, size);
            break;
        default:
            printf("Ignoring packet type %02X\n", packet_type);
            break;
    }
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    printf("att_read_callback conn=%04X attr=%04X\n", connection_handle, att_handle);
    UNUSED(connection_handle);

    // RX
    if (att_handle == ATT_CHARACTERISTIC_00000003_ba2a_46c9_ae49_01b0961f68bb_01_CLIENT_CONFIGURATION_HANDLE) {
        return att_read_callback_handle_little_endian_16(char_rx_client_configuration, offset, buffer, buffer_size);
    }

    if (att_handle == ATT_CHARACTERISTIC_00000003_ba2a_46c9_ae49_01b0961f68bb_01_USER_DESCRIPTION_HANDLE) {
        return att_read_callback_handle_blob(char_rx_user_description, strlen(char_rx_user_description), offset, buffer, buffer_size);
    }

    if (att_handle == ATT_CHARACTERISTIC_00000003_ba2a_46c9_ae49_01b0961f68bb_01_VALUE_HANDLE) {
        return att_read_callback_handle_blob(char_rx_value, strlen(char_rx_value), offset, buffer, buffer_size);
    }

    // TX
    if (att_handle == ATT_CHARACTERISTIC_00000002_ba2a_46c9_ae49_01b0961f68bb_01_USER_DESCRIPTION_HANDLE) {
        return att_read_callback_handle_blob(char_tx_user_description, strlen(char_tx_user_description), offset, buffer, buffer_size);
    } 
    if (att_handle == ATT_CHARACTERISTIC_00000002_ba2a_46c9_ae49_01b0961f68bb_01_VALUE_HANDLE) { 
        return 0;
    }

    return 0;
}

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    printf("att_write_callback conn=%04X attr=%04X\n", connection_handle, att_handle);
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);

    if (att_handle == ATT_CHARACTERISTIC_00000003_ba2a_46c9_ae49_01b0961f68bb_01_CLIENT_CONFIGURATION_HANDLE) {
        // APRS.fi is registering a callback for RX
        char_rx_client_configuration = little_endian_read_16(buffer, 0);
        con_handle = connection_handle;
        if (char_rx_client_configuration) {
            att_server_request_can_send_now_event(con_handle);
        }
        // Register connection callback here after the client has finished initializing
        if (ble_con_callback != NULL) {
            ble_con_callback(false);
        }     
        return 0;
    }

    if (att_handle == ATT_CHARACTERISTIC_00000002_ba2a_46c9_ae49_01b0961f68bb_01_VALUE_HANDLE) {
		if (char_tx_value_len != 0) {
            printf("Ignoring TX packet from BLE since we have one inflight. Data was:\n");
            printf_hexdump(buffer, buffer_size);
            printf("\n");
        } else {
            memcpy(char_tx_value, buffer, buffer_size);
            char_tx_value_len = buffer_size;
            printf("TX BLE -> TNC:\n");
            printf_hexdump(char_tx_value, buffer_size);
            printf("\n");
        }
    }
    
    return 0;
}

/*****************************
 *       Public APIs         *
 *****************************/

void init_ble(ConnectionCallback con_callback) {
    // BLE init
    att_server_init(profile_data, att_read_callback, att_write_callback);    

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for ATT event
    att_server_register_packet_handler(packet_handler);   

    ble_con_callback = con_callback;

    // turn on bluetooth!
    hci_power_control(HCI_POWER_ON);
}


void init_spp(BlockingTxCallback spp_callback, ConnectionCallback con_callback, PinCallback pin_callback) {
    rfcomm_rx_callback = spp_callback;
    rfcomm_con_callback = con_callback;
    rfcomm_pin_callback = pin_callback;
    rfcomm_init();
    rfcomm_register_service(packet_handler, RFCOMM_SERVER_CHANNEL, 0xffff);
}

bool has_connection() {
    return con_handle != 0;
}

/**
 * Copy the buffer into the BLE RX characteristic and notify BTStack that data should be sent to the client.
 */
void handle_tnc_rx_data(uint8_t * buffer, uint8_t len) {
    if (char_rx_value_len > 0) {
        // Check in-flight send to client
        printf("Not sending RX'd packet due to in-flight packet. Packet was:\n");
        printf_hexdump(buffer, len);
        printf("\n");
        return;
    }

    // BLE
    if (char_rx_client_configuration) {
        printf("RX TNC -> BLE\n");
        memcpy(char_rx_value, buffer, len);
        char_rx_value_len = len;
        att_server_request_can_send_now_event(con_handle);     
    } 

    // SPP
    if (rfcomm_channel_id) {
        printf("RX TNC -> SPP\n");
        memcpy(char_rx_value, buffer, len);
        char_rx_value_len = len;
        rfcomm_request_can_send_now_event(rfcomm_channel_id);
    }
}

/*
SPP connect:
Unhandled event 04
Unhandled event 0F
Unhandled event 1B
Unhandled event 13
Unhandled event 38
Unhandled event 13
Unhandled event 13
Unhandled event 17
Unhandled event 08
Unhandled event D8
Unhandled event 0F
Unhandled event 0B
Unhandled event 13
Unhandled event 0F
Unhandled event 23
Unhandled event 0F
Unhandled event 23
Unhandled event 13
Unhandled event 13
RFCOMM channel 1 requested for B4:F1:DA:6B:61:D3
Unhandled event 13
Unhandled event 13
Unhandled event 87  RFCOMM_EVENT_REMOTE_MODEM_STATUS
RFCOMM channel open succeeded. New RFCOMM Channel ID 2, max frame size 990
*/

bool ble_has_tx_data() {
    return char_tx_value_len > 0;
}

/**
 * Call the given callback with the TX characteristic data
 */
int handle_ble_tx_data(BlockingTxCallback ble_callback) {
    uint16_t len = char_tx_value_len;
    ble_callback(char_tx_value, len);
    char_tx_value_len = 0;
    return len;
}
