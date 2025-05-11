/***********************************************
*
* @Proposito: Implementa funciones para manipulación y procesamiento
*               de cadenas de texto
* @Autor/es: Ignacio Giral, Marti Farre (ignacio.giral, marti.farre)
* @Data creacion: 12/10/2024
* @Data ultima modificacion: 18/05/2025
*
************************************************/
#define _GNU_SOURCE
#include "string.h"

#define N_LINES 16
#define M_CHARS 16
/**************************************************
 *
 * @Finalidad: Leer del descriptor de archivo dado todos los caracteres
 *             hasta encontrar el carácter delimitador o el fin de archivo.
 * @Parametros:     in: fd    = descriptor de archivo desde el que se leerá.
 *                  in: cEnd  = carácter que marca el final de la lectura.
 * @Retorno:    Puntero a una cadena dinámica (char*) terminada en '\0'
 *              que contiene los caracteres leídos (sin incluir cEnd).
 *              Devuelve NULL en caso de error o si no se leyó ningún dato.
 *
 **************************************************/
char *STRING_readUntil(int fd, char cEnd) {
    int i = 0;
    ssize_t chars_read;
    char c = 0;
    char *buffer = NULL, *temp = NULL;

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

        temp = (char *)realloc(buffer, i + 2);
        if (!temp) {
            free(buffer);
            return NULL;
        }
        buffer = temp;  
        buffer[i++] = c;                
    }

    buffer[i] = '\0'; 
    return buffer;
}


/**************************************************
 *
 * @Finalidad: Contar cuántas palabras contiene una cadena,
 *             considerando cualquier secuencia de espacios, tabulaciones
 *             o saltos de línea como separadores.
 * @Parametros: in: str = puntero a la cadena de caracteres
 * @Retorno:    Cantidad de palabras encontradas en la cadena.
 *             Devuelve 0 si la cadena está vacía o no contiene palabras.
 *
 **************************************************/
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

/**************************************************
 *
 * @Finalidad: Eliminar los espacios en blanco al inicio y al final de la cadena,
 *             y compacta secuencias de espacios consecutivos en uno solo.
 * @Parametros: in/out: str = puntero a la cadena que será modificada
 *                        in situ para quitar el espacio innecesario.
 * @Retorno:    ---
 *
 **************************************************/
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

/**************************************************
 *
 * @Finalidad: Convertir todos los caracteres alfabéticos de la cadena
 *             a su versión en mayúsculas.
 * @Parametros: in/out: str = puntero a la cadena que se va a convertir.
 * @Retorno:    Devuelve el mismo puntero a la cadena modificada.
 *
 **************************************************/
char *STRING_to_upper_case(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (islower(str[i])) {
            str[i] = toupper(str[i]);
        }
    }
    return str;
}
/**************************************************
 *
 * @Finalidad: Reemplazar todas las apariciones del carácter 'old'
 *             por el carácter 'new' en la cadena.
 * @Parametros: in/out: old_str = puntero a la cadena  a modificar.
 *              in: old     = carácter que se desea reemplazar.
 *              in: new     = carácter con el que se reemplazará.
 * @Retorno:    ---
 *
 **************************************************/
void STRING_replace(char *old_str, char old, char new) {
    int old_str_len = (int)strlen(old_str);
    char *new_str = (char *)malloc((old_str_len + 1) * sizeof(char));
    
    int j = 0;
    for (int i = 0; i < old_str_len; i++) {
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

/**************************************************
 *
 * @Finalidad: Leer del descriptor de archivo una línea de texto completa,
 *             hasta encontrar el carácter '\n' o el fin de archivo (EOF).
 * @Parametros: in:  fd         = descriptor de archivo desde el que se leerá.
 *              out: read_bytes = puntero a entero donde se devolverá el número
 *                                 de bytes leídos (incluyendo '\\n' si se leyó).
 *              out: line       = puntero a puntero donde se asignará dinámicamente
 *                                 la cadena resultante.
 * @Retorno:    true si se leyó al menos un carácter antes de EOF (la línea puede
 *             terminar en '\n'); false si se alcanzó EOF sin leer datos o en
 *             caso de error.
 *
 **************************************************/
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
            case -1:  
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
/**************************************************
 *
 * @Finalidad: Obtener el enésimo campo de un mensaje donde los campos
 *             están separados por un delimitador.
 * @Parametros: in:  message = puntero a la cadena de entrada con campos separados.
 *              in:  x       = índice  del campo a extraer.
 * @Retorno:    Puntero a una cadena dinámica terminada en '\0' que contiene
 *              el campo solicitado. Devuelve NULL si x está fuera de rango
 *              o si ocurre un error de asignación de memoria.
 *
 **************************************************/
char* STRING_getXFromMessage(const char* message, int x) {
    // Verificar si el mensaje es válido
    if (!message || x < 0) {
        return NULL;
    }

    // Crear una copia de la cadena para que strtok no altere el original
    char temp[247];
    strncpy(temp, message, 246); // Copia hasta 246 caracteres
    temp[246] = '\0';

    // Inicializar el primer token
    char *token = strtok(temp, "&");

    // Avanzar a través de los tokens hasta llegar al `x`-ésimo token
    for (int i = 0; i < x; i++) {
        token = strtok(NULL, "&");
        if (token == NULL) {
            return NULL; // Si no hay suficientes tokens, retorna NULL
        }
    }

    return token ? strdup(token) : NULL; // Duplicar el token antes de retornarlo
}
/**************************************************
 *
 * @Finalidad: Extraer la parte de la cadena tras el nombre del comando,
 *             que corresponde a los argumentos completos.
 * @Parametros: in:  global_cmd = puntero a la cadena que contiene el comando
 *                               completo junto con sus argumentos
 * @Retorno:    Puntero a una cadena dinámica (char*)
 *             que contiene todo lo que hay después del primer espacio
 *             en global_cmd (los argumentos). Devuelve NULL si no existen
 *             argumentos o en caso de error de asignación.
 *
 **************************************************/
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
/**************************************************
 *
 * @Finalidad: Convertir todos los caracteres alfabéticos de la cadena
 *             a su versión en minúsculas.
 * @Parametros: in/out: str = puntero a la cadena que se va a convertir.
 * @Retorno:    ---
 *
 **************************************************/
void STRING_to_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

/**************************************************
 *
 * @Finalidad: Extraer la tercera palabra de una cadena de texto.
 * @Parametros: in: input = puntero a la cadena de caracteres de entrada.
 * @Retorno:    Puntero a una nueva cadena dinámica (char*) que contiene
 *             la tercera palabra encontrada.
 *             Devuelve NULL si la cadena tiene menos de tres palabras
 *             o si ocurre un error de asignación de memoria.
 *
 **************************************************/
char* STRING_get_third_word(const char* input) {
    if (!input) {
        return NULL;
    }

    const char* delimiter = " "; // Delimitador para dividir las palabras.
    char* copy = strdup(input);
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

    free(copy);
    return third_word;
}
/**************************************************
 *
 * @Finalidad: Extraer el código de canción embebido en un mensaje,
 *             leyendo los primeros 'length' caracteres relevantes.
 * @Parametros: in: message = puntero a la cadena de entrada que contiene
 *                           información de la canción.
 *              in: length  = número de caracteres del código a extraer.
 * @Retorno:    Puntero a una cadena dinámica (char*) terminada en '\0'
 *              que contiene el código de la canción extraído. Devuelve NULL
 *              si 'message' es NULL, 'length' es <= 0, o en caso de error
 *              de asignación de memoria.
 *
 **************************************************/
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
/**************************************************
 *
 * @Finalidad: Eliminar todas las apariciones de un carácter
 *             específico de una cadena de texto, compactando
 *             la cadena in situ.
 * @Parámetros: in/out: str = puntero a la cadena (terminada en '\\0')
 *                        de la que se eliminará el carácter.
 *              in:     ch  = carácter a buscar y eliminar.
 * @Retorno:    --- 
 *
 **************************************************/
void STRING_remove_char(char *str, char ch) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src != ch) {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}