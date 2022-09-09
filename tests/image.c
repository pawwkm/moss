#include "tests.h"
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

Image read_ppm_p6(FILE* file)
{
    uint16_t width;
    uint16_t height; 
    fscanf(file, "P6\n%" SCNu16 "\n%" SCNu16 "\n255\n", &width, &height);
    
    Image image = allocate_image(width, height);
    if (!image.pixels)
        return image;
    
    for (uint32_t i = 0; i < (uint32_t)image.width * (uint32_t)image.height; i++)
    {
        image.pixels[i] = (Color)
        {
            .r = (uint8_t)fgetc(file),
            .g = (uint8_t)fgetc(file),
            .b = (uint8_t)fgetc(file)
        };
    }
    
    return image;
}

void write_ppm_p6(const Image* image, FILE* file)
{
    fprintf(file, "P6\n%" PRIu16 "\n%" PRIu16 "\n255\n", image->width, image->height);
    for (uint32_t i = 0; i < (uint32_t)image->width * (uint32_t)image->height; i++)
    {
        Color pixel = image->pixels[i];
        fputc(pixel.r, file);
        fputc(pixel.g, file);
        fputc(pixel.b, file);
    }
}

static bool compare_colors(Color a, Color b)
{
    return a.r == b.r &&
           a.g == b.g &&
           a.b == b.b;
}

bool compare_image_sizes(const Image* a, const Image* b)
{
    return a->width  == b->width && 
           a->height == b->height;
}

bool compare_images(const Image* a, const Image* b)
{
    if (!compare_image_sizes(a, b))
        return false;
    
    for (uint32_t i = 0; i < (uint32_t)a->width * (uint32_t)a->height; i++)
    {
        if (!compare_colors(a->pixels[i], b->pixels[i]))
            return false;
    }
    
    return true;
}

Image diff_images(const Image* a, const Image* b, Color diff_color)
{
    // NOTE: If I need to diff images of different sizes I could
    // create a new image with the biggest width and height. 
    // The pixels within both images are diffed as normal but
    // but the remaining width and height are colored with a 
    // color identifying which of the 2 images is of in the 
    // respective dimension.
    assert(compare_image_sizes(a, b));
    
    Image diff = allocate_image(a->width, a->height);
    for (uint32_t i = 0; i < (uint32_t)a->width * (uint32_t)a->height; i++)
    {
        Color a_pixel = a->pixels[i];
        Color b_pixel = b->pixels[i];
        
        diff.pixels[i] = compare_colors(a_pixel, b_pixel) ? a_pixel : diff_color;
    }
    
    return diff;
}

Image allocate_image(uint16_t width, uint16_t height)
{
    uint32_t pixels_length = (uint32_t)width * (uint32_t)height;
    Color* pixels = malloc(pixels_length * sizeof(pixels[0]));
    if (pixels)
    {
        memset(pixels, 0, pixels_length * sizeof(pixels[0]));
        return (Image)
        {
            .width = width,
            .height = height,
            .pixels = pixels
        };
    }
    else
        return (Image) { 0 };
}

void free_image(Image* image)
{
    free(image->pixels);
    *image = (Image) { 0 };
}

