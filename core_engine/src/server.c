#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Engine Headers
#include "../../core_engine/include/pager.h"
#include "../../core_engine/include/buffer_pool.h"
#include "../../core_engine/include/bplus_tree.h"

#define PORT 8080
#define BUFFER_SIZE 1024

// Hashing function to turn service names into B+ Tree keys
uint32_t hash_string_to_key(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

int main() {
    // 1. Boot Database Engine
    printf("========================================\n");
    printf("[SERVER] Booting Zero-Trust Storage Engine...\n");
    Pager* pager = pager_open("kyl_vault.db");
    BufferPoolManager* bpm = bpm_create(pager);
    BPlusTree* tree = tree_create(bpm, 0);

    // 2. Setup TCP Socket
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Forcefully attach socket to the port 8080
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Engine Online. Listening on port %d...\n", PORT);
    printf("========================================\n");

    // 3. Endless Loop: Listen for Python Client Connections
    while(1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        char buffer[BUFFER_SIZE] = {0};
        read(client_socket, buffer, BUFFER_SIZE);
        
        char command[16] = {0}, service[64] = {0}, payload[MAX_VALUE_SIZE] = {0};
        char response[BUFFER_SIZE] = {0};
        
        // Parse incoming string from Python (e.g., "ADD twitter encrypted_pass")
        // We use %s to grab space-separated words
        sscanf(buffer, "%s %s %s", command, service, payload);
        uint32_t key = hash_string_to_key(service);

        // --- Database Routing Logic ---
        if (strcmp(command, "ADD") == 0) {
            tree_insert(tree, key, payload);
            snprintf(response, sizeof(response), "SUCCESS");
            printf("[DB] Inserted/Updated key %u (Service: %s)\n", key, service);
        } 
        else if (strcmp(command, "GET") == 0) {
            char retrieved_val[MAX_VALUE_SIZE] = {0};
            if (tree_search(tree, key, retrieved_val) == STATUS_SUCCESS && strcmp(retrieved_val, "DELETED") != 0) {
                snprintf(response, sizeof(response), "FOUND %s", retrieved_val);
                printf("[DB] Fetched key %u (Service: %s)\n", key, service);
            } else {
                snprintf(response, sizeof(response), "ERROR_NOT_FOUND");
                printf("[DB] Missed key %u (Service: %s)\n", key, service);
            }
        }
        else if (strcmp(command, "DEL") == 0) {
            tree_insert(tree, key, "DELETED");
            snprintf(response, sizeof(response), "SUCCESS_DELETED");
            printf("[DB] Soft-deleted key %u (Service: %s)\n", key, service);
        }
        else {
            snprintf(response, sizeof(response), "ERROR_UNKNOWN_COMMAND");
        }

        // Send response back to Python and close connection
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
    }

    return 0;
}
