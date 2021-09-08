#include "modes.h"

void enter_insert_mode(void)
{
    if (!editor.tabs_length)
        return;

    SDL_Log("Mode_insert");
    editor.mode = Mode_insert;
    editor.refresh_needed = true;
}

void exit_insert_mode(void)
{
    SDL_Log("Mode_normal");
    editor.mode = Mode_normal;
    editor.refresh_needed = true;
}
