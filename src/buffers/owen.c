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
    static String_Slice keywords[] = 
    {
        { 2,  "if"          },
        { 3,  "use"         },
        { 3,  "end"         },
        { 3,  "for"         },
        { 4,  "elif"        },
        { 4,  "else"        },
        { 4,  "true"        },
        { 4,  "null"        },
        { 5,  "while"       },
        { 5,  "break"       },
        { 5,  "union"       },
        { 5,  "false"       },
        { 6,  "public"      },
        { 6,  "sizeof"      },
        { 6,  "return"      },
        { 7,  "version"     },
        { 8,  "external"    },
        { 8,  "function"    },
        { 8,  "continue"    },
        { 9,  "structure"   },
        { 9,  "namespace"   },
        { 11, "enumeration" },
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
                    String_Slice* keyword = &keywords[k];
                    if (keyword->characters_length > token->characters_length)
                        break;

                    if (keyword->characters_length != token->characters_length)
                        continue;

                    if (!strncmp(token->characters, keyword->characters, keyword->characters_length))
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

                    // Upper case identifiers preceded by the namespace or use 
                    // keywords are not types.
                    if (previous_token->characters_length == 3 && !strncmp("use",       previous_token->characters, 3) ||
                        previous_token->characters_length == 9 && !strncmp("namespace", previous_token->characters, 9))
                        token->tag = Token_Tag_plain;

                    break;
                }
            }
        }
        else
        {
            while (index != line->characters_length && !isspace(line->characters[index]) && !ispunct(line->characters[index]))
                index++;

            token->characters_length = index - start_index;
        }
    }
}
