#include "buffers.h"
#include <stdlib.h>
#include <assert.h>

void insert_char(Line* line, char c, uint16_t index)
{
    INSERT_ELEMENTS(line->characters, index, 1, &c);
}

void insert_chars(Line* line, uint16_t index, uint16_t amount, char* chars)
{
    INSERT_ELEMENTS(line->characters, index, amount, chars);
}

void remove_chars(Line* line, uint16_t index, uint16_t amount)
{
    REMOVE_ELEMENTS(line->characters, index, amount);
}

void line_free(Line* line)
{
    free(line->tokens);
    free(line->characters);
}

// The cursor can be placed after the last character in a line which means that
// characters are appended, not inserted. But that also means that if the editor
// tries to read what is under the cursor it may overrun the line buffer by 1
//
// That is not really communicated well by using -1 in some places and not in 
// others.
uint16_t index_of_last_character(const Line* line)
{
    assert(line->characters_length);

    return line->characters_length - 1;
}

uint16_t index_of_character_append(const Line* line)
{
    return line->characters_length;
}
