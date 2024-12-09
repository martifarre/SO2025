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
#include <errno.h>

#define print_text(str) write(1, str, strlen(str))
#define print_error(str) write(2, str, strlen(str))

char *STRING_readUntil(int fd, char cEnd);
bool STRING_read_line(int fd, int *read_bytes, char **buffer);
char *STRING_to_upper_case(char *str);
int STRING_count_words(char *str);
void STRING_strip_whitespace(char *str);
void STRING_replace(char *old_str, char old, char new);
char* STRING_getXFromMessage(const char* message, int x);
char* STRING_extract_substring(char* global_cmd);
void STRING_to_lowercase(char* str); 
    

