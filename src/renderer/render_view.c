#include "renderer.h"

void render_view(const View* const view, const Rectangle view_rectangle)
{
    const bool is_active_view = view == find_active_editor_view();
    const Buffer* const buffer = buffer_handle_to_pointer(view->buffer);

    for (uint16_t l = 0; l + view->offset.line < buffer->lines_length; l++)
    {
        const Line* const line = &buffer->lines[l + view->offset.line];
        const bool render_cursor = is_active_view && view->cursor.line == l;
        const Rectangle line_rectangle =
        {
            .position.x = view_rectangle.position.x,
            .position.y = view_rectangle.position.y + FONT_HEIGHT * l,
            .width = view_rectangle.width,
            .height = FONT_HEIGHT
        };

        render_line(line, view->offset, view->cursor, render_cursor, line_rectangle);
        if (view_rectangle.position.y + (l - 1) * FONT_HEIGHT > view_rectangle.height)
            break;
    }
}
