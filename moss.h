#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    Font_Color_plain = 0,
    Font_Color_comment = 1,
    Font_Color_keyword = 2,
    Font_Color_string = 3,
    Font_Color_type = 4,
    Font_Color_inactive_tab_header = 5,
    Font_Color_active_tab_header = Font_Color_plain,
    Font_Color_mode = 6,
    Font_Color_selected = 7,
    Font_Color_error = 8
} Font_Color;

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

#define CHARACTERS_IN_FONT 94
typedef struct
{
    uint8_t* bitmap;
    uint8_t width;
    uint8_t height;
} Font;

typedef enum
{
    Mode_normal,
    Mode_insert,
    Mode_visual,
    Mode_tab
} Mode;

typedef enum
{
    Language_none,
    Language_c17,
    Language_owen
} Language;

typedef enum
{
    Token_Tag_plain,
    Token_Tag_comment,
    Token_Tag_keyword,
    Token_Tag_string,
    Token_Tag_type,
    Token_Tag_white_space
} Token_Tag;

typedef struct
{
    Token_Tag tag;
    char* characters;
    uint16_t characters_length;
} Token;

typedef struct
{
    char* characters;
    uint16_t characters_length;
    uint16_t characters_capacity;

    Token* tokens;
    uint16_t tokens_length;
    uint16_t tokens_capacity;
} Line;

typedef struct
{
    uint16_t line;
    uint16_t column;
} Location;

typedef struct
{
    uint8_t index;
} Buffer_Handle;

typedef enum
{
    Change_Tag_break,
    Change_Tag_insert_character,
    Change_Tag_remove_character,
    Change_Tag_insert_line,
    Change_Tag_remove_line,
    Change_Tag_merge_line,
    Change_Tag_split_line
} Change_Tag;

typedef struct
{
    Change_Tag tag;
    union
    {
        Location cursor;
        char character;
    };
} Change;

typedef struct
{
    char* path;
    uint16_t path_length;

    Line* lines;
    uint16_t lines_length;
    uint16_t lines_capacity;

    char* file_name;
    uint16_t file_name_length;

    Change* changes;
    uint32_t changes_length;
    uint32_t changes_capacity;
    uint32_t last_flushed_change;
    uint32_t current_change;

    Language language;

    uint8_t references;
} Buffer;

typedef struct
{
    Buffer_Handle buffer;
    Location offset;
    Location cursor;

    uint16_t prefered_column;
} View;

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} Block;

typedef struct
{
    View* views;
    uint8_t views_length;
    uint8_t views_capacity;
    uint8_t active_view_index;
    Block block;
} Tab;

typedef struct
{
    uint8_t font_width;
    uint8_t font_height;
    uint8_t spaces_per_tab;
    uint8_t tab_separator_width;
    uint8_t tab_header_height;
    uint8_t status_line_height;
    uint8_t line_scroll_padding;
        
    Mode mode;
    uint16_t width;
    uint16_t height;

    Tab* tabs;
    uint8_t tabs_length;
    uint8_t tabs_capacity;
    uint8_t active_tab_index;

    char* command;
    uint16_t command_length;
    uint16_t command_capacity;

    // You get a rectangle encompassing any invalidated
    // rectangles in a single WM_PAINT message. That is a
    // waste of time in cases where the cursor moves 
    // because that rectangle and the line:col rectangle 
    // are both invalidated. Another case is invalidating 
    // the tab headers and the status line. That causes the
    // entire window to be invalidated!
    //
    // So lets keep track of that ourselves.
    //
    // It is assumed that blocks are not contained by or
    // overlaps with any other block.
    Block* invalidated;
    uint8_t invalidated_length;
    uint8_t invalidated_capacity;
} Editor;

Editor initialize_editor(void);
void uninitialize_editor(Editor* editor);
bool initialize_font(const Font* font);
void render_editor(Editor* editor);

void resize_editor(Editor* editor, uint16_t width, uint16_t height);
void interpret_character(Editor* editor, char character, bool ctrl);
void open_buffer_in_active_tab(Editor* editor, char* path, uint16_t path_length);

// Platform specific functions.
void render_block(Color color, Block block);
void render_character(uint16_t source_x, uint16_t source_y, uint16_t destination_x, uint16_t destination_y, uint16_t width, uint16_t height);

void set_editor_title(char* path, size_t path_length);
bool allocate_font_sheet(uint16_t width, uint16_t height);
bool set_font_sheet_pixel(Color color, uint16_t x, uint16_t y);

double time_in_us(void);
char* read_file(char* path, size_t* characters_length);

#include "configuration.h"
