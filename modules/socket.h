/***********************************************
*
* @Proposito: Declara funciones y constantes necesarias para la 
*               creación, inicialización y gestión de sockets
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
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


