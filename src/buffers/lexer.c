#include "buffers.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static void lexical_analyze_line(Language language, Line* line, bool* continue_multiline_comment)
{
    line->tokens_length = 0;

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

void lexical_analyze_lines(Buffer* buffer, uint16_t from, uint16_t to)
{
    if (to < from)
    {
        uint16_t temp = to;
        to = from;
        from = temp;
    }

    bool continue_multiline_comment = false;
    for (uint16_t i = from; i <= to; i++)
    {
        assert(i < buffer->lines_length);
        lexical_analyze_line(buffer->language, &buffer->lines[i], &continue_multiline_comment);
    }
}
