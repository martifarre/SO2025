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

// Thread for connection watcher
pthread_t watcher_thread;
int watcher_thread_running = 0; // To track if watcher_thread was started

volatile sig_atomic_t shutdown_requested = 0; // Signal handling flag

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
    free(config.server_port);
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
        free(message);
        return 0;
    }

    write(STDOUT_FILENO, "Connected to Gotham\n", 21);
    TRAMA_sendMessageToSocket(sockfd_G, 0x01, (int16_t)strlen(config.username), config.username);

    struct trama ftrama;
    if (TRAMA_readMessageFromSocket(sockfd_G, &ftrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        close(sockfd_G);
        free(message);
        return 0;
    }

    free(ftrama.data);
    free(message);
    return 1;
}

void loopRecieveFileDistorted (int sockfd, char* filename) {
    struct trama ftrama;
    if(TRAMA_readMessageFromSocket(sockfd, &ftrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        return;
    }
    char* extracted = STRING_getXFromMessage((const char *)ftrama.data, 0);
    int amount = atoi(extracted); 
    free(ftrama.data);
    free(extracted);
    printf("Amount: %d\n", amount);
    TRAMA_sendMessageToSocket(sockfd, 0x03, 0, "");
    for(int i = 0; i < amount; i++) {
        if(TRAMA_readMessageFromSocket(sockfd, &ftrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            return;
        } else if(ftrama.tipo == 0x07) {
            write(STDOUT_FILENO, "Fleck recieved a CTRL+C.\n", 26);
            return;
        } else if(strcmp((const char *)ftrama.data, "Done") == 0) {
            write(STDOUT_FILENO, "Something is wrong.\n", 21);
            free(ftrama.data);
            break;
        } else {
            TRAMA_sendMessageToSocket(sockfd, 0x03, 0, "");
            free(ftrama.data);
        }
    }
    if(TRAMA_readMessageFromSocket(sockfd, &ftrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        return;
    } else if(strcmp((const char *)ftrama.data, "Done") == 0) {
        write(STDOUT_FILENO, "File distorted received correctly.\n", 36);
    } 
    free(ftrama.data);
}

void distortFile (char* type, char* filename) {
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

    struct trama ftrama;
    if(TRAMA_readMessageFromSocket(sockfd_G, &ftrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        free(data);
        return;
    }
    if(strcmp((const char *)ftrama.data, "DISTORT_KO") == 0) {
        write(STDOUT_FILENO, "ERROR: No worker for this type available.\n", 43);
        free(ftrama.data);
    } else {
        char *extracted1 = STRING_getXFromMessage((const char *)ftrama.data, 1);
        char *extracted2 = STRING_getXFromMessage((const char *)ftrama.data, 0);
        free(ftrama.data);  
        memset(data, '\0', 256);
        sprintf(data, "%s&%s", config.username, filename);
        sockfd_W = SOCKET_createSocket(extracted1, extracted2);
        TRAMA_sendMessageToSocket(sockfd_W, 0x03, (int16_t)strlen(data), data);
        if(TRAMA_readMessageFromSocket(sockfd_W, &ftrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            free(extracted1);
            free(extracted2);
            free(data);
            return;
        }
        if(strcmp((const char *)ftrama.data, "CON_KO") == 0) {
            write(STDOUT_FILENO, "ERROR: File could not be distorted\n", 36);
        } else {
            write(STDOUT_FILENO, "File starting to distort.\n", 27);
            free(ftrama.data);
            ftrama.data = NULL;
            loopRecieveFileDistorted(sockfd_W, filename);
            // If loopRecieveFileDistorted doesn't read ftrama here, no problem
        }

        if (ftrama.data) free(ftrama.data);
        free(extracted1);
        free(extracted2);
        close(sockfd_W);
        sockfd_W = -1;
    }

    if (strcmp(type, "Media") == 0) {
        ongoing_media_distortion = 0;
    } else if (strcmp(type, "Text") == 0) {
        ongoing_text_distortion = 0;
    }
    free(data);
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
    

    // Cleanup
    doLogout();
    connected = 0;

    shutdown_requested = 1;
    // Cancel watcher thread if running
    if (watcher_thread_running) {
        pthread_cancel(watcher_thread);
    }

    // Free global_cmd if allocated
    if (global_cmd != NULL) {
        free(global_cmd);
        global_cmd = NULL;
    }

    free_config();
    //join i despues haz un exit, porque si no hacemos un exit se queda en modo espera de comandos, como si hiceramos un logout
    pthread_join(watcher_thread, NULL);
    exit(0);
    // Do not raise SIGINT or reset signal handler.
    // We'll just let main return after terminal() ends.
}

void *connection_watcher(void* arg) {
    while (connected && !shutdown_requested) {
        if (!SOCKET_isSocketOpen(sockfd_G)) {
            write(STDOUT_FILENO, "Connection to Gotham lost\n", 27);
            connected = 0;
            close(sockfd_G);
            sockfd_G = -1;
            // If connection is lost, simulate a CTRL+C scenario or logout:
            shutdown_requested = 1;
            break;
        }
        sleep(5); // Sleep is a cancellation point
    }
    return NULL;
}

void terminal() {
    int words;

    while (!shutdown_requested) {
        global_cmd = read_command(&words);

        if (shutdown_requested) { 
            // If shutdown was requested during read_command()
            free(global_cmd);
            global_cmd = NULL;
            break;
        }

        if (strcmp(global_cmd, "CONNECT") == 0) {
            if(connected) {
                write(STDOUT_FILENO, "Already connected\n", 19);
            } else {
                if(connectToGotham()) {
                    connected = 1;
                    if (pthread_create(&watcher_thread, NULL, connection_watcher, NULL) != 0) {
                        write(STDOUT_FILENO, "Error: Cannot create thread\n", 29);
                    } else {
                        watcher_thread_running = 1;
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
                if(strcmp(type, "Neither") == 0) {  
                    write(STDOUT_FILENO, "File not found\n", 15);
                } else {
                    write(STDOUT_FILENO, "File exists.\n", 14);
                    distortFile(type, extracted);
                }
                free(extracted);
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
                connected = 0;
            } else {
                write(STDOUT_FILENO, "Bye. You were not connected.\n", 30);
                break;
            }
            
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

    // If watcher_thread was created and is still running, join it
    if (watcher_thread_running) {
        pthread_cancel(watcher_thread);
        pthread_join(watcher_thread, NULL);
    }

    // Cleanup if not done yet
    if (global_cmd != NULL) {
        free(global_cmd);
        global_cmd = NULL;
    }
    free_config();

    return 0;
}