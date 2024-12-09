#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int SOCKET_initSocket(char *incoming_Port, char *incoming_IP);
int SOCKET_createSocket(char *incoming_Port, char *incoming_IP);
int SOCKET_isSocketOpen(int sockfd);

#endif // SOCKET_H


