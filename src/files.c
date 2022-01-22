#include "moss.h"
#include "SDL.h"

#include <stdio.h>

char* read_file(char* const path, size_t* const characters_length)
{
    SDL_RWops* file = SDL_RWFromFile(path, "rb");
    if (!file)
        // Error reading file.
        return NULL;

    SDL_RWseek(file, 0, SEEK_END);
    *characters_length = SDL_RWtell(file);
    SDL_RWseek(file, 0, RW_SEEK_SET);

    char* characters = malloc(*characters_length);
    SDL_RWread(file, characters, 1, *characters_length);
    SDL_RWclose(file);

    return characters;
}
