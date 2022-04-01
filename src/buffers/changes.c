#include "buffers.h"
#include <assert.h>

void add_change(Buffer* buffer, Change change)
{
    INSERT_ELEMENTS(buffer->changes, buffer->changes_length, 1, &change);
}

static uint16_t count_consecutive_changes(Buffer* buffer)
{
    uint16_t count = 1;
    Change_Tag tag = buffer->changes[buffer->current_change].tag;

    while (tag == buffer->changes[buffer->current_change + count].tag)
        count++;

    return count;
}

static void apply_changes(Tab* tab, View* view, Buffer* buffer, bool is_undo)
{    
    static Change_Tag undoers[] = 
    {
        [Change_Tag_break]            = Change_Tag_break,
        [Change_Tag_insert_character] = Change_Tag_remove_character,
        [Change_Tag_remove_character] = Change_Tag_insert_character,
        [Change_Tag_insert_line]      = Change_Tag_remove_line,
        [Change_Tag_remove_line]      = Change_Tag_insert_line,
        [Change_Tag_merge_line]       = Change_Tag_split_line,
        [Change_Tag_split_line]       = Change_Tag_merge_line
    };

    Change change = buffer->changes[buffer->current_change++];
    assert(change.tag == Change_Tag_break);

    Location start = change.cursor;
    Location location = change.cursor;
    
    while (true)
    {
        change = buffer->changes[buffer->current_change];
        if (change.tag == Change_Tag_break)
            break;

        if (is_undo)
            change.tag = undoers[change.tag];

        switch (change.tag)
        {
            case Change_Tag_insert_character:
            {
                Line* line = &buffer->lines[location.line];
                insert_char(line, change.character, location.column);
                if (index_of_last_character(line) != location.column)
                    location.column++;

                buffer->current_change++;
                break;
            }

            case Change_Tag_remove_character:
            {
                Line* line = &buffer->lines[location.line];
                assert(line->characters_length);

                uint16_t chars = count_consecutive_changes(buffer);
                remove_chars(line, location.column, chars);
                buffer->current_change += chars;

                break;
            }

            case Change_Tag_insert_line:
                assert(false && "Change_Tag_insert_line not supported.");
                buffer->current_change++;
                break;

            case Change_Tag_remove_line:
            {
                assert(!buffer->lines[location.line].characters_length);
                remove_lines(buffer, location.line, 1);
                // TODO: Count consecutive Change_Tag_remove_line (that may or may 
                // not contain Change_Tag_remove_character in between) and remove them in one go.
                buffer->current_change++;
                break;
            }

            case Change_Tag_merge_line:
            {
                Line* current_line = &buffer->lines[location.line];
                assert(current_line->characters_length);

                Line* next_line = &buffer->lines[location.line + 1];
                uint16_t combined_characters_length = current_line->characters_length + next_line->characters_length;

                RESERVE_ELEMENTS(current_line->characters, combined_characters_length);
                memcpy(&current_line->characters[current_line->characters_length], next_line->characters, next_line->characters_length);
                current_line->characters_length = combined_characters_length;

                remove_lines(buffer, location.line + 1, 1);

                buffer->current_change++;
                break;
            }

            case Change_Tag_split_line:
            {
                Line* current_line = &buffer->lines[location.line];
                Line* next_line = insert_line(buffer, location.line + 1);
                uint16_t remainder = current_line->characters_length - location.column;

                insert_chars(next_line, 0, remainder, &current_line->characters[location.column]);
                remove_chars(current_line, location.column, remainder);

                location.line++;
                location.column = 0;

                buffer->current_change++;
                break;
            }

            default:
                assert(false && "Unexpected Change_Tag.");
                break;
        }
    }

    go_to(view, tab->rectangle, false, is_undo ? start : change.cursor);
    lexical_analyze_lines(buffer, start.line, location.line);

    editor.refresh_needed = true;
}

void do_changes(void)
{
    Tab* tab = &editor.tabs[editor.active_tab_index];
    View* view = find_active_tab_view(tab);
    Buffer* buffer = lookup_buffer(view->buffer);
    
    if (buffer->changes_length - 1 == buffer->current_change)
        return;

    apply_changes(tab, view, buffer, false);
}

void undo_changes(void)
{
    Tab* tab = &editor.tabs[editor.active_tab_index];
    View* view = find_active_tab_view(tab);
    Buffer* buffer = lookup_buffer(view->buffer);

    if (!buffer->current_change)
        return;
    else if (buffer->changes_length - 1 != buffer->current_change)
        buffer->current_change--;

    Change change;
    do
        // This skips the end break of the set of changes that 
        // we are about to undo in order to start from the starting
        // break.
        change = buffer->changes[--buffer->current_change];
    while (change.tag != Change_Tag_break);

    uint32_t temp = buffer->current_change;
    apply_changes(tab, view, buffer, true);

    buffer->current_change = temp;
}
