#include "modes.h"

#include <string.h>

void enter_command_mode(void)
{
    SDL_Log("Mode_command");
    editor.mode = Mode_command;
    editor.refresh_needed = true;
}

void exit_command_mode(void)
{
    SDL_Log("Mode_normal");
    editor.mode = Mode_normal;
    editor.refresh_needed = true;

    clear_command();
}

void insert_code_point_into_command(const uint32_t code_point)
{
    if (editor.command_length == editor.command_capacity)
    {
        editor.command_capacity = editor.command_capacity ? editor.command_capacity * 2 : 4;
        editor.command = realloc(editor.command, sizeof(editor.command[0]) * editor.command_capacity);
    }

    editor.command[editor.command_length++] = code_point;
    editor.command_cursor++;

    editor.refresh_needed = true;
}

void clear_command(void)
{
    editor.command_cursor = 0;
    editor.command_length = 0;
}

void execute_command(void)
{
    if (editor.command_length == 1 && editor.command[0] == 'w')
    {
        if (editor.tabs_length)
            flush_buffer(find_active_editor_view()->buffer);
        else
            SDL_Log("No active buffer to flush.");
    }
    else
        SDL_Log("Unknown command.");

    clear_command();
    exit_command_mode();
}

void delete_current_code_point_in_command(void)
{
    if (editor.command_cursor == editor.command_length)
        return;

    for (uint16_t i = editor.command_cursor; i < editor.command_length - 1; i++)
        editor.command[i] = editor.command[i + 1];

    editor.command_length--;
    editor.command_cursor--;

    editor.refresh_needed = true;
}

void delete_previous_code_point_in_command(void)
{
    if (!editor.command_cursor)
        return;

    for (uint16_t i = editor.command_cursor; i < editor.command_length; i++)
        editor.command[i - 1] = editor.command[i];

    editor.command_length--;
    editor.command_cursor--;

    editor.refresh_needed = true;
}
