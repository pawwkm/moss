#pragma once

#include "../moss.h"
#include <stdbool.h>
#include <stdint.h>

#define RESERVE_ELEMENTS(ELEMENTS, AMOUNT)                                                                        \
do                                                                                                                \
{                                                                                                                 \
    if (ELEMENTS##_capacity < (AMOUNT))                                                                           \
    {                                                                                                             \
        while (ELEMENTS##_capacity < (AMOUNT))                                                                    \
        {                                                                                                         \
            ELEMENTS##_capacity = ELEMENTS##_capacity ? ELEMENTS##_capacity * 2 : 4;                              \
            if (!ELEMENTS##_capacity)                                                                             \
                assert(false && "Out of memory.");                                                                \
        }                                                                                                         \
                                                                                                                  \
        ELEMENTS = realloc(ELEMENTS, sizeof(ELEMENTS[0]) * ELEMENTS##_capacity);                                  \
        memset(ELEMENTS + ELEMENTS##_length, 0, sizeof(ELEMENTS[0]) * (ELEMENTS##_capacity - ELEMENTS##_length)); \
    }                                                                                                             \
} while (false)

#define ADD_ELEMENT(ELEMENTS, ELEMENT)                 \
do                                                     \
{                                                      \
    RESERVE_ELEMENTS(ELEMENTS, ELEMENTS##_length + 1); \
    ELEMENTS[ELEMENTS##_length++] = ELEMENT;           \
} while (false)

#define INSERT_ELEMENTS(ELEMENTS, INDEX, AMOUNT, ES)                                                               \
do                                                                                                                 \
{                                                                                                                  \
    RESERVE_ELEMENTS(ELEMENTS, ELEMENTS##_length + (AMOUNT));                                                      \
    memmove(&ELEMENTS[(INDEX) + (AMOUNT)], &ELEMENTS[INDEX], sizeof(ELEMENTS[0]) * (ELEMENTS##_length - (INDEX))); \
    memcpy(&ELEMENTS[INDEX], ES, sizeof(ELEMENTS[0]) * (AMOUNT));                                                  \
    ELEMENTS##_length += AMOUNT;                                                                                   \
} while (false)

#define REMOVE_ELEMENTS(ELEMENTS, INDEX, AMOUNT)                                                                                \
do                                                                                                                              \
{                                                                                                                               \
    memmove(&ELEMENTS[INDEX], &ELEMENTS[(INDEX) + (AMOUNT)], sizeof(ELEMENTS[0]) * (ELEMENTS##_length - ((INDEX) + (AMOUNT)))); \
    ELEMENTS##_length -= AMOUNT;                                                                                                \
} while (false)

#define REMOVE_ELEMENTS_WITH_DEALLOCATOR(ELEMENTS, INDEX, AMOUNT, DEALLOCATOR) \
do                                                                             \
{                                                                              \
    for (size_t i = INDEX; i < AMOUNT; i++)                                    \
        DEALLOCATOR(&ELEMENTS[i]);                                             \
                                                                               \
    REMOVE_ELEMENTS(ELEMENTS, INDEX, AMOUNT);                                  \
} while (false)

#define REMOVE_ELEMENT_UNORDERED(ELEMENTS, INDEX) ELEMENTS[INDEX] = ELEMENTS[--ELEMENTS##_length]

void show_message(Editor* editor, const char* message);

// TABS
void move_active_view_to_the_left(Editor* editor);
void move_active_view_to_the_right(Editor* editor);
void move_active_tab_to_the_left(Editor* editor);
void move_active_tab_to_the_right(Editor* editor);

void put_active_view_in_new_tab_to_the_left(Editor* editor);
void put_active_view_in_new_tab_to_the_right(Editor* editor);
void put_active_view_in_tab_to_the_left(Editor* editor);
void put_active_view_in_tab_to_the_right(Editor* editor);

void activate_left_hand_side_view(Editor* editor);
void activate_right_hand_side_view(Editor* editor);

void activate_left_hand_side_tab(Editor* editor);
void activate_right_hand_side_tab(Editor* editor);

void close_active_tab(Editor* editor);

View* find_active_editor_view(const Editor* editor);
View* find_active_tab_view(const Tab* tab);

void go_to(Editor* editor, View* view, Block view_block, bool use_preferred_column, Location absolute_location);

void measure_tabs(Editor * editor);

uint16_t visible_lines_in_tabs(const Editor* editor);

// Buffers
Buffer* lookup_buffer(Buffer_Handle handle);

bool open_buffer(char* path, uint16_t path_length, Buffer_Handle* handle);
void close_buffer(Buffer_Handle handle);
void flush_buffer(Buffer_Handle handle);

bool has_unflushed_changes(Buffer_Handle handle);

void add_change(Buffer* buffer, Change change);
void do_changes(Editor* editor);
void undo_changes(Editor* editor);

uint16_t index_of_last_character(const Line* line);
uint16_t index_of_character_append(const Line* line);

// Blocks
Block intersecting_block(Block a, Block b);
bool contains_block(Block outer, Block inner);
bool is_empty_block(Block block);
void invalidate_block(Editor* editor, Block block);
void invalidate_location(Editor* editor);
Block tab_to_view_block(const Editor* editor, Block tab_view);

// ASCII
bool is_tab_or_space(char c);
bool is_space(char c);
bool is_decimal(char c);

void log(const char* format, ...);

// U16
uint16_t max_u16(uint16_t a, uint16_t b);
uint16_t min_u16(uint16_t a, uint16_t b);