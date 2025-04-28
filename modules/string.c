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
#include "string.h"

#define N_LINES 16
#define M_CHARS 16

char *STRING_readUntil(int fd, char cEnd) {
    int i = 0;
    ssize_t chars_read;
    char c = 0;
    char *buffer = NULL, *temp = NULL;

    while (1) {
        chars_read = read(fd, &c, sizeof(char));  
        if (chars_read == 0) {         
            if (i == 0) {              
                return NULL;  // No hay nada que leer, retorna NULL sin alocar memoria
            }
            break;                     
        } else if (chars_read < 0) {   
            free(buffer);
            return NULL;
        }

        if (c == cEnd) {              
            break;
        }

        temp = (char *)realloc(buffer, i + 2); // Se usa `temp` para verificar si `realloc` falla
        if (!temp) {  // Si `realloc` falla, liberar memoria y retornar NULL
            free(buffer);
            return NULL;
        }
        buffer = temp;  
        buffer[i++] = c;                
    }

    buffer[i] = '\0';  // Terminar la cadena con un carácter nulo
    return buffer;
}



int STRING_count_words(char *str) {
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


void STRING_strip_whitespace(char *str) {
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


char *STRING_to_upper_case(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (islower(str[i])) {
            str[i] = toupper(str[i]);
        }
    }
    return str;
}

void STRING_replace(char *old_str, char old, char new) {
    int old_str_len = (int)strlen(old_str);
    // Allocate exactly old_str_len + 1 for the new string + null terminator
    char *new_str = (char *)malloc((old_str_len + 1) * sizeof(char));
    
    int j = 0;
    // Iterate only through the characters (not including the terminating '\0')
    for (int i = 0; i < old_str_len; i++) {
        if (old_str[i] == old) {
            // If we are "removing" the character by replacing it with '\0', skip adding it
            if (new != '\0') {
                new_str[j] = new;
                j++;
            }
        } else {
            new_str[j] = old_str[i];
            j++;
        }
    }

    // Add the null terminator after processing all characters
    new_str[j] = '\0';

    // Copy the modified string back to old_str
    strcpy(old_str, new_str);
    free(new_str);
}


bool STRING_read_line(int fd, int *read_bytes, char **line) {
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

char* STRING_getXFromMessage(const char* message, int x) {
    // Verificar si el mensaje es válido
    if (!message || x < 0) {
        return NULL;
    }

    // Crear una copia de la cadena para que strtok no altere el original
    char temp[247];
    strncpy(temp, message, 246); // Copia hasta 246 caracteres
    temp[246] = '\0';            // Asegura terminación en nulo

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
    return token ? strdup(token) : NULL; // Duplicar el token antes de retornarlo
}

char* STRING_extract_substring(char* global_cmd) {
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

void STRING_to_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

char* STRING_get_third_word(const char* input) {
    if (!input) {
        return NULL; // Si la entrada es NULL, devuelve NULL.
    }

    const char* delimiter = " "; // Delimitador para dividir las palabras.
    char* copy = strdup(input); // Crear una copia de la cadena para no modificar la original.
    if (!copy) {
        perror("Failed to allocate memory");
        return NULL;
    }

    char* token;
    char* third_word = NULL;
    int word_count = 0;

    token = strtok(copy, delimiter);
    while (token != NULL) {
        word_count++;
        if (word_count == 3) {
            third_word = strdup(token); // Copiar la tercera palabra.
            break;
        }
        token = strtok(NULL, delimiter);
    }

    free(copy); // Liberar la memoria de la copia.
    return third_word; // Devuelve la tercera palabra o NULL si no hay tres palabras.
}

char* STRING_getSongCode(const char* message, int length) {
    if (message == NULL || length <= 0) {
        write(STDOUT_FILENO, "Error: Invalid message\n", 24);
        return NULL;
    }

    char* songCode = (char*)malloc(sizeof(char) * (length + 1));
    if (songCode == NULL) {
        write(STDOUT_FILENO, "Error: Memory allocation failed\n", 32);
        return NULL;
    }

    memcpy(songCode, message, length);
    songCode[length] = '\0';
    return songCode;
}