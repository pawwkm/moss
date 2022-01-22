#include "buffers.h"

static bool is_owen_hex_digit(const uint32_t character)
{
    return isdigit(character) || 
           character >= 'A' && character <= 'F';
}
static bool is_owen_hex_quad(const Line* const line, const uint16_t index)
{
    return index + 3 <= line->characters_length &&
           is_owen_hex_digit(line->characters[index]) &&
           is_owen_hex_digit(line->characters[index + 1]) &&
           is_owen_hex_digit(line->characters[index + 2]) &&
           is_owen_hex_digit(line->characters[index + 3]);
}

static bool lex_owen_character(Line* const line, uint16_t* const index, const bool is_string)
{
    if (*index + 1 <= line->characters_length && line->characters[*index] == '\\')
    {
        (*index)++;
        if (line->characters[*index] == 'u' && *index + 4 <= line->characters_length && is_owen_hex_quad(line, *index + 1))
            *index += 4;
        else if (line->characters[*index] == 'U' && *index + 7 <= line->characters_length && is_owen_hex_quad(line, *index + 1) && is_owen_hex_quad(line, *index + 4))
            *index += 8;
    }
    else if (is_string && line->characters[*index] == '"' || !is_string && line->characters[*index] == '\'')
        return false;
    else if (line->characters[*index] == '\t')
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
    while (index != line->characters_length)
    {
        uint16_t start_index = index;

        Token* token = add_token(line);
        token->characters = &line->characters[index];

        if (isspace(line->characters[index]))
        {
            while (index != line->characters_length && isspace(line->characters[index]))
                index++;

            token->characters_length = index - start_index;
            token->tag = Token_Tag_white_space;
        }
        else if (index + 1 <= line->characters_length && line->characters[index] == '/' && line->characters[index + 1] == '/')
        {
            token->characters_length = line->characters_length - start_index;
            token->tag = Token_Tag_comment;

            return;
        }
        else if (line->characters[index] == '\'')
        {
            index++;
            if (index != line->characters_length && lex_owen_character(line, &index, false))
            {
                if (index != line->characters_length && line->characters[index] == '\'')
                {
                    index++;
                    token->tag = Token_Tag_string;
                }
            }

            token->characters_length = index - start_index;
        }
        else if (line->characters[index] == '"')
        {
            do
                index++;
            while (index != line->characters_length && line->characters[index] != '"');

            if (index != line->characters_length && line->characters[index] == '"')
                index++;

            token->characters_length = index - start_index;
            token->tag = Token_Tag_string;
        }
        else if (ispunct(line->characters[index]))
        {
            index++;
            token->characters_length = index - start_index;
        }
        else if (islower(line->characters[index]))
        {
            index++;
            while (index != line->characters_length && (islower(line->characters[index]) || isdigit(line->characters[index])))
            {
                index++;
                if (index + 1 <= line->characters_length && line->characters[index] == '_' && islower(line->characters[index + 1]))
                    index++;
            }

            if (index != line->characters_length && (isalnum(line->characters[index]) || line->characters[index] == '_'))
            {
                // This is not a conforming identifier but to make sure that the lexer
                // doesn't think that the following code points are we consume them now.
                while (index != line->characters_length && (isalnum(line->characters[index]) || line->characters[index] == '_'))
                    index++;

                token->characters_length = index - start_index;
            }
            else
            {
                token->characters_length = index - start_index;
                for (uint8_t k = 0; k < sizeof(keywords) / sizeof(keywords[0]); k++)
                {
                    Predefined_Word* keyword = &keywords[k];
                    if (keyword->characters_length > token->characters_length)
                        break;

                    if (keyword->characters_length != token->characters_length)
                        continue;

                    if (!memcmp(token->characters, &keyword->characters, sizeof(keyword->characters[0]) * keyword->characters_length))
                    {
                        token->tag = Token_Tag_keyword;
                        break;
                    }
                }
            }
        }
        else if (isupper(line->characters[index]))
        {
            do
            {
                index++;
                while (index != line->characters_length && (islower(line->characters[index]) || isdigit(line->characters[index])))
                    index++;

                if (index + 1 <= line->characters_length && line->characters[index] == '_' && isupper(line->characters[index + 1]))
                    index++;
                else
                    break;

            } while (isupper(line->characters[index]));

            if (index != line->characters_length && (isalnum(line->characters[index]) || line->characters[index] == '_'))
            {
                // This is not a conforming identifier but to make sure that the lexer
                // doesn't think that the following code points are we consume them now.
                while (index != line->characters_length && (isalnum(line->characters[index]) || line->characters[index] == '_'))
                    index++;

                token->characters_length = index - start_index;
            }
            else
            {
                token->characters_length = index - start_index;
                token->tag = Token_Tag_type;

                // Check if this really is a type.
                for (int16_t i = line->tokens_length - 2; i >= 0; i--)
                {
                    Token* previous_token = &line->tokens[i];
                    if (previous_token->tag == Token_Tag_white_space)
                        continue;

                    if (previous_token->tag != Token_Tag_keyword)
                        break;

                    if (compare_predefined_word(&keywords[1], 0, previous_token->characters, previous_token->characters_length) ||
                        compare_predefined_word(&keywords[21], 0, previous_token->characters, previous_token->characters_length))
                        token->tag = Token_Tag_plain;

                    break;
                }
            }
        }
        else
        {
            // Pretend that there is only 127 characters.
            while (index != line->characters_length && !isspace(line->characters[index]) && !ispunct(line->characters[index]))
                index++;

            token->characters_length = index - start_index;
        }
    }
}
