#pragma once

#include "../moss.h"
#include "../renderer.h"
#include "../buffers.h"
#include "../tabs.h"
#include "SDL.h"

#define BASIC_MULTILINGUAL_PLANE_RANGE 0xFFFF

typedef enum
{
    Font_Color_plain,
    Font_Color_comment,
    Font_Color_keyword,
    Font_Color_string,
    Font_Color_type,
    Font_Color_inactive_tab_header,
    Font_Color_active_tab_header,
    Font_Color_mode,
    Font_Color_selected
} Font_Color;

typedef struct
{
    uint16_t x;
    uint16_t y;
} Font_Offset;

extern Font_Offset (*font_offsets)[Font_Color_selected + 1];

extern SDL_Color foreground_colors[];
extern SDL_Color background_colors[];
extern SDL_Texture* font_texture;
extern SDL_Renderer* renderer;

void initialize_font();
void uninitialize_font(void);

void render_tabs(void);
void render_view(const View* view, Rectangle view_rectangle);
void render_status_line(void);
void render_line(const Line* line, Location offset, Location cursor, bool render_cursor, Rectangle line_rectangle);
bool render_string(const uint32_t* code_points, uint16_t code_points_length, uint16_t* columns_rendered, Font_Color color, Rectangle line_rectangle);

void render_vertical_line(SDL_Color color, int x, int y, int height);

void set_render_draw_color(SDL_Color color);
