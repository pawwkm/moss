#include "renderer.h"
#include <assert.h>

static Rectangle status_line_rectangle = { .height = STATUS_LINE_HEIGHT };
static uint16_t status_line_columns_rendered;

static void uint16_t_to_characters(uint16_t value, char (*const characters)[5], uint8_t* const characters_used)
{
    assert(value);

    *characters_used = 0;
    while (value)
    {
        const uint16_t remainder = value % 10;
        (*characters)[*characters_used] = (char)(remainder + '0');
        (*characters_used)++;

        value /= 10;
    }

    // Reverse the order of the code points.
    for (uint8_t x = 0, y = *characters_used - 1; x < y; x++, y--)
    {
        const char temp = (*characters)[x];
        (*characters)[x] = (*characters)[y];
        (*characters)[y] = temp;
    }
}

static void render_active_view_location(void)
{
    if (!editor.tabs_length)
        return;

    const View* const restrict view = find_active_editor_view();
    const Location one_indexed_location =
    {
        .line = view->offset.line + view->cursor.line + 1,
        .column = view->offset.column + view->cursor.column + 1
    };

    // Since Location uses uint16_t for lines a columns I know 
    // that the largest number that I have to render is 65535
    // which means that I only need 5 code points per number;
    char line_characters[5];
    uint8_t line_characters_length;

    uint16_t_to_characters(one_indexed_location.line, &line_characters, &line_characters_length);

    char column_characters[5];
    uint8_t column_characters_length;

    uint16_t_to_characters(one_indexed_location.column, &column_characters, &column_characters_length);

    // Render line:column
    Rectangle location_rectangle;
    location_rectangle.position.y = status_line_rectangle.position.y;
    location_rectangle.width = (line_characters_length + column_characters_length + 1) * FONT_WIDTH;
    location_rectangle.height = status_line_rectangle.height;
    location_rectangle.position.x = status_line_rectangle.width - location_rectangle.width;

    status_line_rectangle.width -= location_rectangle.width;

    uint16_t columns_rendered = 0;
    render_string(&line_characters[0], line_characters_length, &columns_rendered, Font_Color_inactive_tab_header, location_rectangle);
    render_string(":", 1, & columns_rendered, Font_Color_inactive_tab_header, location_rectangle);
    render_string(&column_characters[0], column_characters_length, &columns_rendered, Font_Color_inactive_tab_header, location_rectangle);
}

static void render_mode(void)
{
    switch (editor.mode)
    {
        case Mode_normal:
            render_string("NORMAL", 6, &status_line_columns_rendered, Font_Color_mode, status_line_rectangle);
            break;

        case Mode_insert:
            render_string("INSERT", 6, &status_line_columns_rendered, Font_Color_mode, status_line_rectangle);
            break;

        case Mode_visual:
            render_string("VISUAL", 6, &status_line_columns_rendered, Font_Color_mode, status_line_rectangle);
            break;

        case Mode_tab:
            render_string("TAB", 3, &status_line_columns_rendered, Font_Color_mode, status_line_rectangle);
            break;
    }
}

void render_status_line(void)
{
    status_line_rectangle.position.y = editor.height - STATUS_LINE_HEIGHT;
    status_line_rectangle.width = editor.width;

    set_render_draw_color(background_colors[Font_Color_mode]);
    SDL_RenderFillRect(renderer, &(SDL_Rect)
    {
        .x = status_line_rectangle.position.x,
        .y = status_line_rectangle.position.y,
        .w = status_line_rectangle.width,
        .h = status_line_rectangle.height
    });

    status_line_columns_rendered = 0;
    render_mode();

    render_active_view_location();
    if (editor.command_length)
    {
        status_line_columns_rendered++;
        const uint16_t cursor_x = (status_line_columns_rendered + editor.command_cursor) * FONT_WIDTH;

        render_string(editor.command, editor.command_length, &status_line_columns_rendered, Font_Color_inactive_tab_header, status_line_rectangle);
        render_vertical_line(foreground_colors[Font_Color_inactive_tab_header], cursor_x, status_line_rectangle.position.y, STATUS_LINE_HEIGHT);
    }
}
