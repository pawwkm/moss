#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FONT_WIDTH 6
#define FONT_HEIGHT 13
#define SPACES_PER_TAB 4
#define TAB_SEPARATOR_WIDTH FONT_WIDTH
#define TAB_HEADER_HEIGHT (FONT_HEIGHT * 2)
#define STATUS_LINE_HEIGHT FONT_HEIGHT
#define LINE_SCROLL_PADDING 5
#define VISIBLE_LINES_IN_TAB ((editor.height - TAB_HEADER_HEIGHT - STATUS_LINE_HEIGHT) / FONT_HEIGHT)

typedef enum
{
    Mode_normal,
    Mode_command,
    Mode_insert,
    Mode_visual,
    Mode_tab
} Mode;

typedef enum
{
    Language_none,
    Language_c,
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
    Change_Tag_insert_characters,
    Change_Tag_remove_characters,
    Change_Tag_insert_line,
    Change_Tag_remove_line
} Change_Tag;

typedef struct
{
    Change_Tag tag;
    Location cursor;
    char* characters;
    uint16_t characters_length;
    uint16_t characters_capacity;
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
    uint16_t changes_length;
    uint16_t changes_capacity;
    int32_t last_flushed_change;
    int32_t current_change;

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
} Point;

typedef struct
{
    Point position;
    uint16_t width;
    uint16_t height;
} Rectangle;

typedef struct
{
    View* views;
    uint8_t views_length;
    uint8_t views_capacity;
    uint8_t active_view_index;
    Rectangle rectangle;
} Tab;

typedef struct
{
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
    uint16_t command_cursor;

    bool refresh_needed;
} Editor;

extern Editor editor;

void set_editor_title(char* path, size_t path_length);

void show_message(const char* message);

void show_initialization_error_message(const char* message);
