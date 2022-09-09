#include "buffers.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

Line* insert_line(Buffer* buffer, uint16_t index)
{
    Line line = { 0 };
    INSERT_ELEMENTS(buffer->lines, index, 1, &line);

    return &buffer->lines[index];
}

void remove_lines(Buffer* buffer, uint16_t index, uint16_t amount)
{
    REMOVE_ELEMENTS_WITH_DEALLOCATOR(buffer->lines, index, amount, line_free);
}

Token* add_token(Line* line)
{
    RESERVE_ELEMENTS(line->tokens, line->tokens_length + 1);
    return &line->tokens[line->tokens_length++];
}

static Buffer* buffers;
static uint8_t buffers_length;
static uint8_t buffers_capacity;

Buffer* lookup_buffer(Buffer_Handle handle)
{
    return &buffers[handle.index];
}

static Language language_from_extension(char* path, size_t length)
{
    switch (length)
    {
        case 1:
            if (*path == 'c' || *path == 'h')
                return Language_c17;
            
            break;
        case 4:
            if (strcmp("owen", path) == 0)
                return Language_owen;

            break;
    }

    return Language_none;
}

static void read_lines(char* file, size_t file_length, Buffer* buffer)
{
    // TODO: Error handle files with too long lines and too many lines.
    size_t file_index = 0;
    do
    {
        size_t characters_index = file_index;
        size_t characters_length = 0;

        while (file_index != file_length)
        {
            if (file[file_index] == '\n')
            {
                file_index++;
                break;
            }
            else if (file_index + 2 <= file_length && file[file_index] == '\r' && file[file_index + 1] == '\n')
            {
                file_index += 2;
                break;
            }
            else
            {
                file_index++;
                characters_length++;
            }
        }

        RESERVE_ELEMENTS(buffer->lines, buffer->lines_length + 1);
        Line* line = &buffer->lines[buffer->lines_length++];

        if (characters_length)
            INSERT_ELEMENTS(line->characters, 0, (uint16_t)characters_length, &file[characters_index]);

    } while (file_index != file_length);
}

bool open_buffer(char* path, uint16_t path_length, Buffer_Handle* handle)
{
    // Check if the buffer is already open.
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

    size_t file_length;
    char* file = read_file(path, &file_length);
    if (!file)
        return false;

    // Buffers are looked up through handles to slots
    // which means they cannot be moved. Than also means
    // we use the usual macros for memory management.
    // So check if there is an empty slot.
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
    
    if (!buffer)
    {
        // Expand buffers since there aren't any free slots.
        RESERVE_ELEMENTS(buffers, buffers_length + 1);
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
    buffer->language = language_from_extension(&path[start_of_extension], path_length - start_of_extension);

    read_lines(file, file_length, buffer);
    free(file);
    lexical_analyze_lines(buffer, 0, buffer->lines_length - 1);

    buffer->path = path;
    buffer->path_length = path_length;
    buffer->references = 1;
    buffer->last_flushed_change = 0;
    buffer->current_change = 0;

    return true;
}

void close_buffer(Buffer_Handle handle)
{
    Buffer* buffer = lookup_buffer(handle);
    buffer->references--;

    if (!buffer->references)
    {
        free(buffer->path);
        for (uint16_t i = 0; i < buffer->lines_length; i++)
            line_free(&buffer->lines[i]);

        *buffer = (Buffer){ 0 };
    }
}

void flush_buffer(Buffer_Handle handle)
{
    Buffer* buffer = lookup_buffer(handle);

    FILE* file = fopen(buffer->path, "wb");
    if (!file)
    {
        log("Could not open %s: %s\n", buffer->path, strerror(errno));
        return;
    }

    for (uint16_t l = 0; l < buffer->lines_length; l++)
    {
        if (l && fputc('\n', file) != 1)
            log("Could not write \\n to %s: %s\n", buffer->path, strerror(errno));

        //Line* line = &buffer->lines[l];
        //size_t characters_written = fwrite(file, line->characters, 1, line->characters_length);
        //if (characters_written != line->characters_length)
        //    log("Wrote %u characters out of %u to %s: %s\n", characters_written, line->characters_length, buffer->path, strerror(errno));
    }

    if (fclose(file) < 0)
        log("Could not close %s: %s\n", buffer->path, strerror(errno));
}

bool has_unflushed_changes(Buffer_Handle handle)
{
    Buffer* buffer = lookup_buffer(handle);

    return buffer->changes_length && buffer->last_flushed_change != buffer->current_change;
}