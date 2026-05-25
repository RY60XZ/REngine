#include "rengine/movegen.h"
#include "rengine/attack.h"
#include <bit>
namespace rengine {
    namespace {
        constexpr Bitboard FILE_A = 0x0101010101010101ULL;
        constexpr Bitboard FILE_H = 0x8080808080808080ULL;
        constexpr Bitboard RANK_1 = 0x00000000000000FFULL;
        constexpr Bitboard RANK_2 = 0x000000000000FF00ULL;
        constexpr Bitboard RANK_7 = 0x00FF000000000000ULL;
        constexpr Bitboard RANK_8 = 0xFF00000000000000ULL;

        void push_promotion_moves(MoveList& move_list, Square from, Square to, MoveFlag flag) {
            for (int pt = KNIGHT; pt <= QUEEN; ++pt) {
                Move move = encode_move(from, to, flag, static_cast<PieceType>(pt));
                move_list.push_back(move);
            }
        }
    }

    void generate_knight_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard knights = board.pieces[by][KNIGHT];
        while (knights) {
            auto from = pop_lsb(knights);
            Bitboard possible_squares = knight_attacks_from(from) & ~board.occupied[by];
            while (possible_squares) {
                auto to = pop_lsb(possible_squares);
                MoveFlag flag = ((square_bb(to)) & board.occupied[opposite_color(by)])?
                                CAPTURE : QUIET;
                Move move = encode_move(from, to, flag);
                move_list.push_back(move);
            }
        }
    }

    void generate_king_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard kings = board.pieces[by][KING];
        while (kings) {
            auto from = pop_lsb(kings);
            Bitboard possible_squares = king_attacks_from(from) & ~board.occupied[by];
            while (possible_squares) {
                auto to = pop_lsb(possible_squares);
                MoveFlag flag = ((square_bb(to)) & board.occupied[opposite_color(by)])?
                                CAPTURE : QUIET;
                Move move = encode_move(from, to, flag);
                move_list.push_back(move);
            }
        }
    }

    void generate_bishop_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard bishops = board.pieces[by][BISHOP];
        while (bishops) {
            auto from = pop_lsb(bishops);
            Bitboard possible_squares = bishop_attacks_from(from, board.all) & ~board.occupied[by];
            while (possible_squares) {
                auto to = pop_lsb(possible_squares);
                MoveFlag flag = ((square_bb(to)) & board.occupied[opposite_color(by)])?
                                CAPTURE : QUIET;
                Move move = encode_move(from, to, flag);
                move_list.push_back(move);
            }
        }
    }

    void generate_rook_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard rooks = board.pieces[by][ROOK];
        while (rooks) {
            auto from = pop_lsb(rooks);
            Bitboard possible_squares = rook_attacks_from(from, board.all) & ~board.occupied[by];
            while (possible_squares) {
                auto to = pop_lsb(possible_squares);
                MoveFlag flag = ((square_bb(to)) & board.occupied[opposite_color(by)])?
                                CAPTURE : QUIET;
                Move move = encode_move(from, to, flag);
                move_list.push_back(move);
            }
        }
    }

    void generate_queen_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard queens = board.pieces[by][QUEEN];
        while (queens) {
            auto from = pop_lsb(queens);
            Bitboard possible_squares = queen_attacks_from(from, board.all) & ~board.occupied[by];
            while (possible_squares) {
                auto to = pop_lsb(possible_squares);
                MoveFlag flag = ((square_bb(to)) & board.occupied[opposite_color(by)])?
                                CAPTURE : QUIET;
                Move move = encode_move(from, to, flag);
                move_list.push_back(move);
            }
        }
    }

    void generate_pawn_single_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard pawns = board.pieces[by][PAWN];
        Bitboard empty = ~board.all;
        if (by == WHITE) {
            Bitboard targets = ((pawns << 8) & empty) & ~RANK_8;
            while (targets) {
                Square to = pop_lsb(targets);
                Square from = to - 8;
                Move move = encode_move(from, to, QUIET);
                move_list.push_back(move);
            }
        }
        else {
            Bitboard targets = ((pawns >> 8) & empty) & ~RANK_1;
            while (targets) {
                Square to = pop_lsb(targets);
                Square from = to + 8;
                Move move = encode_move(from, to, QUIET);
                move_list.push_back(move);
            }
        }
    }

    void generate_pawn_double_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard pawns = board.pieces[by][PAWN];
        Bitboard empty = ~board.all;
        if (by == WHITE) {
            Bitboard single_pushes = ((pawns & RANK_2) << 8) & empty;
            Bitboard targets = (single_pushes << 8) & empty;
            while (targets) {
                Square to = pop_lsb(targets);
                Square from = to - 16;
                Move move = encode_move(from, to, DOUBLE_PAWN_PUSH);
                move_list.push_back(move);
            }
        }
        else {
            Bitboard single_pushes = ((pawns & RANK_7) >> 8) & empty;
            Bitboard targets = (single_pushes >> 8) & empty;
            while (targets) {
                Square to = pop_lsb(targets);
                Square from = to + 16;
                Move move = encode_move(from, to, DOUBLE_PAWN_PUSH);
                move_list.push_back(move);
            }
        }
    }

    void generate_pawn_capture_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard pawns = board.pieces[by][PAWN];
        Bitboard enemies = board.occupied[opposite_color(by)];
        if (by == WHITE) {
            Bitboard left_captures = ((pawns & ~FILE_A) << 7) & enemies & ~RANK_8;
            while (left_captures) {
                Square to = pop_lsb(left_captures);
                Square from = to - 7;
                Move move = encode_move(from, to, CAPTURE);
                move_list.push_back(move);
            }

            Bitboard right_captures = ((pawns & ~FILE_H) << 9) & enemies & ~RANK_8;
            while (right_captures) {
                Square to = pop_lsb(right_captures);
                Square from = to - 9;
                Move move = encode_move(from, to, CAPTURE);
                move_list.push_back(move);
            }
        }
        else {
            Bitboard left_captures = ((pawns & ~FILE_A) >> 9) & enemies & ~RANK_1;
            while (left_captures) {
                Square to = pop_lsb(left_captures);
                Square from = to + 9;
                Move move = encode_move(from, to, CAPTURE);
                move_list.push_back(move);
            }

            Bitboard right_captures = ((pawns & ~FILE_H) >> 7) & enemies & ~RANK_1;
            while (right_captures) {
                Square to = pop_lsb(right_captures);
                Square from = to + 7;
                Move move = encode_move(from, to, CAPTURE);
                move_list.push_back(move);
            }
        }
    }

    void generate_pawn_promotion_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard pawns = board.pieces[by][PAWN];
        Bitboard empty = ~board.all;
        if (by == WHITE) {
            Bitboard targets = ((pawns & RANK_7) << 8) & empty;
            while (targets) {
                Square to = pop_lsb(targets);
                Square from = to - 8;
                push_promotion_moves(move_list, from, to, PROMOTION);
            }
        }
        else {
            Bitboard targets = ((pawns & RANK_2) >> 8) & empty;
            while (targets) {
                Square to = pop_lsb(targets);
                Square from = to + 8;
                push_promotion_moves(move_list, from, to, PROMOTION);
            }
        }
    }

    void generate_pawn_capture_promotion_moves(const Board& board, MoveList& move_list, Color by) {
        Bitboard pawns = board.pieces[by][PAWN];
        Bitboard enemies = board.occupied[opposite_color(by)];
        if (by == WHITE) {
            Bitboard left_captures = ((pawns & ~FILE_A) << 7) & enemies & RANK_8;
            while (left_captures) {
                Square to = pop_lsb(left_captures);
                Square from = to - 7;
                push_promotion_moves(move_list, from, to, PROMOTION_CAPTURE);
            }

            Bitboard right_captures = ((pawns & ~FILE_H) << 9) & enemies & RANK_8;
            while (right_captures) {
                Square to = pop_lsb(right_captures);
                Square from = to - 9;
                push_promotion_moves(move_list, from, to, PROMOTION_CAPTURE);
            }
        }
        else {
            Bitboard left_captures = ((pawns & ~FILE_A) >> 9) & enemies & RANK_1;
            while (left_captures) {
                Square to = pop_lsb(left_captures);
                Square from = to + 9;
                push_promotion_moves(move_list, from, to, PROMOTION_CAPTURE);
            }

            Bitboard right_captures = ((pawns & ~FILE_H) >> 7) & enemies & RANK_1;
            while (right_captures) {
                Square to = pop_lsb(right_captures);
                Square from = to + 7;
                push_promotion_moves(move_list, from, to, PROMOTION_CAPTURE);
            }
        }
    }

    void generate_pawn_en_passant_moves(const Board &board, MoveList &move_list, Color by) {
        if (board.en_passant_square == INVALID_SQUARE) return;
        Bitboard possible_en_passant_attackers = pawn_attacks_from(board.en_passant_square, opposite_color(by)) & board.pieces[by][PAWN];
        while (possible_en_passant_attackers) {
            auto from = pop_lsb(possible_en_passant_attackers);
            Move move = encode_move(from, board.en_passant_square, EN_PASSANT);
            move_list.push_back(move);
        }
    }

    void generate_pawn_moves(const Board& board, MoveList& move_list, Color by) {
        generate_pawn_single_moves(board, move_list, by);
        generate_pawn_double_moves(board, move_list, by);
        generate_pawn_capture_moves(board, move_list, by);
        generate_pawn_promotion_moves(board, move_list, by);
        generate_pawn_capture_promotion_moves(board, move_list, by);
        generate_pawn_en_passant_moves(board, move_list, by);
    }

    void generate_castling_moves(const Board &board, MoveList &move_list, Color by) {
        const Color enemy = opposite_color(by);
        if (by == WHITE) {
            if (board.castling_rights & WHITE_KINGSIDE_CASTLING) {
                if (board.squares[4] == WHITE_KING && board.squares[7] == WHITE_ROOK &&
                    !(board.all & (square_bb(5) | square_bb(6))) &&
                    !is_square_attacked(board, 4, enemy) &&
                    !is_square_attacked(board, 5, enemy) &&
                    !is_square_attacked(board, 6, enemy)) {
                    Move move = encode_move(4, 6, CASTLING);
                    move_list.push_back(move);
                }
            }
            if (board.castling_rights & WHITE_QUEENSIDE_CASTLING) {
                if (board.squares[4] == WHITE_KING && board.squares[0] == WHITE_ROOK &&
                    !(board.all & (square_bb(1) | square_bb(2) | square_bb(3))) &&
                    !is_square_attacked(board, 4, enemy) &&
                    !is_square_attacked(board, 3, enemy) &&
                    !is_square_attacked(board, 2, enemy)) {
                    Move move = encode_move(4, 2, CASTLING);
                    move_list.push_back(move);
                }
            }
        }
        else {
            if (board.castling_rights & BLACK_KINGSIDE_CASTLING) {
                if (board.squares[60] == BLACK_KING && board.squares[63] == BLACK_ROOK &&
                    !(board.all & (square_bb(61) | square_bb(62))) &&
                    !is_square_attacked(board, 60, enemy) &&
                    !is_square_attacked(board, 61, enemy) &&
                    !is_square_attacked(board, 62, enemy)) {
                    Move move = encode_move(60, 62, CASTLING);
                    move_list.push_back(move);
                }
            }
            if (board.castling_rights & BLACK_QUEENSIDE_CASTLING) {
                if (board.squares[60] == BLACK_KING && board.squares[56] == BLACK_ROOK &&
                    !(board.all & (square_bb(57) | square_bb(58) | square_bb(59))) &&
                    !is_square_attacked(board, 60, enemy) &&
                    !is_square_attacked(board, 59, enemy) &&
                    !is_square_attacked(board, 58, enemy)) {
                    Move move = encode_move(60, 58, CASTLING);
                    move_list.push_back(move);
                }
            }
        }
    }

    void generate_pseudo_legal_moves(const Board &board, MoveList &move_list, Color by) {
        move_list.clear();
        generate_knight_moves(board, move_list, by);
        generate_bishop_moves(board, move_list, by);
        generate_rook_moves(board, move_list, by);
        generate_queen_moves(board, move_list, by);
        generate_king_moves(board, move_list, by);
        generate_pawn_moves(board, move_list, by);
        generate_castling_moves(board, move_list, by);
    }


}
