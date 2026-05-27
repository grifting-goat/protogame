#ifndef CLIENT_T_H
#define CLIENT_T_H

#include "event.h"


typedef struct {

    uint8_t peer_id;

    //state buffer
    //input buffer

    sysEventBus bus;
    


} client_t;

#endif //CLIENT_T_H