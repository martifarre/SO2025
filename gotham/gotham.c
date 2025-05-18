/***********************************************
*
* @Proposito: Implementación del proceso Gotham.
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
#define _GNU_SOURCE
#include "../modules/project.h"

// Variable global para almacenar la configuración
GothamConfig config;

LinkedList listW; 
LinkedList listF;

int gotham_flag = 0;

//Arkham pipe and mutex
int pipe_fds[2];
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int fleck_connecter_fd = -1, worker_connecter_fd = -1;

int pipefd[2];

/**************************************************
 *
 * @Finalidad: Liberar todos los recursos dinámicos asignados
 *             para la configuración de Gotham, incluyendo
 *             cadenas de IP y puerto y estructuras internas.
 * @Parametros: ----.
 * @Retorno:    ----.
 *
 **************************************************/
void free_config() {
    free(config.fleck_server_ip);
    free(config.fleck_server_port); // Liberar la memoria del puerto
    free(config.external_server_ip);
    free(config.external_server_port); // Liberar la memoria del puerto
}

int i = 0;

/**************************************************
 *
 * @Finalidad: Enviar un mensaje de evento al proceso logger (Arkham),
 *             formateando la entrada con timestamp y escribiéndola
 *             en el pipe de comunicación para su registro posterior.
 * @Parametros: in: event = cadena de texto que describe el evento ocurrido
 *                         
 * @Retorno:    ----.
 *
 **************************************************/
void log_event(const char *event) {
    write(STDOUT_FILENO, event, strlen(event));
    char timestamped_event[256];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Format: "[YYYY-MM-DD HH:MM:SS] <event>\n"
    snprintf(timestamped_event, sizeof(timestamped_event), "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec, event);

    pthread_mutex_lock(&log_mutex);
    write(pipe_fds[1], timestamped_event, strlen(timestamped_event));
    pthread_mutex_unlock(&log_mutex);
}

/**************************************************
 *
 * @Finalidad: Atender en Gotham la solicitud de distorsión de un cliente Fleck.
 *             Busca un worker del tipo indicado (Media o Texto) entre los
 *             trabajadores activos y envía al cliente la dirección (IP y puerto)
 *             del worker principal en una trama de respuesta. Si no hay ningún
 *             worker disponible, envía un mensaje de error (DISTORT_KO).
 * @Parametros: in: fleckSock = descriptor del socket conectado con el cliente Fleck.
 *              in: type      = cadena que indica el tipo de worker solicitado.
 *                              (Media o Texto).
 *              in: longitud = longitud en bytes del campo de datos de la trama
 *                            recibida originalmente (se usa para preparar la
 *                            respuesta correctamente).
 * @Retorno:    ----.
 *
 **************************************************/
void searchWorkerAndSendInfo(int fleckSock, char* type, uint16_t longitud) {
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
        TRAMA_sendMessageToSocket(fleckSock, longitud, (int16_t)strlen("DISTORT_KO"), "DISTORT_KO");
    } else {
        write(STDOUT_FILENO, "Worker found, sending to Fleck.\n\n", 33);
        sprintf(message, "%s&%s", element->ip, element->port);  
        TRAMA_sendMessageToSocket(fleckSock, longitud, (int16_t)strlen(message), message);
    }
    free(message);
}
/**************************************************
 *
 * @Finalidad: Función que se ejecuta en un hilo para gestionar de forma concurrente
 *             la sesión de un cliente Fleck conectado a Gotham. Atiende todas las
 *             solicitudes de ese cliente, incluyendo comandos de LIST, DISTORT y LOGOUT,
 *             redirigiendo las peticiones al worker correspondiente y enviando las respuestas.
 * @Parametros: in: arg = puntero a un entero (o estructura) que contiene el descriptor
 *                     de socket del cliente Fleck aceptado por Gotham.
 * @Retorno:    ----.
 *
 **************************************************/
void* threadFleck(void* arg) {
    int fleckSock = *(int*)arg;
    struct trama gtrama;

    // First message read from Fleck
    if (TRAMA_readMessageFromSocket(fleckSock, &gtrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        return NULL;
    }
    
    // Process the first message
    if (gtrama.tipo == 0x01) {
        char* username = STRING_getXFromMessage((const char *)gtrama.data, 0);
        
        free(gtrama.data); 
        gtrama.data = NULL;

        listElement* element = (listElement*)malloc(sizeof(listElement));
        element->sockfd = fleckSock;
        element->fleck_username = username;
        element->thread_id = pthread_self();
        LINKEDLIST_add(listF, element);
        char* data = (char*)malloc(sizeof(char) * 256);
        sprintf(data, "Fleck connected: username=%s", username);
        log_event(data);

        memset(data, '\0', 256);
        sprintf(data, "\nWelcome %s, you are connected to Gotham.\n\n", username);
        write(STDOUT_FILENO, data, strlen(data));
        TRAMA_sendMessageToSocket(fleckSock, 0x01, 0, "");
        free(data);

        //Ahora entra al bucle para leer los mensajes siguientes
        while (1) {
            int result = TRAMA_readMessageFromSocket(fleckSock, &gtrama);
            if(result == -2) {
                write(STDOUT_FILENO, "Thread Worker OUT.\n", strlen("Thread Worker OUT.\n"));
                return NULL;
            }
            if (result < 0) {
                write(STDOUT_FILENO, "Error: Checksum not validated....\n", 35);
                free(gtrama.data);
                break;  // Salir del bucle
            }
            
            // Procesar mensajes
            if (gtrama.tipo == 0x10 || gtrama.tipo == 0x11) {
                if (strcmp((const char *)gtrama.data, "CON_KO") == 0) {
                    write(STDOUT_FILENO, "Error: Distortion of this type already in progress.\n", 53);
                    free(gtrama.data); 
                    gtrama.data = NULL;
                } else {
                    char* type = STRING_getXFromMessage((const char *)gtrama.data, 0);
                    char* filename = STRING_getXFromMessage((const char *)gtrama.data, 1);
                    char* data = (char*)malloc(sizeof(char) * 256); 
                    if (gtrama.tipo == 0x10) {
                        sprintf(data, "Fleck requested distortion: username=%s, mediaType=%s, filename=%s", username, type, filename);
                    } else if (gtrama.tipo == 0x11) {
                        sprintf(data, "Fleck requested continue distortion: username=%s, textType=%s, filename=%s", username, type, filename);
                    }
                    log_event(data);
                    free(data);
                    free(filename);

                    free(gtrama.data); 
                    gtrama.data = NULL;
                    
                    searchWorkerAndSendInfo(fleckSock, type, gtrama.tipo);
                    
                    free(type);
                }
            } else if (gtrama.tipo == 0x07) {
                LINKEDLIST_goToHead(listF);
                while(!LINKEDLIST_isAtEnd(listF)) {
                    listElement* currentElement = LINKEDLIST_get(listF);
                    if(strcmp(currentElement->fleck_username, (const char *)gtrama.data) == 0 && currentElement->sockfd == fleckSock) {
                        free(gtrama.data); 
                        gtrama.data = NULL;
                        char* data = (char*)malloc(sizeof(char) * 256);
                        sprintf(data, "Fleck disconnected: username=%s", currentElement->fleck_username);
                        log_event(data);
                        free(data);
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
        // Si el primer mensaje no es 0x01, lo hemos leído pero no hemos utilizado gtrama.data:
        free(gtrama.data);
        gtrama.data = NULL;
    }

    close(fleckSock);
    return NULL;
}

/**************************************************
 *
 * @Finalidad: Hilo encargado de escuchar continuamente en el socket
 *             de Gotham para nuevas conexiones de clientes Fleck.
 *             Por cada conexión entrante, acepta el socket y lanza
 *             un nuevo hilo (threadFleck) para atender la sesión del cliente.
 * @Parametros: ----.
 * @Retorno:    ----.
 *
 **************************************************/
void* funcThreadFleckConnecter() {
    fleck_connecter_fd = SOCKET_initSocket(config.fleck_server_port, config.fleck_server_ip);
    
    int newsock;
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    // Poner el socket en modo NO BLOQUEANTE
    fcntl(fleck_connecter_fd, F_SETFL, O_NONBLOCK);

    while (1) {
        usleep(10000);  //Pequeña pausa para evitar alto consumo de CPU

        newsock = accept(fleck_connecter_fd, (struct sockaddr*)&c_addr, &c_len);
        if (newsock < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                if(gotham_flag == 1) {
                    write(STDOUT_FILENO, "Thread Fleck Connecter OUT.\n", strlen("Thread Fleck Connecter OUT.\n"));
                    return NULL;
                }
                continue;
            } else {
                perror("Error: Cannot accept connection");
                exit(EXIT_FAILURE);
            }
        }

        pthread_t thread_newFleck;
        pthread_create(&thread_newFleck, NULL, threadFleck, &newsock);
    }
}

/**************************************************
 *
 * @Finalidad: Función que se ejecuta en un hilo para gestionar la sesión
 *             de un worker (Enigma o Harley) conectado a Gotham. Entre sus
 *             responsabilidades están:
 *             - Leer el frame de registro inicial (TYPE=0x02) y añadir
 *               el worker a la lista de activos.
 *             - Mantener el estado del worker, atendiendo
 *               o detectando desconexiones inesperadas.
 *             - Al recibir una desconexión
 *               remueve el worker de la lista.
 * @Parametros: in: arg = puntero a entero que contiene el descriptor
 *                     de socket del worker aceptado por el socket
 *                     de escucha de Gotham.
 * @Retorno:    Devuelve NULL cuando el worker se desconecta o se produce
 *             un error de comunicación, indicando el fin de la ejecución
 *             del hilo.
 *
 **************************************************/
void* threadWorker(void* arg) {
    write(STDOUT_FILENO, "Worker connected\n\n", 19);
    int principal = 0;
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
        free(aux);                          // Liberar aux antes de salir
        return NULL;
    }

    if (gtrama.tipo == 0x02) {
        // Parsear los campos
        char* worker_type = STRING_getXFromMessage((const char *)gtrama.data, 0);
        char* ip          = STRING_getXFromMessage((const char *)gtrama.data, 1);
        char* port        = STRING_getXFromMessage((const char *)gtrama.data, 2);

        free(gtrama.data);                  // Liberar gtrama.data después de usarlo
        gtrama.data = NULL;

        if (!worker_type || !ip || !port) {  // Validar asignación de campos
            write(STDOUT_FILENO, "Error: Invalid worker message.\n", 32);
            free(worker_type);
            free(ip);
            free(port);
            free(aux);
            close(newsock);
            return NULL;
        }

        // Crear y añadir elemento a la lista
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
        element->principal = 0;
        element->thread_id = pthread_self();
        LINKEDLIST_add(listW, element);
        char* data = (char*)malloc(sizeof(char) * 256);
        sprintf(data, "%s connected: IP:%s:%s", worker_type, ip, port);
        log_event(data);
        free(data);
        sprintf(aux, "Worker of type %s added\n\n", worker_type);
        write(STDOUT_FILENO, aux, strlen(aux));
        TRAMA_sendMessageToSocket(newsock, 0x02, 0, "");

        int isFirst = 1;
        LINKEDLIST_goToHead(listW);
        while (!LINKEDLIST_isAtEnd(listW)) {
            listElement* worker = LINKEDLIST_get(listW);

            if (worker == element) {
                break;
            }
            if (strcmp(worker->worker_type, element->worker_type) == 0) {
                isFirst = 0;
                break;
            }
            LINKEDLIST2_next(listW);
        }
        if (isFirst) {
            element->principal = 1;
            TRAMA_sendMessageToSocket(element->sockfd, 0x08, 0, "");
        }

    } else {
        free(gtrama.data);
        gtrama.data = NULL;
        free(aux);
        close(newsock);
        return NULL;
    }

    free(aux);  // Liberar aux tras el uso inicial

    while (1) {
        int result = TRAMA_readMessageFromSocket(newsock, &gtrama);
        if(result == -2) {
            write(STDOUT_FILENO, "Thread Worker OUT.\n", strlen("Thread Worker OUT.\n"));
            return NULL;
        }
        if (result < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated....\n", 35);
            free(gtrama.data);
            break;  // Salir del bucle
        }

        if (gtrama.tipo == 0x07) {
            // Buscar y eliminar el elemento de la lista
            LINKEDLIST_goToHead(listW);
            while (!LINKEDLIST_isAtEnd(listW)) {
                listElement* currentElement = LINKEDLIST_get(listW);
                if (strcmp(currentElement->worker_type, (const char *)gtrama.data) == 0 
                    && currentElement->sockfd == newsock) {
                    char* data = (char*)malloc(sizeof(char) * 256);
                    principal = currentElement->principal;
                    sprintf(data, "%s disconnected: IP:%s:%s", currentElement->worker_type, currentElement->ip, currentElement->port);
                    log_event(data);
                    free(data);
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
            if (!LINKEDLIST_isEmpty(listW) && principal == 1) {
                LINKEDLIST_goToHead(listW);
                while (!LINKEDLIST_isAtEnd(listW)) {
                    listElement* firstWorker = LINKEDLIST_get(listW);
                    if (strcmp(firstWorker->worker_type, (const char *)gtrama.data) == 0) {
                        TRAMA_sendMessageToSocket(firstWorker->sockfd, 0x08, 0, "");
                        char* data = (char*)malloc(sizeof(char) * 256);
                        sprintf(data, "%s is now the principal worker: IP:%s:%s", firstWorker->worker_type, firstWorker->ip, firstWorker->port);
                        log_event(data);
                        free(data);
                        break;
                    }
                    LINKEDLIST_next(listW);
                }
            }
            free(gtrama.data);  // Liberar gtrama.data tras procesar
            gtrama.data = NULL;
            break;              // Salir del bucle
        }

        free(gtrama.data);      // Liberar gtrama.data tras procesar
        gtrama.data = NULL;
    }

    close(newsock);             // Cerrar el socket al final
    return NULL;
}
/**************************************************
 *
 * @Finalidad: Hilo encargado de escuchar de forma continua
 *             en el socket de Gotham para nuevas conexiones
 *             de workers (Enigma o Harley). Por cada conexión entrante:
 *               - Acepta el socket.
 *               - Lanza un hilo (threadWorker) para gestionar la sesión
 *                 de ese worker.
 * @Parametros: ----.
 * @Retorno:    Devuelve NULL al finalizar.
 *
 **************************************************/
void* funcThreadWorkerConnecter() {
    worker_connecter_fd = SOCKET_initSocket(config.external_server_port, config.external_server_ip);
    
    int newsock;
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    //Establecer el socket en modo NO BLOQUEANTE
    fcntl(worker_connecter_fd, F_SETFL, O_NONBLOCK);

    while (1) {
        usleep(10000);  // Pequeña espera para no consumir CPU constantemente

        newsock = accept(worker_connecter_fd, (struct sockaddr*)&c_addr, &c_len);
        if (newsock < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                if(gotham_flag == 1) {
                    write(STDOUT_FILENO, "Thread Worker Connecter OUT.\n", strlen("Thread Worker Connecter OUT.\n"));
                    return NULL;
                }
                continue;
            } else {
                perror("Error: Cannot accept connection");
                exit(EXIT_FAILURE);
            }
        }

        pthread_t thread_newWorker;
        pthread_create(&thread_newWorker, NULL, threadWorker, &newsock);
    }
}

/**************************************************
 *
 * @Finalidad: Detiene el servidor Gotham y desconectar
 *             todas las sesiones activas de Fleck y de workers.
 *             Para cada conexión:
 *               1. Envía la cadena "OUT" directamente por el socket.
 *               2. Realiza shutdown(SHUT_WR) y close() del descriptor.
 *               3. Espera a la terminación del hilo asociado (pthread_join).
 *               4. Libera los recursos de memoria y elimina el nodo de la lista.
 * @Parametros: ----
 * @Retorno:    ----.
 *
 **************************************************/
void doLogout() {
    gotham_flag = 1; // Indicar que Gotham está cerrando

    // Procesar la lista de conexiones de Fleck
    write(STDOUT_FILENO, "[DEBUG] doLogout: Processing Fleck connections...\n", 50);
    LINKEDLIST_goToHead(listF);
    while (!LINKEDLIST_isAtEnd(listF)) {
        listElement* currentElement = LINKEDLIST_get(listF);

        // Cerrar el socket de manera ordenada
        shutdown(currentElement->sockfd, SHUT_WR);
        close(currentElement->sockfd);
        write(STDOUT_FILENO, "[DEBUG] doLogout: Shutting down Fleck socket...\n", 48);

        // Detach el hilo asociado
        pthread_detach(currentElement->thread_id);
        write(STDOUT_FILENO, "[DEBUG] doLogout: Detaching thread for Fleck socket...\n", 55);

        // Liberar memoria asociada al elemento
        free(currentElement->fleck_username);
        free(currentElement);
        LINKEDLIST_remove(listF);
        write(STDOUT_FILENO, "[DEBUG] doLogout: Removed Fleck connection from list.\n", 54);
    }

    // Procesar la lista de conexiones de Workers
    write(STDOUT_FILENO, "[DEBUG] doLogout: Processing Worker connections...\n", 51);
    LINKEDLIST_goToHead(listW);
    while (!LINKEDLIST_isAtEnd(listW)) {
        listElement* currentElement = LINKEDLIST_get(listW);

        // Cerrar el socket de manera ordenada
        shutdown(currentElement->sockfd, SHUT_WR);
        close(currentElement->sockfd);
        write(STDOUT_FILENO, "[DEBUG] doLogout: Shutting down Worker socket...\n", 49);

        // Detach el hilo asociado
        pthread_detach(currentElement->thread_id);
        write(STDOUT_FILENO, "[DEBUG] doLogout: Detaching thread for Worker socket...\n", 56);

        // Liberar memoria asociada al elemento
        free(currentElement->ip);
        free(currentElement->port);
        free(currentElement->worker_type);
        free(currentElement);
        LINKEDLIST_remove(listW);
        write(STDOUT_FILENO, "[DEBUG] doLogout: Removed Worker connection from list.\n", 55);
    }

    write(STDOUT_FILENO, "All connections closed.\n", 24);
}
/**************************************************
 *
 * @Finalidad: Manejador de la señal SIGINT (CTRL+C) para el servidor Gotham.
 *             Se invoca al recibir la señal.
 * @Parametros: in: signum = número de la señal capturada (debe ser SIGINT).
 * @Retorno:    ----.
 *
 **************************************************/
void CTRLC(int signum) {
    print_text("\nInterrupt signal CTRL+C received\n");
    close(pipe_fds[1]); // Close the write end of the pipe
    doLogout();
    close(fleck_connecter_fd);
    close(worker_connecter_fd);
    LINKEDLIST_destroy(&listF);
    LINKEDLIST_destroy(&listW);
    free_config();

    wait(NULL);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

/**************************************************
 *
 * @Finalidad: Manejador de la señal SIGINT (CTRL+C) para el proceso Arkham.
 *             Se invoca al recibir la señal.
 * @Parametros: in: signum = número de la señal capturada (debe ser SIGINT).
 * @Retorno:    ----.
 *
 **************************************************/
void signalHandlerArkham(int signum) {
    close(pipe_fds[0]); 

    exit(EXIT_SUCCESS);
}

/**************************************************
 *
 * @Finalidad: Función que se ejecuta en el proceso Arkham para escribir
 *             los mensajes de log en el archivo logs.txt. Lee del pipe
 *             y escribe en el archivo de log.
 * @Parametros: in: pipe_fd = descriptor del pipe de comunicación.
 * @Retorno:    ----.
 *
 **************************************************/
void write_to_log(int pipe_fd) {
    

    char buffer[256];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1)) > 0) {
        int log_fd = open("logs.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (log_fd < 0) {
            perror("Error opening logs.txt");
            exit(1);
        }
        
        buffer[bytes_read] = '\0'; // Null-terminate the string
        write(log_fd, buffer, strlen(buffer));
        close(log_fd);
    }

    
    close(pipe_fd);
    
}

/**************************************************
 *
 * @Finalidad: Punto de entrada del servidor Gotham. Se encarga de:
 *             - Validar argumentos (ruta al fichero de configuración).
 *             - Leer y parsear la configuración de Gotham (IPs y puertos de Fleck y workers).
 *             - Iniciar el logger Arkham (fork y pipe).
 *             - Crear y lanzar hilos para:
 *                 * Aceptar conexiones de clientes Fleck (funcThreadFleckConnecter).
 *                 * Aceptar conexiones de workers (funcThreadWorkerConnecter).
 *             - Mantener el servicio activo hasta recibir SIGINT.
 *             - Al cerrar, invocar doLogout() y esperar a que terminen los hilos.
 * @Parametros: in: argc = número de argumentos (debe ser 2).
 *              in: argv = vector de cadenas:
 *                     argv[0] = nombre del ejecutable,
 *                     argv[1] = ruta al fichero de configuración de Gotham.
 * @Retorno:    0 si finaliza sin errores tras desconectar todas las sesiones;
 *             distinto de 0 en caso de fallo de argumentos, configuración o inicialización.
 *
 **************************************************/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: Gotham <config_file>\n", 28);
        exit(1);
    }

    signal(SIGINT, CTRLC);

    config = READCONFIG_read_config_gotham(argv[1]);
    if (pipe(pipe_fds) == -1) {
        perror("Error creating pipe");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd) == -1) {
        perror("[ERROR] No se pudo crear la pipe");
        exit(EXIT_FAILURE);
    }

    // Fork to create Arkham process
    pid_t pid = fork();
    if (pid < 0) {
        perror("Error forking Arkham process");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process: Arkham
        signal(SIGINT, signalHandlerArkham);
        close(pipe_fds[1]); // Close unused write end
        write_to_log(pipe_fds[0]);
        exit(EXIT_SUCCESS);
    } else {
        // Parent process: Gotham
        close(pipe_fds[0]); // Close unused read end

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
    }

    return 0;
}