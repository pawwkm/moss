#include "renderer.h"

static void render_tab_headers(const Tab* const tab, const Rectangle tab_rectangle)
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

    uint16_t x_offset = tab_rectangle.position.x;
    for (uint16_t v = 0; v < tab->views_length; v++)
    {
        const View* const view = &tab->views[v];
        const Buffer* const buffer = buffer_handle_to_pointer(view->buffer);

        Rectangle header_line;
        header_line.position.y = (TAB_HEADER_HEIGHT - FONT_HEIGHT) / 2;
        header_line.height = FONT_HEIGHT;
        header_line.width = buffer->file_name_length * FONT_WIDTH;

        SDL_Rect header_background;
        header_background.x = x_offset;
        header_background.y = 0;
        header_background.h = TAB_HEADER_HEIGHT;

        // The + 3 is for the padding on the left, the right
        // and for the >.
        header_background.w = header_line.width + 3 * FONT_WIDTH;

        header_line.position.x = (uint16_t)header_background.x + FONT_WIDTH;

        const Font_Color color = find_active_tab_view(tab) == view ? Font_Color_active_tab_header : Font_Color_inactive_tab_header;
        set_render_draw_color(background_colors[color]);
        SDL_RenderFillRect(renderer, &header_background);

        if (view == find_active_editor_view())
        {
            // Highlight the editors active view with a line down the 
            // left hand side of the tab header.
            render_vertical_line(foreground_colors[Font_Color_keyword],
                                 header_background.x,
                                 header_background.y,
                                 header_background.h);
        }

        Font_Color angle_bracket_color;
        if (has_unflushed_changes(view->buffer))
            angle_bracket_color = find_active_tab_view(tab) == view ? Font_Color_keyword : Font_Color_mode;
        else
            angle_bracket_color = color;

        uint16_t columns_rendered = 0;
        render_string(">", 1, & columns_rendered, angle_bracket_color, header_line);
        render_string(buffer->file_name, buffer->file_name_length, &columns_rendered, color, header_line);

        x_offset += (uint16_t)header_background.w;
    }

    // Draw the remainder of the header in this tab.
    // TODO should this have the same background color as the last header?
    // If there is enough headers then the remainder is not all the large
    // and I think it looks a bit out of place when the last view is the 
    // active one.
    set_render_draw_color(background_colors[Font_Color_inactive_tab_header]);
    SDL_RenderFillRect(renderer, &(SDL_Rect)
    {
        .x = x_offset,
        .y = 0,
        .w = tab_rectangle.width - (x_offset - tab_rectangle.position.x),
        .h = TAB_HEADER_HEIGHT
    });
}

void render_tabs(void)
{
    for (uint8_t i = 0; i < editor.tabs_length; i++)
    {
        const Tab* const tab = &editor.tabs[i];
        render_tab_headers(tab, (Rectangle)
        {
            .position.x = tab->rectangle.position.x,
            .position.y = 0,
            .width = tab->rectangle.width,
            .height = TAB_HEADER_HEIGHT
        });

        render_view(find_active_tab_view(tab), (Rectangle)
        {
            .position.x = tab->rectangle.position.x,
            .position.y = TAB_HEADER_HEIGHT,
            .width = tab->rectangle.width,
            .height = tab->rectangle.height - TAB_HEADER_HEIGHT
        });

        // Render the tab separator
        if (i + 1 < editor.tabs_length)
        {
            set_render_draw_color(background_colors[Font_Color_inactive_tab_header]);
            SDL_RenderFillRect(renderer, &(SDL_Rect)
            {
                .x = tab->rectangle.position.x + tab->rectangle.width,
                .y = 0,
                .w = TAB_SEPARATOR_WIDTH,
                .h = editor.height - STATUS_LINE_HEIGHT
            });
        }
    }
}
