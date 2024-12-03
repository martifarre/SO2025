#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../linkedlist/linkedlist.h"

#define print_text(str) write(1, str, strlen(str))
#define print_error(str) write(2, str, strlen(str))

struct trama {
    uint8_t tipo;        // Campo de tipo (1 byte)
    uint16_t longitud;   // Campo de longitud (2 bytes)
    uint8_t *data;       // Puntero a los datos (variable, longitud dada por `longitud`)
    uint16_t checksum;   // Campo de checksum (2 bytes)
    uint32_t timestamp;  // Campo de timestamp (4 bytes)
};

char *readUntil(int fd, char cEnd);
bool read_line(int fd, int *read_bytes, char **buffer);
char *to_upper_case(char *str);
int count_words(char *str);
void strip_whitespace(char *str);
void replace(char *old_str, char old, char new);
char* getXFromMessage(const char* message, int x);    
uint16_t CONEXIONES_calculatar_checksum(const char *trama);
int readMessageFromSocket(int fd, struct trama *trama);
void sendMessageToSocket(int fd, char type, int16_t data_length, char *data);
int isSocketOpen(int sockfd);   
    

