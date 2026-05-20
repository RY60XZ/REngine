#ifndef CPP_ENGINE_ATTACK_H
#define CPP_ENGINE_ATTACK_H
#include "rengine/board.h"
#include "rengine/square.h"
#include<array>
#include<cassert>
#include<cstddef>

namespace rengine {
    namespace detail {
        struct Offset {
            int file;
            int rank;
        };

        constexpr bool is_on_board(int file, int rank) {
            return file >= 0 && file < 8 && rank >= 0 && rank < 8;
        }

        constexpr Bitboard square_bb_if_valid(int file, int rank) {
            return is_on_board(file, rank)
                ? square_bb(make_square(file, rank))
                : Bitboard{0};
        }

        template<std::size_t N>
        constexpr Bitboard attacks_from_offsets(Square square, const std::array<Offset, N>& offsets) {
            assert(square < 64);

            const int file = file_of(square);
            const int rank = rank_of(square);
            auto attacks = Bitboard{0};

            for (const Offset offset : offsets) {
                attacks |= square_bb_if_valid(file + offset.file, rank + offset.rank);
            }

            return attacks;
        }

        inline constexpr std::array<Offset, 8> king_offsets{{
            {-1, -1}, {0, -1}, {1, -1},
            {-1,  0},          {1,  0},
            {-1,  1}, {0,  1}, {1,  1},
        }};

        inline constexpr std::array<Offset, 8> knight_offsets{{
            {-2, -1}, {-2,  1},
            {-1, -2}, {-1,  2},
            { 1, -2}, { 1,  2},
            { 2, -1}, { 2,  1},
        }};

        inline constexpr std::array<Offset, 2> white_pawn_offsets{{
            {-1, 1}, {1, 1},
        }};

        inline constexpr std::array<Offset, 2> black_pawn_offsets{{
            {-1, -1}, {1, -1},
        }};
    }

    constexpr Bitboard king_attacks_from(Square square) {
        return detail::attacks_from_offsets(square, detail::king_offsets);
    }

    constexpr Bitboard knight_attacks_from(Square square) {
        return detail::attacks_from_offsets(square, detail::knight_offsets);
    }

    constexpr Bitboard pawn_attacks_from(Color color, Square square) {
        return color == WHITE
            ? detail::attacks_from_offsets(square, detail::white_pawn_offsets)
            : detail::attacks_from_offsets(square, detail::black_pawn_offsets);
    }

    constexpr std::array<Bitboard, 64> make_king_attacks() {
        std::array<Bitboard, 64> table{};
        for (Square square = 0; square < 64; ++square) {
            table[square] = king_attacks_from(square);
        }
        return table;
    }

    constexpr std::array<Bitboard, 64> make_knight_attacks() {
        std::array<Bitboard, 64> table{};
        for (Square square = 0; square < 64; ++square) {
            table[square] = knight_attacks_from(square);
        }
        return table;
    }

    constexpr std::array<std::array<Bitboard, 64>, 2> make_pawn_attacks() {
        std::array<std::array<Bitboard, 64>, 2> table{};
        for (int color = 0; color<2; ++color) {
            for (Square square = 0; square<64; ++square) {
                table[color][square] = pawn_attacks_from(static_cast<Color>(color), square);
            }
        }
        return table;
    }

    inline constexpr auto king_attacks = make_king_attacks();
    inline constexpr auto knight_attacks = make_knight_attacks();
    inline constexpr auto pawn_attacks = make_pawn_attacks();
}
#endif //CPP_ENGINE_ATTACK_H
