STREAMING_CHUNK: Configuring C compiler and sources

CC = gcc
CFLAGS = -Wall -Wextra -g

SRCS = core_engine/src/pager.c 

core_engine/src/buffer_pool.c 

core_engine/src/bplus_tree.c 

core_engine/src/server.c

TARGET = KYL_SERVER

STREAMING_CHUNK: Defining build targets

all: $(TARGET)

$(TARGET):$(SRCS)
$(CC)$(CFLAGS) $(SRCS) -o$(TARGET)
@echo "----------------------------------------"
@echo "[+] Build successful! C Database Engine compiled."
@echo "----------------------------------------"

STREAMING_CHUNK: Defining run targets for DB and API

db: $(TARGET)
@echo "Starting Zero-Trust C Database Engine..."
./$(TARGET)

install:
pip3 install fastapi uvicorn pydantic

api:
@echo "Starting FastAPI Gateway..."
cd api_gateway && uvicorn app.main:app --reload

clean:
rm -f $(TARGET) kyl_vault.db
@echo "Cleaned up executable and database files."
