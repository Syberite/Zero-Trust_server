#include "../include/bplus_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==========================================
// Helper Functions
// ==========================================

// Initialize a brand new Leaf Node in memory
void init_leaf_node(LeafNode* node) {
    node->header.type = NODE_TYPE_LEAF;
    node->header.num_keys = 0;
    node->header.parent_page_id = (page_id_t)-1; // -1 means no parent (is root)
    node->next_leaf_page = (page_id_t)-1;
}

// Initialize a brand new Internal Node in memory
void init_internal_node(InternalNode* node) {
    node->header.type = NODE_TYPE_INTERNAL;
    node->header.num_keys = 0;
    node->header.parent_page_id = (page_id_t)-1;
}

// Ask the Buffer Pool for a brand new page ID (expanding the file)
page_id_t get_new_page_id(BufferPoolManager* bpm) {
    // The pager tracks how many pages exist. We just take the next available index.
    return bpm->pager->num_pages;
}

// ==========================================
// Core API
// ==========================================

// Initialize the tree wrapper
BPlusTree* tree_create(BufferPoolManager* bpm, page_id_t root_page_id) {
    BPlusTree* tree = (BPlusTree*)malloc(sizeof(BPlusTree));
    tree->bpm = bpm;
    
    // If the database is brand new (0 pages), we must create the first root page
    if (bpm->pager->num_pages == 0) {
        page_id_t new_root_id = get_new_page_id(bpm);
        Frame* root_frame = bpm_fetch_page(bpm, new_root_id);
        
        init_leaf_node((LeafNode*)root_frame->data);
        tree->root_page_id = new_root_id;
        
        bpm_unpin_page(bpm, new_root_id, true); // Mark as dirty so it saves to disk
    } else {
        tree->root_page_id = root_page_id;
    }
    
    return tree;
}

// Search for a password by User ID (Key)
StatusCode tree_search(BPlusTree* tree, uint32_t key, char* output_value) {
    page_id_t current_page_id = tree->root_page_id;
    Frame* current_frame = bpm_fetch_page(tree->bpm, current_page_id);
    NodeHeader* header = (NodeHeader*)current_frame->data;

    // 1. Traverse down Internal Nodes until we hit a Leaf
    while (header->type == NODE_TYPE_INTERNAL) {
        InternalNode* internal = (InternalNode*)current_frame->data;
        page_id_t next_page_id = (page_id_t)-1;

        // Find the correct child pointer based on the key
        uint32_t i = 0;
        while (i < internal->header.num_keys && key >= internal->keys[i]) {
            i++;
        }
        next_page_id = internal->child_pointers[i];

        bpm_unpin_page(tree->bpm, current_page_id, false); // Release current page
        
        // Fetch the next level down
        current_page_id = next_page_id;
        current_frame = bpm_fetch_page(tree->bpm, current_page_id);
        header = (NodeHeader*)current_frame->data;
    }

    // 2. We are now at the Leaf Node. Scan for the key.
    LeafNode* leaf = (LeafNode*)current_frame->data;
    StatusCode status = STATUS_ERR_PAGE_NOT_FOUND;

    for (uint32_t i = 0; i < leaf->header.num_keys; i++) {
        if (leaf->records[i].key == key) {
            // Cache Hit! Copy the 256-byte encrypted password out
            memcpy(output_value, leaf->records[i].value, MAX_VALUE_SIZE);
            status = STATUS_SUCCESS;
            break;
        }
    }

    bpm_unpin_page(tree->bpm, current_page_id, false);
    return status;
}

// Insert a new User ID and Password into the Tree
StatusCode tree_insert(BPlusTree* tree, uint32_t key, const char* value) {
    page_id_t current_page_id = tree->root_page_id;
    Frame* current_frame = bpm_fetch_page(tree->bpm, current_page_id);
    NodeHeader* header = (NodeHeader*)current_frame->data;

    // (Simplified for this layer: Assumes root is currently a leaf node)
    // In a full implementation, you would traverse down to the leaf first.
    if (header->type != NODE_TYPE_LEAF) {
        printf("Error: Depth > 1 insertion requires recursive internal node logic.\n");
        bpm_unpin_page(tree->bpm, current_page_id, false);
        return STATUS_ERR_UNKNOWN;
    }

    LeafNode* leaf = (LeafNode*)current_frame->data;

    // SCENARIO 1: The Leaf has space. Insert and keep sorted.
    if (leaf->header.num_keys < LEAF_NODE_MAX_RECORDS) {
        uint32_t insert_idx = 0;
        while (insert_idx < leaf->header.num_keys && leaf->records[insert_idx].key < key) {
            insert_idx++;
        }

        // Shift records to the right to maintain sorted order
        for (uint32_t i = leaf->header.num_keys; i > insert_idx; i--) {
            leaf->records[i] = leaf->records[i - 1];
        }

        // Insert the new record
        leaf->records[insert_idx].key = key;
        memcpy(leaf->records[insert_idx].value, value, MAX_VALUE_SIZE);
        leaf->header.num_keys++;

        bpm_unpin_page(tree->bpm, current_page_id, true); // Dirty = true (we changed it)
        return STATUS_SUCCESS;
    }

    // SCENARIO 2: The Leaf is FULL (15 records). We must SPLIT the node.
    printf("Leaf Node Full. Splitting Node...\n");

    // 1. Ask the Buffer Pool for a brand new 4KB page
    page_id_t new_page_id = get_new_page_id(tree->bpm);
    Frame* new_frame = bpm_fetch_page(tree->bpm, new_page_id);
    LeafNode* new_leaf = (LeafNode*)new_frame->data;
    init_leaf_node(new_leaf);

    // 2. Calculate the split point (move the top 7 records to the new page)
    uint32_t split_index = LEAF_NODE_MAX_RECORDS / 2;
    uint32_t num_moving = leaf->header.num_keys - split_index;

    // 3. Move the data over
    for (uint32_t i = 0; i < num_moving; i++) {
        new_leaf->records[i] = leaf->records[split_index + i];
    }
    
    new_leaf->header.num_keys = num_moving;
    leaf->header.num_keys = split_index;

    // 4. Update the Linked List pointers (for sequential scans)
    new_leaf->next_leaf_page = leaf->next_leaf_page;
    leaf->next_leaf_page = new_page_id;

    // 5. Insert the new key into the correct half
    if (key < new_leaf->records[0].key) {
        // Belongs in the old left node
        leaf->records[leaf->header.num_keys].key = key;
        memcpy(leaf->records[leaf->header.num_keys].value, value, MAX_VALUE_SIZE);
        leaf->header.num_keys++;
    } else {
        // Belongs in the new right node
        new_leaf->records[new_leaf->header.num_keys].key = key;
        memcpy(new_leaf->records[new_leaf->header.num_keys].value, value, MAX_VALUE_SIZE);
        new_leaf->header.num_keys++;
    }

    // Unpin both pages and mark them as dirty so the Buffer Pool saves them
    bpm_unpin_page(tree->bpm, current_page_id, true);
    bpm_unpin_page(tree->bpm, new_page_id, true);

    return STATUS_SUCCESS;
}
