# Compilador y banderas
CC = gcc
CFLAGS = -Wall -MMD -g
LDFLAGS = -lpthread -lm

# Directorio para los binarios
BIN_DIR = bin

# Archivos de objetos adicionales
SO_COMPRESSION_OBJ = modules/so_compression.o

# Archivos fuente individuales
SRCS_FLECK = fleck/fleck.c modules/string.c modules/trama.c modules/socket.c modules/files.c linkedlist/linkedlist.c linkedlist/linkedlist2.c modules/readconfig.c modules/distorsion.c
SRCS_GOTHAM = gotham/gotham.c modules/string.c modules/trama.c modules/socket.c modules/files.c linkedlist/linkedlist.c linkedlist/linkedlist2.c modules/readconfig.c modules/distorsion.c
SRCS_WORKER = worker/worker.c modules/string.c modules/trama.c modules/socket.c modules/files.c linkedlist/linkedlist.c linkedlist/linkedlist2.c modules/readconfig.c modules/distorsion.c

# Binarios
BIN_FLECK = $(BIN_DIR)/fleck
BIN_GOTHAM = $(BIN_DIR)/gotham
BIN_WORKER = $(BIN_DIR)/worker

# Objetivo principal
all: $(BIN_DIR) $(BIN_FLECK) $(BIN_GOTHAM) $(BIN_WORKER)

# Crear el directorio de binarios si no existe
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Reglas de compilación
$(BIN_FLECK): $(SRCS_FLECK) $(SO_COMPRESSION_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BIN_GOTHAM): $(SRCS_GOTHAM) $(SO_COMPRESSION_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BIN_WORKER): $(SRCS_WORKER) $(SO_COMPRESSION_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

# Incluye dependencias generadas automáticamente
-include $(SRCS_FLECK:.c=.d) $(SRCS_GOTHAM:.c=.d) $(SRCS_WORKER:.c=.d)

# Limpieza
.PHONY: clean
clean:
	rm -rf $(BIN_DIR) *.o *.d

g: 
	cd bin && valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes -s ./gotham ../config/gotham.dat
g2: 
	cd bin && valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes -s ./gotham ../config/gotham2.dat
f: 
	cd bin && valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes -s ./fleck ../config/fleck.dat

e1: 
	cd bin && valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes -s ./worker ../config/enigma.dat

e2:
	cd bin && valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes -s ./worker ../config/enigma2.dat

h1:
	cd bin && valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes -s ./worker ../config/harley.dat

h2:
	cd bin && valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes -s ./worker ../config/harley2.dat

sg:
	cd bin && ./gotham ../config/gotham.dat

sg2:
	cd bin && ./gotham ../config/gotham2.dat

sf:
	cd bin && ./fleck ../config/fleck.dat

se1:
	cd bin && ./worker ../config/enigma.dat

se2:
	cd bin && ./worker ../config/enigma2.dat

sh1:
	cd bin && ./worker ../config/harley.dat

sh2:
	cd bin && ./worker ../config/harley2.dat