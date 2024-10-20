//Ignacio Giral ignacio.giral
//Marti Farre marti.farre

/***********************************************
*
* @Proposito: Implementació del procés Harley
* @Autor/s: Ignacio Giral, Marti Farre (ignacio.giral, marti.farrea)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 17/10/2024
*
************************************************/
#define _GNU_SOURCE
#include "../utils/utils.h"

// Definición de la estructura HarleyConfig para almacenar la configuración
typedef struct {
    char *gotham_server_ip;
    int gotham_server_port;
    char *harley_server_ip;
    int harley_server_port;
    char *directory;
    char *worker_type;
} HarleyConfig;

// Variable global para almacenar la configuración
HarleyConfig config;

/***********************************************
*
* @Finalidad: Liberar la memoria asignada dinámicamente para la configuración.
* @Parametros: Ninguno.
* @Retorno: Ninguno.
*
************************************************/
void free_config() {
    // Liberar la memoria asignada para cada campo de la configuración
    free(config.gotham_server_ip);
    free(config.harley_server_ip);
    free(config.directory);
    free(config.worker_type);
}

/***********************************************
*
* @Finalidad: Leer el archivo de configuración y almacenar los valores en la estructura HarleyConfig.
* @Parametros: const char *config_file - Ruta del archivo de configuración.
* @Retorno: Ninguno.
*
************************************************/
void read_config_file(const char *config_file) {
    // Abrir el archivo de configuración en modo solo lectura
    int fd = open(config_file, O_RDONLY);
    
    // Error al abrir el archivo de configuración
    if (fd == -1) {
        write(STDOUT_FILENO, "Error: Cannot open configuration file\n", 39);
        exit(1);
    }

    // Leer la IP del servidor Gotham
    config.gotham_server_ip = readUntil(fd, '\n');
    if (config.gotham_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Gotham server IP\n", 39);
        close(fd);
        exit(1);
    }
    replace(config.gotham_server_ip, '\r', '\0');

    // Leer el puerto del servidor Gotham
    char *port_str = readUntil(fd, '\n');
    if (port_str == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Gotham server port\n", 41);
        free_config();
        close(fd);
        exit(1);
    }
    config.gotham_server_port = atoi(port_str);
    free(port_str);

    // Leer la IP del servidor Harley
    config.harley_server_ip = readUntil(fd, '\n');
    if (config.harley_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Harley server IP\n", 40);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.harley_server_ip, '\r', '\0');

    // Leer el puerto del servidor Harley
    port_str = readUntil(fd, '\n');
    if (port_str == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Harley server port\n", 42);
        free_config();
        close(fd);
        exit(1);
    }
    config.harley_server_port = atoi(port_str);
    free(port_str);

    // Leer el directorio
    config.directory = readUntil(fd, '\n');
    if (config.directory == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read directory\n", 32);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.directory, '\r', '\0');

    // Leer el tipo de trabajador
    config.worker_type = readUntil(fd, '\n');
    if (config.worker_type == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read worker type\n", 35);
        free_config();
        close(fd);
        exit(1);
    } else {
        replace(config.worker_type, '\r', '\0');
        if(strcmp(config.worker_type, "Media") != 0) {
            write(STDOUT_FILENO, "Error: Invalid worker type\n", 28);
            free_config();
            close(fd);
            exit(1);
        }
    }

    // Cerrar el archivo de configuración
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
        // Uso incorrecto del programa
        write(STDOUT_FILENO, "Usage: Harley <config_file>\n", 28);
        exit(1);
    }

    // Leer el archivo de configuración
    read_config_file(argv[1]);

    char *msg;

    // Imprimir mensaje de inicialización
    asprintf(&msg, "Harley worker initialized\n");
    print_text(msg);
    free(msg);

    // Imprimir detalles del servidor Gotham
    asprintf(&msg, "Gotham server IP: %s, Port: %d\n",
             config.gotham_server_ip, config.gotham_server_port);
    print_text(msg);
    free(msg);

    // Imprimir detalles del servidor Harley
    asprintf(&msg, "Harley server IP: %s, Port: %d\n",
             config.harley_server_ip, config.harley_server_port);
    print_text(msg);
    free(msg);

    // Imprimir el directorio
    asprintf(&msg, "Directory: %s\n", config.directory);
    print_text(msg);
    free(msg);

    // Imprimir el tipo de trabajador
    asprintf(&msg, "Worker type: %s\n", config.worker_type);
    print_text(msg);
    free(msg);

    // Liberar la memoria asignada
    free_config();

    return 0;
}