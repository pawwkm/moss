#include "renderer.h"

#include <stdlib.h>
#include <assert.h>

void render_clipped_block(Color color, Block surface, Block block)
{
    Block clipped = intersecting_block(surface, block);
    if (!is_empty_block(clipped))
        render_block(color, clipped);
}

static void render_editor_block(Editor* editor, Block surface)
{
    render_block((Color) { 255, 0, 0 }, surface);
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

static void render_editor_region(Editor* editor, Region region)
{
    if (is_scrolling_region(region))
    {
        scroll_region(region);
        
        // TODO: The 1-2 blocks that are exposed after scrolling
        // should be calculated and added invalidate_region.
        // This allows for adjacent blocks to be combined.
        if (region.scroll_x > 0 && !region.scroll_y)
            assert(false && "region.scroll_x > 0 && !region.scroll_y scrolling not implemented.");
        else if (region.scroll_x < 0 && !region.scroll_y)
            assert(false && "region.scroll_x < 0 && !region.scroll_y scrolling not implemented.");
        else if (!region.scroll_x && region.scroll_y > 0)
            assert(false && "!region.scroll_x && region.scroll_y > 0 scrolling not implemented.");
        else if (!region.scroll_x && region.scroll_y < 0)
        {
            render_editor_block(editor, (Block) 
            { 
                .x = region.block.x, 
                .y = region.block.y + region.block.height - -region.scroll_y, 
                .width = region.block.width,
                .height = -region.scroll_y
            });
        }
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
        render_editor_block(editor, region.block);
}

void render_editor(Editor* editor)
{
    double time = time_in_us();
    for (uint16_t i = 0; i < editor->invalidated_length; i++)
        render_editor_region(editor, editor->invalidated[i]);

    double time_span = time_in_us() - time;
    log("Rendered in %fus\n", time_span);

    editor->invalidated_length = 0;
}
