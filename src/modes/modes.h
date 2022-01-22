#pragma once
#include "../buffers.h"
#include "../tabs.h"
#include "../modes.h"

#include "SDL.h"

// NORMAL mode.
void move_cursor_up_in_active_view(void);
void move_cursor_down_in_active_view(void);
void move_cursor_left_in_active_view(void);
void move_cursor_right_in_active_view(void);

void move_cursor_to_last_line_in_active_view(void);

void move_cursor_to_the_first_non_space_in_the_line_in_active_view(void);
void move_cursor_to_the_end_of_the_line_in_active_view(void);

// INSERT mode.
void enter_insert_mode(void);
void exit_insert_mode(void);

// COMMAND mode.
void enter_command_mode(void);
void exit_command_mode(void);

void insert_character_into_command(char character);

void execute_command(void);
void clear_command(void);
void delete_current_character_in_command(void);
void delete_previous_character_in_command(void);

// VISUAL mode.
void enter_visual_mode(void);
void exit_visual_mode(void);

// TAB mode.
void enter_tab_mode(void);
void exit_tab_mode(void);

void move_active_view_to_the_left(void);
void move_active_view_to_the_right(void);
void move_active_tab_to_the_left(void);
void move_active_tab_to_the_right(void);

void put_active_view_in_new_tab_to_the_left(void);
void put_active_view_in_new_tab_to_the_right(void);
void put_active_view_in_tab_to_the_left(void);
void put_active_view_in_tab_to_the_right(void);

void activate_left_hand_side_view(void);
void activate_right_hand_side_view(void);

void activate_left_hand_side_tab(void);
void activate_right_hand_side_tab(void);

void close_active_tab(void);
