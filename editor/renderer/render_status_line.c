#include "renderer.h"
#include <assert.h>

static uint16_t uint16_t_to_characters(uint16_t value, char* characters)
{
    assert(value);

    uint16_t characters_used = 0;
    while (value)
    {
        uint16_t remainder = value % 10;
        characters[characters_used] = (char)(remainder + '0');
        characters_used++;

        value /= 10;
    }

    // Reverse the order of the characters.
    for (uint16_t x = 0, y = characters_used - 1; x < y; x++, y--)
    {
        char temp = characters[x];
        characters[x] = characters[y];
        characters[y] = temp;
    }

    return characters_used;
}

static uint16_t render_active_view_location(const Editor* editor, Block surface)
{
    if (!editor->tabs_length)
        return 0;

    View* view = find_active_editor_view(editor);

    // Since Location uses uint16_t for lines a columns I know 
    // that the largest number that I have to render is 65535
    // which means that I only need 5 characters per number.
    char text[11];
    uint16_t text_length = uint16_t_to_characters(view->offset.line + view->cursor.line + 1, (char*)&text);
    text[text_length++] = ':';
    text_length += uint16_t_to_characters(view->offset.column + view->cursor.column + 1, (char*)&text[text_length]);
    
    uint16_t columns_rendered = 0;
    render_clipped_string(editor, (char*)&text, text_length, &columns_rendered, Font_Color_inactive_tab_header, 0, false, surface, (Block)
    {
        .x = editor->width - text_length * editor->font_width,
        .y = editor->height - editor->status_line_height,
        .width = text_length * editor->font_width,
        .height = editor->status_line_height
    });

    return columns_rendered;
}

static uint16_t render_mode(const Editor* editor, Block surface)
{
    char* text;
    uint16_t text_length;
    
    switch (editor->mode)
    {
        case Mode_normal:
            text = "NORMAL";
            text_length = 6;
            break;

        case Mode_insert:
            text = "INSERT";
            text_length = 6;
            break;

        case Mode_visual:
            text = "VISUAL";
            text_length = 6;
            break;

        case Mode_tab:
            text = "TAB";
            text_length = 3;
            break;
    }
    
    render_clipped_string(editor, text, text_length, &(uint16_t){ 0 }, Font_Color_mode, 0, false, surface, (Block)
    {
        .x = 0,
        .y = editor->height - editor->status_line_height,
        .height = editor->status_line_height,
        .width = editor->font_width * text_length
    });

    return text_length;
}

static void render_command_line(const Editor* editor, uint16_t mode_length, uint16_t location_length, Block surface)
{
    Block command_line =
    {
        .x = mode_length * editor->font_width,
        .y = editor->height - editor->status_line_height,
        .width = editor->width - (mode_length + location_length) * editor->font_width,
        .height = editor->status_line_height
    };

    if (editor->command_length)
    {
        Block gap =
        {
            .x = command_line.x,
            .y = command_line.y,
            .width = editor->font_width,
            .height = editor->font_height
        };

        render_clipped_block(background_colors[Font_Color_inactive_tab_header], surface, gap);

        Block command = intersecting_block(command_line, (Block)
        {
            .x = gap.x + gap.width,
            .y = command_line.y,
            .width = editor->command_length * editor->font_width,
            .height = command_line.height
        });

        // TODO: Truncate before the location.
        render_clipped_string(editor, editor->command, editor->command_length, &(uint16_t) { 0 }, Font_Color_inactive_tab_header, 0, false, surface, command);
        Block remainder =
        {
            .x = command.x + command.width,
            .y = command.y,
            .width = command_line.width - command.width - gap.width,
            .height = command.height
        };

        render_clipped_block(background_colors[Font_Color_inactive_tab_header], surface, remainder);
    }
    else
        render_clipped_block(background_colors[Font_Color_inactive_tab_header], surface, command_line);
}

void render_status_line(const Editor* editor, Block surface)
{
    uint16_t mode_length = render_mode(editor, surface);
    uint16_t location_length = render_active_view_location(editor, surface);

    render_command_line(editor, mode_length, location_length, surface);
}