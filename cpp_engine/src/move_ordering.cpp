#include"rengine/move_ordering.h"
#include<algorithm>
#include<cassert>

namespace rengine {
    namespace {
        constexpr MoveScore PREFERRED_MOVE_SCORE = 1'000'000;
        constexpr MoveScore PROMOTION_SCORE_BASE = 40'000;
        constexpr MoveScore CAPTURE_SCORE_BASE = 20'000;
        constexpr MoveScore KILLER_MOVE_SCORE = 10'000;
        constexpr MoveScore KILLER_SLOT_PENALTY = 500;
        constexpr MoveScore HISTORY_SCORE_MAX = 8'000;
        constexpr MoveScore MVV_LVA_VICTIM_SCALE = 16;
        constexpr std::int32_t HISTORY_VALUE_MAX = 1'000'000;

        PieceType captured_piece_type(const Board& board, Move move) {
            if (is_en_passant(move)) {
                return PAWN;
            }

            Piece captured_piece = board.squares[move_to(move)];
            assert(captured_piece != NO_PIECE);
            return piece_type_of(captured_piece);
        }

        bool is_valid_ply(int ply) {
            return ply >= 0 && ply < MAX_PLY;
        }

        MoveScore tactical_score(const Board& board, Move move) {
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

    bool is_quiet_move(Move move) {
        return move != 0 && !is_noisy_move(move);
    }

    int killer_slot_for_move(const SearchStack& stack, int ply, Move move) {
        if (!is_quiet_move(move) || !is_valid_ply(ply)) {
            return -1;
        }

        for (int slot = 0; slot < KILLER_MOVE_SLOTS; ++slot) {
            if (stack.killer_moves[ply][slot] == move) {
                return slot;
            }
        }
        return -1;
    }

    bool store_killer_move(SearchStack& stack, int ply, Move move) {
        if (!is_quiet_move(move) || !is_valid_ply(ply)) {
            return false;
        }

        if (stack.killer_moves[ply][0] == move) {
            return false;
        }
        if (stack.killer_moves[ply][1] == move) {
            std::swap(stack.killer_moves[ply][0], stack.killer_moves[ply][1]);
            return false;
        }

        stack.killer_moves[ply][1] = stack.killer_moves[ply][0];
        stack.killer_moves[ply][0] = move;
        return true;
    }

    MoveScore history_score(const SearchStack& stack, Color color, Move move) {
        if (!is_quiet_move(move)) {
            return 0;
        }

        MoveScore score = stack.history[color][move_from(move)][move_to(move)];
        return std::min(score, HISTORY_SCORE_MAX);
    }

    void update_history(SearchStack& stack, Color color, Move move, int depth) {
        if (!is_quiet_move(move)) {
            return;
        }
        std::int32_t bonus = depth * depth;
        std::int32_t& entry = stack.history[color][move_from(move)][move_to(move)];
        entry = std::min(HISTORY_VALUE_MAX, entry + bonus);
    }

    MoveScore move_order_score(const Board& board, Move move, Move preferred_move) {
        if (preferred_move != 0 && move == preferred_move) {
            return PREFERRED_MOVE_SCORE;
        }

        return tactical_score(board, move);
    }

    MoveScore move_order_score(const Board& board, Move move, const SearchStack& stack,
                               int ply, Move preferred_move, SearchStats* stats) {
        if (preferred_move != 0 && move == preferred_move) {
            return PREFERRED_MOVE_SCORE;
        }

        MoveScore score = tactical_score(board, move);
        if (!is_quiet_move(move)) {
            return score;
        }

        ++stats->killer_probes;

        int killer_slot = killer_slot_for_move(stack, ply, move);
        if (killer_slot >= 0) {
            ++stats->killer_hits;
            score += KILLER_MOVE_SCORE - killer_slot * KILLER_SLOT_PENALTY;
        }

        score += history_score(stack, board.side_to_move, move);
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

    Move select_best_move(const Board& board, MoveList& moves, int start_index,
                          const SearchStack& stack, int ply, Move preferred_move,
                          SearchStats* stats) {
        assert(start_index >= 0 && start_index < moves.size());

        int best_index = start_index;
        MoveScore best_score = move_order_score(board, moves[start_index], stack, ply, preferred_move, stats);
        for (int index = start_index + 1; index < moves.size(); ++index) {
            MoveScore score = move_order_score(board, moves[index], stack, ply, preferred_move, stats);
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
