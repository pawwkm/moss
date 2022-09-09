#include "moss.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define MOTIONS "hjkl^$webGfFtT"

static void invalidate_mode(Editor* editor)
{
    invalidate_block(editor, (Block)
    {
        .x = 0,
        .y = editor->height - editor->status_line_height,
        .width = editor->font_width * (uint16_t)strlen("NORMAL"),
        .height = editor->font_height
    });
}

static void enter_insert_mode(Editor* editor)
{
    if (!editor->tabs_length)
        return;

    log("Mode_insert");
    editor->mode = Mode_insert;
    invalidate_mode(editor);
}

static void exit_insert_mode(Editor* editor)
{
    log("Mode_normal");
    editor->mode = Mode_normal;
    invalidate_mode(editor);
}

static void enter_visual_mode(Editor* editor)
{
    if (!editor->tabs_length)
        return;

    log("Mode_visual");
    editor->mode = Mode_visual;
    invalidate_mode(editor);
}

static void exit_visual_mode(Editor* editor)
{
    log("Mode_normal");
    editor->mode = Mode_normal;
    invalidate_mode(editor);
}

static void enter_tab_mode(Editor* editor)
{
    if (!editor->tabs_length)
        return;

    log("Mode_tab");
    editor->mode = Mode_tab;
    invalidate_mode(editor);
}

static void exit_tab_mode(Editor* editor)
{
    log("Mode_normal");
    editor->mode = Mode_normal;
    invalidate_mode(editor);
}

static bool character_is_1_of(char c, const char* string)
{
    for (; *string; string++)
    {
        if (*string == c)
            return true;
    }

    return false;
}

static void invalidate_command(Editor* editor)
{
    if (editor->command_length == 1 && character_is_1_of(editor->command[0], MOTIONS))
        // Motions without repitition are never displayed so there is nothing to redraw.
        return;

    invalidate_block(editor, (Block)
    {
        .x = editor->font_width * (uint16_t)strlen("NORMAL"),
        .y = editor->height - editor->status_line_height,
        // +1 for the gap between the mode and command text.
        .width = editor->font_width * (editor->command_length + 1),
        .height = editor->font_height
    });
}

static void insert_character_into_command(Editor* editor, const char character)
{
    RESERVE_ELEMENTS(editor->command, editor->command_length + 1);
    editor->command[editor->command_length++] = character;

    invalidate_command(editor);
}

static void clear_command(Editor* editor)
{
    if (editor->command_length)
    {
        invalidate_command(editor);

        memset(editor->command, 0, editor->command_length);
        editor->command_length = 0;
    }
}

static void delete_previous_character_in_command(Editor* editor)
{
    if (!editor->command_length)
        return;

    REMOVE_ELEMENTS(editor->command, editor->command_length - 1, 1);

    editor->command_length--;
    invalidate_block(editor, (Block)
    {
        .x = 0,
        .y = editor->height - editor->status_line_height,
        .width = editor->font_width * (5 + editor->command_length),
        .height = editor->font_height
    });
}

static Location motion_to_location(char motion, uint16_t repetition, Buffer* buffer, View* view, char c)
{
    Location location =
    {
        .line = view->offset.line + view->cursor.line,
        .column = 0
    };

    switch (motion)
    {
        case 'h':
            if ((int32_t)view->offset.column + (int32_t)view->cursor.column - repetition < 0)
                location.column = 0;
            else
                location.column = view->offset.column + view->cursor.column - repetition;

            location.line = view->offset.line + view->cursor.line;
            break;

        case 'j':
        {
            if ((uint32_t)view->offset.line + (uint32_t)view->cursor.line + (uint32_t)repetition > buffer->lines_length)
                location.line = buffer->lines_length - 1;
            else
                location.line = view->offset.line + view->cursor.line + repetition;

            Line* line = &buffer->lines[location.line];
            location.column = view->offset.column + view->cursor.column;
            if (location.column > index_of_character_append(line))
                location.column = index_of_character_append(line);

            break;
        }

        case 'k':
        {
            if ((int32_t)view->offset.line + (int32_t)view->cursor.line - (int32_t)repetition < 0)
                location.line = 0;
            else
                location.line = view->offset.line + view->cursor.line - repetition;

            Line* line = &buffer->lines[location.line];
            location.column = view->offset.column + view->cursor.column;
            if (location.column > index_of_character_append(line))
                location.column = index_of_character_append(line);

            break;
        }

        case 'l':
        {
            Line* line = &buffer->lines[location.line];
            if ((uint32_t)view->offset.column + (uint32_t)view->cursor.column + (uint32_t)repetition > index_of_character_append(line))
                location.column = index_of_character_append(line);
            else
                location.column = view->offset.column + view->cursor.column + repetition;

            location.line = view->offset.line + view->cursor.line;
            break;
        }

        case '^':
        {
            Line* line = &buffer->lines[location.line];
            while (location.column < index_of_character_append(line) && is_space(line->characters[location.column]))
                location.column++;

            break;
        }

        case '$':
        {
            assert(repetition == 1 && "Repetition not implemented for $ motion.");
            Line* line = &buffer->lines[location.line];
            location.column = line->characters_length;
            break;
        }

        case 'w':
            assert(false && "w motion not implemented.");
            break;
 
        case 'e':
            assert(false && "e motion not implemented.");
            break;

        case 'b':
            assert(false && "b motion not implemented.");
            break;

        case 'G':
            if (repetition <= buffer->lines_length - 1)
                location.line = repetition;
            else
                location.line = buffer->lines_length - 1;
            
            break;
        
        case 'f':
        case 't':
        {
            location.column = view->offset.column + view->cursor.column;
            if (c == 0xFF)
                break;

            Line* line = &buffer->lines[location.line];
            uint16_t index = location.column;
            
            while (index < line->characters_length)
            {
                if (line->characters[index] == c)
                {
                    if (--repetition)
                        index++;
                    else
                        break;
                }

                index++;
            }

            if (!repetition)
                location.column = motion == 'f' ? index + 1 : index;

            break;
        }

        case 'F':
        case 'T':
            location.column = view->offset.column + view->cursor.column;
            if (c == 0xFF)
                break;

            Line* line = &buffer->lines[location.line];
            uint16_t index = location.column;

            while (true)
            {
                if (line->characters[index] == c)
                {
                    if (--repetition)
                        index--;
                    else
                        break;
                }

                if (!index)
                    break;

                index--;
            }

            if (!repetition)
                location.column = motion == 'T' ? index + 1 : index;

            break;

        default:
            assert(false && "Unknown motion.");
            break;
    }

    return location;
}

static void object_to_span(char object, char modifier, Location* start, Location* end, Buffer* buffer, View* view)
{
    *start = (Location)
    {
        .line = view->offset.line + view->cursor.line,
        .column = view->offset.column + view->cursor.column
    };

    Location t_start = *start;
    Location t_end = *start;

    if (modifier)
    {
        char left;
        char right;

        switch (object)
        {
            case '[':
            case '{':
            case '<':
                left = object;
                right = object + 2;
                break;

            case ']':
            case '}':
            case '>':
                left = object - 2;
                right = object;
                break;

            case '(':
            case ')':
                left  = '(';
                right = ')'; 
                break;

            case '\'':
            case '"':
            case '`':
                left  = object;
                right = object; 
                break;
        }

        // Look to the left.
        while (true)
        {
            // Doing this check even when the i modifier is present handles the 
            // case where the cursor is on the right character from the start.
            if (index_of_last_character(&buffer->lines[t_start.line]) >= t_start.column &&
                buffer->lines[t_start.line].characters[t_start.column] == left)
                break;

            if (modifier == 'i')
            {
                if (t_start.column)
                {
                    if (buffer->lines[t_start.line].characters[t_start.column - 1] == left)
                    {
                        t_start.column--;
                        break;
                    }
                }
                else if (t_start.line)
                {
                    Line* line = &buffer->lines[t_start.line - 1];
                    if (line->characters_length && line->characters[index_of_last_character(line)] == left)
                        break;
                }
                else
                    return;
            }

            if (!t_start.line && !t_start.column)
                return;
            else if (t_start.column)
                t_start.column--;
            else
            {
                t_start.line--;
                t_start.column = index_of_character_append(&buffer->lines[t_start.line]);
            }
        }
            
        if (modifier == 'a')
        {
            // Jump over 'left'.
            if (t_start.column)
                t_start.column--;
            else
            {
                t_start.line--;
                t_start.column = index_of_character_append(&buffer->lines[t_start.line]);
            }

            if (t_start.line || t_start.column)
            {
                Line* line = &buffer->lines[t_start.line];
                while (is_space(line->characters[t_start.column]))
                {
                    if (!t_start.line && !t_start.column)
                        break;
                    else if (t_start.column)
                        t_start.column--;
                    else
                    {
                        t_start.line--;
                        t_start.column = index_of_character_append(&buffer->lines[t_start.line]);

                        line = &buffer->lines[t_start.line];
                    }
                }
            }
        }

        // Look to the right.
        while (true)
        {
            // Doing this check even when the i modifier is present handles the 
            // case where the cursor is on the right character from the start.
            if (index_of_last_character(&buffer->lines[t_end.line]) >= t_end.column &&
                buffer->lines[t_end.line].characters[t_end.column] == right)
                break;

            if (modifier == 'i')
            {
                if (index_of_last_character(&buffer->lines[t_end.line]) > t_end.column)
                {
                    if (buffer->lines[t_end.line].characters[t_end.column + 1] == right)
                        break;
                }
                else if (buffer->lines[t_end.line].characters_length != t_end.line)
                {
                    Line* line = &buffer->lines[t_end.line + 1];
                    if (line->characters_length && line->characters[0] == right)
                        break;
                }
                else
                    return;
            }

            if (index_of_character_append(&buffer->lines[t_end.line]) == t_end.column)
            {
                if (t_end.line + 1 == buffer->lines_length)
                    return;
                else
                {
                    t_end.line++;
                    t_end.column = 0;
                }
            }
            else
                t_end.column++;
        }

        if (modifier == 'a')
        {
            // Jump over 'right'.
            if (index_of_last_character(&buffer->lines[t_end.line]) == t_end.column)
            {
                if (t_end.line + 1 <= buffer->lines_length)
                {
                    t_end.line++;
                    t_end.column = 0;
                }
            }
            else
                t_end.column++;

            while (t_end.line < buffer->lines_length || t_end.column < index_of_character_append(&buffer->lines[t_end.line]))
            {
                Line* line = &buffer->lines[t_end.line];
                if (line->characters_length && !is_space(line->characters[t_end.column]))
                    break;

                if (index_of_character_append(line) == t_end.column)
                {
                    if (t_end.line + 1 == buffer->lines_length)
                        break;
                    else
                    {
                        t_end.line++;
                        t_end.column = 0;
                    }
                }
                else
                    t_end.column++;
            }
        }

        *start = t_start;
        *end = t_end;
    }
    else
        assert(false && "w object not implemented.");
}

void interpret_character(Editor* editor, char character, bool ctrl)
{
    assert(character <= 127 && character >= 0);

    // Later to be used for CTRL+n and CTRL+p where going
    // to next and previous x is needed.
    (void)ctrl;

    switch (editor->mode)
    {
        case Mode_normal:
        {
            if (character == '\x1B')
            {
                clear_command(editor);
                return;
            }

            if (!editor->command_length)
            {
                switch (character)
                {
                    case '\x7F':
                        delete_previous_character_in_command(editor);
                        return;

                    case '\t':
                        enter_tab_mode(editor);
                        return;

                    case 'i':
                        enter_insert_mode(editor);
                        return;

                    case 'I':
                    {
                        if (!editor->tabs_length)
                            return;

                        View* view = find_active_editor_view(editor);
                        Buffer* buffer = lookup_buffer(view->buffer);
                        Block block = tab_to_view_block(editor, editor->tabs[editor->active_tab_index].block);
                        Location location = motion_to_location('^', 1, buffer, view, 0xFF);

                        go_to(editor, view, block, false, location);
                        enter_insert_mode(editor);

                        return;
                    }

                    case 'a':
                    {
                        if (!editor->tabs_length)
                            return;

                        View* view = find_active_editor_view(editor);
                        Buffer* buffer = lookup_buffer(view->buffer);
                        Block block = tab_to_view_block(editor, editor->tabs[editor->active_tab_index].block);
                        Location location = motion_to_location('l', 1, buffer, view, 0xFF);

                        go_to(editor, view, block, false, location);
                        enter_insert_mode(editor);

                        return;
                    }

                    case 'A':
                    {
                        if (!editor->tabs_length)
                            return;

                        View* view = find_active_editor_view(editor);
                        Buffer* buffer = lookup_buffer(view->buffer);
                        Block block = tab_to_view_block(editor, editor->tabs[editor->active_tab_index].block);
                        Location location = motion_to_location('$', 1, buffer, view, 0xFF);

                        go_to(editor, view, block, false, location);
                        enter_insert_mode(editor);

                        return;
                    }

                    case 'o':
                        assert(false);
                        break;

                    case 'O':
                        assert(false);
                        break;

                    case 'v':
                        enter_visual_mode(editor);
                        break;
                    
                    case 'u':
                        undo_changes(editor);
                        return;

                    case 'r':
                        do_changes(editor);
                        return;
                }
            }

            char* shorthands = "CD";
            char* operators  = "dcyg";
            char* objects    = "w[]{}()<>`'\"";
            char* modifiers  = "ai";

            // Parse command.
            insert_character_into_command(editor, character);

            uint16_t command_index = 0;
            uint16_t repetition = 0;
            while (command_index != editor->command_length && is_decimal(editor->command[command_index]))
            {
                uint32_t temp = repetition * 10 + editor->command[command_index++] - '0';
                if (temp > UINT16_MAX)
                {
                    repetition = UINT16_MAX;
                    while (command_index != editor->command_length && is_decimal(editor->command[command_index]))
                        command_index++;
                }
                else
                    repetition = (uint16_t)temp;
            }

            if (command_index == editor->command_length)
                return;

            char operator = 0;
            char motion   = 0;
            char modifier = 0;
            char object   = 0;
            char c        = 0xFF;

            if (character_is_1_of(editor->command[command_index], operators))
            {
                operator = editor->command[command_index++];
                if (command_index == editor->command_length)
                    return;

                if (editor->command[command_index] == operator)
                {
                    command_index++;
                    if (operator == 'g')
                    {
                        operator = 0;
                        motion = 'G';
                        repetition = 0;
                    }
                    else
                        motion = 'L'; // Internal motion for `the whole current line`.
                }
                else if (character_is_1_of(editor->command[command_index], MOTIONS))
                {
                    motion = editor->command[command_index++];
                    if (repetition && motion == 'G')
                        repetition--;
                    else if (!repetition)
                        repetition = motion == 'G' ? UINT16_MAX : 1;

                    if (character_is_1_of(motion, "fFtT"))
                    {
                        if (command_index == editor->command_length)
                            return;

                        c = editor->command[command_index++];
                    }
                }
                else if (!repetition)
                {
                    if (character_is_1_of(editor->command[command_index], modifiers))
                        modifier = editor->command[command_index++];
                    else 
                        return;

                    if (command_index != editor->command_length && character_is_1_of(editor->command[command_index], objects))
                        object = editor->command[command_index++];
                    else
                        return;
                }
                else
                    return;
            }
            else if (character_is_1_of(editor->command[command_index], shorthands))
            {
                operator = editor->command[command_index++] + 32;
                motion = '$';

                if (!repetition)
                    repetition = 1;
            }
            else if (character_is_1_of(editor->command[command_index], MOTIONS))
            {
                motion = editor->command[command_index++];
                if (repetition && motion == 'G')
                    repetition--;
                else if (!repetition)
                    repetition = motion == 'G' ? UINT16_MAX : 1;

                if (character_is_1_of(motion, "fFtT"))
                {
                    if (command_index == editor->command_length)
                        return;

                    c = editor->command[command_index++];
                }
            }
            else
                return;

            // Reduce operators, motions and objects to a single span of characters.
            bool use_preferred_column = 'j' == motion || 'k' == motion;

            View* view = find_active_editor_view(editor);
            Buffer* buffer = lookup_buffer(view->buffer);

            Location start = { 0 };
            Location end   = { 0 };
            if (operator && motion)
            {
                start.line = view->offset.line + view->cursor.line;
                start.column = view->offset.column + view->cursor.column;

                end = motion_to_location(motion, repetition, buffer, view, c); 
            }
            else if (object)
                object_to_span(object, modifier, &start, &end, buffer, view);
            else if (motion)
            {
                start.line = view->offset.line + view->cursor.line;
                start.column = view->offset.column + view->cursor.column;

                end = motion_to_location(motion, repetition, buffer, view, c);
                Block block = tab_to_view_block(editor, editor->tabs[editor->active_tab_index].block);

                go_to(editor, view, block, use_preferred_column, end);
            }

            if (start.line > end.line || start.line == end.line && start.column > end.column)
            {
                Location temp = start;
                start = end;
                end = temp;
            }

            if ((start.line != end.line || start.column != end.column) && (operator == 'c' || operator == 'd'))
            {
                // Removes any future changes before appending to the change list.
                buffer->changes_length = buffer->current_change;

                Change change =
                {
                    .tag = Change_Tag_break,
                    .cursor = start
                };

                add_change(buffer, change);
                Location current = start;

                while (current.line != end.line || current.column != end.column)
                {
                    Line* line = &buffer->lines[current.line];
                    if (index_of_character_append(line) == current.column)
                    {
                        if (start.column)
                            change.tag = Change_Tag_merge_line;
                        else
                            change.tag = Change_Tag_remove_line;

                        current.line++;
                        current.column = 0;
                    }
                    else
                    {
                        change.tag = Change_Tag_remove_character;
                        change.character = line->characters[current.column];

                        current.column++;
                    }

                    add_change(buffer, change);
                }

                change.tag = Change_Tag_break;
                change.cursor = start;

                add_change(buffer, change);
                do_changes(editor);
            }
            else if (operator == 'y')
            {
            }
            
            clear_command(editor);
            if (operator == 'c')
                enter_insert_mode(editor);
            
            break;
        }

        case Mode_insert:
        {
            if (character == '\x1B')
                exit_insert_mode(editor);

            break;
        }

        case Mode_tab:
        {
            switch (character)
            {
                case 'h':
                    move_active_view_to_the_left(editor);
                    break;

                case 'j':
                    move_active_view_to_the_right(editor);
                    break;

                case 'H':
                    move_active_tab_to_the_left(editor);
                    break;

                case 'J':
                    move_active_tab_to_the_right(editor);
                    break;

                case 'k':
                    put_active_view_in_new_tab_to_the_left(editor);
                    break;

                case 'l':
                    put_active_view_in_new_tab_to_the_right(editor);
                    break;

                case 'K':
                    put_active_view_in_tab_to_the_left(editor);
                    break;

                case 'L':
                    put_active_view_in_tab_to_the_right(editor);
                    break;

                case 'f':
                    activate_left_hand_side_view(editor);
                    break;

                case 'g':
                    activate_right_hand_side_view(editor);
                    break;

                case 'd':
                    activate_right_hand_side_tab(editor);
                    break;

                case 's':
                    activate_left_hand_side_tab(editor);
                    break;

                case 'C':
                    close_active_tab(editor);
                    break;

                case '\x1B':
                    exit_tab_mode(editor);
                    break;
            }

            break;
        }

        case Mode_visual:
        {
            switch (character)
            {
                case '\x1B':
                    exit_visual_mode(editor);
                    break;
            }
        } 
    }
}
