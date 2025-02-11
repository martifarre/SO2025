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

    for (size_t i = 0; i < file_size; i++) {  // Aquí está la corrección
        if (isalpha(buffer[i])) {
            word_length++;
        } else {
            if (word_length >= (size_t)word_limit) {
                while (start <= &buffer[i] && filtered_index < file_size) {
                    filtered_text[filtered_index++] = *start++;
                }
            } else {
                while (!isalpha(*start) && start <= &buffer[i] && filtered_index < file_size) {
                    filtered_text[filtered_index++] = *start++;
                }
                start = &buffer[i + 1];
            }
            word_length = 0;
        }
    }

    // Asegurar terminador nulo en `filtered_text`
    if (filtered_index < file_size) {
        filtered_text[filtered_index] = '\0';
    } else {
        filtered_text[file_size - 1] = '\0';
    }
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


int DISTORSION_distortFile(listElement2* element, volatile sig_atomic_t *stop_signal) {
    pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
    char* path = NULL;
    asprintf(&path, "%s/%s", element->directory, element->fileName);
    write(STDOUT_FILENO, path, strlen(path));
    char* fileSize3;
    char* actualMd5;

    int fd = open(path, O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        perror("Failed to open file.");
        exit(EXIT_FAILURE);
    }

    struct trama htrama;
    printf("Bytes to write: %d\n", element->bytes_to_writeF1);
    printf("Bytes written: %d\n", element->bytes_writtenF1);
    int bytes_written = element->bytes_writtenF1, bytes_to_write = element->bytes_to_writeF1; 
    if(element->status == 0 || element->status == 1) {
        element->status = 1;
        while (bytes_written < bytes_to_write) {  
            if (*stop_signal) {  
                write(STDOUT_FILENO, "Stopping file reception due to signal...\n", 42);
                close(fd);
                free(path);
                return 1;
            }
            if(TRAMA_readMessageFromSocket(element->fd, &htrama) < 0) {
                write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
                return 1;
            } else if (htrama.tipo == 0x07) {
                write(STDOUT_FILENO, "Fleck received a CTRL+C.\n", 26);
                return 1;
            } else if (htrama.tipo != 0x03) {
                write(STDOUT_FILENO, "Error: Invalid trama type.\n", 28);
                return 1;
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
            element->bytes_writtenF1 = bytes_written;
            usleep(1000); 
        }
        element->status = 2;

        actualMd5 = DISTORSION_getMD5SUM(path);
        printf("MD5SUM: %s\n", actualMd5);
    

        if(strcmp(element->MD5SUM, actualMd5) == 0) {
            TRAMA_sendMessageToSocket(element->fd, 0x06, (int16_t)strlen("CHECK_OK"), "CHECK_OK");  
        } else {
            TRAMA_sendMessageToSocket(element->fd, 0x06, (int16_t)strlen("CHECK_KO"), "CHECK_KO");  
            write(STDOUT_FILENO, "Error: CHECK_KO.\n", 18);
            return 1;
        }
        free(actualMd5);

    } 
    close(fd);


    if(element->status == 2) {
        write(STDOUT_FILENO, "In\n", 4); 
        int error = 0;
        printf("Path: %s\n", path);
        printf("Element->factor: %s\n", element->factor);   
        if(strcmp(element->worker_type, "Text") == 0) {
            error = DISTORSION_compressText(path, atoi(element->factor));
        } else if(FILES_has_extension(element->fileName, (const char *[]) { ".wav", NULL })) {
            write(STDOUT_FILENO, "Compressing audio file\n", 24);

            error = SO_compressAudio(path, atoi(element->factor));
            write(STDOUT_FILENO, "Audio file compressed\n", 23);
        } else {
            write(STDOUT_FILENO, "Compressing image file\n", 24);
            error = SO_compressImage(path, atoi(element->factor));
            write(STDOUT_FILENO, "Image file compressed\n", 22);
        }

        write(STDOUT_FILENO, "Out\n", 4);
        switch (error) {
            case 0:
                write(STDOUT_FILENO, "The file was distorted successfully\n", 37);
                break;
            case -1:
                write(STDOUT_FILENO, "The file could not be read\n", strlen("The file could not be read\n"));
                return 1;
            case -2:
                write(STDOUT_FILENO, "The scaling factor is too large for the file\n", strlen("The scaling factor is too large for the file\n"));
                return 1;
            case -3:
                write(STDOUT_FILENO, "Memory allocation error\n", 25);
                return 1;
            case -4:
                write(STDOUT_FILENO, "Error creating the temporary file\n", 35);
                return 1;
            case -5:
                write(STDOUT_FILENO, "Unsupported image format\n", 26);
                return 1;
            case -6:
                write(STDOUT_FILENO, "Error creating the final file\n", 31);
                return 1;
            case -7:
                write(STDOUT_FILENO, "The audio file is not a WAV file. Only WAV files are supported\n", 63);
                return 1;
            default:
                break;
        }
        fileSize3 = FILES_get_size_of_file(path);
        char* distortedMd5 = DISTORSION_getMD5SUM(path);

        char* data = (char*)malloc(256 * sizeof(char));
        sprintf(data, "%s&%s", fileSize3, distortedMd5);
        TRAMA_sendMessageToSocket(element->fd, 0x04, (int16_t)strlen(data), data);
        printf("Data: %s\n", data);
        free(data);
        element->bytes_to_writeF2 = atoi(fileSize3);
        free(fileSize3);
        free(distortedMd5);
    }

    if (*stop_signal) {  
        write(STDOUT_FILENO, "Stopping file reception due to signal...\n", 42);
        free(path);
        return 1;
    }

    element->status = 3;
    int fd2 = open(path, O_RDONLY);
    if (fd2 < 0) {
        write(STDOUT_FILENO, "Error: Cannot open file\n", 25);
        return 1;
    }

    int bytes_to_write2 = element->bytes_to_writeF2;  
    int bytes_written2 = element->bytes_writtenF2;
    printf("Bytes to write: %d\n", bytes_to_write2);
    printf("Bytes written: %d\n", bytes_written2);
    char* buf = (char*)malloc(sizeof(char) * 247);
    char* message = (char*)malloc(sizeof(char) * 256);
    int sizeOfBuf = 0;
    write(STDOUT_FILENO, "Sending distorted file to Fleck...\n", 36);
    
    if (bytes_written2 > 0) {
        lseek(fd2, bytes_written2, SEEK_SET);
        printf("[DEBUG] Saltando %d bytes, comenzando desde la posición correcta.\n", bytes_written2);
    }
    
    while (bytes_to_write2 > bytes_written2) {
        if (*stop_signal) {  
            write(STDOUT_FILENO, "Stopping file reception due to signal...\n", 42);
            free(path);
            close(fd2);
            return 1;
        }
        memset(buf, '\0', 247);
        memset(message, '\0', 256);
        sizeOfBuf = read(fd2, buf, 247);  
        memcpy(message, buf, sizeOfBuf);
        bytes_written2 += sizeOfBuf;     
        element->bytes_writtenF2 = bytes_written2;   
        pthread_mutex_lock(&myMutex);
        TRAMA_sendMessageToSocket(element->fd, 0x05, sizeOfBuf, message);
        pthread_mutex_unlock(&myMutex);
        usleep(1000); 
    }
    close(fd2);

    free(buf);
    free(message);


    if(TRAMA_readMessageFromSocket(element->fd, &htrama) < 0) {
        write(STDOUT_FILENO, "Error: Checksum not validated.\n", 32);
        return 1;
    } else if (htrama.tipo == 0x06  && strcmp((const char *)htrama.data, "CHECK_OK") == 0) {
        write(STDOUT_FILENO, "File distorted successfully.\n\n", 31);
    } else if (htrama.tipo == 0x06  && strcmp((const char *)htrama.data, "CHECK_KO") == 0) {
        write(STDOUT_FILENO, "Error: File could not be distorted.\n", 37);
    } else {
        write(STDOUT_FILENO, "Error: Invalid trama type.\n", 27);
        free(htrama.data);
        return 1;
    } 

    if (remove(path) == 0) {
        write(STDOUT_FILENO, "Archivo eliminado\n", 18);
    } else {
        perror("Error al eliminar el archivo");
    }
    free(htrama.data);
    free(path);

    element->status = 4;
    close(element->fd);
    return 0;   
}