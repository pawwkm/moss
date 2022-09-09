#include "renderer.h"

bool render_clipped_string(const Editor* editor, const char* characters, uint16_t characters_length, uint16_t* columns_rendered, Font_Color color, uint16_t cursor_column, bool render_cursor_column, Block surface, Block line_block)
{
    Block clipped_line_block = intersecting_block(surface, line_block);
    if (is_empty_block(clipped_line_block))
        return false;

    uint16_t i = 0;
    while (i < characters_length)
    {
        // Avoid negative offset indices for non-ascii values.
        uint8_t character = characters[i];

        if (is_tab_or_space(character))
        {
            Block white_space =
            {
                .x = clipped_line_block.x + editor->font_width * *columns_rendered,
                .y = clipped_line_block.y,
                .width = 0,
                .height = clipped_line_block.height
            };

            if (render_cursor_column && cursor_column == *columns_rendered)
            {
                render_cursor(editor, character, white_space.x, white_space.y);
                white_space.x += editor->font_width;
            }

            while (true)
            {
                i++;
                if (character == ' ')
                {
                    white_space.width += editor->font_width;
                    (*columns_rendered)++;
                }
                else if (character == '\t')
                {
                    uint16_t spaces = editor->spaces_per_tab - *columns_rendered % editor->spaces_per_tab;
                    white_space.width += spaces * editor->font_width;
                    *columns_rendered += spaces;
                }

                if (i == characters_length || !is_tab_or_space(characters[i]))
                    break;

                character = characters[i];
            }

            render_clipped_block(background_colors[color], clipped_line_block, white_space);
        }
        else
        {
            uint16_t x = editor->font_width * *columns_rendered;
            if (x >= clipped_line_block.width)
                return true;

            if (render_cursor_column && cursor_column == *columns_rendered)
                render_cursor(editor, character, clipped_line_block.x + x, clipped_line_block.y);
            else
            {
                Font_Offset fo = font_offsets[character][color];

                // This needs to be the difference in surface and line block.
                Block clipped_character_block =
                {
                    .x = clipped_line_block.x - line_block.x,
                    .y = clipped_line_block.y - line_block.y,
                    .width = min_u16(clipped_line_block.width - x, editor->font_width),
                    .height = clipped_line_block.height
                };

                render_character
                (
                    fo.x + clipped_character_block.x,
                    fo.y + clipped_character_block.y,
                    clipped_line_block.x + x,
                    clipped_line_block.y,
                    clipped_character_block.width,
                    clipped_character_block.height
                );
            }

            (*columns_rendered)++;
            i++;
        }
    }

    return false;
}