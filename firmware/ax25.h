#ifndef AX25_H
#define AX25_H

#include <stdint.h>
#include <stdbool.h>

#ifndef UNUSED
#define UNUSED(x) (void)(sizeof(x))
#endif

/**
 * Data encoded in a AX.25 TOCALL, FROMCALL, and REPEATERS
 */
typedef struct {
    char callsign[7]; // null terminated
    uint8_t ssid;
    uint8_t rr;
    bool c_flag;
    bool last;
} AX25Call;

typedef enum {
    AX25_INFORMATION,
    AX25_UNNUMBERED,
    AX25_SUPERVISORY
} AX25FrameType;

typedef enum {
    AX25_U_SABME,
    AX25_U_SABM,
    AX25_U_DISC,
    AX25_U_DM,
    AX25_U_UA,
    AX25_U_FMRM,
    AX25_U_UI,
    AX25_U_XID,
    AX25_U_TEST,
    AX25_U_NONE
} AX25UnnumberedFrameType;

extern char * unnumbered_frame_type_names[9];

typedef enum {
    AX25_PID_LAYER_3,
    AX25_PID_NET_ROM,
    AX25_PID_NO_LAYER_3,
    AX25_PID_UNKNOWN
} AX25PidType;

extern char * pid_type_names[4];


typedef struct {
    AX25Call source_call;
    AX25Call dest_call;
    AX25Call repeater_call[3];
    AX25FrameType frame_type;
    uint8_t control_byte_pos;
} AX25Packet;

typedef void (*AX25Callback)(AX25Packet * packet, uint8_t * buffer, uint16_t len);

bool ax25_call_eq(AX25Call * call1, AX25Call * call2);

void ax25_call_cpy(AX25Call * dest, AX25Call * src);

uint16_t ax25_decode_call(AX25Call * call, uint8_t * buffer, uint16_t pos);

void ax25_encode_call(uint8_t * buffer, AX25Call * call);

bool ax25_init(AX25Packet * packet, uint8_t * buffer, uint16_t len);

AX25FrameType ax25_frame_type(AX25Packet * packet, uint8_t * buffer);

bool ax25_poll_final(AX25Packet * packet, uint8_t * buffer);

AX25UnnumberedFrameType ax25_u_subtype(AX25Packet * packet, uint8_t * buffer);

AX25PidType ax25_packet_pid_field(AX25Packet * packet, uint8_t * buffer);

uint8_t ax25_format_call( char * buf, AX25Call * call);

void ax25_print_call(AX25Call * call);

#endif
