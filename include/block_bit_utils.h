#ifndef BLOCK_BIT_UTILS_H
#define BLOCK_BIT_UTILS_H

#include <stdbool.h>
#include <stdint.h>

typedef uint16_t BlockBits;

static inline bool block_has_flag(BlockBits value, BlockBits flag)
{
    return (value & flag) == flag;
}

static inline bool block_has_any_flag(BlockBits value, BlockBits flags)
{
    return (value & flags) != 0;
}

static inline void block_set_flag(BlockBits *value, BlockBits flag)
{
    if (value == NULL) {
        return;
    }
    *value |= flag;
}

static inline void block_clear_flag(BlockBits *value, BlockBits flag)
{
    if (value == NULL) {
        return;
    }
    *value &= (BlockBits)~flag;
}

#endif
