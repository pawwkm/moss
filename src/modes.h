#pragma once

#include "SDL.h"
#include <stdint.h>

void input_character_in_current_mode(char character);
void key_down_in_current_mode(const SDL_KeyboardEvent* keyboard);
