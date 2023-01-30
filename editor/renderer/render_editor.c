#include "renderer.h"

#include <stdlib.h>
#include <assert.h>

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
        Region region = editor->invalidated[i];
        Block surface;

        if (region.scroll_x || region.scroll_y)
        {
            scroll_region(region);
            if (region.scroll_x > 0 && !region.scroll_y)
                assert(false && "region.scroll_x > 0 && !region.scroll_y scrolling not implemented.");
            else if (region.scroll_x < 0 && !region.scroll_y)
                assert(false && "region.scroll_x < 0 && !region.scroll_y scrolling not implemented.");
            else if (!region.scroll_x && region.scroll_y > 0)
                assert(false && "!region.scroll_x && region.scroll_y > 0 scrolling not implemented.");
            else if (!region.scroll_x && region.scroll_y < 0)
                surface = (Block){ .x = region.block.x, .y = region.block.y + region.block.height - -region.scroll_y, .width = region.block.width, .height = -region.scroll_y};
            else if (region.scroll_x < 0 && region.scroll_y < 0)
                assert(false && "region.scroll_x < 0 && region.scroll_y < 0 scrolling not implemented.");
            else if (region.scroll_x > 0 && region.scroll_y < 0)
                assert(false && "region.scroll_x > 0 && region.scroll_y < 0 scrolling not implemented.");
            else if (region.scroll_x > 0 && region.scroll_y > 0)
                assert(false && "region.scroll_x > 0 && region.scroll_y > 0 scrolling not implemented.");
            else if (region.scroll_x < 0 && region.scroll_y > 0)
                assert(false && "region.scroll_x < 0 && region.scroll_y > 0 scrolling not implemented.");
        }
        else
            surface = region.block;
        
        //render_block((Color) { 255, 0, 0 }, surface);
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
