#include "modes.h"

void key_down_in_current_mode(const SDL_KeyboardEvent* const keyboard)
{
    switch (editor.mode)
    {
        case Mode_normal:
            if (keyboard->keysym.sym == SDLK_TAB)
                enter_tab_mode();

            break;

        case Mode_command:
            if (keyboard->keysym.sym == SDLK_ESCAPE)
                exit_command_mode();
            else if (keyboard->keysym.sym == SDLK_DELETE)
                delete_current_code_point_in_command();
            else if (keyboard->keysym.sym == SDLK_BACKSPACE)
                delete_previous_code_point_in_command();
            else if (keyboard->keysym.sym == SDLK_RETURN)
                execute_command();
            break;

        case Mode_insert:
            if (keyboard->keysym.sym == SDLK_ESCAPE)
                exit_insert_mode();

            break;

        case Mode_visual:
            if (keyboard->keysym.sym == SDLK_ESCAPE)
                exit_visual_mode();

            break;

        case Mode_tab:
            if (keyboard->keysym.sym == SDLK_ESCAPE)
                exit_tab_mode();

            break;
    }
}

void input_code_point_in_current_mode(const uint32_t code_point)
{
    switch (editor.mode)
    {
        case Mode_normal:
            if (code_point == L':')
                enter_command_mode();
            else if (code_point == 'i')
                enter_insert_mode();
            else if (code_point == 'I')
            {
                move_cursor_to_the_first_non_space_in_the_line_in_active_view();
                enter_insert_mode();
            }
            else if (code_point == 'a')
            {
                move_cursor_right_in_active_view();
                enter_insert_mode();
            }
            else if (code_point == 'A')
            {
                move_cursor_to_the_end_of_the_line_in_active_view();
                enter_insert_mode();
            }
            else if (code_point == 'v')
                enter_visual_mode();
            else if (code_point == 'h')
                move_cursor_left_in_active_view();
            else if (code_point == 'j')
                move_cursor_down_in_active_view();
            else if (code_point == 'k')
                move_cursor_up_in_active_view();
            else if (code_point == 'l')
                move_cursor_right_in_active_view();
            else if (code_point == 'G')
                move_cursor_to_last_line_in_active_view();
            else if (code_point == '^')
                move_cursor_to_the_first_non_space_in_the_line_in_active_view();
            else if (code_point == '$')
                move_cursor_to_the_end_of_the_line_in_active_view();

            break;

        case Mode_command:
            insert_code_point_into_command(code_point);
            break;

        case Mode_insert:
            break;

        case Mode_visual:
            break;

        case Mode_tab:
            if (code_point == L'h')
                move_active_view_to_the_left();
            else if (code_point == L'j')
                move_active_view_to_the_right();
            else if (code_point == L'H')
                move_active_tab_to_the_left();
            else if (code_point == L'J')
                move_active_tab_to_the_right();
            else if (code_point == L'k')
                put_active_view_in_new_tab_to_the_left();
            else if (code_point == L'l')
                put_active_view_in_new_tab_to_the_right();
            else if (code_point == L'K')
                put_active_view_in_tab_to_the_left();
            else if (code_point == L'L')
                put_active_view_in_tab_to_the_right();
            else if (code_point == L'f')
                activate_left_hand_side_view();
            else if (code_point == L'g')
                activate_right_hand_side_view();
            else if (code_point == L'd')
                activate_right_hand_side_tab();
            else if (code_point == L's')
                activate_left_hand_side_tab();
            else if (code_point == L'C')
                close_active_tab();

            break;
    }
}
