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

void do_changes(void)
{
    Tab* tab = &editor.tabs[editor.active_tab_index];
    View* view = find_active_tab_view(tab);
    Buffer* buffer = buffer_handle_to_pointer(view->buffer);
    
    if (buffer->changes_length - 1 == buffer->current_change)
        return;
    else if (buffer->current_change)
        buffer->current_change++;

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
            // TODO: Count consecutive Change_Tag_remove_character and remove them in one go.
            Line* line = &buffer->lines[location.line];
            assert(line->characters_length);

            remove_chars(line, location.column, 1);
        }
        else if (change.tag == Change_Tag_insert_line)
            assert(false && "Change_Tag_insert_line not supported.");
        else if (change.tag == Change_Tag_remove_line)
        {
            assert(!buffer->lines[location.line].characters_length);
            remove_lines(buffer, location.line, 1);
            // TODO: Count consecutive Change_Tag_remove_line and remove them in one go.
        }
        else if (change.tag == Change_Tag_merge_line)
        {
            Line* current_line = &buffer->lines[location.line];
            assert(current_line->characters_length);

            Line* next_line = &buffer->lines[location.line + 1];
            uint16_t combined_characters_length = current_line->characters_length + next_line->characters_length;

            if (current_line->characters_capacity < combined_characters_length)
            {
                while (current_line->characters_capacity < combined_characters_length)
                    current_line->characters_capacity = current_line->characters_capacity ? current_line->characters_capacity * 2 : 4;

                current_line->characters = realloc(current_line->characters, current_line->characters_capacity);
            }

            memcpy(&current_line->characters[current_line->characters_length], next_line->characters, next_line->characters_length);
            current_line->characters_length = combined_characters_length;

            remove_lines(buffer, location.line + 1, 1);
        }
        else if (change.tag == Change_Tag_split_line)
            assert(false && "Change_Tag_split_line not supported.");
        else
            assert(false && "Unexpected Change_Tag.");

        buffer->current_change++;
    }

    go_to(view, tab->rectangle, false, start);
    lexical_analyze_lines(buffer, start.line, change.cursor.line);

    editor.refresh_needed = true;
}

void undo_changes(void)
{
    Tab* tab = &editor.tabs[editor.active_tab_index];
    View* view = find_active_tab_view(tab);
    Buffer* buffer = buffer_handle_to_pointer(view->buffer);

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

    uint32_t current_change = buffer->current_change;
    Location start = change.cursor;
    Location location = change.cursor;

    current_change++;

    // TODO: This loop seems to be the same as in do_changes.
    // So when I support all the tags supported, factor this 
    // loop into a function:
    // 
    // void apply_changes(Tab* tab, View* view, Buffer* buffer, bool is_undo)
    // {
    //     static Change_Tag opposites[] = 
    //     {
    //         [Change_Tag_break]            = Change_Tag_break,
    //         [Change_Tag_insert_character] = Change_Tag_remove_character,
    //         [Change_Tag_remove_character] = Change_Tag_insert_character,
    //         [Change_Tag_insert_line]      = Change_Tag_remove_line,
    //         [Change_Tag_remove_line]      = Change_Tag_insert_line,
    //         [Change_Tag_merge_line]       = Change_Tag_split_line,
    //         [Change_Tag_split_line]       = Change_Tag_merge_line
    //     };
    // 
    //     while (true)
    //     {
    //         change = buffer->changes[current_change];
    //         if (is_undo)
    //              change.tag = opposites[change.tag];
    //        
    //         // Have a single switch here...
    //     }
    // }
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
            insert_char(line, change.character, location.column);
            if (index_of_last_character(line) != location.column)
                location.column++;
        }
        else if (change.tag == Change_Tag_insert_line)
            assert(false && "Change_Tag_insert_line not supported.");
        else if (change.tag == Change_Tag_remove_line)
            assert(false && "Change_Tag_remove_line not supported.");
        else if (change.tag == Change_Tag_merge_line)
        {
            Line* current_line = &buffer->lines[location.line];
            Line* next_line = insert_line(buffer, location.line + 1);
            uint16_t remainder = current_line->characters_length - location.column;

            insert_chars(next_line, 0, remainder, &current_line->characters[location.column]);
            remove_chars(current_line, location.column, remainder);
            
            location.line++;
            location.column = 0;
        }
        else if (change.tag == Change_Tag_split_line)
            assert(false && "Change_Tag_split_line not supported.");
        else
            assert(false && "Unexpected Change_Tag.");

        current_change++;
    }

    go_to(view, tab->rectangle, false, change.cursor);
    lexical_analyze_lines(buffer, start.line, location.line);

    editor.refresh_needed = true;
}
