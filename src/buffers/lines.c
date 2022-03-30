#include "buffers.h"
#include <stdlib.h>

void insert_char(Line* line, char c, uint16_t index)
{
    line->characters_length++;
    if (line->characters_capacity < line->characters_length)
        line->characters_capacity = line->characters_capacity ? line->characters_capacity * 2 : 4;

    memmove(&line->characters[index + 1], &line->characters[index], line->characters_length - index);
    
    line->characters[index] = c;   
}

void line_free(Line* line)
{
    free(line->tokens);
    free(line->characters);
}