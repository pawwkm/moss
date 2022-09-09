#include "renderer.h"

Font_Offset font_offsets[256][Font_Color_error + 1];
static void place_font_offsets(const Font* font)
{
    for (Font_Color color = Font_Color_plain; color <= Font_Color_error; color++)
    {
        for (size_t character = 0; character < 256; character++)
        {
            bool is_valid = character >= '!' && character <= '~';
            font_offsets[character][color] = (Font_Offset)
            {
                .x = (uint16_t)(is_valid ? color : Font_Color_error) * font->width,
                .y = (is_valid ? (uint16_t)character - '!' : '?' - '!') * font->height
            };
        }
    }
}

bool initialize_font(const Font* font)
{
    place_font_offsets(font);
    bool success = allocate_font_sheet
    (
        font_offsets['~'][Font_Color_error].x + font->width, 
        font_offsets['~'][Font_Color_error].y + font->height
    );

    if (!success)
        return false;

    uint8_t w = 0;
    uint8_t h = 0;
    uint32_t bits_to_decode = font->width * font->height * CHARACTERS_IN_FONT;

    uint8_t mask = 0x80;
    const uint8_t* current = font->bitmap;
    char character = '!';
    while (bits_to_decode--)
    {
        bool bit = *current & mask;
        for (Font_Color i = Font_Color_plain; i <= Font_Color_error; i++)
        {
            success = set_font_sheet_pixel
            (
                (bit ? foreground_colors : background_colors)[i],
                font_offsets[character][i].x + w,
                font_offsets[character][i].y + h
            );

            if (!success)
                return false;
        }

        if (++w == font->width)
        {
            w = 0;
            if (++h == font->height)
            {
                h = 0;
                character++;
            }
        }

        mask >>= 1;
        if (!mask)
        {
            mask = 0x80;
            current++;
        }
    }

    return true;
}