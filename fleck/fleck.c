//Ignacio Giral ignacio.giral
//Marti Farre marti.farre

/***********************************************
*
* @Proposito: Implementació del procés Fleck
* @Autor/s: Ignacio Giral, Marti Farre (ignacio.giral, marti.farrea)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 17/10/2024
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

    int bytes_written = element->bytes_writtenF1, bytes_to_write = element->bytes_to_writeF1;
    char* buf = (char*)malloc(sizeof(char) * 247);
    char* message = (char*)malloc(sizeof(char) * 256);
    struct trama ftrama;
    int sizeOfBuf = 0;
    printf("Bytes to write: %d\n", bytes_to_write);
    printf("Bytes written: %d\n", bytes_written);
    printf("Status: %d\n", element->status);


    if(element->status == 0 || element->status == 1) {

        if (bytes_written > 0) {
            lseek(fd, bytes_written, SEEK_SET);
            printf("[DEBUG] Saltando %d bytes, comenzando desde la posición correcta.\n", bytes_written);
        }
        element->status = 1;
        while (bytes_to_write > bytes_written) {
            memset(buf, '\0', 247);
            memset(message, '\0', 256);
            sizeOfBuf = read(fd, buf, 247);
            memcpy(message, buf, sizeOfBuf);
            bytes_written += sizeOfBuf;
            element->bytes_writtenF1 = bytes_written;

            pthread_mutex_lock(&myMutex);

            // 🔹 Verificar si el socket sigue abierto antes de enviar datos
            char test;
            int check = recv(sockfd, &test, 1, MSG_PEEK | MSG_DONTWAIT);

            if (check == 0) {  // 🔹 El worker ha cerrado la conexión
                printf("[ERROR] Worker ha cerrado la conexión.\n");
                pthread_mutex_unlock(&myMutex);
                return 0;
            } else if (check < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
                perror("[ERROR] Conexión perdida");
                pthread_mutex_unlock(&myMutex);
                return 0;
            }

            // 🔹 Enviar datos al worker (sin modificar `TRAMA_sendMessageToSocket()`)
            TRAMA_sendMessageToSocket(sockfd, 0x03, sizeOfBuf, message);

            pthread_mutex_unlock(&myMutex);
            usleep(1000);
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
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
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

    int fd2 = open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd2 < 0) {
        perror("Failed to open file.");
        exit(EXIT_FAILURE);
    }

    int bytes_written2 = element->bytes_writtenF2, bytes_to_write2 = element->bytes_to_writeF2;
    printf("Bytes to write: %d\n", bytes_to_write2);
    printf("Bytes written: %d\n", bytes_written2);
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
        if (bytes_to_write2 - bytes_written2 < 247) {
            incoming_Info = STRING_getSongCode((const char *)ftrama.data, bytes_to_write2 - bytes_written2);
            bytes_written2 += write(fd2, incoming_Info, bytes_to_write2 - bytes_written2);
        } else {
            incoming_Info = STRING_getSongCode((const char *)ftrama.data, 247);
            bytes_written2 += write(fd2, incoming_Info, 247);
        }
        if (bytes_written2 < 0) {
            perror("Failed to write to file.");
            exit(EXIT_FAILURE);
        }
        free(incoming_Info);
        free(ftrama.data);
        element->bytes_writtenF2 = bytes_written2;
        usleep(1000);
    }

    char* actualMd5 = DISTORSION_getMD5SUM(path2);
    printf("Bytes written: %d\n", bytes_written2);
    printf("Bytes to write: %d\n", bytes_to_write2);
    printf("MD5SUM: %s\n", actualMd5);
    printf("Distorted MD5SUM: %s\n", element->distortedMd5);

    if (strcmp(element->distortedMd5, actualMd5) == 0) {
        TRAMA_sendMessageToSocket(sockfd, 0x06, (int16_t)strlen("CHECK_OK"), "CHECK_OK");
        write(STDOUT_FILENO, "File distorted successfully.\n\n", 31);
    } else {
        TRAMA_sendMessageToSocket(sockfd, 0x06, (int16_t)strlen("CHECK_KO"), "CHECK_KO");
        write(STDOUT_FILENO, "Error: File could not be distorted.\n\n", 37);
    }

    remove(path2);
    close(fd2);
    free(path);
    free(path2);
    free(fileSize2);
    free(actualMd5);

    element->status = 4;
    return 1;
}

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
    printf("Filename: %s\n", filename_copy);
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

void doLogout () {
    write(STDOUT_FILENO, "Logging out...\n", 15);
    if (SOCKET_isSocketOpen(sockfd_G)) {
        TRAMA_sendMessageToSocket(sockfd_G, 0x07, (int16_t)strlen(config.username), config.username);
        close(sockfd_G);
        sockfd_G = -1;
        write(STDOUT_FILENO, "Sending logout message to Gotham server...\n", 43);
    }
    if (SOCKET_isSocketOpen(sockfd_H)) {
        write(STDOUT_FILENO, "Sending logout message to Worker server...\n", 43);
        TRAMA_sendMessageToSocket(sockfd_H, 0x07, (int16_t)strlen("CON_KO"), "CON_KO"); 
        close(sockfd_H);
        sockfd_H = -1;
    }
    if(SOCKET_isSocketOpen(sockfd_E)) {
        write(STDOUT_FILENO, "Sending logout message to Worker server...\n", 43);
        TRAMA_sendMessageToSocket(sockfd_E, 0x07, (int16_t)strlen("CON_KO"), "CON_KO"); 
        close(sockfd_E);
        sockfd_E = -1;
    }
}

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

void *connection_watcher() {
    while (connected) { // Solo verifica mientras esté conectado
        if (!SOCKET_isSocketOpen(sockfd_G)) {
            write(STDOUT_FILENO, "Connection to Gotham lost\n", 27);
            connected = 0;
            close(sockfd_G);
            while(1) {
                if(!SOCKET_isSocketOpen(sockfd_E) || !SOCKET_isSocketOpen(sockfd_H)) {
                    CTRLC(0);
                }
            }
        }
        sleep(5); // Verifica cada 5 segundos
    }
    return NULL;
}

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

        if (element->status == 0) {  // Inicialización
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

void* distortFileThread(void* arg) {
    DistortionThreadParams* params = (DistortionThreadParams*)arg;
    distortFile(params->type, params->filename, params->factor, params->element);

    free(params);
    return NULL;
}

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
                    if(pthread_create(&watcher_thread, NULL, connection_watcher, NULL) != 0) {
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