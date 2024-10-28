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

char *readUntil(int fd, char cEnd);
bool read_line(int fd, int *read_bytes, char **buffer);
char *to_upper_case(char *str);
int count_words(char *str);
void strip_whitespace(char *str);
void replace(char *old_str, char old, char new);
char* getXFromMessage(const char* message, int x);    
uint16_t calculate_checksum(char *message, int length);
int validate_checksum(char *message);
char *buildMessage(char *type, char *data);
void sendMessageToSocket(int socket, char *type, char *data);

    

