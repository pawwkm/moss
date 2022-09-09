#include "renderer.h"

// The cursor is expected to be completely visible at all times.
// There is line padding vertically and centering horizontally.
// The only cases where this is not true is when the there isn't
// enough height in the editor for tabs, status line and a single
// character. For width its similar. Either have an insane amount
// of tabs or have a really slim window. At that point who cares?
void render_cursor(const Editor* editor, char character_under_cursor, uint16_t x, uint16_t y)
{
    if (is_tab_or_space(character_under_cursor))
    {
        render_block(background_colors[Font_Color_selected], (Block)
        {
            .x = x,
            .y = y,
            .width = editor->font_width,
            .height = editor->font_height
        });
    }
    else
    {
        render_character
        (
            font_offsets[character_under_cursor][Font_Color_selected].x,
            font_offsets[character_under_cursor][Font_Color_selected].y,
            x,
            y,
            editor->font_width,
            editor->font_height
        );
    }
}