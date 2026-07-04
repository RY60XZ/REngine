#ifndef CPP_ENGINE_ZOBRIST_H
#define CPP_ENGINE_ZOBRIST_H

#include <cstdint>
#include "rengine/board.h"

namespace rengine {

    class SplitMix64 {
    public:
        explicit constexpr SplitMix64(std::uint64_t seed) : state_(seed) {}

        constexpr std::uint64_t next() {
            std::uint64_t value = state_ += 0x9E3779B97F4A7C15ULL;
            value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
            value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
            return value ^ (value >> 31);
        }

        constexpr std::uint64_t state() const {
            return state_;
        }

    private:
        std::uint64_t state_;
    };

    struct ZobristTable {
        ZobristKey piece_square[2][6][64];
        ZobristKey castling[16];
        ZobristKey en_passant_file[8];
        ZobristKey side_to_move;
    };

    ZobristKey compute_zobrist_key(const Board& board);
    void update_zobrist_key(Board& board);
    extern const ZobristTable ZOBRIST_TABLE;

}
#endif //CPP_ENGINE_ZOBRIST_H
