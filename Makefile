--- C DATABASE ENGINE COMMANDS ---

CC = gcc
CFLAGS = -Wall -Wextra -g

Source files for the C Server

SRCS = core_engine/src/pager.c 

core_engine/src/buffer_pool.c 

core_engine/src/bplus_tree.c 

core_engine/src/server.c

TARGET = KYL_SERVER

Default target: Compile the C database

all: $(TARGET)

$(TARGET):$(SRCS)
$(CC)$(CFLAGS) $(SRCS) -o$(TARGET)
@echo "----------------------------------------"
@echo "[+] Build successful! C Database Engine compiled."
@echo "----------------------------------------"

Run the C Database Engine

db: $(TARGET)
@echo "Starting Zero-Trust C Database Engine..."
./$(TARGET)

--- PYTHON FASTAPI COMMANDS ---

Install Python dependencies

install:
pip3 install fastapi uvicorn pydantic

Run the Python Web API

api:
@echo "Starting FastAPI Gateway..."
uvicorn api_gateway.app.main:app --reload

--- UTILITIES ---

Clean up compiled binaries and database files

clean:
rm -f $(TARGET) kyl_vault.db vault.db
@echo "Cleaned up executable and database files."
