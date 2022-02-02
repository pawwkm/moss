#include "moss.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

Editor editor =
{
    .mode           = Mode_normal,
    .width          = FONT_WIDTH * 80,
    .height         = FONT_HEIGHT * 24,
    .refresh_needed = true
};

static bool close_requested;
static void handle_event(const SDL_Event* const event)
{
    static bool ctrl = false;
    switch (event->type)
    {
        case SDL_WINDOWEVENT:
            switch (event->window.event)
            {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    editor.width  = (uint16_t)event->window.data1;
                    editor.height = (uint16_t)event->window.data2;
                    measure_tabs();
                    break;

                case SDL_WINDOWEVENT_EXPOSED:
                case SDL_WINDOWEVENT_RESTORED:
                    editor.refresh_needed = true;
                    break;

                case SDL_WINDOWEVENT_CLOSE:
                    close_requested = true;
                    break;
            }

            break;

        case SDL_TEXTINPUT:
        {
            // Using SDL_TEXTINPUT just gives me the right character.
            // I don't have to deal with shift + lower case characters
            // or scan codes. Very handy for keyboards like mine with QMK
            // firmware that fires multiple codes at once to get 'å'.
            char c = event->text.text[0];
            if (c < 0 || c > '\x7F')
                break;

            interpret_character(c, ctrl);
            if (SDL_ShowCursor(SDL_DISABLE) == -1)
                SDL_Log("Could not disable cursor: %s\n", SDL_GetError());

            break;
        }

        case SDL_KEYDOWN:
        {
            ctrl = event->key.keysym.mod & KMOD_CTRL;
            char c = (char)event->key.keysym.sym;
            if (c > -1 && c < ' ' || c == '\x7F')
                interpret_character(c, ctrl);

            if (SDL_ShowCursor(SDL_DISABLE) == -1)
                SDL_Log("Could not disable cursor: %s\n", SDL_GetError());

            break;
        }
        
        case SDL_MOUSEMOTION:
            if (SDL_ShowCursor(SDL_ENABLE) == -1)
                SDL_Log("Could not enable cursor: %s\n", SDL_GetError());

            break;
    }
}

static SDL_Window* window;
void set_editor_title(char* const path, size_t path_length)
{
    static char title[512] = "Moss - ";
    memcpy(&title[7], path, path_length);
    title[7 + path_length] = '\0';

    SDL_SetWindowTitle(window, title);
}

static void initialize_window(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("SDL initialization error: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("Moss", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, editor.width, editor.height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    SDL_SetWindowMinimumSize(window, editor.width, editor.height);
}

static void uninitialize_window(void)
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void show_message(const char* message)
{
    // TODO display this on the status line.
    SDL_Log(message);
}

void show_initialization_error_message(const char* format, ...)
{
    char message[1000];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not initialize Moss!", message, NULL);
}

int main(int argc, char *argv[])
{
    initialize_window();
    initialize_renderer(window);

    for (int i = 1; i < argc; i++)
    {
        const size_t path_length = strlen(argv[i]);
        if (path_length > UINT16_MAX)
        {
            show_message("Path to long");
            continue;
        }

        char* const path = malloc(path_length + 1);

        strcpy(path, argv[i]);
        open_buffer_in_active_tab(path, (uint16_t)path_length);
    }

    if (!SDL_IsTextInputActive())
        SDL_StartTextInput();

    render_editor();
    while (!close_requested)
    {
        SDL_Event event;
        if (SDL_WaitEvent(&event))
        {
            handle_event(&event);
            while (SDL_PollEvent(&event))
                handle_event(&event);
        }
        else
            SDL_Log("SDL_WaitEvent failed: %s\n", SDL_GetError());

        render_editor();
    }

    uninitialize_renderer();
    uninitialize_window();
    
    return 0;
}
