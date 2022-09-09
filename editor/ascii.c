#include "moss.h"

bool is_tab_or_space(char c)
{
    return c == ' ' || c == '\t';
}

bool is_space(char c)
{
    return c >= 9 && c <= 13 ||
           c == ' ';
}

bool is_decimal(char c)
{
    return c >= '0' && c <= '9';
}