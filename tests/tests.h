#include "../moss.h"
#include <stdio.h>

typedef struct
{
    uint16_t width;
    uint16_t height;

    // There are width x height pixels stored from top
    // left to bottom right.
    Color* pixels;
} Image;

Image read_ppm_p6(FILE* file);
void write_ppm_p6(const Image* image, FILE* file);

bool compare_image_sizes(const Image* a, const Image* b);
bool compare_images(const Image* a, const Image* b);
Image diff_images(const Image* a, const Image* b, Color diff_color);

Image allocate_image(uint16_t width, uint16_t height);
void free_image(Image* image);

typedef struct
{
    bool (*function)(Editor*);
    char* expected_path; 
    char* actual_path; 
    char* diff_path;
} Test;
