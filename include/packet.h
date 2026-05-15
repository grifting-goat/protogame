#ifndef PACKET_H
#define PACKET_H

#define PCKT_SERVER_ID 0x00
#define PCKT_CLIENT_ACK 0x01
#define PCKT_ADD_PLAYER 0x02
#define PCKT_CLIENT_POS 0x03
#define PCKT_SERVER_POS 0x04
#define PCKT_REMOVE_PLAYER 0x05


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;
}
Packet_server_id;

typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;
    Vec3 pos;
    Vec3 vel;
    uint32_t state;
    
} Packet_pos;


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;
    uint64_t uqid;

} Packet_client_ack;


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;
    uint64_t uqid;

} Packet_player;


#endif //PACKET_H