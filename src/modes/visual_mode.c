#include "modes.h"

void enter_visual_mode(void)
{
    if (!editor.tabs_length)
        return;

    SDL_Log("Mode_visual");
    editor.mode = Mode_visual;
    editor.refresh_needed = true;
}

void exit_visual_mode(void)
{
    SDL_Log("Mode_normal");
    editor.mode = Mode_normal;
    editor.refresh_needed = true;
}
