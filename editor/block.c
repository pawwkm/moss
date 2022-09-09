#include "moss.h"

Block intersecting_block(Block a, Block b)
{
    uint16_t x1 = max_u16(a.x, b.x);
    uint16_t x2 = min_u16(a.x + a.width, b.x + b.width);
    uint16_t y1 = max_u16(a.y, b.y);
    uint16_t y2 = min_u16(a.y + a.height, b.y + b.height);
    
    if (x2 >= x1 && y2 >= y1)
    {
        return (Block)
        {
            .x = x1,
            .y = y1,
            .width = x2 - x1,
            .height = y2 - y1
        };
    }

    return (Block) { 0 };
}

bool contains_block(Block outer, Block inner)
{
    return outer.x <= inner.x &&
           outer.y <= inner.y &&
           outer.x + outer.width  >= inner.x + inner.width &&
           outer.y + outer.height >= inner.y + inner.height;
}

bool is_empty_block(Block block)
{
    return !block.width || !block.height;
}
