#include "moss.h"
#include "utf8.h"
#include "SDL.h"

#include <stdio.h>

uint32_t* read_file(char* const path, size_t* const code_points_length)
{
    SDL_RWops* file = SDL_RWFromFile(path, "rb");
    if (!file)
        // Error reading file.
        return NULL;

    SDL_RWseek(file, 0, SEEK_END);
    size_t utf8_text_length = SDL_RWtell(file);
    SDL_RWseek(file, 0, RW_SEEK_SET);

    uint8_t* utf8_text = malloc(utf8_text_length + 1);
    SDL_RWread(file, utf8_text, 1, utf8_text_length);

    utf8_text[utf8_text_length] = '\0';
    SDL_RWclose(file);

    uint32_t* code_points = utf8_string_to_code_points(utf8_text, utf8_text_length, code_points_length);

    free(utf8_text);

    return code_points;
}
