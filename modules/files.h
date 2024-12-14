#ifndef FILES_H
#define FILES_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

// Funciones de manejo de archivos
int FILES_has_extension(const char *filename, const char **extensions);
void FILES_list_files(const char *directory, const char *label);
char* FILES_file_exists_with_type(const char *directory, const char *file_name);
char* FILES_get_size_of_file(char* path);

#endif // FILES_H
