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

/***********************************************
*
* @Finalidad: Leer caracteres de un archivo hasta encontrar un carácter específico.
* @Parametros: int fd - Descriptor de archivo.
*              char cEnd - Carácter que indica el final de la lectura.
* @Retorno: char* - Cadena leída hasta el carácter especificado.
*
************************************************/
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

/***********************************************
*
* @Finalidad: Contar el número de palabras en una cadena.
* @Parametros: char *str - Cadena de caracteres.
* @Retorno: int - Número de palabras en la cadena.
*
************************************************/
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

/***********************************************
*
* @Finalidad: Eliminar espacios en blanco de una cadena.
* @Parametros: char *str - Cadena de caracteres.
* @Retorno: Ninguno.
*
************************************************/
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

/***********************************************
*
* @Finalidad: Convertir una cadena a mayúsculas.
* @Parametros: char *str - Cadena de caracteres.
* @Retorno: char* - Cadena convertida a mayúsculas.
*
************************************************/
char *to_upper_case(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (islower(str[i])) {
            str[i] = toupper(str[i]);
        }
    }
    return str;
}

/***********************************************
*
* @Finalidad: Reemplazar caracteres en una cadena.
* @Parametros: char *old_str - Cadena original.
*              char old - Carácter a reemplazar.
*              char new - Nuevo carácter.
* @Retorno: Ninguno.
*
************************************************/
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

/***********************************************
*
* @Finalidad: Leer una línea desde un archivo.
* @Parametros: int fd - Descriptor de archivo.
*              int *read_bytes - Puntero para almacenar el número de bytes leídos.
*              char **line - Puntero a la línea leída.
* @Retorno: bool - Verdadero si se alcanza el final del archivo, falso en caso contrario.
*
************************************************/
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

/***********************************************
*
* @Finalidad: Extraer el `x`-ésimo token de una cadena `message` separada por `&`, comenzando desde `message[3]` y deteniéndose en `message[249]`.
* @Parametros: char* message - La cadena de caracteres de la cual se desea extraer el token.
*              int x - El índice del token que se desea extraer (0 para el primer token, 1 para el segundo, etc.).
* @Retorno: char* - Un puntero al `x`-ésimo token en la cadena `message`. Si no se encuentra el token, retorna la copia de la cadena.
*
************************************************/
char* getXFromMessage(const char* message, int x) {
    // Ajustar el puntero para que apunte a message[3]
    const char *start = message + 3;

    // Crear una copia de la cadena para que strtok no altere el original
    char temp[247];
    strncpy(temp, start, 246); // Copia hasta 246 caracteres desde start
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

/*
uint16_t calculate_checksum(char *message, int length) {
    uint32_t sum = 0;
    for (int i = 0; i < length; i++) {
        sum += (uint8_t)message[i];
    }
    return sum % 65536;
}

int validate_checksum(char *message) {
    uint16_t received_checksum = (message[250] << 8) | message[251];

    uint16_t calculated_checksum = calculate_checksum(message, 250);

    printf("Received checksum: %u\n", received_checksum);
    printf("Calculated checksum: %u\n", calculated_checksum);

    return received_checksum == calculated_checksum;
}
*/

char *buildMessage(char *type, char *data) {
    char *message = (char *)malloc(256 * sizeof(char));
    memset(message, '\0', 256);

    // Type (1 Byte)
    message[0] = type[0];
    message[1] = '0';
    message[2] = '0';

    /*
    // Data Length (2 Bytes)
    uint16_t data_len = (uint16_t)strlen(data);

    message[1] = (data_len >> 8) & 0xFF;
    message[2] = data_len & 0xFF;
    */

    // Data (up to 245 Bytes)
    for (int i = 0; i < strlen(data) && i < 247; i++) {
        message[3 + i] = data[i];
    }

    /*
    // Checksum (2 Bytes)
    uint16_t checksum = calculate_checksum(message, 250);
    message[250] = (checksum >> 8) & 0xFF;
    message[251] = checksum & 0xFF;

    // Timestamp (4 Bytes)
    int32_t timestamp = (int32_t)time(NULL);
    memcpy(&message[252], &timestamp, 4);
    */
    return message;
}

void sendMessageToSocket(int socket, char *type, char *data) {
    char *messageToSend = buildMessage(type, data);

    if (messageToSend == NULL) {
        print_error("Failed to build message correctly.\n");
        return;
    }
    
    printf("\n\nSending message: %s\n\n", messageToSend);
    // Send the message through the socket
    write(socket, messageToSend, 256);

    // Free the message after sending it
    free(messageToSend);
}