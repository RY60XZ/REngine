#include"rengine/move_ordering.h"
#include<algorithm>
#include<cassert>

namespace rengine {
    namespace {
        constexpr MoveScore PREFERRED_MOVE_SCORE = 1'000'000;
        constexpr MoveScore PROMOTION_SCORE_BASE = 40'000;
        constexpr MoveScore CAPTURE_SCORE_BASE = 20'000;
        constexpr MoveScore MVV_LVA_VICTIM_SCALE = 16;

        PieceType captured_piece_type(const Board& board, Move move) {
            if (is_en_passant(move)) {
                return PAWN;
            }

            Piece captured_piece = board.squares[move_to(move)];
            assert(captured_piece != NO_PIECE);
            return piece_type_of(captured_piece);
        }
    }

    MoveScore piece_order_value(PieceType piece_type) {
        switch (piece_type) {
            case PAWN: return 100;
            case KNIGHT: return 320;
            case BISHOP: return 330;
            case ROOK: return 500;
            case QUEEN: return 900;
            case KING: return 0;
        }
        return 0;
    }

    bool is_noisy_move(Move move) {
        return is_capture(move) || is_promotion(move);
    }

    MoveScore move_order_score(const Board& board, Move move, Move preferred_move) {
        if (preferred_move != 0 && move == preferred_move) {
            return PREFERRED_MOVE_SCORE;
        }

        MoveScore score = QUIET_MOVE_SCORE;
        if (is_capture(move)) {
            Piece attacker = board.squares[move_from(move)];
            assert(attacker != NO_PIECE);

            score += CAPTURE_SCORE_BASE;
            score += piece_order_value(captured_piece_type(board, move)) * MVV_LVA_VICTIM_SCALE;
            score -= piece_order_value(piece_type_of(attacker));
        }

        if (is_promotion(move)) {
            score += PROMOTION_SCORE_BASE;
            score += piece_order_value(move_promotion_type(move));
        }

        return score;
    }

    Move select_best_move(const Board& board, MoveList& moves, int start_index, Move preferred_move) {
        assert(start_index >= 0 && start_index < moves.size());

        int best_index = start_index;
        MoveScore best_score = move_order_score(board, moves[start_index], preferred_move);
        for (int index = start_index + 1; index < moves.size(); ++index) {
            MoveScore score = move_order_score(board, moves[index], preferred_move);
            if (score > best_score) {
                best_score = score;
                best_index = index;
            }
        }

        if (best_index != start_index) {
            std::swap(moves.moves[start_index], moves.moves[best_index]);
        }
        return moves[start_index];
    }
}
