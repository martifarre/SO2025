// Ignacio Giral ignacio.giral
// Marti Farre marti.farre

/***********************************************
*
* @Proposito: Implementación del proceso Enigma
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 17/10/2024
*
************************************************/
#define _GNU_SOURCE
#include "utils.h"

#define N_LINES 16
#define M_CHARS 16

char *readUntil(int fd, char cEnd) {
    int i = 0;
    ssize_t chars_read;
    char c = 0;
    char *buffer = NULL;

    while (1) {
        chars_read = read(fd, &c, sizeof(char));  
        if (chars_read == 0) {         
            if (i == 0) {              
                return NULL;
            }
            break;                     
        } else if (chars_read < 0) {   
            free(buffer);
            return NULL;
        }

        if (c == cEnd) {              
            break;
        }
        buffer = (char *)realloc(buffer, i + 2);
        buffer[i++] = c;                
    }

    buffer[i] = '\0';  // Terminar la cadena con un carácter nulo
    return buffer;
}


int count_words(char *str) {
    int words = 0;
    int is_space = 0;
    for (char *p = str; *p; ++p) {
        if (*p == ' ') {
            is_space = 0;
        } else if (!is_space) {
            is_space = 1;
            ++words;
        }
    }
    return words;
}


void strip_whitespace(char *str) {
    int read_index = 0, write_index = 0;
    int in_word = 0;  // Indica si estamos dentro de una palabra

    // Recorrer la cadena
    while (str[read_index] != '\0') {
        if (str[read_index] != ' ' && str[read_index] != '\t') {
            // Si el carácter actual no es un espacio o tabulación, copiarlo
            str[write_index++] = str[read_index];
            in_word = 1;  // Estamos dentro de una palabra
        } else if (in_word) {
            // Si encontramos un espacio y estamos dentro de una palabra, escribir un solo espacio
            str[write_index++] = ' ';
            in_word = 0;  // Reiniciar in_word porque ahora estamos en un espacio
        }
        read_index++;
    }

    // Eliminar cualquier espacio al final (si el último carácter era un espacio)
    if (write_index > 0 && str[write_index - 1] == ' ') {
        write_index--;
    }

    // Terminar el resultado con un carácter nulo
    str[write_index] = '\0';
}


char *to_upper_case(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (islower(str[i])) {
            str[i] = toupper(str[i]);
        }
    }
    return str;
}


void replace(char *old_str, char old, char new) {
    char *new_str = (char *)malloc(strlen(old_str) * sizeof(char) + 1);
    int old_str_len = (int)strlen(old_str);
    int j = 0;
    for (int i = 0; i < old_str_len + 1; i++) {
        if (old_str[i] == old) {
            if (new != '\0') {
                new_str[j] = new;
                j++;
            }
        } else {
            new_str[j] = old_str[i];
            j++;
        }
    }
    new_str[j] = '\0';

    strcpy(old_str, new_str);
    free(new_str);
}


bool read_line(int fd, int *read_bytes, char **line) {
    char ch;
    int read_value;
    int new_size;
    bool is_eof = false;
    *read_bytes = 0;
    if (*line == NULL) {
        *line = (char *)malloc(M_CHARS);
        if (*line == NULL) {
            print_error("Error trying to allocate memory for line\n");
        }
    }
    do {
        read_value = read(fd, &ch, sizeof(char));
        switch (read_value) {
            case -1:  // error
                print_error("An error ocurred while reading the next byte.");
                exit(-1);
                break;
            case 0:
                is_eof = true;
                break;
            case 1:
                if ((*read_bytes + 1) % M_CHARS == 0) {
                    new_size = *read_bytes + M_CHARS;
                    *line = realloc(*line, new_size);
                }
                (*line)[*read_bytes] = ch;
                (*read_bytes)++;
                break;
        }
        if (is_eof) {
            break;
        }
    } while (ch != '\n');
    (*line)[*read_bytes] = '\0';
    return is_eof;
}


char* getXFromMessage(const char* message, int x) {
    // Crear una copia de la cadena para que strtok no altere el original
    char temp[247];
    strncpy(temp, message, 246); // Copia hasta 246 caracteres
    temp[246] = '\0';          // Asegura terminación en nulo

    // Inicializar el primer token
    char *token = strtok(temp, "&");

    // Avanzar a través de los tokens hasta llegar al `x`-ésimo token
    for (int i = 0; i < x; i++) {
        token = strtok(NULL, "&");
        if (token == NULL) {
            return NULL; // Si no hay suficientes tokens, retorna NULL
        }
    }
    // Retornar una copia del `x`-ésimo token
    return token ? strdup(token) : NULL;
}

uint16_t CONEXIONES_cacular_checksum(const char *trama) {
    uint32_t sum = 0;

    // Asumimos que la trama tiene 256 bytes
    for (int i = 0; i < 250; i++) { // Solo calculamos sobre los primeros 250 bytes
        sum += (unsigned char)trama[i];
    }

    // Reducimos la suma a 16 bits
    uint16_t checksum = (sum & 0xFFFF) + (sum >> 16);

    // Retornamos el complemento a uno del checksum
    return ~checksum;
}

int readMessageFromSocket(int fd, struct trama *trama) {
    char buffer[256];
    int bytes_leidos = 0;
    int checksum = 0;

    bytes_leidos = read(fd, &buffer, sizeof(char) * 256);
    if (bytes_leidos != 256) {
        perror("Error leyendo la trama del socket. NO SON 256 BYTES");
        return -1;
    }

    trama->data = malloc(256 - 9);  // El tamaño de data es 256 - (1 + 2 + 2 + 4)
    trama->tipo = buffer[0];
    trama->longitud = ((unsigned char)buffer[1] << 8 | (unsigned char)buffer[2]);

    for (int i = 0; i < trama->longitud; i++) {
        trama->data[i] = buffer[i + 3];
    }

    trama->checksum = ((unsigned char)buffer[250] << 8 | (unsigned char)buffer[251]);
    trama->timestamp = ((unsigned char)buffer[252] << 24 |
                        (unsigned char)buffer[253] << 16 |
                        (unsigned char)buffer[254] << 8 |
                        (unsigned char)buffer[255]);

    checksum = CONEXIONES_cacular_checksum(buffer);
    if (checksum == trama->checksum) {
        return 1;
    } else {
        return -1;
    }
}

void sendMessageToSocket(int fd, char type, int16_t data_length, char *data) {
    char trama[256];
    int timestamp = 0;
    int checksum = 0;

    memset(trama, '\0', sizeof(trama));
    trama[0] = type;
    trama[1] = (data_length >> 8) & 0xFF;
    trama[2] = data_length & 0xFF;

    for (int i = 0; i < data_length; i++) {
        trama[i + 3] = data[i];
    }

    timestamp = time(NULL);
    trama[252] = (timestamp >> 24) & 0xFF;
    trama[253] = (timestamp >> 16) & 0xFF;
    trama[254] = (timestamp >> 8) & 0xFF;
    trama[255] = timestamp & 0xFF;

    checksum = CONEXIONES_cacular_checksum(trama);
    trama[250] = (checksum >> 8) & 0xFF;
    trama[251] = checksum & 0xFF;

    write(fd, trama, 256);
}

int isSocketOpen(int sockfd) {
    // Comprueba si el descriptor es válido
    if (fcntl(sockfd, F_GETFL) == -1) {
        if (errno == EBADF) {
            return 0; // Descriptor no válido (socket cerrado)
        }
    }
    return 1; // Descriptor válido (socket abierto)
}