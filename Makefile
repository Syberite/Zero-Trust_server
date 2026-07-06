# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Libraries required by KYL (OpenSSL and cURL)
LDFLAGS = -lcrypto -lcurl

# Explicitly list source files to avoid conflicts with old test files
SRCS = core_engine/src/pager.c \
       core_engine/src/buffer_pool.c \
       core_engine/src/bplus_tree.c \
       api_gateway/src/KYL_main.c

# Output executable name
TARGET = KYL

# Default target
all: $(TARGET)

# Compile the target
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "----------------------------------------"
	@echo "Build successful! Run with ./KYL"
	@echo "----------------------------------------"

# Clean up compiled binaries and database files
clean:
	rm -f $(TARGET) kyl_vault.db temp.txt
	@echo "Cleaned up executable and database files."
