#pragma once

#include "moss.h"

void open_buffer_in_active_tab(char* path, uint16_t path_length);

uint8_t insert_tab_before_active_tab(void);
uint8_t insert_tab_after_active_tab(void);

uint8_t add_view(uint8_t tab_index);
void remove_view(uint8_t tab_index, uint8_t view_index);

void set_editor_active_tab(uint8_t tab_index);

void measure_tabs(void);

void set_editor_title_to_buffer_path(Buffer_Handle handle);

View* find_active_editor_view(void);
View* find_active_tab_view(const Tab* tab);

bool go_to(View* view, Rectangle view_rectangle, bool use_preferred_column, Location absolute_location);
