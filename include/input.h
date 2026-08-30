#ifndef MONOPOLY_INPUT_H
#define MONOPOLY_INPUT_H

#include <stdbool.h>
#include <stddef.h>

bool input_read_line(const char *prompt, char *buffer, size_t buffer_size);

#endif
