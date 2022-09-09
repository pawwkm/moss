#include "renderer.h"

static Font_Color token_tag_to_font_color[] =
{
    [Token_Tag_plain]       = Font_Color_plain,
    [Token_Tag_comment]     = Font_Color_comment,
    [Token_Tag_keyword]     = Font_Color_keyword,
    [Token_Tag_string]      = Font_Color_string,
    [Token_Tag_type]        = Font_Color_type,
    [Token_Tag_white_space] = Font_Color_plain
};

void render_line(const Editor* editor, const Line* line, Location offset, Location cursor, bool render_cursor_column, Block surface, Block line_block)
{
    uint16_t columns_rendered = 0;
    uint16_t characters_to_skip = offset.column;

    uint16_t cursor_column = offset.column + cursor.column;
    bool is_cursor_eol = cursor_column == index_of_character_append(line);

    for (uint16_t i = 0; i < line->tokens_length; i++)
    {
        const Token* token = &line->tokens[i];
        Font_Color color = token_tag_to_font_color[token->tag];

        bool stop = false;
        if (characters_to_skip >= token->characters_length)
        {
            characters_to_skip -= token->characters_length;
            continue;
        }
        else if (characters_to_skip < token->characters_length)
        {
            stop = render_clipped_string(editor, token->characters + characters_to_skip, token->characters_length - characters_to_skip, &columns_rendered, color, cursor.column, render_cursor_column, surface, line_block);
            characters_to_skip = 0;
        }
        else
            stop = render_clipped_string(editor, token->characters, token->characters_length, &columns_rendered, color, cursor.column, render_cursor_column, surface, line_block);

        if (stop)
            break;
    }

    uint16_t x_pixels_used = columns_rendered * editor->font_width;
    if (x_pixels_used < line_block.width)
    {
        if (render_cursor_column && is_cursor_eol)
        {
            render_cursor(editor, ' ', x_pixels_used + line_block.x, line_block.y);
            x_pixels_used += editor->font_width;
        }

        // Render the blank space remaining past the last token on the line.
        render_clipped_block(background_colors[Font_Color_plain], surface, (Block)
        {
            .x = x_pixels_used + line_block.x,
            .y = line_block.y,
            .width = line_block.width - x_pixels_used,
            .height = line_block.height
        });
    }
}
