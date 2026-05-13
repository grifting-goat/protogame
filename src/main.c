#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ENET_IMPLEMENTATION
#include "enet.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "client.h"
#include "server.h"

int main(int argc, char* argv[]){

    bool s = true;
    bool c = true;

    if (argc > 1) {
        if (strcmp(argv[1], "s") == 0) {
            c = false;
        }
        else if (strcmp(argv[1], "c") == 0) {
            s = false;
        }
    }

    srand((unsigned int)time(NULL));

    Server server = {0};
    Client client = {0};

    if (s) {
        server_startup(&server);
    }

    if (c) {
        client_startup(&client);
    }


    while (1) {
        if (c) {
            if (!client_run(&client)) break;
        }

        if (s) {
            if (!server_run(&server)) break; 
        }
    }

    if (s) {
        server_close(&server);
    }

    if (c) {
        client_close(&client);
    }

    return 0;
}
