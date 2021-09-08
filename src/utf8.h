#pragma once

#include <stdint.h>

uint32_t* utf8_string_to_code_points(const uint8_t* string, size_t string_length, size_t* code_points_length);
uint32_t utf8_to_code_point(const uint8_t* string);
void code_point_to_utf8(uint32_t code_point, char (*buffer)[4], uint8_t* sequence_length);
