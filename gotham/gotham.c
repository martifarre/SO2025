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
#include "../utils/utils.h"


// Definición de la estructura GothamConfig para almacenar la configuración
typedef struct {
    char *fleck_server_ip;
    char *fleck_server_port; // Cambiado a char*
    char *external_server_ip;
    char *external_server_port; // Cambiado a char*
} GothamConfig;

// Variable global para almacenar la configuración
GothamConfig config;

LinkedList workers;

void free_config() {
    free(config.fleck_server_ip);
    free(config.fleck_server_port); // Liberar la memoria del puerto
    free(config.external_server_ip);
    free(config.external_server_port); // Liberar la memoria del puerto
}


void read_config_file(const char *config_file) {
    int fd = open(config_file, O_RDONLY);
    
    if (fd == -1) {
        write(STDOUT_FILENO, "Error: Cannot open configuration file\n", 39);
        exit(1);
    }

    config.fleck_server_ip = readUntil(fd, '\n');
    replace(config.fleck_server_ip, '\r', '\0');
    if (config.fleck_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Fleck server IP\n", 39);
        close(fd);
        exit(1);
    }

    config.fleck_server_port = readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.fleck_server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Fleck server port\n", 41);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.fleck_server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    config.external_server_ip = readUntil(fd, '\n');
    replace(config.external_server_ip, '\r', '\0');
    if (config.external_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Harley server IP\n", 40);
        free_config();
        close(fd);
        exit(1);
    }

    config.external_server_port = readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.external_server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Harley server port\n", 42);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.external_server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    close(fd);
}

int initSocket(char* incoming_Port, char* incoming_IP) {
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

void searchWorkerAndSendInfo(int fleckSock, char* type) {
    Worker* worker = NULL;
    char* message = (char*)malloc(sizeof(char) * 256);
    char* data = (char*)malloc(sizeof(char) * 256);
    
    if (LINKEDLIST_isEmpty(workers)) {
        write(STDOUT_FILENO, "No workers available\n", 21);
        sendMessageToSocket(fleckSock, "9", "KO");
        return;
    }

    LINKEDLIST_goToHead(workers);

    while(!LINKEDLIST_isAtEnd(workers)) {
        Worker* currentWorker = LINKEDLIST_get(workers);
        if(strcmp(currentWorker->worker_type, type) == 0) {
            worker = currentWorker;
            break;
        }
        LINKEDLIST_next(workers);
    }

    if(worker == NULL) {
        sprintf(message, "No workers of type %s available\n", type);
        write(STDOUT_FILENO, message, strlen(message));
        sendMessageToSocket(fleckSock, "9", "KO");
    } else {
        write(STDOUT_FILENO, "Worker found\n", 13);
        sprintf(data, "%s&%s", worker->ip, worker->port);  
        sendMessageToSocket(fleckSock, "9", data);
    }
}


void* threadFleck(void* arg) {
    int fleckSock = *(int*)arg;
    char* message = (char*)malloc(sizeof(char) * 256);
    memset(message, '\0', 256);
    read(fleckSock, message, 256);
    write(STDOUT_FILENO, message, strlen(message));

    if(message[0] == '1') {
        char* username = getXFromMessage(message, 0);
        char* data = (char*)malloc(sizeof(char) * 256);
        sprintf(data, "Welcome %s, you are connected to Gotham.", username);
        write(STDOUT_FILENO, data, strlen(data));
        sendMessageToSocket(fleckSock, "1", "OK");

        memset(message, '\0', 256);
        while(1) {
            read(fleckSock, message, 256);
            if(message[0] == '9') {
                char* type = getXFromMessage(message, 0);
                //char* filename = getXFromMessage(message, 1);
                searchWorkerAndSendInfo(fleckSock, type);
            }
        }
    }

    close(fleckSock);
    return NULL;
}

void* funcThreadFleckConnecter() {
    int socket_fd = initSocket(config.fleck_server_port, config.fleck_server_ip);
    
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

void* funcThreadWorkers() {
    int socket_fd = initSocket(config.external_server_port, config.external_server_ip);
    
    char* message = (char*)malloc(sizeof(char) * 256);
    char* aux = (char*)malloc(sizeof(char) * 256);
    int newsock;
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof (c_addr);

    while(1) {
        newsock = accept(socket_fd, (void *) &c_addr, &c_len);
        if (newsock < 0) {
            write(STDOUT_FILENO, "Error: Cannot accept connection\n", 33);
            exit (EXIT_FAILURE);
        }

        memset(message, '\0', 256);
        read(newsock, message, 256);

        if(message[0] == '2') {
            char* worker_type = getXFromMessage(message, 0);
            char* ip = getXFromMessage(message, 1);
            char* port = getXFromMessage(message, 2);

            Worker* worker = (Worker*)malloc(sizeof(Worker));
            worker->ip = ip;
            worker->port = port;
            worker->worker_type = worker_type;

            LINKEDLIST_add(workers, worker);
            sprintf(aux, "Worker of type %s added\n", worker_type);
            write(STDOUT_FILENO, aux, strlen(aux));
            sendMessageToSocket(newsock, "2", "OK");
        }

        /*
        if (validate_checksum(message)) {
            write(STDOUT_FILENO, "Checksum valid\n", 15);
        } else {
            write(STDOUT_FILENO, "Checksum invalid\n", 17);
        }
        */


        close(newsock);
    }
}

void doLogout() {

}

void CTRLC(int signum) {
    print_text("\nInterrupt signal CTRL+C received\n");
    doLogout();
    free_config();
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: Gotham <config_file>\n", 28);
        exit(1);
    }

    signal(SIGINT, CTRLC);

    read_config_file(argv[1]);

    char *msg;

    asprintf(&msg, "Gotham server initialized.\n Waiting for connections...\n");
    print_text(msg);
    free(msg);

    workers = LINKEDLIST_create();

    pthread_t thread_Workers;
    pthread_t thread_FleckConnecter;

    if (pthread_create(&thread_Workers, NULL, funcThreadWorkers, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
        return 1;
    }

    if(pthread_create(&thread_FleckConnecter, NULL, funcThreadFleckConnecter, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
        return 1;
    }
        
    if (pthread_join(thread_Workers, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot join thread\n", 27);
        return 1; 
    }

    if (pthread_join(thread_FleckConnecter, NULL) != 0) {
        write(STDOUT_FILENO, "Error: Cannot join thread\n", 27);
        return 1; 
    }

    free_config();

    return 0;
}