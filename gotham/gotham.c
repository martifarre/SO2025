//Ignacio Giral ignacio.giral
//Marti Farre marti.farre

/***********************************************
*
* @Proposito: Implementació del procés Gotham
* @Autor/s: Ignacio Giral, Marti Farre (ignacio.giral, marti.farrea)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 17/10/2024
*
************************************************/
#define _GNU_SOURCE
#include "../utils/utils.h"

// Definición de la estructura GothamConfig para almacenar la configuración
typedef struct {
    char *fleck_server_ip;
    int fleck_server_port;
    char *external_server_ip;
    int external_server_port;
} GothamConfig;

// Variable global para almacenar la configuración
GothamConfig config;

/***********************************************
*
* @Finalidad: Liberar la memoria asignada dinámicamente para la configuración.
* @Parametros: Ninguno.
* @Retorno: Ninguno.
*
************************************************/
void free_config() {
    free(config.fleck_server_ip);
    free(config.external_server_ip);
}

/***********************************************
*
* @Finalidad: Leer el archivo de configuración y almacenar los valores en la estructura GothamConfig.
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

    config.fleck_server_ip = readUntil(fd, '\n');
    replace(config.fleck_server_ip, '\r', '\0');
    if (config.fleck_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Fleck server IP\n", 39);
        close(fd);
        exit(1);
    }

    char *port_str = readUntil(fd, '\n');
    if (port_str == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Fleck server port\n", 41);
        free_config();
        close(fd);
        exit(1);
    }
    config.fleck_server_port = atoi(port_str);
    free(port_str);

    config.external_server_ip = readUntil(fd, '\n');
    replace(config.external_server_ip, '\r', '\0');
    if (config.external_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Harley server IP\n", 40);
        free_config();
        close(fd);
        exit(1);
    }

    port_str = readUntil(fd, '\n');
    if (port_str == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Harley server port\n", 42);
        free_config();
        close(fd);
        exit(1);
    }
    config.external_server_port = atoi(port_str);
    free(port_str);

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
        write(STDOUT_FILENO, "Usage: Gotham <config_file>\n", 28);
        exit(1);
    }

    read_config_file(argv[1]);

    char *msg;

    asprintf(&msg, "Gotham server initialized\n");
    print_text(msg);
    free(msg);

    asprintf(&msg, "Fleck server IP: %s, Port: %d\n",
             config.fleck_server_ip, config.fleck_server_port);
    print_text(msg);
    free(msg);

    asprintf(&msg, "Harley server IP: %s, Port: %d\n",
             config.external_server_ip, config.external_server_port);
    print_text(msg);
    free(msg);

    free_config();

    return 0;
}