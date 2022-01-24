#pragma once

#include "../moss.h"
#include "../buffers.h"
#include "../files.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
    uint8_t characters_length;
    char* characters;
} Predefined_Word;

typedef struct
{
    uint8_t characters_length;
    char* characters;
} Predefined_Type;

void lexical_analyze_c(Line* line, bool* continue_multiline_comment);
void lexical_analyze_owen(Line* line);
