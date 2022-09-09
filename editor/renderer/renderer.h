#pragma once

#include "../moss.h"

typedef struct
{
    uint16_t x;
    uint16_t y;
} Font_Offset;

// Originally I had a loop for looking up the each character's
// offset when needed. It was also where most of the time was spent.
// At the cost of 9kb more memory usage character lookups where 
// reduced to font_offsets[character][color]
extern Font_Offset font_offsets[256][Font_Color_error + 1];

void render_tabs(const Editor* editor, Block surface);
void render_view(const Editor* editor, const View* view, Block surface, Block view_block);
void render_status_line(const Editor* editor, Block surface);
void render_line(const Editor* editor, const Line* line, Location offset, Location cursor, bool render_cursor, Block surface, Block line_block);
void render_clipped_block(Color color, Block surface, Block block);
bool render_clipped_string(const Editor* editor, const char* characters, uint16_t characters_length, uint16_t* columns_rendered, Font_Color color, uint16_t cursor_column, bool render_cursor_column, Block surface, Block line_block);
void render_cursor(const Editor* editor, char character_under_cursor, uint16_t x, uint16_t y);