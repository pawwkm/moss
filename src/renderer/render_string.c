#include "renderer.h"

bool render_string(const char* const characters, const uint16_t characters_length, uint16_t* columns_rendered, const Font_Color color, const Rectangle line_rectangle)
{
    SDL_Rect destination =
    {
        .y = line_rectangle.position.y,
        .w = FONT_WIDTH,
        .h = FONT_HEIGHT
    };

    for (uint16_t i = 0; i < characters_length; i++)
    {
        // Avoid negative offset indices for non-ascii values.
        const uint8_t character = characters[i];

        if (character == ' ')
            (*columns_rendered)++;
        else if (character == '\t')
            *columns_rendered += SPACES_PER_TAB - *columns_rendered % SPACES_PER_TAB;
        else
        {
            destination.x = line_rectangle.position.x + FONT_WIDTH * *columns_rendered;
            if (destination.x > line_rectangle.position.x + line_rectangle.width)
                return true;

            Font_Offset* offset = &font_offsets[character][color];
            SDL_Rect source = 
            { 
                .x = offset->x, 
                .y = offset->y, 
                .w = FONT_WIDTH, 
                .h = FONT_HEIGHT 
            };

            SDL_RenderCopy(renderer, font_texture, &source, &destination);

            (*columns_rendered)++;
        }
    }

    return false;
}
