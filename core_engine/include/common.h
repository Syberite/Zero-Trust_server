#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>

// Database Storage Constants
#define PAGE_SIZE 4096
#define MAX_KEY_SIZE 64
#define MAX_VALUE_SIZE 256

// Status and Error Codes
typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERR_DISK_FULL = 1,
    STATUS_ERR_CACHE_MISS = 2,
    STATUS_ERR_PAGE_NOT_FOUND = 3,
    STATUS_ERR_AUTH_FAILED = 4,
    STATUS_ERR_UNKNOWN = 5
} StatusCode;

// Basic Types
typedef uint32_t page_id_t;

#endif // COMMON_H
