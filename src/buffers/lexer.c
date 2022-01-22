#include "buffers.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

bool compare_predefined_word(Predefined_Word* const word, const uint16_t index, const char* const restrict characters, const uint16_t characters_length)
{
    if (!word->characters_length || index + word->characters_length > characters_length)
        return false;

    return !memcmp(word->characters, &characters[index], sizeof(word->characters[0]) * word->characters_length);
}

void lexical_analyze(Language language, Line* line, bool* continue_multiline_comment)
{
    switch (language)
    {
        case Language_none:
            Token* token = line->tokens_length ? line->tokens : add_token(line);
            token->characters = line->characters;
            token->characters_length = line->characters_length;
            token->tag = Token_Tag_plain;
            break;

        case Language_c:
            lexical_analyze_c(line, continue_multiline_comment);
            break;

        case Language_owen:
            lexical_analyze_owen(line);
            break;
    }
}
