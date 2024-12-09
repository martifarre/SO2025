#define _GNU_SOURCE
#include "string.h"

int SOCKET_initSocket(char* incoming_Port, char* incoming_IP) {
    uint16_t port;
    int aux = atoi (incoming_Port);

    if (aux < 1 || aux > 65535) {
        write(STDOUT_FILENO, "Error: Invalid port\n", 21);
        exit (EXIT_FAILURE);
    }
    port = aux;
        

    int sockfd;
    sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        write(STDOUT_FILENO, "Error: Cannot create socket\n", 29);
        exit (EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    memset (&s_addr, '\0', sizeof (s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);

    int result = inet_pton(AF_INET, incoming_IP, &s_addr.sin_addr);
    if (result <= 0) {
        write(STDOUT_FILENO, "Error: Cannot convert IP address\n", 34);
        exit(EXIT_FAILURE);
    }

    if (bind (sockfd, (void *) &s_addr, sizeof (s_addr)) < 0) {
        write(STDOUT_FILENO, "Error: Cannot bind socket\n", 27);
        exit (EXIT_FAILURE);
    }

    if(listen (sockfd, 5) < 0) {
        write(STDOUT_FILENO, "Error: Cannot listen on socket\n", 31);
        exit (EXIT_FAILURE);
    }

    return sockfd;

}

int SOCKET_createSocket(char *incoming_Port, char* incoming_IP) {
    uint16_t port;
    int aux = atoi(incoming_Port);
    if (aux < 1 || aux > 65535) {
        write(STDOUT_FILENO, "Error: Invalid port\n", 21);
        exit (EXIT_FAILURE);

    }
    port = aux;

    struct in_addr ip_addr;
    if (inet_pton(AF_INET, incoming_IP, &ip_addr) != 1) {
        write(STDOUT_FILENO, "Error: Cannot convert IP address\n", 34);
        exit (EXIT_FAILURE);
    }

    int sockfd;
    sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        write(STDOUT_FILENO, "Error: Cannot create socket\n", 29);
        exit (EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    memset (&s_addr, '\0', sizeof (s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons (port);
    s_addr.sin_addr = ip_addr;

    if (connect (sockfd, (void *) &s_addr, sizeof (s_addr)) < 0) {
        write(STDOUT_FILENO, "Error: Cannot connect.\n", 24);
        sockfd = -1;
    }
     
    return sockfd;
}

int SOCKET_isSocketOpen(int sockfd) {
    char buffer[1];
    int flags = fcntl(sockfd, F_GETFL);
    if (flags == -1 && errno == EBADF) {
        return 0; // Descriptor no válido (socket cerrado)
    }

    // Intentar leer sin bloquear
    int result = recv(sockfd, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
    if (result == 0) {
        return 0; // Socket cerrado por el otro extremo
    } else if (result < 0 && (errno == ECONNRESET || errno == ENOTCONN)) {
        return 0; // Conexión reseteada o no conectada
    }

    return 1; // Socket abierto
}