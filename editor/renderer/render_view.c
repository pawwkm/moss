#include "renderer.h"

void render_view(const Editor* editor, const View* const view, Block surface, Block view_block)
{
    if (is_empty_block(intersecting_block(surface, view_block)))
        return;

    bool is_active_view = view == find_active_editor_view(editor);
    Buffer* buffer = lookup_buffer(view->buffer);
    Block remainder = view_block;

    for (uint16_t l = 0; l + view->offset.line < buffer->lines_length; l++)
    {
        Line* line = &buffer->lines[l + view->offset.line];
        bool render_cursor = is_active_view && view->cursor.line == l;
        Block line_block = intersecting_block(view_block, (Block)
        {
            .x = view_block.x,
            .y = view_block.y + editor->font_height * l,
            .width = view_block.width,
            .height = editor->font_height
        });

        render_line(editor, line, view->offset, view->cursor, render_cursor, surface, line_block);
        if (line_block.y + line_block.height >= surface.y + surface.height ||
            line_block.y + line_block.height >= view_block.y + view_block.height)
            return;

        remainder.y += line_block.height;
        remainder.height -= line_block.height;
    }

    // Draw whitespace below the last line.
    render_clipped_block(background_colors[Font_Color_plain], surface, remainder);
}
