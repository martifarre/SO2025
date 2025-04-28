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

#define WORKER_FILE "worker_count"

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
    write(STDOUT_FILENO, "[DEBUG] Iniciando `read_from_msq`...\n", 37);

    key_t key = ftok("worker.c", type);

    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("[ERROR] msgget failed");
        return;
    }

    MessageQueueElement msg;
    while (1) {

        if (msgrcv(msqid, &msg, sizeof(MessageQueueElement) - sizeof(long), 0, IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                write(STDOUT_FILENO, "[DEBUG] No hay más mensajes en la cola.\n", 40);
                break;
            } else {
                perror("[ERROR] msgrcv failed");
            }
        } else {
            // Verificar si los campos clave son válidos
            if (msg.filename[0] == '\0' || msg.username[0] == '\0' || msg.worker_type[0] == '\0') {
                continue; // Ignorar este mensaje y continuar con el siguiente
            }
            write(STDOUT_FILENO, "[DEBUG] Mensaje recibido de la cola.\n", 37);

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

            
            LINKEDLIST2_add(listW, newWorker);
            write(STDOUT_FILENO, "[DEBUG] Mensaje añadido a la lista.\n", 36);
        }
    }

    // 🔹 Definir `buf` antes de usarlo
    struct msqid_ds buf;  
    msgctl(msqid, IPC_STAT, &buf);
    if (buf.msg_qnum == 0) { // Solo eliminar si ya no hay mensajes
        // Intentar cerrar la cola antes de eliminarla
        if (msgctl(msqid, IPC_RMID, NULL) == 0) {
            write(STDOUT_FILENO, "[DEBUG] Cola de mensajes eliminada.\n", 36);
        } else {
            perror("[ERROR] No se pudo eliminar la cola");
        }
    } else {
        write(STDOUT_FILENO, "[WARNING] La cola aún tiene mensajes, no se eliminó.\n", 53);
    }
    write(STDOUT_FILENO, "[DEBUG] `read_from_msq` finalizado.\n", 36);
}

void send_to_msq(listElement2* element, int workerType) {
    if (element == NULL) {
        write(STDOUT_FILENO, "[ERROR] NULL element in send_to_message_queue.\n", 47);
        return;
    }

    write(STDOUT_FILENO, "[DEBUG] Iniciando `send_to_msq`...\n", 35);

    key_t key = ftok("worker.c", workerType);

    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("[ERROR] Error creating/getting message queue");
        return;
    }

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

    write(STDOUT_FILENO, "[DEBUG] Enviando mensaje a la cola...\n", 38);

    if (msgsnd(msqid, &msg, sizeof(MessageQueueElement) - sizeof(long), 0) == -1) {
        perror("[ERROR] Error sending message to queue");
    } else {
        write(STDOUT_FILENO, "[DEBUG] Message successfully sent to queue.\n", 44);
    }
}

// Crear o asegurar el archivo necesario para ftok
void ensure_worker_file_exists() {
    if (access(WORKER_FILE, F_OK) == -1) {
        // El archivo no existe, lo creamos
        FILE *f = fopen(WORKER_FILE, "w");
        if (f == NULL) {
            perror("[ERROR] Cannot create worker_count file");
            exit(1);
        }
        fclose(f);
    }
}

// Crear o obtener la MSQ
int get_worker_count_msq() {
    ensure_worker_file_exists(); // Asegurar que el archivo existe

    key_t key = ftok(WORKER_FILE, 1); // Clave única basada en el archivo
    if (key == -1) {
        perror("[ERROR] ftok failed");
        exit(1);
    }

    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("[ERROR] Error creating/getting worker count MSQ");
        exit(1);
    }
    return msqid;
}

void increment_worker_count() {
    int msqid = get_worker_count_msq();
    struct {
        long message_type;
    } msg = {1}; // Mensaje genérico con tipo 1

    if (msgsnd(msqid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("[ERROR] increment_worker_count: msgsnd failed");
    } 
}

void decrement_worker_count() {
    int msqid = get_worker_count_msq();
    struct {
        long message_type;
    } msg;

    if (msgrcv(msqid, &msg, sizeof(msg) - sizeof(long), 1, 0) == -1) {
        perror("[ERROR] decrement_worker_count: msgrcv failed");
    } 
}

int get_worker_count() {
    int msqid = get_worker_count_msq();
    struct msqid_ds buf;
    if (msgctl(msqid, IPC_STAT, &buf) == -1) {
        perror("[ERROR] get_worker_count: msgctl failed");
        return 0;
    }
    return buf.msg_qnum; // Número de mensajes en la cola
}

void delete_worker_count_msq() {
    int msqid = get_worker_count_msq();
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("[ERROR] delete_worker_count_msq: msgctl failed");
    } 
}

void* distortFileThread(void* arg) {
    listElement2* element = (listElement2*)arg;
    if (element == NULL) {
        write(STDOUT_FILENO, "[ERROR] distortFileThread: NULL element received.\n", 50);
        return NULL;
    }

    // Asignar el ID del hilo actual
    element->thread_id = pthread_self();
    write(STDOUT_FILENO, "[DEBUG] distortFileThread: Thread started.\n", 43);
    int i = 0;
    i = DISTORSION_distortFile(element, stop_signal);
    // Llamar a la función de distorsión
    if (i == 0 || i == 2) {
        // Determinar la lista objetivo
        LinkedList2 targetList = (strcmp(element->worker_type, "Media") == 0) ? listH : listE;
        if (!LINKEDLIST2_isEmpty(targetList)) {
            LINKEDLIST2_goToHead(targetList);
            while (!LINKEDLIST2_isAtEnd(targetList)) {
                listElement2* current = LINKEDLIST2_get(targetList);
                if (current == element) {
                    LINKEDLIST2_remove(targetList);
                    write(STDOUT_FILENO, "[DEBUG] distortFileThread: Element removed from list.\n", 54);

                    // Liberar memoria asociada al elemento
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
        } else {
            write(STDOUT_FILENO, "[DEBUG] distortFileThread: List is empty, nothing to remove.\n", 61);
        }
    } else {
        write(STDOUT_FILENO, "[ERROR] distortFileThread: Distortion failed.\n", 46);
    }

    write(STDOUT_FILENO, "[DEBUG] distortFileThread: Thread exiting.\n\n", 44);
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
        write(STDOUT_FILENO, "List not empty...\n", 19);
        LINKEDLIST2_goToHead(targetList);
        while (!LINKEDLIST2_isAtEnd(targetList)) {
            listElement2* element = LINKEDLIST2_get(targetList);
            write(STDOUT_FILENO, "Stopping thread...\n", 20);
            if (element->fd >= 0) {
                write(STDOUT_FILENO, "Closing fleck socket...\n", 25);
                if (element->status != 1) {
                    write(STDOUT_FILENO, "[DEBUG] doLogout: Sending CON_KO message to Fleck socket...\n", 60);
                    TRAMA_sendMessageToSocket(element->fd, 0x07, (int16_t)strlen("CON_KO"), "CON_KO");
                }
                close(element->fd);
                element->fd = -1;
                write(STDOUT_FILENO, "[DEBUG] doLogout: Worker socket closed.\n", 40);
            }
            write(STDOUT_FILENO, "Stopping thread...\n", 19);

            if (element->thread_id != 0) {
                pthread_join(element->thread_id, NULL); 
            }
            
            // Verificar si es el último Worker conectado
            int worker_count = get_worker_count();
            if (worker_count == 1) {
                delete_worker_count_msq(); // Eliminar la cola de mensajes
                write(STDOUT_FILENO, "[DEBUG] Last Worker disconnecting. Deleting MSQ.\n", 49);
            } else {
                if (strcmp(config.worker_type, "Media") == 0 && element->status == 2) {
                    send_to_msq(element, MEDIA);
                } else if (strcmp(config.worker_type, "Text") == 0 && element->status == 2) {
                    send_to_msq(element, TEXT);
                }
                decrement_worker_count(); // Reducir el contador de Workers
                write(STDOUT_FILENO, "[DEBUG] Other Workers still connected. Decrementing count.\n", 59);
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
    } else {
        int worker_count = get_worker_count();
        if (worker_count == 1) {
            write(STDOUT_FILENO, "[DEBUG] Last Worker disconnecting. Deleting MSQ.\n", 49);
            delete_worker_count_msq();
        } else {
            write(STDOUT_FILENO, "[DEBUG] Other Workers still connected. Decrementing count.\n", 59);
            decrement_worker_count();
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
    write(STDOUT_FILENO, "[DEBUG] connection_watcher: Started watching connection...\n", 59);

    while (1) {
        if (!SOCKET_isSocketOpen(sockfd)) {
            write(STDOUT_FILENO, "[DEBUG] connection_watcher: Connection to Gotham lost.\n", 55);
            close(sockfd);

            // Entramos en bucle para revisar el fleckSock
            while (1) {
                if (!SOCKET_isSocketOpen(fleckSock)) {
                    write(STDOUT_FILENO, "[DEBUG] connection_watcher: Fleck socket closed too. Calling CTRLC...\n", 70);
                    CTRLC(0);
                } 
                sleep(1); // Evitar bucle ultra-rápido consumiendo CPU
            }
        } 
        sleep(5); // Verifica cada 5 segundos
    }

    write(STDOUT_FILENO, "[DEBUG] connection_watcher: Exiting thread.\n", 44);
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

    increment_worker_count();
    int a = get_worker_count();
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