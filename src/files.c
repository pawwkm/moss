#include "moss.h"
#include "SDL.h"

#include <stdio.h>

char* read_file(char* const path, size_t* const characters_length)
{
    SDL_RWops* file = SDL_RWFromFile(path, "rb");
    if (!file)
    {
        SDL_Log("Could not open %s: %s\n", path, SDL_GetError());
        return NULL;
    }

    if (SDL_RWseek(file, 0, SEEK_END) == -1)
    {
        SDL_Log("Could not seek in %s\n", path);
        return NULL;
    }

    *characters_length = SDL_RWtell(file);
    if (SDL_RWseek(file, 0, RW_SEEK_SET) == -1)
    {
        SDL_Log("Could not seek in %s\n", path);
        return NULL;
    }

    char* characters = malloc(*characters_length);
    if (!characters)
    {
        SDL_Log("Could not allocate characters for %s\n", path);
        return NULL;
    }

    size_t characters_read = SDL_RWread(file, characters, 1, *characters_length);
    if (characters_read != *characters_length)
    {
        SDL_Log("Read %u characters out of %u from %s\n", characters_read, *characters_length, path, SDL_GetError());
        free(characters);

        if (SDL_RWclose(file) < 0)
            SDL_Log("Could not close %s: %s\n", path, SDL_GetError());

        return NULL;
    }

    if (SDL_RWclose(file) < 0)
        SDL_Log("Could not close %s: %s\n", path, SDL_GetError());

    return characters;
}
