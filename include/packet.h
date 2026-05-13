#ifndef PACKET_H
#define PACKET_H

#define PCKT_SERVER_ID 0x00
#define PCKT_CLIENT_ACK 0x01
#define PCKT_ADD_PLAYER 0x02
#define PCKT_CLIENT_POS 0x03
#define PCKT_SERVER_POS 0x04

typedef struct {
    uint8_t pckt_id;
    uint8_t server_id;
    Vec3 pos;
    
} Packet_pos;


typedef struct {
    uint8_t pckt_id;
    uint8_t server_id;
    uint64_t uqid;

} Packet_client_ack;


typedef struct {
    uint8_t pckt_id;
    uint8_t server_id;
    uint64_t uqid;
} Packet_new_player;


#endif //PACKET_H