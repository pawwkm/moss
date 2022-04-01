#include "moss.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static Tab* add_tab(void)
{
    if (editor.tabs_length == editor.tabs_capacity)
    {
        editor.tabs_capacity = editor.tabs_capacity ? editor.tabs_capacity * 2 : 4;
        editor.tabs = realloc(editor.tabs, sizeof(editor.tabs[0]) * editor.tabs_capacity);

        memset(editor.tabs + editor.tabs_length, 0, sizeof(editor.tabs[0]) * (editor.tabs_capacity - editor.tabs_length));
    }

    return &editor.tabs[editor.tabs_length++];
}

static void set_editor_title_to_buffer_path(const Buffer_Handle handle)
{
    const Buffer* const buffer = lookup_buffer(handle);
    set_editor_title(buffer->path, buffer->path_length);
}

static void set_editor_active_tab(const uint8_t tab_index)
{
    editor.active_tab_index = tab_index;
    
    const Tab* const tab = &editor.tabs[tab_index];
    set_editor_title_to_buffer_path(tab->views[tab->active_view_index].buffer);
}

void measure_tabs(void)
{
    if (!editor.tabs_length)
        return;

    uint16_t width_of_all_separators = TAB_SEPARATOR_WIDTH * (editor.tabs_length - 1);
    uint16_t width_of_tabs = (editor.width - width_of_all_separators) / editor.tabs_length;

    for (uint8_t i = 0; i < editor.tabs_length; i++)
    {
        editor.tabs[i].rectangle = (Rectangle)
        {
            .position.x = (width_of_tabs + TAB_SEPARATOR_WIDTH) * i,
            .position.y = 0,
            .width = width_of_tabs,
            .height = editor.height - STATUS_LINE_HEIGHT
        };
    }

    // In case there is some remainder width
    // add the remainder to the last tab.
    if (editor.tabs_length > 1)
        editor.tabs[editor.tabs_length - 1].rectangle.width += (editor.width - width_of_all_separators) % editor.tabs_length;

    for (uint8_t t = 0; t < editor.tabs_length; t++)
    {
        Tab* restrict tab = &editor.tabs[t];
        for (uint8_t v = 0; v < tab->views_length; v++)
        {
            View* restrict view = &tab->views[v];
            go_to(view, tab->rectangle, false, (Location)
            {
                .line = view->offset.line + view->cursor.line,
                .column = view->offset.column + view->cursor.column
            });
        }
    }
}

static void insert_tab(const uint8_t index)
{
    add_tab();
    for (uint8_t i = editor.tabs_length - 1; i != index; i--)
        editor.tabs[i] = editor.tabs[i - 1];

    editor.tabs[index] = (Tab){ 0 };
}

static uint8_t insert_tab_before_active_tab(void)
{
    insert_tab(editor.active_tab_index);

    return editor.active_tab_index;
}

static uint8_t insert_tab_after_active_tab(void)
{
    insert_tab(editor.active_tab_index + 1);

    return editor.active_tab_index + 1;
}

static uint8_t add_view(const uint8_t tab_index)
{
    Tab* tab = &editor.tabs[tab_index];
    RESERVE_ELEMENTS(tab->views, tab->views_length + 1);

    return tab->views_length++;
}

static void remove_view(uint8_t tab_index, uint8_t view_index)
{
    Tab* tab = &editor.tabs[tab_index];
    while (view_index < tab->views_length - 1)
    {
        tab->views[view_index] = tab->views[view_index + 1];
        view_index++;
    }

    tab->views[--tab->views_length] = (View){ 0 };
}

void open_buffer_in_active_tab(char* path, uint16_t path_length)
{
    if (!editor.tabs_length)
    {
        add_tab();
        editor.active_tab_index = 0;

        measure_tabs();
    }

    uint8_t view_index = add_view(editor.active_tab_index);
    View* view = &editor.tabs[editor.active_tab_index].views[view_index];

    if (open_buffer(path, path_length, &view->buffer))
        set_editor_title_to_buffer_path(view->buffer);
    else
    {
        show_message("Could not open file.");
        remove_view(editor.active_tab_index, view_index);
    }
}

View* find_active_editor_view(void)
{
    assert(editor.tabs_length);
    return find_active_tab_view(&editor.tabs[editor.active_tab_index]);
}

View* find_active_tab_view(const Tab* tab)
{
    assert(tab->views_length);
    return &tab->views[tab->active_view_index];
}

typedef enum
{
    Vertical_Movement_none,
    Vertical_Movement_up,
    Vertical_Movement_down
} Vertical_Movement;

// TODO: Detect needed movement when resizing tabs.
static Vertical_Movement find_vertical_movement(const View* view, uint16_t absolute_line)
{
    if (view->offset.line + view->cursor.line < absolute_line)
        return Vertical_Movement_down;
    else if (view->offset.line + view->cursor.line > absolute_line)
        return Vertical_Movement_up;
    else
        return Vertical_Movement_none;
}

static void center_on_line(View* view, uint16_t absolute_line)
{
    view->offset.line = absolute_line - VISIBLE_LINES_IN_TAB / 2;
    view->cursor.line = VISIBLE_LINES_IN_TAB / 2;
}

static bool is_line_within_view(const View* view, uint16_t absolute_line)
{
    return view->offset.line <= absolute_line &&
           view->offset.line + VISIBLE_LINES_IN_TAB >= absolute_line;
}

static bool is_line_within_first_padded_page(uint16_t absolute_line)
{
    return absolute_line <= VISIBLE_LINES_IN_TAB - LINE_SCROLL_PADDING;
}

static void go_up(View* view, uint16_t absolute_line)
{
    uint16_t line_to_go_up = (view->offset.line + view->cursor.line) - absolute_line;
    if (line_to_go_up == 1)
    {
        if (view->offset.line && view->cursor.line + 1 == LINE_SCROLL_PADDING)
            view->offset.line--;
        else if (view->cursor.line)
            // This is within the first Tabs worth of lines.
            view->cursor.line--;
        else
            view->offset.line--;
    }
    else if (is_line_within_first_padded_page(absolute_line))
    {
        view->offset.line = 0;
        view->cursor.line = absolute_line;
    }
    else if (is_line_within_view(view, absolute_line))
    {
        if (view->offset.line && view->offset.line + LINE_SCROLL_PADDING > absolute_line)
        {
            view->offset.line = absolute_line - LINE_SCROLL_PADDING;
            view->cursor.line = LINE_SCROLL_PADDING;
        }
        else
            view->cursor.line -= line_to_go_up;
    }
    else
        center_on_line(view, absolute_line);
}

static void go_down(View* view, uint16_t absolute_line)
{
    uint16_t line_to_go_down = absolute_line - (view->offset.line + view->cursor.line);
    Buffer* buffer = lookup_buffer(view->buffer);

    if (line_to_go_down == 1)
    {
        if (view->offset.line + view->cursor.line + 1 == buffer->lines_length)
        {
            if (view->offset.line + LINE_SCROLL_PADDING == buffer->lines_length)
                return;

            view->offset.line++;
            view->cursor.line--;
        }
        else if (view->cursor.line + LINE_SCROLL_PADDING >= VISIBLE_LINES_IN_TAB)
            view->offset.line++;
        else
            view->cursor.line++;
    }
    else if (is_line_within_view(view, absolute_line))
    {
        if (view->offset.line + VISIBLE_LINES_IN_TAB - LINE_SCROLL_PADDING < absolute_line)
        {
            uint16_t lines_over_padding = absolute_line - (view->offset.line + VISIBLE_LINES_IN_TAB - LINE_SCROLL_PADDING);
            view->offset.line += lines_over_padding * 2;
            view->cursor.line -= lines_over_padding;
        }
        else
            view->cursor.line += line_to_go_down;
    }
    else
        center_on_line(view, absolute_line);
}

typedef enum
{
    Horizontal_Movement_none,
    Horizontal_Movement_left,
    Horizontal_Movement_right
} Horizontal_Movement;

// TODO: Detect needed movement when resizing tabs.
static Horizontal_Movement find_horizontal_movement(View* view, uint16_t absolute_column)
{
    if (view->offset.column + view->cursor.column > absolute_column)
        return Horizontal_Movement_left;
    else if (view->offset.column + view->cursor.column < absolute_column)
        return Horizontal_Movement_right;
    else
        return Horizontal_Movement_none;
}

static void center_on_column(View* view, uint16_t visible_columns, uint16_t absolute_column)
{
    view->offset.column = absolute_column - visible_columns / 2;
    view->cursor.column = visible_columns / 2;
}

static void go_left(View* view, Rectangle view_rectangle, uint16_t absolute_column)
{
    uint16_t visible_columns = view_rectangle.width / FONT_WIDTH;
    uint16_t columns_to_move = view->offset.column + view->cursor.column - absolute_column;

    if (columns_to_move > view->cursor.column && absolute_column < visible_columns)
    {
        view->offset.column = 0;
        view->cursor.column = absolute_column;
    }
    else if (view->offset.column <= absolute_column && view->offset.column + visible_columns > absolute_column)
        view->cursor.column = absolute_column - view->offset.column;
    else 
        center_on_column(view, visible_columns, absolute_column);
}

static void go_right(View* view, Rectangle view_rectangle, uint16_t absolute_column)
{
    uint16_t visible_columns = view_rectangle.width / FONT_WIDTH;
    if (absolute_column < visible_columns)
    {
        view->offset.column = 0;
        view->cursor.column = absolute_column;
    }
    else if (view->offset.column <= absolute_column && view->offset.column + visible_columns > absolute_column)
        view->cursor.column = absolute_column - view->offset.column;
    else
        center_on_column(view, visible_columns, absolute_column);
}

bool go_to(View* view, Rectangle view_rectangle, bool use_preferred_column, Location absolute_location)
{
    Buffer* buffer = lookup_buffer(view->buffer);
    if (buffer->lines_length < absolute_location.line)
        absolute_location.line = buffer->lines_length - 1;

    Line* line = &buffer->lines[absolute_location.line];
    if (line->characters_length < absolute_location.column)
        absolute_location.column = line->characters_length;

    Vertical_Movement vertical_movement = find_vertical_movement(view, absolute_location.line);
    switch (vertical_movement)
    {
        case Vertical_Movement_none:
            break;

        case Vertical_Movement_up:
            go_up(view, absolute_location.line);
            break;
        
        case Vertical_Movement_down:
            go_down(view, absolute_location.line);
            break;
    }

    Horizontal_Movement horizontal_movement = Horizontal_Movement_none;
    if (use_preferred_column)
    {
        if (vertical_movement)
        {
            uint16_t column = line->characters_length < view->prefered_column ? line->characters_length : view->prefered_column;
            horizontal_movement = find_horizontal_movement(view, column);

            switch (horizontal_movement)
            {
                case Horizontal_Movement_none:
                    break;

                case Horizontal_Movement_left:
                    go_left(view, view_rectangle, column);
                    break;

                case Horizontal_Movement_right:
                    go_right(view, view_rectangle, column);
                    break;
            }
        }
    }
    else
    {
        horizontal_movement = find_horizontal_movement(view, absolute_location.column);
        switch (horizontal_movement)
        {
            case Horizontal_Movement_none:
                break;

            case Horizontal_Movement_left:
                go_left(view, view_rectangle, absolute_location.column);
                break;

            case Horizontal_Movement_right:
                go_right(view, view_rectangle, absolute_location.column);
                break;
        }

        view->prefered_column = view->offset.column + view->cursor.column;
    }

    return vertical_movement || horizontal_movement;
}

void move_active_view_to_the_left(void)
{

    if (editor.tabs_length < 2 || editor.active_tab_index == 0)
        return;

    // Draw the rest of the owl.
    editor.refresh_needed = true;
}

void move_active_view_to_the_right(void)
{
    if (!editor.tabs_length)
        return;

    Tab* tab = &editor.tabs[editor.active_tab_index];
    if (tab->active_view_index == tab->views_length - 1)
        return;

    editor.refresh_needed = true;
}

void move_active_tab_to_the_left(void)
{
    if (editor.tabs_length < 2 || editor.active_tab_index == 0)
        return;

    Tab temp = editor.tabs[editor.active_tab_index - 1];
    editor.tabs[editor.active_tab_index - 1] = editor.tabs[editor.active_tab_index];
    editor.tabs[editor.active_tab_index] = temp;
    measure_tabs();

    set_editor_active_tab(editor.active_tab_index - 1);
    editor.refresh_needed = true;
}

void move_active_tab_to_the_right(void)
{
    if (editor.tabs_length < 2 || editor.active_tab_index == editor.tabs_length - 1)
        return;

    Tab temp = editor.tabs[editor.active_tab_index + 1];
    editor.tabs[editor.active_tab_index + 1] = editor.tabs[editor.active_tab_index];
    editor.tabs[editor.active_tab_index] = temp;
    measure_tabs();

    set_editor_active_tab(editor.active_tab_index + 1);
    editor.refresh_needed = true;
}

void put_active_view_in_new_tab_to_the_left(void)
{
    if (!editor.tabs_length)
        return;
    
    if (editor.tabs[editor.active_tab_index].views_length == 1)
        return;

    uint8_t new_tab_index = insert_tab_before_active_tab();
    Tab* restrict new_tab = &editor.tabs[new_tab_index];

    Tab* restrict active_tab = &editor.tabs[++editor.active_tab_index];

    uint8_t view_index_in_new_tab = add_view(new_tab_index);
    new_tab->views[view_index_in_new_tab] = active_tab->views[active_tab->active_view_index];

    remove_view(editor.active_tab_index, active_tab->active_view_index);
    set_editor_active_tab(active_tab->active_view_index);

    measure_tabs();
    editor.refresh_needed = true;
}

void put_active_view_in_new_tab_to_the_right(void)
{
    if (!editor.tabs_length)
        return;

    Tab* restrict active_tab = &editor.tabs[editor.active_tab_index];
    if (active_tab->views_length == 1)
        return;

    uint8_t new_tab_index = insert_tab_after_active_tab();
    Tab* restrict new_tab = &editor.tabs[new_tab_index];

    uint8_t view_index_in_new_tab = add_view(new_tab_index);
    new_tab->views[view_index_in_new_tab] = active_tab->views[active_tab->active_view_index];

    remove_view(editor.active_tab_index, active_tab->active_view_index);
    if (active_tab->active_view_index)
        active_tab->active_view_index--;

    set_editor_active_tab(active_tab->active_view_index);

    measure_tabs();
    editor.refresh_needed = true;
}

void put_active_view_in_tab_to_the_left(void)
{
    if (editor.tabs_length < 2 || editor.active_tab_index == editor.tabs_length - 1)
        return;

    editor.refresh_needed = true;
}

void put_active_view_in_tab_to_the_right(void)
{
    if (editor.tabs_length < 2 || editor.active_tab_index == 0)
        return;

    editor.refresh_needed = true;
}

void activate_left_hand_side_view(void)
{
    if (!editor.tabs_length)
        return;

    Tab* active_tab = &editor.tabs[editor.active_tab_index];
    if (!active_tab->active_view_index)
        return;

    set_editor_title_to_buffer_path(active_tab->views[--active_tab->active_view_index].buffer);
    editor.refresh_needed = true;
}

void activate_right_hand_side_view(void)
{
    if (!editor.tabs_length)
        return;

    Tab* active_tab = &editor.tabs[editor.active_tab_index];
    if (active_tab->active_view_index == active_tab->views_length - 1)
        return;

    set_editor_title_to_buffer_path(active_tab->views[++active_tab->active_view_index].buffer);
    editor.refresh_needed = true;
}

void activate_left_hand_side_tab(void)
{
    if (editor.tabs_length < 2 || editor.active_tab_index == 0)
        return;

    set_editor_active_tab(--editor.active_tab_index);
    editor.refresh_needed = true;
}

void activate_right_hand_side_tab(void)
{
    if (editor.tabs_length < 2 || editor.active_tab_index == editor.tabs_length - 1)
        return;

    set_editor_active_tab(++editor.active_tab_index);
    editor.refresh_needed = true;
}

void close_active_tab(void)
{
    // TODO: Check that all buffers that has a single reference has 
    // no unsaved changes.
}
