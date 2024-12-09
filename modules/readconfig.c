#define _GNU_SOURCE
#include "readconfig.h"

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
