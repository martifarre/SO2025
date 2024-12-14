#ifndef DISTORSION_H
#define DISTORSION_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "trama.h"
#include "string.h"
#include "files.h"
#include "distorsion.h"
#include "so_compression.h"
#include <errno.h>
#include <ctype.h>

#define NO_ERROR 0
#define ERROR_OPENING_FILE -1
#define ERROR_READING_FILE -2
#define ERROR_WRITING_FILE -3
#define ERROR_INVALID_LIMIT -4
#define ERROR_MEMORY_ALLOCATION -5

char* DISTORSION_getMD5SUM(const char* path);
void DISTORSION_distortFile(int sockfd, char* directory, char* MD5SUM, char* factor, char* fileSize, char* fileName, char* worker_type);  
int DISTORSION_compressText(char *input_file, int word_limit);

#endif // FILES_H