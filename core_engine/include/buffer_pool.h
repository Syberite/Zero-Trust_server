#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include "pager.h"
#include <stdbool.h>

// Define how many 4KB pages we can hold in RAM at once
#define POOL_SIZE 10

// A Frame holds one 4KB page in memory, plus metadata
typedef struct {
  page_id_t page_id;    // Which disk page is currently sitting here?
  char data[PAGE_SIZE]; // The actual 4096 bytes of memory
  bool is_dirty;        // Has this page been modified? (Needs writing to disk)
  int pin_count;        // How many active processes are reading this right now?
  uint32_t last_used;   // Timestamp counter for the LRU eviction algorithm
} Frame;

// The Manager handles the RAM array and talks to the Pager
typedef struct {
  Frame frames[POOL_SIZE]; // Our fixed block of RAM
  Pager *pager;            // Pointer to the disk I/O engine
  uint32_t current_time;   // Global counter to track LRU usage
} BufferPoolManager;

// API Prototypes
BufferPoolManager *bpm_create(Pager *pager);
Frame *bpm_fetch_page(BufferPoolManager *bpm, page_id_t page_id);
StatusCode bpm_unpin_page(BufferPoolManager *bpm, page_id_t page_id,
                          bool is_dirty);
StatusCode bpm_flush_all(BufferPoolManager *bpm);
void bpm_destroy(BufferPoolManager *bpm);

#endif // BUFFER_POOL_H
