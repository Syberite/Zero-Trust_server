#include "../include/common.h"
#include "../include/pager.h"
#include "../include/buffer_pool.h"
#include "../include/bplus_tree.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ==========================================
// Integration Helper: String Hashing (djb2)
// ==========================================
// Converts a string key (like "twitter") into a unique uint32_t integer for the B+ Tree.
uint32_t hash_string_to_key(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

int main() {
    printf("--- Booting Zero-Trust Database Engine ---\n");

    // 1. Initialize Layer 5: Disk Storage (Pager)
    Pager* pager = pager_open("vault.db");
    printf("[+] Pager initialized. Database file: vault.db (Pages on disk: %u)\n", pager->num_pages);

    // 2. Initialize Layer 5: Memory Cache (Buffer Pool)
    BufferPoolManager* bpm = bpm_create(pager);
    printf("[+] Buffer Pool Manager initialized (10 RAM Frames).\n");

    // 3. Initialize Layer 4: B+ Tree Index
    // For this simple test, we assume the root node is always on Page 0
    BPlusTree* tree = tree_create(bpm, 0);
    printf("[+] Engine Online. Ready for KYL integration.\n\n");

    // 4. Simulate KYL 'add' command
    // User types: KYL add twitter MySecurePassword123
    const char* service1 = "twitter";
    const char* encrypted_pass1 = "ENCRYPTED_MySecurePassword123";
    
    uint32_t tree_key1 = hash_string_to_key(service1);
    
    printf("[*] KYL CMD: Adding password for '%s' (Hash Key: %u)...\n", service1, tree_key1);
    tree_insert(tree, tree_key1, encrypted_pass1);
    
    const char* service2 = "google";
    uint32_t tree_key2 = hash_string_to_key(service2);
    printf("[*] KYL CMD: Adding password for '%s' (Hash Key: %u)...\n", service2, tree_key2);
    tree_insert(tree, tree_key2, "ENCRYPTED_GooglePass456");

    // 5. Simulate KYL 'get' command
    // User types: KYL get twitter
    char retrieved_password[MAX_VALUE_SIZE];
    memset(retrieved_password, 0, MAX_VALUE_SIZE);

    printf("\n[*] KYL CMD: Searching for '%s' (Hash Key: %u)...\n", service1, tree_key1);
    StatusCode status = tree_search(tree, tree_key1, retrieved_password);
    
    if (status == STATUS_SUCCESS) {
        printf("[SUCCESS] Found! Encrypted Data: %s\n", retrieved_password);
        // Here, KYL would decrypt 'retrieved_password' and show it to the user.
    } else {
        printf("[FAILED] Service not found.\n");
    }

    // 6. Graceful Shutdown (Triggers dirty page flushes to disk)
    printf("\nShutting down engine and flushing cache to disk...\n");
    free(tree);
    bpm_destroy(bpm);
    pager_close(pager);

    printf("--- Engine Offline ---\n");
    return 0;
}
