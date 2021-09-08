#include "renderer.h"

#include <stdlib.h>

void render_editor(void)
{
    if (!editor.refresh_needed)
        return;

    // If I need more precision I can use SDL_GetPerformanceCounter(void)
    // but if I get consistent < 1ms input to screen refresh times then 
    // I should be very happy and consider if it even worth measuring 
    // more precise than 1ms. PS I know that I only measure rendering 
    // here, smartass ;)
    const uint32_t ticks = SDL_GetTicks();

    set_render_draw_color(background_colors[Font_Color_plain]);
    SDL_RenderClear(renderer);

    render_tabs();
    render_status_line();
    
    SDL_RenderPresent(renderer);
    SDL_Log("Rendered in %ums\n", SDL_GetTicks() - ticks);

    editor.refresh_needed = false;
}
