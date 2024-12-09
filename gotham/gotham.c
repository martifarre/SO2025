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

void free_config() {
    free(config.fleck_server_ip);
    free(config.fleck_server_port); // Liberar la memoria del puerto
    free(config.external_server_ip);
    free(config.external_server_port); // Liberar la memoria del puerto
}

int i = 0;

void searchWorkerAndSendInfo(int fleckSock, char* type) {
    listElement* element = NULL;
    char* message = (char*)malloc(sizeof(char) * 256);
    char* data = (char*)malloc(sizeof(char) * 256);
    
    if (LINKEDLIST_isEmpty(listW)) {
        write(STDOUT_FILENO, "No workers available\n", 21);
        TRAMA_sendMessageToSocket(fleckSock, 0x10, (int16_t)strlen("DISTORT_KO"), "DISTORT_KO");
        free(message);
        free(data);
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
        sprintf(data, "%s&%s", element->ip, element->port);  
        TRAMA_sendMessageToSocket(fleckSock, 0x10, (int16_t)strlen(data), data);
    }
    free(message);
    free(data);
}

void* threadFleck(void* arg) {
    int fleckSock = *(int*)arg;
    char* message = (char*)malloc(sizeof(char) * 256);
    struct trama gtrama;

    // First message read from Fleck
    if (TRAMA_readMessageFromSocket(fleckSock, &gtrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        free(message);
        return NULL;
    }
    
    // Process the first message
    if (gtrama.tipo == 0x01) {
        char* username = STRING_getXFromMessage((const char *)gtrama.data, 0);
        
        // We no longer need gtrama.data after extracting username
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

        memset(message, '\0', 256);

        // Now enter the loop to read subsequent messages
        while (1) {
            if (TRAMA_readMessageFromSocket(fleckSock, &gtrama) < 0) {
                write(STDOUT_FILENO, "Error: Checksum not validated..\n", 32);
                free(message);
                return NULL;
            }
            
            // Process subsequent messages
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
                        // Finish using gtrama.data after comparison
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
                // If other message types appear, ensure to free gtrama.data after using it:
                free(gtrama.data);
                gtrama.data = NULL;
            }
        }
    } else {
        // If the first message isn't 0x01, we've read it but not used gtrama.data:
        free(gtrama.data);
        gtrama.data = NULL;
    }

    close(fleckSock);
    free(message);
    return NULL;
}

// void* threadFleck(void* arg) {
//     int fleckSock = *(int*)arg;
//     char* message = (char*)malloc(sizeof(char) * 256);
//     struct trama gtrama;
    
//     if(TRAMA_readMessageFromSocket(fleckSock, &gtrama) < 0) {
//         write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
//         free(message);
//         return NULL;
//     }

//     if(gtrama.tipo == 0x01) {
//         char* username = STRING_getXFromMessage((const char *)gtrama.data, 0);
//         listElement* element = (listElement*)malloc(sizeof(listElement));
//         element->sockfd = fleckSock;
//         element->fleck_username = username;
//         LINKEDLIST_add(listF, element);

//         char* data = (char*)malloc(sizeof(char) * 256);
//         sprintf(data, "\nWelcome %s, you are connected to Gotham.\n\n", username);
//         write(STDOUT_FILENO, data, strlen(data));
//         TRAMA_sendMessageToSocket(fleckSock, 0x01, 0, "");
//         free(data);

//         memset(message, '\0', 256);
//         while(1) {
//             if(TRAMA_readMessageFromSocket(fleckSock, &gtrama) < 0) {
//                 write(STDOUT_FILENO, "Error: Checksum not validated..\n", 32);
//                 free(message);
//                 return NULL;
//             }
//             if(gtrama.tipo == 0x10) {
//                 if(strcmp((const char *)gtrama.data, "CON_KO") == 0) {
//                     write(STDOUT_FILENO, "Error: Distortion of this type already in progress.\n", 53);
//                 } else {
//                     char* type = STRING_getXFromMessage((const char *)gtrama.data, 0);
//                     //char* filename = STRING_getXFromMessage((const char *)gtrama.data, 1);
//                     searchWorkerAndSendInfo(fleckSock, type);
//                 }
//             } else if(gtrama.tipo == 0x07) {
//                 LINKEDLIST_goToHead(listF);
//                 while(!LINKEDLIST_isAtEnd(listF)) {
//                     listElement* currentElement = LINKEDLIST_get(listF);
//                     if(strcmp(currentElement->fleck_username, (const char *)gtrama.data) == 0 && currentElement->sockfd == fleckSock) {
//                         free(currentElement->fleck_username);
//                         free(currentElement);
//                         LINKEDLIST_remove(listF);
//                         break;
//                     }
//                     LINKEDLIST_next(listF);
//                 }
//                 write(STDOUT_FILENO, "Fleck was disconnected.\n\n", 26);
//                 break;
//             }
//         }
//     }

//     close(fleckSock);
//     free(message);
//     return NULL;
// }

void* funcThreadFleckConnecter() {
    int socket_fd = SOCKET_initSocket(config.fleck_server_port, config.fleck_server_ip);
    
    int newsock;
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof (c_addr);

    while(1) {
        newsock = accept(socket_fd, (void *) &c_addr, &c_len);
        if (newsock < 0) {
            write(STDOUT_FILENO, "Error: Cannot accept connection\n", 33);
            exit (EXIT_FAILURE);
        }

        pthread_t thread_newFleck;
        pthread_create(&thread_newFleck, NULL, threadFleck, &newsock); 
        pthread_detach(thread_newFleck);
    }
}

void* threadWorker(void* arg) {
    int newsock = *(int*)arg;
    char* aux = (char*)malloc(sizeof(char) * 256);
    struct trama gtrama;

    if (TRAMA_readMessageFromSocket(newsock, &gtrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated...\n", 34);
        free(aux);
        return NULL;
    }

    if (gtrama.tipo == 0x02) {
        char* worker_type = STRING_getXFromMessage((const char *)gtrama.data, 0);
        char* ip          = STRING_getXFromMessage((const char *)gtrama.data, 1);
        char* port        = STRING_getXFromMessage((const char *)gtrama.data, 2);

        free(gtrama.data);
        gtrama.data = NULL;

        listElement* element = (listElement*)malloc(sizeof(listElement));
        element->sockfd = newsock;
        element->ip = ip;
        element->port = port;
        element->worker_type = worker_type;

        LINKEDLIST_add(listW, element);
        sprintf(aux, "Worker of type %s added\n\n", worker_type);
        write(STDOUT_FILENO, aux, strlen(aux));
        TRAMA_sendMessageToSocket(newsock, 0x02, 0, "");

        while (1) {
            if (TRAMA_readMessageFromSocket(newsock, &gtrama) < 0) {
                write(STDOUT_FILENO, "Error: Checksum not validated....\n", 35);
                free(aux);
                return NULL;
            }

            if (gtrama.tipo == 0x07) {
                LINKEDLIST_goToHead(listW);
                while (!LINKEDLIST_isAtEnd(listW)) {
                    listElement* currentElement = LINKEDLIST_get(listW);
                    if (strcmp(currentElement->worker_type, (const char *)gtrama.data) == 0 
                        && currentElement->sockfd == newsock) {
                        
                        // Done using gtrama.data for comparison
                        free(gtrama.data);
                        gtrama.data = NULL;
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
                break;
            } else {
                // If other message types appear, ensure to free gtrama.data after using it
                // If you used the message data, free it. If it's not needed, free directly:
                free(gtrama.data);
                gtrama.data = NULL;
            }
        }
    } else {
        // If the first message isn't tipo == 0x02, free gtrama.data now
        free(gtrama.data);
        gtrama.data = NULL;
    }

    close(newsock);
    free(aux);
    return NULL;
}


// void* threadWorker(void* arg) {
//     int newsock = *(int*)arg;
//     char* aux = (char*)malloc(sizeof(char) * 256);
//     struct trama gtrama;
       
//     if(TRAMA_readMessageFromSocket(newsock, &gtrama) < 0) {
//        write(STDOUT_FILENO, "Error: Checksum not validated...\n", 34);
//        free(aux);
//        return NULL;
//     }

//     if(gtrama.tipo == 0x02) {
//         char* worker_type = STRING_getXFromMessage((const char *)gtrama.data, 0);
//         char* ip = STRING_getXFromMessage((const char *)gtrama.data, 1);
//         char* port = STRING_getXFromMessage((const char *)gtrama.data, 2);

//         listElement* element = (listElement*)malloc(sizeof(listElement));
//         element->sockfd = newsock;
//         element->ip = ip;
//         element->port = port;
//         element->worker_type = worker_type;

//         LINKEDLIST_add(listW, element);
//         sprintf(aux, "Worker of type %s added\n\n", worker_type);
//         write(STDOUT_FILENO, aux, strlen(aux));
//         TRAMA_sendMessageToSocket(newsock, 0x02, 0, "");
//         //TRAMA_sendMessageToSocket(newsock, 0x02, (int16_t)strlen("CON_KO"), "CON_KO");
        
//         while(1) {
//             if(TRAMA_readMessageFromSocket(newsock, &gtrama) < 0) {
//                 write(STDOUT_FILENO, "Error: Checksum not validated....\n", 35);
//                 return NULL;
//             }

//             if(gtrama.tipo == 0x07) {
//                 LINKEDLIST_goToHead(listW);

//                 while(!LINKEDLIST_isAtEnd(listW)) {
//                     listElement* currentElement = LINKEDLIST_get(listW);
//                     if(strcmp(currentElement->worker_type, (const char *)gtrama.data) == 0 && currentElement->sockfd == newsock) {
//                         free(currentElement->ip);
//                         free(currentElement->port);
//                         free(currentElement->worker_type);
//                         free(currentElement);
//                         LINKEDLIST_remove(listW);
//                         break;
//                     }
//                     LINKEDLIST_next(listW);
//                 }
//                 write(STDOUT_FILENO, "Worker was disconnected.\n\n", 27);
//                 break;
//             }
//         }

//     }
//     close(newsock);
//     free(aux);
//     return NULL;
// }

void* funcThreadWorkerConnecter() {
    int socket_fd = SOCKET_initSocket(config.external_server_port, config.external_server_ip);
    
    int newsock;
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof (c_addr);

    while(1) {
        newsock = accept(socket_fd, (void *) &c_addr, &c_len);
        if (newsock < 0) {
            write(STDOUT_FILENO, "Error: Cannot accept connection\n", 33);
            exit (EXIT_FAILURE);
        }
        write(STDOUT_FILENO, "Worker connected\n\n", 19);

        pthread_t thread_newWorker;
        pthread_create(&thread_newWorker, NULL, threadWorker, &newsock); 
        pthread_detach(thread_newWorker);
    }
}

void doLogout() {
    LINKEDLIST_goToHead(listF);
    while(!LINKEDLIST_isAtEnd(listF)) {
        listElement* currentElement = LINKEDLIST_get(listF);
        printf("Disconnecting %s\n", currentElement->fleck_username);
        close(currentElement->sockfd);

        free(currentElement->fleck_username);
        free(currentElement);
        LINKEDLIST_remove(listF);
        //LINKEDLIST_next(listF);
    }

    LINKEDLIST_goToHead(listW);
    while(!LINKEDLIST_isAtEnd(listW)) {
        listElement* currentElement = LINKEDLIST_get(listW);
        printf("Disconnecting worker of type %s\n", currentElement->worker_type);
        close(currentElement->sockfd);

        free(currentElement->ip);
        free(currentElement->port);
        free(currentElement->worker_type);
        free(currentElement);

        LINKEDLIST_remove(listW);
        //LINKEDLIST_next(listW);
    }
}

void CTRLC(int signum) {
    print_text("\nInterrupt signal CTRL+C received\n");
    doLogout();
    LINKEDLIST_destroy(&listF);
    LINKEDLIST_destroy(&listW);
    free_config();
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
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

    pthread_t thread_WorkerConnecter;
    pthread_t thread_FleckConnecter;

    if (pthread_create(&thread_WorkerConnecter, NULL, funcThreadWorkerConnecter, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
        return 1;
    }

    if(pthread_create(&thread_FleckConnecter, NULL, funcThreadFleckConnecter, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
        return 1;
    }
        
    if (pthread_join(thread_WorkerConnecter, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot join thread\n", 27);
        return 1; 
    }

    if (pthread_join(thread_FleckConnecter, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot join thread\n", 27);
        return 1; 
    }
    
    LINKEDLIST_destroy(&listF);
    LINKEDLIST_destroy(&listW);
    free_config();
    return 0;
}