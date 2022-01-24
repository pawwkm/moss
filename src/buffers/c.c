#include "buffers.h"

#include <ctype.h>
#include <string.h>

static void c_multiline(Line* line, Token* token, uint16_t* index, const uint16_t start_index, bool* const continue_multiline_comment)
{
    while (*index != line->characters_length)
    {
        if (*index + 1 <= line->characters_length && line->characters[*index] == '*' && line->characters[*index + 1] == '/')
            break;

        (*index)++;
    }

    token->tag = Token_Tag_comment;
    if (*index + 1 <= line->characters_length && line->characters[*index] == '*' && line->characters[*index + 1] == '/')
    {
        *index += 2;
        token->characters_length = *index - start_index;
        *continue_multiline_comment = false;
    }
    else
    {
        token->characters_length = line->characters_length - start_index;
        *continue_multiline_comment = true;

        return;
    }
}

static bool is_c_hex_quad(const Line* const line, uint16_t index)
{
    return index + 3 <= line->characters_length  &&
           isxdigit(line->characters[index])     &&
           isxdigit(line->characters[index + 1]) &&
           isxdigit(line->characters[index + 2]) &&
           isxdigit(line->characters[index + 3]);
}

// true if the next code point(s) is a valid or forms a valid escape sequence.
static bool lex_c_character(Line* line, uint16_t* index, bool is_string)
{
    if (*index + 1 <= line->characters_length && line->characters[*index] == '\\')
    {
        switch (line->characters[++(*index)])
        {
            case '"':
                if (is_string) 
                {
                    (*index)++;
                    return true;
                }
                else
                    return false;

            case '\'':
                if (!is_string)
                {
                    (*index)++;
                    return true;
                }
                else
                    return false;

            case '?':
            case '\\':
            case 'a':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case 'v':
                (*index)++;
                return true;

            // Octal.
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                if (*index != line->characters_length && line->characters[*index] >= '0' && line->characters[*index] <= '7')
                    (*index)++;

                if (*index != line->characters_length && line->characters[*index] >= '0' && line->characters[*index] <= '7')
                    (*index)++;

                return true;

            case 'x':
                (*index)++;
                if (*index != line->characters_length && isxdigit(line->characters[*index]))
                {
                    (*index)++;
                    while (*index != line->characters_length && isxdigit(line->characters[*index]))
                        (*index)++;

                    return true;
                }
                else
                    return false;

            case 'u':
                if (is_c_hex_quad(line, *index))
                {
                    *index += 3;
                    return true;
                }
                else
                    return false;

            case 'U':
                if (is_c_hex_quad(line, *index) && is_c_hex_quad(line, *index + 3))
                {
                    *index += 7;
                    return true;
                }
                else
                    return false;

            default:
                return false;
        }
    }
    else if (*index != line->characters_length)
    {
        (*index)++;
        return true;
    }
    else 
        return false;
}

// These are sorted by length ascending order which 
// is used to bail out of a loop early if next type
// is longer than expected.
static Predefined_Type* predefined_c_types;
static uint16_t predefined_c_types_length;
static uint16_t predefined_c_types_capacity;

static bool last_token_is_preceeded_by_pound_sign(const Line* const line)
{
    for (int32_t i = line->tokens_length - 2; i >= 0; i--)
    {
        const Token* const token = &line->tokens[i];
        if (token->tag == Token_Tag_white_space)
            continue;
        else
            return token->characters_length == 1 && token->characters[0] == '#';
    }

    return false;
}

void lexical_analyze_c(Line* line, bool* continue_multiline_comment)
{
    // These are sorted by length ascending order which 
    // is used to bail out of a loop early if next keyword
    // is longer than expected.
    static Predefined_Word keywords[47] =
    {
        { 2,  "if"             },
        { 2,  "do"             },
        { 3,  "int"            },
        { 3,  "for"            },
        { 4,  "case"           },
        { 4,  "char"           },
        { 4,  "auto"           },
        { 4,  "goto"           },
        { 4,  "long"           },
        { 4,  "else"           },
        { 4,  "enum"           },
        { 4,  "void"           },
        { 5,  "break"          },
        { 5,  "const"          },
        { 5,  "float"          },
        { 5,  "union"          },
        { 5,  "short"          },
        { 5,  "while"          },
        { 5,  "_Bool"          },
        { 6,  "double"         },
        { 6,  "extern"         },
        { 6,  "inline"         },
        { 6,  "return"         },
        { 6,  "signed"         },
        { 6,  "sizeof"         },
        { 6,  "static"         },
        { 6,  "struct"         },
        { 6,  "switch"         },
        { 7,  "default"        },
        { 7,  "_Atomic"        },
        { 7,  "typedef"        },
        { 8,  "continue"       },
        { 8,  "register"       },
        { 8,  "restrict"       },
        { 8,  "unsigned"       },
        { 8,  "volatile"       },
        { 8,  "_Alignas"       },
        { 8,  "_Alignof"       },
        { 8,  "_Complex"       },
        { 8,  "_Generic"       },
        { 9,  "_Noreturn"      },
        { 11, "_Decimal128"    },
        { 10, "_Decimal32"     },
        { 10, "_Decimal64"     },
        { 10, "_Imaginary"     },
        { 13, "_Thread_local"  },
        { 14, "_Static_assert" }
    };

    uint16_t index = 0;
    if (index == line->characters_length && *continue_multiline_comment)
        // This is an empty line inside of a multiline comment.
        // Return now to keep *continue_multiline_comment == true.
        return;

    while (index != line->characters_length)
    {
        const uint16_t start_index = index;

        Token* const token = add_token(line);
        token->characters = &line->characters[index];
        
        if (*continue_multiline_comment)
        {
            c_multiline(line, token, &index, start_index, continue_multiline_comment);
            if (*continue_multiline_comment)
                return;
        }
        else if (isspace(line->characters[index]))
        {
            while (index != line->characters_length && isspace(line->characters[index]))
                index++;

            token->characters_length = index - start_index;
            token->tag = Token_Tag_white_space;
        }
        else if (index + 1 != line->characters_length && line->characters[index] == '/' && line->characters[index + 1] == '/')
        {
            token->characters_length = line->characters_length - start_index;
            token->tag = Token_Tag_comment;

            break;
        }
        else if (index + 1 <= line->characters_length && line->characters[index] == '/' && line->characters[index + 1] == '*')
        {
            index++;
            
            c_multiline(line, token, &index, start_index, continue_multiline_comment);
            if (*continue_multiline_comment)
                return;
        }
        else if (line->characters[index] == '"' || index + 1 <= line->characters_length && line->characters[index] == 'L' && line->characters[index + 1] == '"')
        {
            if (line->characters[index] == 'L')
                index += 2;
            else
                index++;

            while (index != line->characters_length && line->characters[index] != '"')
            {
                if (!lex_c_character(line, &index, true))
                    break;
            }

            if (index != line->characters_length && line->characters[index] == '"')
            {
                index++;
                token->characters_length = index - start_index;
                token->tag = Token_Tag_string;
            }
            else
                token->characters_length = index - start_index;
        }
        else if (line->characters[index] == '\'' || index + 1 <= line->characters_length && line->characters[index] == 'L' && line->characters[index + 1] == '\'')
        {
            if (line->characters[index] == 'L')
                index += 2;
            else
                index++;

            if (index != line->characters_length && line->characters[index] == '\'')
            {
                index++;
                token->characters_length = index - start_index;
            }
            else
            {
                while (index != line->characters_length && line->characters[index] != '\'')
                {
                    if (!lex_c_character(line, &index, false))
                        break;
                }

                if (index != line->characters_length && line->characters[index] == '\'')
                {
                    index++;
                    token->characters_length = index - start_index;
                    token->tag = Token_Tag_string;
                }
                else
                    token->characters_length = index - start_index;
            }
        }
        else if (ispunct(line->characters[index]))
        {
            index++;
            token->characters_length = index - start_index;
        }
        else if (isalpha(line->characters[index]))
        {
            do
                index++;
            while (index != line->characters_length && (isalnum(line->characters[index]) || line->characters[index] == '_'));

            token->characters_length = index - start_index;
            for (uint8_t k = 0; k < sizeof(keywords) / sizeof(keywords[0]); k++)
            {
                Predefined_Word* const keyword = &keywords[k];
                if (keyword->characters_length > token->characters_length)
                    break;

                if (keyword->characters_length != token->characters_length)
                    continue;

                if (!strncmp(token->characters, keyword->characters, keyword->characters_length))
                {
                    if (k != 0 || !last_token_is_preceeded_by_pound_sign(line))
                        token->tag = Token_Tag_keyword;

                    break;
                }
            }

            if (token->tag == Token_Tag_keyword)
                continue;

            // If the identifier ends with _t it is probably a type.
            if (token->characters_length >= 2 && line->characters[index - 2] == '_' && line->characters[index - 1] == 't')
            {
                token->tag = Token_Tag_type;
                continue;
            }

            for (uint16_t t = 0; t < predefined_c_types_length; t++)
            {
                Predefined_Type* type = &predefined_c_types[t];
                if (type->characters_length > token->characters_length)
                    break;

                if (type->characters_length != token->characters_length)
                    continue;

                if (!memcmp(token->characters, type->characters, sizeof(type->characters[0]) * type->characters_length))
                {
                    token->tag = Token_Tag_type;
                    break;
                }
            }

            if (token->tag == Token_Tag_type)
                continue;

            // Owen style types.
            if (isupper(token->characters[0]))
            {
                uint16_t i = 0;
                do
                {
                    i++;
                    while (i != token->characters_length && (islower(token->characters[i]) || isdigit(token->characters[i])))
                        i++;

                    if (i + 1 <= token->characters_length && token->characters[i] == '_' && isupper(token->characters[i + 1]))
                        i++;
                    else
                        break;

                } while (isupper(token->characters[i]));
                
                if (i == token->characters_length)
                    token->tag = Token_Tag_type;
            }
        }
        else
        {
            while (index != line->characters_length && !isspace(line->characters[index]) && !ispunct(line->characters[index]))
                index++;

            token->characters_length = index - start_index;
        }
    }

    *continue_multiline_comment = false;
}

static int compare_keyword_length_ascending(const void* a, const void* b)
{
    return ((Predefined_Type*)a)->characters_length - ((Predefined_Type*)b)->characters_length;
}

bool initialize_lexer(void)
{
    size_t file_length;
    char* file = read_file("c_types.txt", &file_length);
    if (!file)
    {
        show_initialization_error_message("Could not read c_types.txt");
        return false;
    }

    size_t file_index = 0;
    while (file_index != file_length)
    {
        size_t file_start_index = file_index;
        size_t end_of_line_index = 0;

        while (file_index != file_length)
        {
            if (file[file_index] == '\n')
            {
                end_of_line_index = file_index;
                file_index++;

                break;
            }
            else
                end_of_line_index = ++file_index;
        }

        if (predefined_c_types_length == predefined_c_types_capacity)
        {
            predefined_c_types_capacity = predefined_c_types_capacity ? predefined_c_types_capacity * 2 : 4;
            predefined_c_types = realloc(predefined_c_types, sizeof(predefined_c_types[0]) * predefined_c_types_capacity);
        }

        predefined_c_types[predefined_c_types_length++] = (Predefined_Type)
        {
            .characters = &file[file_start_index],
            .characters_length = (uint8_t)(end_of_line_index - file_start_index)
        };
    }

    qsort(predefined_c_types, predefined_c_types_length, sizeof(Predefined_Type), compare_keyword_length_ascending);

    return true;
}
