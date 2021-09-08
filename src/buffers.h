#pragma once

#include "moss.h"

Line* add_line(Buffer* buffer);
Token* add_token(Line* line);

Buffer* buffer_handle_to_pointer(Buffer_Handle handle);

bool open_buffer(char* path, uint16_t path_length, Buffer_Handle* handle);

void flush_buffer(Buffer_Handle handle);

bool has_unflushed_changes(Buffer_Handle handle);

bool initialize_lexer(void);
void lexical_analyze(Language language, Line* line, bool* continue_multiline_comment);
