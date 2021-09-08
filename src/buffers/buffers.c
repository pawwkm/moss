#include "buffers.h"
#include "SDL.h"

#include <stdlib.h>

Line* add_line(Buffer* const buffer)
{
    if (buffer->lines_length == buffer->lines_capacity)
    {
        buffer->lines_capacity = buffer->lines_capacity ? buffer->lines_capacity * 2 : 4;
        buffer->lines = realloc(buffer->lines, sizeof(buffer->lines[0]) * buffer->lines_capacity);

        memset(buffer->lines + buffer->lines_length, 0, sizeof(buffer->lines[0]) * (buffer->lines_capacity - buffer->lines_length));
    }

    return &buffer->lines[buffer->lines_length++];
}

Token* add_token(Line* const line)
{
    if (line->tokens_length == line->tokens_capacity)
    {
        line->tokens_capacity = line->tokens_capacity ? line->tokens_capacity * 2 : 4;
        line->tokens = realloc(line->tokens, sizeof(line->tokens[0]) * line->tokens_capacity);

        memset(line->tokens + line->tokens_length, 0, sizeof(line->tokens[0]) * (line->tokens_capacity - line->tokens_length));
    }

    return &line->tokens[line->tokens_length++];
}

static Buffer* buffers;
static uint8_t buffers_length;
static uint8_t buffers_capacity;

Buffer* buffer_handle_to_pointer(Buffer_Handle handle)
{
    return &buffers[handle.index];
}

bool open_buffer(char* const path, const uint16_t path_length, Buffer_Handle* const handle)
{
    // Check if the utf8 is already open.
    for (uint8_t i = 0; i < buffers_length; i++)
    {
        if (buffers[i].path_length == path_length && !strncmp(buffers[i].path, path, path_length))
        {
            free(path);
            buffers[i].references++;
            handle->index = i;

            return true;
        }
    }

    // Check if there is an empty slot.
    Buffer* buffer = NULL;
    for (uint8_t i = 0; i < buffers_length; i++)
    {
        if (!buffers[i].references)
        {
            handle->index = i;
            buffer = &buffers[i];

            break;
        }
    }

    size_t file_length;
    uint32_t* const file = read_file(path, &file_length);
    if (!file)
        return false;

    // Expand buffers since there aren't any free slots.
    if (!buffer)
    {
        buffers_capacity = buffers_capacity ? buffers_capacity * 2 : 4;
        buffers = realloc(buffers, sizeof(buffers[0]) * buffers_capacity);

        memset(buffers + buffers_length, 0, sizeof(buffers[0]) * (buffers_capacity - buffers_length));

        handle->index = buffers_length;
        buffer = &buffers[buffers_length++];
    }

    uint16_t start_of_file_index = 0;
    for (uint16_t i = path_length - 1; i > 0; i--)
    {
        if (path[i] == '\\' || path[i] == '/')
        {
            start_of_file_index = i + 1;
            break;
        }
    }

    uint16_t start_of_extension = 0;
    for (uint16_t i = path_length - 1; i > 0; i--)
    {
        if (path[i] == '.')
        {
            start_of_extension = i + 1;
            break;
        }
    }

    size_t file_name_length = 0;
    buffer->file_name = utf8_string_to_code_points((uint8_t*)&path[start_of_file_index], path_length - start_of_file_index, &file_name_length);

    buffer->file_name_length = (uint16_t)file_name_length;

    switch (path_length - start_of_extension)
    {
        case 1:
            if (path[start_of_extension] == 'c' || path[start_of_extension] == 'h')
                buffer->language = Language_c;
            
            break;
        case 4:
            if (strcmp("owen", &path[start_of_extension]) == 0)
                buffer->language = Language_owen;

            break;
        default:
            break;
    }


    // TODO: Error handle files with too long lines and too many lines.
    Line* line = add_line(buffer);
    size_t file_index = 0;
    size_t line_start_index = 0;
    while (file_index != file_length)
    {
        if (file_index + 2 <= file_length && file[file_index] == '\r' && file[file_index + 1] == '\n' || file[file_index] == '\n')
        {
            line->code_points_length = (uint16_t)(file_index - line_start_index);
            if (line->code_points_length)
            {
                while (line->code_points_capacity < line->code_points_length)
                    line->code_points_capacity = line->code_points_capacity ? line->code_points_capacity * 2 : 4;

                line->code_points = malloc(sizeof(file[0]) * line->code_points_capacity);
                memcpy(line->code_points, &file[line_start_index], sizeof(line->code_points[0]) * line->code_points_length);
            }

            if (file[file_index] == '\r')
                file_index += 2;
            else
                file_index++;

            line_start_index = file_index;
            line = add_line(buffer);
        }
        else
            file_index++;
    }

    if (file_index != line_start_index)
    {
        line->code_points_length = (uint16_t)(file_index - line_start_index);
        if (line->code_points_length)
        {
            while (line->code_points_capacity < line->code_points_length)
                line->code_points_capacity = line->code_points_capacity ? line->code_points_capacity * 2 : 4;

            line->code_points = malloc(sizeof(file[0]) * line->code_points_capacity);
            memcpy(line->code_points, &file[line_start_index], sizeof(line->code_points[0]) * line->code_points_length);
        }
    }

    bool continue_multiline_comment = false;
    for (uint16_t i = 0; i < buffer->lines_length; i++)
        lexical_analyze(buffer->language, &buffer->lines[i], &continue_multiline_comment);

    buffer->path = path;
    buffer->path_length = path_length;
    buffer->references = 1;
    buffer->last_flushed_change = -1;
    buffer->current_change = -1;

    free(file);

    return true;
}

void close_buffer(Buffer_Handle handle)
{
    Buffer* const buffer = buffer_handle_to_pointer(handle);
    buffer->references--;

    if (!buffer->references)
    {
        free(buffer->path);
        for (uint16_t i = 0; i < buffer->lines_length; i++)
        {
            Line* const line = &buffer->lines[i];
            free(line->tokens);
            free(line->code_points);
        }

        memset(buffer, 0, sizeof(*buffer));
        if (handle.index == buffers_length)
            buffers_length--;
    }
}

void flush_buffer(Buffer_Handle handle)
{
    const Buffer* const buffer = buffer_handle_to_pointer(handle);
    SDL_RWops* file = SDL_RWFromFile(buffer->path, "wb");

    for (uint16_t l = 0; l < buffer->lines_length; l++)
    {
        if (l)
            SDL_RWwrite(file, &(char){'\n'}, 1, 1);

        const Line* const line = &buffer->lines[l];
        for (uint16_t c = 0; c < line->code_points_length; c++)
        {
            char utf8[4] = { 0 };
            uint8_t sequence_length;

            code_point_to_utf8(line->code_points[c], &utf8, &sequence_length);

            SDL_RWwrite(file, utf8, 1, sequence_length);
        }
    }

    SDL_RWclose(file);
}

bool has_unflushed_changes(Buffer_Handle handle)
{
    const Buffer* buffer = buffer_handle_to_pointer(handle);

    return buffer->last_flushed_change != buffer->current_change;
}
