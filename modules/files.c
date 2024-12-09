#define _GNU_SOURCE
#include "files.h"

// Función para comprobar la extensión
int FILES_has_extension(const char *filename, const char **extensions) {
    if (!filename || !extensions) return 0; // Validación de entrada

    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return 0;

    for (int i = 0; extensions[i] != NULL; i++) {
        if (strcasecmp(dot, extensions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Listar archivos con extensiones específicas
void FILES_list_files(const char *directory, const char *label) {
    if (!directory || !label) return; // Validación de entrada

    const char *media_extensions[] = {".wav", ".png", ".jpg", ".jpeg", ".mp3", ".mp4", NULL};
    const char *text_extensions[] = {".txt", ".dat", NULL};

    const char **extensions = (strcasecmp(label, "media files") == 0) ? media_extensions : text_extensions;

    DIR *dir;
    struct dirent *entry;
    int count = 0;
    char **file_list = NULL;
    char *buffer;
    
    dir = opendir(directory);
    if (dir == NULL) {
        fprintf(stderr, "Error: Cannot open directory %s\n", directory);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) continue;
        if (FILES_has_extension(entry->d_name, extensions)) {
            file_list = realloc(file_list, sizeof(char *) * (count + 1));
            file_list[count] = strdup(entry->d_name);
            count++;
        }
    }
    closedir(dir);

    asprintf(&buffer, "There are %d %s available:\n", count, label);
    printf("%s", buffer);
    free(buffer);

    for (int i = 0; i < count; i++) {
        asprintf(&buffer, "%d. %s\n", i + 1, file_list[i]);
        printf("%s", buffer);
        free(buffer);
        free(file_list[i]);
    }
    free(file_list);
}

// Comprobar si un archivo existe y determinar su tipo
char* FILES_file_exists_with_type(const char *directory, const char *file_name) {
    if (!directory || !file_name) return "Neither"; // Validación de entrada

    const char *media_extensions[] = {".wav", ".png", ".jpg", ".jpeg", ".mp3", ".mp4", NULL};
    const char *text_extensions[] = {".txt", ".dat", NULL};

    DIR *dir;
    struct dirent *entry;
    char *result = "Neither";

    dir = opendir(directory);
    if (dir == NULL) {
        fprintf(stderr, "Error: Cannot open directory %s\n", directory);
        return result;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) continue;
        if (strcmp(entry->d_name, file_name) == 0) {
            if (FILES_has_extension(file_name, media_extensions)) {
                result = "Media";
            } else if (FILES_has_extension(file_name, text_extensions)) {
                result = "Text";
            }
            break;
        }
    }
    closedir(dir);
    return result;
}
