#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define ENET_IMPLEMENTATION
#include "enet.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "client.h"
#include "server.h"

char* host = "127.0.0.1";
uint32_t port = 7777;

static volatile int running = 1;

HANDLE server_tid = NULL;

static DWORD WINAPI server_thread(LPVOID arg) {
    Server* server = (Server*)arg;
    while (running) {
        if (!server_run(server)) {
            running = 0;
            break;
        }
    }
    return 0;
}

static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char* argv[]) {

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
    signal(SIGINT, handle_sigint);

    Server* server = (Server*)calloc(1, sizeof(Server));
    Client client = {0};

    if (!server) {
        fprintf(stderr, "Failed to allocate server.\n");
        return 1;
    }

    if (s) {
        server_startup(server);
        server_tid = CreateThread(NULL, 0, server_thread, server, 0, NULL);
    }

    if (c) {
        client_startup(&client, host, port);
    }


    while (running) {
        if (c) {
            if (!client_run(&client)) {
                running = 0;  
                break;
            }
        }
    }

    if (server_tid) {
        WaitForSingleObject(server_tid, INFINITE);
        CloseHandle(server_tid);
    }


    if (s) {
        server_close(server);
    }

    if (c) {
        client_close(&client);
    }

    free(server);

    return 0;
}
