/***********************************************
*
* @Proposito: Implementación del proceso Fleck.
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
#define _GNU_SOURCE
#include "../modules/project.h"

// Variable global para almacenar la configuración
FleckConfig config;
// Variable global para almacenar el comando leído
char *global_cmd = NULL;
int sockfd_G = -1, sockfd_E = -1, sockfd_H = -1;

// Variable global para almacenar el estado de las distorsiones
int ongoing_media_distortion = 0;
int ongoing_text_distortion = 0;

int connected = 0;

pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t watcher_thread;

LinkedList2 distortionsList; 

typedef struct {
    char* type;
    char* filename;
    char* factor;
    listElement2* element;  // Elemento de la lista que será modificado
} DistortionThreadParams;

/***********************************************
*
* @Finalidad: Liberar la memoria asignada dinámicamente para la configuración.
* @Parametros: Ninguno.
* @Retorno: Ninguno.
*
************************************************/
void free_config() {
    free(config.username);
    free(config.directory);
    free(config.server_ip);
    free(config.server_port); // Liberar la memoria del puerto
}

/**************************************************
 *
 * @Finalidad: Leer una línea completa desde la entrada estándar,
 *             dividirla en palabras separadas por espacios y contar cuántas hay.
 * @Parametros: out: words = puntero a entero donde se almacenará el número
 *                          de palabras encontradas en la línea.
 * @Retorno:    Puntero a una cadena dinámica con el
 *             comando completo (la línea leída). Devuelve NULL si ocurre
 *             un error o si no se ingresó ningún dato.
 *
 **************************************************/
char *read_command(int *words) {
    int read_bytes;
    print_text("\n");
    STRING_read_line(STDIN_FILENO, &read_bytes, &global_cmd);
    global_cmd = STRING_to_upper_case(global_cmd);
    STRING_replace(global_cmd, '\n', '\0');
    *words = STRING_count_words(global_cmd);
    STRING_strip_whitespace(global_cmd);
    return global_cmd;
}
/**************************************************
 *
 * @Finalidad: Establecer la conexión inicial desde el cliente Fleck
 *             al servidor Gotham, enviando el frame de registro
 *             y esperando confirmación.
 * @Parametros: ----.
 * @Retorno:    Descriptor de socket conectado a Gotham (>= 0) si la
 *             conexión y el handshake (TYPE=0x01) fueron exitosos;
 *             -1 en caso de error de creación de socket, conexión o
 *             respuesta negativa de Gotham (CON_KO).
 *
 **************************************************/
int connectToGotham() {
    char *message = (char *)malloc(256 * sizeof(char));
    sprintf(message, "\nConnecting %s...\n", config.username);
    write(STDOUT_FILENO, message, strlen(message));

    sockfd_G = SOCKET_createSocket(config.server_port, config.server_ip);
    if (sockfd_G < 0) {
        write(STDOUT_FILENO, "Error: Cannot connect to Gotham\n", 33);
        free(message);  // Liberar antes de retornar
        return 0;
    }

    write(STDOUT_FILENO, "Connected to Gotham\n", 21);
    TRAMA_sendMessageToSocket(sockfd_G, 0x01, (int16_t)strlen(config.username), config.username);

    struct trama ftrama;
    if (TRAMA_readMessageFromSocket(sockfd_G, &ftrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        close(sockfd_G);
        free(message);  // Liberar antes de retornar
        return 0;
    }

    free(ftrama.data);
    free(message);  // Liberar antes de retornar
    return 1;
}

/**************************************************
 *
 * @Finalidad: Gestionar de forma completa y síncrona la transferencia
 *             de un fichero de distorsión con un worker (Harley o Enigma):
 *               1. Enviar el frame de petición TYPE=0x03 con metadatos
 *                  (nombre, tamaño, MD5 original, factor) al socket del worker.
 *               2. Recibir el frame de respuesta TYPE=0x03 OK que autoriza
 *                  el envío de datos.
 *               3. Enviar el fichero en tramas TYPE=0x05 de tamaño fijo,
 *                  comprobando el número de bytes enviados.
 *               4. Tras completar la subida, recibir el frame TYPE=0x04
 *                  con metadatos del fichero distorsionado (nuevo tamaño
 *                  y MD5).
 *               5. Descargar el fichero resultante en fragmentos TYPE=0x05,
 *                  y reconstruirlo localmente, comprobando integridad.
 *               6. Finalizar con TYPE=0x06 CHECK_OK o gestionar error
 *                  en caso de CHECK_KO.
 * @Parametros: in: sockfd     = descriptor de socket conectado al worker.
 *              in: fileName   = nombre del fichero a distorsionar.
 *              in: fileSize   = cadena con el tamaño original en bytes.
 *              in/out: element = puntero a la estructura de estado que
 *                               almacena desplazamientos, MD5 y progreso.
 * @Retorno:    0 si todo el proceso de subida, procesamiento y descarga
 *             se completó correctamente; < 0 si ocurre cualquier error.
 *
 **************************************************/
int realFileDistorsion(int sockfd, char* fileName, char* fileSize, listElement2* element) {
    // Crear path del archivo
    char* path = NULL;
    if (asprintf(&path, "%s/%s", config.directory, fileName) == -1) return 1;
    char* fileSize2;
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        write(STDOUT_FILENO, "Error: Cannot open file..\n", 27);
        free(path);
        return 1;
    }

    if(element->status == 0) {
        element->bytes_writtenF1 = 0;
    }
    strncpy(element->fileName, fileName, sizeof(element->fileName) - 1);
    element->fileName[sizeof(element->fileName) - 1] = '\0';

    element->bytes_to_writeF1 = atoi(fileSize);

    int bytes_written = 0, bytes_to_write = element->bytes_to_writeF1;
    element->bytes_writtenF1 = bytes_written;
    char* buf = (char*)malloc(sizeof(char) * 247);
    char* message = (char*)malloc(sizeof(char) * 256);
    struct trama ftrama;
    int sizeOfBuf = 0;


    if(element->status == 0 || element->status == 1) {
        element->status = 1;
        while (bytes_to_write > bytes_written) {
            memset(buf, '\0', 247);
            memset(message, '\0', 256);
            sizeOfBuf = read(fd, buf, 247);
            memcpy(message, buf, sizeOfBuf);
            bytes_written += sizeOfBuf;
            element->bytes_writtenF1 = bytes_written;

            pthread_mutex_lock(&myMutex);

            //Verificar si el socket sigue abierto antes de enviar datos
            char test;
            int check = recv(sockfd, &test, 1, MSG_PEEK | MSG_DONTWAIT);

            //El worker ha cerrado la conexión
            if (check == 0) { 
                write(STDOUT_FILENO, "Worker connection closed.\n", 26);
                pthread_mutex_unlock(&myMutex);
                return 0;
            } else if (check < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
                perror("[ERROR] Conexión perdida");
                pthread_mutex_unlock(&myMutex);
                return 0;
            }

            //Enviar datos al worker (sin modificar TRAMA_sendMessageToSocket())
            TRAMA_sendMessageToSocket(sockfd, 0x03, sizeOfBuf, message);

            pthread_mutex_unlock(&myMutex);
            usleep(1);
        }
        element->status = 2;

        if (TRAMA_readMessageFromSocket(sockfd, &ftrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            return 1;
        } else if (ftrama.tipo == 0x06 && strcmp((const char *)ftrama.data, "CHECK_OK") == 0) {
            write(STDOUT_FILENO, "File received in worker successfully. MD5sum came back OK.\n", 60);
        } else if (ftrama.tipo == 0x06 && strcmp((const char *)ftrama.data, "CHECK_KO") == 0) {
            write(STDOUT_FILENO, "Error: File could not be distorted.\n", 37);
        } else if (ftrama.tipo == 0x07) {
            write(STDOUT_FILENO, "Worker received a CTRL+C.\n", 26);
            return 0;
        } else {
            write(STDOUT_FILENO, "Error: Invalid trama type. 1\n", 29);
            free(ftrama.data);
            return 1;
        }
        free(ftrama.data);
    }
    close(fd);
    free(buf);

    if(element->status == 2) {

        if (TRAMA_readMessageFromSocket(sockfd, &ftrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated. 2\n", 32);
            return 1;
        } else if (ftrama.tipo != 0x04 && ftrama.tipo != 0x07) {
            write(STDOUT_FILENO, "Error: Invalid trama type. 2\n", 29);
            free(ftrama.data);
            return 1;
        } else if (ftrama.tipo == 0x07) {
            write(STDOUT_FILENO, "Worker received a CTRL+C.\n", 26);
            return 0;
        }
        fileSize2 = STRING_getXFromMessage((const char *)ftrama.data, 0);
        element->bytes_to_writeF2 = atoi(fileSize2);
        element->distortedMd5 = STRING_getXFromMessage((const char *)ftrama.data, 1);
        free(ftrama.data);
    }

    char* path2 = NULL;
    if (asprintf(&path2, "%s/D%s", config.directory, fileName) == -1) return 1;
    
    
    // Verificar si el archivo ya existe y eliminarlo
    if (access(path2, F_OK) == 0) { // F_OK verifica si el archivo existe
        if (remove(path2) == 0) {
            write(STDOUT_FILENO, "Existing file deleted successfully.\n", 36);
        } else {
            perror("Failed to delete existing file");
            free(path2);
            return 1;
        }
    }    

    int fd2 = open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd2 < 0) {
        perror("Failed to open file.");
        exit(EXIT_FAILURE);
    }

    int bytes_written2 = 0, bytes_to_write2 = element->bytes_to_writeF2;
    element->status = 3;
    while (bytes_written2 < bytes_to_write2) {
        if (TRAMA_readMessageFromSocket(sockfd, &ftrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            return 1;
        } else if (ftrama.tipo == 0x07) {
            write(STDOUT_FILENO, "Worker received a CTRL+C.\n", 26);
            return 0;
        } else if (ftrama.tipo != 0x05) {
            write(STDOUT_FILENO, "Error: Invalid trama type. 3\n", 29);
            return 1;
        }

        char* incoming_Info = NULL;
        if(bytes_written2 < element->bytes_writtenF2) {
            bytes_written2 += 247;
        } else if (bytes_to_write2 - bytes_written2 < 247) {
            incoming_Info = STRING_getSongCode((const char *)ftrama.data, bytes_to_write2 - bytes_written2);
            bytes_written2 += write(fd2, incoming_Info, bytes_to_write2 - bytes_written2);
            element->bytes_writtenF2 = bytes_written2;
        } else {
            incoming_Info = STRING_getSongCode((const char *)ftrama.data, 247);
            bytes_written2 += write(fd2, incoming_Info, 247);
            element->bytes_writtenF2 = bytes_written2;
        }
        if (bytes_written2 < 0) {
            perror("Failed to write to file.");
            exit(EXIT_FAILURE);
        }
        free(incoming_Info);
        free(ftrama.data);
        usleep(1);
    }

    char* actualMd5 = DISTORSION_getMD5SUM(path2);

    if (strcmp(element->distortedMd5, actualMd5) == 0) {
        TRAMA_sendMessageToSocket(sockfd, 0x06, (int16_t)strlen("CHECK_OK"), "CHECK_OK");
        write(STDOUT_FILENO, "File distorted successfully.\n\n", 31);
    } else {
        TRAMA_sendMessageToSocket(sockfd, 0x06, (int16_t)strlen("CHECK_KO"), "CHECK_KO");
        write(STDOUT_FILENO, "Error: File could not be distorted.\n\n", 37);
    }

    //remove(path2);
    close(fd2);
    free(path);
    free(path2);
    free(fileSize2);
    free(actualMd5);

    element->status = 4;
    return 1;
}

/**************************************************
 *
 * @Finalidad: Enviar la trama de metadatos del fichero
 *             al socket del worker, incluyendo el MD5
 *             original y el factor de distorsión.
 * @Parametros: in: sockfd    = descriptor de socket conectado al worker.
 *              in: filename  = nombre del fichero a distorsionar.
 *              in: factor    = factor de distorsión.
 *              in: fileSize  = tamaño del fichero en bytes.
 *              in: path      = ruta completa del fichero a distorsionar.
 * @Retorno:    Ninguno.
 *
 **************************************************/
void sendSongInfo(int sockfd, char* filename, char* factor, char* fileSize, char* path) {
    int fds[2];
    pipe(fds);
    pid_t childPid = fork();

    if (childPid == 0) {
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);

        char *cmd = NULL;
        asprintf(&cmd, "md5sum %s", path);
        
        execlp("/bin/sh", "sh", "-c", cmd, NULL);
        exit(EXIT_FAILURE);
    } else {
        close(fds[1]);
        wait(NULL); 

        char actualMd5[33];
        read(fds[0], actualMd5, 32); 
        actualMd5[32] = '\0';

        close(fds[0]);
        char* data = (char*)malloc(256 * sizeof(char));
        sprintf(data, "%s&%s&%s&%s&%s", config.username, filename, fileSize, actualMd5, factor);
        TRAMA_sendMessageToSocket(sockfd, 0x03, (int16_t)strlen(data), data);
        free(data);
    }
}
/**************************************************
 *
 * @Finalidad: Proceso de distorsión de un fichero
 *             desde el cliente Fleck: conecta al worker adecuado
 *             según el tipo, envía los metadatos (nombre, factor),
 *             delega la transferencia de datos y actualiza el estado
 *             de la tarea en ‘element’.
 * @Parametros: in: type     = cadena que indica el tipo de distorsión
 *                             (Media o Text).
 *              in: filename = nombre del fichero a distorsionar.
 *              in: factor   = cadena con el factor de distorsión
 *              in/out: element = puntero a la estructura listElement2
 *                               donde se almacenan el progreso,
 *                               los offsets y el resultado final.
 * @Retorno:    ----.
 *
 **************************************************/
void distortFile (char* type, char* filename, char* factor, listElement2* element) {
    if (strcmp(type, "Media") == 0) {
        if (ongoing_media_distortion) {
            write(STDOUT_FILENO, "A media distortion is already in progress.\n", 44);
            TRAMA_sendMessageToSocket(sockfd_G, 0x10, (int16_t)strlen("CON_KO"), "CON_KO");
            LINKEDLIST2_remove(distortionsList);
            return;
        } else {
            ongoing_media_distortion = 1; 
        }
    } else if (strcmp(type, "Text") == 0) {
        if (ongoing_text_distortion) {
            write(STDOUT_FILENO, "A text distortion is already in progress.\n", 43);
            TRAMA_sendMessageToSocket(sockfd_G, 0x10, (int16_t)strlen("CON_KO"), "CON_KO");
            LINKEDLIST2_remove(distortionsList);
            return;
        } else {
            ongoing_text_distortion = 1; 
        }
    }

    char* data = NULL;
    char* filename_copy = strdup(filename); 
    if (asprintf(&data, "%s&%s", type, filename_copy) == -1) return;
    write(STDOUT_FILENO, data, strlen(data));
    TRAMA_sendMessageToSocket(sockfd_G, 0x10, (int16_t)strlen(data), data);
    free(data);

    struct trama ftrama;
    while(1) {
        if(TRAMA_readMessageFromSocket(sockfd_G, &ftrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            return;
        }
        if(strcmp((const char *)ftrama.data, "DISTORT_KO") == 0) {
            write(STDOUT_FILENO, "ERROR: No worker for this type available.\n", 43);
            free(ftrama.data);
            ongoing_media_distortion = 0;
        } else {
            char *port = STRING_getXFromMessage((const char *)ftrama.data, 1);
            char *ip = STRING_getXFromMessage((const char *)ftrama.data, 0);
            int s_fd = -1;
            s_fd = SOCKET_createSocket(port, ip);
            if(strcmp (type, "Media") == 0) {
                sockfd_H = s_fd;
            } else if(strcmp (type, "Text") == 0) {
                sockfd_E = s_fd;
            }
            free(ftrama.data);  
            char* path = NULL;
            if (asprintf(&path, "%s/%s", config.directory, filename_copy) == -1) return;

            char* fileSize = FILES_get_size_of_file(path);
            sendSongInfo(s_fd, filename_copy, factor, fileSize, path);
            free(path);
            if(TRAMA_readMessageFromSocket(s_fd, &ftrama) < 0) {
                write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
                return;
            }
            if(strcmp((const char *)ftrama.data, "CON_KO") == 0) {
                write(STDOUT_FILENO, "ERROR: File could not be distorted\n", 36);
            } else {
                if(ftrama.tipo == 0x03) {
                    write(STDOUT_FILENO, "File starting to distort.\n", 27);
                    if(realFileDistorsion(s_fd, filename_copy, fileSize, element) == 0) {
                        char* data2 = NULL;
                        if (asprintf(&data2, "%s&%s", type, filename_copy) == -1) return;
                        TRAMA_sendMessageToSocket(sockfd_G, 0x11, (int16_t)strlen(data2), data2);
                        free(data2);
                        sleep(1);
                    } else {
                        free(ftrama.data);
                        free(port);
                        free(ip);
                        close(s_fd);
                        s_fd = -1;
                        if(strcmp (type, "Media") == 0) {
                            sockfd_H = s_fd;
                            ongoing_media_distortion = 0;
                        } else if(strcmp (type, "Text") == 0) {
                            sockfd_E = s_fd;
                            ongoing_text_distortion = 0;
                        }
                        break;
                    }
                }
            }
            free(ftrama.data);
            free(port);
            free(ip);
            close(s_fd);
            s_fd = -1;
            if(strcmp (type, "Media") == 0) {
                sockfd_H = s_fd;
                ongoing_media_distortion = 0;
            } else if(strcmp (type, "Text") == 0) {
                sockfd_E = s_fd;
                ongoing_text_distortion = 0;
            }
        }
    }

}

/**************************************************
 *
 * @Finalidad: Desconectar al cliente Fleck del sistema.
 *             Realiza los siguientes pasos:
 *               1. Envía el frame de logout (TYPE=0x07) a Gotham
 *                  con el nombre de usuario.
 *               2. Cierra el socket de comunicación con Gotham.
 *               3. Libera los recursos asociados (memoria, listas de tareas).
 *               4. Vuelve al prompt del shell o finaliza el proceso.
 * @Parametros: ----.
 * @Retorno:    ----.
 *
 **************************************************/
void doLogout () {
    write(STDOUT_FILENO, "Logging out...\n", 15);
    if (SOCKET_isSocketOpen(sockfd_G)) {
        TRAMA_sendMessageToSocket(sockfd_G, 0x07, (int16_t)strlen(config.username), config.username);
        close(sockfd_G);
        sockfd_G = -1;
        write(STDOUT_FILENO, "Sending logout message to Gotham server...\n", 43);
    }
    if (SOCKET_isSocketOpen(sockfd_H)) {
        shutdown(sockfd_H, SHUT_WR);
        close(sockfd_H);
        sockfd_H = -1;
    }
    if(SOCKET_isSocketOpen(sockfd_E)) {
        write(STDOUT_FILENO, "Sending logout message to Enigma server...\n", 43);
        TRAMA_sendMessageToSocket(sockfd_E, 0x07, (int16_t)strlen("CON_KO"), "CON_KO"); 
        close(sockfd_E);
        sockfd_E = -1;
    }
}
/**************************************************
 *
 * @Finalidad: Manejador de la señal SIGINT (CTRL+C) para el cliente Fleck.
 *             Se invoca al presionar CTRL+C y debe:
 *               1. Si hay conexión activa con Gotham, enviar el frame de logout.
 *               2. Cerrar el socket de comunicación con Gotham.
 *               3. Liberar los recursos asignados (configuración, listas de tareas, etc.).
 *               4. Terminar la ejecución del proceso.
 * @Parametros: in: signum = número de la señal recibida (debe ser SIGINT).
 * @Retorno:    ----.
 *
 **************************************************/
void CTRLC(int signum) {
    print_text("\nInterrupt signal CTRL+C received\n");
    doLogout();
    if (global_cmd != NULL) {
        free(global_cmd);
        global_cmd = NULL;
    }
    free_config();
    LINKEDLIST2_destroy(&distortionsList);
    pthread_cancel(watcher_thread);
    pthread_join(watcher_thread, NULL);
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

/**************************************************
 *
 * @Finalidad: Hilo que vigila la conexión del cliente Fleck
 *             con el worker asignado.
 * @Parametros: ----.
 * @Retorno:    ----.
 *
 **************************************************/
void *connection_watcher() {
    while (connected) {                                 // Solo verifica mientras esté conectado
        if (!SOCKET_isSocketOpen(sockfd_G)) {
            write(STDOUT_FILENO, "Connection to Gotham server lost.\n", 34);
            connected = 0;
            close(sockfd_G);

            // Verificar si hay distorsiones en curso
            int distortions_in_progress = 1;
            while (distortions_in_progress) {
                distortions_in_progress = 0;

                pthread_mutex_lock(&myMutex);           // Proteger el acceso a la lista
                if (!LINKEDLIST2_isEmpty(distortionsList)) {
                    LINKEDLIST2_goToHead(distortionsList);
                    while (!LINKEDLIST2_isAtEnd(distortionsList)) {
                        listElement2* element = LINKEDLIST2_get(distortionsList);
                        if (element->status != 4) {     // Si no está completada
                            distortions_in_progress = 1;
                            break;
                        }
                        LINKEDLIST2_next(distortionsList);
                    }
                }
                pthread_mutex_unlock(&myMutex);

                if (distortions_in_progress) {
                    write(STDOUT_FILENO, "Waiting for distortions to finish...\n", 37);
                    sleep(1);                           // Esperar un segundo antes de volver a verificar
                }
            }

            // Cerrar los sockets de los Workers
        
            if (SOCKET_isSocketOpen(sockfd_E)) {
                write(STDOUT_FILENO, "Closing Enigma worker socket...\n", 32);
                close(sockfd_E);
                sockfd_E = -1;
            }
            if (SOCKET_isSocketOpen(sockfd_H)) {
                write(STDOUT_FILENO, "Closing Gotham worker socket...\n", 32);
                close(sockfd_H);
                sockfd_H = -1;
            }

            // Llamar a CTRLC para manejar la desconexión
            write(STDOUT_FILENO, "CTRL+C signal sent to main thread.\n", 35);
            CTRLC(0);
            break;
        } 

        sleep(5);                                   // Esperar antes de volver a verificar
    }

    return NULL;
}

/**************************************************
 *
 * @Finalidad: Mostrar al usuario el estado de todas las operaciones de distorsión
 *             que existen en la lista de tareas de Fleck, incluyendo tanto las
 *             que están en curso como las ya finalizadas. Para cada tarea imprime
 *             el porcentaje de avance y una barra gráfica.
 * @Parametros: ----.
 * @Retorno:    ----.
 *
 **************************************************/
void CHECK_STATUS() {
    if (LINKEDLIST2_isEmpty(distortionsList)) {
        write(STDOUT_FILENO, "No distortions in progress.\n", 28);
        return;
    }

    write(STDOUT_FILENO, "Distortion Status:\n", strlen("Distortion Status:\n"));

    char buffer[256];

    LINKEDLIST2_goToHead(distortionsList);
    while (!LINKEDLIST2_isAtEnd(distortionsList)) {
        listElement2* element = LINKEDLIST2_get(distortionsList);

        snprintf(buffer, sizeof(buffer), "File: %.248s\n", element->fileName);
        write(STDOUT_FILENO, buffer, strlen(buffer));

        float progress = 0.0;

        if (element->status == 0) {         // Inicialización
            write(STDOUT_FILENO, "Status: Initialization\n", strlen("Status: Initialization\n"));
        } else if (element->status == 1) {  // Fase de envío (0 - 50%)
            progress = ((float)element->bytes_writtenF1 / element->bytes_to_writeF1) * 50;
            snprintf(buffer, sizeof(buffer), "Status: (%.2f%%)\n", progress);
            write(STDOUT_FILENO, buffer, strlen(buffer));
        } else if (element->status == 2) {
            progress = 50;
            snprintf(buffer, sizeof(buffer), "Status: (%.2f%%)\n", progress);
            write(STDOUT_FILENO, buffer, strlen(buffer));
        } else if (element->status == 3) {  // Fase de distorsión (50 - 100%)
            progress = 50 + ((float)element->bytes_writtenF2 / element->bytes_to_writeF2) * 50;
            snprintf(buffer, sizeof(buffer), "Status: (%.2f%%)\n", progress);
            write(STDOUT_FILENO, buffer, strlen(buffer));
        } else if (element->status == 4) {  // Completado
            write(STDOUT_FILENO, "Status: Completed\n", strlen("Status: Completed\n"));
        }

        // Pasar al siguiente elemento en la LinkedList
        LINKEDLIST2_next(distortionsList);
    }
}

/**************************************************
 *
 * @Finalidad: Eliminar de la lista interna de tareas de distorsión
 *             aquellas que ya han finalizado, liberando los recursos
 *             asociados y dejando únicamente las operaciones en curso.
 * @Parametros: ----.
 * @Retorno:    ----.
 *
 **************************************************/
void CLEAR_ALL() {
    if (LINKEDLIST2_isEmpty(distortionsList)) {
        write(STDOUT_FILENO, "No distortions in the list.\n", 28);
        return;
    }

    int found = 0;  

    LINKEDLIST2_goToHead(distortionsList);

    while (!LINKEDLIST2_isAtEnd(distortionsList)) {
        listElement2* element = LINKEDLIST2_get(distortionsList);

        if (element->status == 4) { 
            LINKEDLIST2_remove(distortionsList);
            found = 1; 
        } else {
            LINKEDLIST2_next(distortionsList);
        }
    }

    if (!found) {
        write(STDOUT_FILENO, "No completed distortions to clear.\n", 36);
    } else {
        write(STDOUT_FILENO, "All completed distortions cleared.\n", 36);
    }
}

/**************************************************
 *
 * @Finalidad: Hilo destinado a gestionar la distorsión
 *               completa de un fichero solicitado por Fleck.
 * @Parametros: in: arg = puntero a la estructura `listElement2` que contiene
 *                     toda la información de la tarea a procesar.
 * @Retorno:    ----.
 *
 **************************************************/
void* distortFileThread(void* arg) {
    DistortionThreadParams* params = (DistortionThreadParams*)arg;
    distortFile(params->type, params->filename, params->factor, params->element);

    free(params);
    return NULL;
}

/**************************************************
 *
 * @Finalidad: Ejecutar el bucle principal del intérprete de comandos de Fleck,
 *             mostrando el prompt, leyendo entradas del usuario, parseando la
 *             orden y delegando en la función correspondiente (CONNECT, LIST,
 *             DISTORT, CHECK STATUS, CLEAR ALL, LOGOUT, etc.).
 *             Permite iterar hasta que el usuario finalice la sesión o pulse CTRL+C.
 * @Parametros: ----.
 * @Retorno:    ----
 *
 **************************************************/
void terminal() {
    char* type_copy;
    char* factor_copy;
    int words;

    while (1) {
        global_cmd = read_command(&words);

        if (strcmp(global_cmd, "CONNECT") == 0) {
            if(connected) {
                write(STDOUT_FILENO, "Already connected\n", 19);
            } else {
                if(connectToGotham()) {
                    connected = 1;
                    write(STDOUT_FILENO, "Connected to Gotham\n", 20);
                    // Lanzar el connection_watcher
                    if (pthread_create(&watcher_thread, NULL, connection_watcher, NULL) != 0) {
                        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
                    } else {
                        pthread_detach(watcher_thread);
                    }
                }
            }
        } else if (strcmp(global_cmd, "LIST MEDIA") == 0) {
            FILES_list_files(config.directory, "media files");
        } else if (strcmp(global_cmd, "LIST TEXT") == 0) {
            FILES_list_files(config.directory, "text files");
        } else if (strncmp(global_cmd, "DISTORT", 7) == 0 && words == 3) {
            if (!connected) {
                write(STDOUT_FILENO, "Not connected\n", 15);
            } else {
                char* extracted = STRING_extract_substring(global_cmd);
                STRING_to_lowercase(extracted);
                char* type = FILES_file_exists_with_type(config.directory, extracted);
                char* factor = STRING_get_third_word(global_cmd);

                if (strcmp(type, "Neither") == 0) {
                    write(STDOUT_FILENO, "File not found\n", 15);
                } else {
                    write(STDOUT_FILENO, "File exists.\n", 14);

                    char* extracted_copy = strdup(extracted);
                    type_copy = strdup(type);
                    factor_copy = strdup(factor);

                    //  Crear el nuevo elemento y agregarlo a la LinkedList ANTES de lanzar el hilo
                    listElement2* newElement = (listElement2*)malloc(sizeof(listElement2));
                    if (newElement == NULL) {
                        write(STDOUT_FILENO, "Error: Memory allocation failed for listElement.\n", 50);
                        return;
                    }

                    // Inicializar el nuevo elemento
                    newElement->fileName = extracted_copy;
                    newElement->status = 0;
                    newElement->bytes_writtenF1 = 0;
                    newElement->bytes_writtenF2 = 0;
                    newElement->bytes_to_writeF1 = 0;
                    newElement->bytes_to_writeF2 = 0;


                    // Agregarlo a la LinkedList
                    LINKEDLIST2_add(distortionsList, newElement);

                    // Preparar los parámetros del hilo
                    DistortionThreadParams* params = (DistortionThreadParams*)malloc(sizeof(DistortionThreadParams));
                    params->type = type_copy;
                    params->filename = extracted_copy;
                    params->factor = factor_copy;
                    params->element = newElement;  // Pasamos el puntero del nuevo elemento

                    // Crear el hilo que ejecutará `distortFileThread`
                    pthread_t thread_id;
                    if(pthread_create(&thread_id, NULL, distortFileThread, params) < 0) {
                        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
                    } else {
                        pthread_detach(thread_id);
                    }
                }
                if (factor != factor_copy) free(factor);  
            }
        } else if (strcmp(global_cmd, "CHECK STATUS") == 0) {
            if (!connected) {
                write(STDOUT_FILENO, "Not connected\n", 15);
            } else {
                CHECK_STATUS();
            }
        } else if (strcmp(global_cmd, "CLEAR ALL") == 0) {
            if (!connected) {
                write(STDOUT_FILENO, "Not connected\n", 15);
            } else {
                CLEAR_ALL();
            }
        } else if (strcmp(global_cmd, "LOGOUT") == 0) {
            write(STDOUT_FILENO, "Command OK. Bye bye.\n", 22);
            free(global_cmd);
            global_cmd = NULL;
            if(connected) {
                doLogout();
            } else {
                write(STDOUT_FILENO, "Bye. You were not connected.\n", 30);
            }
            connected = 0;
            break;
        } else {
            write(STDOUT_FILENO, "Unknown command\n", 17);
        }

        free(global_cmd);
        global_cmd = NULL;
    }
}

/**************************************************
 *
 * @Finalidad: Punto de entrada del cliente Fleck. Se encarga de:
 *             - Validar la recepción de la ruta del fichero de configuración
 *               como argumento de línea de comandos.
 *             - Leer y parsear la configuración (usuario, carpeta,
 *               IP y puerto de Gotham).
 *             - Registrar el manejador de SIGINT (CTRL+C) para desconexión limpia.
 *             - Iniciar el bucle del terminal (terminal()) para procesar
 *               comandos del usuario (CONNECT, LIST, DISTORT, etc.).
 *             - Al finalizar la sesión, realizar logout ordenado y
 *               liberar recursos antes de salir.
 * @Parametros: in: argc = número de argumentos (debe ser 2).
 *              in: argv = vector de cadenas:
 *                     argv[0] = nombre del ejecutable,
 *                     argv[1] = ruta al fichero de configuración de Fleck.
 * @Retorno:    0 si finaliza correctamente tras cerrar conexión y liberar recursos;
 *             distinto de 0 si ocurre un error en argumentos, lectura de
 *             configuración o conexión inicial.
 *
 **************************************************/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: Fleck <config_file>\n", 27);
        exit(1);
    }

    signal(SIGINT, CTRLC);
    
    config = READCONFIG_read_config_fleck(argv[1]);

    distortionsList = LINKEDLIST2_create();

    char* msg;

    asprintf(&msg, "\n%s user initialized\n", config.username);
    print_text(msg);
    free(msg);

    terminal();

    free_config();
    LINKEDLIST2_destroy(&distortionsList);
    return 0;
}