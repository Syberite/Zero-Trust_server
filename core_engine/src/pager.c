#include "../include/pager.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Initialize the pager, open the database file, and calculate current pages
Pager *pager_open(const char *filename) {
  // Allocate memory for our Pager struct
  Pager *pager = (Pager *)malloc(sizeof(Pager));
  if (pager == NULL) {
    printf("Error: Memory allocation failed for Pager.\n");
    exit(EXIT_FAILURE);
  }

  // Open file with POSIX open().
  // O_RDWR: Read/Write mode. O_CREAT: Create if it doesn't exist.
  // S_IWUSR | S_IRUSR: Sets user read/write permissions for the new file.
  pager->file_descriptor = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

  if (pager->file_descriptor == -1) {
    printf("Error: Unable to open database file.\n");
    free(pager);
    exit(EXIT_FAILURE);
  }

  // Calculate the length of the file by seeking to the end
  off_t file_length = lseek(pager->file_descriptor, 0, SEEK_END);
  if (file_length == -1) {
    printf("Error: Unable to seek to end of database file.\n");
    close(pager->file_descriptor);
    free(pager);
    exit(EXIT_FAILURE);
  }

  pager->file_length = file_length;

  // Check if the file length is a perfect multiple of our PAGE_SIZE (4096)
  if (file_length % PAGE_SIZE != 0) {
    printf("Warning: Database file size is not a multiple of PAGE_SIZE. "
           "Potential corruption.\n");
  }

  // Calculate how many pages currently exist on the disk
  pager->num_pages = (file_length / PAGE_SIZE);

  return pager;
}

// Read a specific 4KB page from the disk into the provided destination memory
// buffer
StatusCode pager_read(Pager *pager, page_id_t page_num, void *destination) {
  // Check if the requested page is out of bounds
  if (page_num >= pager->num_pages) {
    return STATUS_ERR_PAGE_NOT_FOUND;
  }

  // Jump exactly to the correct 4KB block on the hard drive
  off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
  if (offset == -1) {
    printf("Error: File seek failed during read operation.\n");
    return STATUS_ERR_UNKNOWN;
  }

  // Read exactly PAGE_SIZE bytes into our memory buffer
  ssize_t bytes_read = read(pager->file_descriptor, destination, PAGE_SIZE);

  if (bytes_read == -1) {
    printf("Error: Reading from disk failed. Errno: %d\n", errno);
    return STATUS_ERR_UNKNOWN;
  }

  return STATUS_SUCCESS;
}

// Write a 4KB memory buffer safely back to its spot on the disk
StatusCode pager_write(Pager *pager, page_id_t page_num, const void *source) {
  // Jump to the start of the required page
  off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
  if (offset == -1) {
    printf("Error: File seek failed during write operation.\n");
    return STATUS_ERR_UNKNOWN;
  }

  // Write the raw payload to the disk
  ssize_t bytes_written = write(pager->file_descriptor, source, PAGE_SIZE);

  if (bytes_written == -1) {
    printf("Error: Writing to disk failed. Errno: %d\n", errno);
    return STATUS_ERR_UNKNOWN;
  }

  // If we are writing a brand new page at the end of the file, update our
  // tracker
  if (page_num >= pager->num_pages) {
    pager->num_pages = page_num + 1;
    pager->file_length = pager->num_pages * PAGE_SIZE;
  }

  return STATUS_SUCCESS;
}

// Safely close the file handle and free the Pager memory
void pager_close(Pager *pager) {
  if (pager != NULL) {
    close(pager->file_descriptor);
    free(pager);
  }
}
