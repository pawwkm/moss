#include "moss.h"
#include <stdlib.h>

// Layout of UTF-8 byte sequences.
// | Number of bytes | First code point | Last code point | Byte 1   | Byte 2   | Byte 3   | Byte 4   |
// |-----------------|------------------|-----------------|----------|----------|----------|----------|
// |        1        |           U+0000 |          U+007F | 0xxxxxxx |          |          |          |
// |        2        |           U+0080 |          U+07FF | 110xxxxx | 10xxxxxx |          |          |
// |        3        |           U+0800 |          U+FFFF | 1110xxxx | 10xxxxxx | 10xxxxxx |          |
// |        4        |          U+10000 |        U+10FFFF | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |

typedef struct
{
    uint32_t first_code_point;
    uint32_t last_code_point;
    uint8_t leading_byte;
    uint8_t mask;
    uint8_t bits_in_first_byte;
} Sequence;

Sequence sequences[4] =
{
    // The index of each entry is also the amount of bytes in the sequence.
    // The first entry is used to find invalid code points.
    // { 0x0000,  0x0000,   0x0080, 0x003F, 6 },
    
    { 0x0000,  0x007F,   0x0000, 0x007F, 7 },
    { 0x0080,  0x07FF,   0x00C0, 0x001F, 5 },
    { 0x0800,  0xFFFF,   0x00E0, 0x000F, 4 },
    { 0x10000, 0x10FFFF, 0x00F0, 0x0007, 3 }
};

static bool utf8_string_has_bom(uint8_t const* string)
{
    return string[0] == 0xEF &&
           string[1] == 0xBB &&
           string[2] == 0xBF;
}

static bool lookup_sequence_by_leading_byte(const uint8_t leading_byte, Sequence** const sequence, uint8_t* const sequence_length)
{
    for (uint8_t i = 0; i < sizeof(sequences) / sizeof(sequences[0]); i++)
    {
        Sequence* const s = &sequences[i];
        if ((leading_byte & ~s->mask) == s->leading_byte)
        {
            *sequence = s;
            *sequence_length = i + 1;

            return true;
        }
    }

    return false;
}

uint32_t utf8_to_code_point(const uint8_t* const string)
{
    Sequence* sequence;
    uint8_t sequence_length = 0;

    if (!lookup_sequence_by_leading_byte(string[0], &sequence, &sequence_length))
        return 0xFFFFFFFF;

    uint32_t shift = 6 * (sequence_length - 1);
    uint32_t code_point = (string[0] & sequence->mask) << shift;

    for (uint8_t i = 1; i < sequence_length; i++)
    {
        shift -= 6;
        code_point |= (string[i++] & 0x3F) << shift;
    }

    if (code_point < sequence->first_code_point || code_point > sequence->last_code_point)
        return 0xFFFFFFFF;

    return code_point;
}

// If null is returned the string is invalid.
uint32_t* utf8_string_to_code_points(const uint8_t* string, size_t string_length, size_t* code_points_length)
{
    if (utf8_string_has_bom(string))
    {
        string += 3;
        string_length -= 3;
    }

    *code_points_length = 0;
    for (size_t i = 0; i < string_length;)
    {
        Sequence* sequence;
        uint8_t sequence_length;
        if (!lookup_sequence_by_leading_byte(string[i], &sequence, &sequence_length))
            return NULL;

        i += sequence_length;
        (*code_points_length)++;
    }

    uint32_t* const code_points = malloc(sizeof(uint32_t) * *code_points_length);
    uint32_t* current_code_point = code_points;

    for (size_t string_index = 0; string_index < string_length;)
    {
        Sequence* sequence;
        uint8_t sequence_length;
        const uint8_t leading_byte = string[string_index++];
        
        lookup_sequence_by_leading_byte(leading_byte, &sequence, &sequence_length);

        uint32_t shift = 6 * (sequence_length - 1);
        uint32_t code_point = (leading_byte & sequence->mask) << shift;

        for (uint8_t i = 1; i < sequence_length; i++)
        {
            shift -= 6;
            code_point |= (string[string_index++] & 0x3F) << shift;
        }

        if (code_point < sequence->first_code_point || code_point > sequence->last_code_point)
        {
            free(code_points);
            return NULL;
        }

        *current_code_point++ = code_point;
    }

    return code_points;
}

void code_point_to_utf8(const uint32_t code_point, char (* const buffer)[4], uint8_t* const sequence_length)
{
    uint32_t shift = 0;
    for (uint8_t i = 0; i < sizeof(sequences) / sizeof(sequences[0]); i++)
    {
        Sequence* sequence = &sequences[i];
        if (code_point >= sequence->first_code_point && code_point <= sequence->last_code_point)
        {
            *sequence_length = i + 1;
            
            shift = 6 * (*sequence_length - 1);
            (*buffer)[0] = (code_point >> shift & sequence->mask) | sequence->leading_byte;
            shift -= 6;

            break;
        }
    }

    for (uint8_t i = 1; i < *sequence_length; i++)
    {
        (*buffer)[i] = (code_point >> shift & 0x3F) | 0x80;
        shift -= 6;
    }
}
