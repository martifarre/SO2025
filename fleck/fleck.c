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
#include "../utils/utils.h"
#include "fleck.h"

// Definición de la estructura FleckConfig para almacenar la configuración
typedef struct {
    char *username;
    char *directory;
    char *server_ip;
    char *server_port; // Cambiado a char*
} FleckConfig;

// Variable global para almacenar la configuración
FleckConfig config;
// Variable global para almacenar el comando leído
char *global_cmd = NULL;
int sockfd_G, sockfd_W;


// Variable global para almacenar el estado de las distorsiones
int ongoing_media_distortion = 0;
int ongoing_text_distortion = 0;

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

/***********************************************
*
* @Finalidad: Leer el archivo de configuración y almacenar los valores en la estructura FleckConfig.
* @Parametros: const char *config_file - Ruta del archivo de configuración.
* @Retorno: Ninguno.
*
************************************************/
void read_config_file(const char *config_file) {
    int fd = open(config_file, O_RDONLY);
    if (fd == -1) {
        write(STDOUT_FILENO, "Error: Cannot open configuration file\n", 39);
        exit(1);
    }

    config.username = readUntil(fd, '\n');
    if (strchr(config.username, '$') != NULL) {
        write(STDOUT_FILENO, "Error: Invalid character '$' in username\n", 41);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.username, '\r', '\0');

    if (config.username == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read username\n", 31);
        free_config();
        close(fd);
        exit(1);
    }

    config.directory = readUntil(fd, '\n');
    replace(config.directory, '\r', '\0');
    if (config.directory == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read directory\n", 32);
        free_config();
        close(fd);
        exit(1);
    }

    config.server_ip = readUntil(fd, '\n');
    replace(config.server_ip, '\r', '\0');
    if (config.server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read server IP\n", 33);
        free_config();
        close(fd);
        exit(1);
    }

    config.server_port = readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read server port\n", 34);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    close(fd);
}

// Extensiones de archivos de medios y texto
const char *media_extensions[] = {".wav", ".png", ".jpg", ".jpeg", ".mp3", ".mp4", NULL};
const char *text_extensions[] = {".txt", ".dat", NULL};


int has_extension(const char *filename, const char **extensions) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return 0;

    for (int i = 0; extensions[i] != NULL; i++) {
        if (strcasecmp(dot, extensions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}


void list_files(const char *directory, const char **extensions, const char *label) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    char **file_list = NULL;
    char* buffer;

    dir = opendir(directory);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            continue;
        }
        if (has_extension(entry->d_name, extensions)) {
            file_list = realloc(file_list, sizeof(char *) * (count + 1));
            file_list[count] = strdup(entry->d_name);
            count++;
        }
    }
    closedir(dir);

    asprintf(&buffer, "There are %d %s available:\n", count, label);
    print_text(buffer);
    free(buffer);

    for (int i = 0; i < count; i++) {
        asprintf(&buffer, "%d. %s\n", i + 1, file_list[i]);
        print_text(buffer);
        free(buffer);

        free(file_list[i]); 
    }
    free(file_list);
}


char *read_command(int *words) {
    int read_bytes;
    print_text("\n$ ");
    read_line(STDIN_FILENO, &read_bytes, &global_cmd);
    global_cmd = to_upper_case(global_cmd);
    replace(global_cmd, '\n', '\0');
    *words = count_words(global_cmd);
    strip_whitespace(global_cmd);
    return global_cmd;
}

int createSocket(char *incoming_Port, char* incoming_IP) {
    char* message = (char*)malloc(sizeof(char) * 256);
    sprintf(message, "Connecting %s...\n", config.username);
    write(STDOUT_FILENO, message, strlen(message));
    free(message);

    uint16_t port;
    int aux = atoi(incoming_Port);
    if (aux < 1 || aux > 65535) {
        write(STDOUT_FILENO, "Error: Invalid port\n", 21);
        exit (EXIT_FAILURE);

    }
    port = aux;

    struct in_addr ip_addr;
    if (inet_pton(AF_INET, incoming_IP, &ip_addr) != 1) {
        write(STDOUT_FILENO, "Error: Cannot convert IP address\n", 34);
        exit (EXIT_FAILURE);
    }

    int sockfd;
    sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        write(STDOUT_FILENO, "Error: Cannot create socket\n", 29);
        exit (EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    memset (&s_addr, '\0', sizeof (s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons (port);
    s_addr.sin_addr = ip_addr;

    if (connect (sockfd, (void *) &s_addr, sizeof (s_addr)) < 0) {
        write(STDOUT_FILENO, "Error: Cannot connect.\n", 45);
        sockfd = -1;
    }

    return sockfd;
}

// Función para comprobar si el archivo existe y devolver su tipo
char* file_exists_with_type(const char *directory, const char *file_name) {
    DIR *dir;
    struct dirent *entry;
    char *result = "Neither";  // Por defecto

    dir = opendir(directory);
    if (dir == NULL) {
        write(STDOUT_FILENO, "Error: Cannot open directory\n", 30);
        return "Neither"; // Error al abrir el directorio o directorio no existe
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            continue;
        }
        if (strcmp(entry->d_name, file_name) == 0) {
            if (has_extension(file_name, media_extensions)) {
                result = "Media";
            } else if (has_extension(file_name, text_extensions)) {
                result = "Text";
            }
            break;
        }
    }
    closedir(dir);
    return result;
}

int connectToGotham () {
    char *message = (char *)malloc(256 * sizeof(char));
    sockfd_G = createSocket(config.server_port, config.server_ip);
    if (sockfd_G < 0) {
        write(STDOUT_FILENO, "Error: Cannot connect to Gotham\n", 33);
        return 0;
    } else {
        write(STDOUT_FILENO, "Connected to Gotham\n", 21);
        sendMessageToSocket(sockfd_G, "1", config.username);
        memset(message, '\0', 256);
        read(sockfd_G, message, 256);
        return 1;
    }
}

void distortFile (char* type, char* filename) {
    if (strcmp(type, "Media") == 0) {
        if (ongoing_media_distortion) {
            write(STDOUT_FILENO, "A media distortion is already in progress.\n", 44);
            return;
        } else {
            ongoing_media_distortion = 1; 
        }
    } else if (strcmp(type, "Text") == 0) {
        if (ongoing_text_distortion) {
            write(STDOUT_FILENO, "A text distortion is already in progress.\n", 43);
            return;
        } else {
            ongoing_text_distortion = 1; 
        }
    }

    char *message = (char *)malloc(256 * sizeof(char));
    char* data = (char*)malloc(256 * sizeof(char));

    sprintf(data, "%s&%s", type, filename);
    sendMessageToSocket(sockfd_G, "9", data);
    read(sockfd_G, message, 256);
    if(message[3] == 'K') {
        write(STDOUT_FILENO, "ERROR: No worker for this type available.\n", 7);
    } else {
        sockfd_W = createSocket(getXFromMessage(message, 1), getXFromMessage(message, 0));
        sprintf(data, "%s&%s", config.username, filename);
        sendMessageToSocket(sockfd_W, "3", data);
        read(sockfd_W, message, 256);
        if(message[3] == 'K') {
            write(STDOUT_FILENO, "ERROR: File could not be distorted\n", 7);
        } else {
            write(STDOUT_FILENO, "File distorted correctly.\n", 16);
        }
        close(sockfd_W);
    }

    // Reset 
    if (strcmp(type, "Media") == 0) {
        ongoing_media_distortion = 0;
    } else if (strcmp(type, "Text") == 0) {
        ongoing_text_distortion = 0;
    }

    /*free(data);
    free(message);*/
}

char* extract_substring(char* global_cmd) {
    char* start = global_cmd + 8;
    char* end = strchr(start, ' ');

    if (end == NULL) {
        // Si no se encuentra un espacio, copiar hasta el final de la cadena
        return strdup(start);
    }

    size_t length = end - start;
    char* substring = (char*)malloc((length + 1) * sizeof(char));
    strncpy(substring, start, length);
    substring[length] = '\0';

    return substring;
}

void to_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}


void terminal() {
    int words;
    int connected = 0;

    while (1) {
        global_cmd = read_command(&words);

        if (strcmp(global_cmd, "CONNECT") == 0) {
            if(connected) {
                write(STDOUT_FILENO, "Already connected\n", 19);
            } else {
                if(connectToGotham()) {
                    connected = 1;
                }
            }
        } else if (strcmp(global_cmd, "LIST MEDIA") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
            list_files(config.directory, media_extensions, "media files");
        } else if (strcmp(global_cmd, "LIST TEXT") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
            list_files(config.directory, text_extensions, "text files");
        } else if (strncmp(global_cmd, "DISTORT", 7) == 0 && words == 3) {
            char* extracted = extract_substring(global_cmd);
            to_lowercase(extracted);
            char* type = file_exists_with_type(config.directory, extracted);
            if(strcmp(type, "Neither") == 0) {  
                write(STDOUT_FILENO, "File not found\n", 15);
            } else {
                write(STDOUT_FILENO, "File exists.\n", 12);
                distortFile(type, extracted);
            }
        } else if (strcmp(global_cmd, "CHECK STATUS") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
        } else if (strcmp(global_cmd, "CLEAR ALL") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
        } else if (strcmp(global_cmd, "LOGOUT") == 0) {
            write(STDOUT_FILENO, "Command OK\n", 12);
            free(global_cmd);
            global_cmd = NULL;
            close(sockfd_G);
            break;
        } else {
            write(STDOUT_FILENO, "Unknown command\n", 17);
        }

        free(global_cmd);
        global_cmd = NULL;
    }
}

void doLogout () {
    if (isSocketOpen(sockfd_G)) {
        sendMessageToSocket(sockfd_G, "7", username);
        close(sockfd_G);
    }
    if (isSocketOpen(sockfd_W)) {
        sendMessageToSocket(sockfd_W, "7", username);
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
    exit(1);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: Fleck <config_file>\n", 27);
        exit(1);
    }

    signal(SIGINT, CTRLC);
    
    read_config_file(argv[1]);

    char* msg;

    asprintf(&msg, "%s user initialized\n", config.username);
    print_text(msg);
    free(msg);

    terminal();

    free_config();

    return 0;
}