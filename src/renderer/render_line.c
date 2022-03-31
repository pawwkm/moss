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

static uint16_t logical_columns_to_rendered_columns(const Line* const line, const uint16_t from, const uint16_t to)
{
    uint16_t column = 0;
    for (uint16_t i = from; i < to; i++)
    {
        if (line->characters[i] == '\t')
            column += SPACES_PER_TAB - column % SPACES_PER_TAB;
        else
            column++;
    }

    return column;
}

static void render_block_cursor(const uint16_t cursor_column, const Point position)
{
    set_render_draw_color(background_colors[Font_Color_selected]);
    SDL_RenderFillRect(renderer, &(SDL_Rect)
    {
        .x = position.x + FONT_WIDTH * cursor_column,
        .y = position.y,
        .w = FONT_WIDTH,
        .h = FONT_HEIGHT
    });
}

void render_line(const Line* line, const Location offset, const Location cursor, const bool render_cursor, const Rectangle line_rectangle)
{
    uint16_t columns_rendered = 0;
    uint16_t characters_to_skip = offset.column;
    for (uint16_t t = 0; t < line->tokens_length; t++)
    {
        const Token* const token = &line->tokens[t];
        const Font_Color color = token_tag_to_font_color[token->tag];

        bool stop = false;
        if (characters_to_skip >= token->characters_length)
        {
            characters_to_skip -= token->characters_length;
            continue;
        }
        else if (characters_to_skip < token->characters_length)
        {
            stop = render_string(token->characters + characters_to_skip, token->characters_length - characters_to_skip, &columns_rendered, color, line_rectangle);
            characters_to_skip = 0;
        }
        else
            stop = render_string(token->characters, token->characters_length, &columns_rendered, color, line_rectangle);

        if (stop)
            break;
    }

    if (!render_cursor)
        return;

    // The cursor is rendered after the line is rendered.
    // This prevents a token being accidentally rendered 
    // over the cursor after adjusting for tabs.

    uint16_t cursor_column = logical_columns_to_rendered_columns(line, offset.column, offset.column + cursor.column);
    if (editor.mode == Mode_normal)
    {
        if (offset.column + cursor.column == index_of_character_append(line))
            render_block_cursor(cursor_column, line_rectangle.position);
        else
        {
            const uint8_t character_under_cursor = line->characters[offset.column + cursor.column];
            if (character_under_cursor == ' ' || character_under_cursor == '\t')
                render_block_cursor(cursor_column, line_rectangle.position);
            else
                render_string(&line->characters[offset.column + cursor.column], 1, &cursor_column, Font_Color_selected, line_rectangle);
        }
    }
    else if (editor.mode == Mode_insert)
    {
        const int x = line_rectangle.position.x + FONT_WIDTH * cursor_column;

        set_render_draw_color(foreground_colors[Font_Color_plain]);
        SDL_RenderDrawLine(renderer, x, line_rectangle.position.y, x, line_rectangle.position.y + line_rectangle.height - 1);
    }
}
