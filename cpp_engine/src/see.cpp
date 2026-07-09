#include "rengine/see.h"
#include "rengine/attack.h"
#include <algorithm>
#include <array>
#include <bit>

namespace rengine {
    namespace {
        constexpr std::array<Score, 6> SEE_PIECE_VALUES = {
            100,
            320,
            330,
            500,
            900,
            0,
        };

        struct Attacker {
            Square square = INVALID_SQUARE;
            PieceType type = PAWN;
        };

        Score piece_value(PieceType piece_type) {
            return SEE_PIECE_VALUES[piece_type];
        }

        Bitboard attackers_to(const Board& board, Square target, Bitboard occupancy, Color color) {
            Bitboard pieces = board.occupied[color] & occupancy;
            Bitboard attackers = 0;
            attackers |= pawn_attacks_from(target, opposite_color(color)) &
                         board.pieces[color][PAWN];
            attackers |= knight_attacks_from(target) & board.pieces[color][KNIGHT];
            attackers |= king_attacks_from(target) & board.pieces[color][KING];
            attackers |= bishop_attacks_from(target, occupancy) &
                         (board.pieces[color][BISHOP] | board.pieces[color][QUEEN]);
            attackers |= rook_attacks_from(target, occupancy) &
                         (board.pieces[color][ROOK] | board.pieces[color][QUEEN]);
            return attackers & pieces;
        }

        bool least_valuable_attacker(const Board& board, Square target, Bitboard occupancy,
                                     Color color, Attacker& attacker) {
            Bitboard attackers = attackers_to(board, target, occupancy, color);
            for (int piece_type = PAWN; piece_type <= KING; ++piece_type) {
                Bitboard candidates = attackers & board.pieces[color][piece_type] & occupancy;
                if (candidates != 0) {
                    attacker.square = static_cast<Square>(std::countr_zero(candidates));
                    attacker.type = static_cast<PieceType>(piece_type);
                    return true;
                }
            }
            return false;
        }

        Score captured_value(const Board& board, Move move) {
            if (is_en_passant(move)) {
                return piece_value(PAWN);
            }
            Piece captured = board.squares[move_to(move)];
            return captured == NO_PIECE ? 0 : piece_value(piece_type_of(captured));
        }

        Bitboard occupancy_after_first_capture(const Board& board, Move move, Color moving_color) {
            Square from = move_from(move);
            Square to = move_to(move);
            Bitboard occupancy = board.all;
            occupancy &= ~square_bb(from);

            if (is_en_passant(move)) {
                Square captured_square = moving_color == WHITE ? to - 8 : to + 8;
                occupancy &= ~square_bb(captured_square);
                occupancy |= square_bb(to);
            }

            return occupancy;
        }
    }

    Score see_score(const Board& board, Move move) {
        Square from = move_from(move);
        Square to = move_to(move);
        Piece moving_piece = board.squares[from];
        if (moving_piece == NO_PIECE) {
            return 0;
        }

        Color moving_color = color_of(moving_piece);
        PieceType occupied_target = piece_type_of(moving_piece);
        Score promotion_gain = 0;
        if (is_promotion(move)) {
            occupied_target = move_promotion_type(move);
            promotion_gain = piece_value(occupied_target) - piece_value(PAWN);
        }

        std::array<Score, 32> gain{};
        int depth = 0;
        gain[0] = captured_value(board, move) + promotion_gain;

        Bitboard occupancy = occupancy_after_first_capture(board, move, moving_color);
        Color side = opposite_color(moving_color);

        while (depth + 1 < static_cast<int>(gain.size())) {
            Attacker attacker{};
            if (!least_valuable_attacker(board, to, occupancy, side, attacker)) {
                break;
            }

            Bitboard attacker_square = square_bb(attacker.square);
            Bitboard occupancy_after_capture = occupancy & ~attacker_square;

            if (attacker.type == KING &&
                attackers_to(board, to, occupancy_after_capture, opposite_color(side)) != 0) {
                break;
            }

            ++depth;
            gain[depth] = piece_value(occupied_target) - gain[depth - 1];
            occupied_target = attacker.type;
            occupancy = occupancy_after_capture;
            side = opposite_color(side);
        }

        while (depth > 0) {
            --depth;
            gain[depth] = -std::max(-gain[depth], gain[depth + 1]);
        }

        return gain[0];
    }

    bool see_ge(const Board& board, Move move, Score threshold) {
        return see_score(board, move) >= threshold;
    }
}
