#pragma once

#include "SDL.h"
#include <stdint.h>

void input_code_point_in_current_mode(uint32_t code_point);
void key_down_in_current_mode(const SDL_KeyboardEvent* keyboard);
