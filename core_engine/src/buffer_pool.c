#include "../include/buffer_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize the RAM pool
BufferPoolManager *bpm_create(Pager *pager) {
  BufferPoolManager *bpm =
      (BufferPoolManager *)malloc(sizeof(BufferPoolManager));
  if (bpm == NULL) {
    printf("Error: Failed to allocate Buffer Pool Manager.\n");
    exit(EXIT_FAILURE);
  }

  bpm->pager = pager;
  bpm->current_time = 0;

  // Set all frames to empty initially
  for (int i = 0; i < POOL_SIZE; i++) {
    bpm->frames[i].page_id = (page_id_t)-1; // -1 denotes an empty frame
    bpm->frames[i].is_dirty = false;
    bpm->frames[i].pin_count = 0;
    bpm->frames[i].last_used = 0;
    memset(bpm->frames[i].data, 0, PAGE_SIZE);
  }

  return bpm;
}

// The core LRU cache algorithm
Frame *bpm_fetch_page(BufferPoolManager *bpm, page_id_t page_id) {
  bpm->current_time++; // Increment global timer for LRU tracking

  // 1. Search for a Cache Hit
  for (int i = 0; i < POOL_SIZE; i++) {
    if (bpm->frames[i].page_id == page_id) {
      bpm->frames[i].pin_count++;
      bpm->frames[i].last_used = bpm->current_time;
      return &bpm->frames[i]; // Fast return from RAM
    }
  }

  // 2. Cache Miss: We need to load it from disk. Find a frame to replace.
  int target_frame_index = -1;
  uint32_t oldest_time = 0xFFFFFFFF; // Max uint32 value

  for (int i = 0; i < POOL_SIZE; i++) {
    // Find an empty frame immediately if one exists
    if (bpm->frames[i].page_id == (page_id_t)-1) {
      target_frame_index = i;
      break;
    }
    // Otherwise, apply LRU logic: find the oldest unpinned frame
    if (bpm->frames[i].pin_count == 0 &&
        bpm->frames[i].last_used < oldest_time) {
      oldest_time = bpm->frames[i].last_used;
      target_frame_index = i;
    }
  }

  // If all frames are pinned (in use), the cache is deadlocked
  if (target_frame_index == -1) {
    printf("Error: All buffer frames are pinned. Cannot fetch page %u.\n",
           page_id);
    return NULL;
  }

  Frame *target_frame = &bpm->frames[target_frame_index];

  // 3. Evict the old page if it was dirty
  if (target_frame->is_dirty && target_frame->page_id != (page_id_t)-1) {
    pager_write(bpm->pager, target_frame->page_id, target_frame->data);
  }

  // 4. Load the new page from the disk into this RAM frame
  target_frame->page_id = page_id;
  target_frame->pin_count = 1; // Pin it for the caller
  target_frame->is_dirty = false;
  target_frame->last_used = bpm->current_time;

  // Call the Pager we built to fetch the data
  if (pager_read(bpm->pager, page_id, target_frame->data) != STATUS_SUCCESS) {
    // If the page doesn't exist yet, we just initialize it with zeros
    memset(target_frame->data, 0, PAGE_SIZE);
  }

  return target_frame;
}

// Release a frame so it can be evicted later
StatusCode bpm_unpin_page(BufferPoolManager *bpm, page_id_t page_id,
                          bool is_dirty) {
  for (int i = 0; i < POOL_SIZE; i++) {
    if (bpm->frames[i].page_id == page_id) {
      if (bpm->frames[i].pin_count > 0) {
        bpm->frames[i].pin_count--;
      }
      if (is_dirty) {
        bpm->frames[i].is_dirty = true;
      }
      return STATUS_SUCCESS;
    }
  }
  return STATUS_ERR_PAGE_NOT_FOUND;
}

// Flush all modified pages to disk (used during safe shutdown)
StatusCode bpm_flush_all(BufferPoolManager *bpm) {
  for (int i = 0; i < POOL_SIZE; i++) {
    if (bpm->frames[i].page_id != (page_id_t)-1 && bpm->frames[i].is_dirty) {
      pager_write(bpm->pager, bpm->frames[i].page_id, bpm->frames[i].data);
      bpm->frames[i].is_dirty = false;
    }
  }
  return STATUS_SUCCESS;
}

void bpm_destroy(BufferPoolManager *bpm) {
  if (bpm != NULL) {
    bpm_flush_all(bpm); // Ensure no data is lost
    free(bpm);
  }
}
