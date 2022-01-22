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
                delete_current_character_in_command();
            else if (keyboard->keysym.sym == SDLK_BACKSPACE)
                delete_previous_character_in_command();
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

void input_character_in_current_mode(const char character)
{
    switch (editor.mode)
    {
        case Mode_normal:
            if (character == ':')
                enter_command_mode();
            else if (character == 'i')
                enter_insert_mode();
            else if (character == 'I')
            {
                move_cursor_to_the_first_non_space_in_the_line_in_active_view();
                enter_insert_mode();
            }
            else if (character == 'a')
            {
                move_cursor_right_in_active_view();
                enter_insert_mode();
            }
            else if (character == 'A')
            {
                move_cursor_to_the_end_of_the_line_in_active_view();
                enter_insert_mode();
            }
            else if (character == 'v')
                enter_visual_mode();
            else if (character == 'h')
                move_cursor_left_in_active_view();
            else if (character == 'j')
                move_cursor_down_in_active_view();
            else if (character == 'k')
                move_cursor_up_in_active_view();
            else if (character == 'l')
                move_cursor_right_in_active_view();
            else if (character == 'G')
                move_cursor_to_last_line_in_active_view();
            else if (character == '^')
                move_cursor_to_the_first_non_space_in_the_line_in_active_view();
            else if (character == '$')
                move_cursor_to_the_end_of_the_line_in_active_view();

            break;

        case Mode_command:
            insert_character_into_command(character);
            break;

        case Mode_insert:
            break;

        case Mode_visual:
            break;

        case Mode_tab:
            if (character == 'h')
                move_active_view_to_the_left();
            else if (character == 'j')
                move_active_view_to_the_right();
            else if (character == 'H')
                move_active_tab_to_the_left();
            else if (character == 'J')
                move_active_tab_to_the_right();
            else if (character == 'k')
                put_active_view_in_new_tab_to_the_left();
            else if (character == 'l')
                put_active_view_in_new_tab_to_the_right();
            else if (character == 'K')
                put_active_view_in_tab_to_the_left();
            else if (character == 'L')
                put_active_view_in_tab_to_the_right();
            else if (character == 'f')
                activate_left_hand_side_view();
            else if (character == 'g')
                activate_right_hand_side_view();
            else if (character == 'd')
                activate_right_hand_side_tab();
            else if (character == 's')
                activate_left_hand_side_tab();
            else if (character == 'C')
                close_active_tab();

            break;
    }
}
