CC = gcc
CFLAGS = -Wall
LDFLAGS = -lpthread

BIN_DIR = bin

# Archivos fuente individuales
SRCS_FLECK = fleck/fleck.c modules/string.c modules/trama.c modules/socket.c modules/files.c linkedlist/linkedlist.c modules/readconfig.c
SRCS_GOTHAM = gotham/gotham.c modules/string.c modules/trama.c modules/socket.c modules/files.c linkedlist/linkedlist.c modules/readconfig.c
SRCS_HARLEY = harley/harley.c modules/string.c modules/trama.c modules/socket.c modules/files.c linkedlist/linkedlist.c modules/readconfig.c
SRCS_ENIGMA = enigma/enigma.c modules/string.c modules/trama.c modules/socket.c modules/files.c linkedlist/linkedlist.c modules/readconfig.c
SRCS_WORKER = worker/worker.c modules/string.c modules/trama.c modules/socket.c modules/files.c linkedlist/linkedlist.c modules/readconfig.c

# Binarios
BIN_FLECK = $(BIN_DIR)/fleck
BIN_GOTHAM = $(BIN_DIR)/gotham
BIN_HARLEY = $(BIN_DIR)/harley
BIN_ENIGMA = $(BIN_DIR)/enigma
BIN_WORKER = $(BIN_DIR)/worker

all: $(BIN_DIR) $(BIN_FLECK) $(BIN_GOTHAM) $(BIN_HARLEY) $(BIN_ENIGMA) $(BIN_WORKER)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_FLECK): $(SRCS_FLECK)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BIN_GOTHAM): $(SRCS_GOTHAM)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BIN_HARLEY): $(SRCS_HARLEY)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BIN_ENIGMA): $(SRCS_ENIGMA)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BIN_WORKER): $(SRCS_WORKER)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf $(BIN_DIR)
