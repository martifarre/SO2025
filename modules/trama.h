#ifndef TRAMA_H
#define TRAMA_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

struct trama {
    uint8_t tipo;        // Campo de tipo (1 byte)
    uint16_t longitud;   // Campo de longitud (2 bytes)
    uint8_t *data;       // Puntero a los datos (variable, longitud dada por `longitud`)
    uint16_t checksum;   // Campo de checksum (2 bytes)
    uint32_t timestamp;  // Campo de timestamp (4 bytes)
};

uint16_t TRAMA_calculate_checksum(const char *trama);
int TRAMA_readMessageFromSocket(int fd, struct trama *trama);
void TRAMA_sendMessageToSocket(int fd, char type, int16_t data_length, char *data);

#endif // TRAMA_H
