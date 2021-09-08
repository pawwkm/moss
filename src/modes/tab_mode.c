#include "modes.h"

void enter_tab_mode(void)
{
    if (!editor.tabs_length)
        return;

    SDL_Log("Mode_tab");
    editor.mode = Mode_tab;
    editor.refresh_needed = true;
}

void exit_tab_mode(void)
{
    SDL_Log("Mode_normal");
    editor.mode = Mode_normal;
    editor.refresh_needed = true;
}

void move_active_view_to_the_left(void)
{
    if (editor.tabs_length < 2 || editor.active_tab_index == 0)
        return;

    editor.refresh_needed = true;
}

void move_active_view_to_the_right(void)
{
    if (!editor.tabs_length)
        return;

    Tab* const tab = &editor.tabs[editor.active_tab_index];
    if (tab->active_view_index == tab->views_length - 1)
        return;

    editor.refresh_needed = true;
}

void move_active_tab_to_the_left(void)
{
    if (editor.tabs_length < 2 || editor.active_tab_index == 0)
        return;

    const Tab temp = editor.tabs[editor.active_tab_index - 1];
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

    const Tab temp = editor.tabs[editor.active_tab_index + 1];
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

    const uint8_t new_tab_index = insert_tab_before_active_tab();
    Tab* const restrict new_tab = &editor.tabs[new_tab_index];

    Tab* const restrict active_tab = &editor.tabs[++editor.active_tab_index];

    const uint8_t view_index_in_new_tab = add_view(new_tab_index);
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

    Tab* const restrict active_tab = &editor.tabs[editor.active_tab_index];
    if (active_tab->views_length == 1)
        return;

    const uint8_t new_tab_index = insert_tab_after_active_tab();
    Tab* const restrict new_tab = &editor.tabs[new_tab_index];

    const uint8_t view_index_in_new_tab = add_view(new_tab_index);
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

    Tab* const active_tab = &editor.tabs[editor.active_tab_index];
    if (!active_tab->active_view_index)
        return;

    set_editor_title_to_buffer_path(active_tab->views[--active_tab->active_view_index].buffer);
    editor.refresh_needed = true;
}

void activate_right_hand_side_view(void)
{
    if (!editor.tabs_length)
        return;

    Tab* const active_tab = &editor.tabs[editor.active_tab_index];
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
