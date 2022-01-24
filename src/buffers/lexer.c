#include "buffers.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

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
