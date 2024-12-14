#define _GNU_SOURCE
#include "distorsion.h"

char* DISTORSION_getMD5SUM(const char* path) {
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

        char *md5sum = (char *)malloc(33 * sizeof(char));
        if (md5sum != NULL) {
            strcpy(md5sum, actualMd5);
        }
        return md5sum;
    }
}

int DISTORSION_compressText(char *input_file_path, int word_limit) {
    if (word_limit <= 0) {
        return ERROR_INVALID_LIMIT; // El límite debe ser mayor que 0
    }

    FILE *file = fopen(input_file_path, "r+");
    if (!file) {
        perror("Error al abrir el archivo");
        return ERROR_OPENING_FILE;
    }

    // Lee todo el contenido del archivo en memoria
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return ERROR_MEMORY_ALLOCATION;
    }

    size_t read_size = fread(buffer, 1, file_size, file);
    if (read_size != file_size) {
        free(buffer);
        fclose(file);
        return ERROR_READING_FILE;
    }
    buffer[file_size] = '\0'; // Asegura que la cadena está terminada

    // Filtrar las palabras según el límite de longitud
    char *filtered_text = (char *)malloc(file_size + 1); // Espacio para el texto filtrado
    if (!filtered_text) {
        free(buffer);
        fclose(file);
        return ERROR_MEMORY_ALLOCATION;
    }

    size_t filtered_index = 0;
    size_t word_length = 0;
    char *start = buffer;

    for (size_t i = 0; i <= file_size; i++) {
        if (isalpha(buffer[i])) {
            // Contar la longitud de la palabra
            word_length++;
        } else {
            // Si encontramos un delimitador o fin de palabra
            if (word_length >= (size_t)word_limit) {
                // Copia la palabra completa, incluyendo los caracteres no alfabéticos
                while (start <= &buffer[i]) {
                    filtered_text[filtered_index++] = *start++;
                }
            } else {
                // Ignorar la palabra actual, pero copiar los delimitadores
                while (!isalpha(*start) && start <= &buffer[i]) {
                    filtered_text[filtered_index++] = *start++;
                }
                start = &buffer[i + 1];
            }
            word_length = 0; // Reiniciar el contador de la palabra
        }
    }

    filtered_text[filtered_index] = '\0'; // Termina la cadena

    // Sobrescribe el archivo con el texto filtrado
    rewind(file);
    if (fwrite(filtered_text, 1, filtered_index, file) != filtered_index) {
        free(buffer);
        free(filtered_text);
        fclose(file);
        return ERROR_WRITING_FILE;
    }

    // Trunca el archivo si el texto filtrado es más corto
    if (ftruncate(fileno(file), filtered_index) != 0) {
        perror("Error truncando el archivo");
        free(buffer);
        free(filtered_text);
        fclose(file);
        return ERROR_WRITING_FILE;
    }

    // Limpieza
    free(buffer);
    free(filtered_text);
    fclose(file);

    return NO_ERROR; // Todo salió bien
}


void DISTORSION_distortFile(int fleckSock, char* directory, char* MD5SUM, char* factor, char* fileSize, char* fileName, char* worker_type) {
    pthread_mutex_t harleyMutex = PTHREAD_MUTEX_INITIALIZER;
    char* path = (char*)malloc(sizeof(char) * (strlen(directory) + strlen(fileName) + 1));
    sprintf(path, "%s/%s",directory, fileName);
    write(STDOUT_FILENO, path, strlen(path));

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Failed to open file.");
        exit(EXIT_FAILURE);
    }
    struct trama htrama;
    printf("File size: %d\n", atoi(fileSize));
    int bytes_written = 0, bytes_to_write = atoi(fileSize), i = 0;   
    while (bytes_written < bytes_to_write) {
        if(TRAMA_readMessageFromSocket(fleckSock, &htrama) < 0) {
            write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
            return;
        } else if (htrama.tipo == 0x07) {
            write(STDOUT_FILENO, "Fleck received a CTRL+C.\n", 26);
            return;
        } else if (htrama.tipo != 0x03) {
            write(STDOUT_FILENO, "Error: Invalid trama type.\n", 27);
            return;
        }
        char* incoming_Info = NULL;
        if (bytes_to_write - bytes_written < 247) {
            incoming_Info = STRING_getSongCode((const char *)htrama.data, bytes_to_write - bytes_written);
            bytes_written += write(fd, incoming_Info, bytes_to_write - bytes_written);
        } else {
            incoming_Info = STRING_getSongCode((const char *)htrama.data, 247);
            bytes_written += write(fd, incoming_Info, 247);
        }
        if (bytes_written < 0) {
            perror("Failed to write to file.");
            exit(EXIT_FAILURE);
        }
        free(incoming_Info);
        free(htrama.data);
        i++;
        printf("Value of i: %d\n", i);
    }
    printf("Value of i: %d\n", i);  
     
    close(fd);
    
    printf("MD5: %s\n", MD5SUM);
    printf("File size: %s\n", fileSize);
    printf("Bytes to write: %d\n", bytes_to_write);
    printf("Bytes written: %d\n", bytes_written);

    char* actualMd5 = DISTORSION_getMD5SUM(path);
    char* fileSize2 = FILES_get_size_of_file(path);
    printf("FileSize2: %s\n", fileSize2);
    printf("Actual MD5: %s\n", actualMd5);
    if(strcmp(MD5SUM, actualMd5) == 0) {
        TRAMA_sendMessageToSocket(fleckSock, 0x06, (int16_t)strlen("CHECK_OK"), "CHECK_OK");  
    } else {
        TRAMA_sendMessageToSocket(fleckSock, 0x06, (int16_t)strlen("CHECK_KO"), "CHECK_KO");  
        return;
    }

    int error = 0;
    if(strcmp(worker_type, "Text") == 0) {
        error = DISTORSION_compressText(path, atoi(factor));
    } else if(FILES_has_extension(fileName, (const char *[]) { ".wav", NULL })) {
        write(STDOUT_FILENO, "Compressing audio file\n", 24);
        error = SO_compressAudio(path, atoi(factor));
        write(STDOUT_FILENO, "Audio file compressed\n", 23);
    } else {
        error = SO_compressImage(path, atoi(factor));
    }
    write(STDOUT_FILENO, "Out\n", 4);
    switch (error) {
        case 0:
            write(STDOUT_FILENO, "The image was distorted successfully\n", 37);
            break;
        case -1:
            write(STDOUT_FILENO, "The image could not be read\n", 29);
            return;
        case -2:
            write(STDOUT_FILENO, "The scaling factor is too large for the image\n", 47);
            return;
        case -3:
            write(STDOUT_FILENO, "Memory allocation error\n", 25);
        return;
        case -4:
            write(STDOUT_FILENO, "Error creating the temporary file\n", 35);
            return;
        case -5:
            write(STDOUT_FILENO, "Unsupported image format\n", 26);
            return;
        case -6:
            write(STDOUT_FILENO, "Error creating the final file\n", 31);
            return;
        case -7:
            write(STDOUT_FILENO, "The audio file is not a WAV file. Only WAV files are supported\n", 63);
            return;
        default:
            break;
    }

    char* fileSize3 = FILES_get_size_of_file(path);
    char* distortedMd5 = DISTORSION_getMD5SUM(path);
    printf("FileSize3: %s\n", fileSize3);
    printf("Distorted MD5: %s\n", distortedMd5);
    char* data = (char*)malloc(256 * sizeof(char));
    sprintf(data, "%s&%s", fileSize3, distortedMd5);
    TRAMA_sendMessageToSocket(fleckSock, 0x04, (int16_t)strlen(data), data);
    free(data);

    int fd2 = open(path, O_RDONLY);
    if (fd2 < 0) {
        write(STDOUT_FILENO, "Error: Cannot open file\n", 25);
        return;
    }

    int bytes_written2 = 0;
    char* buf = (char*)malloc(sizeof(char) * 247);
    char* message = (char*)malloc(sizeof(char) * 256);
    int sizeOfBuf = 0;
    write(STDOUT_FILENO, "Sending distorted file to Fleck...\n", 36);
    int a = 0;
    while (atoi(fileSize3) > bytes_written2) {
        memset(buf, '\0', 247);
        memset(message, '\0', 256);
        sizeOfBuf = read(fd2, buf, 247);  
        memcpy(message, buf, sizeOfBuf);
        bytes_written2 += sizeOfBuf;         
        pthread_mutex_lock(&harleyMutex);
        TRAMA_sendMessageToSocket(fleckSock, 0x05, sizeOfBuf, message);
        pthread_mutex_unlock(&harleyMutex);
        a++;
    }
    close(fd2);
    printf("Value of a: %d\n", a); 
    printf("Bytes to write: %d\n", atoi(fileSize3)); 
    printf("Bytes written: %d\n", bytes_written2);

    if(TRAMA_readMessageFromSocket(fleckSock, &htrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        return;
    } else if (htrama.tipo == 0x06  && strcmp((const char *)htrama.data, "CHECK_OK") == 0) {
        write(STDOUT_FILENO, "File distorted successfully.\n\n", 31);
    } else if (htrama.tipo == 0x06  && strcmp((const char *)htrama.data, "CHECK_KO") == 0) {
        write(STDOUT_FILENO, "Error: File could not be distorted.\n", 37);
    } else {
        write(STDOUT_FILENO, "Error: Invalid trama type.\n", 27);
        free(htrama.data);
        return;
    } 

    free(htrama.data);
    free(buf);
    free(actualMd5);
    free(fileSize2);
    free(path);
}