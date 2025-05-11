/***********************************************
*
* @Proposito:  Implementación de funciones para leer y
*               gestionar fichero de configuración
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
#define _GNU_SOURCE
#include "files.h"

/**************************************************
 *
 * @Finalidad: Verificar si un nombre de fichero termina
 *             con alguna de las extensiones especificadas.
 * @Parametros: in: filename   = puntero a la cadena
 *                              con el nombre del fichero a comprobar.
 *              in: extensions = array de cadenas
 *                              con las extensiones válidas.
 * @Retorno:    1 si el nombre de fichero coincide con alguna de las
 *             extensiones del array; 0 en caso contrario.
 *
 **************************************************/
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

/**************************************************
 *
 * @Finalidad: Listar en pantalla los ficheros contenidos
 *             en un directorio y mostrarlos.
 * @Parametros: in: directory = ruta al directorio
 *                              cuyas entradas se desean listar.
 *              in: label     = cadena que precede cada nombre de fichero
 *                              al imprimir Media o texto.
 * @Retorno:    ---
 *
 **************************************************/
void FILES_list_files(const char *directory, const char *label) {
    if (!directory || !label) return; // Validación de entrada

    const char *media_extensions[] = {".wav", ".png", ".jpg", ".jpeg", ".bmp", ".tga", NULL};
    const char *text_extensions[] = {".txt", NULL};

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
    write(STDOUT_FILENO, buffer, strlen(buffer));
    free(buffer);

    for (int i = 0; i < count; i++) {
        asprintf(&buffer, "%d. %s\n", i + 1, file_list[i]);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);
        free(file_list[i]);
    }
    free(file_list);
}


/**************************************************
 *
 * @Finalidad: Buscar en un directorio si existe un fichero
 *             con el nombre dado y una de las extensiones válidas,
 *             devolviendo la ruta completa si se encuentra.
 * @Parametros: in: directory = ruta al directorio donde buscar los ficheros.
 *              in: file_name = nombre base del fichero (sin ruta).
 * @Retorno:    "Media" si el fichero es de tipo media,
 *             "Text" si es de tipo texto,
 *             "Neither" si no se encuentra el fichero o no tiene una extensión válida.
 *
 **************************************************/
char* FILES_file_exists_with_type(const char *directory, const char *file_name) {
    if (!directory || !file_name) return "Neither"; // Validación de entrada

    const char *media_extensions[] = {".wav", ".png", ".jpg", ".jpeg", ".bmp", ".tga", NULL};
    const char *text_extensions[] = {".txt", NULL};

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

/**************************************************
 *
 * @Finalidad: Obtener el tamaño, en bytes, de un fichero
 *             y devolverlo como cadena de caracteres.
 * @Parametros: in: path = puntero a la cadena 
 *                     que contiene la ruta al fichero.
 * @Retorno:    Puntero a una cadena dinámica (char*) que contiene
 *             el tamaño del fichero.
 *             Devuelve NULL si ocurre un error al acceder al fichero
 *             o al asignar memoria.
 *
 **************************************************/
char* FILES_get_size_of_file(char* path) {
    int fileDescriptor = open(path, O_RDONLY);
    if (fileDescriptor < 0) {
        write(STDERR_FILENO, "Error: Cannot open file\n", 25);
        return NULL;
    }
    
    off_t sizeOfFile = lseek(fileDescriptor, 0, SEEK_END);
    close(fileDescriptor);

    char *resultString = (char*)malloc(sizeof(char) * 30);
    if (!resultString) {
        perror("Failed to allocate memory \n");
        return NULL;
    }
    sprintf(resultString, "%ld", (long)sizeOfFile);

    return resultString;
}

