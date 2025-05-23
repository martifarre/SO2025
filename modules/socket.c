/***********************************************
*
* @Proposito:  Implementa funciones para la creación, inicialización,
*                conexión y verificación del estado de sockets
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
#define _GNU_SOURCE
#include "string.h"
/**************************************************
 *
 * @Finalidad: Inicializar un socket TCP en modo servidor y enlazarlo
 *             a la dirección IP y puerto especificados para aceptar conexiones entrantes.
 * @Parametros: in: incoming_Port = cadena con el número de puerto donde escuchar.
 *              in: incoming_IP   = cadena con la dirección IP en la que bindear el socket.
 * @Retorno:    Descriptor de archivo del socket en modo escucha (>=0) en caso de éxito;
 *              -1 si ocurre un error al crear o enlazar el socket.
 *
 **************************************************/
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

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        write(STDOUT_FILENO, "Error: Cannot set SO_REUSEADDR\n", 31);
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
/**************************************************
 *
 * @Finalidad: Crear un socket TCP en modo cliente y conectarlo
 *             al servidor especificado por dirección IP y puerto.
 * @Parametros: in: incoming_Port = cadena con el número de puerto
 *                                al que se desea conectar.
 *              in: incoming_IP   = cadena con la dirección IP
 *                                del servidor de destino.
 * @Retorno:    Descriptor de archivo del socket conectado (>= 0)
 *             en caso de éxito; -1 si ocurre un error al crear
 *             o conectar el socket.
 *
 **************************************************/
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
/**************************************************
 *
 * @Finalidad: Comprueba si el descriptor de socket dado sigue abierto
 *             y es válido para operaciones de lectura/escritura.
 * @Parametros: in: sockfd = descriptor de archivo del socket a verificar.
 * @Retorno:    >0  si el socket está abierto y listo para usar.
 *             ==0 si el socket está cerrado o la conexión se ha cerrado.
 *             <0  en caso de error al comprobar el estado del socket.
 *
 **************************************************/
int SOCKET_isSocketOpen(int sockfd) {
    char buffer[1];
    int flags = fcntl(sockfd, F_GETFL);
    if (flags == -1) {
        if (errno == EBADF) {
            return 0; // Descriptor no válido (cerrado)
        }
        // Otro error 
        return 0;
    }

    int result = recv(sockfd, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
    if (result == 0) {
        return 0; // El otro extremo cerró la conexión
    } else if (result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 1; // No hay datos, pero la conexión sigue viva
        }
        // Otros errores => conexión rota
        return 0;
    }

    return 1; // Todo bien, conexión viva
}