#include <stdio.h>
#include <string.h>

#include "ax25.h"

char * unnumbered_frame_type_names[] = {"SABME", "SABM", "DISC", "DM", "UA", "FMRM", "UI", "XID", "TEST"};

char * pid_type_names[] = {"L3", "NETROM", "NoL3", "?"};


/**
 * Compare two AX25Call objects. Only considers the callsign and SSID
 */
bool ax25_call_eq(AX25Call * call1, AX25Call * call2) {
    return strcmp(call1->callsign, call2->callsign) == 0 && call1->ssid == call2->ssid;
}

/**
 * Copy an AX25Call objects into another. Only copies the callsign and SSID
 */
void ax25_call_cpy(AX25Call * dest, AX25Call * src) {
    strcpy(dest->callsign, src->callsign);
    dest->ssid = src->ssid;
}

/**
 * Read a AX25Call off of the buffer. This advances the position by 7 bytes.
 */
uint16_t ax25_decode_call(AX25Call * call, uint8_t * buffer, uint16_t pos) {
    for (uint8_t i=0; i<6; i++) {
        char c = (buffer[pos++] & 0xff) >> 1;
        if (c != ' ') {
            call->callsign[i] = c;
        } else {
            call->callsign[i] = '\0';
        }
    }
    call->callsign[6] = '\0';
    uint8_t ssid = buffer[pos++];
    call->ssid = (ssid & 0x1E) >> 1;
    call->rr = (ssid & 0x60) >> 5;
    call->c_flag = (ssid & 0x80);   // bit 8 is "repeated"
    call->last = (ssid & 0x01);     // bit 1 is "last"
    return pos;
}

void ax25_encode_call(uint8_t * buffer, AX25Call * call) {
    for (uint8_t i=0; i<6; i++) {
        char c = call->callsign[i];
        if (c == '\0') {
            buffer[i] = ' ' << 1;
        } else {
            buffer[i] = call->callsign[i] << 1;
        }
    }
    uint8_t ssid = ((call->ssid << 1) & 0x1E) | call->last;
    if (call->c_flag) {
        ssid |= ((call->rr << 5) & 0x60);
        ssid |= 0x80;
    }
    buffer[6] = ssid;
}

void ax25_print_call(AX25Call * call) {
    printf("%s-%d (rr=%d c=%d l=%d)\n", call->callsign, call->ssid, call->rr, call->c_flag, call->last);
}

uint8_t ax25_format_call(char * buf, AX25Call * call) {
    return sprintf(buf, "%s-%d", call->callsign, call->ssid);
}

/**
 * Partially decode an AX25 packet using the given buffer.
 */
bool ax25_init(AX25Packet * packet, uint8_t * buffer, uint16_t len) {
    uint16_t pos = 0;
    pos = ax25_decode_call(&(packet->dest_call), buffer, pos);
    pos = ax25_decode_call(&(packet->source_call), buffer, pos);
    
    printf("Dest: "); ax25_print_call(&(packet->dest_call));
    printf("Source: "); ax25_print_call(&(packet->source_call));
    
    if (!packet->source_call.last) {
        uint8_t rpt_idx = 0;
        printf("Repeaters:\n");
        do {
            pos = ax25_decode_call(&(packet->repeater_call[rpt_idx]), buffer, pos);
            ax25_print_call(&(packet->repeater_call[rpt_idx]));
            rpt_idx++;
        } while (!packet->repeater_call[rpt_idx-1].last);
    }

    // If we find a matching "WIDE" repeater path, we replace it with our call and the C bit set, then WIDE n-1.
    // E.g., we receive WIDE1-1, we send K4DBZ-3*,WIDE1*.

    /*
    control_byte = next(byte_iter)
    poll_final = (control_byte & 0x10) == 0x10
    if (control_byte & 0x01) == 0:
        # I frame
        pid_byte = next(byte_iter)
        protocol = L3Protocol(pid_byte)
        info = bytes(byte_iter)
        recv_seq = (control_byte >> 5) & 0x07
        send_seq = (control_byte >> 1) & 0x07
        i_frame = IFrame(buffer, dest, source, repeaters, control_byte, poll_final, recv_seq, send_seq, protocol, info)
        if next(byte_iter, None):
            raise BufferError(f"Underflow exception, did not expect any more bytes here. {str(buffer)} IFrame: {i_frame}")
        else:
            return i_frame
            */
    
    packet->control_byte_pos = pos;
    /*
    uint8_t control_byte = buffer[pos++];
    bool poll_final = (control_byte & 0x10) == 0x10;
    ControlFieldType control_type;
    if (!(control_byte & 0b01)) {
        // bit 0 is unset, I-frame
        printf("I-frame p/f=%d\n", poll_final);
    } else if ((control_byte & 0b10) == 0b10) {
        // bit 1 is set, U-frame
        switch (control_byte & 0b11101100) {
            case 0b01101100:
                control_type = SABME;
                break;
            case 0b00101100:
                control_type = SABM;
                break;
            case 0b01000000:
                control_type = DISC;
                break;
            case 0b00001100:
                control_type = DM;
                break;
            case 0b01100000:
                control_type = UA;
                break;
            case 0b10000100:
                control_type = FMRM;
                break;
            case 0b00000000:
                control_type = UI;
                break;
            case 0b10101100:
                control_type = XID;
                break;
            case 0b11100000:
            default:
                control_type = TEST;
                break;
        }

        packet->control_type = control_type;
        if (control_type == UI) {
            uint8_t pid = buffer[pos++];
            printf("UI frame pid=%d\n", pid);
            while(pos < len) {
                printf("%02x ",buffer[pos++]);
            }
            //printf_hexdump(&buffer[pos], len-pos);
            printf("\n");
        } else {
            printf("U-frame type=%d\n p/f=%d\n", control_type, poll_final);
        }
       
    } else {
        // bit 1 is unset, S-frame
        printf("S-frame p/f=%d\n", poll_final);
    }
     */
    return true;
}


bool ax25_poll_final(AX25Packet * packet, uint8_t * buffer) {
    uint8_t control_byte = buffer[packet->control_byte_pos];
    bool poll_final = (control_byte & 0x10) == 0x10;
    return poll_final;
}

AX25FrameType ax25_frame_type(AX25Packet * packet, uint8_t * buffer) {
    uint8_t control_byte = buffer[packet->control_byte_pos];
    if (!(control_byte & 0b01)) {
        // bit 0 is unset, I-frame
        return AX25_INFORMATION;
    } else if ((control_byte & 0b10) == 0b10) {
        // bit 1 is set, U-frame
        return AX25_UNNUMBERED;
    } else {
        // bit 1 is unset, S-frame
        return AX25_SUPERVISORY;
    }
}

AX25UnnumberedFrameType ax25_u_subtype(AX25Packet * packet, uint8_t * buffer) {
    uint8_t control_byte = buffer[packet->control_byte_pos];
    if ((control_byte & 0b10) != 0b10) {
        // not U frame
        return AX25_U_NONE;
    }

    switch (control_byte & 0b11101100) {
        case 0b01101100:
            return AX25_U_SABME;
        case 0b00101100:
            return AX25_U_SABM;
        case 0b01000000:
            return AX25_U_DISC;
        case 0b00001100:
            return AX25_U_DM;
        case 0b01100000:
            return AX25_U_UA;
        case 0b10000100:
            return AX25_U_FMRM;
        case 0b00000000:
            return AX25_U_UI;
        case 0b10101100:
            return AX25_U_XID;
        case 0b11100000:
            return AX25_U_TEST;
        default:
            return AX25_U_NONE;
    }
}

AX25PidType ax25_packet_pid_field(AX25Packet * packet, uint8_t * buffer) {
    AX25FrameType frame_type = ax25_frame_type(packet, buffer);
    if (frame_type == AX25_SUPERVISORY) {
        return AX25_PID_UNKNOWN;
    } else if (frame_type == AX25_UNNUMBERED) {
        AX25UnnumberedFrameType u_subtype = ax25_u_subtype(packet, buffer);
        if (u_subtype != AX25_U_UI) {
            return AX25_PID_UNKNOWN;
        }
    }
    
    uint8_t pid_byte = buffer[packet->control_byte_pos + 1];
    if (pid_byte == 0xCF) {
        return AX25_PID_NET_ROM;
    } else if (pid_byte == 0xF0) {
        return AX25_PID_NO_LAYER_3;
    } else {
        return AX25_PID_UNKNOWN;
    }
}