//Ignacio Giral ignacio.giral
//Marti Farre marti.farre

/***********************************************
*
* @Proposito: Implementació del procés Worker
* @Autor/s: Ignacio Giral, Marti Farre (ignacio.giral, marti.farrea)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 17/10/2024
*
************************************************/
#define _GNU_SOURCE
#include "../modules/project.h"

// Variable global para almacenar la configuración
WorkerConfig config;

int fleckSock = -1, sockfd = -1;
/***********************************************
*
* @Finalidad: Liberar la memoria asignada dinámicamente para la configuración.
* @Parametros: Ninguno.
* @Retorno: Ninguno.
*
************************************************/
void free_config() {
    free(config.gotham_server_ip);
    free(config.gotham_server_port); // Liberar la memoria del puerto
    free(config.worker_server_ip);
    free(config.worker_server_port); // Liberar la memoria del puerto
    free(config.directory);
    free(config.worker_type);
}

void loopSendFileDistorted(int sockfd, char* filename) {
    char* data = (char*)malloc(sizeof(char) * 256);
    if (!data) {
        perror("Error allocating memory for data");
        return;
    }

    int amount = 10;
    sprintf(data, "%d&MD5SUM", amount);
    TRAMA_sendMessageToSocket(sockfd, 0x04, (int16_t)strlen(data), data);
    free(data);

    struct trama wtrama;
    wtrama.data = NULL;  // Inicializar

    // Primera lectura
    if (TRAMA_readMessageFromSocket(sockfd, &wtrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        if (wtrama.data != NULL) {
            free(wtrama.data);
            wtrama.data = NULL;
        }
        return;
    }
    free(wtrama.data);  
    wtrama.data = NULL;

    for (int i = 0; i < amount; i++) {
        TRAMA_sendMessageToSocket(sockfd, 0x05, (int16_t)strlen("FileChunk"), "FileChunk");

        // Liberar la memoria asignada anteriormente antes de sobrescribir
        if (wtrama.data != NULL) {
            free(wtrama.data);
            wtrama.data = NULL;
        }

        if (TRAMA_readMessageFromSocket(sockfd, &wtrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            if (wtrama.data != NULL) {
                free(wtrama.data);
                wtrama.data = NULL;
            }
            return;
        }

        if (wtrama.tipo == 0x07) {
            write(STDOUT_FILENO, "Worker received a CTRL+C in the middle of the distortion.\n", 59);
            if (wtrama.data != NULL) {
                free(wtrama.data);
                wtrama.data = NULL;
            }
            return;
        }
    }

    // Liberación final
    if (wtrama.data != NULL) {
        free(wtrama.data);
        wtrama.data = NULL;
    }

    TRAMA_sendMessageToSocket(sockfd, 0x05, (int16_t)strlen("Done"), "Done");
    write(STDOUT_FILENO, "\nFile distorted sent correctly.\n\n", 34);
}

void initServer() {
    int socket_fd = SOCKET_initSocket(config.worker_server_port, config.worker_server_ip);
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    char data[256]; // Uso de memoria en la pila
    while (1) {
        memset(data, '\0', 256);

        fleckSock = accept(socket_fd, (void *)&c_addr, &c_len);
        if (fleckSock < 0) {
            write(STDOUT_FILENO, "Error: Cannot accept connection\n", 33);
            exit(EXIT_FAILURE);
        }

        struct trama wtrama;
        if (TRAMA_readMessageFromSocket(fleckSock, &wtrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            free(wtrama.data);
            close(fleckSock);
            fleckSock = -1;
            continue; // Saltar al siguiente ciclo del bucle
        }

        char* extracted = STRING_getXFromMessage((const char *)wtrama.data, 0);
        char* extracted2 = STRING_getXFromMessage((const char *)wtrama.data, 1);
        sprintf(data, "Fleck username: %s, File to distort: %s", extracted, extracted2);
        write(STDOUT_FILENO, data, strlen(data));

        TRAMA_sendMessageToSocket(fleckSock, 0x03, 0, ""); // Indicar que se puede empezar a enviar el archivo.
        // TRAMA_sendMessageToSocket(fleckSock, 0x03, (int16_t)strlen("CON_KO"), "CON_KO"); // Error, no se puede enviar archivo.

        loopSendFileDistorted(fleckSock, extracted2);
        free(extracted);
        free(extracted2);
        free(wtrama.data);
        close(fleckSock);
        fleckSock = -1;
    }
}

void doLogout() {
    if(SOCKET_isSocketOpen(fleckSock)) {
        TRAMA_sendMessageToSocket(fleckSock, 0x07, (int16_t)strlen("CON_KO"), "CON_KO"); 
        close(fleckSock);
    }

    write(STDOUT_FILENO, "Sending logout message to Gotham server...\n", 43);
    TRAMA_sendMessageToSocket(sockfd, 0x07, (int16_t)strlen(config.worker_type),config.worker_type);
    close(sockfd);
}

void CTRLC(int signum) {
    print_text("\nInterrupt signal CTRL+C received\n");
    doLogout();
    free_config();
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void *connection_watcher() {
    while (1) {
        if (!SOCKET_isSocketOpen(sockfd)) {
            write(STDOUT_FILENO, "Connection to Gotham lost\n", 27);
            while(1) {
                if(!SOCKET_isSocketOpen(fleckSock)) {
                    free_config();
                    exit(0);
                }
            }
        }
        sleep(5); // Verifica cada 5 segundos
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: Worker <config_file>\n", 28);
        exit(1);
    }

    signal(SIGINT, CTRLC);

    char *data = (char *)malloc(sizeof(char) * 256);

    config = READCONFIG_read_config_worker(argv[1]);
    
    write(STDOUT_FILENO, "\nWorker initialized\n\n", 22);

    sprintf(data, "Connecting %s Server to the Gotham system...\n\n", config.directory);
    write(STDOUT_FILENO, data, strlen(data));
    memset(data, '\0', 256);

    sockfd = SOCKET_createSocket(config.gotham_server_port, config.gotham_server_ip);
    if(sockfd < 0) {
        write(STDOUT_FILENO, "Error: Cannot connect to Gotham server\n", 40);
        free_config();
        free(data);
        exit(1);
    }
    
    write(STDOUT_FILENO, "Successfully connected to Gotham.\n", 35);
    sprintf(data, "%s&%s&%s", config.worker_type, config.worker_server_ip, config.worker_server_port);
    TRAMA_sendMessageToSocket(sockfd, 0x02, (int16_t)strlen(data), data);    
    free(data);

    struct trama wtrama;
    if(TRAMA_readMessageFromSocket(sockfd, &wtrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
    } else if(strcmp((const char *)wtrama.data, "CON_KO") == 0) {
        write(STDOUT_FILENO, "Error: Connection not validated.\n", 34);
    }
    free(wtrama.data);

    pthread_t watcher_thread;
    if(pthread_create(&watcher_thread, NULL, connection_watcher, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
    }  
    if (pthread_detach(watcher_thread) != 0) {
        perror("Error detaching thread");
    }

    initServer();

    close(sockfd);
    free_config();

    return 0;
}