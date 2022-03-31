#include "buffers.h"
#include <stdlib.h>
#include <assert.h>

void insert_char(Line* line, char c, uint16_t index)
{
    line->characters_length++;
    if (line->characters_capacity < line->characters_length)
        line->characters_capacity = line->characters_capacity ? line->characters_capacity * 2 : 4;

    memmove(&line->characters[index + 1], &line->characters[index], line->characters_length - index);
    
    line->characters[index] = c;   
}

void insert_chars(Line* line, uint16_t index, uint16_t amount, char* chars)
{
    if (line->characters_capacity < index + amount)
    {
        while (line->characters_capacity < index + amount)
            line->characters_capacity = line->characters_capacity ? line->characters_capacity * 2 : 4;

        line->characters = realloc(line->characters, line->characters_capacity);
    }

    memmove(&line->characters[index + amount], &line->characters[index], amount);
    memcpy(&line->characters[index], chars, amount);

    line->characters_length += amount;
}

void remove_chars(Line* line, uint16_t index, uint16_t amount)
{
    assert(line->characters_length >= index + amount);
    memmove(&line->characters[index], &line->characters[index + amount], line->characters_length - (index + amount - 1));

    line->characters_length -= amount;
}

void line_free(Line* line)
{
    free(line->tokens);
    free(line->characters);
}