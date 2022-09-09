#include "renderer.h"

#include <stdlib.h>

void render_clipped_block(Color color, Block surface, Block block)
{
    Block clipped = intersecting_block(surface, block);
    if (!is_empty_block(clipped))
        render_block(color, clipped);
}

void render_editor(Editor* editor)
{
    double time = time_in_us();
    for (uint16_t i = 0; i < editor->invalidated_length; i++)
    {
        Block surface = editor->invalidated[i];
        render_block((Color){ 255, 0, 0 }, surface);

        if (editor->tabs_length)
            render_tabs(editor, surface);
        else
        {
            render_clipped_block(background_colors[Font_Color_plain], surface, (Block)
            {
                .x = 0,
                .y = 0,
                .width = editor->width,
                .height = editor->height - editor->status_line_height
            });
        }

        render_status_line(editor, surface);
    }

    double time_span = time_in_us() - time;
    log("Rendered in %fus\n", time_span);

    editor->invalidated_length = 0;
}
