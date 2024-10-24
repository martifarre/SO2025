//Ignacio Giral ignacio.giral
//Marti Farre marti.farre

/***********************************************
*
* @Proposito: Implementació del procés Enigma
* @Autor/s: Ignacio Giral, Marti Farre (ignacio.giral, marti.farrea)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 17/10/2024
*
************************************************/
#define _GNU_SOURCE
#include "../utils/utils.h"

// Definición de la estructura EnigmaConfig para almacenar la configuración
typedef struct {
    char *gotham_server_ip;
    int gotham_server_port;
    char *enigma_server_ip;
    int enigma_server_port;
    char *directory;
    char *worker_type;
} EnigmaConfig;

// Variable global para almacenar la configuración
EnigmaConfig config;

/***********************************************
*
* @Finalidad: Liberar la memoria asignada dinámicamente para la configuración.
* @Parametros: Ninguno.
* @Retorno: Ninguno.
*
************************************************/
void free_config() {
    free(config.gotham_server_ip);
    free(config.enigma_server_ip);
    free(config.directory);
    free(config.worker_type);
}

/***********************************************
*
* @Finalidad: Leer el archivo de configuración y almacenar los valores en la estructura EnigmaConfig.
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

    config.gotham_server_ip = readUntil(fd, '\n');
    if (config.gotham_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Gotham server IP\n", 39);
        close(fd);
        exit(1);
    }
    replace(config.gotham_server_ip, '\r', '\0');

    char *port_str = readUntil(fd, '\n');
    if (port_str == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Gotham server port\n", 41);
        free_config();
        close(fd);
        exit(1);
    }
    config.gotham_server_port = atoi(port_str);
    free(port_str);

    config.enigma_server_ip = readUntil(fd, '\n');
    if (config.enigma_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Enigma server IP\n", 40);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.enigma_server_ip, '\r', '\0');

    port_str = readUntil(fd, '\n');
    if (port_str == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Enigma server port\n", 42);
        free_config();
        close(fd);
        exit(1);
    }
    config.enigma_server_port = atoi(port_str);
    free(port_str);

    config.directory = readUntil(fd, '\n');
    if (config.directory == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read directory\n", 32);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.directory, '\r', '\0');

    config.worker_type = readUntil(fd, '\n');
    if (config.worker_type == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read worker type\n", 35);
        free_config();
        close(fd);
        exit(1);
    } else { 
        replace(config.worker_type, '\r', '\0');
        if (strcmp(config.worker_type, "Text") != 0) {
            write(STDOUT_FILENO, "Error: Invalid worker type\n", 28);
            free_config();
            close(fd);
            exit(1);
        }
    }

    close(fd);
}

/***********************************************
*
* @Finalidad: Función principal del programa. Lee el archivo de configuración y muestra los detalles.
* @Parametros: int argc - Número de argumentos.
*              char *argv[] - Array de argumentos.
* @Retorno: int - Código de salida del programa.
*
************************************************/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: Enigma <config_file>\n", 28);
        exit(1);
    }

    read_config_file(argv[1]);

    char *msg;

    asprintf(&msg, "Enigma worker initialized\n");
    print_text(msg);
    free(msg);

    asprintf(&msg, "Gotham server IP: %s, Port: %d\n",
             config.gotham_server_ip, config.gotham_server_port);
    print_text(msg);
    free(msg);

    asprintf(&msg, "Enigma server IP: %s, Port: %d\n",
             config.enigma_server_ip, config.enigma_server_port);
    print_text(msg);
    free(msg);

    asprintf(&msg, "Directory: %s\n", config.directory);
    print_text(msg);
    free(msg);

    asprintf(&msg, "Worker type: %s\n", config.worker_type);
    print_text(msg);
    free(msg);

    free_config();

    return 0;
}