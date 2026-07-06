#ifndef PAGER_H
#define PAGER_H

#include "common.h"
#include <stdio.h> // For FILE* or you can use int file_descriptor for POSIX

// The Pager struct holds the state of our database file
typedef struct {
    int file_descriptor; // The OS-level file handle
    uint32_t file_length; // Total size of the file in bytes
    uint32_t num_pages;   // file_length / PAGE_SIZE
} Pager;

// Function Prototypes (The API for Layer 5)
Pager* pager_open(const char* filename);
StatusCode pager_read(Pager* pager, page_id_t page_num, void* destination);
StatusCode pager_write(Pager* pager, page_id_t page_num, const void* source);
void pager_close(Pager* pager);

#endif // PAGER_H
