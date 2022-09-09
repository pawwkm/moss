#include "moss.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void invalidate_block(Editor* editor, Block block)
{
    for (uint8_t i = 0; i < editor->invalidated_length; i++)
    {
        Block b = editor->invalidated[i];
        if (contains_block(b, block))
            return;

        if (contains_block(block, b))
        {
            if (i + 1 == editor->invalidated_length)
                editor->invalidated_length--;
            else
                REMOVE_ELEMENT_UNORDERED(editor->invalidated, i);
        }
    }

    ADD_ELEMENT(editor->invalidated, block);
}

void invalidate_location(Editor* editor)
{
    // The widest location is 65536:65536.
    uint16_t widest_location = 11 * editor->font_width;
    invalidate_block(editor, (Block)
    {
        .x = editor->width - widest_location,
        .y = editor->height - editor->status_line_height,
        .width = widest_location,
        .height = editor->status_line_height
    });
}

Block tab_to_view_block(const Editor* editor, Block tab_view)
{
    return (Block)
    {
        .x = tab_view.x,
        .y = tab_view.y + editor->tab_header_height,
        .width = tab_view.width,
        .height = tab_view.height - editor->tab_header_height
    };
}

void resize_editor(Editor* editor, uint16_t width, uint16_t height)
{
    if (editor->height > height && editor->width == width)
    {
        // The status line is moved up so we can
        // get away we redrawing just that.
        invalidate_block(editor, (Block)
        {
            .x = 0,
            .y = height - editor->status_line_height,
            .width = width,
            .height = editor->status_line_height
        });
    }
    else if (editor->height < height && editor->width == width)
    {
        // The status line is moved down which exposes more of the tabs vertically. 
        // We have to draw the the newly exposed part then redraw the
        // status line below that.
        Block block;
        block.x = 0;
        block.width = width;
        block.height = height - editor->height + editor->status_line_height;
        block.y = height - block.height;

        invalidate_block(editor, block);
    }
    else if (editor->width > width && editor->height == height)
    {
        // Tab headers.
        invalidate_block(editor, (Block)
        {
            .x = 0,
            .y = 0,
            .width = width,
            .height = editor->tab_header_height
        });
        
        // Status line.
        invalidate_block(editor, (Block)
        {
            .x = 0,
            .y = height - editor->status_line_height,
            .width = width,
            .height = editor->status_line_height
        });

        // If there are only 1 tab we don't need to do any more since the text is hidden!
        // But if there are multiple taps then we have to redraw all content after the first
        // tab because tabs are the same width.
        if (editor->tabs_length > 1)
        {
            Block tab_block = editor->tabs[0].block;
            invalidate_block(editor, (Block)
            {
                .x = tab_block.x + tab_block.width,
                .y = editor->tab_header_height,
                .width = width - (tab_block.x + tab_block.width),
                .height = tab_block.height
            });
        }
    }
    else if (editor->width < width && editor->height == height && editor->tabs_length == 1)
    {
        // We don't have to deal with any separators so we can safely say
        // that only the newly exposed tab header, view and location space
        // needs to be redrawn.

        // Tab header and view.
        invalidate_block(editor, (Block)
        {
            .x = editor->width,
            .y = 0,
            .width = width - editor->width,
            .height = height - editor->status_line_height
        });

        // BUG: If width - editor->width > location_width the some or all
        // of the previously rendered location is still visible after rendering
        // the new one. To fix this check if the added width is less than the 
        // width of current location. If so invalidate the current location plus
        // the added width as a separate block. But if not, extend the above block
        // to cover the entire height of the editor.
        editor->width = width;
        editor->height = height;
        invalidate_location(editor);
    }
    else if (editor->width > width && editor->height > height && editor->tabs_length == 1)
    {
        // Status line.
        invalidate_block(editor, (Block)
        {
            .x = 0,
            .y = height - editor->status_line_height,
            .width = width,
            .height = editor->status_line_height
        });
    }
    else
    {
        invalidate_block(editor, (Block)
        {
            .x = 0,
            .y = 0,
            .width = width,
            .height = height
        });
    }

    editor->width = width;
    editor->height = height;
    // TODO: Detect needed movement when resizing tabs.
    measure_tabs(editor);
}

Editor initialize_editor(void)
{
    Editor editor = { 0 };
    editor.font_width = cozette.width;
    editor.font_height = cozette.height;
    editor.spaces_per_tab = 4;
    editor.tab_separator_width = editor.font_width;
    editor.tab_header_height = 2 * editor.font_height;
    editor.status_line_height = editor.font_height;
    editor.line_scroll_padding = 5;
    editor.width = editor.font_width * 80;
    editor.height = editor.font_height * 24;
    
    return editor;
}

void uninitialize_editor(Editor* editor)
{
    for (uint8_t a = 0; a < editor->tabs_length; a++)
    {
        Tab* tab = &editor->tabs[a];
        for (uint8_t b = 0; b < tab->views_length; b++)
            close_buffer(tab->views[b].buffer);

        free(tab->views);
    }

    free(editor->tabs);
    free(editor->command);
    free(editor->invalidated);

    *editor = (Editor){ 0 };
}