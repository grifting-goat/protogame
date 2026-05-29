#ifndef CLIENT_T_H
#define CLIENT_T_H

#include "event.h"

//each client that connects gets this fun thing


typedef struct {

    uint8_t peer_id;

    //state buffer
    //input buffer

    Event_bus bus;
    


} client_t;

#endif //CLIENT_T_H