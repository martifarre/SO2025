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

#define MEDIA 0
#define TEXT 1

// Variable global para almacenar la configuración
WorkerConfig config;

volatile sig_atomic_t *stop_signal = NULL;

int fleckSock = -1, sockfd = -1, fleck_connecter_fd = -1;

LinkedList2 listE;
LinkedList2 listH;
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

void read_from_msq(LinkedList2 listW, int type) {
    key_t key = ftok("worker.c", type);
    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("msgget failed");
        return;
    }

    listElement2 msg;
    while (1) {
        if (msgrcv(msqid, &msg, sizeof(listElement2) - sizeof(long), 1, IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                break;
            } else {
                perror("msgrcv failed");
            }
        } else {
            listElement2* newWorker = malloc(sizeof(listElement2));
            if (!newWorker) {
                perror("Memory allocation failed");
                continue;
            }

            newWorker->username = strdup(msg.username);
            newWorker->worker_type = strdup(msg.worker_type);
            newWorker->factor = strdup(msg.factor);
            newWorker->MD5SUM = strdup(msg.MD5SUM);
            newWorker->directory = strdup(msg.directory);
            strncpy(newWorker->fileName, msg.fileName, sizeof(newWorker->fileName));
            newWorker->bytes_writtenF1 = msg.bytes_writtenF1;
            newWorker->bytes_to_writeF1 = msg.bytes_to_writeF1;
            newWorker->bytes_writtenF2 = msg.bytes_writtenF2;
            newWorker->bytes_to_writeF2 = msg.bytes_to_writeF2;
            newWorker->fd = msg.fd;
            newWorker->thread_id = msg.thread_id;
            newWorker->status = msg.status;
            LINKEDLIST2_add(listW, newWorker);
        }
    }
}

void send_to_msq(listElement2* element, int workerType) {
    if (element == NULL) {
        write(STDOUT_FILENO, "Error: NULL element in send_to_message_queue.\n", 47);
        return;
    }

    key_t key = ftok("worker.c", workerType);
    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("Error creating/getting message queue");
        return;
    }

    listElement2 msg;
    msg.message_type = 1;
    
    strncpy(msg.fileName, element->fileName, sizeof(msg.fileName));

    msg.username = strdup(element->username);
    msg.worker_type = strdup(element->worker_type);
    msg.factor = strdup(element->factor);
    msg.MD5SUM = strdup(element->MD5SUM);
    msg.directory = strdup(element->directory);
    msg.bytes_writtenF1 = element->bytes_writtenF1;
    msg.bytes_to_writeF1 = element->bytes_to_writeF1;
    msg.bytes_writtenF2 = element->bytes_writtenF2;
    msg.bytes_to_writeF2 = element->bytes_to_writeF2;
    msg.fd = -1; 
    msg.thread_id = element->thread_id;
    msg.status = element->status;

    if (msgsnd(msqid, &msg, sizeof(listElement2) - sizeof(long), 0) == -1) {
        perror("Error sending message to queue");
    } else {
        write(STDOUT_FILENO, "Message successfully sent to queue.\n", 37);
    }

    free(msg.username);
    free(msg.worker_type);
    free(msg.factor);
    free(msg.MD5SUM);
    free(msg.directory);
}

void* distortFileThread(void* arg) {
    listElement2* element = (listElement2*)arg;
    if (element == NULL) {
        write(STDOUT_FILENO, "Error: NULL element received in distortFileThread.\n", 52);
        return NULL;
    }

    element->thread_id = pthread_self(); 
    if(DISTORSION_distortFile(element, stop_signal) == 0) {
        LinkedList2 targetList = (strcmp(element->worker_type, "Media") == 0) ? listH : listE;
        if (!LINKEDLIST2_isEmpty(targetList)) {
            LINKEDLIST2_goToHead(targetList);
            while (!LINKEDLIST2_isAtEnd(targetList)) {
                listElement2* current = LINKEDLIST2_get(targetList);
                if (current == element) {
                    LINKEDLIST2_remove(targetList);
                    free(element);
                    break;
                }
                LINKEDLIST2_next(targetList);
            }
        }
    } else {
        write(STDOUT_FILENO, "Error: Distortion failed.\n", 27);
    }
    return NULL;
}


void initServer() {
    struct trama wtrama;

    if(TRAMA_readMessageFromSocket(sockfd, &wtrama) < 0) {
        write(STDOUT_FILENO, "Error: Reading from socket.\n", 29);
        free(wtrama.data);
        return;
    } else if(wtrama.tipo != 0x08) {
        write(STDOUT_FILENO, "Error: I'm not the principal what is this.\n", 44);
        free(wtrama.data);
        return;
    } else if(wtrama.tipo == 0x08) {
        write(STDOUT_FILENO, "I'm the principal worker.\n\n", 27);
        free(wtrama.data);
    }

    if(strcmp(config.worker_type, "Media") == 0) {
        read_from_msq(listH, MEDIA);
    } else if(strcmp(config.worker_type, "Text") == 0) {
        read_from_msq(listE, TEXT);
    }

    fleck_connecter_fd = SOCKET_initSocket(config.worker_server_port, config.worker_server_ip);
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    char* data = (char*)malloc(sizeof(char) * 256);
    while (1) {
        fleckSock = accept(fleck_connecter_fd, (void *)&c_addr, &c_len);
        if (fleckSock < 0) {
            write(STDOUT_FILENO, "Error: Cannot accept connection\n", 33);
            exit(EXIT_FAILURE);
        }

        struct trama wtrama;
        if (TRAMA_readMessageFromSocket(fleckSock, &wtrama) < 0) {
            write(STDOUT_FILENO, "Error: Reading distortion info from fleck.\n", 44);
            free(wtrama.data);
            close(fleckSock);
            fleckSock = -1;
            continue; // Saltar al siguiente ciclo del bucle
        }

        char* userName = STRING_getXFromMessage((const char *)wtrama.data, 0);
        char* fileName = STRING_getXFromMessage((const char *)wtrama.data, 1);
        char* fileSize = STRING_getXFromMessage((const char *)wtrama.data, 2);
        char* MD5SUM = STRING_getXFromMessage((const char *)wtrama.data, 3);
        char* factor = STRING_getXFromMessage((const char *)wtrama.data, 4);

        sprintf(data, "Fleck name: %s File received: %s\n", userName, fileName);
        write(STDOUT_FILENO, data, strlen(data));
        free(data);

        LinkedList2 targetList = (strcmp(config.worker_type, "Media") == 0) ? listH : listE;

        listElement2* existingElement = NULL;
        int found = 0;

        if (!LINKEDLIST2_isEmpty(targetList)) {
            LINKEDLIST2_goToHead(targetList);
            while (!LINKEDLIST2_isAtEnd(targetList)) {
                listElement2* element = LINKEDLIST2_get(targetList);
                if (strcmp(element->fileName, fileName) == 0 && strcmp(element->username, userName) == 0) {
                    existingElement = element;
                    existingElement->fd = fleckSock;
                    found = 1;
                    break;
                }
                LINKEDLIST2_next(targetList);
            }
        }

        listElement2* newElement = NULL;
        if (!found) {
            newElement = malloc(sizeof(listElement2));
            strncpy(newElement->fileName, fileName, sizeof(newElement->fileName));

            newElement->username = strdup(userName);
            newElement->worker_type = strdup(config.worker_type);
            newElement->factor = strdup(factor);
            newElement->MD5SUM = strdup(MD5SUM);
            newElement->directory = strdup(config.directory);
            newElement->bytes_to_writeF1 = atoi(fileSize);
            newElement->bytes_writtenF1 = 0;
            newElement->bytes_to_writeF2 = 0;
            newElement->bytes_writtenF2 = 0;
            newElement->fd = fleckSock;
            newElement->status = 0;

            LINKEDLIST2_add(targetList, newElement);
        }

        TRAMA_sendMessageToSocket(fleckSock, 0x03, 0, ""); // Indicar que se puede empezar a enviar el archivo.
        // TRAMA_sendMessageToSocket(fleckSock, 0x03, (int16_t)strlen("CON_KO"), "CON_KO"); // Error, no se puede enviar archivo.

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, distortFileThread, found ? existingElement : newElement);
        pthread_detach(thread_id);

        free(userName);
        free(fileName);
        free(fileSize);
        free(MD5SUM);
        free(factor);
        free(wtrama.data);
    }
}

    

void doLogout() {
    if (SOCKET_isSocketOpen(sockfd)) {
        write(STDOUT_FILENO, "Sending logout message to Gotham server...\n", 43);
        TRAMA_sendMessageToSocket(sockfd, 0x07, (int16_t)strlen(config.worker_type), config.worker_type);
        close(sockfd);
    }

    write(STDOUT_FILENO, "Stopping all active threads...\n", 32);
    LinkedList2 targetList = (strcmp(config.worker_type, "Media") == 0) ? listH : listE;

    if (!LINKEDLIST2_isEmpty(targetList)) {
        LINKEDLIST2_goToHead(targetList);
        while (!LINKEDLIST2_isAtEnd(targetList)) {
            listElement2* element = LINKEDLIST2_get(targetList);

            if (element->fd > 0) {
                TRAMA_sendMessageToSocket(element->fd, 0x07, (int16_t)strlen("CON_KO"), "CON_KO");
                close(element->fd);
            }

            if (element->thread_id != 0) {
                pthread_join(element->thread_id, NULL); 
            }
            
            if(strcmp(config.worker_type, "Media") == 0) {
                send_to_msq(element, MEDIA);
            } else if(strcmp(config.worker_type, "Text") == 0) {
                send_to_msq(element, TEXT);
            }

            LINKEDLIST2_remove(targetList);
            free(element);
        }
    }

    write(STDOUT_FILENO, "All connections closed.\n", 25);
}

void CTRLC(int signum) {
    write(STDOUT_FILENO, "\nInterrupt signal CTRL+C received. Stopping worker...\n", 54);
    *stop_signal = 1; 
    doLogout(); 

    close(fleck_connecter_fd);
    free_config();

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void *connection_watcher() {
    while (1) {
        if (!SOCKET_isSocketOpen(sockfd)) {
            write(STDOUT_FILENO, "Connection to Gotham lost\n", 27);
            close(sockfd);
            while(1) {
                if(!SOCKET_isSocketOpen(fleckSock)) {
                    CTRLC(0);
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
    volatile sig_atomic_t stop_flag = 0;
    stop_signal = &stop_flag;
    signal(SIGINT, CTRLC);

    listE = LINKEDLIST2_create();
    listH = LINKEDLIST2_create();

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
    LINKEDLIST2_destroy(&listE);
    LINKEDLIST2_destroy(&listH);

    return 0;
}