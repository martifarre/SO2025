CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lpthread
DEPS = fleck/fleck.h utils/utils.h linkedlist/linkedlist.h

OBJ_FLECK = fleck/fleck.o utils/utils.o linkedlist/linkedlist.o
OBJ_ENIGMA = enigma/enigma.o utils/utils.o linkedlist/linkedlist.o
OBJ_GOTHAM = gotham/gotham.o utils/utils.o linkedlist/linkedlist.o
OBJ_HARLEY = harley/harley.o utils/utils.o linkedlist/linkedlist.o

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
    $(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BIN_ENIGMA): $(OBJ_ENIGMA)
    $(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BIN_GOTHAM): $(OBJ_GOTHAM)
    $(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BIN_HARLEY): $(OBJ_HARLEY)
    $(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
    rm -f $(BIN_FLECK) $(BIN_ENIGMA) $(BIN_GOTHAM) $(BIN_HARLEY) $(OBJ_FLECK) $(OBJ_ENIGMA) $(OBJ_GOTHAM) $(OBJ_HARLEY) linkedlist/linkedlist.o