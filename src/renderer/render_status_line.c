#include "renderer.h"
#include <assert.h>

static Rectangle status_line_rectangle = { .height = STATUS_LINE_HEIGHT };
static uint16_t status_line_columns_rendered;

static void uint16_t_to_code_points(uint16_t value, uint32_t (*const code_points)[5], uint8_t* const code_points_used)
{
    assert(value);

    *code_points_used = 0;
    while (value)
    {
        const uint16_t remainder = value % 10;
        (*code_points)[*code_points_used] = remainder + '0';
        (*code_points_used)++;

        value /= 10;
    }

    // Reverse the order of the code points.
    for (uint8_t x = 0, y = *code_points_used - 1; x < y; x++, y--)
    {
        const uint32_t temp = (*code_points)[x];
        (*code_points)[x] = (*code_points)[y];
        (*code_points)[y] = temp;
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
    uint32_t line_code_points[5];
    uint8_t line_code_points_length;

    uint16_t_to_code_points(one_indexed_location.line, &line_code_points, &line_code_points_length);

    uint32_t column_code_points[5];
    uint8_t column_code_points_length;

    uint16_t_to_code_points(one_indexed_location.column, &column_code_points, &column_code_points_length);

    // Render line:column
    Rectangle location_rectangle;
    location_rectangle.position.y = status_line_rectangle.position.y;
    location_rectangle.width = (line_code_points_length + column_code_points_length + 1) * FONT_WIDTH;
    location_rectangle.height = status_line_rectangle.height;
    location_rectangle.position.x = status_line_rectangle.width - location_rectangle.width;

    status_line_rectangle.width -= location_rectangle.width;

    uint16_t columns_rendered = 0;
    render_string(&line_code_points[0], line_code_points_length, &columns_rendered, Font_Color_inactive_tab_header, location_rectangle);
    render_string(&(uint32_t) { ':' }, 1, & columns_rendered, Font_Color_inactive_tab_header, location_rectangle);
    render_string(&column_code_points[0], column_code_points_length, &columns_rendered, Font_Color_inactive_tab_header, location_rectangle);
}

static void render_mode(void)
{
    switch (editor.mode)
    {
        case Mode_normal:
            render_string((uint32_t[]) { 0x4E, 0x4F, 0x52, 0x4D, 0x41, 0x4C }, 6, &status_line_columns_rendered, Font_Color_mode, status_line_rectangle);
            break;

        case Mode_command:
            render_string((uint32_t[]) { 0x43, 0x4F, 0x4D, 0x4D, 0x41, 0x4E, 0x44 }, 7, &status_line_columns_rendered, Font_Color_mode, status_line_rectangle);
            break;

        case Mode_insert:
            render_string((uint32_t[]) { 0x49, 0x4E, 0x53, 0x45, 0x52, 0x54 }, 6, &status_line_columns_rendered, Font_Color_mode, status_line_rectangle);
            break;

        case Mode_visual:
            render_string((uint32_t[]) { 0x56, 0x49, 0x53, 0x55, 0x41, 0x4C }, 6, &status_line_columns_rendered, Font_Color_mode, status_line_rectangle);
            break;

        case Mode_tab:
            render_string((uint32_t[]) { 0x54, 0x41, 0x42 }, 3, &status_line_columns_rendered, Font_Color_mode, status_line_rectangle);
            break;
    }
}

static void render_command_line(void)
{
    status_line_columns_rendered++;
    const uint16_t cursor_x = (status_line_columns_rendered + editor.command_cursor) * FONT_WIDTH;

    render_string(editor.command, editor.command_length, &status_line_columns_rendered, Font_Color_inactive_tab_header, status_line_rectangle);
    render_vertical_line(foreground_colors[Font_Color_inactive_tab_header], cursor_x, status_line_rectangle.position.y, STATUS_LINE_HEIGHT);
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
    if (editor.mode == Mode_command)
        render_command_line();   
}
