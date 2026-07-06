#include "../include/common.h"
#include "../include/pager.h"
#include "../include/buffer_pool.h"
#include "../include/bplus_tree.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    printf("[+] B+ Tree Indexing Engine initialized.\n\n");

    // 4. Test Insertion
    printf("[*] Inserting User 101...\n");
    tree_insert(tree, 101, "AES_GCM_ENCRYPTED_PAYLOAD_101");
    
    printf("[*] Inserting User 999...\n");
    tree_insert(tree, 999, "AES_GCM_ENCRYPTED_PAYLOAD_999");

    // 5. Test Searching
    char retrieved_password[MAX_VALUE_SIZE];
    memset(retrieved_password, 0, MAX_VALUE_SIZE);

    printf("\n[*] Searching for User 101...\n");
    StatusCode status = tree_search(tree, 101, retrieved_password);
    
    if (status == STATUS_SUCCESS) {
        printf("[SUCCESS] Found User 101! Password Data: %s\n", retrieved_password);
    } else {
        printf("[FAILED] Could not find User 101.\n");
    }

    // 6. Graceful Shutdown (Triggers dirty page flushes to disk)
    printf("\nShutting down engine and flushing cache to disk...\n");
    free(tree);
    bpm_destroy(bpm);
    pager_close(pager);

    printf("--- Engine Offline ---\n");
    return 0;
}
