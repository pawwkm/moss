#include "renderer.h"

bool render_string(const uint32_t* const code_points, const uint16_t code_points_length, uint16_t* columns_rendered, const Font_Color color, const Rectangle line_rectangle)
{
    SDL_Rect destination =
    {
        .y = line_rectangle.position.y,
        .w = FONT_WIDTH,
        .h = FONT_HEIGHT
    };

    for (uint16_t i = 0; i < code_points_length; i++)
    {
        const uint32_t code_point = code_points[i];
        if (code_point == ' ')
            (*columns_rendered)++;
        else if (code_point == '\t')
            *columns_rendered += SPACES_PER_TAB - *columns_rendered % SPACES_PER_TAB;
        else
        {
            destination.x = line_rectangle.position.x + FONT_WIDTH * *columns_rendered;
            if (destination.x > line_rectangle.position.x + line_rectangle.width)
                return true;

            Font_Offset* offset = &font_offsets[code_point][color];
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
