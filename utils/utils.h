#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>

#define print_text(str) write(1, str, strlen(str))
#define print_error(str) write(2, str, strlen(str))

char *readUntil(int fd, char cEnd);
bool read_line(int fd, int *read_bytes, char **buffer);
char *to_upper_case(char *str);
int count_words(char *str);
void strip_whitespace(char *str);
void replace(char *old_str, char old, char new);

