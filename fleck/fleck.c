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
int sockfd_G = -1, sockfd_W = -1;

// Variable global para almacenar el estado de las distorsiones
int ongoing_media_distortion = 0;
int ongoing_text_distortion = 0;

int connected = 0;

pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

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
    print_text("\n$ ");
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


void realFileDistorsion (int sockfd, char* fileName, char* fileSize) {
    char* path = (char*)malloc(sizeof(char) * (strlen(config.directory) + strlen(fileName) + 1));
    sprintf(path, "%s/%s", config.directory, fileName);
    write(STDOUT_FILENO, path, strlen(path));

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        write(STDOUT_FILENO, "Error: Cannot open file..\n", 27);
        return;
    }

    int bytes_written = 0;
    char* buf = (char*)malloc(sizeof(char) * 247);
    char* message = (char*)malloc(sizeof(char) * 256);
    struct trama ftrama;
    int sizeOfBuf = 0;
    printf("File size: %d\n", atoi(fileSize));
    while (atoi(fileSize) > bytes_written) {
        memset(buf, '\0', 247);
        memset(message, '\0', 256);
        sizeOfBuf = read(fd, buf, 247); 
        memcpy(message, buf, sizeOfBuf);
        bytes_written += sizeOfBuf;       
        pthread_mutex_lock(&myMutex);
        TRAMA_sendMessageToSocket(sockfd, 0x03, sizeOfBuf, message);
        pthread_mutex_unlock(&myMutex);
    }
    close(fd);
    free(buf);

    printf("\nBytes to write: %d\n", atoi(fileSize));
    printf("Bytes written: %d\n", bytes_written);
    if(TRAMA_readMessageFromSocket(sockfd, &ftrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        return;
    } else if (ftrama.tipo == 0x06  && strcmp((const char *)ftrama.data, "CHECK_OK") == 0) {
        write(STDOUT_FILENO, "File recieved in worker successfully. MD5sum came back OK.\n", 60);
    } else if (ftrama.tipo == 0x06  && strcmp((const char *)ftrama.data, "CHECK_KO") == 0) {
        write(STDOUT_FILENO, "Error: File could not be distorted.\n", 37);
    } else {
        write(STDOUT_FILENO, "Error: Invalid trama type.\n", 27);
        free(ftrama.data);
        return;
    } 
    free(ftrama.data);

    if(TRAMA_readMessageFromSocket(sockfd, &ftrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        return;
    } else if (ftrama.tipo != 0x04) {
        write(STDOUT_FILENO, "Error: Invalid trama type.\n", 27);
        free(ftrama.data);
        return;
    } 
    char* fileSize2 = STRING_getXFromMessage((const char *)ftrama.data, 0);
    char* distortedMd5 = STRING_getXFromMessage((const char *)ftrama.data, 1);
    printf("File size distorted: %s\n", fileSize2);
    printf("distortedMD5: %s\n", distortedMd5);

    free(ftrama.data);
    
    char* path2 = (char*)malloc(sizeof(char) * (strlen(config.directory) + strlen(fileName) + strlen(".//D")));
    sprintf(path2, "./%s/%sD", config.directory, fileName);

    int fd2 = open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd2 < 0) {
        perror("Failed to open file.");
        exit(EXIT_FAILURE);
    }
    printf("Path2: %s\n", path2);

    int bytes_written2 = 0, bytes_to_write = atoi(fileSize2);
    int i = 0;   
    while (bytes_written2 < bytes_to_write) {
        if(TRAMA_readMessageFromSocket(sockfd, &ftrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            return;
        } else if (ftrama.tipo == 0x07) {
            write(STDOUT_FILENO, "Fleck received a CTRL+C.\n", 26);
            return;
        } else if (ftrama.tipo != 0x05) {
            write(STDOUT_FILENO, "Error: Invalid trama type.\n", 27);
            return;
        }
        char* incoming_Info = NULL;
        if (bytes_to_write - bytes_written2 < 247) {
            incoming_Info = STRING_getSongCode((const char *)ftrama.data, bytes_to_write - bytes_written2);
            bytes_written2 += write(fd2, incoming_Info, bytes_to_write - bytes_written2);
        } else {
            incoming_Info = STRING_getSongCode((const char *)ftrama.data, 247);
            bytes_written2 += write(fd, incoming_Info, 247);
        }
        if (bytes_written2 < 0) {
            perror("Failed to write to file.");
            exit(EXIT_FAILURE);
        }
        free(incoming_Info);
        free(ftrama.data);
        i++;
    }
    printf("Value of i: %d\n", i);
    char* actualMd5 = DISTORSION_getMD5SUM(path2);
    printf("Actual MD5: %s\n", actualMd5);
    printf("Bytes to write: %d\n", atoi(fileSize2));
    printf("Bytes written: %d\n", bytes_written2);

    if(strcmp(distortedMd5, actualMd5) == 0) {
        TRAMA_sendMessageToSocket(sockfd, 0x06, (int16_t)strlen("CHECK_OK"), "CHECK_OK");  
    } else {
        TRAMA_sendMessageToSocket(sockfd, 0x06, (int16_t)strlen("CHECK_KO"), "CHECK_KO");  
        return;
    }

    remove(path2);
    close(fd2);
    free(path);
    free(path2);
    free(fileSize2);
    free(actualMd5);
    free(distortedMd5);
}

void sendSongInfo(int sockfd, char* filename, char* factor, char* fileSize, char* path) {
    int fds[2];
    pipe(fds);
    pid_t childPid = fork();

    if (childPid == 0) {
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);

        char *cmd = (char *)malloc(sizeof(char) * (strlen("md5sum ") + strlen(path) + 1));
        sprintf(cmd, "md5sum %s", path);
        
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
        printf("MD5: %s\n", actualMd5);
        printf("File size: %s\n", fileSize);
        sprintf(data, "%s&%s&%s&%s&%s", config.username, filename, fileSize, actualMd5, factor);
        TRAMA_sendMessageToSocket(sockfd, 0x03, (int16_t)strlen(data), data);
        free(data);
    }
}

void distortFile (char* type, char* filename, char* factor) {
    if (strcmp(type, "Media") == 0) {
        if (ongoing_media_distortion) {
            write(STDOUT_FILENO, "A media distortion is already in progress.\n", 44);
            TRAMA_sendMessageToSocket(sockfd_G, 0x10, (int16_t)strlen("CON_KO"), "CON_KO");
            return;
        } else {
            ongoing_media_distortion = 1; 
        }
    } else if (strcmp(type, "Text") == 0) {
        if (ongoing_text_distortion) {
            write(STDOUT_FILENO, "A text distortion is already in progress.\n", 43);
            TRAMA_sendMessageToSocket(sockfd_G, 0x10, (int16_t)strlen("CON_KO"), "CON_KO");
            return;
        } else {
            ongoing_text_distortion = 1; 
        }
    }

    char* data = (char*)malloc(256 * sizeof(char));

    sprintf(data, "%s&%s", type, filename);
    TRAMA_sendMessageToSocket(sockfd_G, 0x10, (int16_t)strlen(data), data);
    free(data);

    struct trama ftrama;
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
        sockfd_W = SOCKET_createSocket(port, ip);
        free(ftrama.data);  
        char* path = (char*)malloc(sizeof(char) * (strlen(config.directory) + strlen(filename) + 1));
        sprintf(path, "%s/%s", config.directory, filename);
        write(STDOUT_FILENO, path, strlen(path));
        char* fileSize = FILES_get_size_of_file(path);
        sendSongInfo(sockfd_W, filename, factor, fileSize, path);
        if(TRAMA_readMessageFromSocket(sockfd_W, &ftrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            return;
        }
        if(strcmp((const char *)ftrama.data, "CON_KO") == 0) {
            write(STDOUT_FILENO, "ERROR: File could not be distorted\n", 36);
        } else {
            if(ftrama.tipo == 0x03) {
                write(STDOUT_FILENO, "File starting to distort.\n", 27);
                realFileDistorsion(sockfd_W, filename, fileSize);
            }
        }
        free(ftrama.data);
        free(port);
        free(ip);
        close(sockfd_W);
        sockfd_W = -1;
    }

    // Reset 
    if (strcmp(type, "Media") == 0) {
        ongoing_media_distortion = 0;
    } else if (strcmp(type, "Text") == 0) {
        ongoing_text_distortion = 0;
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
    if (SOCKET_isSocketOpen(sockfd_W)) {
        write(STDOUT_FILENO, "Sending logout message to Worker server...\n", 43);
        TRAMA_sendMessageToSocket(sockfd_W, 0x07, (int16_t)strlen("CON_KO"), "CON_KO"); 
        close(sockfd_W);
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
                if(!SOCKET_isSocketOpen(sockfd_W)) {
                    CTRLC(0);
                }
            }
        }
        sleep(5); // Verifica cada 5 segundos
    }
    return NULL;
}


void terminal() {
    int words;

    while (1) {
        global_cmd = read_command(&words);

        if (strcmp(global_cmd, "CONNECT") == 0) {
            if(connected) {
                write(STDOUT_FILENO, "Already connected\n", 19);
            } else {
                if(connectToGotham()) {
                    connected = 1;
                    pthread_t watcher_thread;
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
            if(!connected) {
                write(STDOUT_FILENO, "Not connected\n", 15);
            } else {
                char* extracted = STRING_extract_substring(global_cmd);
                STRING_to_lowercase(extracted);
                char* type = FILES_file_exists_with_type(config.directory, extracted);
                char* factor = STRING_get_third_word(global_cmd);
                if(strcmp(type, "Neither") == 0) {  
                    write(STDOUT_FILENO, "File not found\n", 15);
                } else {
                    write(STDOUT_FILENO, "File exists.\n", 14);
                    distortFile(type, extracted, factor);
                }
                if (extracted != NULL) {
                    free(extracted);
                }
            }
        } else if (strcmp(global_cmd, "CHECK STATUS") == 0) {
            write(STDOUT_FILENO, "Command OK. Command not ready yet\n", 35);
        } else if (strcmp(global_cmd, "CLEAR ALL") == 0) {
            write(STDOUT_FILENO, "Command OK. Command not ready yet\n", 35);
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

    char* msg;

    asprintf(&msg, "\n%s user initialized\n", config.username);
    print_text(msg);
    free(msg);

    terminal();

    free_config();

    return 0;
}