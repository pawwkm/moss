#include "moss.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static Tab* add_tab(Editor* editor)
{
    if (editor->tabs_length == editor->tabs_capacity)
    {
        editor->tabs_capacity = editor->tabs_capacity ? editor->tabs_capacity * 2 : 4;
        editor->tabs = realloc(editor->tabs, sizeof(editor->tabs[0]) * editor->tabs_capacity);

        memset(editor->tabs + editor->tabs_length, 0, sizeof(editor->tabs[0]) * (editor->tabs_capacity - editor->tabs_length));
    }

    return &editor->tabs[editor->tabs_length++];
}

static void set_editor_title_to_buffer_path(const Buffer_Handle handle)
{
    const Buffer* const buffer = lookup_buffer(handle);
    set_editor_title(buffer->path, buffer->path_length);
}

static void set_editor_active_tab(Editor* editor, const uint8_t tab_index)
{
    editor->active_tab_index = tab_index;
    
    const Tab* const tab = &editor->tabs[tab_index];
    set_editor_title_to_buffer_path(tab->views[tab->active_view_index].buffer);
}

void update_all_tabs(Editor* editor)
{
    invalidate_block(editor, (Block)
    {
        .x = 0,
        .y = 0,
        .width = editor->width,
        .height = editor->height - editor->status_line_height
    });
}

void measure_tabs(Editor* editor)
{
    if (!editor->tabs_length)
        return;

    uint16_t width_of_all_separators = editor->tab_separator_width * (editor->tabs_length - 1);
    uint16_t width_of_tabs = (editor->width - width_of_all_separators) / editor->tabs_length;

    for (uint8_t i = 0; i < editor->tabs_length; i++)
    {
        editor->tabs[i].block = (Block)
        {
            .x = (width_of_tabs + editor->tab_separator_width) * i,
            .y = 0,
            .width = width_of_tabs,
            .height = editor->height - editor->status_line_height
        };
    }

    // In case there is some remainder width
    // add the remainder to the last tab.
    if (editor->tabs_length > 1)
        editor->tabs[editor->tabs_length - 1].block.width += (editor->width - width_of_all_separators) % editor->tabs_length;

    for (uint8_t t = 0; t < editor->tabs_length; t++)
    {
        Tab* restrict tab = &editor->tabs[t];
        for (uint8_t v = 0; v < tab->views_length; v++)
        {
            View* restrict view = &tab->views[v];
            go_to(editor, view, tab_to_view_block(editor, tab->block), (Location)
            {
                .line = view->offset.line + view->cursor.line,
                .column = view->offset.column + view->cursor.column
            });
        }
    }
}

static void insert_tab(Editor* editor, const uint8_t index)
{
    add_tab(editor);
    for (uint8_t i = editor->tabs_length - 1; i != index; i--)
        editor->tabs[i] = editor->tabs[i - 1];

    editor->tabs[index] = (Tab){ 0 };
}

static uint8_t insert_tab_before_active_tab(Editor* editor)
{
    insert_tab(editor, editor->active_tab_index);

    return editor->active_tab_index;
}

static uint8_t insert_tab_after_active_tab(Editor* editor)
{
    insert_tab(editor, editor->active_tab_index + 1);

    return editor->active_tab_index + 1;
}

static uint8_t add_view(Editor* editor, const uint8_t tab_index)
{
    Tab* tab = &editor->tabs[tab_index];
    RESERVE_ELEMENTS(tab->views, tab->views_length + 1);

    return tab->views_length++;
}

static void remove_view(Editor* editor, uint8_t tab_index, uint8_t view_index)
{
    Tab* tab = &editor->tabs[tab_index];
    while (view_index < tab->views_length - 1)
    {
        tab->views[view_index] = tab->views[view_index + 1];
        view_index++;
    }

    tab->views[--tab->views_length] = (View){ 0 };
}

void open_buffer_in_active_tab(Editor* editor, char* path, uint16_t path_length)
{
    if (!editor->tabs_length)
    {
        add_tab(editor);
        editor->active_tab_index = 0;

        measure_tabs(editor);
    }

    uint8_t view_index = add_view(editor, editor->active_tab_index);
    View* view = &editor->tabs[editor->active_tab_index].views[view_index];

    if (open_buffer(path, path_length, &view->buffer))
        set_editor_title_to_buffer_path(view->buffer);
    else
    {
        show_message(editor, "Could not open file.");
        remove_view(editor, editor->active_tab_index, view_index);
    }
}

View* find_active_editor_view(const Editor* editor)
{
    assert(editor->tabs_length);
    return find_active_tab_view(&editor->tabs[editor->active_tab_index]);
}

View* find_active_tab_view(const Tab* tab)
{
    assert(tab->views_length);
    return &tab->views[tab->active_view_index];
}

static void center_on_line(Editor* editor, View* view, Block view_block, uint16_t absolute_line)
{
    invalidate_block(editor, (Block)
    {
        .x = view_block.x,
        .y = editor->tab_header_height,
        .width = view_block.width,
        .height = view_block.height
    });

    view->offset.line = absolute_line - visible_lines_in_tabs(editor) / 2;
    view->cursor.line = visible_lines_in_tabs(editor) / 2;
}

static bool is_line_within_view(const Editor* editor, const View* view, uint16_t absolute_line)
{
    return view->offset.line <= absolute_line &&
           view->offset.line + visible_lines_in_tabs(editor) >= absolute_line;
}

static bool is_line_within_first_padded_page(const Editor* editor, uint16_t absolute_line)
{
    return absolute_line <= visible_lines_in_tabs(editor) - editor->line_scroll_padding;
}

static void go_up(Editor* editor, View* view, Block view_block, uint16_t absolute_line)
{   
    uint16_t line_to_go_up = (view->offset.line + view->cursor.line) - absolute_line;
    if (line_to_go_up == 1)
    {
        assert(false && "Invalidate view.");
        if (view->offset.line && view->cursor.line + 1 == editor->line_scroll_padding)
            view->offset.line--;
        else if (view->cursor.line)
            // This is within the first Tabs worth of lines.
            view->cursor.line--;
        else
            view->offset.line--;
    }
    else if (is_line_within_first_padded_page(editor, absolute_line))
    {
        assert(false && "Invalidate view.");
        view->offset.line = 0;
        view->cursor.line = absolute_line;
    }
    else if (is_line_within_view(editor, view, absolute_line))
    {
        assert(false && "Invalidate view.");
        if (view->offset.line && view->offset.line + editor->line_scroll_padding > absolute_line)
        {
            view->offset.line = absolute_line - editor->line_scroll_padding;
            view->cursor.line = editor->line_scroll_padding;
        }
        else
            view->cursor.line -= line_to_go_up;
    }
    else
        center_on_line(editor, view, view_block, absolute_line);
}

static void go_down(Editor* editor, View* view, Block view_block, uint16_t absolute_line)
{
    uint16_t lines_to_go_down = absolute_line - (view->offset.line + view->cursor.line);
    Buffer* buffer = lookup_buffer(view->buffer);

    if (lines_to_go_down == 1)
    {
        if (view->offset.line + view->cursor.line + 1 == buffer->lines_length)
        {
            if (view->offset.line + editor->line_scroll_padding == buffer->lines_length)
                return;

            // i just need to redraw the amount of lines since the
            // remainder is already on the current frame.

            invalidate_block(editor, (Block)
            {
                .x = view_block.x,
                .y = view_block.y,
                .width = view_block.width,
                .height = (buffer->lines_length - view->offset.line) * editor->font_height
            });

            view->offset.line++;
            view->cursor.line--;
        }
        else if (view->cursor.line + editor->line_scroll_padding >= visible_lines_in_tabs(editor))
        {
            invalidate_region(editor, (Region)
            {
                .scroll_x = 0,
                .scroll_y = -editor->font_height,
                .block = view_block
            });

            // Current cursor.
            invalidate_block(editor, (Block)
            {
                .x = 0,
                .y = (visible_lines_in_tabs(editor) - editor->line_scroll_padding + 1) * editor->font_height,
                .width = editor->font_width,
                .height = editor->font_height
            });

            // Next cursor.
            invalidate_block(editor, (Block)
            {
                .x = 0,
                .y = (visible_lines_in_tabs(editor) - editor->line_scroll_padding + 2) * editor->font_height,
                .width = editor->font_width,
                .height = editor->font_height
            });

            view->offset.line++;
        }
        else
        {
            // Invalidate the previous cursor block and the new one.
            invalidate_block(editor, intersecting_block(view_block, (Block)
            {
                .x = view_block.x + view->cursor.column * editor->font_width,
                .y = view_block.y + view->cursor.line * editor->font_height,
                .width = editor->font_width,
                .height = editor->font_height * 2
            }));

            view->cursor.line++;
        }
    }
    else if (is_line_within_view(editor, view, absolute_line))
    {
        if (view->offset.line + visible_lines_in_tabs(editor) - editor->line_scroll_padding < absolute_line)
        {
            uint16_t lines_over_padding = absolute_line - (view->offset.line + visible_lines_in_tabs(editor) - editor->line_scroll_padding);
            view->offset.line += lines_over_padding * 2;
            view->cursor.line -= lines_over_padding;
        }
        else
            view->cursor.line += lines_to_go_down;

        assert(false && "Invalidate view.");
    }
    else
        center_on_line(editor, view, view_block, absolute_line);
}

static void center_on_column(View* view, uint16_t visible_columns, uint16_t absolute_column)
{
    view->offset.column = absolute_column - visible_columns / 2;
    view->cursor.column = visible_columns / 2;
}

static void go_left(Editor* editor, View* view, Block view_block, uint16_t absolute_column)
{
    uint16_t visible_columns = view_block.width / editor->font_width;
    uint16_t columns_to_move = view->offset.column + view->cursor.column - absolute_column;

    if (columns_to_move > view->cursor.column && absolute_column < visible_columns)
    {
        view->offset.column = 0;
        view->cursor.column = absolute_column;
        assert(false && "Invalidate view.");
    }
    else if (view->offset.column <= absolute_column && view->offset.column + visible_columns > absolute_column)
    {
        assert(false && "Invalidate view.");
        view->cursor.column = absolute_column - view->offset.column;
    }
    else 
        center_on_column(view, visible_columns, absolute_column);
}

static void go_right(Editor* editor, View* view, Block view_block, uint16_t absolute_column)
{
    uint16_t visible_columns = view_block.width / editor->font_width;
    if (absolute_column < visible_columns)
    {
        view->offset.column = 0;
        view->cursor.column = absolute_column;
        assert(false && "Invalidate view.");
    }
    else if (view->offset.column <= absolute_column && view->offset.column + visible_columns > absolute_column)
    {
        view->cursor.column = absolute_column - view->offset.column;
        assert(false && "Invalidate view.");
    }
    else
        center_on_column(view, visible_columns, absolute_column);
}

void go_to(Editor* editor, View* view, Block view_block, Location absolute)
{
    Buffer* buffer = lookup_buffer(view->buffer);
    if (buffer->lines_length < absolute.line)
        absolute.line = buffer->lines_length - 1;

    Line* line = &buffer->lines[absolute.line];
    if (line->characters_length < absolute.column)
        absolute.column = line->characters_length;

    if (view->offset.column + view->cursor.column != absolute.column)
        absolute.column = line->characters_length < view->prefered_column ? line->characters_length : view->prefered_column;

    bool v_move = true;
    if (view->offset.line + view->cursor.line < absolute.line)
        go_down(editor, view, view_block, absolute.line);
    else if (view->offset.line + view->cursor.line > absolute.line)
        go_up(editor, view, view_block, absolute.line);
    else
        v_move = false;

    bool h_move = true;
    if (view->offset.column + view->cursor.column > absolute.column)
        go_left(editor, view, view_block, absolute.column);
    else if (view->offset.column + view->cursor.column < absolute.column)
        go_right(editor, view, view_block, absolute.column);
    else
        h_move = false;

    view->prefered_column = view->offset.column + view->cursor.column;
    if (v_move || h_move)
        invalidate_location(editor);
}

void move_active_view_to_the_left(Editor* editor)
{
    if (editor->tabs_length < 2 || editor->active_tab_index == 0)
        return;

    // TODO: Draw the rest of the owl.
}

void move_active_view_to_the_right(Editor* editor)
{
    if (!editor->tabs_length)
        return;

    Tab* tab = &editor->tabs[editor->active_tab_index];
    if (tab->active_view_index == tab->views_length - 1)
        return;

    // TODO: Draw the rest of the owl.
}

void move_active_tab_to_the_left(Editor* editor)
{
    if (editor->tabs_length < 2 || editor->active_tab_index == 0)
        return;

    Tab temp = editor->tabs[editor->active_tab_index - 1];
    editor->tabs[editor->active_tab_index - 1] = editor->tabs[editor->active_tab_index];
    editor->tabs[editor->active_tab_index] = temp;

    set_editor_active_tab(editor, editor->active_tab_index - 1);

    invalidate_block(editor, editor->tabs[editor->active_tab_index].block);
    invalidate_block(editor, editor->tabs[editor->active_tab_index - 1].block);
}

void move_active_tab_to_the_right(Editor* editor)
{
    if (editor->tabs_length < 2 || editor->active_tab_index == editor->tabs_length - 1)
        return;

    Tab temp = editor->tabs[editor->active_tab_index + 1];
    editor->tabs[editor->active_tab_index + 1] = editor->tabs[editor->active_tab_index];
    editor->tabs[editor->active_tab_index] = temp;

    set_editor_active_tab(editor, editor->active_tab_index + 1);

    invalidate_block(editor, editor->tabs[editor->active_tab_index].block);
    invalidate_block(editor, editor->tabs[editor->active_tab_index + 1].block);
}

void put_active_view_in_new_tab_to_the_left(Editor* editor)
{
    if (!editor->tabs_length)
        return;
    
    if (editor->tabs[editor->active_tab_index].views_length == 1)
        return;

    uint8_t new_tab_index = insert_tab_before_active_tab(editor);
    Tab* restrict new_tab = &editor->tabs[new_tab_index];

    Tab* restrict active_tab = &editor->tabs[++editor->active_tab_index];

    uint8_t view_index_in_new_tab = add_view(editor, new_tab_index);
    new_tab->views[view_index_in_new_tab] = active_tab->views[active_tab->active_view_index];

    remove_view(editor, editor->active_tab_index, active_tab->active_view_index);
    set_editor_active_tab(editor, active_tab->active_view_index);

    measure_tabs(editor);
    update_all_tabs(editor);
}

void put_active_view_in_new_tab_to_the_right(Editor* editor)
{
    if (!editor->tabs_length)
        return;

    Tab* restrict active_tab = &editor->tabs[editor->active_tab_index];
    if (active_tab->views_length == 1)
        return;

    uint8_t new_tab_index = insert_tab_after_active_tab(editor);
    Tab* restrict new_tab = &editor->tabs[new_tab_index];

    uint8_t view_index_in_new_tab = add_view(editor, new_tab_index);
    new_tab->views[view_index_in_new_tab] = active_tab->views[active_tab->active_view_index];

    remove_view(editor, editor->active_tab_index, active_tab->active_view_index);
    if (active_tab->active_view_index)
        active_tab->active_view_index--;

    set_editor_active_tab(editor, active_tab->active_view_index);

    measure_tabs(editor);
    update_all_tabs(editor);
}

void put_active_view_in_tab_to_the_left(Editor* editor)
{
    if (editor->tabs_length < 2 || editor->active_tab_index == editor->tabs_length - 1)
        return;
}

void put_active_view_in_tab_to_the_right(Editor* editor)
{
    if (editor->tabs_length < 2 || editor->active_tab_index == 0)
        return;
}

void activate_left_hand_side_view(Editor* editor)
{
    if (!editor->tabs_length)
        return;

    Tab* active_tab = &editor->tabs[editor->active_tab_index];
    if (!active_tab->active_view_index)
        return;

    set_editor_title_to_buffer_path(active_tab->views[--active_tab->active_view_index].buffer);
    invalidate_block(editor, active_tab->block);
}

void activate_right_hand_side_view(Editor* editor)
{
    if (!editor->tabs_length)
        return;

    Tab* active_tab = &editor->tabs[editor->active_tab_index];
    if (active_tab->active_view_index == active_tab->views_length - 1)
        return;

    set_editor_title_to_buffer_path(active_tab->views[++active_tab->active_view_index].buffer);
    invalidate_block(editor, active_tab->block);
}

void activate_left_hand_side_tab(Editor* editor)
{
    if (editor->tabs_length < 2 || editor->active_tab_index == 0)
        return;

    invalidate_block(editor, editor->tabs[editor->active_tab_index--].block);
    invalidate_block(editor, editor->tabs[editor->active_tab_index].block);
    set_editor_active_tab(editor, editor->active_tab_index);
}

void activate_right_hand_side_tab(Editor* editor)
{
    if (editor->tabs_length < 2 || editor->active_tab_index == editor->tabs_length - 1)
        return;

    invalidate_block(editor, editor->tabs[editor->active_tab_index++].block);
    invalidate_block(editor, editor->tabs[editor->active_tab_index].block);
    set_editor_active_tab(editor, editor->active_tab_index);
}

void close_active_tab(Editor* editor)
{
    // TODO: Check that all buffers that has a single reference has 
    // no unsaved changes.
    
    (void)editor;
}

uint16_t visible_lines_in_tabs(const Editor* editor)
{
    return (editor->height - editor->tab_header_height - editor->status_line_height) / editor->font_height;
}
