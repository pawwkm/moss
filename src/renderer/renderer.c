#include "renderer.h"

// Previous plain text foreground colors:
//     0xFF, 0xFF, 0xFF
//     0xC2, 0xD3, 0xDE
//     0xCD, 0xD3, 0xDE
//     0xDB, 0xDB, 0xDB
//     0xE2, 0xE2, 0xE2
//     0xE8, 0xE8, 0xE8
//     0xD4, 0xE0, 0xE6
//     0xE6, 0xE6, 0xE6

#define PLAIN_TEXT_FOREGROUND_COLOR   { 0xE8, 0xF4, 0xFA, SDL_ALPHA_OPAQUE }
#define PLAIN_TEXT_BACKGROUND_COLOR   { 0x26, 0x32, 0x38, SDL_ALPHA_OPAQUE }
#define KEYWORD_TEXT_FOREGROUND_COLOR { 0x16, 0x83, 0xB9, SDL_ALPHA_OPAQUE }

SDL_Color foreground_colors[] =
{
    [Font_Color_plain] = PLAIN_TEXT_FOREGROUND_COLOR,
    [Font_Color_comment] = { 0x0B, 0xB1, 0x23, SDL_ALPHA_OPAQUE },
    [Font_Color_keyword] = KEYWORD_TEXT_FOREGROUND_COLOR,
    [Font_Color_string] = { 0xA9, 0xB7, 0xC4, SDL_ALPHA_OPAQUE },
    [Font_Color_type] = { 0x0B, 0xB1, 0xB0, SDL_ALPHA_OPAQUE },
    [Font_Color_inactive_tab_header] = PLAIN_TEXT_FOREGROUND_COLOR,
    [Font_Color_active_tab_header] = PLAIN_TEXT_FOREGROUND_COLOR,
    [Font_Color_mode] = KEYWORD_TEXT_FOREGROUND_COLOR,
    [Font_Color_selected] = PLAIN_TEXT_BACKGROUND_COLOR
};

SDL_Color background_colors[] =
{
    [Font_Color_plain] = PLAIN_TEXT_BACKGROUND_COLOR,
    [Font_Color_comment] = PLAIN_TEXT_BACKGROUND_COLOR,
    [Font_Color_keyword] = PLAIN_TEXT_BACKGROUND_COLOR,
    [Font_Color_string] = PLAIN_TEXT_BACKGROUND_COLOR,
    [Font_Color_type] = PLAIN_TEXT_BACKGROUND_COLOR,
    [Font_Color_inactive_tab_header] = { 31, 38, 42, SDL_ALPHA_OPAQUE },
    [Font_Color_active_tab_header] = PLAIN_TEXT_BACKGROUND_COLOR,
    [Font_Color_mode] = { 31, 38, 42, SDL_ALPHA_OPAQUE },
    [Font_Color_selected] = PLAIN_TEXT_FOREGROUND_COLOR,
};

void set_render_draw_color(const SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void render_vertical_line(const SDL_Color color, const int x, const int y, const int height)
{
    set_render_draw_color(color);

    // From my testing this function draws one more pixel than I 
    // expected no matter the height. -1 fixes that.
    SDL_RenderDrawLine(renderer, x, y, x, y + height - 1);
}

SDL_Renderer* renderer;
void initialize_renderer(SDL_Window* const window)
{
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_Log("Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        
        exit(1);
    }

    if (!SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1"))
    {
        SDL_Log("Could not use render batching\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    initialize_font();
}

void uninitialize_renderer(void)
{
    uninitialize_font();
    SDL_DestroyRenderer(renderer);
}
