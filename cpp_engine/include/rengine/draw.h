#ifndef CPP_ENGINE_DRAW_H
#define CPP_ENGINE_DRAW_H
#include"rengine/board.h"
#include<algorithm>
#include<bit>

namespace rengine {
    inline bool is_fifty_move_rule_draw(const Board& board) {
        return board.half_move_clock>=100;
    }

    constexpr int square_color(Square square) {
        return static_cast<int>(((square & 7U) + (square >> 3U)) & 1U);
    }

    inline bool is_insufficient_material_draw(const Board& board) {
        Bitboard pawns_rooks_queens =
            board.pieces[WHITE][PAWN] | board.pieces[BLACK][PAWN] |
            board.pieces[WHITE][ROOK] | board.pieces[BLACK][ROOK] |
            board.pieces[WHITE][QUEEN] | board.pieces[BLACK][QUEEN];
        if (pawns_rooks_queens != 0) {
            return false;
        }

        Bitboard knights = board.pieces[WHITE][KNIGHT] | board.pieces[BLACK][KNIGHT];
        Bitboard bishops = board.pieces[WHITE][BISHOP] | board.pieces[BLACK][BISHOP];
        int minor_count = std::popcount(knights | bishops);

        if (minor_count <= 1) {
            return true;
        }
        if (knights != 0) {
            return false;
        }

        int bishop_color = -1;
        while (bishops != 0) {
            Square square = static_cast<Square>(std::countr_zero(bishops));
            int color = square_color(square);
            if (bishop_color == -1) {
                bishop_color = color;
            }
            else if (bishop_color != color) {
                return false;
            }
            bishops &= bishops - 1;
        }
        return true;
    }
    inline bool is_threefold_draw(const Board& board) {
        if (board.position_history_size < 5 || board.half_move_clock < 4) {
            return false;
        }

        int repetitions = 0;
        int latest_index = board.position_history_size - 1;
        int max_back = std::min(board.half_move_clock, latest_index);
        for (int offset = 0; offset <= max_back; offset += 2) {
            if (board.position_history[latest_index - offset] == board.zobrist_key) {
                ++repetitions;
                if (repetitions >= 3) {
                    return true;
                }
            }
        }
        return false;
    }
}
#endif //CPP_ENGINE_DRAW_H
