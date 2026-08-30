#include "input.h"

#include <stdio.h>
#include <string.h>

void input_clear_screen(void)
{
    fputs("\033[H\033[2J", stdout);
    fflush(stdout);
}

bool input_read_line(const char *prompt, char *buffer, size_t buffer_size)
{
    int ch;

    if (buffer == NULL || buffer_size == 0) return false;
    if (prompt != NULL) {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    if (fgets(buffer, (int)buffer_size, stdin) == NULL) return false;
    if (strchr(buffer, '\n') == NULL && strchr(buffer, '\r') == NULL) {
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return true;
}
