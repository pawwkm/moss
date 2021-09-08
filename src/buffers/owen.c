#include "buffers.h"

static bool is_owen_hex_digit(const uint32_t code_point)
{
    return isdigit(code_point) || 
           code_point >= 'A' && code_point <= 'F';
}
static bool is_owen_hex_quad(const Line* const line, const uint16_t index)
{
    return index + 3 <= line->code_points_length &&
           is_owen_hex_digit(line->code_points[index]) &&
           is_owen_hex_digit(line->code_points[index + 1]) &&
           is_owen_hex_digit(line->code_points[index + 2]) &&
           is_owen_hex_digit(line->code_points[index + 3]);
}

static bool lex_owen_character(Line* const line, uint16_t* const index, const bool is_string)
{
    if (*index + 1 <= line->code_points_length && line->code_points[*index] == '\\')
    {
        (*index)++;
        if (line->code_points[*index] == 'u' && *index + 4 <= line->code_points_length && is_owen_hex_quad(line, *index + 1))
            *index += 4;
        else if (line->code_points[*index] == 'U' && *index + 7 <= line->code_points_length && is_owen_hex_quad(line, *index + 1) && is_owen_hex_quad(line, *index + 4))
            *index += 8;
    }
    else if (is_string && line->code_points[*index] == '"' || !is_string && line->code_points[*index] == '\'')
        return false;
    else if (line->code_points[*index] == '\t')
        // Some characters will be invalid in literals down the line.
        return false;

    (*index)++;

    return true;
}

void lexical_analyze_owen(Line* const line)
{
    // These are sorted by length ascending order which 
    // is used to bail out of a loop early if next keyword
    // is longer than expected.
    static Predefined_Word keywords[] = 
    {
        // if
        { 2, { 0x69, 0x66 } },

        // use
        { 3, { 0x75, 0x73, 0x65 } },

        // end
        { 3, { 0x65, 0x6E, 0x64 } },

        // for
        { 3, { 0x66, 0x6F, 0x72 } },

        // elif
        { 4, { 0x65, 0x6C, 0x69, 0x66 } },

        // else
        { 4, { 0x65, 0x6C, 0x73, 0x65 } },

        // true
        { 4, { 0x74, 0x72, 0x75, 0x65 } },

        // null
        { 4, { 0x6E, 0x75, 0x6C, 0x6C } },

        // while
        { 5, { 0x77, 0x68, 0x69, 0x6C, 0x65 } },

        // break
        { 5, { 0x62, 0x72, 0x65, 0x61, 0x6B } },

        // union
        { 5, { 0x75, 0x6E, 0x69, 0x6F, 0x6E } },

        // false
        { 5, { 0x66, 0x61, 0x6C, 0x73, 0x65 } },

        // public
        { 6, { 0x70, 0x75, 0x62, 0x6C, 0x69, 0x63 } },

        // sizeof
        { 6, { 0x73, 0x69, 0x7A, 0x65, 0x6F, 0x66 } },

        // return
        { 6, { 0x72, 0x65, 0x74, 0x75, 0x72, 0x6E } },

        // assert
        { 6, { 0x61, 0x73, 0x73, 0x65, 0x72, 0x74 } },

        // version
        { 7, { 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E } },

        // external
        { 8, { 0x65, 0x78, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C } },

        // function
        { 8, { 0x66, 0x75, 0x6E, 0x63, 0x74, 0x69, 0x6F, 0x6E } },

        // continue
        { 8, { 0x63, 0x6F, 0x6E, 0x74, 0x69, 0x6E, 0x75, 0x65 } },

        // structure
        { 9, { 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x75, 0x72, 0x65 } },

        // namespace      
        { 9, { 0x6E, 0x61, 0x6D, 0x65, 0x73, 0x70, 0x61, 0x63, 0x65 } },

        // proposition
        { 11, { 0x70, 0x72, 0x6F, 0x70, 0x6F, 0x73, 0x69, 0x74, 0x69, 0x6F, 0x6E } },

        // enumeration
        { 11, { 0x65, 0x6E, 0x75, 0x6D, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6F, 0x6E } },
    };

    uint16_t index = 0;
    while (index != line->code_points_length)
    {
        uint16_t start_index = index;

        Token* token = add_token(line);
        token->code_points = &line->code_points[index];

        if (isspace(line->code_points[index]))
        {
            while (index != line->code_points_length && isspace(line->code_points[index]))
                index++;

            token->code_points_length = index - start_index;
            token->tag = Token_Tag_white_space;
        }
        else if (index + 1 <= line->code_points_length && line->code_points[index] == '/' && line->code_points[index + 1] == '/')
        {
            token->code_points_length = line->code_points_length - start_index;
            token->tag = Token_Tag_comment;

            return;
        }
        else if (line->code_points[index] == '\'')
        {
            index++;
            if (index != line->code_points_length && lex_owen_character(line, &index, false))
            {
                if (index != line->code_points_length && line->code_points[index] == '\'')
                {
                    index++;
                    token->tag = Token_Tag_string;
                }
            }

            token->code_points_length = index - start_index;
        }
        else if (line->code_points[index] == '"')
        {
            do
                index++;
            while (index != line->code_points_length && line->code_points[index] != '"');

            if (index != line->code_points_length && line->code_points[index] == '"')
                index++;

            token->code_points_length = index - start_index;
            token->tag = Token_Tag_string;
        }
        else if (ispunct(line->code_points[index]))
        {
            index++;
            token->code_points_length = index - start_index;
        }
        else if (islower(line->code_points[index]))
        {
            index++;
            while (index != line->code_points_length && (islower(line->code_points[index]) || isdigit(line->code_points[index])))
            {
                index++;
                if (index + 1 <= line->code_points_length && line->code_points[index] == '_' && islower(line->code_points[index + 1]))
                    index++;
            }

            if (index != line->code_points_length && (isalnum(line->code_points[index]) || line->code_points[index] == '_'))
            {
                // This is not a conforming identifier but to make sure that the lexer
                // doesn't think that the following code points are we consume them now.
                while (index != line->code_points_length && (isalnum(line->code_points[index]) || line->code_points[index] == '_'))
                    index++;

                token->code_points_length = index - start_index;
            }
            else
            {
                token->code_points_length = index - start_index;
                for (uint8_t k = 0; k < sizeof(keywords) / sizeof(keywords[0]); k++)
                {
                    Predefined_Word* keyword = &keywords[k];
                    if (keyword->code_points_length > token->code_points_length)
                        break;

                    if (keyword->code_points_length != token->code_points_length)
                        continue;

                    if (!memcmp(token->code_points, &keyword->code_points, sizeof(keyword->code_points[0]) * keyword->code_points_length))
                    {
                        token->tag = Token_Tag_keyword;
                        break;
                    }
                }
            }
        }
        else if (isupper(line->code_points[index]))
        {
            do
            {
                index++;
                while (index != line->code_points_length && (islower(line->code_points[index]) || isdigit(line->code_points[index])))
                    index++;

                if (index + 1 <= line->code_points_length && line->code_points[index] == '_' && isupper(line->code_points[index + 1]))
                    index++;
                else
                    break;

            } while (isupper(line->code_points[index]));

            if (index != line->code_points_length && (isalnum(line->code_points[index]) || line->code_points[index] == '_'))
            {
                // This is not a conforming identifier but to make sure that the lexer
                // doesn't think that the following code points are we consume them now.
                while (index != line->code_points_length && (isalnum(line->code_points[index]) || line->code_points[index] == '_'))
                    index++;

                token->code_points_length = index - start_index;
            }
            else
            {
                token->code_points_length = index - start_index;
                token->tag = Token_Tag_type;

                // Check if this really is a type.
                for (int16_t i = line->tokens_length - 2; i >= 0; i--)
                {
                    Token* previous_token = &line->tokens[i];
                    if (previous_token->tag == Token_Tag_white_space)
                        continue;

                    if (previous_token->tag != Token_Tag_keyword)
                        break;

                    if (compare_predefined_word(&keywords[1], 0, previous_token->code_points, previous_token->code_points_length) ||
                        compare_predefined_word(&keywords[21], 0, previous_token->code_points, previous_token->code_points_length))
                        token->tag = Token_Tag_plain;

                    break;
                }
            }
        }
        else
        {
            // Pretend that there is only 127 characters.
            while (index != line->code_points_length && !isspace(line->code_points[index]) && !ispunct(line->code_points[index]))
                index++;

            token->code_points_length = index - start_index;
        }
    }
}
