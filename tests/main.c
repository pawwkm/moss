#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

double time_in_us(void)
{
    return 0;
}

static Image surface;
static Image font_sheet;
void render_block(Color color, Block block)
{
    for (uint32_t y = block.y; y < (uint32_t)block.y + (uint32_t)block.height && y <= surface.height; y++)
    {
        for (uint32_t x = block.x; x < (uint32_t)block.x + (uint32_t)block.width && x <= surface.width; x++)
            surface.pixels[y * surface.width + x] = color;
    }
}

void render_character(uint16_t source_x, uint16_t source_y, uint16_t destination_x, uint16_t destination_y, uint16_t width, uint16_t height)
{
    for (uint16_t y = 0; y < height; y++)
    {
        for (uint16_t x = 0; x < width; x++)
            surface.pixels[(y + destination_y) * surface.width + x + destination_x] = font_sheet.pixels[(y + source_y) * font_sheet.width + x + source_x];
    }
}

void set_editor_title(char* path, size_t path_length)
{
    (void)path;
    (void)path_length;
}

bool allocate_font_sheet(uint16_t width, uint16_t height)
{
    font_sheet = allocate_image(width, height);

    return font_sheet.pixels;
}

bool set_font_sheet_pixel(Color color, uint16_t x, uint16_t y)
{
    if (x > font_sheet.width || y > font_sheet.height)
        return false;

    font_sheet.pixels[y * font_sheet.width + x] = color;

    return true;
}

typedef struct
{
    const char* path;
    char* contents;
} File_Entry;

typedef struct
{
    uint8_t files_length;
    uint8_t files_capacity;
    File_Entry* files;
} Virtual_File_System;

static Virtual_File_System vfs;
static void add_file_to_vfs(const char* path, const char* contents)
{
    if (vfs.files_capacity == vfs.files_length)
    {
        vfs.files_capacity = vfs.files_capacity ? vfs.files_capacity * 2 : 4;
        if (!vfs.files_capacity)
            assert(false && "Out of memory.");

        vfs.files = realloc(vfs.files, sizeof(vfs.files[0]) * vfs.files_capacity);
        memset(vfs.files + vfs.files_length, 0, sizeof(vfs.files[0]) * (vfs.files_capacity - vfs.files_length));
    }

    File_Entry entry;
    entry.path = path;
    entry.contents = malloc(strlen(contents) + 1);
    
    strcpy(entry.contents, contents);

    vfs.files[vfs.files_length++] = entry;
}

static void clear_files(void)
{
    for (uint8_t i = 0; i < vfs.files_length; i++)
        free(vfs.files[i].contents);

    memset(vfs.files, 0, sizeof(vfs.files[0]) * vfs.files_length);
    vfs.files_length = 0;
}

char* read_file(char* const path, size_t* characters_length)
{
    File_Entry* entry = NULL;
    for (uint8_t i = 0; i < vfs.files_length; i++)
    {
        if (!strcmp(vfs.files[i].path, path))
        {
            entry = &vfs.files[i];
            break;
        }
    }

    if (!entry)
        return NULL;

    *characters_length = strlen(entry->contents);
    char* contents = malloc(*characters_length + 1);

    strcpy(contents, entry->contents);

    return contents;
}

#define ASSERT_OPERATOR_TYPE(EXPECTED, OPERATOR, ACTUAL, TYPE, TYPE_FORMAT)                                                                                                                     \
do                                                                                                                                                                                              \
{                                                                                                                                                                                               \
    TYPE _e = EXPECTED;                                                                                                                                                                         \
    TYPE _a = ACTUAL;                                                                                                                                                                           \
    if (!(_e OPERATOR _a))                                                                                                                                                                      \
    {                                                                                                                                                                                           \
        fprintf(stderr, "ERROR> %s:%u: %s: Assertion `" #EXPECTED " " #OPERATOR " " #ACTUAL "` (%" TYPE_FORMAT " " #OPERATOR " %" TYPE_FORMAT ")\n", __FILE__, __LINE__, __FUNCTION__, _e, _a); \
        return false;                                                                                                                                                                           \
    }                                                                                                                                                                                           \
} while (false)

#define ASSERT_EQUAL_U8(EXPECTED, ACTUAL)  ASSERT_OPERATOR_TYPE(EXPECTED, ==, ACTUAL, uint8_t,  PRIu8)
#define ASSERT_EQUAL_U16(EXPECTED, ACTUAL) ASSERT_OPERATOR_TYPE(EXPECTED, ==, ACTUAL, uint16_t, PRIu16)
#define ASSERT_EQUAL_U32(EXPECTED, ACTUAL) ASSERT_OPERATOR_TYPE(EXPECTED, ==, ACTUAL, uint32_t, PRIu32)
#define ASSERT_EQUAL_U64(EXPECTED, ACTUAL) ASSERT_OPERATOR_TYPE(EXPECTED, ==, ACTUAL, uint64_t, PRIu64)
#define ASSERT_EQUAL_I8(EXPECTED, ACTUAL)  ASSERT_OPERATOR_TYPE(EXPECTED, ==, ACTUAL, int8_t,   PRIi8)
#define ASSERT_EQUAL_I16(EXPECTED, ACTUAL) ASSERT_OPERATOR_TYPE(EXPECTED, ==, ACTUAL, int16_t,  PRIi16)
#define ASSERT_EQUAL_I32(EXPECTED, ACTUAL) ASSERT_OPERATOR_TYPE(EXPECTED, ==, ACTUAL, int32_t,  PRIi32)
#define ASSERT_EQUAL_I64(EXPECTED, ACTUAL) ASSERT_OPERATOR_TYPE(EXPECTED, ==, ACTUAL, int64_t,  PRIi64)

#define ASSERT_NULL(ACTUAL) ASSERT_OPERATOR_TYPE(NULL, ==, ACTUAL, void*, "p")

bool editor_without_tabs(const Editor* editor)
{
    ASSERT_EQUAL_U8(0, editor->active_tab_index);
    ASSERT_EQUAL_U8(0, editor->tabs_length);
    ASSERT_EQUAL_U8(0, editor->tabs_capacity);
    ASSERT_NULL(editor->tabs);

    ASSERT_EQUAL_U8((uint8_t)Mode_normal, (uint8_t)editor->mode);
    
    ASSERT_EQUAL_U16(0, editor->command_length);
    ASSERT_EQUAL_U16(0, editor->command_capacity);
    ASSERT_NULL(editor->command);

    return true;
}

static void interpret_string(Editor* editor, const char* string)
{
    while (*string)
    {
        interpret_character(editor, *string++, false);
        render_editor(editor);
    }
}

static bool known_bug(Editor* editor)
{
    add_file_to_vfs("test.txt", "abc\ndef");
    open_buffer_in_active_tab(editor, "test.txt", 8);

    interpret_string(editor, "lj");

    const View* view = &editor->tabs[0].views[0];
    
    ASSERT_EQUAL_U16(0, view->offset.line);
    ASSERT_EQUAL_U16(0, view->offset.column);

    ASSERT_EQUAL_U16(1, view->cursor.line);
    ASSERT_EQUAL_U16(1, view->cursor.column);

    return true;
}

#define TEST_FROM_FUNCTION_NAME(FUNCTION)                          \
{                                                                  \
    .function = FUNCTION,                                          \
    .expected_path = "../tests/images/" #FUNCTION "_expected.ppm", \
    .actual_path =   "../tests/images/" #FUNCTION "_actual.ppm",   \
    .diff_path =     "../tests/images/" #FUNCTION "_diff.ppm"      \
}

static const Test tests[] = 
{
    TEST_FROM_FUNCTION_NAME(editor_without_tabs),
    TEST_FROM_FUNCTION_NAME(known_bug)
};

static void resize_editor_and_surface(Editor* editor, uint16_t width, uint16_t height)
{
    if (surface.pixels && (surface.width != width || surface.height != height))
        assert(false && "Copy pixels from the old surface to the new one.");
    else
        surface = allocate_image(width, height);

    resize_editor(editor, editor->width, editor->height);
    render_editor(editor);
}

int main(void) 
{
    int exit_code = EXIT_SUCCESS;
    if (initialize_font(&cozette))
    {
        FILE* file = fopen("../tests/images/font_sheet.ppm", "wb");
        write_ppm_p6(&font_sheet, file);
        fclose(file);
    }
    else
    {
        fprintf(stderr, "Could not initialize font!");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        Editor editor = initialize_editor();
        resize_editor_and_surface(&editor, editor.width, editor.height);

        Test test = tests[i];
        if (test.function(&editor))
        {
            FILE* file = fopen(test.expected_path, "rb"); 
            if (!file)
            {
                file = fopen(test.actual_path, "wb");
                write_ppm_p6(&surface, file);
                fclose(file);
             
                fprintf(stderr, "Could not open %s.", test.expected_path);
                exit_code = EXIT_FAILURE;
                continue;
            }
            
            Image expected = read_ppm_p6(file);
            fclose(file);
            
            if (!compare_images(&surface, &expected))
            {
                file = fopen(test.actual_path, "wb");
                write_ppm_p6(&surface, file);
                fclose(file);
                
                Image diff = diff_images(&surface, &expected, (Color) { 255, 0, 255 });
                
                file = fopen(test.diff_path, "wb");
                write_ppm_p6(&diff, file);
                fclose(file);

                free_image(&diff);
            }
            
            free_image(&expected);
        }
        else
            exit_code = EXIT_FAILURE;

        uninitialize_editor(&editor);
        clear_files();

        if (surface.pixels)
            render_block((Color) { 0 }, (Block) { 0, 0, surface.width, surface.height });
    }

    return exit_code;
}