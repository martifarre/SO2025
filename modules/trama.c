#define _GNU_SOURCE
#include "trama.h"

uint16_t TRAMA_calculate_checksum(const char *trama) {
    uint32_t sum = 0;

    // Asumimos que la trama tiene 256 bytes
    for (int i = 0; i < 250; i++) { // Solo calculamos sobre los primeros 250 bytes
        sum += (unsigned char)trama[i];
    }

    // Reducimos la suma a 16 bits
    uint16_t checksum = (sum & 0xFFFF) + (sum >> 16);

    // Retornamos el complemento a uno del checksum
    return ~checksum;
}

int TRAMA_readMessageFromSocket(int sockfd, struct trama *trama) {
    char buffer[256];
    int checksum = 0;

    // Leer los datos del socket
    int bytes_leidos = read(sockfd, buffer, 256);
    if (bytes_leidos != 256) {
        perror("Error reading from socket, size was not 256 bytes");
        return -1;
    }

    // Asignar memoria para los datos de la trama
    trama->data = malloc(247);  // Reservar espacio para los datos
    if (!trama->data) {
        perror("Error allocating memory for trama->data");
        return -1;
    }

    // Extraer tipo y longitud de la trama
    trama->tipo = buffer[0];
    trama->longitud = ((unsigned char)buffer[1] << 8 | (unsigned char)buffer[2]);

    // Validar longitud de los datos
    if (trama->longitud > 247) {
        perror("Error: Invalid trama length");
        free(trama->data);  // Liberar memoria antes de salir
        trama->data = NULL;
        return -1;
    }

    // Copiar los datos a trama->data
    for (int i = 0; i < trama->longitud; i++) {
        trama->data[i] = buffer[3 + i];
    }

    // Si los datos son tratados como cadena, agregar terminador nulo
    trama->data[trama->longitud] = '\0';

    // Extraer checksum y timestamp
    trama->checksum = ((unsigned char)buffer[250] << 8 | (unsigned char)buffer[251]);
    trama->timestamp = ((unsigned char)buffer[252] << 24 |
                        (unsigned char)buffer[253] << 16 |
                        (unsigned char)buffer[254] << 8 |
                        (unsigned char)buffer[255]);

    // Calcular y validar el checksum
    checksum = TRAMA_calculate_checksum(buffer);
    if (checksum != trama->checksum) {
        perror("Error: Checksum validation failed");
        free(trama->data);  // Liberar memoria antes de salir
        trama->data = NULL;
        return -1;
    }

    return 1;  // Éxito
}


void TRAMA_sendMessageToSocket(int sockfd, char type, int16_t size, char *data) {
    char trama[256];
    memset(trama, '\0', 256);

    int timestamp = time(NULL);
    int checksum = 0;

    trama[0] = type;

    trama[1] = (size >> 8) & 0xFF;
    trama[2] = size & 0xFF;

    for (int i = 0; i < size; i++) {
        trama[3 + i] = data[i];
    }

    trama[252] = (timestamp >> 24) & 0xFF;
    trama[253] = (timestamp >> 16) & 0xFF;
    trama[254] = (timestamp >> 8) & 0xFF;
    trama[255] = timestamp & 0xFF;

    checksum = TRAMA_calculate_checksum(trama);

    trama[250] = (checksum >> 8) & 0xFF;
    trama[251] = checksum & 0xFF;

    write(sockfd, trama, 256);
}