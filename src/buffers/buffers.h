#pragma once

#include "../moss.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
    uint8_t characters_length;
    char* characters;
} String_Slice;

void lexical_analyze_lines(Buffer* buffer, uint16_t from, uint16_t to);
void lexical_analyze_c(Line* line, bool* continue_multiline_comment);
void lexical_analyze_owen(Line* line);

void line_free(Line* line);
void insert_char(Line* line, char c, uint16_t index);
void insert_chars(Line* line, uint16_t index, uint16_t amount, char* chars);
void remove_chars(Line* line, uint16_t index, uint16_t amount);
void remove_lines(Buffer* buffer, uint16_t index, uint16_t amount);

Line* insert_line(Buffer* buffer, uint16_t index);
Line* add_line(Buffer* buffer);
Token* add_token(Line* line);
