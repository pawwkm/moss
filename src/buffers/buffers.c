#include "buffers.h"

#include <stdlib.h>
#include <assert.h>

Line* add_line(Buffer* buffer)
{
    if (buffer->lines_length == buffer->lines_capacity)
    {
        buffer->lines_capacity = buffer->lines_capacity ? buffer->lines_capacity * 2 : 4;
        buffer->lines = realloc(buffer->lines, sizeof(buffer->lines[0]) * buffer->lines_capacity);

        memset(buffer->lines + buffer->lines_length, 0, sizeof(buffer->lines[0]) * (buffer->lines_capacity - buffer->lines_length));
    }

    return &buffer->lines[buffer->lines_length++];
}

Line* insert_line(Buffer* buffer, uint16_t index)
{
    if (buffer->lines_length == buffer->lines_capacity)
    {
        buffer->lines_capacity = buffer->lines_capacity ? buffer->lines_capacity * 2 : 4;
        buffer->lines = realloc(buffer->lines, sizeof(buffer->lines[0]) * buffer->lines_capacity);

        memset(buffer->lines + buffer->lines_length, 0, sizeof(buffer->lines[0]) * (buffer->lines_capacity - buffer->lines_length));
    }

    memmove(&buffer->lines[index + 1], &buffer->lines[index], sizeof(buffer->lines[0]) * (buffer->lines_length - index));
    Line* inserted = &buffer->lines[index];
    memset(inserted, 0, sizeof(inserted[0]));

    buffer->lines_length++;

    return inserted;
}

void remove_lines(Buffer* buffer, uint16_t index, uint16_t amount)
{
    for (uint16_t i = 0; i < amount; i++)
        line_free(&buffer->lines[index + i]);

    memmove(&buffer->lines[index], &buffer->lines[index + amount], sizeof(buffer->lines[0]) * (buffer->lines_length - index));
    buffer->lines_length -= amount;
}

Token* add_token(Line* line)
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

bool open_buffer(char* const path, uint16_t path_length, Buffer_Handle* handle)
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
    char* const file = read_file(path, &file_length);
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

    buffer->file_name = &path[start_of_file_index];
    buffer->file_name_length = path_length - start_of_file_index;

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
            line->characters_length = (uint16_t)(file_index - line_start_index);
            if (line->characters_length)
            {
                while (line->characters_capacity < line->characters_length)
                    line->characters_capacity = line->characters_capacity ? line->characters_capacity * 2 : 4;

                line->characters = malloc(sizeof(file[0]) * line->characters_capacity);
                memcpy(line->characters, &file[line_start_index], sizeof(line->characters[0]) * line->characters_length);
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
        line->characters_length = (uint16_t)(file_index - line_start_index);
        if (line->characters_length)
        {
            while (line->characters_capacity < line->characters_length)
                line->characters_capacity = line->characters_capacity ? line->characters_capacity * 2 : 4;

            line->characters = malloc(sizeof(file[0]) * line->characters_capacity);
            memcpy(line->characters, &file[line_start_index], line->characters_length);
        }
    }

    lexical_analyze_lines(buffer, 0, buffer->lines_length - 1);

    buffer->path = path;
    buffer->path_length = path_length;
    buffer->references = 1;
    buffer->last_flushed_change = 0;
    buffer->current_change = 0;

    free(file);

    return true;
}

void close_buffer(Buffer_Handle handle)
{
    Buffer* buffer = buffer_handle_to_pointer(handle);
    buffer->references--;

    if (!buffer->references)
    {
        free(buffer->path);
        for (uint16_t i = 0; i < buffer->lines_length; i++)
            line_free(&buffer->lines[i]);

        memset(buffer, 0, sizeof(*buffer));
        if (handle.index == buffers_length)
            buffers_length--;
    }
}

void flush_buffer(Buffer_Handle handle)
{
    const Buffer* const buffer = buffer_handle_to_pointer(handle);

    SDL_RWops* file = SDL_RWFromFile(buffer->path, "wb");
    if (!file)
    {
        SDL_Log("Could not open %s: %s\n", buffer->path, SDL_GetError());
        return;
    }

    for (uint16_t l = 0; l < buffer->lines_length; l++)
    {
        if (l)
        {
            if (SDL_RWwrite(file, &"\n", 1, 1) != 1)
                SDL_Log("Could not write \\n to %s: %s\n", buffer->path, SDL_GetError());
        }

        const Line* const line = &buffer->lines[l];
        size_t characters_written = SDL_RWwrite(file, line->characters, 1, line->characters_length);
        if (characters_written != line->characters_length)
            SDL_Log("Wrote %u characters out of %u to %s: %s\n", characters_written, line->characters_length, buffer->path, SDL_GetError());
    }

    if (SDL_RWclose(file) < 0)
        SDL_Log("Could not close %s: %s\n", buffer->path, SDL_GetError());
}

bool has_unflushed_changes(Buffer_Handle handle)
{
    const Buffer* buffer = buffer_handle_to_pointer(handle);

    return buffer->changes_length && buffer->last_flushed_change != buffer->current_change;
}

// The cursor can be placed after the last character in a line which means that
// characters are appended, not inserted. But that also means that if the editor
// tries to read what is under the cursor it may overrun the line buffer by 1
//
// That is not really communicated well by using -1 in some places and not in 
// others.
uint16_t index_of_last_character(const Line* line)
{
    assert(line->characters_length);

    return line->characters_length - 1;
}

uint16_t index_of_character_append(const Line* line)
{
    return line->characters_length;
}

