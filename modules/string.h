/***********************************************
*
* @Proposito: Declaracion de funciones para manipulación y procesamiento
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
#ifndef STRING_H
#define STRING_H

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
char* STRING_get_third_word(const char* input);
char* STRING_getSongCode(const char* message, int length);
void STRING_remove_char(char *str, char ch);
    
#endif // STRING_H

