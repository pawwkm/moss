#include "renderer.h"

static void render_tab_headers(const Editor* editor, const Tab* const tab, Block surface, Block tab_block)
{
    // The > character is used to mark "here starts a tab header"
    // and it pulls double duty as the indicator of unsaved changes
    // by changing its color. Windows doesn'i allow > in file names
    // and from a quick search Linux supports it but it is discouraged.
    // So while not impossible it is very unlikely that I will run
    // into that situation. A nice bonus of always rendering the >
    // means that the width of the header won't "flicker" when 
    // changing state.

    // TODO Headers must fit within the tab_width somehow.
    // Visual Studio uses a down facing arrow for a drop 
    // down menu when there isn't enough space to display
    // all tab headers.
    //
    // I could use <> to communicate that there are more 
    // tabs to the left and/or right that aren't visible.

    uint16_t x_offset = tab_block.x;
    for (uint16_t v = 0; v < tab->views_length; v++)
    {
        const View* view = &tab->views[v];
        const Buffer* buffer = lookup_buffer(view->buffer);

        Block header_line;
        header_line.y = (editor->tab_header_height - editor->font_height) / 2;
        header_line.height = editor->font_height;
        // +1 for the >.
        header_line.width = (buffer->file_name_length + 1) * editor->font_width;

        Block header_background;
        header_background.x = x_offset;
        header_background.y = 0;
        header_background.height = editor->tab_header_height;

        // The + 2 is for the padding on the left and right.
        header_background.width = header_line.width + 2 * editor->font_width;

        header_line.x = (uint16_t)header_background.x + editor->font_width;

        Font_Color color = find_active_tab_view(tab) == view ? Font_Color_active_tab_header : Font_Color_inactive_tab_header;
        render_clipped_block(background_colors[color], surface, header_background);

        if (view == find_active_editor_view(editor))
        {
            // Highlight the editors active view with a line down the 
            // left hand side of the tab header.
            render_clipped_block(foreground_colors[Font_Color_keyword], surface, (Block)
            {
                 .x = header_background.x,
                 .y = header_background.y,
                 .width = 1,
                 .height = header_background.height
            });
        }

        Font_Color angle_bracket_color;
        if (has_unflushed_changes(view->buffer))
            angle_bracket_color = find_active_tab_view(tab) == view ? Font_Color_keyword : Font_Color_mode;
        else
            angle_bracket_color = color;

        uint16_t columns_rendered = 0;
        render_clipped_string(editor, ">", 1, &columns_rendered, angle_bracket_color, 0, false, surface, header_line);
        render_clipped_string(editor, buffer->file_name, buffer->file_name_length, &columns_rendered, color, 0, false, surface, header_line);

        x_offset += header_background.width;
    }

    // Draw the remainder of the header in this tab.
    // TODO should this have the same background color as the last header?
    // If there is enough headers then the remainder is not all the large
    // and I think it looks a bit out of place when the last view is the 
    // active one.
    render_clipped_block(background_colors[Font_Color_inactive_tab_header], surface, (Block)
    {
        .x = x_offset,
        .y = 0,
        .width = tab_block.width - (x_offset - tab_block.x),
        .height = editor->tab_header_height
    });
}

void render_tabs(const Editor* editor, Block surface)
{
    for (uint8_t i = 0; i < editor->tabs_length; i++)
    {
        const Tab* const tab = &editor->tabs[i];
        render_tab_headers(editor, tab, surface, (Block)
        {
            .x = tab->block.x,
            .y = 0,
            .width = tab->block.width,
            .height = editor->tab_header_height
        });

        render_view(editor, find_active_tab_view(tab), surface, (Block)
        {
            .x = tab->block.x,
            .y = editor->tab_header_height,
            .width = tab->block.width,
            .height = tab->block.height - editor->tab_header_height
        });

        // Render the tab separator
        if (i + 1 < editor->tabs_length)
        {
            render_clipped_block(background_colors[Font_Color_inactive_tab_header], surface, (Block)
            {
                .x = tab->block.x + tab->block.width,
                .y = 0,
                .width = editor->tab_separator_width,
                .height = editor->height - editor->status_line_height
            });
        }
    }
}
