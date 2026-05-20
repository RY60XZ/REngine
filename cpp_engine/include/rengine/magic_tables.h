#ifndef CPP_ENGINE_MAGIC_TABLE_H
#define CPP_ENGINE_MAGIC_TABLE_H
#include "rengine/types.h"
#include <array>

namespace rengine {
    constexpr int BISHOP_INDEX_BITS = 11;
    constexpr int ROOK_INDEX_BITS = 13;

    extern const std::array<Bitboard, 64> rook_masks;
    extern const std::array<Bitboard, 64> bishop_masks;
    extern const std::array<Bitboard, 64> rook_magic;
    extern const std::array<Bitboard, 64> bishop_magic;

    extern const std::array<std::array<Bitboard, 1 << BISHOP_INDEX_BITS>, 64> bishop_attacks;
    extern const std::array<std::array<Bitboard, 1 << ROOK_INDEX_BITS>, 64> rook_attacks;
}

#endif //CPP_ENGINE_MAGIC_TABLE_H