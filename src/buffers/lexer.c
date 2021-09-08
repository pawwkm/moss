#include "buffers.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

bool compare_predefined_word(Predefined_Word* const word, const uint16_t index, const uint32_t* const restrict code_points, const uint16_t code_points_length)
{
    if (!word->code_points_length || index + word->code_points_length > code_points_length)
        return false;

    return !memcmp(word->code_points, &code_points[index], sizeof(word->code_points[0]) * word->code_points_length);
}

void lexical_analyze(Language language, Line* line, bool* continue_multiline_comment)
{
    switch (language)
    {
        case Language_none:
            Token* token = line->tokens_length ? line->tokens : add_token(line);
            token->code_points = line->code_points;
            token->code_points_length = line->code_points_length;
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
