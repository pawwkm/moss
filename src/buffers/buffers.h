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

void lexical_analyze_c(Line* line, bool* continue_multiline_comment);
void lexical_analyze_owen(Line* line);
