#include "modes.h"

void move_cursor_up_in_active_view(void)
{
    if (!editor.tabs_length)
        return;

    View* const view = find_active_editor_view();
    if (!view->offset.line && !view->cursor.line)
        return;

    editor.refresh_needed = go_to(view, editor.tabs[editor.active_tab_index].rectangle, true, (Location)
    {
        .line = view->offset.line + view->cursor.line - 1,
        .column = view->offset.column + view->cursor.column
    });
}

void move_cursor_down_in_active_view(void)
{
    if (!editor.tabs_length)
        return;

    View* const view = find_active_editor_view();
    if (view->offset.line + view->cursor.line > UINT16_MAX)
        return;

    editor.refresh_needed = go_to(view, editor.tabs[editor.active_tab_index].rectangle, true, (Location)
    {
        .line = view->offset.line + view->cursor.line + 1,
        .column = view->offset.column + view->cursor.column
    });
}

void move_cursor_left_in_active_view(void)
{
    if (!editor.tabs_length)
        return;

    View* const view = find_active_editor_view();
    if (!view->offset.column && !view->cursor.column)
        return;

    editor.refresh_needed = go_to(view, editor.tabs[editor.active_tab_index].rectangle, false, (Location)
    {
        .line = view->offset.line + view->cursor.line,
        .column = view->offset.column + view->cursor.column - 1
    });
}

void move_cursor_right_in_active_view(void)
{
    if (!editor.tabs_length)
        return;

    View* const view = find_active_editor_view();
    if (view->offset.column + view->cursor.column + 1 > UINT16_MAX)
        return;

    editor.refresh_needed = go_to(view, editor.tabs[editor.active_tab_index].rectangle, false, (Location)
    {
        .line = view->offset.line + view->cursor.line,
        .column = view->offset.column + view->cursor.column + 1
    });
}

void move_cursor_to_last_line_in_active_view(void)
{
    if (!editor.tabs_length)
        return;

    View* const view = find_active_editor_view();
    const Buffer* const buffer = buffer_handle_to_pointer(view->buffer);

    editor.refresh_needed = go_to(view, editor.tabs[editor.active_tab_index].rectangle, false, (Location)
    {
        .line = buffer->lines_length - 1,
        .column = view->offset.column + view->cursor.column
    });
}

void move_cursor_to_the_first_non_space_in_the_line_in_active_view(void)
{
    if (!editor.tabs_length)
        return;

    View* const view = find_active_editor_view();
    const Buffer* const buffer = buffer_handle_to_pointer(view->buffer);
    const Line* const line = &buffer->lines[view->offset.line + view->cursor.line];

    uint16_t column = 0;
    while (column < line->characters_length && (line->characters[column] == ' ' || line->characters[column] == '\t'))
        column++;

    go_to(view, editor.tabs[editor.active_tab_index].rectangle, false, (Location)
    {
        .line = view->offset.line + view->cursor.line,
        .column = column
    });

    // ^ is a dead key on Danish keyboards and calling go_to
    // with the same absolute location twice in a row returns
    // false the second time. 
    editor.refresh_needed = true;
}

void move_cursor_to_the_end_of_the_line_in_active_view(void)
{
    if (!editor.tabs_length)
        return;

    View* const view = find_active_editor_view();
    const Buffer* const buffer = buffer_handle_to_pointer(view->buffer);
    const Line* const line = &buffer->lines[view->offset.line + view->cursor.line];

    editor.refresh_needed = go_to(view, editor.tabs[editor.active_tab_index].rectangle, false, (Location)
    {
        .line = view->offset.line + view->cursor.line,
        .column = line->characters_length
    });
}
