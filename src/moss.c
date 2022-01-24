#include "moss.h"
#include "buffers.h"
#include "modes.h"
#include "renderer.h"
#include "tabs.h"
#include "SDL.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Editor editor =
{
    .mode = Mode_normal,
    .width = FONT_WIDTH * 80,
    .height = FONT_HEIGHT * 24,
    .refresh_needed = true
};

static bool close_requested;

static void handle_window_event(const SDL_WindowEvent* const window)
{
    switch (window->event)
    {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            editor.width = (uint16_t)window->data1;
            editor.height = (uint16_t)window->data2;
            measure_tabs();
            break;

        case SDL_WINDOWEVENT_EXPOSED:
        case SDL_WINDOWEVENT_RESTORED:
            editor.refresh_needed = true;
            break;

        case SDL_WINDOWEVENT_CLOSE:
            close_requested = true;
            break;

        default:
            break;
    }
}

static void handle_event(const SDL_Event* const event)
{
    switch (event->type)
    {
        case SDL_WINDOWEVENT:
            handle_window_event(&event->window);
            break;

        case SDL_KEYDOWN:
            key_down_in_current_mode(&event->key);
            SDL_ShowCursor(SDL_DISABLE);
            break;

        case SDL_TEXTINPUT:
            input_character_in_current_mode(event->text.text[0]);
            SDL_ShowCursor(SDL_DISABLE);
            break;

        case SDL_MOUSEMOTION:
            SDL_ShowCursor(SDL_ENABLE);
            break;

        default:
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

// TODO: Make it printf style so I can append SDL_GetError().
void show_initialization_error_message(const char* const message)
{
    // The nice thing about using SDL_ShowMessageBox is that
    // it works even before SDL_Init is called. Perfect if 
    // the renderer or something can't be created.

    const SDL_MessageBoxButtonData buttons[] = 
    {
        { 0, 0, "Ok" },
    };

    const SDL_MessageBoxData messageboxdata = 
    {
        .flags = SDL_MESSAGEBOX_ERROR,
        .window = NULL,
        .title = "Could not initialize Moss!",
        .message = message,
        .numbuttons = 1,
        .buttons = buttons,
        .colorScheme = NULL
    };

    SDL_ShowMessageBox(&messageboxdata, &(int) { 0 });
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

        render_editor();
    }

    uninitialize_renderer();
    uninitialize_window();
    
    return 0;
}
