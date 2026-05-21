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

char* host = "127.0.0.1";
uint32_t port = 7777;

int main(int argc, char* argv[]){

    bool s = true;
    bool c = true;

    if (argc > 1) {
        if (strcmp(argv[1], "s") == 0) {
            c = false;

        }
        else if (strcmp(argv[1], "c") == 0) {
            s = false;
            if (argc > 2) {
                host = argv[2];
            }
            if (argc > 3) {
                char* end_ptr = NULL;
                unsigned long parsed = strtoul(argv[3], &end_ptr, 10);
                if (end_ptr && *end_ptr == '\0' && parsed > 0 && parsed <= 65535) {
                    port = (uint32_t)parsed;
                } else {
                    printf("Invalid port '%s', using default 7777.\n", argv[3]);
                }
            }
        }
    }

    srand((unsigned int)time(NULL));

    Server server = {0};
    Client client = {0};

    if (s) {
        server_startup(&server);
    }

    if (c) {
        client_startup(&client, host, port);
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
