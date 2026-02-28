#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "client.h"
#include "server.h"

int main(void){

    srand((unsigned int)time(NULL));

    Client client;
    Server server;

    client_startup(&client);
    server_startup(&server);

    bool running = true;

    while (running) {
        running = client_run(&client) & server_run(&server);
    }

    client_close(&client);
    server_close(&server);

    return 0;
}
