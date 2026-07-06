#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include "buffer_pool.h"
#include <stdint.h>
#include <stdbool.h>

// B+ Tree Node Types
typedef enum {
    NODE_TYPE_INTERNAL = 0,
    NODE_TYPE_LEAF = 1
} NodeType;

// ==========================================
// 1. Common Node Header (Applies to both)
// ==========================================
// Every 4KB page in the tree starts with this header so we know what it is.
typedef struct {
    NodeType type;           // Is it an Internal or Leaf node?
    uint32_t num_keys;       // How many keys are currently in this node?
    page_id_t parent_page_id; // Pointer to the parent node (-1 if root)
} NodeHeader;

// ==========================================
// 2. Leaf Node Structure (Stores actual data)
// ==========================================
// Record size: 4 bytes (Key) + 256 bytes (Value) = 260 bytes.
// (4096 - sizeof(NodeHeader) - 4 bytes for next_leaf) / 260 = ~15 records max
#define LEAF_NODE_MAX_RECORDS 15

typedef struct {
    uint32_t key;
    char value[MAX_VALUE_SIZE]; // 256 bytes for AES encrypted payload
} LeafRecord;

typedef struct {
    NodeHeader header;
    page_id_t next_leaf_page; // Pointer to the next leaf for sequential scans
    LeafRecord records[LEAF_NODE_MAX_RECORDS];
} LeafNode;

// ==========================================
// 3. Internal Node Structure (Stores routing paths)
// ==========================================
// Pointer size: 4 bytes. Key size: 4 bytes.
// (4096 - sizeof(NodeHeader)) / 8 = ~500 routing paths max
#define INTERNAL_NODE_MAX_KEYS 500

typedef struct {
    NodeHeader header;
    uint32_t keys[INTERNAL_NODE_MAX_KEYS];
    page_id_t child_pointers[INTERNAL_NODE_MAX_KEYS + 1]; // Always one more pointer than keys
} InternalNode;

// ==========================================
// Tree Manager API
// ==========================================
typedef struct {
    BufferPoolManager* bpm;
    page_id_t root_page_id;
} BPlusTree;

// Core functions
BPlusTree* tree_create(BufferPoolManager* bpm, page_id_t root_page_id);
StatusCode tree_insert(BPlusTree* tree, uint32_t key, const char* value);
StatusCode tree_search(BPlusTree* tree, uint32_t key, char* output_value);

#endif // BPLUS_TREE_H
