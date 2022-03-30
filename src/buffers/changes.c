#include "buffers.h"
#include <assert.h>

void add_change(Buffer* buffer, Change change)
{
    if (buffer->changes_length == buffer->changes_capacity)
    {
        buffer->changes_capacity = buffer->changes_capacity ? buffer->changes_capacity * 2 : 4;
        buffer->changes = realloc(buffer->changes, sizeof(buffer->changes[0]) * buffer->changes_capacity);

        memset(buffer->changes + buffer->changes_length, 0, sizeof(buffer->changes[0]) * (buffer->changes_capacity - buffer->changes_length));
    }

    buffer->changes[buffer->changes_length++] = change;
}

static void partial_lex(Buffer* buffer, uint16_t from, uint16_t to)
{
    if (to < from)
    {
        uint16_t temp = to;
        to = from;
        from = temp;
    }

    bool continue_multiline_comment = false;
    for (uint16_t i = from; i <= to; i++)
    {
        assert(i < buffer->lines_length);
        lexical_analyze(buffer->language, &buffer->lines[i], &continue_multiline_comment);
    }
}

void do_changes(void)
{
    Tab* tab = &editor.tabs[editor.active_tab_index];
    View* view = find_active_tab_view(tab);
    Buffer* buffer = buffer_handle_to_pointer(view->buffer);
    
    if (buffer->changes_length == buffer->current_change)
        return;

    Change change = buffer->changes[buffer->current_change++];
    assert(change.tag == Change_Tag_break);

    Location start = change.cursor;
    Location location = change.cursor;
    while (buffer->current_change <= buffer->changes_length)
    {
        change = buffer->changes[buffer->current_change];
        if (change.tag == Change_Tag_break)
            break;
        else if (change.tag == Change_Tag_insert_character)
            assert(false && "Change_Tag_insert_character not supported.");
        else if (change.tag == Change_Tag_remove_character)
        {
            Line* line = &buffer->lines[location.line];
            assert(line->characters_length);

            memmove(&line->characters[location.column], &line->characters[location.column + 1], line->characters_length - location.column);
            if (location.column == line->characters_length)
                location.column++;

            line->characters_length--;
        }
        else if (change.tag == Change_Tag_insert_line)
            assert(false && "Change_Tag_insert_line not supported.");
        else if (change.tag == Change_Tag_remove_line)
            assert(false && "Change_Tag_remove_line not supported.");
        else if (change.tag == Change_Tag_merge_line)
            assert(false && "Change_Tag_merge_line not supported.");
        else if (change.tag == Change_Tag_split_line)
            assert(false && "Change_Tag_split_line not supported.");
        else
            assert(false && "Unexpected Change_Tag.");

        buffer->current_change++;
    }

    go_to(view, tab->rectangle, false, start);
    partial_lex(buffer, start.line, change.cursor.line);

    editor.refresh_needed = true;
}

void undo_changes(void)
{
    Tab* tab = &editor.tabs[editor.active_tab_index];
    View* view = find_active_tab_view(tab);
    Buffer* buffer = buffer_handle_to_pointer(view->buffer);

    if (!buffer->current_change)
        return;

    Change change;
    do
        // This skips the end break of the set of changes that 
        // we are about to undo in order to start from the starting
        // break.
        change = buffer->changes[--buffer->current_change];
    while (change.tag != Change_Tag_break);

    uint32_t current_change = buffer->current_change;
    Location start = change.cursor;
    Location location = change.cursor;

    current_change++;
    while (true)
    {
        change = buffer->changes[current_change];
        if (change.tag == Change_Tag_break)
            break;
        else if (change.tag == Change_Tag_insert_character)
            assert(false && "Change_Tag_insert_character not supported.");
        else if (change.tag == Change_Tag_remove_character)
        {
            Line* line = &buffer->lines[location.line];

            memmove(&line->characters[location.column + 1], &line->characters[location.column], line->characters_length - location.column);
            line->characters_length++;
            line->characters[location.column++] = change.character;
        }
        else if (change.tag == Change_Tag_insert_line)
            assert(false && "Change_Tag_insert_line not supported.");
        else if (change.tag == Change_Tag_remove_line)
            assert(false && "Change_Tag_remove_line not supported.");
        else if (change.tag == Change_Tag_merge_line)
            assert(false && "Change_Tag_merge_line not supported.");
        else if (change.tag == Change_Tag_split_line)
            assert(false && "Change_Tag_split_line not supported.");
        else
            assert(false && "Unexpected Change_Tag.");

        current_change++;
    }

    go_to(view, tab->rectangle, false, change.cursor);
    partial_lex(buffer, start.line, change.cursor.line);

    editor.refresh_needed = true;
}
