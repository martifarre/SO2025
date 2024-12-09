#ifndef READCONFIG_H
#define READCONFIG_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "string.h" 

typedef struct {
    char *username;
    char *directory;
    char *server_ip;
    char *server_port;
} FleckConfig;

typedef struct {
    char *fleck_server_ip;
    char *fleck_server_port;
    char *external_server_ip;
    char *external_server_port;
} GothamConfig;

typedef struct {
    char *gotham_server_ip;
    char *gotham_server_port;
    char *worker_server_ip;
    char *worker_server_port;
    char *directory;
    char *worker_type;
} WorkerConfig;

GothamConfig READCONFIG_read_config_gotham(const char *config_file);
WorkerConfig READCONFIG_read_config_worker(const char *config_file);
FleckConfig READCONFIG_read_config_fleck(const char *config_file);

#endif