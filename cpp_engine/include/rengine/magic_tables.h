#ifndef CPP_ENGINE_MAGIC_TABLE_H
#define CPP_ENGINE_MAGIC_TABLE_H
#include "rengine/types.h"
#include <array>
#include <cstddef>

namespace rengine {
    struct MagicEntry {
        Bitboard mask;
        Bitboard magic;
        int index_bits;
        std::size_t offset;
    };

    constexpr std::size_t ROOK_ATTACK_TABLE_SIZE = 102400;
    constexpr std::size_t BISHOP_ATTACK_TABLE_SIZE = 5248;

    extern const std::array<MagicEntry, 64> rook_magic_entries;
    extern const std::array<MagicEntry, 64> bishop_magic_entries;
    extern const std::array<Bitboard, ROOK_ATTACK_TABLE_SIZE> rook_attacks;
    extern const std::array<Bitboard, BISHOP_ATTACK_TABLE_SIZE> bishop_attacks;

    inline std::size_t magic_index(Bitboard occupancy, const MagicEntry& entry) {
        Bitboard blockers = occupancy & entry.mask;
        return static_cast<std::size_t>((blockers * entry.magic) >> (64 - entry.index_bits));
    }
}

#endif //CPP_ENGINE_MAGIC_TABLE_H
