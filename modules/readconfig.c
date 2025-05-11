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
#include "readconfig.h"
/**************************************************
 *
 * @Finalidad: Leer y parsear el fichero de configuración
 *             específico para el servidor Gotham.
 * @Parametros: in: config_file = ruta al fichero de configuración
 *                                (terminado en '\0') que contiene:
 *                                - IP y puerto para conexiones de Fleck
 *                                - IP y puerto para conexiones de Enigma/Harley
 * @Retorno:    Estructura GothamConfig con los campos inicializados
 *             (IPs y puertos) extraídos del fichero.
 *
 **************************************************/
GothamConfig READCONFIG_read_config_gotham(const char *config_file) {
    GothamConfig config;
    memset(&config, 0, sizeof(GothamConfig)); 
    int fd = open(config_file, O_RDONLY);
    
    if (fd == -1) {
        write(STDOUT_FILENO, "Error: Cannot open configuration file\n", 39);
        exit(1);
    }

    config.fleck_server_ip = STRING_readUntil(fd, '\n');
    STRING_replace(config.fleck_server_ip, '\r', '\0');
    if (config.fleck_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Fleck server IP\n", 39);
        close(fd);
        exit(1);
    }

    config.fleck_server_port = STRING_readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.fleck_server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Fleck server port\n", 41);
        close(fd);
        exit(1);
    }
    STRING_replace(config.fleck_server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    config.external_server_ip = STRING_readUntil(fd, '\n');
    STRING_replace(config.external_server_ip, '\r', '\0');
    if (config.external_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Worker server IP\n", 40);
        close(fd);
        exit(1);
    }

    config.external_server_port = STRING_readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.external_server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Worker server port\n", 42);
        close(fd);
        exit(1);
    }
    STRING_replace(config.external_server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    close(fd);
    return config;
}
/**************************************************
 *
 * @Finalidad: Leer y parsear el fichero de configuración
 *             para un proceso trabajador (Enigma o Harley),
 *             obteniendo los parámetros necesarios para:
 *             - Conectarse a Gotham (IP y puerto).
 *             - Abrir su propio socket de escucha (IP y puerto).
 *             - Definir el directorio de trabajo donde guardar archivos.
 *             - Identificar su tipo de worker (Media o Text).
 * @Parametros: in: config_file = ruta al fichero de configuración
 *                                 que contiene, en este orden:
 *                                1. IP de Gotham
 *                                2. Puerto de Gotham
 *                                3. IP de escucha del worker
 *                                4. Puerto de escucha del worker
 *                                5. Ruta de la carpeta de trabajo
 *                                6. Tipo de worker (Media o Text)
 * @Retorno:    Estructura WorkerConfig con todos los campos inicializados
 *             según el contenido del fichero.
 *
 **************************************************/
WorkerConfig READCONFIG_read_config_worker(const char *config_file) {
    WorkerConfig config;
    memset(&config, 0, sizeof(WorkerConfig));
    int fd = open(config_file, O_RDONLY);
    
    if (fd == -1) {
        write(STDOUT_FILENO, "Error: Cannot open configuration file\n", 39);
        exit(1);
    }

    config.gotham_server_ip = STRING_readUntil(fd, '\n');
    if (config.gotham_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Gotham server IP\n", 39);
        close(fd);
        exit(1);
    }
    STRING_replace(config.gotham_server_ip, '\r', '\0');

    config.gotham_server_port = STRING_readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.gotham_server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Gotham server port\n", 41);
        close(fd);
        exit(1);
    }
    STRING_replace(config.gotham_server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    config.worker_server_ip = STRING_readUntil(fd, '\n');
    if (config.worker_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Worker server IP\n", 40);
        close(fd);
        exit(1);
    }
    STRING_replace(config.worker_server_ip, '\r', '\0');

    config.worker_server_port = STRING_readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.worker_server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Worker server port\n", 42);
        close(fd);
        exit(1);
    }
    STRING_replace(config.worker_server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    config.directory = STRING_readUntil(fd, '\n');
    if (config.directory == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read directory\n", 32);
        close(fd);
        exit(1);
    }
    STRING_replace(config.directory, '\r', '\0');

    config.worker_type = STRING_readUntil(fd, '\n');
    if (config.worker_type == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read worker type\n", 35);
        close(fd);
        exit(1);
    } else {
        STRING_replace(config.worker_type, '\r', '\0');
        if(strcmp(config.worker_type, "Media") != 0 && strcmp(config.worker_type, "Text") != 0) {
            write(STDOUT_FILENO, "Error: Invalid worker type\n", 28);
            close(fd);
            exit(1);
        }
    }

    close(fd);
    return config;
}
/**************************************************
 *
 * @Finalidad: Leer y parsear el fichero de configuración
 *             para un proceso cliente Fleck, obteniendo los parámetros necesarios para:
 *             - Identificar el nombre de usuario.
 *             - Determinar la carpeta de trabajo del usuario.
 *             - Conectar al servidor Gotham (IP y puerto).
 * @Parametros: in: config_file = ruta al fichero de configuración
 *                                que contiene, en este orden:
 *                                1. Nombre de usuario
 *                                2. Ruta de la carpeta de usuario
 *                                3. IP del servidor Gotham
 *                                4. Puerto del servidor Gotham
 * @Retorno:    Estructura FleckConfig con todos los campos inicializados
 *             según el contenido del fichero.
 **************************************************/
FleckConfig READCONFIG_read_config_fleck(const char *config_file) {
    FleckConfig config;
    memset(&config, 0, sizeof(FleckConfig));
    int fd = open(config_file, O_RDONLY);
    if (fd == -1) {
        write(STDOUT_FILENO, "Error: Cannot open configuration file\n", 39);
        exit(1);
    }

    config.username = STRING_readUntil(fd, '\n');
    if (strchr(config.username, '$') != NULL) {
        write(STDOUT_FILENO, "Error: Invalid character '$' in username\n", 41);
        close(fd);
        exit(1);
    }
    STRING_replace(config.username, '\r', '\0');
    STRING_remove_char(config.username, '&');
    
    if (config.username == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read username\n", 31);
        close(fd);
        exit(1);
    }

    config.directory = STRING_readUntil(fd, '\n');
    STRING_replace(config.directory, '\r', '\0');
    if (config.directory == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read directory\n", 32);
        close(fd);
        exit(1);
    }

    config.server_ip = STRING_readUntil(fd, '\n');
    STRING_replace(config.server_ip, '\r', '\0');
    if (config.server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read server IP\n", 33);
        close(fd);
        exit(1);
    }

    config.server_port = STRING_readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read server port\n", 34);
        close(fd);
        exit(1);
    }
    STRING_replace(config.server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    close(fd);
    return config;
}
