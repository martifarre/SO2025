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

typedef struct {
    long message_type;
    char filename[256];  // Ajusta el tamaño según lo necesario
    char username[64]; 
    char worker_type[64]; 
    char factor[64]; 
    int bytes_writtenF1;
    int bytes_to_writeF1;
    int bytes_writtenF2;
    int bytes_to_writeF2;
    int fd;
    char MD5SUM[64];
    char directory[256];
    pthread_t thread_id;
    int status; // 0: No empezada, 1: Transfiriendo1 , 2: Distorsionando, 3: Transfiriendo2, 4: Completada
} MessageQueueElement;

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
    printf("[DEBUG] Iniciando `read_from_msq` con type=%d\n", type);

    key_t key = ftok("worker.c", type);
    printf("[DEBUG] Clave generada: %d\n", key);

    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("[ERROR] msgget failed");
        return;
    }

    printf("[DEBUG] Cola de mensajes obtenida con ID: %d\n", msqid);

    MessageQueueElement msg;
    while (1) {
        printf("[DEBUG] Esperando mensajes en la cola...\n");
        if (msgrcv(msqid, &msg, sizeof(MessageQueueElement) - sizeof(long), 0, IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                printf("[DEBUG] No hay más mensajes en la cola.\n");
                break;
            } else {
                perror("[ERROR] msgrcv failed");
            }
        } else {
            printf("[DEBUG] Mensaje recibido:\n");
            printf("        Filename: %s\n", msg.filename);
            printf("        Username: %s\n", msg.username);
            printf("        Worker Type: %s\n", msg.worker_type);
            printf("        Factor: %s\n", msg.factor);
            printf("        Status: %d\n", msg.status);
            printf("        MD5SUM: %s\n", msg.MD5SUM);
            printf("        Directory: %s\n", msg.directory);
            printf("        Bytes written F1: %d\n", msg.bytes_writtenF1);
            printf("        Bytes to write F1: %d\n", msg.bytes_to_writeF1);
            printf("        Bytes written F2: %d\n", msg.bytes_writtenF2);
            printf("        Bytes to write F2: %d\n", msg.bytes_to_writeF2);

            listElement2* newWorker = malloc(sizeof(listElement2));
            if (!newWorker) {
                perror("[ERROR] Memory allocation failed");
                continue;
            }

            newWorker->username = strdup(msg.username);
            newWorker->worker_type = strdup(msg.worker_type);
            newWorker->factor = strdup(msg.factor);
            newWorker->MD5SUM = strdup(msg.MD5SUM);
            newWorker->directory = strdup(msg.directory);
            newWorker->fileName = strdup(msg.filename);

            newWorker->bytes_writtenF1 = msg.bytes_writtenF1;
            newWorker->bytes_to_writeF1 = msg.bytes_to_writeF1;
            newWorker->bytes_writtenF2 = msg.bytes_writtenF2;
            newWorker->bytes_to_writeF2 = msg.bytes_to_writeF2;
            newWorker->fd = msg.fd;
            newWorker->thread_id = msg.thread_id;
            newWorker->status = msg.status;

            printf("[DEBUG] Nuevo elemento creado y agregado a la LinkedList.\n");
            LINKEDLIST2_add(listW, newWorker);
        }
    }

    // 🔹 Definir `buf` antes de usarlo
    struct msqid_ds buf;  
    msgctl(msqid, IPC_STAT, &buf);
    if (buf.msg_qnum == 0) { // Solo eliminar si ya no hay mensajes
        printf("[DEBUG] Cerrando todos los descriptores abiertos...\n");

        // Intentar cerrar la cola antes de eliminarla
        if (msgctl(msqid, IPC_RMID, NULL) == 0) {
            printf("[DEBUG] Cola de mensajes eliminada correctamente.\n");
        } else {
            perror("[ERROR] No se pudo eliminar la cola");
        }
    } else {
        printf("[WARNING] La cola aún tiene mensajes, no se eliminó.\n");
    }
    printf("[DEBUG] Terminando `read_from_msq`\n");
}

void send_to_msq(listElement2* element, int workerType) {
    if (element == NULL) {
        write(STDOUT_FILENO, "[ERROR] NULL element in send_to_message_queue.\n", 47);
        return;
    }

    printf("[DEBUG] Iniciando `send_to_msq` con workerType=%d\n", workerType);

    key_t key = ftok("worker.c", workerType);
    printf("[DEBUG] Clave generada: %d\n", key);

    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("[ERROR] Error creating/getting message queue");
        return;
    }

    printf("[DEBUG] Cola de mensajes obtenida con ID: %d\n", msqid);

    MessageQueueElement msg;
    memset(&msg, 0, sizeof(MessageQueueElement));

    msg.message_type = 1;

    strncpy(msg.filename, element->fileName, sizeof(msg.filename) - 1);
    msg.filename[sizeof(msg.filename) - 1] = '\0';

    strncpy(msg.username, element->username, sizeof(msg.username) - 1);
    msg.username[sizeof(msg.username) - 1] = '\0';

    strncpy(msg.worker_type, element->worker_type, sizeof(msg.worker_type) - 1);
    msg.worker_type[sizeof(msg.worker_type) - 1] = '\0';

    strncpy(msg.factor, element->factor, sizeof(msg.factor) - 1);
    msg.factor[sizeof(msg.factor) - 1] = '\0';

    strncpy(msg.MD5SUM, element->MD5SUM, sizeof(msg.MD5SUM) - 1);
    msg.MD5SUM[sizeof(msg.MD5SUM) - 1] = '\0';

    strncpy(msg.directory, element->directory, sizeof(msg.directory) - 1);
    msg.directory[sizeof(msg.directory) - 1] = '\0';

    msg.bytes_writtenF1 = element->bytes_writtenF1;
    msg.bytes_to_writeF1 = element->bytes_to_writeF1;
    msg.bytes_writtenF2 = element->bytes_writtenF2;
    msg.bytes_to_writeF2 = element->bytes_to_writeF2;
    msg.fd = -1; // Lo forzamos a -1
    msg.thread_id = element->thread_id;
    msg.status = element->status;

    printf("[DEBUG] Enviando mensaje a la cola:\n");
    printf("        Filename: %s\n", msg.filename);
    printf("        Username: %s\n", msg.username);
    printf("        Worker Type: %s\n", msg.worker_type);
    printf("        Factor: %s\n", msg.factor);
    printf("        Status: %d\n", msg.status);
    printf("        MD5SUM: %s\n", msg.MD5SUM);
    printf("        Directory: %s\n", msg.directory);
    printf("        Bytes written F1: %d\n", msg.bytes_writtenF1);
    printf("        Bytes to write F1: %d\n", msg.bytes_to_writeF1);
    printf("        Bytes written F2: %d\n", msg.bytes_writtenF2);
    printf("        Bytes to write F2: %d\n", msg.bytes_to_writeF2);

    if (msgsnd(msqid, &msg, sizeof(MessageQueueElement) - sizeof(long), 0) == -1) {
        perror("[ERROR] Error sending message to queue");
    } else {
        write(STDOUT_FILENO, "[DEBUG] Message successfully sent to queue.\n", 44);
    }
}

void* distortFileThread(void* arg) {
    listElement2* element = (listElement2*)arg;
    if (element == NULL) {
        write(STDOUT_FILENO, "Error: NULL element received in distortFileThread.\n", 52);
        return NULL;
    }

    element->thread_id = pthread_self(); 
    if (DISTORSION_distortFile(element, stop_signal) == 0) {
        LinkedList2 targetList = (strcmp(element->worker_type, "Media") == 0) ? listH : listE;
        if (!LINKEDLIST2_isEmpty(targetList)) {
            LINKEDLIST2_goToHead(targetList);
            while (!LINKEDLIST2_isAtEnd(targetList)) {
                listElement2* current = LINKEDLIST2_get(targetList);
                if (current == element) {
                    LINKEDLIST2_remove(targetList);

                    free(element->fileName);
                    free(element->username);
                    free(element->worker_type);
                    free(element->factor);
                    free(element->MD5SUM);
                    free(element->directory);
                    
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

    fleck_connecter_fd = SOCKET_initSocket(config.worker_server_port, config.worker_server_ip);
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    while (1) {
        fleckSock = accept(fleck_connecter_fd, (void *)&c_addr, &c_len);
        if (fleckSock < 0) {
            write(STDOUT_FILENO, "Error: Cannot accept connection\n", 33);
            exit(EXIT_FAILURE);
        }

        sleep(3);   

        if(strcmp(config.worker_type, "Media") == 0) {
            read_from_msq(listH, MEDIA);
        } else if(strcmp(config.worker_type, "Text") == 0) {
            read_from_msq(listE, TEXT);
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

        char *data = NULL;
        if (asprintf(&data, "Fleck name: %s File received: %s\n", userName, fileName) == -1) return;
        write(STDOUT_FILENO, data, strlen(data));
        free(data);

        LinkedList2 targetList = (strcmp(config.worker_type, "Media") == 0) ? listH : listE;

        listElement2* existingElement = NULL;
        int found = 0;

        if (!LINKEDLIST2_isEmpty(targetList)) {
            write(STDOUT_FILENO, "List not empty...\n", 19);
            LINKEDLIST2_goToHead(targetList);
            while (!LINKEDLIST2_isAtEnd(targetList)) {
                listElement2* element = LINKEDLIST2_get(targetList);
                printf("Filename element: %s, filaname: %s\n", element->fileName, fileName);
                printf("Username element: %s, username: %s\n", element->username, userName);
                if (strcmp(element->fileName, fileName) == 0 && strcmp(element->username, userName) == 0) {
                    existingElement = element;
                    existingElement->fd = fleckSock;
                    found = 1;
                    write (STDOUT_FILENO, "Element found...\n", 18);
                    break;
                }
                LINKEDLIST2_next(targetList);
            }
        }

        listElement2* newElement = NULL;
        if (!found) {
            newElement = malloc(sizeof(listElement2));
            newElement->fileName = strdup(fileName);
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
                if(element->status != 1) {
                    TRAMA_sendMessageToSocket(element->fd, 0x07, (int16_t)strlen("CON_KO"), "CON_KO");
                }
                close(element->fd);
                element->fd = -1;
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

            free(element->fileName);
            free(element->username);
            free(element->worker_type);
            free(element->factor);
            free(element->MD5SUM);
            free(element->directory);
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
    LINKEDLIST2_destroy(&listE);
    LINKEDLIST2_destroy(&listH);

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
    signal(SIGPIPE, SIG_IGN);

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