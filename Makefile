CC = gcc
CFLAGS = -Wall
DEPS = fleck/fleck.h utils/utils.h

OBJ_FLECK = fleck/fleck.o utils/utils.o
OBJ_ENIGMA = enigma/enigma.o utils/utils.o
OBJ_GOTHAM = gotham/gotham.o utils/utils.o
OBJ_HARLEY = harley/harley.o utils/utils.o

BIN_DIR = bin
BIN_FLECK = $(BIN_DIR)/fleck
BIN_ENIGMA = $(BIN_DIR)/enigma
BIN_GOTHAM = $(BIN_DIR)/gotham
BIN_HARLEY = $(BIN_DIR)/harley

all: $(BIN_DIR) $(BIN_FLECK) $(BIN_ENIGMA) $(BIN_GOTHAM) $(BIN_HARLEY)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_FLECK): $(OBJ_FLECK)
	$(CC) -o $@ $^ $(CFLAGS)

$(BIN_ENIGMA): $(OBJ_ENIGMA)
	$(CC) -o $@ $^ $(CFLAGS)

$(BIN_GOTHAM): $(OBJ_GOTHAM)
	$(CC) -o $@ $^ $(CFLAGS)

$(BIN_HARLEY): $(OBJ_HARLEY)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN_FLECK) $(BIN_ENIGMA) $(BIN_GOTHAM) $(BIN_HARLEY) $(OBJ_FLECK) $(OBJ_ENIGMA) $(OBJ_GOTHAM) $(OBJ_HARLEY)
