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
#include "../modules/project.h"

// Variable global para almacenar la configuración
GothamConfig config;
LinkedList listW; 
LinkedList listF;

int fleck_connecter_fd = -1, worker_connecter_fd = -1;

// Thread IDs for the main listener threads
pthread_t thread_WorkerConnecter;
pthread_t thread_FleckConnecter;

// A flag to indicate that we got a SIGINT and need to shutdown
volatile sig_atomic_t shutdown_requested = 0;

void free_config() {
    free(config.fleck_server_ip);
    free(config.fleck_server_port);
    free(config.external_server_ip);
    free(config.external_server_port);
}

void searchWorkerAndSendInfo(int fleckSock, char* type) {
    listElement* element = NULL;
    char* message = (char*)malloc(sizeof(char) * 256);
    
    if (LINKEDLIST_isEmpty(listW)) {
        write(STDOUT_FILENO, "No workers available\n", 21);
        TRAMA_sendMessageToSocket(fleckSock, 0x10, (int16_t)strlen("DISTORT_KO"), "DISTORT_KO");
        free(message);
        return;
    }

    LINKEDLIST_goToHead(listW);

    while(!LINKEDLIST_isAtEnd(listW)) {
        listElement* currentElement = LINKEDLIST_get(listW);
        if(strcmp(currentElement->worker_type, type) == 0) {
            element = currentElement;
            break;
        }
        LINKEDLIST_next(listW);
    }

    if(element == NULL) {
        sprintf(message, "\nNo workers of type %s available.\n\n", type);
        write(STDOUT_FILENO, message, strlen(message));
        TRAMA_sendMessageToSocket(fleckSock, 0x10, (int16_t)strlen("DISTORT_KO"), "DISTORT_KO");
    } else {
        write(STDOUT_FILENO, "Worker found, sending to Fleck.\n\n", 33);
        sprintf(message, "%s&%s", element->ip, element->port);  
        TRAMA_sendMessageToSocket(fleckSock, 0x10, (int16_t)strlen(message), message);
    }
    free(message);
}

void* threadFleck(void* arg) {
    int fleckSock = *(int*)arg;
    struct trama gtrama;

    if (TRAMA_readMessageFromSocket(fleckSock, &gtrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        return NULL;
    }
    
    if (gtrama.tipo == 0x01) {
        char* username = STRING_getXFromMessage((const char *)gtrama.data, 0);
        free(gtrama.data); 
        gtrama.data = NULL;

        listElement* element = (listElement*)malloc(sizeof(listElement));
        element->sockfd = fleckSock;
        element->fleck_username = username;
        LINKEDLIST_add(listF, element);

        char* data = (char*)malloc(sizeof(char) * 256);
        sprintf(data, "\nWelcome %s, you are connected to Gotham.\n\n", username);
        write(STDOUT_FILENO, data, strlen(data));
        TRAMA_sendMessageToSocket(fleckSock, 0x01, 0, "");
        free(data);

        while (1) {
            if (TRAMA_readMessageFromSocket(fleckSock, &gtrama) < 0) {
                write(STDOUT_FILENO, "Error: Checksum not validated..\n", 32);
                return NULL;
            }
            
            if (gtrama.tipo == 0x10) {
                if (strcmp((const char *)gtrama.data, "CON_KO") == 0) {
                    write(STDOUT_FILENO, "Error: Distortion of this type already in progress.\n", 53);
                    free(gtrama.data); 
                    gtrama.data = NULL;
                } else {
                    char* type = STRING_getXFromMessage((const char *)gtrama.data, 0);
                    free(gtrama.data); 
                    gtrama.data = NULL;
                    
                    searchWorkerAndSendInfo(fleckSock, type);
                    free(type);
                }
            } else if (gtrama.tipo == 0x07) {
                LINKEDLIST_goToHead(listF);
                while(!LINKEDLIST_isAtEnd(listF)) {
                    listElement* currentElement = LINKEDLIST_get(listF);
                    if(strcmp(currentElement->fleck_username, (const char *)gtrama.data) == 0 && currentElement->sockfd == fleckSock) {
                        free(gtrama.data); 
                        gtrama.data = NULL;

                        free(currentElement->fleck_username);
                        free(currentElement);
                        LINKEDLIST_remove(listF);
                        break;
                    }
                    LINKEDLIST_next(listF);
                }
                write(STDOUT_FILENO, "Fleck was disconnected.\n\n", 26);
                break;
            } else {
                free(gtrama.data);
                gtrama.data = NULL;
            }
        }
    } else {
        free(gtrama.data);
        gtrama.data = NULL;
    }

    close(fleckSock);
    return NULL;
}

void* funcThreadFleckConnecter(void* arg) {
    fleck_connecter_fd = SOCKET_initSocket(config.fleck_server_port, config.fleck_server_ip);
    int newsock;
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof (c_addr);

    while(!shutdown_requested) {
        newsock = accept(fleck_connecter_fd, (void *) &c_addr, &c_len);
        if (newsock < 0) {
            if (shutdown_requested) break;
            write(STDOUT_FILENO, "Error: Cannot accept connection\n", 33);
            break;
        }

        pthread_t thread_newFleck;
        pthread_create(&thread_newFleck, NULL, threadFleck, &newsock); 
        pthread_detach(thread_newFleck);
    }
    return NULL;
}

void* threadWorker(void* arg) {
    int newsock = *(int*)arg;
    char* aux = (char*)malloc(sizeof(char) * 256);  // Para mensajes temporales
    if (!aux) {
        perror("Error: Memory allocation for aux failed");
        return NULL;
    }

    struct trama gtrama;
     // Leer el primer mensaje
    if (TRAMA_readMessageFromSocket(newsock, &gtrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated...\n", 34);
        free(aux);
        return NULL;
    }

    if (gtrama.tipo == 0x02) {
        char* worker_type = STRING_getXFromMessage((const char *)gtrama.data, 0);
        char* ip          = STRING_getXFromMessage((const char *)gtrama.data, 1);
        char* port        = STRING_getXFromMessage((const char *)gtrama.data, 2);

        free(gtrama.data);  // Liberar gtrama.data después de usarlo
        gtrama.data = NULL;

        if (!worker_type || !ip || !port) {
            write(STDOUT_FILENO, "Error: Invalid worker message.\n", 32);
            free(worker_type);
            free(ip);
            free(port);
            free(aux);
            close(newsock);
            return NULL;
        }

        listElement* element = (listElement*)malloc(sizeof(listElement));
        if (!element) {
            perror("Error: Memory allocation for listElement failed");
            free(worker_type);
            free(ip);
            free(port);
            free(aux);
            close(newsock);
            return NULL;
        }

        element->sockfd = newsock;
        element->ip = ip;
        element->port = port;
        element->worker_type = worker_type;
        LINKEDLIST_add(listW, element);

        sprintf(aux, "Worker of type %s added\n\n", worker_type);
        write(STDOUT_FILENO, aux, strlen(aux));
        TRAMA_sendMessageToSocket(newsock, 0x02, 0, "");
    } else {
        free(gtrama.data);
        gtrama.data = NULL;
        free(aux);
        close(newsock);
        return NULL;
    }

    free(aux);  // Liberar aux tras el uso inicial

    while (1) {
        if (TRAMA_readMessageFromSocket(newsock, &gtrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated....\n", 35);
            free(gtrama.data);
            break;
        }

        if (gtrama.tipo == 0x07) {
            LINKEDLIST_goToHead(listW);
            while (!LINKEDLIST_isAtEnd(listW)) {
                listElement* currentElement = LINKEDLIST_get(listW);
                if (strcmp(currentElement->worker_type, (const char *)gtrama.data) == 0 
                    && currentElement->sockfd == newsock) {
                    free(currentElement->ip);
                    free(currentElement->port);
                    free(currentElement->worker_type);
                    free(currentElement);
                    LINKEDLIST_remove(listW);
                    break;
                }
                LINKEDLIST_next(listW);
            }
            write(STDOUT_FILENO, "Worker was disconnected.\n\n", 27);
            LINKEDLIST_shuffle(listW);
            free(gtrama.data);
            gtrama.data = NULL;
            break;
        }

        free(gtrama.data);
        gtrama.data = NULL;
    }

    close(newsock);
    return NULL;
}

void* funcThreadWorkerConnecter(void* arg) {
    worker_connecter_fd = SOCKET_initSocket(config.external_server_port, config.external_server_ip);
    int newsock;
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof (c_addr);

    while(!shutdown_requested) {
        newsock = accept(worker_connecter_fd, (void *) &c_addr, &c_len);
        if (newsock < 0) {
            if (shutdown_requested) break;
            write(STDOUT_FILENO, "Error: Cannot accept connection\n", 33);
            break;
        }
        write(STDOUT_FILENO, "Worker connected\n\n", 19);

        pthread_t thread_newWorker;
        pthread_create(&thread_newWorker, NULL, threadWorker, &newsock);
        pthread_detach(thread_newWorker);
    }
    return NULL;
}

void doLogout() {
    LINKEDLIST_goToHead(listF);
    while(!LINKEDLIST_isAtEnd(listF)) {
        listElement* currentElement = LINKEDLIST_get(listF);
        close(currentElement->sockfd);
        free(currentElement->fleck_username);
        free(currentElement);
        LINKEDLIST_remove(listF);
    }

    LINKEDLIST_goToHead(listW);
    while(!LINKEDLIST_isAtEnd(listW)) {
        listElement* currentElement = LINKEDLIST_get(listW);
        close(currentElement->sockfd);
        free(currentElement->ip);
        free(currentElement->port);
        free(currentElement->worker_type);
        free(currentElement);
        LINKEDLIST_remove(listW);
    }
}

void CTRLC(int signum) {
    write(STDOUT_FILENO, "\nInterrupt signal CTRL+C received\n", 34);

    // Indicate that we need to shut down
    shutdown_requested = 1;

    // Cancel the listener threads so their accept() calls return
    pthread_cancel(thread_WorkerConnecter);
    pthread_cancel(thread_FleckConnecter);

    // We do not join them here in the signal handler.
    // We'll let main() handle the joining after this function returns.
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: Gotham <config_file>\n", 28);
        exit(1);
    }

    signal(SIGINT, CTRLC);

    config = READCONFIG_read_config_gotham(argv[1]);

    char *msg;
    asprintf(&msg, "\nGotham server initialized.\nWaiting for connections...\n\n");
    print_text(msg);
    free(msg);

    listW = LINKEDLIST_create();
    listF = LINKEDLIST_create();

    if (pthread_create(&thread_WorkerConnecter, NULL, funcThreadWorkerConnecter, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
        return 1;
    }

    if (pthread_create(&thread_FleckConnecter, NULL, funcThreadFleckConnecter, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
        return 1;
    }
        
    // Wait for threads to finish
    // If CTRL+C was pressed, they have been canceled
    pthread_join(thread_WorkerConnecter, NULL);
    pthread_join(thread_FleckConnecter, NULL);

    // Perform the final cleanup
    doLogout();
    close(fleck_connecter_fd);
    close(worker_connecter_fd);
    LINKEDLIST_destroy(&listF);
    LINKEDLIST_destroy(&listW);
    free_config();

    return 0;
}
