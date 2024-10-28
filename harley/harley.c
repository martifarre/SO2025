//Ignacio Giral ignacio.giral
//Marti Farre marti.farre

/***********************************************
*
* @Proposito: Implementació del procés Harley
* @Autor/s: Ignacio Giral, Marti Farre (ignacio.giral, marti.farrea)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 17/10/2024
*
************************************************/
#define _GNU_SOURCE
#include "../utils/utils.h"

// Definición de la estructura HarleyConfig para almacenar la configuración
typedef struct {
    char *gotham_server_ip;
    char *gotham_server_port; // Cambiado a char*
    char *harley_server_ip;
    char *harley_server_port; // Cambiado a char*
    char *directory;
    char *worker_type;
} HarleyConfig;

// Variable global para almacenar la configuración
HarleyConfig config;

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
    free(config.harley_server_ip);
    free(config.harley_server_port); // Liberar la memoria del puerto
    free(config.directory);
    free(config.worker_type);
}

/***********************************************
*
* @Finalidad: Leer el archivo de configuración y almacenar los valores en la estructura HarleyConfig.
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

    config.gotham_server_ip = readUntil(fd, '\n');
    if (config.gotham_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Gotham server IP\n", 39);
        close(fd);
        exit(1);
    }
    replace(config.gotham_server_ip, '\r', '\0');

    config.gotham_server_port = readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.gotham_server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Gotham server port\n", 41);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.gotham_server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    config.harley_server_ip = readUntil(fd, '\n');
    if (config.harley_server_ip == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Harley server IP\n", 40);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.harley_server_ip, '\r', '\0');

    config.harley_server_port = readUntil(fd, '\n'); // Leer el puerto como char*
    if (config.harley_server_port == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read Harley server port\n", 42);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.harley_server_port, '\r', '\0'); // Eliminar el carácter '\r' al final

    config.directory = readUntil(fd, '\n');
    if (config.directory == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read directory\n", 32);
        free_config();
        close(fd);
        exit(1);
    }
    replace(config.directory, '\r', '\0');

    config.worker_type = readUntil(fd, '\n');
    if (config.worker_type == NULL) {
        write(STDOUT_FILENO, "Error: Failed to read worker type\n", 35);
        free_config();
        close(fd);
        exit(1);
    } else {
        replace(config.worker_type, '\r', '\0');
        if(strcmp(config.worker_type, "Media") != 0) {
            write(STDOUT_FILENO, "Error: Invalid worker type\n", 28);
            free_config();
            close(fd);
            exit(1);
        }
    }

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

int createSocket(){
    char* message = (char*)malloc(sizeof(char) * 256);
    sprintf(message, "Connecting to %s Server to the Gotham system...\n", config.directory);
    write(STDOUT_FILENO, message, strlen(message)); 
    free(message);

    uint16_t port;
    int aux = atoi(config.gotham_server_port);
    if (aux < 1 || aux > 65535) {
        write(STDOUT_FILENO, "Error: Invalid port\n", 21);
        exit (EXIT_FAILURE);

    }
    port = aux;

    struct in_addr ip_addr;
    if (inet_pton(AF_INET, config.gotham_server_ip, &ip_addr) != 1) {
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
        write(STDOUT_FILENO, "Error: Cannot connect to Gotham\n", 33);
        exit (EXIT_FAILURE);
    }

    return sockfd;
}

void initServer() {
    int socket_fd = initSocket(config.harley_server_port, config.harley_server_ip);
    int newsock;
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof (c_addr);

    
    while(1) {
        char *data = (char *)malloc(sizeof(char) * 256);
        char *message = (char *)malloc(sizeof(char) * 256);
        newsock = accept(socket_fd, (void *) &c_addr, &c_len);
        if (newsock < 0) {
            write(STDOUT_FILENO, "Error: Cannot accept connection\n", 33);
            exit (EXIT_FAILURE);
        }

        read(newsock, message, 256);
        sprintf(data, "Fleck username: %s, File to distort: %s", getXFromMessage(message, 0), getXFromMessage(message, 1));
        write(STDOUT_FILENO, data, strlen(data));

        sendMessageToSocket(newsock, "3", "OK");
        close(newsock);
    }
}

/***********************************************
*
* @Finalidad: Función principal del programa. Lee el archivo de configuración y muestra los detalles.
* @Parametros: int argc - Número de argumentos.
*              char *argv[] - Array de argumentos.
* @Retorno: int - Código de salida del programa.
*
************************************************/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDOUT_FILENO, "Usage: Harley <config_file>\n", 28);
        exit(1);
    }

    read_config_file(argv[1]);

    char *msg;
    char *data = (char *)malloc(sizeof(char) * 256);

    asprintf(&msg, "Harley worker initialized\n");
    print_text(msg);
    free(msg);

    int sockfd = createSocket();
    
    sprintf(data, "%s&%s&%s", config.worker_type, config.harley_server_ip, config.harley_server_port);
    sendMessageToSocket(sockfd, "2", data);
        
    memset(data, '\0', 256);
    read(sockfd, data, 256);

    close(sockfd);

    initServer();

    free(data); 
    free_config();

    return 0;
}