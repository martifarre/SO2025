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
#include <sys/ipc.h>
#include "../linkedlist/linkedlist2.h"
#include <sys/msg.h>

#define NO_ERROR 0
#define ERROR_OPENING_FILE -1
#define ERROR_READING_FILE -2
#define ERROR_WRITING_FILE -3
#define ERROR_INVALID_LIMIT -4
#define ERROR_MEMORY_ALLOCATION -5

char* DISTORSION_getMD5SUM(const char* path);
int DISTORSION_distortFile(listElement2* element, volatile sig_atomic_t *stop_signal);
int DISTORSION_compressText(char *input_file, int word_limit);

#endif // FILES_H