//Ignacio Giral ignacio.giral
//Marti Farre marti.farre

/***********************************************
*
* @Proposito: Implementació del procés Fleck
* @Autor/s: Ignacio Giral, Marti Farre (ignacio.giral, marti.farrea)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 17/10/2024
*
************************************************/
#define _GNU_SOURCE
#include "../utils/utils.h"
#include "fleck.h"

// Definición de la estructura FleckConfig para almacenar la configuración
typedef struct {
    char *username;
    char *directory;
    char *server_ip;
    int server_port;
} FleckConfig;

// Variable global para almacenar la configuración
FleckConfig config;
// Variable global para almacenar el comando leído
char *global_cmd = NULL;

/***********************************************
*
* @Finalidad: Liberar la memoria asignada dinámicamente para la configuración.
* @Parametros: Ninguno.
* @Retorno: Ninguno.
*
************************************************/
void free_config() {
    free(config.username);
    free(config.directory);
    free(config.server_ip);
}

/***********************************************
*
* @Finalidad: Leer el archivo de configuración y almacenar los valores en la estructura FleckConfig.
* @Parametros: const char *config_file - Ruta del archivo de configuración.
* @Retorno: Ninguno.
*
************************************************/
void read_config_file(const char *config_file) {
    int fd = open(config_file, O_RDONLY);
    if (fd == -1) {
        write(STDOUT_FILENO, "Error: Cannot open configuration file\n", 39);
        exit(1);
    }

    config.username = readUntil(fd, '\n');
    if (strchr(config.username, '$') != NULL) {
        write(STDOUT_FILENO, "Error: Invalid character '$' in username\n", 41);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.username, '\r', '\0');

    if (config.username == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read username\n", 31);
        free_config();
        close(fd);
        exit(1);
    }

    config.directory = readUntil(fd, '\n');
    replace(config.directory, '\r', '\0');
    if (config.directory == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read directory\n", 32);
        free_config();
        close(fd);
        exit(1);
    }

    config.server_ip = readUntil(fd, '\n');
    replace(config.server_ip, '\r', '\0');
    if (config.server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read server IP\n", 33);
        free_config();
        close(fd);
        exit(1);
    }

    char *port_str = readUntil(fd, '\n');
    if (port_str == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read server port\n", 34);
        free_config();
        close(fd);
        exit(1);
    }
    config.server_port = atoi(port_str);
    free(port_str);

    close(fd);
}

// Extensiones de archivos de medios y texto
const char *media_extensions[] = {".wav", ".png", ".jpg", ".jpeg", ".mp3", ".mp4", NULL};
const char *text_extensions[] = {".txt", ".dat", NULL};

/***********************************************
*
* @Finalidad: Verificar si un archivo tiene una de las extensiones especificadas.
* @Parametros: const char *filename - Nombre del archivo.
*              const char **extensions - Lista de extensiones.
* @Retorno: int - 1 si el archivo tiene una de las extensiones, 0 en caso contrario.
*
************************************************/
int has_extension(const char *filename, const char **extensions) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return 0;

    for (int i = 0; extensions[i] != NULL; i++) {
        if (strcasecmp(dot, extensions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/***********************************************
*
* @Finalidad: Listar archivos en un directorio basados en extensiones.
* @Parametros: const char *directory - Ruta del directorio.
*              const char **extensions - Lista de extensiones.
*              const char *label - Etiqueta para los archivos listados.
* @Retorno: Ninguno.
*
************************************************/
void list_files(const char *directory, const char **extensions, const char *label) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    char **file_list = NULL;
    char* buffer;

    dir = opendir(directory);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            continue;
        }
        if (has_extension(entry->d_name, extensions)) {
            file_list = realloc(file_list, sizeof(char *) * (count + 1));
            file_list[count] = strdup(entry->d_name);
            count++;
        }
    }
    closedir(dir);

    asprintf(&buffer, "There are %d %s available:\n", count, label);
    print_text(buffer);
    free(buffer);

    for (int i = 0; i < count; i++) {
        asprintf(&buffer, "%d. %s\n", i + 1, file_list[i]);
        print_text(buffer);
        free(buffer);

        free(file_list[i]); 
    }
    free(file_list);
}

/***********************************************
*
* @Finalidad: Leer un comando desde la entrada estándar.
* @Parametros: int *words - Puntero para almacenar el número de palabras en el comando.
* @Retorno: char* - El comando leído.
*
************************************************/
char *read_command(int *words) {
    int read_bytes;
    print_text("\n$ ");
    read_line(STDIN_FILENO, &read_bytes, &global_cmd);
    global_cmd = to_upper_case(global_cmd);
    replace(global_cmd, '\n', '\0');
    *words = count_words(global_cmd);
    strip_whitespace(global_cmd);
    return global_cmd;
}

/***********************************************
*
* @Finalidad: Función principal del terminal que procesa los comandos del usuario.
* @Parametros: Ninguno.
* @Retorno: Ninguno.
*
************************************************/
void terminal() {
    int words;

    while (1) {
        global_cmd = read_command(&words);

        if (strcmp(global_cmd, "CONNECT") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
        } else if (strcmp(global_cmd, "LIST MEDIA") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
            list_files(config.directory, media_extensions, "media files");
        } else if (strcmp(global_cmd, "LIST TEXT") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
            list_files(config.directory, text_extensions, "text files");
        } else if (strncmp(global_cmd, "DISTORT", 7) == 0 && words == 3) {
            write(STDOUT_FILENO, "Command OK\n", 12);
        } else if (strcmp(global_cmd, "CHECK STATUS") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
        } else if (strcmp(global_cmd, "CLEAR ALL") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
        } else if (strcmp(global_cmd, "LOGOUT") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
            free(global_cmd);
            global_cmd = NULL;
            break;
        } else {
            write(STDOUT_FILENO, "Unknown command\n", 17);
        }

        free(global_cmd);
        global_cmd = NULL;
    }
}

/***********************************************
*
* @Finalidad: Manejador de la señal CTRL+C para liberar recursos y salir del programa.
* @Parametros: int signum - Número de la señal.
* @Retorno: Ninguno.
*
************************************************/
void CTRLC(int signum) {
    print_text("\nInterrupt signal CTRL+C received\n");
    if (global_cmd != NULL) {
        free(global_cmd);
        global_cmd = NULL;
    }
    free_config();
    exit(1);
}

/***********************************************
*
* @Finalidad: Función principal del programa. Lee el archivo de configuración y ejecuta el terminal.
* @Parametros: int argc - Número de argumentos.
*              char *argv[] - Array de argumentos.
* @Retorno: int - Código de salida del programa.
*
************************************************/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: Fleck <config_file>\n", 27);
        exit(1);
    }

    signal(SIGINT, CTRLC);
    
    read_config_file(argv[1]);

    char* msg;

    asprintf(&msg, "%s user initialized\n", config.username);
    print_text(msg);
    free(msg);

    asprintf(&msg, "%s, %s, %s, %d\n", config.username, config.directory, config.server_ip, config.server_port);
    print_text(msg);
    free(msg);

    terminal();

    free_config();

    return 0;
}